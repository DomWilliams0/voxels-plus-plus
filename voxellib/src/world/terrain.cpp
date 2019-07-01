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
    }

    assert(false);
    return kBack; // unreachable
}

ChunkTerrain::ChunkTerrain() : grid_(Block()) {}

Block &ChunkTerrain::operator[](GridIndex flat_index) {
    return grid_[flat_index];
}

Block &ChunkTerrain::operator[](const BlockGrid::Coord &coord) {
    return grid_[coord];
}

const Block &ChunkTerrain::operator[](const BlockGrid::Coord &coord) const {
    return grid_[coord];
}

const Block &ChunkTerrain::operator[](const BlockCoord &coord) const {
    // unsigned to signed conversion
    assert(coord[0] >= 0);
    assert(coord[1] >= 0);
    assert(coord[2] >= 0);

    return grid_[{
            static_cast<unsigned long>(coord[0]),
            static_cast<unsigned long>(coord[1]),
            static_cast<unsigned long>(coord[2]),
    }];
}

void ChunkTerrain::expand(unsigned int index, BlockCoord &out) {
    BlockGrid::Coord expanded;
    grid_.unflatten(index, expanded);

    out[0] = expanded.x;
    out[1] = expanded.y;
    out[2] = expanded.z;
}

bool
ChunkTerrain::is_opaque_internal_only(const BlockCoord &src, Face face, bool bounds_check, bool *was_out_of_bounds) {
    BlockCoord offset(src);
    face.offset(offset.data());

    // check for out of bounds
    if (is_out_of_bounds_invalid(offset) ||
        (bounds_check && is_out_of_bounds_in_another_chunk(offset))) {
        // not visible for now, will be updated later
        *was_out_of_bounds = true;
        return false;
    }

    // safe to cast from signed to unsigned and deref now
    const Block &offset_block = (*this)[offset];
    return offset_block.type_.opaque();
}


bool ChunkTerrain::is_out_of_bounds_in_another_chunk(const ChunkTerrain::BlockCoord &pos) {
    return pos[0] < 0 || pos[0] >= kChunkWidth || pos[2] < 0 || pos[2] >= kChunkDepth;
}

bool ChunkTerrain::is_out_of_bounds_invalid(const ChunkTerrain::BlockCoord &pos) {
    return pos[1] < 0 || pos[1] >= kChunkHeight;
}

void ChunkTerrain::calculate_ao(AmbientOcclusion::Builder &ao, ChunkTerrain::BlockCoord offset_pos, Face face) {
    Face fa, fb;

    for (auto vertex : AmbientOcclusion::Builder::kVertices) {
        AmbientOcclusion::Builder::get_face_offsets(face, vertex, fa, fb);

        BlockCoord fa_pos(offset_pos);
        fa.offset(fa_pos.data());

        bool abort = false;
        bool s1 = is_opaque_internal_only(offset_pos, fa, true, &abort);
        if (abort) continue;

        bool s2 = is_opaque_internal_only(offset_pos, fb, true, &abort);
        if (abort) continue;

        bool corner = is_opaque_internal_only(fa_pos, fb, false, nullptr); // dont need to check corner for out of bounds
        ao.set_vertex(face, vertex, s1, s2, corner);
    }
}

void ChunkTerrain::update_face_visibility() {
    BlockCoord pos;
    for (unsigned int i = 0; i < kBlocksPerChunk; ++i) {
        Block &b = (*this)[i];

        FaceVisibility &visibility = b.face_visibility_;
        AmbientOcclusion::Builder ao;

        if (!b.type_.opaque()) {
            // fully visible and not occluded because transparent
            visibility.set_invisible();
            ao.set_brightest();
        } else {
            expand(i, pos);

            // check each face individually
            for (const Face &face : Face::kFaces) {
                BlockCoord offset_pos(pos);
                face.offset(offset_pos.data());

                // facing top/bottom of world, so this face is visible
                if (is_out_of_bounds_invalid(offset_pos)) {
                    visibility.set_face_visible(face, true);
                    continue;
                }

                // faces chunk boundary, will be updated later
                if (is_out_of_bounds_in_another_chunk(offset_pos)) {
                    visibility.set_face_visible(face, true);
                    ao.set_brightest(); // looks funny if the edge of the world goes dark
                    continue;
                }

                // inside this chunk, safe to get the block type
                const Block &offset_block = (*this)[offset_pos];
                bool offset_opaque = offset_block.type_.opaque();
                visibility.set_face_visible(face, !offset_opaque);

                // update ao for visible faces only
                if (!offset_opaque) {
                    calculate_ao(ao, offset_pos, face);
                }
            }
        }

        ao.build(b.ao_);
    }

}

