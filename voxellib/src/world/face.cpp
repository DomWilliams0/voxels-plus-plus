#include <cassert>
#include <constants.h>
#include "face.h"

void face_offset_with_copy(Face face, const int *in, int *out) {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    face_offset(face, in, out);
}

void face_offset(Face face, const int *in, int *out) {
    switch (face) {
        case kFront:
            // -x
            out[0] = in[0] - 1;
            break;
        case kLeft:
            // -z
            out[2] = in[2] - 1;
            break;
        case kRight:
            // +z
            out[2] = in[2] + 1;
            break;
        case kTop:
            // +y
            out[1] = in[1] + 1;
            break;
        case kBottom:
            // -y
            out[1] = in[1] - 1;
            break;
        case kBack:
            // +x
            out[0] = in[0] + 1;
            break;
    }
}

void face_offset(Face face, int *out) {
    face_offset(face, out, out);
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
    auto &value = values_[face];
    value.dirty_ = true;
    value.vertices_[vertex] = val;
}

void AmbientOcclusion::Builder::build(AmbientOcclusion &out) const {
    for (Face face : kFaces) {
        auto value = values_[face];
        if (!value.dirty_)
            continue;

        const int *vertices = value.vertices_;
        unsigned long byte =
                vertices[0] << 0 | // v05
                vertices[1] << 2 | // v1
                vertices[2] << 4 | // v23
                vertices[3] << 6;  // v4

        unsigned long face_shift = face * 8;

        unsigned long clear_mask = 0xffUL << face_shift;
        unsigned long new_value = byte << face_shift;
        out.bits_ = (out.bits_ & ~clear_mask) | new_value;
    }

}

void AmbientOcclusion::Builder::set_brightest() {
    for (Face face : kFaces) {
        auto &val = values_[face];
        val.dirty_ = true;
        for (int vertex = 0; vertex < kVertexCount; ++vertex) {
            val.vertices_[vertex] = kVertexNone;
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
