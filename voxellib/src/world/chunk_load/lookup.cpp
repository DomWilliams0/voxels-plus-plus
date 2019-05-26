#include "loguru/loguru.hpp"
#include "lookup.h"

Chunk *ChunkMap::get_chunk(ChunkId_t chunk_id, ChunkState *state_out) const {
    auto it = map_.find(chunk_id);

    Chunk *ret_chunk;
    ChunkState ret_state;

    if (it == map_.end()) {
        ret_chunk = nullptr;
        ret_state = ChunkState::kUnloaded;
    } else {
        ret_chunk = it->second;
        ret_state = ret_chunk->get_state();
    }

    if (state_out != nullptr)
        *state_out = ret_state;

    return ret_chunk;
}

void ChunkMap::set(ChunkId_t chunk_id, Chunk *chunk) {
    if (chunk == nullptr) {
        map_.erase(chunk_id);
    } else {
        auto it = map_.find(chunk_id);
        if (it != map_.end()) {
            LOG_F(WARNING, "updated chunk map with existing chunk: is %p, set to %p", it->second, chunk);
        }

        map_[chunk_id] = chunk;
    }
}
