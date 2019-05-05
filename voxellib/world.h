#ifndef VOXELS_WORLD_H
#define VOXELS_WORLD_H

enum class BlockType {
    kAir = 0,
    kThing, // bah
};

struct Block {
    BlockType type;

};

class World {
public:
    World();

private:
    Block my_block;

};


#endif
