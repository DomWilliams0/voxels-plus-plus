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


struct Block {
    BlockType type;
};

const float kBlockSize = 0.1f;

#endif