void ChunkTerrain::populate_neighbour_opacity() {
    // back: +x
    for (int y = 0; y < kChunkHeight; ++y) {
        for (int z = 0; z < kChunkDepth; ++z) {
            const int x = kChunkWidth - 1;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.back_[{y, z}] = block.type_.opaque();
        }
    }

    // front: -x
    for (int y = 0; y < kChunkHeight; ++y) {
        for (int z = 0; z < kChunkDepth; ++z) {
            const int x = 0;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.front_[{y, z}] = block.type_.opaque();
        }
    }

    // right: +z
    for (int x = 0; x < kChunkWidth; ++x) {
        for (int y = 0; y < kChunkHeight; ++y) {
            const int z = kChunkDepth - 1;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.right_[{x, y}] = block.type_.opaque();
        }
    }

    // left: -z
    for (int x = 0; x < kChunkWidth; ++x) {
        for (int y = 0; y < kChunkHeight; ++y) {
            const int z = 0;
            Block &block = (*this)[{x, y, z}];
            neighbour_opacity_.left_[{x, y}] = block.type_.opaque();
        }
    }
}

template<size_t dim1, size_t dim2>
bool ChunkTerrain::is_opaque_perhaps_in_neighbour(const BlockCoord &pos,
                                                  const NeighbourOpacity<dim1, dim2> &neighbour_opacity,
                                                  int idx1, int idx2) {
    // vertically out of world, dont care
    if (is_out_of_bounds_invalid(pos))
        return false;

    if (is_out_of_bounds_in_another_chunk(pos)) {
        // translate to neighbour coordinates
        BlockCoord neighbour_pos(pos);
        if (neighbour_pos[idx1] < 0) neighbour_pos[idx1] += dim1;
        if (neighbour_pos[idx2] < 0) neighbour_pos[idx2] += dim2;
        if (neighbour_pos[idx1] >= static_cast<int>(dim1)) neighbour_pos[idx1] -= dim1;
        if (neighbour_pos[idx2] >= static_cast<int>(dim2)) neighbour_pos[idx2] -= dim2;

        // read from neighbour
        assert(neighbour_pos[idx1] >= 0 && neighbour_pos[idx2] >= 0);
        return neighbour_opacity[{static_cast<unsigned long>(neighbour_pos[idx1]),
                                  static_cast<unsigned long>(neighbour_pos[idx2])}];
    }

    // internal
    const Block &block = (*this)[pos];
    return block.type_.opaque();
}

template<size_t dim1, size_t dim2>
void ChunkTerrain::calculate_edge_ao(Block &block, const BlockCoord &pos,
                                     const NeighbourOpacity<dim1, dim2> &neighbour_opacity,
                                     int idx1, int idx2) {
    AmbientOcclusion::Builder ao;
    Face fa, fb;

    // skip corners immediately
    // TODO use a custom iterator to avoid corners instead of this monstrosity
    int p1 = pos[idx1], p2 = pos[idx2];
    if ((p1 == 0 && p2 == 0) || (p1 == 0 && p2 == dim2 - 1) ||
        (p1 == dim1 - 1 && p2 == 0) || (p1 == dim1 - 1 && p2 == dim2 - 1))
        return;

    for (Face face : Face::kFaces) {
        // face is not visible, skip
        if (!block.face_visibility_.visible(face))
            continue;

        BlockCoord offset_pos; // TODO helper to go direct in one line
        face.offset_with_copy(pos.data(), offset_pos.data());

        // go around face
        for (auto vertex : AmbientOcclusion::Builder::kVertices) {
            AmbientOcclusion::Builder::get_face_offsets(face, vertex, fa, fb);

            // calculate blocks we need
            BlockCoord fa_pos, fb_pos, corner_pos;
            fa.offset_with_copy(offset_pos.data(), fa_pos.data());
            fb.offset_with_copy(offset_pos.data(), fb_pos.data());
            fb.offset_with_copy(fa_pos.data(), corner_pos.data());

            bool s1 = is_opaque_perhaps_in_neighbour(fa_pos, neighbour_opacity, idx1, idx2);
            bool s2 = is_opaque_perhaps_in_neighbour(fb_pos, neighbour_opacity, idx1, idx2);
            bool corner = is_opaque_perhaps_in_neighbour(corner_pos, neighbour_opacity, idx1, idx2);

            ao.set_vertex(face, vertex, s1, s2, corner);
        }
    }

    ao.build(block.ao_);
}

