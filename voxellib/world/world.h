#ifndef VOXELS_WORLD_H
#define VOXELS_WORLD_H

#include <vector>
#include <unordered_map>
#include "chunk.h"

enum class ChunkState {
    kUnloaded = 1,
    kLoaded,

};

inline bool ChunkState_renderable(const ChunkState &cs) {
    return cs == ChunkState::kLoaded; // TODO or mesh being updated
}

// chunk map entry
struct ChunkEntry {
    ChunkState state;
    Chunk *chunk;
};

typedef std::unordered_map<ChunkId_t, ChunkEntry> ChunkMap;

class World {
public:
    World();

    /**
     * @param chunk Must have terrain and mesh fully loaded
     */
    void add_loaded_chunk(Chunk *chunk);

private:
    // TODO map of chunk id -> {load state, optional chunk *}
    // TODO use a hashset keyed with chunk id
    ChunkMap chunks_;


    class RenderableChunkIterator {
    public:
        RenderableChunkIterator(const ChunkMap &chunks) : iterator_(chunks.cbegin()), end_(chunks.cend()) {}

        bool next(Chunk **out);

    private:
        ChunkMap::const_iterator iterator_;
        ChunkMap::const_iterator end_;
    };

public:
    World::RenderableChunkIterator renderable_chunks();
};


#endif
