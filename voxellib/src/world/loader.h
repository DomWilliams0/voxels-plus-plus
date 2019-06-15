#ifndef VOXELS_LOADER_H
#define VOXELS_LOADER_H

#include <boost/pool/object_pool.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/atomic.hpp>

#include "chunk.h"
#include "threadpool.h"
#include "world/chunk_load/double_buffered.h"

// lives in another thread
class WorldLoader {

public:
    static WorldLoader *create(int seed);

    void update_world_centre(ChunkId_t world_centre, int loaded_chunk_radius);

    // no caching
    // includes chunks that are currently cached too
    inline void unload_all_chunks() { unload_all_chunks_ = true; }

    void get_renderable_chunks(std::vector<ChunkMesh *> &out);

    void finished_rendering();

    // glDeleteVertexArrays if vertex array else glDeleteBuffers
    struct GlGarbage {int buf; bool is_vertex_array;
        GlGarbage(int buf, bool is_vertex_array) : buf(buf), is_vertex_array(is_vertex_array) {}
    };
    void get_gl_goshdarn_garbage(std::vector<GlGarbage> &out);

private:
    WorldLoader(int seed);
    int seed_;

    ThreadPool pool_;

    boost::unordered_map<ChunkId_t, ChunkMesh *> renderable_;
    boost::mutex renderable_lock_;

    boost::atomic_bool currently_rendering_;

    boost::unordered_map<ChunkId_t, std::pair<Chunk *, ChunkState>> chunks_;
    boost::unordered_map<ChunkId_t, Chunk *> chunk_cache_;
    unsigned long cache_limit_;

    boost::atomic_bool unload_all_chunks_;

    std::vector<GlGarbage> gl_garbage_;
    boost::mutex gl_garbage_lock_;

    // updated each tick by main thread
    struct {
        int cx_, cz_;
        int load_radius_;
        boost::shared_mutex lock_;
    } world_state_;

    DoubleBufferedSet<ChunkId_t> finalization_queue_;

    bool flush_cache_;

    boost::posix_time::ptime unload_barrier_;

    boost::unordered_set<ChunkId_t> to_unload_;
    boost::unordered_set<ChunkId_t> per_frame_chunks_;

    boost::object_pool<ChunkMeshRaw> mesh_pool_;
    boost::object_pool<Chunk> chunk_pool_;

    void tick();

    ChunkState get_chunk(ChunkId_t chunk_id, Chunk **chunk_out = nullptr);

    void set_chunk_state(Chunk *chunk, ChunkState new_state);

    // must already be in the map
    void set_chunk_state(ChunkId_t chunk_id, ChunkState new_state);


    /**
     * Will either load from disk, load from chunk cache or generate from scratch
     * Posts request and does not block
     */
    void request_chunk(ChunkId_t chunk_id);

    // unload right now
    void unload_chunk(Chunk *chunk, bool allow_cache = true);

    bool should_unload(ChunkId_t chunk_id);

    void flush_cache_wrt_distance();

};

#endif
