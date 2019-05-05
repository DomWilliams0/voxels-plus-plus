#ifndef VOXELS_FACE_H
#define VOXELS_FACE_H

#include <glm/vec3.hpp>

// TODo store bitwise value outside
enum Face {
    kFront = 0,
    kLeft,
    kRight,
    kTop,
    kBottom,
    kBack,
};

const int kFaceCount = 6;

const enum Face kFaces[kFaceCount] = {
        kFront,
        kLeft,
        kRight,
        kTop,
        kBottom,
        kBack
};

void face_offset(Face face, const glm::ivec3 &in, glm::ivec3 &out);

Face face_opposite(Face face);

const int kFaceVisibilityAll = (1 << kFaceCount) - 1;
const int kFaceVisibilityNone = 0;

inline bool face_is_visible(int visibility, Face face) {
    return ((visibility) & (1 << (face))) != kFaceVisibilityNone;
}


#endif
