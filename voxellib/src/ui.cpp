#include <GL/gl.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "glm/glm.hpp"
#include "ui.h"
#include "camera.h"
#include "world/world.h"

void Ui::init(SDL_Window *window, const char *glsl_version, SDL_GLContext *gl_context) {
    window_ = window;
    gl_ = gl_context;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse; // no mouse
    io.IniFilename = nullptr; // no settings

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    pos_str_.reserve(64);
    chunk_str_.reserve(64);
    dir_str_.reserve(64);
    chunk_rad_str_.resize(64, '\0');
}

void Ui::do_frame(const Camera &camera, const World &world) {

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();

    make_ui(camera, world);

    // Rendering
    ImGui::Render();
    SDL_GL_MakeCurrent(window_, gl_);
//    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

enum Format {
    kFormatVec3,
    kFormatVec2,
    kFormatiVec3,
    kFormatiVec2,
};

static void format(const char *prefix, Format format, void *vec, std::string &out) {
    char buf[64];

    out.clear();
    out.append(prefix);

    switch (format) {
        case kFormatVec3: {
            glm::vec3 *v = static_cast<glm::vec3 *>(vec);
            snprintf(buf, 64, "%6.02f, %6.02f, %6.02f", v->x, v->y, v->z);
            break;
        }
        case kFormatVec2: {
            glm::vec2 *v = static_cast<glm::vec2 *>(vec);
            snprintf(buf, 64, "%6.02f, %6.02f", v->x, v->y);
            break;
        }
        case kFormatiVec3: {
            glm::ivec3 *v = static_cast<glm::ivec3 *>(vec);
            snprintf(buf, 64, "%6d, %6d, %6d", v->x, v->y, v->z);
            break;
        }
        case kFormatiVec2: {
            glm::ivec2 *v = static_cast<glm::ivec2 *>(vec);
            snprintf(buf, 64, "%6d, %6d", v->x, v->y);
            break;
        }
    }

    out.append(buf);

    ImGui::TextUnformatted(out.c_str());
}

void Ui::make_ui(const Camera &camera, const World &world) {
    int pos = 10;
    int flags = ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::SetNextWindowPos(
            ImVec2(pos, pos),
            ImGuiCond_Always
    );

    ImGui::Begin("Debug", nullptr, flags);

    // camera position
    format("Position:  ", kFormatVec3, (void *) &camera.pos(), pos_str_);

    {
        ChunkId_t cam_chunk_id = Chunk::owning_chunk(Block::from_world_pos(camera.pos()));
        int x, z;
        ChunkId_deconstruct(cam_chunk_id, x, z);
        glm::ivec2 vec(x, z);
        format("Chunk:     ", kFormatiVec2, (void *) &vec, chunk_str_);
    }

    // camera direction
    glm::vec2 cam_dir = camera.dir();
    format("Direction: ", kFormatVec2, (void *) &cam_dir, dir_str_);

    // loaded chunks
    {
        int chunk_radius = world.loaded_chunk_radius();
        int chunk_count = loaded_radius_chunk_count(chunk_radius);

        char *str = chunk_rad_str_.data();
        snprintf(str, 64, "Loaded chunks: %d (radius %d)", chunk_count, chunk_radius);
        ImGui::TextUnformatted(str);
    }

    ImGui::End();
}

void Ui::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    window_ = nullptr;
    gl_ = nullptr;
}
