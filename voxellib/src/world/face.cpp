#include <cassert>
#include "face.h"

void face_offset(Face face, size_t *out) {
    switch (face) {
        case kFront:
            out[0]--;
            break;
        case kLeft:
            out[2]--;
            break;
        case kRight:
            out[2]++;
            break;
        case kTop:
            out[1]++;
            break;
        case kBottom:
            out[1]--;
            break;
        case kBack:
            out[0]++;
            break;
    }
}

Face face_opposite(Face face) {
    switch (face) {
        case kFront:
            return kBack;
        case kLeft:
            return kRight;
        case kRight:
            return kLeft;
        case kTop:
            return kBottom;
        case kBottom:
            return kTop;
        case kBack:
            return kFront;
        default:
            assert(false);
    }
}

bool FaceVisibility::visible(Face face) const {
    int bit = face;
    return operator[](bit);
}

void FaceVisibility::set_fully_visible() {
    set();
}

void FaceVisibility::set_face_visible(Face face, bool visible) {
    int bit = face;
    set(face, visible);
}

bool FaceVisibility::invisible() const {
    return none();
}
