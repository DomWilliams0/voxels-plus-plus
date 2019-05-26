#include <GL/glew.h>
#include <glm/vec3.hpp>
#include "chunk.h"
#include "world_renderer.h"
#include "face.h"
#include "util.h"
#include "centre.h"


Chunk::Chunk(ChunkId_t id, ChunkMeshRaw *mesh) : id_(id), mesh_(mesh) {

}

ChunkState Chunk::get_state() {
    boost::shared_lock lock(state_lock_);
    return state_;
}

void Chunk::post_set_state(ChunkState prev_state, ChunkState new_state) {
    if (prev_state != new_state) {
        DLOG_F(INFO, "set state %s : %s -> %s", CHUNKSTR(this), prev_state.str().c_str(), new_state.str().c_str());
    }
}

void Chunk::set_state(ChunkState state) {
    ChunkState old;
    {
        boost::unique_lock lock(state_lock_);
        old = state_;
        state_ = state;
    }

    post_set_state(old, state);
}

// TODO is this even needed?
bool Chunk::loaded() const {
    // TODO what if mesh is empty because its invisible so mesh is 0?
    return /*terrain_.size() > 0 && */mesh_.has_mesh();
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

ChunkMeshRaw *Chunk::swap_mesh_and_set_renderable(ChunkMeshRaw *new_mesh) {
    return nullptr;
}

ChunkMeshRaw *Chunk::populate_mesh(ChunkMeshRaw *alternate) {
    ChunkMeshRaw &mesh = alternate == nullptr ? *mesh_.mesh_ : *alternate;

    ChunkTerrain::BlockCoord block_pos;
    size_t out_idx = 0;

    for (int block_idx = 0; block_idx < kBlocksPerChunk; ++block_idx) {
        const Block &block = terrain_[block_idx];
        // cull if totally occluded
        if (block.face_visibility_ == kFaceVisibilityNone)
            continue;

        // cull air blocks
        if (block.type_ == BlockType::kAir)
            continue;

        terrain_.expand(block_idx, block_pos);

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
                    mesh[out_idx++] = f_or_i.i;
                }
                // colour
                int colour = kBlockTypeColours[static_cast<int>(block.type_)];
                mesh[out_idx++] = colour;
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
    DLOG_F(INFO, "new mesh is size %lu/%d", out_idx, kChunkMeshSize);

    ChunkMeshRaw *old_mesh = nullptr;
    ChunkState old_state;
    {
        // take lock
        boost::unique_lock lock(state_lock_);

        // swap meshes if necessary
        if (alternate != nullptr) {
            old_mesh = mesh_.mesh_;
            mesh_.mesh_ = alternate;
        }

        // set state
        old_state = state_;
        state_ = ChunkState::kRenderable;
    }

    post_set_state(old_state, ChunkState::kRenderable);
    return old_mesh;

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

void Chunk::merge_faces_with_neighbour(Chunk *neighbour_chunk, ChunkNeighbour side) {
    terrain_.merge_faces(neighbour_chunk->terrain_, side);
}

bool WorldCentre::chunk(ChunkId_t &chunk_out) {
    auto current_chunk = Chunk::owning_chunk(Block::from_world_pos(pos_));
    bool changed = current_chunk != last_chunk_;

    last_chunk_ = current_chunk;
    chunk_out = current_chunk;

    return changed;
}


ChunkMesh::ChunkMesh(ChunkMeshRaw *mesh) : mesh_(mesh) {}

ChunkMeshRaw *ChunkMesh::steal_mesh() {
    auto tmp = mesh_;
    mesh_ = nullptr;
    mesh_size_ = 0;
    return tmp;
}

ChunkMesh::~ChunkMesh() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

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
