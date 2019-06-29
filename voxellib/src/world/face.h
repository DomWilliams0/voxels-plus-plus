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

void face_offset(Face face, int *out);

void face_offset_with_copy(Face face, const int *in, int *out);

void face_offset(Face face, const int *in, int *out);

Face face_opposite(Face face);

class FaceVisibility : std::bitset<kFaceCount> {
public:
    bool visible(Face face) const;

    void set_fully_visible();

    void set_face_visible(Face face, bool visible);

    // no faces visible
    bool invisible() const;

};

// ambient occlusion
// 2 bits per 4 unique vertices per 6 faces
const int kAoBitCount = 2 * 4 * 6;

class AmbientOcclusion {
public:
    AmbientOcclusion() : bits_(kAll) {}

    float get_vertex(Face face, int vertex) const;

    // TODO should flip quad(face)

    struct Builder {
        enum Vertex {
            kV05 = 0,
            kV1,
            kV23,
            kV4
        };
        const static int kVertexCount = 4;
        constexpr static Vertex kVertices[] = {kV05, kV1, kV23, kV4};

        static void get_face_offsets(Face face, Vertex vertex, Face &a_out, Face &b_out);

        void set_vertex(Face face, AmbientOcclusion::Builder::Vertex vertex, bool s1, bool s2,
                        bool corner);

        // write out bits to AO
        void build(AmbientOcclusion &out) const;

        void set_brightest();

    private:
        struct {
            bool dirty_ = 0;
            int vertices_[kVertexCount] = {0};
        } values_[kFaceCount];

    };

private:
    unsigned long bits_ : kAoBitCount;

    // block level
    const static unsigned long kNone = 0;
    const static unsigned long kAll = ((1L << kAoBitCount) - 1);

    // vertex level
    const static int kVertexNone = 3; // both bits set
    const static int kVertexFull = 0; // both bits clear
};


#endif
