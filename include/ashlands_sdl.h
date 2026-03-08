#ifndef ASHLANDS_SDL_H
#define ASHLANDS_SDL_H

#if defined(__has_include)
#if __has_include(<SDL.h>)
#include <SDL.h>
#elif __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#error "SDL headers not found"
#endif
#else
#include <SDL2/SDL.h>
#endif

#endif
