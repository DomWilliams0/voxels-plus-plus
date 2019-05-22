#ifndef VOXELS_LOADER_H
#define VOXELS_LOADER_H

#include <boost/lockfree/stack.hpp>
#include <boost/pool/object_pool.hpp>
#include "world/generation/generator.h"
#include "chunk.h"
#include "threadpool.h"

class World;

// lives in main thread, posts requests to requester thread
class WorldLoader {

public:
    explicit WorldLoader(int seed);

    /**
     * Will either load from disk, load from chunk cache or generate from scratch
     * Posts request and does not block
     */
    void request_chunk(ChunkId_t chunk_id);

    void unload_chunk(Chunk *chunk, bool allow_cache = true);

    bool pop_done(Chunk *&chunk_out);

    void clear_garbage();

    // number of chunks in loaded radius
    int loaded_chunk_radius_chunk_count() const;

    void tweak_loaded_chunk_radius(int delta);

private:
    int seed_;
    ThreadPool pool_;
    boost::lockfree::stack<Chunk *> done_;
    boost::lockfree::stack<Chunk *> garbage_;

    boost::object_pool<ChunkMeshRaw> mesh_pool_;
    boost::object_pool<Chunk> chunk_pool_;

    // radius around player to load chunks
    int loaded_chunk_radius_;
};

#endif
