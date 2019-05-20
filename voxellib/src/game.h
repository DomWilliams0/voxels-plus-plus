#ifndef VOXELS_GAME_H
#define VOXELS_GAME_H


#include "world/world_renderer.h"
#include "world/world.h"
#include "camera.h"
#include "ui.h"

#include <SDL2/SDL.h>

class Game {
public:
    explicit Game(World &world) : world_(world) {}

    int run();

private:
    World &world_;
    WorldRenderer renderer_;
    SDL_Window *window_ = nullptr;
    Camera camera_;
    CameraState last_camera_state_;
    bool running_ = true;

    Ui ui_;

    void tick(double dt);

    void render(double alpha);

    void handle_keypress(SDL_Keycode key, bool down);

    void cleanup();
};


#endif
