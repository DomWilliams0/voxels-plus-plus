#ifndef VOXELS_CHUNK_H
#define VOXELS_CHUNK_H

#include <cstdint>
#include <GL/gl.h>
#include "glm/ext/vector_int3.hpp"

#include "multidim_grid.hpp"
#include "block.h"

const int kChunkWidth = 16;
const int kChunkHeight = 16; // TODO 1024
const int kChunkDepth = 16;

typedef uint64_t ChunkId_t;

inline ChunkId_t ChunkId(int32_t x, int32_t z) {
    return ((int64_t) x << 32) | z;
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

    bool loaded() const;

    void world_offset(glm::ivec3 &out);

    inline int vertex_count() const { return mesh_.mesh_size_; }

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
