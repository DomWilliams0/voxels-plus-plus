#ifndef VOXELS_CHUNK_H
#define VOXELS_CHUNK_H

#include <cstdint>
#include <array>
#include <GL/gl.h>
#include <boost/thread/shared_mutex.hpp>
#include "glm/vec3.hpp"

#include "multidim_grid.hpp"
#include "block.h"
#include "constants.h"
#include "chunk_load/state.h"
#include "terrain.h"


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
    int x, z;
    ChunkId_deconstruct(id, x, z);
    std::ostringstream s;
    s << "(" << x << ", " << z << ")";
    return s.str();
}

// helper
#define CHUNKSTR(c) (ChunkId_str(c->id()).c_str())


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

typedef std::array<ChunkId_t, ChunkNeighbour::kCount> ChunkNeighbours;

class Chunk {
public:
    Chunk(ChunkId_t id, ChunkMeshRaw *mesh);

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
     * @param block_pos Global block pos
     * @return Chunk that owns it
     */
    static ChunkId_t owning_chunk(const glm::ivec3 &block_pos);

    /**
     * Lazy, will only init if not set
     */
    void lazily_init_render_buffers();

    void neighbours(ChunkNeighbours &out) const;

    ChunkState get_state();

    void set_state(ChunkState state);

    void post_terrain_update();

    /**
     * @param side from the perspective of this
     * @return true if a merge was done, false if it has already been done
     */
    bool merge_faces_with_neighbour(Chunk *neighbour_chunk, ChunkNeighbour side);

    /**
     * @param alternate If not null, is swapped with current mesh
     * @return Old mesh if swapped, otherwise null
     */
    ChunkMeshRaw *populate_mesh(ChunkMeshRaw *alternate);
private:
    ChunkId_t id_;

    ChunkTerrain terrain_;
    ChunkMesh mesh_;

    ChunkState state_;
    boost::shared_mutex state_lock_;

    friend class IGenerator; // to allow direct access to terrain_

    void post_set_state(ChunkState prev_state, ChunkState new_state);
};


#endif
