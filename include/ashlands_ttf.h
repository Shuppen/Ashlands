#ifndef ASHLANDS_TTF_H
#define ASHLANDS_TTF_H

#include "ashlands_sdl.h"

#if defined(__has_include)
#if __has_include(<SDL_ttf.h>)
#include <SDL_ttf.h>
#elif __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#error "SDL_ttf headers not found"
#endif
#else
#include <SDL2/SDL_ttf.h>
#endif

#endif
