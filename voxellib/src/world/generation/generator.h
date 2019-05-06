#ifndef VOXELS_GENERATOR_H
#define VOXELS_GENERATOR_H


#include <world/chunk.h>

class IGenerator {
public:
    virtual int generate(ChunkId_t chunk_id, ChunkTerrain *terrain_out) = 0;
};

class DummyGenerator : public IGenerator {
    int generate(ChunkId_t chunk_id, ChunkTerrain *terrain_out) override;
};


#endif
