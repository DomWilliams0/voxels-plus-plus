#ifndef VOXELS_BLOCK_H
#define VOXELS_BLOCK_H

#include "face.h"
#include "constants.h"

enum class BlockType : int8_t {
    kAir = 0,
    kGrass,
    kStone,
    kMarker,
};

const static long kBlockTypeColours[] = {
        0xfffffff,  // kAir
        0xff007815, // kGrass
        0xff828282, // kStone
        0xffff0d00, // kMarker
};

inline bool BlockType_opaque(BlockType bt) {
    return bt != BlockType::kAir;
}

struct Block {
    BlockType type_;
    FaceVisibility face_visibility_ : kFaceCount;

    Block(BlockType type = BlockType::kAir, FaceVisibility face_visibility = kFaceVisibilityNone)
            : type_(type), face_visibility_(face_visibility) {}

    inline static glm::ivec3 from_world_pos(const glm::vec3 &world_pos) {
        return {
                world_pos.x * kBlockScale,
                world_pos.y * kBlockScale,
                world_pos.z * kBlockScale,
        };
    }

    // helper
    inline void set_face_visible(Face f, bool visible) {
        if (visible)
            face_visibility_ &= ~face_visibility(f);
        else
            face_visibility_ |= face_visibility(f);
    }
};

#endif
