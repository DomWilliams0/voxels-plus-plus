#include "terrain.h"

ChunkNeighbour ChunkNeighbour::opposite() const {
    switch (this->value_) {
        case kFront:
            return kBack;
        case kLeft:
            return kRight;
        case kRight:
            return kLeft;
        case kBack:
            return kFront;
        default:
            assert(false);
    }
}

Block &ChunkTerrain::operator[](unsigned int flat_index) {
    return grid_[grid_.unflatten(flat_index)];
}

Block &ChunkTerrain::operator[](const GridType::ArrayCoord &coord) {
    return grid_[coord];
}

void ChunkTerrain::expand(unsigned int index, BlockCoord &out) {
    GridType::ArrayCoord expanded = grid_.unflatten(index);
    std::copy(expanded.cbegin(), expanded.cend(), out.begin());
}

bool ChunkTerrain::is_visible(const BlockCoord &src, Face face, bool bounds_check, bool *was_out_of_bounds) {
    BlockCoord offset(src);
    face_offset(face, offset.data());

    // check for out of bounds
    if (bounds_check && is_out_of_bounds(offset)) {
        // not visible for now, will be updated later
        *was_out_of_bounds = true;
        return false;
    }

    // safe to cast from signed to unsigned and deref now
    Block &offset_block = (*this)[{static_cast<unsigned long>(offset[0]),
                                   static_cast<unsigned long>(offset[1]),
                                   static_cast<unsigned long>(offset[2])}];
    return BlockType_opaque(offset_block.type_);
}


bool ChunkTerrain::is_out_of_bounds(const ChunkTerrain::BlockCoord &pos) {
    return pos[0] < 0 || pos[0] >= kChunkWidth || pos[2] < 0 || pos[2] >= kChunkDepth;
}

void ChunkTerrain::calculate_vertex_ao(AmbientOcclusion::Builder &ao, AmbientOcclusion::Builder::Vertex vertex,
                                       ChunkTerrain::BlockCoord offset_pos, Face face) {
    Face fa, fb;
    ao.get_face_offsets(face, vertex, fa, fb);

    BlockCoord fa_pos(offset_pos);
    face_offset(fa, fa_pos.data());

    bool abort = false;
    bool s1 = is_visible(offset_pos, fa, true, &abort);
    if (abort) return;

    bool s2 = is_visible(offset_pos, fb, true, &abort);
    if (abort) return;

    bool corner = is_visible(fa_pos, fb, false, NULL); // dont need to check corner for out of bounds
    ao.set_vertex(face, vertex, s1, s2, corner);
}

void ChunkTerrain::update_face_visibility() {
    BlockCoord pos;
    for (int i = 0; i < kBlocksPerChunk; ++i) {
        Block &b = (*this)[i];

        FaceVisibility &visibility = b.face_visibility_;
        AmbientOcclusion::Builder ao;

        if (!BlockType_opaque(b.type_)) {
            // fully visible and not occluded because transparent
            visibility.set_fully_visible();
            ao.set_brightest();
        } else {
            expand(i, pos);

            // check each face individually
            BlockCoord offset_pos;
            for (int j = 0; j < kFaceCount; ++j) {
                offset_pos = pos; // reset
                Face face = kFaces[j];
                face_offset(face, offset_pos.data());

                // facing top/bottom of world, so this face is visible
                if (offset_pos[1] < 0 || offset_pos[1] >= kChunkHeight) {
                    visibility.set_face_visible(face, true);
                    continue;
                }

                // faces chunk boundary, will be updated later
                if (is_out_of_bounds(offset_pos)) {
                    visibility.set_face_visible(face, true);
                    ao.set_brightest(); // looks funny if the edge of the world goes dark
                    continue;
                }

                // inside this chunk, safe to get the block type
                Block &offset_block = (*this)[{static_cast<unsigned long>(offset_pos[0]),
                                               static_cast<unsigned long>(offset_pos[1]),
                                               static_cast<unsigned long>(offset_pos[2])}];
                bool offset_opaque = BlockType_opaque(offset_block.type_);
                visibility.set_face_visible(face, !offset_opaque);

                // update ao for visible faces only
                if (!offset_opaque) {
                    calculate_vertex_ao(ao, AmbientOcclusion::Builder::kV05, offset_pos, face);
                    calculate_vertex_ao(ao, AmbientOcclusion::Builder::kV1, offset_pos, face);
                    calculate_vertex_ao(ao, AmbientOcclusion::Builder::kV23, offset_pos, face);
                    calculate_vertex_ao(ao, AmbientOcclusion::Builder::kV4, offset_pos, face);
                }
            }
        }

        ao.build(b.ao_);
    }

}

