#include <GL/glew.h>
#include <glm/vec3.hpp>
#include "chunk.h"
#include "world_renderer.h"
#include "face.h"

Chunk::Chunk(int32_t x, int32_t z) : x_(x), z_(z) {

}

bool Chunk::loaded() const {
    // TODO what if mesh is empty because its invisible so mesh is 0?
    return terrain_.size() > 0 && mesh_.mesh_size() > 0;
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
    out[0] = x_ * kChunkWidth * kBlockRadius * 2;
    out[1] = 0;
    out[2] = z_ * kChunkDepth * kBlockRadius * 2;
}

void Chunk::generate_mesh() {
    int *buffer = new int[kBlocksPerChunk * kChunkMeshWordsPerInstance * kChunkMeshVerticesPerBlock];
    glm::ivec3 block_pos;
    size_t out_idx = 0;

    for (int block_idx = 0; block_idx < kBlocksPerChunk; ++block_idx) {
        const Block &block = block_from_index(block_idx);

        // cull air blocks
        if (block.type == BlockType::kAir)
            continue;

        expand_block_index(block_idx, block_pos);

        for (int face_idx = 0; face_idx < kFaceCount; ++face_idx) {
            auto face = kFaces[face_idx];

//            if (!FACE_IS_VISIBLE(block.face_visibility, face))
//                continue;

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
                    buffer[out_idx++] = f_or_i.i;
                }
                // colour
                int colour = kBlockTypeColours[static_cast<int>(block.type)];
                buffer[out_idx++] = colour;
/*
                // ao
                char ao_idx = ao_get_vertex(block.ao, face, v);
                float ao = AO_CURVE[ao_idx];
                f_or_i.f = ao;
                buffer[out_idx++] = f_or_i.i;*/
            }
        }
    }

    mesh_.mesh_ = buffer;
    mesh_.mesh_size_ = out_idx;
}

const Block &Chunk::block_from_index(unsigned long index) const {
    return terrain_[terrain_.unflatten(index)];
}

void Chunk::expand_block_index(int idx, glm::ivec3 &out) const {
    auto coord = terrain_.unflatten(idx);
    out[0] = coord[0];
    out[1] = coord[1];
    out[2] = coord[2];
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
