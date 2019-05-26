#include "error.h"
#include "util.h"
#include "loader.h"
#include "config.h"
#include "world.h"
#include "generation/generator.h"

WorldLoader::WorldLoader(int seed) : seed_(seed),
                                     pool_(config::kTerrainThreadWorkers),
                                     loaded_chunk_radius_(config::kInitialLoadedChunkRadius),
                                     mesh_pool_(loaded_chunk_radius_chunk_count() * 2), // large buffer
                                     chunk_pool_(loaded_chunk_radius_chunk_count()) {
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
    DLOG_F(INFO, "allocated new chunk %s", CHUNKSTR(chunk));
    chunk->set_state(ChunkState::kLoadedTerrain);

    pool_.post([this, chunk_id, mesh, chunk]() {
        // TODO deleted ever?
        thread_local IGenerator *gen = config::new_generator();

        int ret = gen->generate(chunk_id, seed_, chunk);
        if (ret == kErrorSuccess) {
            chunk->post_terrain_update();

            finalization_queue_.add({.chunk_=chunk, .merely_update_=false});
            return;
        }

        LOG_F(WARNING, "failed to generate chunk %s with seed %d: %d", CHUNKSTR(chunk), seed_, ret);
        unload_chunk(chunk, false);
    });
}


void WorldLoader::unload_chunk(Chunk *chunk, bool allow_cache) {
    // TODO add to garbage queue as a pair
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


void WorldLoader::tick() {
    // consume finalization queue
    DoubleBufferedSet::SetType &finalization = finalization_queue_.swap();
    for (auto &it : finalization) {
        ChunkMeshRaw *new_mesh = nullptr;
        if (it.merely_update_)
            new_mesh = mesh_pool_.construct();

        pool_.post([this, it, new_mesh]() {
            do_finalization(it.chunk_, it.merely_update_, new_mesh);
        });
    }
    finalization.clear();

    // TODO consume unload queue
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
                chunk->merge_faces_with_neighbour(n_chunk, n_side);

                if (!merely_update) {
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
        // TODO add to old mesh queue to be reclaimed by the pool
    }
}
