#include "error.h"
#include "util.h"
#include "loader.h"
#include "config.h"
#include "world.h"
#include "generation/generator.h"

WorldLoader::WorldLoader(int seed) : seed_(seed),
                                     pool_(config::kTerrainThreadWorkers),
                                     loaded_chunk_radius_(config::kInitialLoadedChunkRadius),
                                     cache_count_(0) {
    mesh_pool_.set_next_size(loaded_chunk_radius_chunk_count() * 2); // large buffer
    chunk_pool_.set_next_size(loaded_chunk_radius_chunk_count());

    // TODO set based on available memory
    cache_limit_ = 128;
    LOG_F(INFO, "chunk cache size set to %d", cache_limit_);
}


void WorldLoader::request_chunk(ChunkId_t chunk_id) {
    // check current state
    ChunkState state;
    Chunk *chunk = chunks_.get_chunk(chunk_id, &state);
    if (state == ChunkState::kCached) {
        REF_INC(chunk);
        uncache_queue_.add(chunk_id);
        return;
    } else if (state != ChunkState::kUnloaded) {
        LOG_F(WARNING, "requested load of already loaded chunk %s (state %s)", CHUNKSTR(chunk), state.str().c_str());
        return;
    }

    ChunkMeshRaw *mesh = mesh_pool_.construct();
    chunk = chunk_pool_.construct(chunk_id, mesh);
    if (mesh == nullptr || chunk == nullptr) {
        LOG_F(ERROR, "failed to allocate new chunk: mesh=%p, chunk=%p", mesh, chunk);
        if (mesh) mesh_pool_.destroy(mesh);
        if (chunk) chunk_pool_.destroy(chunk);
        return;
    }

    chunks_.set(chunk_id, chunk);

    DLOG_F(INFO, "allocated new chunk %s", CHUNKSTR(chunk));
    chunk->set_state(ChunkState::kLoadingTerrain);

    pool_.post([this, chunk_id, mesh, chunk]() {
        // TODO deleted ever?
        thread_local IGenerator *gen = config::new_generator();

        int ret = gen->generate(chunk_id, seed_, chunk);
        if (ret == kErrorSuccess) {
            DLOG_F(INFO, "successfully generated terrain for %s", CHUNKSTR(chunk));
            chunk->post_terrain_update();
            chunk->set_state(ChunkState::kLoadedTerrain);

            submit_for_finalization(chunk, false);
            return;
        }

        LOG_F(WARNING, "failed to generate chunk %s with seed %d: %d", CHUNKSTR(chunk), seed_, ret);
        unload_chunk(chunk, false);
    });
}


void WorldLoader::unload_chunk(Chunk *chunk, bool allow_cache) {
    if (chunk != nullptr) {
        ChunkState state = chunk->get_state();
        if (state != ChunkState::kUnloading) {
            if (unload_queue_.add({.chunk_id_=chunk->id(), .allow_cache_=allow_cache})) {// TODO remove this if
                chunk->set_state(ChunkState::kUnloading);
                REF_INC(chunk);
            }
        }
    }
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
        Chunk *chunk = entry.second;
        unload_chunk(chunk, false);
    }
}

bool ChunkMap::RenderableChunkIterator::next(Chunk **out) {
    while (iterator_ != end_) {
        auto &pair = *(iterator_++);

        if (pair.second->get_state() != ChunkState::kRenderable)
            continue;

        Chunk *chunk = pair.second;

        // return this chunk
        *out = chunk;
        return true;
    }

    // all done
    return false;
}


