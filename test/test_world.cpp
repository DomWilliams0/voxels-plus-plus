#include "catch.hpp"
#include "world/world.h"

TEST_CASE("world things", "[world]") {

    SECTION("chunk ids") {
        int x, z;
        ChunkId_t cid;

        cid = ChunkId(100, 5);
        ChunkId_deconstruct(cid, x, z);
        REQUIRE(x == 100);
        REQUIRE(z == 5);

        cid = ChunkId(-5, 0);
        ChunkId_deconstruct(cid, x, z);
        REQUIRE(x == -5);
        REQUIRE(z == 0);

        cid = ChunkId(10, -2);
        ChunkId_deconstruct(cid, x, z);
        REQUIRE(x == 10);
        REQUIRE(z == -2);
    }

    glm::vec3 pos(4.1, 1, 4.28);
    glm::ivec3 block = Block::from_world_pos(pos);

//    REQUIRE(block.x == pos.x * kBlockScale);


}

/*TEST_CASE("chunk neighbours", "[world]") {
    SECTION("out of range test") {
        ChunkNeighbourMask mask;
        REQUIRE(mask.mask() == 0);

        // 2 and -2 are in range
        mask.update_load_range(2, 0, 0, 0, 3);
        REQUIRE(mask.mask() == 0);
        mask.update_load_range(-2, 0, 0, 0, 3);
        REQUIRE(mask.mask() == 0);

        // 3/-3 is on the edge, BACK/FRONT should be out of range
        mask.update_load_range(3, 0, 0, 0, 3);
        REQUIRE(mask.mask() == 1 << (int) ChunkNeighbour::kBack);
        mask.update_load_range(-3, 0, 0, 0, 3);
        REQUIRE(mask.mask() == 1 << (int) ChunkNeighbour::kFront);

        // totally out of range
        mask.update_load_range(-100, 100, 30, -10, 5);
        REQUIRE(mask.mask() == (1 << (int) ChunkNeighbour::kFront | 1 << (int) ChunkNeighbour::kRight));
    }
}*/
/*
TEST_CASE("range check", "[world]") {
    REQUIRE(World::is_in_loaded_range(0, 0, 5, 0, 0));
    REQUIRE(World::is_in_loaded_range(0, 0, 5, 5, -5));
    REQUIRE(World::is_in_loaded_range(0, 0, 5, -5, 2));

    REQUIRE(!World::is_in_loaded_range(0, 0, 5, -2, 10));
    REQUIRE(!World::is_in_loaded_range(0, 0, 5, 100, 0));
}*/
