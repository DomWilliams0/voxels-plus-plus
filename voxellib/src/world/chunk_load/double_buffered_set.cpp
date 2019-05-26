#include "double_buffered_set.h"

std::size_t hash_value(const DoubleBufferedSet::Entry &e) {
    size_t seed = 0;
    boost::hash_combine(seed, e.chunk_);
    return seed;
}

DoubleBufferedSet::SetType &DoubleBufferedSet::swap() {
    boost::lock_guard lock(lock_);

    // swap buffers
    read_index_ = 1 - read_index_;

    sets_[write()].clear();
    return sets_[read()];
}

void DoubleBufferedSet::add(const DoubleBufferedSet::Entry &e) {
    boost::lock_guard lock(lock_);
    SetType &write_set = sets_[write()];
    write_set.insert(e);
}

unsigned int DoubleBufferedSet::read() const {
    return read_index_;
}

unsigned int DoubleBufferedSet::write() const {
    return 1 - read_index_;
}

bool operator==(const DoubleBufferedSet::Entry &lhs, const DoubleBufferedSet::Entry &rhs) {
    return lhs.chunk_ == rhs.chunk_;
}
