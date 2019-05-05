#include <GL/glew.h>
#include "world_renderer.h"
#include "util.h"
#include "error.h"
#include "shader_loader.h"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// globals to be kept updated
int window_width = 0;
int window_height = 0;

int WorldRenderer::init(World *world) {
    world_ = world;

    // init glew
    GLenum ret_gl;
    if ((ret_gl = glewInit()) != GLEW_OK) {
        log("glewInit failed: %s", glewGetErrorString(ret_gl));
        return kErrorGlew;
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


void WorldRenderer::render_world(const glm::mat4 &view, double alpha) {
    // clear screen
    glClearColor(1, 1, 1, 1);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, window_width, window_height);

    glUseProgram(prog_);

    // update projection
    {
        float aspect = ((float) window_width) / window_height;
        auto proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 50.0f);

        int loc = glGetUniformLocation(prog_, "projection");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
    }

    // update view TODO per chunk
    {
        glm::mat4 view_copy(view);
        glm::translate(view_copy, glm::vec3(0.5f, 5.f, 0.f));
        int loc = glGetUniformLocation(prog_, "view");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view_copy));
    }

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    int vertex_count = 36;
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

}
