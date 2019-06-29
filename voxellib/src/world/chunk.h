#ifndef VOXELS_CHUNK_H
#define VOXELS_CHUNK_H

#include <cstdint>
#include <array>
#include <boost/thread/shared_mutex.hpp>
#include "glm/vec3.hpp"
#include <atomic>

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

inline unsigned int loaded_radius_chunk_count(int radius) {
    return (2 * radius + 1) * (2 * radius + 1);
}

typedef std::array<int32_t, kChunkMeshSize> ChunkMeshRaw;

class ChunkMesh {

public:
    ChunkMesh(ChunkId_t chunk_id);

    inline int mesh_size() const { return mesh_size_; }

    inline bool has_mesh() const { return mesh_ != nullptr; }

    inline unsigned int vao() const { return vao_; }

    inline unsigned int vbo() const { return vbo_; }

    inline ChunkMeshRaw &mesh() { return *mesh_; }

    // must be run in main thread
    // returns if successful
    bool prepare_render();

    // new_mesh is optional, if non-null is swapped in and old mesh is returned
    ChunkMeshRaw *on_mesh_update(size_t new_size, ChunkMeshRaw *new_mesh);

    // takes ownership of mesh, sets field to null
    ChunkMeshRaw *steal_mesh();

    /**
     * @param out Set to the block pos of the bottom corner of this chunk
     */
    void world_offset(glm::ivec3 &out);

private:
    ChunkMeshRaw *mesh_;
    unsigned int mesh_size_ = 0;

    unsigned int vao_ = 0, vbo_ = 0;
    bool dirty_;

    // chunk coords
    int x_, z_;
};

typedef std::array<ChunkId_t, ChunkNeighbour::kCount> ChunkNeighbours;

class Chunk {
public:
    Chunk(ChunkId_t id);

    inline ChunkId_t id() const { return id_; }

    /**
     * @return If terrain and mesh are initialised
     */
    bool loaded() const;

    /**
     * @param block_pos Global block pos
     * @return Chunk that owns it
     */
    static ChunkId_t owning_chunk(const glm::ivec3 &block_pos);

    void neighbours(ChunkNeighbours &out) const;

    void post_terrain_update();

    /**
     * @param side from the perspective of this
     * @return true if a merge was done, false if it has already been done
     */
    bool merge_faces_with_neighbour(Chunk *neighbour_chunk, ChunkNeighbour side);

    /**
     */
    unsigned long populate_mesh(ChunkMeshRaw &mesh);

    void reset_for_cache();

    inline ChunkMesh *mesh() { return &mesh_; }

    // set load time to now
    void mark_load_time_now();

    bool was_loaded_before(boost::posix_time::ptime barrier) const;

private:
    ChunkId_t id_;
    boost::posix_time::ptime load_time_;

    ChunkTerrain terrain_;
    ChunkMesh mesh_;

    friend class IGenerator; // to allow direct access to terrain_
};


#endif
