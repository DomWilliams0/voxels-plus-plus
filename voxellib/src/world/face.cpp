#include <cassert>
#include "face.h"

void face_offset(Face face, int *out) {
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

void AmbientOcclusion::Builder::get_face_offsets(Face face, AmbientOcclusion::Builder::Vertex vertex,
                                                 Face &a_out, Face &b_out) {
    static const Face definitions[kFaceCount][8] = {
            // front
            {
                    kBottom, kLeft,
                    kBottom, kRight,
                    kTop,    kRight,
                    kTop,    kLeft
            },
            // left
            {
                    kBottom, kBack,
                    kBottom, kFront,
                    kTop,    kFront,
                    kTop,    kBack
            },
            // right
            {
                    kBottom, kFront,
                    kBottom, kBack,
                    kTop,    kBack,
                    kTop,    kFront
            },
            // top
            {
                    kFront,  kLeft,
                    kFront,  kRight,
                    kBack,   kRight,
                    kBack,   kLeft,
            },
            // bottom
            {
                    kBack,   kLeft,
                    kBack,   kRight,
                    kFront,  kRight,
                    kFront,  kLeft
            },
            // back
            {
                    kTop,    kLeft,
                    kTop,    kRight,
                    kBottom, kRight,
                    kBottom, kLeft
            },
    };

    const Face *def = definitions[face];
    a_out = def[(2 * vertex) + 0];
    b_out = def[(2 * vertex) + 1];
}

void AmbientOcclusion::Builder::set_vertex(Face face, AmbientOcclusion::Builder::Vertex vertex, bool s1, bool s2,
                                           bool corner) {
    int val = (s1 && s2 ? kVertexFull : (kVertexNone - (s1 + s2 + corner)));
    values_[face][vertex] = val;
}

void AmbientOcclusion::Builder::build(AmbientOcclusion &out) const {
    for (Face face : kFaces) {
        const int *values = values_[face];
        unsigned long byte =
                values[0] << 0 | // v05
                values[1] << 2 | // v1
                values[2] << 4 | // v23
                values[3] << 6;  // v4

        unsigned long face_shift = face * 8;

        unsigned long clear_mask = 0xffUL << face_shift;
        unsigned long new_value = byte << face_shift;
        out.bits_ = (out.bits_ & ~clear_mask) | new_value;
    }

}

AmbientOcclusion::Builder::Builder() {
    for (Face face : kFaces) {
        for (int vertex = 0; vertex < kVertexCount; ++vertex) {
            values_[face][vertex] = kVertexFull;
        }
    }
}

void AmbientOcclusion::Builder::set_brightest() {
    for (Face face : kFaces) {
        for (int vertex = 0; vertex < kVertexCount; ++vertex) {
            values_[face][vertex] = kVertexNone;
        }
    }
}

float AmbientOcclusion::get_vertex(Face face, int vertex) const {
    int real_idx;
    switch (vertex) {
        case 0:
        case 5:
            real_idx = 0;
            break;
        case 1:
            real_idx = 1;
            break;
        case 2:
        case 3:
            real_idx = 2;
            break;
        case 4:
            real_idx = 3;
            break;
        default:
            assert(false); // vertex must be < 6
            break;
    }

    int byte_mask_shift = face * 8;
    unsigned long byte_mask = (1 << 8) - 1;
    unsigned long shifted_mask = byte_mask << byte_mask_shift;
    unsigned long byte = (bits_ & shifted_mask) >> byte_mask_shift;

    unsigned int vertex_shift = real_idx * 2;
    unsigned int vertex_mask = 3 << vertex_shift;

    unsigned long result = (byte & vertex_mask) >> vertex_shift;

    static const float curve[] = {0, 0.6, 0.8, 1};
    assert(result < 4);
    float out = curve[result];
    return out;
}
