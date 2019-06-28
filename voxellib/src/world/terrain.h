#ifndef VOXELS_TERRAIN_H
#define VOXELS_TERRAIN_H

#include <array>
#include <bitset>

#include "multidim_grid.hpp"
#include "block.h"

class ChunkNeighbour {
public:
    enum Value : uint8_t {
        kFront,
        kLeft,
        kRight,
        kBack,
    };

    ChunkNeighbour() = default;

    ChunkNeighbour(uint8_t value) : value_(static_cast<Value>(value)) {}

    constexpr ChunkNeighbour(Value val) : value_(val) {}

    constexpr bool operator==(ChunkNeighbour o) const { return value_ == o.value_; }

    constexpr bool operator!=(ChunkNeighbour o) const { return value_ != o.value_; }

    constexpr Value operator*() const { return value_; }

    constexpr static int kCount = 4;

    ChunkNeighbour opposite() const;

private:
    Value value_;
};


// TODO vertical subchunks
class ChunkTerrain {
public:
    typedef multidim::Grid<Block, kChunkWidth, kChunkHeight, kChunkDepth> GridType;
    typedef std::array<int, 3> BlockCoord;

    Block &operator[](unsigned int flat_index);

    Block &operator[](const GridType::ArrayCoord &coord);

    void expand(unsigned int index, BlockCoord &out);

    void update_face_visibility();

    void populate_neighbour_opacity();

    void merge_faces(const ChunkTerrain &neighbour, ChunkNeighbour side);

    inline bool has_merged_faces(const ChunkNeighbour &neighbour) const { return merged_sides_[*neighbour]; }

    inline void reset_merged_faces() { merged_sides_.reset(); }

private:
    GridType grid_;

    bool is_visible(const BlockCoord &pos, Face face, bool bounds_check, bool *was_out_of_bounds);

    static bool is_out_of_bounds(const BlockCoord &pos);

    void calculate_vertex_ao(AmbientOcclusion::Builder &ao, AmbientOcclusion::Builder::Vertex vertex,
                                    ChunkTerrain::BlockCoord offset_pos, Face face);

    template<size_t dim1, size_t dim2>
    struct NeighbourOpacity {
        typedef bool Bit; // TODO bits instead of huge bools
        typedef multidim::Grid<Bit, dim1, dim2> OpacityGridType;

        NeighbourOpacity() = default;
        NeighbourOpacity(const NeighbourOpacity &o) : grid_(o.grid_) {}

        constexpr inline Bit &operator[](const typename OpacityGridType::ArrayCoord &coord) {
            return grid_[coord];
        }

        constexpr inline Bit operator[](const typename OpacityGridType::ArrayCoord &coord) const {
            return grid_[coord];
        }

    private:
        OpacityGridType grid_;
    };

    struct {
        NeighbourOpacity<kChunkHeight, kChunkDepth> back_, front_;
        NeighbourOpacity<kChunkWidth, kChunkHeight> left_, right_;
    } neighbour_opacity_;

    std::bitset<ChunkNeighbour::kCount> merged_sides_;
};


#endif
