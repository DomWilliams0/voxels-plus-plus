#include <cstdio>
#include <GL/glew.h>
#include <cstring>
#include <cstdlib>

#include "shader_loader.h"
#include "util.h"
#include "error.h"

// TODO convert to c++

static char *read_shader_source(const char *filename) {
    std::string path;
    resolve_resource_path(path, filename);
    filename = path.c_str();

    FILE *f = fopen(filename, "r");

    if (!f) {
        LOG_F(ERROR, "failed to open shader '%s'", filename);
        return nullptr;
    }

    size_t len, read;

    // find file length
    fseek(f, 0, SEEK_END);
    len = (size_t) ftell(f);
    rewind(f);

    char *buf = nullptr;

    // alloc buffer
    buf = static_cast<char *>(calloc(len + 1, sizeof(char)));
    if (!buf) {
        LOG_F(ERROR, "failed to allocate %ld bytes for shader", len);
        goto cleanup;
    }

    // read
    read = fread(buf, sizeof(char), len, f);
    if (read != len) {
        LOG_F(ERROR, "failed to read shader: read %ld/%ld", read, len);
        free(buf);
        buf = nullptr;
    }

    cleanup:
    fclose(f);
    return buf;
}

int load_shader(const char *filename, int type) {
    char *src = read_shader_source(filename);
    int shader = 0;
    if (src) {
        shader = glCreateShader(type);
        const int len = strlen(src);
        glShaderSource(shader, 1, &src, &len);
        glCompileShader(shader);

        int compile_status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (compile_status == GL_FALSE) {
            int log_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

            char *log_str = static_cast<char *>(calloc(sizeof(char), log_len + 1));
            if (!log_str) {
                LOG_F(ERROR, "failed to alloc log buffer while failing to load shader, oh dear");
            } else {
                glGetShaderInfoLog(shader, log_len, nullptr, log_str);
                LOG_F(ERROR, "failed to compile shader: %s", log_str);
                free(log_str);

            }
            glDeleteShader(shader);
            shader = 0;
        }

        free(src);
    }

    return shader;
}

int load_program(GLuint *prog_out, const char *vertex_path, const char *fragment_path) {
    int vert = load_shader(vertex_path, GL_VERTEX_SHADER);
    int frag = load_shader(fragment_path, GL_FRAGMENT_SHADER);

    if (vert == 0 || frag == 0) {
        LOG_F(ERROR, "failed to load shaders");
        return kErrorShaderLoad;
    }

    GLuint prog = *prog_out = glCreateProgram();

    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    int link_status;
    glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        LOG_F(ERROR, "failed to link program");
        return kErrorShaderLoad;
    }

    glProgramParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);

    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return kErrorSuccess;

}
