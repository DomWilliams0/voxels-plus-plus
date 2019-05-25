#include <boost/container/vector.hpp>
#include "error.h"
#include "util.h"
#include "loader.h"
#include "config.h"
#include "world.h"

static void update_face_visibility(ChunkTerrain &terrain) {
    glm::ivec3 pos;
    for (int i = 0; i < kBlocksPerChunk; ++i) {
        Chunk::expand_block_index(terrain, i, pos);
        Block &b = terrain[i];
        FaceVisibility visibility = b.face_visibility_;

        if (!BlockType_opaque(b.type_)) {
            // fully visible because transparent
            visibility = kFaceVisibilityAll;
        } else {
            // check each face individually
            glm::ivec3 offset_pos;
            for (int j = 0; j < kFaceCount; ++j) {
                offset_pos = pos; // reset
                Face face = kFaces[j];
                face_offset(face, offset_pos);

                // looks outside of the world, so this face is visible
                // TODO YEAH?!
                if (offset_pos.y < 0 || offset_pos.y >= kChunkHeight) {
                    visibility |= face_visibility(face);
                    continue;
                }

                // steps over chunk boundary
                if (offset_pos.x < 0 || offset_pos.x >= kChunkWidth ||
                    offset_pos.z < 0 || offset_pos.z >= kChunkDepth) {
                    // TODO actually get it
                    // edge, never mind
                    visibility |= face_visibility(face);
                    continue;
                }

                // inside this chunk, safe to get the block type (rather ugly...)
                Block &offset_block = terrain[{static_cast<unsigned long>(offset_pos.x),
                                               static_cast<unsigned long>(offset_pos.y),
                                               static_cast<unsigned long>(offset_pos.z)}];

                if (BlockType_opaque(offset_block.type_))
                    visibility &= ~face_visibility(face); // not visible
                else
                    visibility |= face_visibility(face); // visible
            }
        }

        b.face_visibility_ = visibility;
    }

}

WorldLoader::WorldLoader(int seed) : seed_(seed),
                                     internal_terrain_complete_(32), garbage_(32),
                                     all_terrain_complete_(32), mesh_complete_(32),
                                     pool_(config::kTerrainThreadWorkers),
                                     loaded_chunk_radius_(config::kInitialLoadedChunkRadius),
                                     mesh_pool_(loaded_chunk_radius_chunk_count() * 2), // large buffer
                                     chunk_pool_(loaded_chunk_radius_chunk_count()) {
}


void WorldLoader::request_chunk(ChunkId_t chunk_id) {
    // TODO decide here if we are loading from cache, so we only alloc a new chunk if necessary
    // don't want to allocate from memory pool from worker thread
    int x, z;
    ChunkId_deconstruct(chunk_id, x, z);

    ChunkMeshRaw *mesh = mesh_pool_.construct();
    Chunk *chunk = chunk_pool_.construct(x, z, mesh);
    DLOG_F(INFO, "allocating new chunk(%d, %d)", x, z);
    chunks_.set_state(chunk, ChunkState::kLoading);

    pool_.post([this, chunk_id, mesh, chunk, x, z]() {
//        DLOG_F(INFO, "about to load chunk(%d, %d)", x, z);

        // TODO load from cache/disk too
        // TODO delete when?
        thread_local IGenerator *gen = config::new_generator();

        int ret = gen->generate(chunk_id, seed_, chunk->terrain_);
        if (ret == kErrorSuccess) {
            // finalise terrain
            // TODO might already be populated, check first
            update_face_visibility(chunk->terrain_);

            if (internal_terrain_complete_.push(chunk))
                return; // success

            LOG_F(WARNING, "failed to push to internal_terrain_complete_");
        }

        LOG_F(WARNING, "failed to generate chunk(%d, %d) with seed %d: %d", x, z, seed_, ret);
        unload_chunk(chunk, false);
    });

}


void WorldLoader::unload_chunk(Chunk *chunk, bool allow_cache) {
    if (chunk == nullptr)
        return;

    // TODO add to cache if param set
    garbage_.push(chunk);
}


int WorldLoader::loaded_chunk_radius_chunk_count() const {
    return (2 * loaded_chunk_radius_ + 1) * (2 * loaded_chunk_radius_ + 1);
}

void WorldLoader::tweak_loaded_chunk_radius(int delta) {
    loaded_chunk_radius_ += delta;

    if (loaded_chunk_radius_ < 1)
        loaded_chunk_radius_ = 1;

    else
        LOG_F(INFO, "%s loaded chunk radius to %d", delta > 0 ? "bumped" : "reduced", loaded_chunk_radius_);
}

void WorldLoader::unload_all_chunks() {
    for (auto &entry : chunks_) {
        Chunk *chunk = entry.second.chunk_;
        unload_chunk(chunk);
    }
    chunks_.clear();
}

