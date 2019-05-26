#include "error.h"
#include "util.h"
#include "loader.h"
#include "config.h"
#include "world.h"
#include "generation/generator.h"

WorldLoader::WorldLoader(int seed) : seed_(seed),
                                     pool_(config::kTerrainThreadWorkers),
                                     loaded_chunk_radius_(config::kInitialLoadedChunkRadius) {
    mesh_pool_.set_next_size(loaded_chunk_radius_chunk_count() * 2); // large buffer
    chunk_pool_.set_next_size(loaded_chunk_radius_chunk_count());
}


void WorldLoader::request_chunk(ChunkId_t chunk_id) {
    // check current state
    ChunkState state;
    Chunk *chunk = chunks_.get_chunk(chunk_id, &state);
    if (state == ChunkState::kCached) {
        uncache_chunk(chunk);
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

            finalization_queue_.add({.chunk_=chunk, .merely_update_=false});
            return;
        }

        LOG_F(WARNING, "failed to generate chunk %s with seed %d: %d", CHUNKSTR(chunk), seed_, ret);
        unload_chunk(chunk, false);
    });
}


void WorldLoader::unload_chunk(Chunk *chunk, bool allow_cache) {
    if (chunk != nullptr)
        unload_queue_.add({.chunk_=chunk, .allow_cache_=allow_cache});
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


void WorldLoader::tick() {
    // consume finalization queue
    ChunkFinalizationQueue::Entries &finalization = finalization_queue_.swap();
    for (auto &it : finalization) {
        ChunkMeshRaw *new_mesh = nullptr;
        if (it.merely_update_) {
            new_mesh = mesh_pool_.construct();
            if (new_mesh == nullptr) {
                LOG_F(ERROR, "failed to allocate a new mesh from pool?!");
                unload_chunk(it.chunk_, false);
                continue;
            }
        }

        pool_.post([this, it, new_mesh]() {
            do_finalization(it.chunk_, it.merely_update_, new_mesh);
        });
    }

    // consume unload queue
    ChunkUnloadQueue::Entries &to_unload = unload_queue_.swap();
    for (auto &it : to_unload) {
        Chunk *chunk = it.chunk_;
        ChunkState state = chunk->get_state();

        switch (*state) {
            case ChunkState::kUnloaded:
                // nop
                continue;

            case ChunkState::kLoadedTerrain:
            case ChunkState::kLoadingTerrain:
                // in progress, try again next tick
                // TODO what if they come back into range?
                unload_queue_.add(it);
                break;

            case ChunkState::kCached:
            case ChunkState::kRenderable:
                if (!it.allow_cache_) {
                    // no caching for you
                    delete_chunk(chunk);
                    continue;
                }

                // dont recache :^)
                if (state != ChunkState::kCached) {
                    // TODO keep count of cached chunks and test limit here
                    // evict chunks if necessary

                    chunk->set_state(ChunkState::kCached);
                    DLOG_F(INFO, "moved %s into cache", CHUNKSTR(chunk));
                }

        }
    }

    // reclaim mesh garbage
    MeshGarbage::Entries &mesh_garbage = mesh_garbage_.swap();
    for (ChunkMeshRaw *garbage : mesh_garbage) {
        mesh_pool_.destroy(garbage);
        DLOG_F(INFO, "reclaimed a mesh");
    }
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
                    finalization_queue_.add({.chunk_=n_chunk, .merely_update_=true});
                    LOG_F(INFO, "posting a merely update finalization task for chunk %s (by chunk %s neighbour %d)",
                          CHUNKSTR(n_chunk), CHUNKSTR(chunk), i);
                }
                break;
        }

    }

    if (retry) {
        // try again soon
        finalization_queue_.add({.chunk_=chunk, .merely_update_=merely_update});
        return;
    }

    // generate mesh and set state to renderable
    ChunkMeshRaw *old_mesh = chunk->populate_mesh(merely_update ? new_mesh : nullptr);

    if (old_mesh) {
        mesh_garbage_.add(old_mesh);
    }
}

void WorldLoader::uncache_chunk(Chunk *chunk) {
    chunk->set_state(ChunkState::kLoadedTerrain);
    finalization_queue_.add({.chunk_=chunk, .merely_update_=true});
    LOG_F(INFO, "recovered %s from the cache", CHUNKSTR(chunk));

}

void WorldLoader::delete_chunk(Chunk *chunk) {
    if (chunk != nullptr) {
        ChunkMeshRaw *mesh = chunk->steal_mesh();
        if (mesh != nullptr)
            mesh_pool_.destroy(mesh);

        ChunkId_t id = chunk->id();
        chunk_pool_.destroy(chunk);
        LOG_F(INFO, "destroyed chunk %s", ChunkId_str(id).c_str());

        chunks_.set(id, nullptr);
    }

}
