#ifndef VOXELS_LOADER_H
#define VOXELS_LOADER_H

#include <boost/pool/object_pool.hpp>
#include "chunk.h"
#include "threadpool.h"

#include "chunk_load/lookup.h"
#include "world/chunk_load/double_buffered.h"


// lives in main thread, posts requests to thread pool
class WorldLoader {

public:
    explicit WorldLoader(int seed);

    /**
     * Will either load from disk, load from chunk cache or generate from scratch
     * Posts request and does not block
     *
     * @param centre_chunk Remains constant throughout load pipeline
     */
    void request_chunk(ChunkId_t chunk_id);

    void unload_chunk(Chunk *chunk, bool allow_cache = true);

    // number of chunks in loaded radius
    int loaded_chunk_radius_chunk_count() const;

    void tweak_loaded_chunk_radius(int delta);

    inline int loaded_chunk_radius() const { return loaded_chunk_radius_; }

    // no caching
    // includes chunks that are currently cached too
    void unload_all_chunks();

    void tick();

    inline ChunkMap::RenderableChunkIterator renderable_chunks() const {
        return ChunkMap::RenderableChunkIterator(chunks_);
    }

    inline ChunkMap &chunkmap() { return chunks_; }

private:
    int seed_;
    ThreadPool pool_; // TODO should this be moved?

    ChunkMap chunks_;
    ChunkFinalizationQueue finalization_queue_;
    ChunkUnloadQueue unload_queue_;

    boost::object_pool<ChunkMeshRaw> mesh_pool_;
    boost::object_pool<Chunk> chunk_pool_;

    // radius around player to load chunks
    int loaded_chunk_radius_;

    // move from cache back into business
    void uncache_chunk(Chunk *chunk);

    // free and return back to pool
    void delete_chunk(Chunk *chunk);

    void do_finalization(Chunk *chunk, bool merely_update, ChunkMeshRaw *new_mesh);
};

#endif