/*static bool is_chunk_out_of_range(ChunkId_t chunk, ChunkId_t centre, int load_radius) {
    int cx, cy, x, y;
    ChunkId_deconstruct(centre, cx, cy);
    ChunkId_deconstruct(chunk, x, y);

    return World::is_in_loaded_range(cx, cy, load_radius, x, y);
}*/

void WorldLoader::tick(ChunkId_t world_centre) {
    LOG_F(INFO, "------");

    // update out of range mask for new world centre
    int centre_x, centre_z;
    ChunkId_deconstruct(world_centre, centre_x, centre_z);
    for (auto &e : chunkmap()) {
        Chunk *chunk = e.second.chunk_;
        if (chunk != nullptr) {
            int x, z;
            ChunkId_deconstruct(e.first, x, z);
            chunk->neighbour_mask_.update_load_range(x, z, centre_x, centre_z, loaded_chunk_radius_);
        }
    }
    // move chunks through pipeline
    Chunk *chunk = nullptr;
    ChunkNeighbours neighbours;
    boost::container::vector<Chunk *> retry;
    retry.reserve(32);

    // merge terrain
    while (internal_terrain_complete_.pop(chunk)) {
        chunks_.set_state(chunk, ChunkState::kLoadedIsolatedTerrain);

        DLOG_F(INFO, "doing %s", ChunkId_str(chunk->id()).c_str());

        // TODO if (update neighbours) {post neighbour merges to queue}
        int neighbour_mask = chunk->neighbour_mask_.mask();
        chunk->neighbours(neighbours);
        for (int i = 0; i < kChunkNeighbourCount; ++i) {
            ChunkId_t n_id = neighbours[i];
            ChunkNeighbour n = kChunkNeighbourValues[i];

            DLOG_F(INFO, "neighbour %d: %s", n, ChunkId_str(n_id).c_str());

            // one way comparison only
            if (n_id < chunk->id())
                continue;

            // already merged or neighbour is out of range anyway
            DLOG_F(INFO, "mask (%d) & n (%d) == %d", neighbour_mask, n, (neighbour_mask|(unsigned int)n));
            if (neighbour_mask & (unsigned int) n)
                continue;

            // get neighbour load state
            ChunkMap::Entry e;
            chunks_.find_chunk(n_id, e);

            switch (e.state_) {
                case ChunkState::kLoadedAllTerrain:
                case ChunkState::kRenderable:
                case ChunkState::kUnloaded:
                    // should have been caught be previous checks
                    assert(false);
                    continue;

                case ChunkState::kLoading:
                    // come back to this pair later
                    retry.push_back(chunk);
                    DLOG_F(INFO, "retrying %lu because n %lu state is %d", chunk->id(), n_id, e.state_);
                    goto next_chunk;

                case ChunkState::kLoadedIsolatedTerrain:
                    // TODO push neighbour merge task
                    break;
            }
        }
        next_chunk:;
    }
    if (!retry.empty())
        internal_terrain_complete_.push(retry.cbegin(), retry.cend());
    retry.clear();


//    Chunk *done_chunk = nullptr;
//    while (internal_terrain_complete_.pop(done_chunk)) {
//        int x, z;
//        ChunkId_deconstruct(done_chunk->id(), x, z);
//        assert(done_chunk->loaded());
//
//        chunks_.set_state(done_chunk, ChunkState::kRenderable);
//    }

    // free unloaded chunks
    garbage_.consume_all([this](Chunk *c) {
        ChunkMeshRaw *mesh = c->steal_mesh();
        DLOG_F(INFO, "deallocating mesh %p", mesh);
        if (mesh != nullptr)
            mesh_pool_.destroy(mesh);
        chunk_pool_.destroy(c);
    });

}

bool ChunkMap::RenderableChunkIterator::next(Chunk **out) {
    while (iterator_ != end_) {
        auto &pair = *(iterator_++);

        // not renderable
        if (!ChunkState_renderable(pair.second.state_))
            continue;

        Chunk *chunk = pair.second.chunk_;

        // return this chunk
        *out = chunk;
        return true;
    }

    // all done
    return false;
}


void ChunkMap::find_chunk(ChunkId_t chunk_id, Entry &out) const {
    auto it = map_.find(chunk_id);

    if (it == map_.end()) {
        out.state_ = ChunkState::kUnloaded;
    } else {
        out = it->second;
    }
}

void ChunkMap::set_state(Chunk *chunk, ChunkState state) {
    ChunkState prev = ChunkState ::kUnloaded;

    if (state == ChunkState::kUnloaded)
        map_.erase(chunk->id());
    else {
        Entry e = {.chunk_=chunk, .state_ = state};
        auto ret = map_.insert(std::make_pair(chunk->id(), e));
        if (!ret.second) {
            // already existed
            assert(ret.first->second.chunk_ == chunk);
            prev = ret.first->second.state_;

            // actually write
            map_[chunk->id()] = e;
        }
    }

    if (prev != state) {
        int x, z;
        ChunkId_deconstruct(chunk->id(), x, z);
        DLOG_F(INFO, "set chunk(%d, %d) state to %d from %d", x, z, state, prev);
    }
}

