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
    glBufferData(GL_ARRAY_BUFFER, sizeof(kBlockVertices), kBlockVertices, GL_STATIC_DRAW);


    return kErrorSuccess;
}

/**
 * Must call init() before doing anything
 */
WorldRenderer::WorldRenderer() : world_(nullptr) {} // uninitialised


void WorldRenderer::render_world(const glm::mat4 &view) {
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

    // iterate renderable chunks
    glm::ivec3 world_transform;
    auto chunks_it(world_->renderable_chunks());
    Chunk *chunk;
    while (chunks_it.next(&chunk)) {
        // enable chunk
        // TODO can we use the same vao for all chunks?
        glBindVertexArray(chunk->vao());
        glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo());

        // enable attributes
        {
            size_t word_size = sizeof(float);
            size_t stride = kChunkMeshWordsPerInstance * word_size;

            // 0: pos
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, 0);

            // 1: colour
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, stride, reinterpret_cast<const void *>(3L * word_size));

        }

        // update view with chunk world offset
        chunk->world_offset(world_transform);
        update_view(view, world_transform);

        // TODO instancing?
        glDrawArrays(GL_TRIANGLES, 0, chunk->vertex_count());
    }
}

void WorldRenderer::toggle_wireframe() {
    glPolygonMode(GL_FRONT_AND_BACK, (wireframe_ = !wireframe_) ? GL_LINE : GL_FILL);
}

void WorldRenderer::update_view(const glm::mat4 &view, const glm::vec3 &world_transform) {
    glm::mat4 translated_view = glm::translate(view, world_transform);
    int loc = glGetUniformLocation(prog_, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(translated_view));
}