#include <GL/glew.h>
#include <glm/vec3.hpp>
#include "chunk.h"
#include "world_renderer.h"
#include "face.h"
#include "util.h"

Chunk::Chunk(int32_t x, int32_t z, ChunkMeshRaw *mesh) : id_(ChunkId(x, z)), mesh_(mesh) {

}

bool Chunk::loaded() const {
    // TODO what if mesh is empty because its invisible so mesh is 0?
    return terrain_.size() > 0 && mesh_.has_mesh();
}

void Chunk::lazily_init_render_buffers() {
    if (mesh_.vao_ == 0 || mesh_.vbo_ == 0) {
        glGenBuffers(1, &mesh_.vbo_);
        glGenVertexArrays(1, &mesh_.vao_);

        if (mesh_.mesh_ == nullptr)
            throw std::runtime_error("expected mesh to be non-null because chunk is loaded!");

        glBindBuffer(GL_ARRAY_BUFFER, mesh_.vbo_);
        glBufferData(GL_ARRAY_BUFFER, mesh_.mesh_size_ * sizeof(int), mesh_.mesh_, GL_STATIC_DRAW);
    }
}

void Chunk::world_offset(glm::ivec3 &out) {
    int x, z;
    ChunkId_deconstruct(id_, x, z);
    out[0] = x * kChunkWidth * kBlockRadius * 2;
    out[1] = 0;
    out[2] = z * kChunkDepth * kBlockRadius * 2;
}

void Chunk::populate_mesh() {
    glm::ivec3 block_pos;
    size_t out_idx = 0;

    for (int block_idx = 0; block_idx < kBlocksPerChunk; ++block_idx) {
        const Block &block = block_from_index(block_idx);
        // cull if totally occluded
        if (block.face_visibility_ == kFaceVisibilityNone)
            continue;

        // cull air blocks
        if (block.type_ == BlockType::kAir)
            continue;

        expand_block_index(block_idx, block_pos);

        for (int face_idx = 0; face_idx < kFaceCount; ++face_idx) {
            auto face = kFaces[face_idx];

            // cull face if not visible
            if (!face_is_visible(block.face_visibility_, face))
                continue;

            int stride = 6 * 3; // 6 vertices * 6 floats per face
            const float *verts = kBlockVertices + (stride * (int) face);

            union {
                float f;
                int i;
            } f_or_i;

            for (int v = 0; v < 6; ++v) {
                // vertex pos in chunk space
                int v_idx = v * 3;
                for (int j = 0; j < 3; ++j) {
                    f_or_i.f = verts[v_idx + j] + block_pos[j] * 2 * kBlockRadius;
                    (*mesh_.mesh_)[out_idx++] = f_or_i.i;
                }
                // colour
                int colour = kBlockTypeColours[static_cast<int>(block.type_)];
                (*mesh_.mesh_)[out_idx++] = colour;
                assert(out_idx < kChunkMeshSize);
/*
                // ao
                char ao_idx = ao_get_vertex(block.ao, face, v);
                float ao = AO_CURVE[ao_idx];
                f_or_i.f = ao;
                buffer[out_idx++] = f_or_i.i;*/
            }
        }
    }

    mesh_.mesh_size_ = out_idx;
    LOG_F(INFO, "new mesh is size %lu/%d", out_idx, kChunkMeshSize);
}

const Block &Chunk::block_from_index(unsigned long index) const {
    return terrain_[terrain_.unflatten(index)];
}

void Chunk::expand_block_index(const ChunkTerrain &terrain, int idx, glm::ivec3 &out) {
    auto coord = terrain.unflatten(idx);
    out[0] = coord[0];
    out[1] = coord[1];
    out[2] = coord[2];
}

void Chunk::expand_block_index(int idx, glm::ivec3 &out) const {
    Chunk::expand_block_index(terrain_, idx, out);
}

ChunkId_t Chunk::owning_chunk(const glm::ivec3 &block_pos) {
    return ChunkId(
            block_pos.x >> kChunkWidthShift,
            block_pos.z >> kChunkDepthShift
    );
}

bool WorldCentre::chunk(ChunkId_t &chunk_out) {
    auto current_chunk = Chunk::owning_chunk(Block::from_world_pos(pos_));
    bool changed = current_chunk != last_chunk_;

    last_chunk_ = current_chunk;
    chunk_out = current_chunk;

    return changed;
}


ChunkMesh::ChunkMesh(ChunkMeshRaw *mesh) : mesh_(mesh) {}

ChunkMeshRaw * ChunkMesh::steal_mesh() {
    auto tmp = mesh_;
    mesh_ = nullptr;
    return tmp;
}
