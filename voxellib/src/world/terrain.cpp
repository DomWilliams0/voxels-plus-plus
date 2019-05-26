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

void ChunkTerrain::update_face_visibility() {
    BlockCoord pos;
    for (int i = 0; i < kBlocksPerChunk; ++i) {
        Block &b = (*this)[i];

        FaceVisibility visibility = b.face_visibility_;

        if (!BlockType_opaque(b.type_)) {
            // fully visible because transparent
            visibility = kFaceVisibilityAll;
        } else {
            expand(i, pos);

            // check each face individually
            BlockCoord offset_pos;
            for (int j = 0; j < kFaceCount; ++j) {
                offset_pos = pos; // reset
                Face face = kFaces[j];
                face_offset(face, offset_pos.data_);

                // facing top/bottom of world, so this face is visible
                if (offset_pos[1] < 0 || offset_pos[1] >= kChunkHeight) {
                    visibility |= face_visibility(face);
                    continue;
                }

                // faces chunk boundary, will be set later
                if (offset_pos[0] < 0 || offset_pos[0] >= kChunkWidth ||
                    offset_pos[2] < 0 || offset_pos[2] >= kChunkDepth) {
                    continue;
                }

                // inside this chunk, safe to get the block type (rather ugly...)
                Block &offset_block = (*this)[offset_pos];

                if (BlockType_opaque(offset_block.type_))
                    visibility &= ~face_visibility(face); // not visible
                else
                    visibility |= face_visibility(face); // visible
            }
        }

        b.face_visibility_ = visibility;
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

