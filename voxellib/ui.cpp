#include <GL/gl.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "ui.h"
#include "camera.h"

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
    dir_str_.reserve(64);
}

void Ui::do_frame(const Camera &camera) {

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window_);
    ImGui::NewFrame();

    make_ui(camera);

    // Rendering
    ImGui::Render();
    SDL_GL_MakeCurrent(window_, gl_);
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static void format_vec3(const char *prefix, const glm::vec3 &vec, std::string &out) {
    char buf[64];

    out.clear();
    out.append(prefix);

    snprintf(buf, 64, "%6.02f, %6.02f, %6.02f", vec.x, vec.y, vec.z);
    out.append(buf);

    ImGui::TextUnformatted(out.c_str());
}

void Ui::make_ui(const Camera &camera) {
    int pos = 10;
    int flags = ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::SetNextWindowPos(
            ImVec2(pos, pos),
            ImGuiCond_Always
    );

    ImGui::Begin("Debug", nullptr, flags);

    // position
    format_vec3("Position:  ", camera.pos(), pos_str_);

    // direction
    format_vec3("Direction: ", camera.dir(), dir_str_);

    ImGui::End();
}

void Ui::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    window_ = nullptr;
    gl_ = nullptr;
}
