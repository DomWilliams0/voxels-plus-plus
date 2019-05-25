#ifndef VOXELS_UI_H
#define VOXELS_UI_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "camera.h"

class World;

class Ui {

public:
    void init(SDL_Window *window, const char *glsl_version, SDL_GLContext *gl_context);

    void do_frame(const Camera &camera, const World &world);

    void cleanup();

private:
    void make_ui(const Camera &camera, const World &world);

    SDL_Window *window_;
    SDL_GLContext *gl_;

    std::string pos_str_;
    std::string dir_str_;
    std::string chunk_str_;
    std::vector<char> chunk_rad_str_; // poor man's string
};


#endif
