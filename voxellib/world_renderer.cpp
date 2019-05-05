#include <GL/glew.h>
#include <SFML/OpenGL.h>
#include "world_renderer.h"
#include "util.h"
#include "error.h"
#include "shader_loader.h"


int WorldRenderer::init(World *world) {
    world_ = world;

    // init glew
    GLenum ret_gl;
    if ((ret_gl = glewInit()) != GLEW_OK) {
        log("glewInit failed: %s", glewGetErrorString(ret_gl));
        return kErrorGLEW;
    }

    // load shaders
    int ret;
    if ((ret = load_program(&prog_, "shaders/world.glslv", "shaders/world.glslf")) != kErrorSuccess)
        return ret;


    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(BLOCK_VERTICES), BLOCK_VERTICES, GL_STATIC_DRAW);


    return kErrorSuccess;
}

/**
 * Must call init() before doing anything
 */
WorldRenderer::WorldRenderer() : world_(nullptr) {} // uninitialised


void WorldRenderer::render_world(double interpolation) {
    glClearColor(1, 1, 1, 1);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    // TODO glViewport(0, 0, window_width, window_height)

    glUseProgram(prog_);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    int vertex_count = 12;
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

}
