#include <cassert>
#include <constants.h>
#include "face.h"

void Face::offset(int *out) const {
    offset(out, out);
}

void Face::offset(const int *in, int *out) const {
    switch (value_) {
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

void Face::offset_with_copy(const int *in, int *out) const {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    offset(in, out);
}

FaceVisibility::FaceVisibility() {
    set_invisible();
}

bool FaceVisibility::visible(const Face &face) const {
    return operator[](*face);
}

void FaceVisibility::set_fully_visible() {
    set();
}

void FaceVisibility::set_face_visible(const Face &face, bool visible) {
    set(*face, visible);
}

bool FaceVisibility::invisible() const {
    return none();
}

void FaceVisibility::set_invisible() { reset(); }

void AmbientOcclusion::Builder::get_face_offsets(const Face &face, AmbientOcclusion::Builder::Vertex vertex,
                                                 Face &a_out, Face &b_out) {
    static const Face definitions[Face::kCount][8] = {
            // front
            {
                    Face::kBottom, Face::kLeft,
                    Face::kBottom, Face::kRight,
                    Face::kTop,    Face::kRight,
                    Face::kTop,    Face::kLeft
            },
            // left
            {
                    Face::kBottom, Face::kBack,
                    Face::kBottom, Face::kFront,
                    Face::kTop,    Face::kFront,
                    Face::kTop,    Face::kBack
            },
            // right
            {
                    Face::kBottom, Face::kFront,
                    Face::kBottom, Face::kBack,
                    Face::kTop,    Face::kBack,
                    Face::kTop,    Face::kFront
            },
            // top
            {
                    Face::kFront,  Face::kLeft,
                    Face::kFront,  Face::kRight,
                    Face::kBack,   Face::kRight,
                    Face::kBack,   Face::kLeft,
            },
            // bottom
            {
                    Face::kBack,   Face::kLeft,
                    Face::kBack,   Face::kRight,
                    Face::kFront,  Face::kRight,
                    Face::kFront,  Face::kLeft
            },
            // back
            {
                    Face::kTop,    Face::kLeft,
                    Face::kTop,    Face::kRight,
                    Face::kBottom, Face::kRight,
                    Face::kBottom, Face::kLeft
            },
    };

    const Face *def = definitions[*face];
    a_out = def[(2 * vertex) + 0];
    b_out = def[(2 * vertex) + 1];
}

void AmbientOcclusion::Builder::set_vertex(const Face &face, AmbientOcclusion::Builder::Vertex vertex,
                                           bool s1, bool s2, bool corner) {
    const unsigned int val = (s1 && s2 ? kVertexFull : (kVertexNone - (s1 + s2 + corner)));
    auto &value = values_[*face];
    value.dirty_ = true;
    value.vertices_[vertex] = val;
}

void AmbientOcclusion::Builder::build(AmbientOcclusion &out) const {
    for (Face face : Face::kFaces) {
        auto value = values_[*face];
        if (!value.dirty_)
            continue;

        const unsigned int *vertices = value.vertices_;
        unsigned long byte =
                vertices[0] << 0UL | // v05
                vertices[1] << 2UL | // v1
                vertices[2] << 4UL | // v23
                vertices[3] << 6UL;  // v4

        unsigned long face_shift = *face * 8;

        unsigned long clear_mask = 0xffUL << face_shift;
        unsigned long new_value = byte << face_shift;
        out.bits_ = (out.bits_ & ~clear_mask) | new_value;
    }

}

void AmbientOcclusion::Builder::set_brightest() {
    for (Face face : Face::kFaces) {
        auto &val = values_[*face];
        val.dirty_ = true;
        for (unsigned int &vertex : val.vertices_) {
            vertex = kVertexNone;
        }
    }
}

float AmbientOcclusion::get_vertex(Face face, int vertex) const {
    assert(vertex < 6);

    unsigned int real_idx;
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
            // caught by assert above already
            return 0;
    }

    unsigned int byte_mask_shift = *face * 8;
    unsigned long byte_mask = (1U << 8U) - 1;
    unsigned long shifted_mask = byte_mask << byte_mask_shift;
    unsigned long byte = (bits_ & shifted_mask) >> byte_mask_shift;

    unsigned int vertex_shift = real_idx * 2;
    unsigned int vertex_mask = 3U << vertex_shift;

    unsigned long result = (byte & vertex_mask) >> vertex_shift;

    static const float curve[] = {0, 0.6, 0.8, 1};
    assert(result < 4);
    float out = curve[result];
    return out;
}
