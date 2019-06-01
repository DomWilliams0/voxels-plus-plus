#ifndef VOXELS_DOUBLE_BUFFERED_H
#define VOXELS_DOUBLE_BUFFERED_H

#include <boost/unordered_set.hpp>
#include <boost/container/vector.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <functional>

#include "world/chunk.h"

template<typename Entry, template<typename> typename Collection>
class DoubleBufferedCollection {
public:
    using Entries = Collection<Entry>;

    virtual Entries &swap() {
        boost::lock_guard lock(lock_);

        // swap buffers
        read_index_ = 1 - read_index_;

        collections_[write()].clear();
        return collections_[read()];
    }

    bool add(const Entry &e) {
        boost::lock_guard lock(lock_);
        Entries &write_collection = collections_[write()];
        return insert(write_collection, e);
    }

protected:
    virtual bool insert(Entries &entries, const Entry &e) = 0;


private:
    unsigned int read() const { return read_index_; }

    unsigned int write() const { return 1 - read_index_; }

    Entries collections_[2];
    unsigned int read_index_ = 0;
    boost::mutex lock_;
};


// chunk finalization
class Chunk;

struct ChunkFinalizationEntry {
    ChunkId_t chunk_id_;
    bool merely_update_;

    friend bool operator==(const ChunkFinalizationEntry &lhs, const ChunkFinalizationEntry &rhs);
};

// hash is keyed on chunk only
std::size_t hash_value(ChunkFinalizationEntry const &e);

// this is the worst thing ive ever written

class ChunkFinalizationQueue : public DoubleBufferedCollection<ChunkFinalizationEntry, boost::unordered_set> {
protected:
    bool insert(Entries &entries, const ChunkFinalizationEntry &e) override;
};

// unloading
struct ChunkUnloadEntry {
    ChunkId_t chunk_id_;
    bool allow_cache_;

    friend bool operator==(const ChunkUnloadEntry &lhs, const ChunkUnloadEntry &rhs);
};

// hash is keyed on chunk id, cant be on the ptr because it could be recycled
std::size_t hash_value(ChunkUnloadEntry const &e);

class ChunkUnloadQueue : public DoubleBufferedCollection<ChunkUnloadEntry, boost::unordered_set> {
protected:
    bool insert(Entries &entries, const ChunkUnloadEntry &e) override;
};

// uncache
class ChunkUncacheQueue : public DoubleBufferedCollection<ChunkId_t, boost::container::vector> {
protected:
    bool insert(Entries &entries, const ChunkId_t &e) override;
};

// mesh garbage
typedef ChunkMeshRaw *ChunkMeshRaw_ptr;

class MeshGarbage : public DoubleBufferedCollection<ChunkMeshRaw *, boost::container::vector> {
protected:
    bool insert(Entries &entries, ChunkMeshRaw *const &e) override;
};
#endif
