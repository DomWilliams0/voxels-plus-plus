#ifndef VOXELS_LOOKUP_H
#define VOXELS_LOOKUP_H

#include "state.h"
#include "world/chunk.h"

class ChunkMap;

// maps chunk id -> Chunk instance
class ChunkMap {
public:
    Chunk *get_chunk(ChunkId_t chunk_id, ChunkState *state_out) const;

    typedef std::unordered_map<ChunkId_t, Chunk *> MapType;
    typedef MapType::const_iterator const_iterator;

    inline const MapType::iterator begin() { return map_.begin(); };

    inline const MapType::iterator end() { return map_.end(); };

//
    inline bool empty() const { return map_.empty(); }
//
//    inline void clear() { return map_.clear(); }
//
//    inline MapType::iterator erase(MapType::iterator it) { return map_.erase(it); }

    class RenderableChunkIterator {
    public:
        RenderableChunkIterator(const ChunkMap &chunks) : iterator_(chunks.map_.cbegin()), end_(chunks.map_.cend()) {}

        bool next(Chunk **out);

    private:
        ChunkMap::const_iterator iterator_;
        ChunkMap::const_iterator end_;
    };

private:
    MapType map_;
};


#endif
