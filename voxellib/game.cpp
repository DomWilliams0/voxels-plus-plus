#include "game.h"
#include "error.h"
#include "util.h"
#include "camera.h"

int Game::run() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        log("failed to init SDL: %s", SDL_GetError());
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
        log("failed to create window: %s", SDL_GetError());
        return kErrorSdl;
    }

    SDL_GLContext gl = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, gl);

    // init renderer
    int ret;
    if ((ret = renderer_.init(&world_)) != kErrorSuccess)
        return ret;

    // TODO start at a less hard-coded position
//    camera_ = Camera(glm::vec3(-4.f, 3.f, 0.75f),
//                     glm::vec3(-0.3f, -0.5f, 1.f));
    CameraState last_camera_state{};


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

    return kErrorSuccess;
}

void Game::tick(double dt) {
//    log("ticking with dt %f", dt);
    last_camera_state_ = camera_.tick(dt);
}

void Game::render(double alpha) {
//    log("rendering with alpha %f", alpha);

    CameraState interpolated(camera_.interpolate_from(last_camera_state_, alpha));
    renderer_.render_world(interpolated.transform());
}

void Game::handle_keypress(SDL_Keycode key, bool down) {
    if (key == SDLK_ESCAPE && !down)
        running_ = false;
    else if (key == SDLK_y && !down)
        renderer_.toggle_wireframe();
}

