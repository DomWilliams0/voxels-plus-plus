#ifndef VOXELS_CHUNK_H
#define VOXELS_CHUNK_H

#include <cstdint>
#include <array>
#include <GL/gl.h>
#include <bitset>
#include <sstream>
#include "glm/vec3.hpp"

#include "multidim_grid.hpp"
#include "block.h"
#include "constants.h"


typedef uint64_t ChunkId_t;

inline ChunkId_t ChunkId(int32_t x, int32_t z) {
    return ((uint64_t) x << 32) | (uint32_t) z;
}
const ChunkId_t kChunkIdInit = UINT64_MAX;

inline void ChunkId_deconstruct(ChunkId_t c_id, int32_t &x, int32_t &z) {
    x = c_id >> 32;
    z = c_id & ((1L << 32) - 1);
}

inline std::string ChunkId_str(ChunkId_t id) {
    int x,z;
    ChunkId_deconstruct(id, x, z);
    std::ostringstream s;
    s << "(" << x << ", " << z << ")";
    return s.str();
}


typedef std::array<int32_t, kChunkMeshSize> ChunkMeshRaw;

class ChunkMesh {

public:
    ChunkMesh(ChunkMeshRaw *mesh);

    ~ChunkMesh();

    inline int mesh_size() const { return mesh_size_; }

    inline bool has_mesh() const { return mesh_ != nullptr; }

    // takes ownership of mesh, sets field to null
    ChunkMeshRaw *steal_mesh();

private:
    ChunkMeshRaw *mesh_;
    unsigned int mesh_size_ = 0;

    GLuint vao_ = 0, vbo_ = 0;

    friend class Chunk;
};

const int kChunkNeighbourCount = 4;
enum class ChunkNeighbour { // bit mask
    kFront = 1,
    kLeft = 2,
    kRight = 4,
    kBack = 8,
};

// "ChunkNeighbour.values()"
static const std::array<ChunkNeighbour , kChunkNeighbourCount> kChunkNeighbourValues = {
        ChunkNeighbour::kFront,
        ChunkNeighbour::kLeft,
        ChunkNeighbour::kRight,
        ChunkNeighbour::kBack,
};

typedef std::array<ChunkId_t, kChunkNeighbourCount> ChunkNeighbours;

class ChunkNeighbourMask {
public:
    ChunkNeighbourMask();

    void set(ChunkNeighbour neighbour, bool set);

    unsigned int mask() const;

    void update_load_range(int my_x, int my_z, int centre_x, int centre_z, int load_radius);

    static const int kComplete;

private:
    std::bitset<kChunkNeighbourCount> real_;
    std::bitset<kChunkNeighbourCount> out_of_range_;
};

typedef multidim::Grid<Block, kChunkWidth, kChunkHeight, kChunkDepth> ChunkTerrain;

class Chunk {
public:
    /**
     * @param x Chunk world x coord
     * @param z Chunk world z coord
     */
    Chunk(int32_t x, int32_t z, ChunkMeshRaw *mesh);

    inline ChunkId_t id() const { return id_; }

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

    inline ChunkMeshRaw *steal_mesh() { return mesh_.steal_mesh(); }

    /**
     *
     * @param block_pos Global block pos
     * @return Chunk that owns it
     */
    static ChunkId_t owning_chunk(const glm::ivec3 &block_pos);

    static void expand_block_index(const ChunkTerrain &terrain, int idx, glm::ivec3 &out);

    /**
     * Lazy, will only init if not set
     */
    void lazily_init_render_buffers();

    void neighbours(ChunkNeighbours &out) const;

private:
    ChunkId_t id_;

    // TODO subchunks
    ChunkTerrain terrain_; // TODO move to a special heap instead of being inline
    ChunkMesh mesh_;
    ChunkNeighbourMask neighbour_mask_;

    friend class World;

    friend class WorldLoader;

    void populate_mesh();

    const Block &block_from_index(unsigned long index) const;

    void expand_block_index(int idx, glm::ivec3 &out) const;
};


#endif
