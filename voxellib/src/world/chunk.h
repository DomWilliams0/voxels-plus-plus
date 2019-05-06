#ifndef VOXELS_CHUNK_H
#define VOXELS_CHUNK_H

#include <cstdint>
#include <GL/gl.h>
#include "glm/ext/vector_int3.hpp"
#include "glm/vec2.hpp"

#include "multidim_grid.hpp"
#include "block.h"

const int kChunkWidthShift = 4; // 16
const int kChunkHeightShift = 4; // 16 TODO change to 10 = 1024
const int kChunkDepthShift = 4; // 16

const int kChunkWidth = 1 << kChunkWidthShift;
const int kChunkHeight = 1 << kChunkHeightShift;
const int kChunkDepth = 1 << kChunkDepthShift;

typedef uint64_t ChunkId_t;

inline ChunkId_t ChunkId(int32_t x, int32_t z) {
    return ((int64_t) x << 32) | z;
}

const ChunkId_t kChunkIdInit = INT64_MAX;

inline void ChunkId_deconstruct(ChunkId_t c_id, int32_t &x, int32_t &z) {
    x = c_id >> 32;
    z = c_id & ((1L << 32) - 1);
}

class ChunkMesh {

public:
    inline int mesh_size() const { return mesh_size_; }

private:
    int *mesh_ = nullptr;
    unsigned int mesh_size_ = 0;

    GLuint vao_ = 0, vbo_ = 0;

    friend class Chunk;
};


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
     * @param world_block_pos Global block pos
     * @return chunk that owns it
     */
    static ChunkId_t owning_chunk_coord(const glm::ivec3 &world_block_pos);

private:
    int32_t x_, z_;

    // TODO subchunks
    multidim::Grid<Block, kChunkWidth, kChunkHeight, kChunkDepth> terrain_;
    ChunkMesh mesh_;

    friend class World;

    /**
     * Lazy, will only init if not set
     */
    void lazily_init_render_buffers();

    void generate_mesh();

    const Block &block_from_index(unsigned long index) const;

    void expand_block_index(int idx, glm::ivec3 &out) const;
};


#endif
