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

    bool chunk(ChunkId_t &chunk_out);

    inline ChunkId_t chunk() const { return last_chunk_; };

    inline void tick() { if (followee_ != nullptr) followee_->get_current_position(pos_); }

    inline void reset() { last_chunk_ = kChunkIdInit; }

private:
    ICentreOfTheGoddamnWorld *followee_ = nullptr;

    glm::vec3 pos_;

    /**
     * id of the chunk the centre was in last tick
     */
    ChunkId_t last_chunk_ = kChunkIdInit;
};


#endif
