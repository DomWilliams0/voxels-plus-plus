#ifndef VOXELS_WORLD_RENDERER_H
#define VOXELS_WORLD_RENDERER_H


#include <SFML/OpenGL.h>

#include "world.h"
#include "error.h"

class WorldRenderer {
public:
    WorldRenderer(); // uninitialised

    int init(World *world);

    void render_world(double interpolation);

private:
    World *world_;

    GLuint prog_, vao_, vbo_;

};

const float BLOCK_SIZE = 0.5f;

#define B BLOCK_SIZE // for brevity
const float BLOCK_VERTICES[] = {
        // front
        -B, -B, -B,
        -B, -B, +B,
        -B, +B, +B,
        -B, +B, +B,
        -B, +B, -B,
        -B, -B, -B,

        // left
        +B, -B, -B,
        -B, -B, -B,
        -B, +B, -B,
        -B, +B, -B,
        +B, +B, -B,
        +B, -B, -B,

        // right
        -B, -B, +B,
        +B, -B, +B,
        +B, +B, +B,
        +B, +B, +B,
        -B, +B, +B,
        -B, -B, +B,

        // top
        -B, +B, -B,
        -B, +B, +B,
        +B, +B, +B,
        +B, +B, +B,
        +B, +B, -B,
        -B, +B, -B,

        // bottom
        +B, -B, -B,
        +B, -B, +B,
        -B, -B, +B,
        -B, -B, +B,
        -B, -B, -B,
        +B, -B, -B,

        // back
        +B, +B, -B,
        +B, +B, +B,
        +B, -B, +B,
        +B, -B, +B,
        +B, -B, -B,
        +B, +B, -B,
};
#undef B

#endif
