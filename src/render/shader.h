#ifndef ASHLANDS_SHADER_H
#define ASHLANDS_SHADER_H

#include <stdbool.h>

#include "../../include/ashlands_gl.h"

enum {
    SHADER_ATTRIB_POSITION = 0,
    SHADER_ATTRIB_TEXCOORD = 1,
    SHADER_ATTRIB_NORMAL = 2,
    SHADER_ATTRIB_COLOR = 3,
};

typedef struct {
    GLuint program;
} ShaderProgram;

bool shader_program_load_files(ShaderProgram *program,
                               const char *vertex_path,
                               const char *fragment_path);
void shader_program_destroy(ShaderProgram *program);
void shader_program_use(const ShaderProgram *program);
GLint shader_program_uniform(const ShaderProgram *program, const char *name);

#endif
