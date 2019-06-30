#ifndef VOXELS_MULTIDIM_H
#define VOXELS_MULTIDIM_H

#include <array>

// coord type
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

using GridIndex = unsigned int;

template<GridIndex X, GridIndex Y, GridIndex Z>
class Coords3D {
public:
    using Coord = glm::vec<3, unsigned int>;
    static constexpr GridIndex FullSize = X * Y * Z;

    static size_t flatten(const Coord &coord) {
        return coord.x + X * (coord.y + Y * coord.z);
    }

    static void unflatten(GridIndex index, Coord &out) {
        out.x = index % X;
        out.y = (index / X) % Y;
        out.z = index / (Y * X);
    }
};

template<GridIndex X, GridIndex Y>
class Coords2D {
public:
    using Coord = glm::vec<2, unsigned int>;
    static constexpr GridIndex FullSize = X * Y;

    static GridIndex flatten(const Coord &coord) {
        return coord.x + coord.y * X;
    }

    static void unflatten(GridIndex index, Coord &out) {
        out.x = index % X;
        out.y = index / X;
    }

};

template<typename T, typename GridImpl>
struct Grid : private GridImpl {
    static constexpr GridIndex FullSize = GridImpl::FullSize;
    using Coord = typename GridImpl::Coord;

    Grid(const T &val) {
        fill(val);
    }

    GridIndex flatten(const Coord &coord) const {
        return GridImpl::flatten(coord);
    }

    void unflatten(GridIndex index, Coord &out) const {
        GridImpl::unflatten(index, out);
    }

    T &operator[](const Coord &coord) {
        return array_[flatten(coord)];
    }

    const T &operator[](const Coord &coord) const {
        return array_[flatten(coord)];
    }

    T &operator[](GridIndex index) {
        return array_[index];
    }

    const T &operator[](GridIndex index) const {
        return array_[index];
    }

    void fill(const T &val) {
        array_.fill(val);
    }

private:
    std::array<T, FullSize> array_;
};


#endif
