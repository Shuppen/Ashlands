#ifndef ASHLANDS_GL_H
#define ASHLANDS_GL_H

#include <stdbool.h>

#include "ashlands_sdl.h"

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)

#if defined(__has_include)
#if __has_include(<SDL_opengles2.h>)
#include <SDL_opengles2.h>
#elif __has_include(<SDL2/SDL_opengles2.h>)
#include <SDL2/SDL_opengles2.h>
#else
#error "SDL OpenGL ES headers not found"
#endif
#else
#include <SDL2/SDL_opengles2.h>
#endif

static inline bool ashlands_gl_load(void) {
    return true;
}

#else

#if defined(__has_include)
#if __has_include(<SDL_opengl.h>)
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#elif __has_include(<SDL2/SDL_opengl.h>)
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#else
#error "SDL OpenGL headers not found"
#endif
#else
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#endif

bool ashlands_gl_load(void);

extern PFNGLACTIVETEXTUREPROC ash_glActiveTexture;
extern PFNGLATTACHSHADERPROC ash_glAttachShader;
extern PFNGLBINDBUFFERPROC ash_glBindBuffer;
extern PFNGLBINDATTRIBLOCATIONPROC ash_glBindAttribLocation;
extern PFNGLBUFFERDATAPROC ash_glBufferData;
extern PFNGLCOMPILESHADERPROC ash_glCompileShader;
extern PFNGLCREATEPROGRAMPROC ash_glCreateProgram;
extern PFNGLCREATESHADERPROC ash_glCreateShader;
extern PFNGLDELETEBUFFERSPROC ash_glDeleteBuffers;
extern PFNGLDELETEPROGRAMPROC ash_glDeleteProgram;
extern PFNGLDELETESHADERPROC ash_glDeleteShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC ash_glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC ash_glEnableVertexAttribArray;
extern PFNGLGENBUFFERSPROC ash_glGenBuffers;
extern PFNGLGENERATEMIPMAPPROC ash_glGenerateMipmap;
extern PFNGLGETPROGRAMINFOLOGPROC ash_glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC ash_glGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC ash_glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC ash_glGetShaderiv;
extern PFNGLGETUNIFORMLOCATIONPROC ash_glGetUniformLocation;
extern PFNGLLINKPROGRAMPROC ash_glLinkProgram;
extern PFNGLSHADERSOURCEPROC ash_glShaderSource;
extern PFNGLUNIFORM1FPROC ash_glUniform1f;
extern PFNGLUNIFORM1IPROC ash_glUniform1i;
extern PFNGLUNIFORM3FPROC ash_glUniform3f;
extern PFNGLUNIFORMMATRIX4FVPROC ash_glUniformMatrix4fv;
extern PFNGLUSEPROGRAMPROC ash_glUseProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC ash_glVertexAttribPointer;

#define glActiveTexture ash_glActiveTexture
#define glAttachShader ash_glAttachShader
#define glBindBuffer ash_glBindBuffer
#define glBindAttribLocation ash_glBindAttribLocation
#define glBufferData ash_glBufferData
#define glCompileShader ash_glCompileShader
#define glCreateProgram ash_glCreateProgram
#define glCreateShader ash_glCreateShader
#define glDeleteBuffers ash_glDeleteBuffers
#define glDeleteProgram ash_glDeleteProgram
#define glDeleteShader ash_glDeleteShader
#define glDisableVertexAttribArray ash_glDisableVertexAttribArray
#define glEnableVertexAttribArray ash_glEnableVertexAttribArray
#define glGenBuffers ash_glGenBuffers
#define glGenerateMipmap ash_glGenerateMipmap
#define glGetProgramInfoLog ash_glGetProgramInfoLog
#define glGetProgramiv ash_glGetProgramiv
#define glGetShaderInfoLog ash_glGetShaderInfoLog
#define glGetShaderiv ash_glGetShaderiv
#define glGetUniformLocation ash_glGetUniformLocation
#define glLinkProgram ash_glLinkProgram
#define glShaderSource ash_glShaderSource
#define glUniform1f ash_glUniform1f
#define glUniform1i ash_glUniform1i
#define glUniform3f ash_glUniform3f
#define glUniformMatrix4fv ash_glUniformMatrix4fv
#define glUseProgram ash_glUseProgram
#define glVertexAttribPointer ash_glVertexAttribPointer

#endif

#endif
