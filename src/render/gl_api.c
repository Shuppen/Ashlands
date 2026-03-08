#include "../../include/ashlands_gl.h"

#if !defined(PLATFORM_WEB) && !defined(PLATFORM_ANDROID)

#include <stdio.h>

#define ASHLANDS_GL_FUNCTIONS(X) \
    X(PFNGLACTIVETEXTUREPROC, ash_glActiveTexture, glActiveTexture) \
    X(PFNGLATTACHSHADERPROC, ash_glAttachShader, glAttachShader) \
    X(PFNGLBINDBUFFERPROC, ash_glBindBuffer, glBindBuffer) \
    X(PFNGLBINDATTRIBLOCATIONPROC, ash_glBindAttribLocation, glBindAttribLocation) \
    X(PFNGLBUFFERDATAPROC, ash_glBufferData, glBufferData) \
    X(PFNGLCOMPILESHADERPROC, ash_glCompileShader, glCompileShader) \
    X(PFNGLCREATEPROGRAMPROC, ash_glCreateProgram, glCreateProgram) \
    X(PFNGLCREATESHADERPROC, ash_glCreateShader, glCreateShader) \
    X(PFNGLDELETEBUFFERSPROC, ash_glDeleteBuffers, glDeleteBuffers) \
    X(PFNGLDELETEPROGRAMPROC, ash_glDeleteProgram, glDeleteProgram) \
    X(PFNGLDELETESHADERPROC, ash_glDeleteShader, glDeleteShader) \
    X(PFNGLDISABLEVERTEXATTRIBARRAYPROC, ash_glDisableVertexAttribArray, glDisableVertexAttribArray) \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, ash_glEnableVertexAttribArray, glEnableVertexAttribArray) \
    X(PFNGLGENBUFFERSPROC, ash_glGenBuffers, glGenBuffers) \
    X(PFNGLGENERATEMIPMAPPROC, ash_glGenerateMipmap, glGenerateMipmap) \
    X(PFNGLGETPROGRAMINFOLOGPROC, ash_glGetProgramInfoLog, glGetProgramInfoLog) \
    X(PFNGLGETPROGRAMIVPROC, ash_glGetProgramiv, glGetProgramiv) \
    X(PFNGLGETSHADERINFOLOGPROC, ash_glGetShaderInfoLog, glGetShaderInfoLog) \
    X(PFNGLGETSHADERIVPROC, ash_glGetShaderiv, glGetShaderiv) \
    X(PFNGLGETUNIFORMLOCATIONPROC, ash_glGetUniformLocation, glGetUniformLocation) \
    X(PFNGLLINKPROGRAMPROC, ash_glLinkProgram, glLinkProgram) \
    X(PFNGLSHADERSOURCEPROC, ash_glShaderSource, glShaderSource) \
    X(PFNGLUNIFORM1FPROC, ash_glUniform1f, glUniform1f) \
    X(PFNGLUNIFORM1IPROC, ash_glUniform1i, glUniform1i) \
    X(PFNGLUNIFORM3FPROC, ash_glUniform3f, glUniform3f) \
    X(PFNGLUNIFORMMATRIX4FVPROC, ash_glUniformMatrix4fv, glUniformMatrix4fv) \
    X(PFNGLUSEPROGRAMPROC, ash_glUseProgram, glUseProgram) \
    X(PFNGLVERTEXATTRIBPOINTERPROC, ash_glVertexAttribPointer, glVertexAttribPointer)

#define ASHLANDS_GL_DEFINE(type, var, name) type var = NULL;
ASHLANDS_GL_FUNCTIONS(ASHLANDS_GL_DEFINE)
#undef ASHLANDS_GL_DEFINE

bool ashlands_gl_load(void) {
    bool ok = true;

#define ASHLANDS_GL_LOAD(type, var, name)                                        \
    do {                                                                         \
        var = (type)SDL_GL_GetProcAddress(#name);                                \
        if (!var) {                                                              \
            fprintf(stderr, "[gl] missing %s\n", #name);                       \
            ok = false;                                                          \
        }                                                                        \
    } while (0);

    ASHLANDS_GL_FUNCTIONS(ASHLANDS_GL_LOAD);

#undef ASHLANDS_GL_LOAD

    return ok;
}

#endif
