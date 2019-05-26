#ifndef VOXELS_TERRAIN_H
#define VOXELS_TERRAIN_H

#include <array>

#include "multidim_grid.hpp"
#include "block.h"

// TODO vertical subchunks
class ChunkTerrain {
public:
    typedef multidim::Grid<Block, kChunkWidth, kChunkHeight, kChunkDepth> GridType;
//    typedef std::array<ChunkTerrain::GridType::size_type, ChunkTerrain> BlockCoord;
    typedef ChunkTerrain::GridType::ArrayCoord BlockCoord;

    Block &operator[](unsigned int flat_index);

    Block &operator[](const GridType::ArrayCoord &coord);

    void expand(unsigned int index, BlockCoord &out);

    void update_face_visibility();

private:
    GridType grid_;
};


#endif
