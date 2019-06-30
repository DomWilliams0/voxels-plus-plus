#ifndef VOXELS_FACE_H
#define VOXELS_FACE_H

#include <array>
#include <bitset>

class Face {
public:
    enum Value : uint8_t {
        kFront = 0,
        kLeft,
        kRight,
        kTop,
        kBottom,
        kBack,
    };

    Face() = default;

    constexpr explicit Face(Value val) : value_(val) {}

    constexpr Face(int val) : value_(static_cast<Value>(val)) {}

    constexpr bool operator==(Face o) const { return value_ == o.value_; }

    constexpr bool operator!=(Face o) const { return value_ != o.value_; }

    constexpr Value operator*() const { return value_; }

    constexpr static int kCount = 6;

    constexpr static Value kFaces[kCount] = {kFront, kLeft, kRight, kTop, kBottom, kBack};

    void offset(int *out) const;

    void offset(const int *in, int *out) const;

    void offset_with_copy(const int *in, int *out) const;


private:
    Value value_;

};

class FaceVisibility : std::bitset<Face::kCount> {
public:
    FaceVisibility();

    bool visible(const Face &face) const;

    void set_fully_visible();

    void set_invisible();

    void set_face_visible(const Face &face, bool visible);

    // no faces visible
    bool invisible() const;

};

// ambient occlusion
// 2 bits per 4 unique vertices per 6 faces
const unsigned int kAoBitCount = 2 * 4 * 6;

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

        static void get_face_offsets(const Face &face, Vertex vertex, Face &a_out, Face &b_out);

        void set_vertex(const Face &face, AmbientOcclusion::Builder::Vertex vertex, bool s1, bool s2, bool corner);

        // write out bits to AO
        void build(AmbientOcclusion &out) const;

        void set_brightest();

    private:
        struct {
            bool dirty_ = false;
            unsigned int vertices_[kVertexCount] = {0};
        } values_[Face::kCount];

    };

private:
    unsigned long bits_ : kAoBitCount;

    // block level
    const static unsigned long kNone = 0;
    const static unsigned long kAll = ((1UL << kAoBitCount) - 1);

    // vertex level
    const static int kVertexNone = 3; // both bits set
    const static int kVertexFull = 0; // both bits clear
};


#endif
