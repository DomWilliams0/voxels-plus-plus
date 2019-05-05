#ifndef VOXELS_GAME_H
#define VOXELS_GAME_H


#include "world.h"

#include <SFML/Graphics/RenderTarget.hpp>

class Game {
public:
    explicit Game(World &world) : world_(world) {}

    int run();

private:
    World &world_;
    sf::RenderTarget *target_ = nullptr;

    void tick(float dt);

    void render(double alpha);
};


#endif
