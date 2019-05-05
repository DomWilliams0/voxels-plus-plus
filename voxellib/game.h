#ifndef VOXELS_GAME_H
#define VOXELS_GAME_H


#include "world.h"
#include "camera.h"
#include "world_renderer.h"

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

    void tick(double dt);

    void render(double alpha);
};


#endif
