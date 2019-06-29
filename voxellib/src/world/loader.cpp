#include <jemalloc/jemalloc.h>
#include "error.h"
#include "util.h"
#include "loader.h"
#include "config.h"
#include "world.h"
#include "iterators.h"
#include "generation/generator.h"

WorldLoader *WorldLoader::create(int seed, boost::thread **thread_out) {
    WorldLoader *loader = new WorldLoader(seed);
    *thread_out = new boost::thread([loader]() {
        // wait a tad for game to properly start :^)
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        while (loader->running_) {
            loader->tick();
            boost::this_thread::yield();
        }

        LOG_F(INFO, "loader thread exiting");
        loader->pool_.shutdown();
        delete loader;
    });

    return loader;
}

WorldLoader::WorldLoader(int seed) :
        seed_(seed),
        pool_(config::kTerrainThreadWorkers),
        unload_barrier_(boost::posix_time::microsec_clock::local_time()),
        chunk_pool_(128) {

    // TODO set based on available memory
    cache_limit_ = 128;
    LOG_F(INFO, "chunk cache size set to %lu", cache_limit_);
}

WorldLoader::~WorldLoader() {
    chunk_pool_.delete_all();
    chunk_pool_.reclaim_memory();
}

void WorldLoader::update_world_centre(ChunkId_t world_centre, int loaded_chunk_radius) {
    boost::unique_lock lock(world_state_.lock_);
    ChunkId_deconstruct(world_centre, world_state_.cx_, world_state_.cz_);
    world_state_.load_radius_ = loaded_chunk_radius;
}

void WorldLoader::tick() {
    // handle unload all
    if (unload_all_chunks_) {
        really_unload_all_chunks();
        unload_all_chunks_ = false;
    }

    // determine new chunks to load and unload
    int cx, cz;
    int load_radius, load_radius_chunk_count;
    {
        boost::shared_lock lock(world_state_.lock_);
        cx = world_state_.cx_;
        cz = world_state_.cz_;
        load_radius = world_state_.load_radius_;
    }
    load_radius_chunk_count = loaded_radius_chunk_count(load_radius);

    per_frame_chunks_.clear();
    ITERATOR_CHUNK_SPIRAL_BEGIN(load_radius_chunk_count, cx, cz)
        ChunkId_t c = ChunkId(x, z);

        // mark this chunk as in range
        per_frame_chunks_.insert(c);

        if (get_chunk(c) == ChunkState::kUnloaded)
            request_chunk(c);
    ITERATOR_CHUNK_SPIRAL_END

    for (auto &e : chunks_) {
        ChunkState state = e.second.second;
        if (state == ChunkState::kRenderable &&
            per_frame_chunks_.find(e.first) == per_frame_chunks_.end()) {
            // loaded and not in range
            to_unload_.insert(e.first);
        }
    }

    // finalization
    auto &finalization = finalization_queue_.swap();

    // first pass to update states
    for (auto it = finalization.begin(); it != finalization.end();) {
        ChunkId_t c = *it;

        // marked for unload
        if (should_unload(c) || get_chunk(c) == ChunkState::kUnloaded) {
            it = finalization.erase(it);
            continue;
        }

        DLOG_F(INFO, "iterated %s in finalization, setting to loaded", ChunkId_str(c).c_str());
        set_chunk_state(c, ChunkState::kLoadedTerrain);
        it++;
    }

    // second pass to merge neighbours
    for (ChunkId_t c : finalization) {
        assert(!should_unload(c)); // should have been filtered out in first pass

        Chunk *chunk;
        ChunkState state = get_chunk(c, &chunk);
        ChunkNeighbours neighbours;
        chunk->neighbours(neighbours);

        int neighbours_done = 0;
        for (int i = 0; i < ChunkNeighbour::kCount; i++) {
            ChunkNeighbour n_side = i;
            ChunkId_t n_id = neighbours[i];

            Chunk *n_chunk;
            ChunkState n_state = get_chunk(n_id, &n_chunk);

            switch (*n_state) {
                case ChunkState::kUnloaded:
                    // nothing to do with this neighbour
                    neighbours_done++;
                    continue;

                case ChunkState::kLoadingTerrain:
                    // wait for neighbour to be loaded
                    continue;

                case ChunkState::kRenderable:
                case ChunkState::kLoadedTerrain:
                    // terrain available to merge with
                    bool merged = chunk->merge_faces_with_neighbour(n_chunk, n_side);
                    neighbours_done++;

                    if (merged && n_state == ChunkState::kRenderable) {
                        // avoid propagation of updates for already complete chunks
                        DLOG_F(INFO,
                               "posting a merely update finalization task for chunk %s (by chunk %s neighbour %d)",
                               CHUNKSTR(n_chunk), CHUNKSTR(chunk), i);

                        finalization_queue_.add(n_id);
                    }
                    break;
            }
        }

        DLOG_F(INFO, "%s had %d neighbours", CHUNKSTR(chunk), neighbours_done);

        bool update_mesh;

        // more cpu intensive but smoother experience
        update_mesh = true; // populate no matter what

        // less cpu intensive but less smooth
        // update_mesh = neighbours_done == ChunkNeighbour::kCount;

        if (!update_mesh) {
            // try again next tick
            finalization_queue_.add(c);
            continue;
        } else {
            // fill temporary buffer to calculate size
            static ChunkMeshRaw tmp_mesh;

            // populate it and get length
            unsigned long tmp_mesh_len = chunk->populate_mesh(tmp_mesh);

            // alloc mesh of that length and copy in
            ChunkMeshRaw *new_mesh = alloc_mesh(tmp_mesh_len);
            std::copy(std::cbegin(tmp_mesh), std::cbegin(tmp_mesh) + tmp_mesh_len, std::begin(*new_mesh));

            // swap out and reclaim old mesh
            ChunkMeshRaw *old_mesh = chunk->mesh()->on_mesh_update(tmp_mesh_len, new_mesh);
            if (old_mesh != nullptr)
                dealloc_mesh(old_mesh);

            // update state to renderable
            if (state != ChunkState::kRenderable) {
                set_chunk_state(c, ChunkState::kRenderable);

                {
                    boost::lock_guard lock(renderable_lock_);
                    renderable_[c] = chunk->mesh();
                }
            }
        }

    }

    // unload queued chunks
    for (auto it = to_unload_.begin(); it != to_unload_.end();) {
        if (currently_rendering_) {
            it++;
            continue;
        }

        Chunk *c;
        get_chunk(*it, &c);
        unload_chunk(c);
        it = to_unload_.erase(it);
    }

    // flush cache
    if (flush_cache_) {
        flush_cache_ = false;
        flush_cache_wrt_distance();
    }
}

