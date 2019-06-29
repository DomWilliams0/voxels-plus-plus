#ifndef VOXELS_TERRAIN_H
#define VOXELS_TERRAIN_H

#include <array>
#include <bitset>

#include "multidim.h"
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
    typedef Grid<Block, Coords3D<kChunkWidth, kChunkHeight, kChunkDepth>> BlockGrid;
    typedef std::array<signed int, 3> BlockCoord; // local block coord in a chunk

    ChunkTerrain();

    // delegates
    Block &operator[](GridIndex flat_index);

    Block &operator[](const BlockGrid::Coord &coord);

    const Block &operator[](const BlockGrid::Coord &coord) const;

    const Block &operator[](const BlockCoord &coord) const;

    void expand(unsigned int index, BlockCoord &out);


    void update_face_visibility();

    void populate_neighbour_opacity();

    void merge_faces(const ChunkTerrain &neighbour, ChunkNeighbour side);

    inline bool has_merged_faces(const ChunkNeighbour &neighbour) const { return merged_sides_[*neighbour]; }

    inline void reset_merged_faces() { merged_sides_.reset(); }

private:
    BlockGrid grid_;

    std::bitset<ChunkNeighbour::kCount> merged_sides_;

    bool is_opaque_internal_only(const BlockCoord &src, Face face, bool bounds_check, bool *was_out_of_bounds);

    static bool is_out_of_bounds_in_another_chunk(const BlockCoord &pos);

    static bool is_out_of_bounds_invalid(const BlockCoord &pos);

    void calculate_ao(AmbientOcclusion::Builder &ao, ChunkTerrain::BlockCoord offset_pos, Face face);


    // TODO bits instead of huge bools
    template<size_t dim1, size_t dim2>
    struct NeighbourOpacity : public Grid<bool, Coords2D<dim1, dim2>> {
        NeighbourOpacity() : Grid<bool, Coords2D<dim1, dim2>>(false) {}
    };

    struct {
        NeighbourOpacity<kChunkHeight, kChunkDepth> back_, front_;
        NeighbourOpacity<kChunkWidth, kChunkHeight> left_, right_;
    } neighbour_opacity_;

    template<size_t dim1, size_t dim2>
    bool is_opaque_perhaps_in_neighbour(const BlockCoord &pos,
                                        const NeighbourOpacity<dim1, dim2> &neighbour_opacity,
                                        int idx1, int idx2);

    template<size_t dim1, size_t dim2>
    void calculate_edge_ao(Block &block, const BlockCoord &pos,
                           const NeighbourOpacity<dim1, dim2> &neighbour_opacity,
                           int idx1, int idx2);
};


#endif
