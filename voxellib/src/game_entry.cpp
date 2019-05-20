#include "game_entry.h"
#include "game.h"

int run_game() {
    // TODO dont create World directly, load from another class
    // e.g. from file, generate from seed, etc
    World world({0, 2, 0});
    Game game(world);
    return game.run();
}