ChunkState WorldLoader::get_chunk(ChunkId_t chunk_id, Chunk **chunk_out) {
    auto it = chunks_.find(chunk_id);

    Chunk *ret_chunk;
    ChunkState ret_state;

    if (it == chunks_.end()) {
        ret_chunk = nullptr;
        ret_state = ChunkState::kUnloaded;
    } else {
        ret_chunk = it->second.first;
        ret_state = it->second.second;
    }

    if (chunk_out != nullptr)
        *chunk_out = ret_chunk;

    return ret_state;
}

void WorldLoader::set_chunk_state(Chunk *chunk, ChunkState new_state) {
    ChunkId_t c = chunk->id();

    ChunkState old_state = get_chunk(c);
    if (old_state == new_state)
        return;

    if (new_state == ChunkState::kUnloaded)
        chunks_.erase(c);
    else
        chunks_[c] = {chunk, new_state};

    DLOG_F(INFO, "set chunk %s state from %s to %s", CHUNKSTR(chunk),
           old_state.str().c_str(), new_state.str().c_str());
}

void WorldLoader::set_chunk_state(ChunkId_t chunk_id, ChunkState new_state) {
    auto it = chunks_.find(chunk_id);
    assert(it != chunks_.end()); // must already be in the map

    set_chunk_state(it->second.first, new_state);
}


void WorldLoader::request_chunk(ChunkId_t chunk_id) {
    // check cache
    auto cache_result = chunk_cache_.find(chunk_id);
    if (cache_result != chunk_cache_.end()) {
        // use terrain from cache
        Chunk *cached_chunk = cache_result->second;
        DLOG_F(INFO, "uncached requested chunk %s", CHUNKSTR(cached_chunk));
        chunk_cache_.erase(cache_result);
        set_chunk_state(cached_chunk, ChunkState::kLoadedTerrain);
        finalization_queue_.add(chunk_id);
        return;
    }

    // allocate chunk from pool
    Chunk *chunk = chunk_pool_.new_object(chunk_id);
    if (chunk == nullptr) {
        LOG_F(ERROR, "failed to allocate new chunk");
        return;
    }

    DLOG_F(INFO, "allocated new chunk %s", CHUNKSTR(chunk));
    chunk->mark_load_time_now();
    set_chunk_state(chunk, ChunkState::kLoadingTerrain);

    pool_.post([this, chunk_id, chunk]() {
        thread_local IGenerator *gen = config::new_generator(); // TODO ever deleted?

        int ret = gen->generate(chunk_id, seed_, chunk);
        if (ret == kErrorSuccess) {
            DLOG_F(INFO, "successfully generated terrain for %s", CHUNKSTR(chunk));
            chunk->post_terrain_update();
            finalization_queue_.add(chunk_id);
            return;
        }

        LOG_F(WARNING, "failed to generate chunk %s with seed %d: %d", CHUNKSTR(chunk), seed_, ret);
        unload_chunk(chunk, false);
    });
}

