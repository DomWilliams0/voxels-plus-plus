#include <boost/algorithm/clamp.hpp>
#include "FastNoise/FastNoise.h"
#include "procgen.h"
#include "util.h"

static FastNoise noise_(10);

int __attribute__((constructor)) init() {
    noise_.SetNoiseType(FastNoise::NoiseType::Perlin);
    noise_.SetFractalOctaves(5);
    // TODO other noise config
    return 0;
}

int generate(int chunk_x, int chunk_z, int seed, ChunkTerrain &terrain_out) {
    const double scale = 3.0;

    for (unsigned int x = 0; x < kChunkWidth; x++) {
        for (unsigned int z = 0; z < kChunkDepth; z++) {
            int nx = (chunk_x * kChunkWidth) + x;
            int nz = (chunk_z * kChunkDepth) + z;
            double n = noise_.GetPerlin(nx / scale, seed, nz / scale) + 0.7;

            int top = (int) boost::algorithm::clamp(n * kChunkHeight, 1, kChunkHeight - 1);
            for (unsigned int y = top; y > 0; y--) {
                terrain_out[{x, y, z}] = static_cast<BlockType>((rand() % 3) + 1);
            }
        }
    }

    return 0;
}
