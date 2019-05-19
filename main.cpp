#include "game.h"

int main() {
    World world({0, 2, 0});
    Game game(world);
    return game.run();
}