void WorldLoader::tick(ChunkId_t world_centre) {
    // consume uncache queue
    ChunkUncacheQueue::Entries &uncache = uncache_queue_.swap();
    for (auto &chunk_id : uncache) {
        ChunkState state;
        Chunk *chunk = chunks_.get_chunk(chunk_id, &state);

        if (state == ChunkState::kCached)
            uncache_chunk(chunk);

        REF_DEC(chunk);
    }

    // consume finalization queue
    ChunkFinalizationQueue::Entries &finalization = finalization_queue_.swap();
    for (auto &it : finalization) {
        ChunkState state;
        Chunk *c = chunks_.get_chunk(it.chunk_id_, &state);

        if (state != ChunkState::kLoadedTerrain && state != ChunkState::kRenderable) {
            LOG_F(WARNING, "wanted to finalize %s its in state %d", ChunkId_str(it.chunk_id_).c_str(), state);
            if (c != nullptr) REF_DEC(c);
            continue;
        }

        ChunkMeshRaw *new_mesh = nullptr;
        if (it.merely_update_) {
            new_mesh = mesh_pool_.construct();
            if (new_mesh == nullptr) {
                LOG_F(ERROR, "failed to allocate a new mesh from pool?!");
                unload_chunk(c, false);
                REF_DEC(c);
                continue;
            }
        }

        pool_.post([this, c, it, new_mesh]() {
            do_finalization(c, it.merely_update_, new_mesh);
            REF_DEC(c);
        });
    }

    // consume unload queue
    bool flush_cache = false;
    ChunkUnloadQueue::Entries &to_unload = unload_queue_.swap();
    for (auto &it : to_unload) {
        ChunkId_t chunk_id = it.chunk_id_;
        DLOG_F(INFO, "unloading %lu (%s)", chunk_id, ChunkId_str(chunk_id).c_str());

        ChunkState state;
        Chunk *chunk = chunks_.get_chunk(chunk_id, &state);

        if (state != ChunkState::kUnloading) {
            // nop
            DLOG_F(WARNING, "wont unload %s because state is %d", ChunkId_str(chunk_id).c_str(), *state);
            goto next;
        }

/*
        if (state == ChunkState::kLoadedTerrain ||
            state == ChunkState::kLoadingTerrain ||
            chunk->refs_ != 1) {
            // in progress, try again next tick
            // TODO what if they come back into range?
            if (chunk->refs_ > 1)
                LOG_F(WARNING, "what on earth, state %d and refs %d", *state, chunk->refs_.load());
            REF_INC(chunk);
            unload_queue_.add(it);
            goto next;
        }*/

        if (!it.allow_cache_ || 1) {
            // no caching for you
            REF_DEC(chunk);
            if (delete_chunk(chunk))
                continue; // cant deref deleted chunk

            // did not delete because still in use by another thread, try again next time
            LOG_F(WARNING, "resubmitting %s for unloading", CHUNKSTR(chunk));
            REF_INC(chunk);
            unload_queue_.add(it);
            continue; // we have already decremented above
        }

        // dont recache :^)
        if (state != ChunkState::kCached) {
            // evict chunks if necessary
            if (cache_count_ > cache_limit_) {
                // flush those a certain distance from the player
                flush_cache = true;
            }

            chunk->set_state(ChunkState::kCached);
            cache_count_++;
            DLOG_F(INFO, "moved %s into cache (%d/%d)", CHUNKSTR(chunk), cache_count_, cache_limit_);
            goto next;
        }

        next:
        REF_DEC(chunk);
    }

    if (flush_cache || 0)
        flush_cache_wrt_distance(world_centre);

    // reclaim mesh garbage
    MeshGarbage::Entries &mesh_garbage = mesh_garbage_.swap();
    for (ChunkMeshRaw *garbage : mesh_garbage) {
        mesh_pool_.destroy(garbage);
    }
    if (!mesh_garbage.empty())
            DLOG_F(INFO, "reclaimed %d meshes", mesh_garbage.size());
}

