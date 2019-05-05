#ifndef VOXELS_SHADER_LOADER_H
#define VOXELS_SHADER_LOADER_H


int load_shader(const char *filename, int type);

int load_program(GLuint *prog_out, const char *vertex_path, const char *fragment_path);

#endif
