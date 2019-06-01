#ifndef VOXELS_DOUBLE_BUFFERED_H
#define VOXELS_DOUBLE_BUFFERED_H

#include <boost/unordered_set.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include "world/chunk.h"

template<typename Entry>
class DoubleBufferedSet {
public:
    using Entries = boost::unordered_set<Entry>;

    Entries &swap() {
        boost::lock_guard lock(lock_);

        // swap buffers
        read_index_ = 1 - read_index_;

        collections_[write()].clear();
        return collections_[read()];
    }

    bool add(const Entry &e) {
        boost::lock_guard lock(lock_);
        Entries &write_collection = collections_[write()];
        return write_collection.insert(e).second;
    }

private:
    unsigned int read() const { return read_index_; }

    unsigned int write() const { return 1 - read_index_; }

    Entries collections_[2];
    unsigned int read_index_ = 0;
    boost::mutex lock_;
};

#endif
