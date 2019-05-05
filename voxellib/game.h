#ifndef VOXELS_GAME_H
#define VOXELS_GAME_H


#include "world.h"
#include "camera.h"
#include "world_renderer.h"

#include <SFML/Window.hpp>

class Game {
public:
    explicit Game(World &world) : world_(world) {}

    int run();

private:
    World &world_;
    WorldRenderer renderer_;
    sf::Window *window_ = nullptr;
    Camera camera_;
    CameraState last_camera_state_;

    bool mouse_grabbed_ = false;
    sf::Vector2i original_mouse_pos_;
    sf::Vector2i last_mouse_pos_;

    void tick(double dt);

    void render(double alpha);

    void grab_cursor(bool grab);

    void on_mouse_grab(int x, int y);
};


#endif
