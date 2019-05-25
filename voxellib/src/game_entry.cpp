#include "game_entry.h"
#include "config.h"

//#define NO_GFX

#ifndef NO_GFX
#include "game.h"
#endif

#include "world/world.h"
#include "camera.h"


int run_game() {
    config::load();

    // TODO dont create World directly, load from another class
    // e.g. from file, generate from seed, etc
    World world({0, 100, 0}, {0, -1, 0});

#ifdef NO_GFX
        Camera cam;

        world.register_camera(&cam);

        for (int i = 0; i < 200; i++) {
            world.tick();
            usleep(1000);

            cam.set({i, 0, 0}, {});
        }

        return 0;
#else
    Game game(world);
    return game.run();
#endif
}
