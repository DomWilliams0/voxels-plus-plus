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

void face_offset(Face face, glm::ivec3 &out);

Face face_opposite(Face face);

// TODO std::bitset
typedef int8_t FaceVisibility;

const FaceVisibility kFaceVisibilityAll = (1 << kFaceCount) - 1;
const FaceVisibility kFaceVisibilityNone = 0;

inline FaceVisibility face_visibility(Face face) { return 1 << face; }

inline bool face_is_visible(FaceVisibility visibility, Face face) {
    return (visibility & face_visibility(face)) != kFaceVisibilityNone;
}


#endif
