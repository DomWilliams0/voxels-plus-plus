#ifndef VOXELS_BLOCK_H
#define VOXELS_BLOCK_H

enum class BlockType {
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


/**
 * blocks per m
 */
const int kBlockScale = 2;

// /2 again because radius, not diameter
const float kBlockRadius = 1.f / kBlockScale / 2.f;

struct Block {
    BlockType type;

    inline static glm::ivec3 from_world_pos(const glm::vec3 &world_pos) {
        return {
                world_pos.x * kBlockScale,
                world_pos.y * kBlockScale,
                world_pos.z * kBlockScale,
        };
    }
};

#endif
