#ifndef VOXELS_BLOCK_H
#define VOXELS_BLOCK_H

enum class BlockType {
    kAir = 0,
    kThing, // bah
};

const static long kBlockTypeColours[] = {
        0xfffffff,  // kAir
        0xff577fda, // kThing
};


struct Block {
    BlockType type;
};

const float kBlockSize = 0.5f;

#endif
