#include "game.h"
#include "error.h"
#include "util.h"
#include "camera.h"
#include "loguru/loguru.hpp"

const char *kGlslVersion = "#version 330 core";


static void init_logging() {
    loguru::add_file("voxels.log", loguru::FileMode::Truncate, loguru::Verbosity_MAX);
}

int Game::run() {
    init_logging();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_F(ERROR, "failed to init SDL: %s", SDL_GetError());
        return kErrorSdl;
    }

    // window config
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    const int width = 800;
    const int height = 600;
    register_window_resize(width, height);

    // create window
    if ((window_ = SDL_CreateWindow("OpenGL",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    width, height, SDL_WINDOW_OPENGL)) == nullptr) {
        LOG_F(ERROR, "failed to create window: %s", SDL_GetError());
        return kErrorSdl;
    }

    SDL_GLContext gl = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, gl);

    // init renderer and GL
    int ret;
    if ((ret = renderer_.init(&world_)) != kErrorSuccess)
        return ret;

    // init UI
    ui_.init(window_, kGlslVersion, &gl);

    // init camera
//    CameraState last_camera_state;
    world_.register_camera(&camera_);

    // timestep
    double t = 0;
    const int tps = 20;
    const double dt = 1.0 / tps;
    double now_s = ((float) SDL_GetTicks()) / 1000;
    double acc = 0;

    SDL_Event event;
    while (running_) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running_ = false;
                    break;

                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    handle_keypress(event.key.keysym.sym, event.type == SDL_KEYDOWN);
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                        register_window_resize(event.window.data1, event.window.data2);
                    break;

                case SDL_MOUSEMOTION:
                    if (event.motion.state & SDL_BUTTON_LMASK)
                        camera_.rotate(event.motion.xrel, event.motion.yrel);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        SDL_SetRelativeMouseMode(event.button.state == SDL_PRESSED ? SDL_TRUE : SDL_FALSE);
                    break;

                default:
                    break;
            }
        }

        // independent frame rate
        double new_now_s = ((float) SDL_GetTicks()) / 1000;
        double frame_time = new_now_s - now_s;
        if (frame_time > 0.25)
            frame_time = 0.25;
        now_s = new_now_s;

        acc += frame_time;

        while (acc >= dt) {
            tick(dt);

            t += dt;
            acc -= dt;
        }

        const double alpha = acc / dt;
        render(alpha);

        SDL_GL_SwapWindow(window_);
    }

    cleanup();
    return kErrorSuccess;
}

void Game::tick(double dt) {
//    log("ticking with dt %f", dt);
    last_camera_state_ = camera_.tick(dt);
    world_.tick();
}

void Game::render(double alpha) {
//    log("rendering with alpha %f", alpha);

    CameraState interpolated(camera_.interpolate_from(last_camera_state_, alpha));
    renderer_.render_world(interpolated.transform());

    ui_.do_frame(camera_, world_);
}

void Game::handle_keypress(SDL_Keycode key, bool down) {
    if (key == SDLK_ESCAPE && !down)
        running_ = false;
    else if (key == SDLK_y && !down)
        renderer_.toggle_wireframe();
    else if (key == SDLK_r && !down)
        world_.clear_all_chunks();
    else if (key == SDLK_u && !down)
        world_.stop_chunk_loading();
    else if (key == SDLK_UP && !down)
        world_.tweak_loaded_chunk_radius(+1);
    else if (key == SDLK_DOWN && !down)
        world_.tweak_loaded_chunk_radius(-1);
}

void Game::cleanup() {
    world_.cleanup();
    ui_.cleanup();
}

