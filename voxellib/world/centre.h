#ifndef VOXELS_CENTRE_H
#define VOXELS_CENTRE_H

#include "glm/vec3.hpp"

class ICentreOfTheGoddamnWorld {
public:
    virtual void get_current_position(glm::vec3 &out) const = 0;
};

class WorldCentre {
public:

    inline void follow(ICentreOfTheGoddamnWorld *followee) { followee_ = followee; }

    inline void stop_following() { followee_ = nullptr; }

    inline glm::vec3 pos() const { return pos_; }

    inline void tick() { if (followee_ != nullptr) followee_->get_current_position(pos_); }

private:
    ICentreOfTheGoddamnWorld *followee_ = nullptr;

    glm::vec3 pos_;
};


#endif
