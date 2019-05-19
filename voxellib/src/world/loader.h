#ifndef VOXELS_LOADER_H
#define VOXELS_LOADER_H

#include <boost/asio/thread_pool.hpp>
#include <boost/lockfree/stack.hpp>
#include "world/generation/generator.h"
#include "chunk.h"

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

    void unload_chunk(Chunk *chunk);

    bool pop_done(Chunk *&chunk_out);

private:
    int seed_;
    boost::asio::thread_pool pool_;
    boost::lockfree::stack<Chunk *> done_;
};

#endif
