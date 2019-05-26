#ifndef VOXELS_DOUBLE_BUFFERED_THREAD_SAFE_SET_H
#define VOXELS_DOUBLE_BUFFERED_THREAD_SAFE_SET_H

#include <boost/unordered_set.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

class Chunk;

class Set {
public:
    struct Entry {
        Chunk *chunk_;
        bool merely_update_;
    };
    typedef boost::unordered_set<Entry> SetType;

    void write(boost::lock_guard<boost::mutex> &lock, SetType &set);

    void swap(SetType &out);

private:
    unsigned int read() const;

    unsigned int write() const;


    SetType sets_[2];
    unsigned int read_index_ = 0;
    boost::mutex lock_;
};


#endif
