#include <glm/gtc/matrix_access.hpp>
#include "frustum.h"
#include "face.h"

Frustum::Frustum(const glm::mat4 &view, const glm::mat4 &proj) {
    const glm::mat4 &v = view;
    const glm::mat4 &p = proj;

    glm::mat4 clipMatrix;

    clipMatrix[0][0] = v[0][0] * p[0][0] + v[0][1] * p[1][0] + v[0][2] * p[2][0] + v[0][3] * p[3][0];
    clipMatrix[1][0] = v[0][0] * p[0][1] + v[0][1] * p[1][1] + v[0][2] * p[2][1] + v[0][3] * p[3][1];
    clipMatrix[2][0] = v[0][0] * p[0][2] + v[0][1] * p[1][2] + v[0][2] * p[2][2] + v[0][3] * p[3][2];
    clipMatrix[3][0] = v[0][0] * p[0][3] + v[0][1] * p[1][3] + v[0][2] * p[2][3] + v[0][3] * p[3][3];
    clipMatrix[0][1] = v[1][0] * p[0][0] + v[1][1] * p[1][0] + v[1][2] * p[2][0] + v[1][3] * p[3][0];
    clipMatrix[1][1] = v[1][0] * p[0][1] + v[1][1] * p[1][1] + v[1][2] * p[2][1] + v[1][3] * p[3][1];
    clipMatrix[2][1] = v[1][0] * p[0][2] + v[1][1] * p[1][2] + v[1][2] * p[2][2] + v[1][3] * p[3][2];
    clipMatrix[3][1] = v[1][0] * p[0][3] + v[1][1] * p[1][3] + v[1][2] * p[2][3] + v[1][3] * p[3][3];
    clipMatrix[0][2] = v[2][0] * p[0][0] + v[2][1] * p[1][0] + v[2][2] * p[2][0] + v[2][3] * p[3][0];
    clipMatrix[1][2] = v[2][0] * p[0][1] + v[2][1] * p[1][1] + v[2][2] * p[2][1] + v[2][3] * p[3][1];
    clipMatrix[2][2] = v[2][0] * p[0][2] + v[2][1] * p[1][2] + v[2][2] * p[2][2] + v[2][3] * p[3][2];
    clipMatrix[3][2] = v[2][0] * p[0][3] + v[2][1] * p[1][3] + v[2][2] * p[2][3] + v[2][3] * p[3][3];
    clipMatrix[0][3] = v[3][0] * p[0][0] + v[3][1] * p[1][0] + v[3][2] * p[2][0] + v[3][3] * p[3][0];
    clipMatrix[1][3] = v[3][0] * p[0][1] + v[3][1] * p[1][1] + v[3][2] * p[2][1] + v[3][3] * p[3][1];
    clipMatrix[2][3] = v[3][0] * p[0][2] + v[3][1] * p[1][2] + v[3][2] * p[2][2] + v[3][3] * p[3][2];
    clipMatrix[3][3] = v[3][0] * p[0][3] + v[3][1] * p[1][3] + v[3][2] * p[2][3] + v[3][3] * p[3][3];

    using Plane = Face;
    planes_[Plane::kRight].x = clipMatrix[3][0] - clipMatrix[0][0];
    planes_[Plane::kRight].y = clipMatrix[3][1] - clipMatrix[0][1];
    planes_[Plane::kRight].z = clipMatrix[3][2] - clipMatrix[0][2];
    planes_[Plane::kRight].w = clipMatrix[3][3] - clipMatrix[0][3];

    planes_[Plane::kLeft].x = clipMatrix[3][0] + clipMatrix[0][0];
    planes_[Plane::kLeft].y = clipMatrix[3][1] + clipMatrix[0][1];
    planes_[Plane::kLeft].z = clipMatrix[3][2] + clipMatrix[0][2];
    planes_[Plane::kLeft].w = clipMatrix[3][3] + clipMatrix[0][3];

    planes_[Plane::kBottom].x = clipMatrix[3][0] + clipMatrix[1][0];
    planes_[Plane::kBottom].y = clipMatrix[3][1] + clipMatrix[1][1];
    planes_[Plane::kBottom].z = clipMatrix[3][2] + clipMatrix[1][2];
    planes_[Plane::kBottom].w = clipMatrix[3][3] + clipMatrix[1][3];

    planes_[Plane::kTop].x = clipMatrix[3][0] - clipMatrix[1][0];
    planes_[Plane::kTop].y = clipMatrix[3][1] - clipMatrix[1][1];
    planes_[Plane::kTop].z = clipMatrix[3][2] - clipMatrix[1][2];
    planes_[Plane::kTop].w = clipMatrix[3][3] - clipMatrix[1][3];

    planes_[Plane::kBack].x = clipMatrix[3][0] - clipMatrix[2][0];
    planes_[Plane::kBack].y = clipMatrix[3][1] - clipMatrix[2][1];
    planes_[Plane::kBack].z = clipMatrix[3][2] - clipMatrix[2][2];
    planes_[Plane::kBack].w = clipMatrix[3][3] - clipMatrix[2][3];

    planes_[Plane::kFront].x = clipMatrix[3][0] + clipMatrix[2][0];
    planes_[Plane::kFront].y = clipMatrix[3][1] + clipMatrix[2][1];
    planes_[Plane::kFront].z = clipMatrix[3][2] + clipMatrix[2][2];
    planes_[Plane::kFront].w = clipMatrix[3][3] + clipMatrix[2][3];

    for (auto &plane : planes_) {
        plane = glm::normalize(plane);
    }
}

bool Frustum::is_rectangle_visible(const glm::vec3 &min, const glm::vec3 &max) const {
#define OUTSTANDING_CORNER(normal) { \
        normal.x < 0 ? min.x : max.x, \
        normal.y < 0 ? min.y : max.y, \
        normal.z < 0 ? min.z : max.z, \
}

    for (const glm::vec4 &plane : planes_) {
        const float pos = plane.w;
        const glm::vec3 &normal = glm::vec3(plane);

        if (glm::dot(normal, OUTSTANDING_CORNER(normal) /* i say! */) + pos < 0)
            return false; // outside
    }

    return true; // inside or intersecting

}
