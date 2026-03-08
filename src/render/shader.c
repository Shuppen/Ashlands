#include "shader.h"

#include <stdio.h>
#include <stdlib.h>

static char *shader_read_text(const char *path) {
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    char *text;
    Sint64 size;
    size_t got;

    if (!rw) {
        fprintf(stderr, "[shader] open %s: %s\n", path, SDL_GetError());
        return NULL;
    }

    size = SDL_RWsize(rw);
    if (size <= 0 || size > (1 << 20)) {
        fprintf(stderr, "[shader] bad size for %s\n", path);
        SDL_RWclose(rw);
        return NULL;
    }

    text = calloc((size_t)size + 1, 1);
    if (!text) {
        SDL_RWclose(rw);
        return NULL;
    }

    got = SDL_RWread(rw, text, 1, (size_t)size);
    SDL_RWclose(rw);
    if (got != (size_t)size) {
        fprintf(stderr, "[shader] read %s failed\n", path);
        free(text);
        return NULL;
    }

    return text;
}

static void shader_print_log(GLuint handle, bool is_program, const char *label) {
    GLint log_len = 0;
    char *log;

    if (is_program) {
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_len);
    } else {
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_len);
    }

    if (log_len <= 1) {
        return;
    }

    log = calloc((size_t)log_len + 1, 1);
    if (!log) {
        return;
    }

    if (is_program) {
        glGetProgramInfoLog(handle, log_len, NULL, log);
    } else {
        glGetShaderInfoLog(handle, log_len, NULL, log);
    }

    fprintf(stderr, "[shader] %s\n%s\n", label, log);
    free(log);
}

static GLuint shader_compile(GLenum type, const char *path, const char *body) {
#ifdef PLATFORM_WEB
    const GLchar *sources[2] = {
        (const GLchar *)"precision mediump float;\nprecision mediump int;\n",
        (const GLchar *)body
    };
#else
    const GLchar *sources[2] = {
        (const GLchar *)"#version 120\n",
        (const GLchar *)body
    };
#endif
    GLuint shader = glCreateShader(type);
    GLint ok = GL_FALSE;

    glShaderSource(shader, 2, sources, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) {
        return shader;
    }

    shader_print_log(shader, false, path);
    glDeleteShader(shader);
    return 0;
}

bool shader_program_load_files(ShaderProgram *program,
                               const char *vertex_path,
                               const char *fragment_path) {
    char *vert_text;
    char *frag_text;
    GLuint vert;
    GLuint frag;
    GLuint linked;
    GLint ok = GL_FALSE;

    if (!program) {
        return false;
    }

    vert_text = shader_read_text(vertex_path);
    frag_text = shader_read_text(fragment_path);
    if (!vert_text || !frag_text) {
        free(vert_text);
        free(frag_text);
        return false;
    }

    vert = shader_compile(GL_VERTEX_SHADER, vertex_path, vert_text);
    frag = shader_compile(GL_FRAGMENT_SHADER, fragment_path, frag_text);
    free(vert_text);
    free(frag_text);
    if (!vert || !frag) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        return false;
    }

    linked = glCreateProgram();
    glAttachShader(linked, vert);
    glAttachShader(linked, frag);
    glBindAttribLocation(linked, SHADER_ATTRIB_POSITION, "a_position");
    glBindAttribLocation(linked, SHADER_ATTRIB_TEXCOORD, "a_texcoord");
    glBindAttribLocation(linked, SHADER_ATTRIB_NORMAL, "a_normal");
    glBindAttribLocation(linked, SHADER_ATTRIB_COLOR, "a_color");
    glLinkProgram(linked);
    glGetProgramiv(linked, GL_LINK_STATUS, &ok);
    glDeleteShader(vert);
    glDeleteShader(frag);
    if (ok != GL_TRUE) {
        shader_print_log(linked, true, "program link failed");
        glDeleteProgram(linked);
        return false;
    }

    program->program = linked;
    return true;
}

void shader_program_destroy(ShaderProgram *program) {
    if (program && program->program) {
        glDeleteProgram(program->program);
        program->program = 0;
    }
}

void shader_program_use(const ShaderProgram *program) {
    glUseProgram(program ? program->program : 0);
}

GLint shader_program_uniform(const ShaderProgram *program, const char *name) {
    if (!program || !program->program) {
        return -1;
    }
    return glGetUniformLocation(program->program, name);
}
