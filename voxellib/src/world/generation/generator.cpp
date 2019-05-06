#include <error.h>
#include "generator.h"

int DummyGenerator::generate(ChunkId_t chunk_id, ChunkTerrain *terrain_out) {
    // ground
    for (size_t x = 0; x < kChunkWidth; ++x) {
        for (size_t z = 0; z < kChunkDepth; ++z) {
            for (size_t y = 0; y < 3; ++y) {
                Block &b = terrain_out->operator[]({x, y, z});
                b.type = x == 0 || x == kChunkWidth - 1 ||
                         z == 0 || z == kChunkDepth - 1 ? BlockType::kGrass : BlockType::kStone;
            }
        }
    }

    // TODO some seeded features


    return kErrorSuccess;
}