void ChunkTerrain::merge_faces(const ChunkTerrain &neighbour, ChunkNeighbour side) {
    // local copy to avoid constantly reading from neighbour
    auto n_opacity = neighbour.neighbour_opacity_;

    switch (*side) {
        case ChunkNeighbour::kBack:
            for (size_t y = 0; y < kChunkHeight; ++y) {
                for (size_t z = 0; z < kChunkDepth; ++z) {
                    const size_t x = kChunkWidth - 1;
                    auto &n = n_opacity.front_;
                    bool n_opaque = n[{y, z}];
                    Face face = Face::kBack;

                    // face visibility
                    Block &block = (*this)[{x, y, z}];
                    if (!block.type_.opaque()) continue;
                    block.face_visibility_.set_face_visible(face, !n_opaque);

                    // ao
                    BlockCoord pos = {x, static_cast<int>(y), static_cast<int>(z)};
                    calculate_edge_ao(block, pos, n, 1, 2);
                }
            }
            break;

        case ChunkNeighbour::kFront:
            for (size_t y = 0; y < kChunkHeight; ++y) {
                for (size_t z = 0; z < kChunkDepth; ++z) {
                    const size_t x = 0;
                    auto &n = n_opacity.back_;
                    bool n_opaque = n[{y, z}];
                    Face face = Face::kFront;

                    // face visibility
                    Block &block = (*this)[{x, y, z}];
                    if (!block.type_.opaque()) continue;
                    block.face_visibility_.set_face_visible(face, !n_opaque);

                    // ao
                    BlockCoord pos = {x, static_cast<int>(y), static_cast<int>(z)};
                    calculate_edge_ao(block, pos, n, 1, 2);
                }
            }
            break;

        case ChunkNeighbour::kRight:
            for (size_t x = 0; x < kChunkWidth; ++x) {
                for (size_t y = 0; y < kChunkHeight; ++y) {
                    const size_t z = kChunkDepth - 1;
                    auto &n = n_opacity.left_;
                    bool n_opaque = n[{x, y}];
                    Face face = Face::kRight;

                    // face visibility
                    Block &block = (*this)[{x, y, z}];
                    if (!block.type_.opaque()) continue;
                    block.face_visibility_.set_face_visible(face, !n_opaque);

                    // ao
                    BlockCoord pos = {static_cast<int>(x), static_cast<int>(y), z};
                    calculate_edge_ao(block, pos, n, 0, 1);
                }
            }
            break;

        case ChunkNeighbour::kLeft:
            for (size_t x = 0; x < kChunkWidth; ++x) {
                for (size_t y = 0; y < kChunkHeight; ++y) {
                    const size_t z = 0;
                    auto &n = n_opacity.right_;
                    bool n_opaque = n[{x, y}];
                    Face face = Face::kLeft;

                    // face visibility
                    Block &block = (*this)[{x, y, z}];
                    if (!block.type_.opaque()) continue;
                    block.face_visibility_.set_face_visible(face, !n_opaque);

                    // ao
                    BlockCoord pos = {static_cast<int>(x), static_cast<int>(y), z};
                    calculate_edge_ao(block, pos, n, 0, 1);
                }
            }
            break;
    }

    merged_sides_[*side] = true;
}

