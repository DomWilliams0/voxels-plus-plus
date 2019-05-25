#ifndef VOXELS_LOADER_H
#define VOXELS_LOADER_H

#include <boost/unordered_set.hpp>
#include <boost/lockfree/stack.hpp>
#include <boost/pool/object_pool.hpp>
#include "world/generation/generator.h"
#include "chunk.h"
#include "threadpool.h"

enum class ChunkState {
    kUnloaded = 1,
    kLoading,               // terrain load in progress
    kLoadedIsolatedTerrain, // terrain loaded, face visibility for own internal faces calculated
    kLoadedAllTerrain,      // terrain loaded, all face visibility incl. external facing calculated
    kRenderable,            // terrain loaded, mesh generated
};

inline bool ChunkState_renderable(const ChunkState &cs) {
    return cs == ChunkState::kRenderable;
}

class ChunkMap {
public:
    struct Entry {
        ChunkState state_;
        Chunk *chunk_;
    };

    void log_debug_summary() const;

    void find_chunk(ChunkId_t chunk_id, Entry &out) const;

    ChunkState get_state(Chunk *chunk) const;

    void set_state(Chunk *chunk, ChunkState state);

    typedef std::unordered_map<ChunkId_t, Entry> MapType;
    typedef MapType::const_iterator const_iterator;

    inline MapType::const_iterator cbegin() const { return map_.cbegin(); };

    inline MapType::const_iterator cend() const { return map_.cend(); };

    inline MapType::iterator begin() { return map_.begin(); };

    inline MapType::iterator end() { return map_.end(); };

    inline bool empty() const { return map_.empty(); }

    inline void clear() { return map_.clear(); }

    inline MapType::iterator erase(MapType::iterator it) { return map_.erase(it); }

    class RenderableChunkIterator {
    public:
        RenderableChunkIterator(const ChunkMap &chunks) : iterator_(chunks.cbegin()), end_(chunks.cend()) {}

        bool next(Chunk **out);

    private:
        ChunkMap::const_iterator iterator_;
        ChunkMap::const_iterator end_;
    };

private:
    MapType map_;
};


class World;

// lives in main thread, posts requests to requester thread
class WorldLoader {

public:
    explicit WorldLoader(int seed);

    /**
     * Will either load from disk, load from chunk cache or generate from scratch
     * Posts request and does not block
     *
     * @param centre_chunk Remains constant throughout load pipeline
     */
    void request_chunk(ChunkId_t chunk_id, ChunkId_t centre_chunk);

    void unload_chunk(Chunk *chunk, bool allow_cache = true);

    // number of chunks in loaded radius
    int loaded_chunk_radius_chunk_count() const;

    void tweak_loaded_chunk_radius(int delta);

    inline int loaded_chunk_radius() const { return loaded_chunk_radius_; }

    void unload_all_chunks();

    void tick(ChunkId_t world_centre);

    inline ChunkMap::RenderableChunkIterator renderable_chunks() const {
        return ChunkMap::RenderableChunkIterator(chunks_);
    }

    inline ChunkMap &chunkmap() { return chunks_; }

    // for hashset
    struct NeighbourMergeJobEntry {
        ChunkId_t a, b;
        ChunkNeighbour side;

        friend bool operator==(NeighbourMergeJobEntry const &a, NeighbourMergeJobEntry const &b);
    };

private:
    int seed_;
    ThreadPool pool_;

    struct NeighbourMergeJob {
        Chunk *chunk, *neighbour;
        ChunkNeighbour side;
    };

    boost::lockfree::stack<Chunk *>
            internal_terrain_complete_, // loading -> loaded internal terrain
            all_terrain_complete_,      // loaded internal terrain -> loaded all terrain
            mesh_complete_,             // loaded all terrain -> renderable
            garbage_;                   // to be unloaded
    boost::lockfree::stack<NeighbourMergeJob>
            complete_merge_jobs_;
    boost::unordered_set<NeighbourMergeJobEntry> merge_jobs_;

    boost::object_pool<ChunkMeshRaw> mesh_pool_;
    boost::object_pool<Chunk> chunk_pool_;

    // radius around player to load chunks
    int loaded_chunk_radius_;

    ChunkMap chunks_;

    bool post_neighbouring_chunks_merge(Chunk *a, Chunk *b, ChunkNeighbour neighbour);
};

#endif
