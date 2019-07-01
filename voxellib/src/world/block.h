#ifndef VOXELS_BLOCK_H
#define VOXELS_BLOCK_H

#include <glm/vec3.hpp>
#include "face.h"
#include "constants.h"

class BlockType {
public:
    enum Value : uint8_t {
        kAir = 0,
        kGrass,
        kStone,
        kDarkStone,
        kSnow,
        kSand,
        kMarker,
    };

    BlockType() = default;

    constexpr BlockType(Value val) : value_(val) {}

    constexpr explicit BlockType(int val) : value_(static_cast<Value>(val)) {}

    constexpr bool operator==(BlockType o) const { return value_ == o.value_; }

    constexpr bool operator!=(BlockType o) const { return value_ != o.value_; }

    constexpr Value operator*() const { return value_; }

    unsigned int colour() const;

    bool opaque() const;

private:
    Value value_;
};

struct Block {
    BlockType type_;
    FaceVisibility face_visibility_;
    AmbientOcclusion ao_;

    explicit Block(BlockType type = BlockType::kAir) : type_(type) {}

    inline static glm::ivec3 from_world_pos(const glm::vec3 &world_pos) {
        return {
                world_pos.x * kBlockScale,
                world_pos.y * kBlockScale,
                world_pos.z * kBlockScale,
        };
    }
};

#endif
