#ifndef VOXELS_FRUSTUM_H
#define VOXELS_FRUSTUM_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Frustum {
public:
    Frustum(const glm::mat4 &view, const glm::mat4 &proj);

    bool is_rectangle_visible(const glm::vec3 &min, const glm::vec3 &max) const;

private:
    glm::vec4 planes_[6];
};


#endif
