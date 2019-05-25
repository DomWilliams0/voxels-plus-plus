#ifndef VOXELS_PROCGEN_H
#define VOXELS_PROCGEN_H

#include "world/chunk.h"

extern "C" {


typedef int (*generate_t)(int, int, int, ChunkTerrain &);

int generate(int chunk_x, int chunk_z, int seed, ChunkTerrain &terrain_out);

}


#endif
