/*
 * main.c — Ashlands entry point
 * C11, MIT License
 *
 * «В пепельных пустошах нет героев.
 *  Есть только те, кто ещё не сдался.»
 */
#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
 * Parse a very simple command-line interface
 * ========================================================= */
static void parse_args(int argc, char **argv, EngineConfig *cfg) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fullscreen") == 0 ||
            strcmp(argv[i], "-f") == 0) {
            cfg->fullscreen = true;
        } else if (strcmp(argv[i], "--ascii") == 0) {
            cfg->render_mode = RENDER_ASCII;
        } else if (strcmp(argv[i], "--tiles") == 0) {
            cfg->render_mode = RENDER_TILES_2D;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            cfg->world_seed = (uint32_t)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            cfg->screen_w = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            cfg->screen_h = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--version") == 0 ||
                   strcmp(argv[i], "-v") == 0) {
            printf("Ashlands %s\n", ASHLANDS_VERSION_STR);
            exit(0);
        } else if (strcmp(argv[i], "--help") == 0 ||
                   strcmp(argv[i], "-h") == 0) {
            printf("Usage: ashlands [options]\n"
                   "  --fullscreen, -f   Start in fullscreen\n"
                   "  --ascii            Use ASCII renderer (default)\n"
                   "  --tiles            Use 2D tile renderer\n"
                   "  --seed N           Set world seed\n"
                   "  --width W          Screen width  (default 1024)\n"
                   "  --height H         Screen height (default 768)\n"
                   "  --version, -v      Show version and exit\n"
                   "  --help, -h         Show this help and exit\n");
            exit(0);
        }
    }
}

/* =========================================================
 * main
 * ========================================================= */
int main(int argc, char **argv) {
    EngineConfig cfg = engine_default_config();
    parse_args(argc, argv, &cfg);

    printf("Ashlands %s — starting (seed=%u)\n",
           ASHLANDS_VERSION_STR, cfg.world_seed);

    EngineState *eng = engine_create(&cfg);
    if (!eng) {
        fprintf(stderr, "[main] Failed to create engine\n");
        return 1;
    }

    engine_run(eng);
    engine_destroy(eng);

    printf("Goodbye from the ashlands.\n");
    return 0;
}
