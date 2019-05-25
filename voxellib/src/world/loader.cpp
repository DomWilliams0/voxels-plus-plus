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

WorldLoader::WorldLoader(int seed) : seed_(seed), complete_merge_jobs_(32),
                                     internal_terrain_complete_(32), garbage_(32),
                                     all_terrain_complete_(32), mesh_complete_(32),
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

void WorldLoader::tick(ChunkId_t world_centre) {
    chunks_.log_debug_summary();

    // move chunks through pipeline
    Chunk *chunk = nullptr;
    ChunkNeighbours neighbours;
    boost::container::vector<Chunk *> retry;
    retry.reserve(32);

    // merge terrain
    while (internal_terrain_complete_.pop(chunk)) {
        // check for chunks that completed while sitting in the retry queue
        if (chunks_.get_state(chunk) > ChunkState::kLoadedIsolatedTerrain) {
//            DLOG_F(WARNING, "apparently chunk %s needs a retry even though it is state %d", CHUNKSTR(chunk), e.state_);
            continue;
        }

        chunks_.set_state(chunk, ChunkState::kLoadedIsolatedTerrain);

        DLOG_F(INFO, "doing %s", ChunkId_str(chunk->id()).c_str());

        // TODO if (update neighbours) {post neighbour merges to queue}
        unsigned int neighbour_mask = chunk->neighbour_mask_.mask();
        chunk->neighbours(neighbours);
        for (int n = 0; n < kChunkNeighbourCount; ++n) {
            ChunkId_t n_id = neighbours[n];
            unsigned int n_mask = 1 << n;

            DLOG_F(INFO, "neighbour %d: %s", n, ChunkId_str(n_id).c_str());

            // one way comparison only
            // TODO check for this pair in job queue instead
            // if (n_id < chunk->id())
            //      continue;

            // already merged or neighbour is out of range anyway
            DLOG_F(INFO, "mask (%d) & n (%d) == %d", neighbour_mask, n, (neighbour_mask & n_mask));
            if (neighbour_mask & n_mask)
                continue;

            // get neighbour load state
            ChunkMap::Entry e;
            chunks_.find_chunk(n_id, e);

            switch (e.state_) {
                case ChunkState::kUnloaded:
                    // should have been caught be previous checks
                    LOG_F(ERROR, "neighbour chunk %s is unloaded!", ChunkId_str(n_id).c_str());
                    continue;

                case ChunkState::kLoading:
                    // come back to this pair later
                    retry.push_back(chunk);
                    DLOG_F(INFO, "retrying %s because n %s state is %s",
                           ChunkId_str(chunk->id()).c_str(), ChunkId_str(n_id).c_str(),
                           ChunkState_str[(int)e.state_].c_str());
                    goto next_chunk;

                case ChunkState::kLoadedAllTerrain: // TODO do we need to abort anything?
                case ChunkState::kRenderable:
                    DLOG_F(INFO, "chunk %s was already loaded, demoted in order to merge with new chunk %s",
                           CHUNKSTR(e.chunk_), CHUNKSTR(chunk));

                    // downgrade to terrain-loaded only
                    // fall through

                case ChunkState::kLoadedIsolatedTerrain:
                    // push neighbour merge task
                    if (post_neighbouring_chunks_merge(chunk, e.chunk_, static_cast<ChunkNeighbour>(n))) {
                        DLOG_F(INFO, "submitting merge task between %s and neighbour %d %s ",
                               CHUNKSTR(chunk), n, ChunkId_str(n_id).c_str());
                    }
                    break;
            }
        }
        next_chunk:;
    }
    if (!retry.empty())
        internal_terrain_complete_.push(retry.cbegin(), retry.cend());
    retry.clear();

    // process merged terrain
    complete_merge_jobs_.consume_all([this, world_centre](const NeighbourMergeJob &job) {
        DLOG_F(INFO, "merging %s and neighbour %d %s", CHUNKSTR(job.chunk), job.side, CHUNKSTR(job.neighbour));

        job.chunk->neighbour_mask_.set(job.side, true);
        job.neighbour->neighbour_mask_.set(ChunkNeighbour_opposite(job.side), true);

        Chunk *chunks[2] = {job.chunk, job.neighbour}; // because nicer to iterate
        for (Chunk *chunk : chunks) {
            if (chunk->neighbour_mask_.complete()) {
                // terrain is loaded, generate mesh
                DLOG_F(INFO, "chunk %s has all sides merged, can now generate mesh", CHUNKSTR(chunk));
                chunks_.set_state(chunk, ChunkState::kLoadedAllTerrain);
                pool_.post([this, chunk]() {
                    chunk->populate_mesh();
                    mesh_complete_.push(chunk);
                });
            } else {
                // fancy debugging
                DLOG_F(WARNING, "chunk %s has not finished all sides, what do?! stuck in state %d? mask is %d",
                       CHUNKSTR(chunk), chunks_.get_state(chunk), chunk->neighbour_mask_.mask());

                unsigned int mask = chunk->neighbour_mask_.mask();
                ChunkNeighbours neigs;
                chunk->neighbours(neigs);

                for (int i = 0; i < kChunkNeighbourCount; ++i) {
                    unsigned int b = 1 << i;
                    bool set = mask & b;
                    DLOG_F(INFO, " * neighbour %d (%s) is %d", i, ChunkId_str(neigs[i]).c_str(), set);
                }
            }
        }
    });

    // process new meshes
    mesh_complete_.consume_all([this](Chunk *chunk) {
        chunks_.set_state(chunk, ChunkState::kRenderable);
    });


    // free unloaded chunks
    garbage_.consume_all([this](Chunk *c) {
        chunks_.set_state(c, ChunkState::kUnloaded);
        LOG_F(INFO, "destroying chunk %s", CHUNKSTR(c));

        ChunkMeshRaw *mesh = c->steal_mesh();
        DLOG_F(INFO, "deallocating mesh %p", mesh);
        if (mesh != nullptr)
            mesh_pool_.destroy(mesh);
        chunk_pool_.destroy(c);
    });

}