void WorldLoader::do_finalization(Chunk *chunk, bool merely_update, ChunkMeshRaw *new_mesh) {
    ChunkNeighbours neighbours;
    chunk->neighbours(neighbours);

    bool retry = false;
    for (int i = 0; i < ChunkNeighbour::kCount; i++) {
        ChunkNeighbour n_side = i;
        ChunkId_t n_id = neighbours[i];

        ChunkState n_state;
        Chunk *n_chunk = chunks_.get_chunk(n_id, &n_state);

        switch (*n_state) {
            case ChunkState::kUnloaded:
            case ChunkState::kUnloading:
            case ChunkState::kCached:
                // nothing to do with this neighbour
                continue;

            case ChunkState::kLoadingTerrain:
                // wait for neighbour to be loaded
                retry = true;
                continue;

            case ChunkState::kLoadedTerrain:
            case ChunkState::kRenderable:
                // terrain available to merge with
                bool merged = chunk->merge_faces_with_neighbour(n_chunk, n_side);

                if (merged && !merely_update && n_state == ChunkState::kRenderable) {
                    // avoid propagation of updates for already complete chunks
                    DLOG_F(INFO, "posting a merely update finalization task for chunk %s (by chunk %s neighbour %d)",
                           CHUNKSTR(n_chunk), CHUNKSTR(chunk), i);
                    submit_for_finalization(n_chunk, true);
                }
                break;
        }

    }

    if (retry) {
        // try again soon
        submit_for_finalization(chunk, merely_update);
        return;
    }

    // generate mesh and set state to renderable
    ChunkMeshRaw *old_mesh = chunk->populate_mesh(merely_update ? new_mesh : nullptr);

    if (old_mesh) {
        mesh_garbage_.add(old_mesh);
    }
}

void WorldLoader::submit_for_finalization(Chunk *chunk, bool merely_update) {
    if (finalization_queue_.add({.chunk_id_=chunk->id(), .merely_update_=merely_update}))
        REF_INC(chunk);
    else
            DLOG_F(WARNING, "tried to submit %s for finalization again!", CHUNKSTR(chunk));
}

void WorldLoader::uncache_chunk(Chunk *chunk) {
    chunk->set_state(ChunkState::kLoadedTerrain);
    submit_for_finalization(chunk, true);
    cache_count_--;
    LOG_F(INFO, "recovered %s from the cache (%d/%d left)", CHUNKSTR(chunk), cache_count_, cache_limit_);
}

bool WorldLoader::delete_chunk(Chunk *chunk) {
    if (chunk == nullptr)
        return false;

    ChunkId_t id = chunk->id();
    int refs = chunk->refs_.load();
    if (refs != 0) {
        DLOG_F(WARNING, "UH OH! %s has %d refs upon deletion!", CHUNKSTR(chunk), refs);
        return false;
    }

    chunks_.set(id, nullptr);

        ChunkMeshRaw *mesh = chunk->steal_mesh();
        if (mesh != nullptr)
            mesh_pool_.destroy(mesh);

//        std::memset(chunk, 0, sizeof(Chunk));
        chunk_pool_.destroy(chunk);
    DLOG_F(INFO, "destroyed chunk %s with %d refs", ChunkId_str(id).c_str(), refs);
    return true;
}

void WorldLoader::flush_cache_wrt_distance(ChunkId_t world_centre) {
    // TODO decide a better way
    const int kMaxDistance = 4; // chunks
    constexpr int kMaxDistanceSqrd = kMaxDistance * kMaxDistance;

    LOG_F(INFO, "commence flushing");

    int count = 0;
    int cx, cz;
    ChunkId_deconstruct(world_centre, cx, cz);
    for (auto &it : chunks_) {
        int x, z;
        ChunkId_deconstruct(it.first, x, z);

        int dx = x - cx;
        int dz = z - cz;
        int dst_sqrd = dx * dx + dz * dz;
        if (dst_sqrd > kMaxDistanceSqrd) {
            if (it.second->get_state() == ChunkState::kCached) {
                unload_chunk(it.second, false);
                count++;
            }
        }
    }

    if (count > 0)
        LOG_F(INFO, "flushed %d chunks from cache", count);
    else
        LOG_F(WARNING, "flushed %d chunks from cache with range limit of %d chunks", count, kMaxDistance);
}
