#include "double_buffered.h"

std::size_t hash_value(const ChunkFinalizationEntry &e) {
    size_t seed = 0;
    boost::hash_combine(seed, e.chunk_);
    return seed;
}

bool operator==(const ChunkFinalizationEntry &lhs, const ChunkFinalizationEntry &rhs) {
    return lhs.chunk_ == rhs.chunk_;
}

void ChunkFinalizationQueue::insert(Entries &entries, const ChunkFinalizationEntry &e) {
    entries.insert(e);
}


std::size_t hash_value(const ChunkUnloadEntry &e) {
    size_t seed = 0;
    boost::hash_combine(seed, e.chunk_);
    return seed;
}

bool operator==(const ChunkUnloadEntry &lhs, const ChunkUnloadEntry &rhs) {
    return lhs.chunk_ == rhs.chunk_;
}

void ChunkUnloadQueue::insert(Entries &entries, const ChunkUnloadEntry &e) {
    entries.push_back(e);
}
