#ifndef VOXELS_LOADER_H
#define VOXELS_LOADER_H

#include <world/generation/generator.h>
#include "chunk.h"

// should live in chunk requester thread
class WorldLoader {

public:
    WorldLoader(IGenerator *generator) : generator_(generator) {}

    /**
     * Will either load from disk, load from chunk cache or generate from scratch
     * Blocks
     */
    int load(ChunkId_t chunk_id, ChunkTerrain *terrain_out);

private:
    IGenerator *generator_;

};

// TEMPORARY global singleton (nice buzzwords)
extern WorldLoader kWorldLoader;

#endif
