#include "game_entry.h"
#include "config.h"
#include "game.h"

int run_game() {
    config::load();

    // TODO dont create World directly, load from another class
    // e.g. from file, generate from seed, etc
    World world({0, 25, 0});
    Game game(world);
    return game.run();
}
