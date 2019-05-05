#ifndef VOXELS_WORLD_RENDERER_H
#define VOXELS_WORLD_RENDERER_H


#include <glm/glm.hpp>
#include <GL/glew.h>

#include "world.h"
#include "block.h"
#include "error.h"

extern int window_width;
extern int window_height;

inline void register_window_resize(int w, int h) {
    window_width = w;
    window_height = h;
}

class WorldRenderer {
public:
    WorldRenderer(); // uninitialised

    int init(World *world);

    void render_world(const glm::mat4 &view);

    void toggle_wireframe();

private:
    World *world_;

    GLuint prog_, vao_, vbo_;
    bool wireframe_ = false;

    /**
     * Sets the shader uniform `view` after translating by `world_transform`
     *
     * @param view Camera view matrix
     * @param world_transform Offset to apply
     */
    void update_view(const glm::mat4 &view, const glm::vec3 &world_transform);
};

#define B kBlockSize // for brevity
const float kBlockVertices[] = {
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
