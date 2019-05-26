#ifndef VOXELS_GENERATOR_H
#define VOXELS_GENERATOR_H

#include <world/chunk.h>
#include <boost/thread/shared_mutex.hpp>
#include "../../../procgen/src/procgen.h"

class IGenerator {
public:
    virtual int generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) = 0;
};

class DummyGenerator : public IGenerator {
public:
    int generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) override;
};

class PythonGenerator : public IGenerator {
public:
    int generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) override;

private:
    int get_socket();

    int sock_;

};

class NativeGenerator : public IGenerator {
public:
    int generate(ChunkId_t chunk_id, int seed, ChunkTerrain &terrain_out) override;

    static void mark_dirty();

private:
    static int ensure_handle();

    static boost::shared_mutex kHandleMutex;
    static bool kDirty;
    static void *kHandle;
    static generate_t kFunc;
};


#endif
