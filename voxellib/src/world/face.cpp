#include "face.h"

void face_offset(Face face, const glm::ivec3 &in, glm::ivec3 &out) {
    switch (face) {
        case kFront:
            out = in + glm::ivec3(-1, 0, 0);
            break;
        case kLeft:
            out = in + glm::ivec3(0, 0, -1);
            break;
        case kRight:
            out = in + glm::ivec3(0, 0, 1);
            break;
        case kTop:
            out = in + glm::ivec3(0, 1, 0);
            break;
        case kBottom:
            out = in + glm::ivec3(0, -1, 0);
            break;
        case kBack:
            out = in + glm::ivec3(1, 0, 0);
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
    }

    return kFront; // why on earth do we need this
}
