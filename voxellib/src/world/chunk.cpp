#include <GL/glew.h>
#include <glm/vec3.hpp>
#include "chunk.h"
#include "world_renderer.h"
#include "face.h"
#include "util.h"
#include "centre.h"


Chunk::Chunk(ChunkId_t id) : id_(id), mesh_(id) {

}
// TODO is this even needed?
bool Chunk::loaded() const {
    // TODO what if mesh is empty because its invisible so mesh is 0?
    return /*terrain_.size() > 0 && */mesh_.has_mesh();
}

unsigned long Chunk::populate_mesh(ChunkMeshRaw &mesh) {
    ChunkTerrain::BlockCoord block_pos;
    size_t out_idx = 0;

    for (int block_idx = 0; block_idx < kBlocksPerChunk; ++block_idx) {
        const Block &block = terrain_[block_idx];
        // cull if totally occluded
        if (block.face_visibility_.invisible())
            continue;

        // cull air blocks
        if (block.type_ == BlockType::kAir)
            continue;

        terrain_.expand(block_idx, block_pos);

        for (Face face : Face::kFaces) {
            // cull face if not visible
            if (!block.face_visibility_.visible(face))
                continue;

            int stride = 6 * 3; // 6 vertices * 3 floats per face (TODO add normals)
            const float *verts = kBlockVertices + (stride * *face);

            union {
                float f;
                int i;
            } f_or_i{0};

            for (int v = 0; v < Face::kCount; ++v) {
                // vertex pos in chunk space
                int v_idx = v * 3;
                for (int j = 0; j < 3; ++j) {
                    f_or_i.f = verts[v_idx + j] + block_pos[j] * 2 * kBlockRadius;
                    mesh[out_idx++] = f_or_i.i;
                }
                // colour
                unsigned int colour = block.type_.colour();
                mesh[out_idx++] = colour;

                // ao
                float ao = block.ao_.get_vertex(face, v);
                f_or_i.f = ao;
                mesh[out_idx++] = f_or_i.i;

                assert(out_idx < kChunkMeshSize);
            }
        }
    }

    DLOG_F(INFO, "%s: new mesh is size %lu/%d", CHUNKSTR(this), out_idx, kChunkMeshSize);
    return out_idx;
}

ChunkId_t Chunk::owning_chunk(const glm::ivec3 &block_pos) {
    return ChunkId(
            block_pos.x >> kChunkWidthShift,
            block_pos.z >> kChunkDepthShift
    );
}

void Chunk::post_terrain_update() {
    // TODO dont do this if loaded from file and already populated
    terrain_.update_face_visibility();
    terrain_.populate_neighbour_opacity();
}

bool Chunk::merge_faces_with_neighbour(Chunk *neighbour_chunk, ChunkNeighbour side) {
    bool should_merge = !terrain_.has_merged_faces(side);
    if (should_merge) {
        DLOG_F(INFO, "merging %s's faces with %s on side %d", CHUNKSTR(this), CHUNKSTR(neighbour_chunk), *side);
        terrain_.merge_faces(neighbour_chunk->terrain_, side);
    }

    return should_merge;
}

void Chunk::neighbours(ChunkNeighbours &out) const {
    int x, z;
    ChunkId_deconstruct(id_, x, z);

    out = {
            ChunkId(x - 1, z), // front
            ChunkId(x, z - 1), // left
            ChunkId(x, z + 1), // right
            ChunkId(x + 1, z), // back
    };
}

void Chunk::reset_for_cache() {
    terrain_.reset_merged_faces();
}

void Chunk::mark_load_time_now() {
    load_time_ = boost::posix_time::microsec_clock::local_time();
}

bool Chunk::was_loaded_before(boost::posix_time::ptime barrier) const {
    return load_time_ <= barrier;
}


bool WorldCentre::chunk(ChunkId_t &chunk_out) {
    auto current_chunk = Chunk::owning_chunk(Block::from_world_pos(pos_));
    bool changed = current_chunk != last_chunk_;

    last_chunk_ = current_chunk;
    chunk_out = current_chunk;

    return changed;
}

ChunkMesh::ChunkMesh(ChunkId_t chunk_id) : mesh_(nullptr), dirty_(true) {
    ChunkId_deconstruct(chunk_id, x_, z_);
}

ChunkMeshRaw *ChunkMesh::steal_mesh() {
    auto tmp = mesh_;
    mesh_ = nullptr;
    mesh_size_ = 0;
    return tmp;
}

bool ChunkMesh::prepare_render() {
    if (mesh_ == nullptr) {
        LOG_F(WARNING, "mesh for %d, %d is missing, skipping", x_, z_);
        return false;
    }

    if (vao_ == 0 || vbo_ == 0) {
        glGenBuffers(1, &vbo_);
        glGenVertexArrays(1, &vao_);
    }

    if (dirty_) {
        dirty_ = false;
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, mesh_size_ * sizeof(int), mesh_, GL_STATIC_DRAW);
    }

    return true;
}

ChunkMeshRaw *ChunkMesh::on_mesh_update(size_t new_size, ChunkMeshRaw *new_mesh) {
    mesh_size_ = new_size;
    dirty_ = true;
    if (new_mesh != nullptr) {
        ChunkMeshRaw *old = mesh_;
        mesh_ = new_mesh;
        return old;
    }

    return nullptr;
}

void ChunkMesh::world_offset(glm::ivec3 &out) {
    out[0] = x_ * kChunkWidth * kBlockRadius * 2;
    out[1] = 0;
    out[2] = z_ * kChunkDepth * kBlockRadius * 2;
}

