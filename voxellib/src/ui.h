#ifndef VOXELS_UI_H
#define VOXELS_UI_H

#include <SDL2/SDL.h>
#include <string>
#include "camera.h"


class Ui {

public:
    void init(SDL_Window *window, const char *glsl_version, SDL_GLContext *gl_context);

    void do_frame(const Camera &camera);

    void cleanup();

private:
    void make_ui(const Camera &camera);

    SDL_Window *window_;
    SDL_GLContext *gl_;

    std::string pos_str_;
    std::string dir_str_;
    std::string chunk_str_;
};


#endif
