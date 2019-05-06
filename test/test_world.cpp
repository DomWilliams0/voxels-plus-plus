#include "catch.hpp"
#include "world/world.h"

TEST_CASE("coordinate resolution", "[world]") {

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
