#ifndef VOXELS_CHUNK_H
#define VOXELS_CHUNK_H

#include <cstdint>
#include <GL/gl.h>
#include "glm/vec3.hpp"

#include "multidim_grid.hpp"
#include "block.h"

const int kChunkWidthShift = 4; // 16
const int kChunkHeightShift = 6; // 16 TODO change to 10 = 1024
const int kChunkDepthShift = 4; // 16

const int kChunkWidth = 1 << kChunkWidthShift;
const int kChunkHeight = 1 << kChunkHeightShift;
const int kChunkDepth = 1 << kChunkDepthShift;

const int kBlocksPerChunk = kChunkWidth * kChunkHeight * kChunkDepth;


typedef uint64_t ChunkId_t;

inline ChunkId_t ChunkId(int32_t x, int32_t z) {
    return ((uint64_t) x << 32) | (uint32_t) z;
}

const ChunkId_t kChunkIdInit = UINT64_MAX;

inline void ChunkId_deconstruct(ChunkId_t c_id, int32_t &x, int32_t &z) {
    x = c_id >> 32;
    z = c_id & ((1L << 32) - 1);
}

class ChunkMesh {

public:
    inline int mesh_size() const { return mesh_size_; }

    inline bool has_mesh() const { return mesh_ != nullptr; }

private:
    int *mesh_ = nullptr;
    unsigned int mesh_size_ = 0;

    GLuint vao_ = 0, vbo_ = 0;

    friend class Chunk;
};

typedef multidim::Grid<Block, kChunkWidth, kChunkHeight, kChunkDepth> ChunkTerrain;

class Chunk {
public:
    /**
     * @param x Chunk world x coord
     * @param z Chunk world z coord
     */
    Chunk(int32_t x, int32_t z);

    inline ChunkId_t id() const { return ChunkId(x_, z_); }

    inline GLuint vao() const { return mesh_.vao_; }

    inline GLuint vbo() const { return mesh_.vbo_; }

    /**
     * @return If terrain and mesh are initialised
     */
    bool loaded() const;

    /**
     * @param out Set to the block pos of the bottom corner of this chunk
     */
    void world_offset(glm::ivec3 &out);

    inline int vertex_count() const { return mesh_.mesh_size_; }

    /**
     *
     * @param block_pos Global block pos
     * @return Chunk that owns it
     */
    static ChunkId_t owning_chunk(const glm::ivec3 &block_pos);

    static void expand_block_index(const ChunkTerrain &terrain, int idx, glm::ivec3 &out);

private:
    int32_t x_, z_;

    // TODO subchunks
    ChunkTerrain terrain_; // TODO move to a special heap instead of being inline
    ChunkMesh mesh_;

    friend class World;

    friend class WorldLoader;

    /**
     * Lazy, will only init if not set
     */
    void lazily_init_render_buffers();

    void generate_mesh();

    const Block &block_from_index(unsigned long index) const;

    void expand_block_index(int idx, glm::ivec3 &out) const;
};


#endif
