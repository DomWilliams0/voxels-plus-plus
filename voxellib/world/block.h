#ifndef VOXELS_BLOCK_H
#define VOXELS_BLOCK_H

enum class BlockType {
    kAir = 0,
    kThing, // bah
};

struct Block {
    BlockType type;
};

const float kBlockSize = 0.5f;

#endif
