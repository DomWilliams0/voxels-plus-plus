#include "game.h"
#include "world.h"

int main() {

    World world;
    Game game(world);
    return game.run();
}
