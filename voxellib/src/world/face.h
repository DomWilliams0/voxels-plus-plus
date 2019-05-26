#ifndef VOXELS_FACE_H
#define VOXELS_FACE_H

#include <array>
#include <bitset>

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

void face_offset(Face face, size_t *out);

Face face_opposite(Face face);

class FaceVisibility : std::bitset<kFaceCount> {
public:
    bool visible(Face face) const;

    void set_fully_visible();

    void set_face_visible(Face face, bool visible);

    // no faces visible
    bool invisible() const;

};


#endif
