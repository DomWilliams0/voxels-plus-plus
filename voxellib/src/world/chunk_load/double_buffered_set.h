#ifndef VOXELS_DOUBLE_BUFFERED_SET_H
#define VOXELS_DOUBLE_BUFFERED_SET_H

#include <boost/unordered_set.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

class Chunk;

class DoubleBufferedSet {
public:
    struct Entry {
        Chunk *chunk_;
        bool merely_update_;

        friend bool operator==(const Entry &lhs, const Entry &rhs);
    };
    typedef boost::unordered_set<Entry> SetType;

    SetType &swap();

    void add(const Entry &e);

private:
    unsigned int read() const;

    unsigned int write() const;

    SetType sets_[2];
    unsigned int read_index_ = 0;
    boost::mutex lock_;
};

// hash is keyed on chunk ptr only
std::size_t hash_value(DoubleBufferedSet::Entry const &e);

#endif
