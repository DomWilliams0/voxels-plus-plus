#include "game.h"
#include "world/world.h"

int main() {

    World world;
    Game game(world);
    return game.run();
}