void WorldLoader::unload_chunk(Chunk *chunk, bool allow_cache) {
    set_chunk_state(chunk, ChunkState::kUnloaded);

    {
        boost::lock_guard lock(renderable_lock_);
        int n = renderable_.erase(chunk->id());
        if (n > 0)
            DLOG_F(INFO, "removed renderable chunk %s", CHUNKSTR(chunk));
    }

    if (allow_cache) {
        if (chunk_cache_.size() >= cache_limit_) {
            flush_cache_ = true;
        } else {
            // put into cache
            chunk->reset_for_cache();
            chunk_cache_[chunk->id()] = chunk;
            DLOG_F(INFO, "put chunk %s in the cache (%lu/%lu)", CHUNKSTR(chunk), chunk_cache_.size(), cache_limit_);
            return;
        }
    }

    DLOG_F(INFO, "deleting chunk %s", CHUNKSTR(chunk));

    // delete now
    ChunkMeshRaw *mesh = chunk->mesh()->steal_mesh();
    if (mesh != nullptr)
        dealloc_mesh(mesh);

    {
        boost::lock_guard lock(gl_garbage_lock_);
        if (chunk->mesh()->vao() != 0)
            gl_garbage_.emplace_back(chunk->mesh()->vao(), true);
        if (chunk->mesh()->vbo() != 0)
            gl_garbage_.emplace_back(chunk->mesh()->vbo(), false);
    }

    chunk_pool_.delete_object(chunk);
}

bool WorldLoader::should_unload(ChunkId_t chunk_id) {
    Chunk *chunk;
    get_chunk(chunk_id, &chunk);

    // check if in unload set or was loaded before unload barrier
    auto it = to_unload_.find(chunk_id);
    bool unload = it != to_unload_.end() && !chunk->was_loaded_before(unload_barrier_);

    if (unload && !currently_rendering_) {
        to_unload_.erase(it);
        unload_chunk(chunk);
    }

    return unload;
}

void WorldLoader::flush_cache_wrt_distance() {
    int cx, cz, load_rad;
    {
        boost::shared_lock lock(world_state_.lock_);
        cx = world_state_.cx_;
        cz = world_state_.cz_;
        load_rad = world_state_.load_radius_;
    }

    const int chunk_flush_max_distance = load_rad; // TODO experiment
    const int threshold = chunk_flush_max_distance * chunk_flush_max_distance;

    for (auto it = chunk_cache_.begin(); it != chunk_cache_.end(); ) {
        int x, z;
        ChunkId_deconstruct(it->first, x, z);

        int dx = cx - x;
        int dz = cz - z;
        int dist_sqr = (dx*dx) + (dz+dz);

        if (dist_sqr > threshold) {
            unload_chunk(it->second, false);
            it = chunk_cache_.erase(it);
        } else
            it++;
    }

    DLOG_F(INFO, "flushed chunk cache to %lu", chunk_cache_.size());

    // might as well do this here
    chunk_pool_.reclaim_memory();
}

void WorldLoader::get_renderable_chunks(std::vector<ChunkMesh *> &out) {
    currently_rendering_ = true;

    boost::lock_guard lock(renderable_lock_);
    for (auto &it : renderable_) {
        out.push_back(it.second);
    }
}

void WorldLoader::finished_rendering() {
    currently_rendering_ = false;
}

void WorldLoader::get_gl_goshdarn_garbage(std::vector<WorldLoader::GlGarbage> &out) {
    boost::lock_guard lock(gl_garbage_lock_);
    out = std::move(gl_garbage_);
}

void WorldLoader::really_unload_all_chunks() {
    LOG_F(INFO, "unloading all chunks");

    // unload all before now
    unload_barrier_ = boost::posix_time::microsec_clock::local_time();

    // no more rendering
    {
        boost::lock_guard lock(renderable_lock_);
        renderable_.clear();
    }

    // wait for render to finish
    while (currently_rendering_) {}

    // move to temporary map to avoid invalidating iterators
    auto chunks_copy = std::move(chunks_);
    for (auto &it : chunks_copy) {
        ChunkState state = it.second.second;
        switch (*state) {
            case ChunkState::kUnloaded:
                // nop
                break;
            case ChunkState::kLoadingTerrain:
            case ChunkState::kLoadedTerrain:
                // nop, will be cleansed by unload time barrier
                break;

            case ChunkState::kRenderable:
                unload_chunk(it.second.first, false);
                break;
        }
    }

    // clear cache
    for (auto &it : chunk_cache_) {
        unload_chunk(it.second, false);
    }
    chunk_cache_.clear();

    to_unload_.clear();
}

ChunkMeshRaw *WorldLoader::alloc_mesh(unsigned long length) {
    void *ret = mallocx(length * sizeof(ChunkMeshRaw::value_type), MALLOCX_ARENA(0));
    return static_cast<ChunkMeshRaw *>(ret);
}

void WorldLoader::dealloc_mesh(ChunkMeshRaw *mesh) {
    dallocx(mesh, 0);
}

void WorldLoader::stop() {
    running_ = false;
}
