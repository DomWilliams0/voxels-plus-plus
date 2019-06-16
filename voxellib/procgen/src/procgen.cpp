#include <boost/algorithm/clamp.hpp>
#include "FastNoise/FastNoise.h"
#include "procgen.h"
#include "util.h"

static thread_local FastNoise noise_(10);

int __attribute__((constructor)) init() {
    noise_.SetNoiseType(FastNoise::NoiseType::Perlin);
    noise_.SetFractalOctaves(5);
    // TODO other noise config
    return 0;
}

double perlin_scaled(int x, int z, int seed, double scale) {
    double n = noise_.GetPerlin(x / scale, seed, z / scale);
    double min = -0.8;
    double max = +0.8;

    double new_min = 0;
    double new_max = 1;
    return ((n - min) * (new_max - new_min)) / (max - min) + new_min;
}

int generate(int chunk_x, int chunk_z, int seed, ChunkTerrain &terrain_out) {
    const double scale = 0.9;

    for (unsigned int bx = 0; bx < kChunkWidth; bx++) {
        for (unsigned int bz = 0; bz < kChunkDepth; bz++) {
            int x = (chunk_x * kChunkWidth) + bx;
            int z = (chunk_z * kChunkDepth) + bz;

            double noise_height = perlin_scaled(x, z, seed, scale);
            double noise_block = perlin_scaled(x, z, seed + 10.0216, 2);

            BlockType bt;
            if (noise_height > 0.8)
                bt = BlockType::kDarkStone;
            else
                bt = noise_block < 0.5 ? BlockType::kStone : BlockType::kGrass;

            int top = (int) boost::algorithm::clamp(noise_height * kChunkHeight, 1, kChunkHeight - 1);
            for (unsigned int y = top; y > 0; y--)
                terrain_out[{bx, y, bz}].type_ = bt;
        }
    }

//    for (unsigned int y = 0; y < kChunkHeight; y++)
//        terrain_out[{0, y, 0}].type_ = BlockType::kMarker;

    return 0;
}
