#ifndef ASHLANDS_SAVE_H
#define ASHLANDS_SAVE_H

#include <stdbool.h>

typedef struct AshEngineState AshEngineState;

bool save_game(const char *path, const AshEngineState *eng);
bool load_game(const char *path, AshEngineState *eng);

#endif
