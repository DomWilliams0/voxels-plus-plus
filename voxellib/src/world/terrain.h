#ifndef VOXELS_TERRAIN_H
#define VOXELS_TERRAIN_H

#include <array>

#include "multidim_grid.hpp"
#include "block.h"

class ChunkNeighbour {
    enum Value : uint8_t {
        kFront,
        kLeft,
        kRight,
        kBack,
    };

    ChunkNeighbour() = default;

    constexpr ChunkNeighbour(Value val) : value_(val) {}

    constexpr bool operator==(ChunkNeighbour o) const { return value_ == o.value_; }

    constexpr bool operator!=(ChunkNeighbour o) const { return value_ != o.value_; }

    constexpr static int kCount = 4;

    ChunkNeighbour opposite() const;

private:
    Value value_;
};


// TODO vertical subchunks
class ChunkTerrain {
public:
    typedef multidim::Grid<Block, kChunkWidth, kChunkHeight, kChunkDepth> GridType;
//    typedef std::array<ChunkTerrain::GridType::size_type, ChunkTerrain> BlockCoord;
    typedef ChunkTerrain::GridType::ArrayCoord BlockCoord;

    Block &operator[](unsigned int flat_index);

    Block &operator[](const GridType::ArrayCoord &coord);

    void expand(unsigned int index, BlockCoord &out);

    void update_face_visibility();

    void populate_neighbour_opacity();

private:
    GridType grid_;

    template<size_t dim1, size_t dim2>
    struct NeighbourOpacity {
        typedef bool Bit; // TODO bits instead of huge bools
        typedef multidim::Grid<Bit, dim1, dim2> OpacityGridType;

        constexpr inline Bit &operator[](const typename OpacityGridType::ArrayCoord &coord) {
            return grid_[coord];
        }

    private:
        OpacityGridType grid_;
    };

    struct {
        NeighbourOpacity<kChunkHeight, kChunkDepth> back_, front_;
        NeighbourOpacity<kChunkWidth, kChunkHeight> left_, right_;
    } neighbour_opacity_;
};


#endif