bool operator==(WorldLoader::NeighbourMergeJobEntry const &a, WorldLoader::NeighbourMergeJobEntry const &b) {
    return a.a == b.a && a.b == b.b && a.side == b.side;
}

std::size_t hash_value(const WorldLoader::NeighbourMergeJobEntry &e) {
    std::size_t seed = 0;
    boost::hash_combine(seed, e.a);
    boost::hash_combine(seed, e.b);
    boost::hash_combine(seed, e.side);
    return seed;
}


bool WorldLoader::post_neighbouring_chunks_merge(Chunk *a, Chunk *b, ChunkNeighbour neighbour) {
    NeighbourMergeJobEntry opposite_job = {b->id(), a->id(), ChunkNeighbour_opposite(neighbour)};
    if (merge_jobs_.find(opposite_job) != merge_jobs_.end()) {
        // nae bother
        return false;
    }

    merge_jobs_.insert({a->id(), b->id(), neighbour});

    pool_.post([this, a, b, neighbour]() {
        // back and front: x axis
        if (neighbour == ChunkNeighbour::kBack) {
            for (unsigned int y = 0; y < kChunkHeight; ++y) {
                for (unsigned int z = 0; z < kChunkDepth; ++z) {
                    unsigned int ax = kChunkWidth - 1;
                    unsigned int bx = 0;

                    Face af = Face::kBack;
                    Face bf = face_opposite(af);

                    Block &block_a = a->terrain_[{ax, y, z}];
                    Block &block_b = b->terrain_[{bx, y, z}];

                    bool a_vis = (BlockType_opaque(block_b.type_));
                    bool b_vis = (BlockType_opaque(block_a.type_));

                    block_a.set_face_visible(af, a_vis);
                    block_b.set_face_visible(bf, b_vis);
                }
            }
        } else if (neighbour == ChunkNeighbour::kFront) {
            for (unsigned int y = 0; y < kChunkHeight; ++y) {
                for (unsigned int z = 0; z < kChunkDepth; ++z) {
                    unsigned int ax = 0;
                    unsigned int bx = kChunkWidth - 1;

                    Face af = Face::kFront;
                    Face bf = face_opposite(af);

                    Block &block_a = a->terrain_[{ax, y, z}];
                    Block &block_b = b->terrain_[{bx, y, z}];

                    bool a_vis = (BlockType_opaque(block_b.type_));
                    bool b_vis = (BlockType_opaque(block_a.type_));

                    block_a.set_face_visible(af, a_vis);
                    block_b.set_face_visible(bf, b_vis);
                }
            }
        }

            // right and left: z axis
        else if (neighbour == ChunkNeighbour::kRight) {
            for (unsigned int y = 0; y < kChunkHeight; ++y) {
                for (unsigned int x = 0; x < kChunkWidth; ++x) {
                    unsigned int az = kChunkDepth - 1;
                    unsigned int bz = 0;

                    Face af = Face::kRight;
                    Face bf = face_opposite(af);

                    Block &block_a = a->terrain_[{x, y, az}];
                    Block &block_b = b->terrain_[{x, y, bz}];

                    bool a_vis = (BlockType_opaque(block_b.type_));
                    bool b_vis = (BlockType_opaque(block_a.type_));

                    block_a.set_face_visible(af, a_vis);
                    block_b.set_face_visible(bf, b_vis);
                }
            }
        } else if (neighbour == ChunkNeighbour::kLeft) {
            for (unsigned int y = 0; y < kChunkHeight; ++y) {
                for (unsigned int x = 0; x < kChunkWidth; ++x) {
                    unsigned int az = 0;
                    unsigned int bz = kChunkDepth - 1;

                    Face af = Face::kLeft;
                    Face bf = face_opposite(af);

                    Block &block_a = a->terrain_[{x, y, az}];
                    Block &block_b = b->terrain_[{x, y, bz}];

                    bool a_vis = (BlockType_opaque(block_b.type_));
                    bool b_vis = (BlockType_opaque(block_a.type_));

                    block_a.set_face_visible(af, a_vis);
                    block_b.set_face_visible(bf, b_vis);
                }
            }
        }

        // job is complete
        NeighbourMergeJob job = {.chunk = a, .neighbour = b, .side=neighbour};
        complete_merge_jobs_.push(job);
    });
    return true;
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

ChunkState ChunkMap::get_state(Chunk *chunk) const {
    Entry e;
    find_chunk(chunk->id(), e);
    return e.state_;
}

void ChunkMap::set_state(Chunk *chunk, ChunkState state) {
    ChunkState prev = ChunkState::kUnloaded;

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
        DLOG_F(INFO, "set chunk(%d, %d) state to %s from %s", x, z,
               ChunkState_str[(int)state].c_str(), ChunkState_str[(int)prev].c_str());
    }
}

void ChunkMap::log_debug_summary() const {
    int unloaded = 0, loading = 0, isolated = 0, all = 0, render = 0;
    std::ostringstream isolated_chunks;
    for (auto &e : map_) {
        switch (e.second.state_) {
            case ChunkState::kUnloaded:
                unloaded++;
                break;
            case ChunkState::kLoading:
                loading++;
                break;
            case ChunkState::kLoadedIsolatedTerrain:
                isolated++;
                isolated_chunks << CHUNKSTR(e.second.chunk_) << "|";
                break;
            case ChunkState::kLoadedAllTerrain:
                all++;
                break;
            case ChunkState::kRenderable:
                render++;
                break;
        }
    }


    LOG_F(INFO, "CHUNKS SUMMARY: %d unloaded, %d loading, %d isolated (%s), %d all terrain, %d renderable",
          unloaded, loading, isolated, isolated_chunks.str().c_str(), all, render);

}