void ChunkTerrain::populate_neighbour_opacity() {
    // back: +x
    for (size_t y = 0; y < kChunkHeight; ++y) {
        for (size_t z = 0; z < kChunkDepth; ++z) {
            const size_t x = kChunkWidth - 1;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.back_[{y, z}] = BlockType_opaque(block.type_);
        }
    }

    // front: -x
    for (size_t y = 0; y < kChunkHeight; ++y) {
        for (size_t z = 0; z < kChunkDepth; ++z) {
            const size_t x = 0;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.front_[{y, z}] = BlockType_opaque(block.type_);
        }
    }

    // right: +z
    for (size_t x = 0; x < kChunkWidth; ++x) {
        for (size_t y = 0; y < kChunkHeight; ++y) {
            const size_t z = kChunkDepth - 1;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.right_[{x, y}] = BlockType_opaque(block.type_);
        }
    }

    // left: -z
    for (size_t x = 0; x < kChunkWidth; ++x) {
        for (size_t y = 0; y < kChunkHeight; ++y) {
            const size_t z = 0;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.left_[{x, y}] = BlockType_opaque(block.type_);
        }
    }
}

void ChunkTerrain::merge_faces(const ChunkTerrain &neighbour, ChunkNeighbour side) {
    // local copy to avoid constantly reading from neighbour
    auto n_opacity = neighbour.neighbour_opacity_;

    switch (*side) {
        case ChunkNeighbour::kBack:
            for (size_t y = 0; y < kChunkHeight; ++y) {
                for (size_t z = 0; z < kChunkDepth; ++z) {
                    const size_t x = kChunkWidth - 1;
                    bool n_opaque = n_opacity.front_[{y, z}];
                    (*this)[{x, y, z}].face_visibility_.set_face_visible(Face::kBack, !n_opaque);
                }
            }
            break;

        case ChunkNeighbour::kFront:
            for (size_t y = 0; y < kChunkHeight; ++y) {
                for (size_t z = 0; z < kChunkDepth; ++z) {
                    const size_t x = 0;
                    bool n_opaque = n_opacity.back_[{y, z}];
                    (*this)[{x, y, z}].face_visibility_.set_face_visible(Face::kFront, !n_opaque);
                }
            }
            break;

        case ChunkNeighbour::kRight:
            for (size_t x = 0; x < kChunkWidth; ++x) {
                for (size_t y = 0; y < kChunkHeight; ++y) {
                    const size_t z = kChunkDepth - 1;
                    bool n_opaque = n_opacity.left_[{x, y}];
                    (*this)[{x, y, z}].face_visibility_.set_face_visible(Face::kRight, !n_opaque);

                }
            }
            break;

        case ChunkNeighbour::kLeft:
            for (size_t x = 0; x < kChunkWidth; ++x) {
                for (size_t y = 0; y < kChunkHeight; ++y) {
                    const size_t z = 0;
                    bool n_opaque = n_opacity.right_[{x, y}];
                    (*this)[{x, y, z}].face_visibility_.set_face_visible(Face::kLeft, !n_opaque);
                }
            }
            break;
    }

    merged_sides_[*side] = true;
}

