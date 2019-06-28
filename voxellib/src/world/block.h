#ifndef VOXELS_BLOCK_H
#define VOXELS_BLOCK_H

#include <glm/vec3.hpp>
#include "face.h"
#include "constants.h"

// TODO upgrade enum
enum class BlockType : int8_t {
    kAir = 0,
    kGrass,
    kStone,
    kDarkStone,
    kMarker,
};

const static long kBlockTypeColours[] = {
        0xfffffff,  // kAir
        0xff007815, // kGrass
        0xff828282, // kStone
        0xff444444, // kDarkStone
        0xffff0d00, // kMarker
};

inline bool BlockType_opaque(BlockType bt) {
    return bt != BlockType::kAir;
}

struct Block {
    BlockType type_;
    FaceVisibility face_visibility_ {};
    AmbientOcclusion ao_ {};

    Block(BlockType type = BlockType::kAir) : type_(type) {}

    inline static glm::ivec3 from_world_pos(const glm::vec3 &world_pos) {
        return {
                world_pos.x * kBlockScale,
                world_pos.y * kBlockScale,
                world_pos.z * kBlockScale,
        };
    }
};

#endif
