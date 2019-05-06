#ifndef VOXELS_CENTRE_H
#define VOXELS_CENTRE_H

#include "glm/vec3.hpp"
#include "chunk.h"

class ICentreOfTheGoddamnWorld {
public:
    virtual void get_current_position(glm::vec3 &out) const = 0;
};

class WorldCentre {
public:

    inline void follow(ICentreOfTheGoddamnWorld *followee) { followee_ = followee; }

    inline void stop_following() { followee_ = nullptr; }

    inline glm::vec3 pos() const { return pos_; }

    inline glm::ivec3 ipos() const { return { pos_.x, pos_.y, pos_.z}; }

    bool chunk(ChunkId_t &chunk_out);

    inline void tick() { if (followee_ != nullptr) followee_->get_current_position(pos_); }

private:
    ICentreOfTheGoddamnWorld *followee_ = nullptr;

    glm::vec3 pos_;

    /**
     * id of the chunk the centre was in last tick
     */
    ChunkId_t last_chunk_ = kChunkIdInit;
};


#endif
