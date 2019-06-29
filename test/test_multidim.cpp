#include <iostream>
#include "catch.hpp"
#include "multidim.h"

TEST_CASE("multidim", "[world]") {
    SECTION("grid") {
        using MyGrid = Grid<int, Coords3D<4, 5, 6>>;
        MyGrid grid(0);

        REQUIRE(grid.flatten({0, 0, 0}) == 0);
        REQUIRE(grid.flatten({1, 0, 0}) == 1);
        REQUIRE(grid.flatten({0, 1, 0}) == 4);
        REQUIRE(grid.flatten({0, 0, 1}) == 20);

        for (GridIndex i = 0; i < grid.FullSize; ++i) {
            MyGrid::Coord pos;
            grid.unflatten(i, pos);

            GridIndex flat = grid.flatten(pos);
            REQUIRE(i == flat);
        }

        MyGrid::Coord coord = {1, 4, 2};
        grid.fill(5);
        REQUIRE(grid[coord] == 5);

        grid[coord] = 10;
        REQUIRE(grid[coord] == 10);


        using YourGrid = Grid<bool, Coords2D<2,6>>;
        YourGrid square(0);
        REQUIRE(square.flatten({0, 0}) == 0);
        REQUIRE(square.flatten({1, 0}) == 1);
        REQUIRE(square.flatten({0, 1}) == 2);

        for (GridIndex i = 0; i < square.FullSize; ++i) {
            YourGrid::Coord pos;
            square.unflatten(i, pos);

            GridIndex flat = square.flatten(pos);
            REQUIRE(i == flat);
        }
    }
}