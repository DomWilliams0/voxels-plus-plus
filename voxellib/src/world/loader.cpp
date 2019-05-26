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

                // facing top/bottom of world, so this face is visible
                if (offset_pos.y < 0 || offset_pos.y >= kChunkHeight) {
                    visibility |= face_visibility(face);
                    continue;
                }

                // faces chunk boundary, will be set later
                if (offset_pos.x < 0 || offset_pos.x >= kChunkWidth ||
                    offset_pos.z < 0 || offset_pos.z >= kChunkDepth) {
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
                                     pool_(config::kTerrainThreadWorkers),
                                     loaded_chunk_radius_(config::kInitialLoadedChunkRadius),
                                     mesh_pool_(loaded_chunk_radius_chunk_count() * 2), // large buffer
                                     chunk_pool_(loaded_chunk_radius_chunk_count()) {
}


void WorldLoader::request_chunk(ChunkId_t chunk_id, ChunkId_t centre_chunk) {
    // TODO decide here if we are loading from cache, so we only alloc a new chunk if necessary
    // don't want to allocate from memory pool from worker thread
    int x, z;
    ChunkId_deconstruct(chunk_id, x, z);
/*

    ChunkMeshRaw *mesh = mesh_pool_.construct();
    Chunk *chunk = chunk_pool_.construct(x, z, mesh);
    DLOG_F(INFO, "allocating new chunk(%d, %d)", x, z);
    chunks_.set_state(chunk, ChunkState::kLoading);

    int cx, cz;
    ChunkId_deconstruct(centre_chunk, cx, cz);
    chunk->neighbour_mask_.update_load_range(x, z, cx, cz, loaded_chunk_radius_);

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
*/

}


void WorldLoader::unload_chunk(Chunk *chunk, bool allow_cache) {
    if (chunk == nullptr)
        return;

    // TODO add to cache if param set
//    garbage_.push(chunk);
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
/*    for (auto &entry : chunks_) {
        Chunk *chunk = entry.second.chunk_;
        unload_chunk(chunk);
    }
    chunks_.clear();*/
}

void WorldLoader::tick() {
}


bool ChunkMap::RenderableChunkIterator::next(Chunk **out) {
    while (iterator_ != end_) {
        auto &pair = *(iterator_++);

        // not renderable
//        if (!ChunkState_renderable(pair.second.state_))
//            continue;

        Chunk *chunk = pair.second;

        // return this chunk
        *out = chunk;
        return true;
    }

    // all done
    return false;
}
