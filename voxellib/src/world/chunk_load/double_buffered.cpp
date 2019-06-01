#include "double_buffered.h"

std::size_t hash_value(const ChunkFinalizationEntry &e) {
    return boost::hash<ChunkId_t>()(e.chunk_id_);
}

bool operator==(const ChunkFinalizationEntry &lhs, const ChunkFinalizationEntry &rhs) {
    return lhs.chunk_id_ == rhs.chunk_id_;
}

bool ChunkFinalizationQueue::insert(Entries &entries, const ChunkFinalizationEntry &e) {
    return entries.insert(e).second;
}


std::size_t hash_value(const ChunkUnloadEntry &e) {
    return boost::hash<ChunkId_t>()(e.chunk_id_);
}

bool operator==(const ChunkUnloadEntry &lhs, const ChunkUnloadEntry &rhs) {
    return lhs.chunk_id_ == rhs.chunk_id_;
}

#include "loguru/loguru.hpp"
bool ChunkUnloadQueue::insert(Entries &entries, const ChunkUnloadEntry &e) {
    return entries.insert(e).second;
}

bool ChunkUncacheQueue::insert(Entries &entries, const ChunkId_t &e) {
    entries.push_back(e);
    return true;
}

bool MeshGarbage::insert(Entries &entries, ChunkMeshRaw *const &e) {
    entries.push_back(e);
    return true;
}
