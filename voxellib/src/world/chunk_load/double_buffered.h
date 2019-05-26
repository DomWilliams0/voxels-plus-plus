#ifndef VOXELS_DOUBLE_BUFFERED_H
#define VOXELS_DOUBLE_BUFFERED_H

#include <boost/unordered_set.hpp>
#include <boost/container/vector.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <functional>

template<typename Entry, template<typename> typename Collection>
class DoubleBufferedCollection {
public:
    using Entries = Collection<Entry>;

    Entries &swap() {
        boost::lock_guard lock(lock_);

        // swap buffers
        read_index_ = 1 - read_index_;

        collections_[write()].clear();
        return collections_[read()];
    }

    void add(const Entry &e) {
        boost::lock_guard lock(lock_);
        Entries &write_collection = collections_[write()];
        insert(write_collection, e);
    }

protected:
    virtual void insert(Entries &entries, const Entry &e) = 0;


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
    Chunk *chunk_;
    bool merely_update_;

    friend bool operator==(const ChunkFinalizationEntry &lhs, const ChunkFinalizationEntry &rhs);
};

// hash is keyed on chunk ptr only
std::size_t hash_value(ChunkFinalizationEntry const &e);

// this is the worst thing ive ever written

class ChunkFinalizationQueue : public DoubleBufferedCollection<ChunkFinalizationEntry, boost::unordered_set> {
protected:
    void insert(Entries &entries, const ChunkFinalizationEntry &e) override;
};

// unloading
struct ChunkUnloadEntry {
    Chunk *chunk_;
    bool allow_cache_;

    friend bool operator==(const ChunkUnloadEntry &lhs, const ChunkUnloadEntry &rhs);
};

// hash is keyed on chunk ptr only
std::size_t hash_value(ChunkUnloadEntry const &e);

class ChunkUnloadQueue : public DoubleBufferedCollection<ChunkUnloadEntry, boost::container::vector> {
protected:
    void insert(Entries &entries, const ChunkUnloadEntry &e) override;
};
#endif
