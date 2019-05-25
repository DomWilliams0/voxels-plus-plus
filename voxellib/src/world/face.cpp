#include "face.h"

void face_offset(Face face, glm::ivec3 &out) {
    switch (face) {
        case kFront:
            out.x--;
            break;
        case kLeft:
            out.z--;
            break;
        case kRight:
            out.z++;
            break;
        case kTop:
            out.y++;
            break;
        case kBottom:
            out.y--;
            break;
        case kBack:
            out.x++;
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
