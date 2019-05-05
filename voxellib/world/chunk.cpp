#include <GL/glew.h>
#include "chunk.h"

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

void Chunk::world_offset(glm::vec3 &out) {
    out[0] = x_ * kChunkWidth * kBlockSize * 2;
    out[1] = 0;
    out[2] = z_ * kChunkDepth * kBlockSize * 2;
}

