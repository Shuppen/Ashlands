# AGENTS.md

Repo-specific guide for agentic coding assistants working in Ashlands.
Use this together with `README.md`, `CONTRIBUTING.md`, `Makefile`, `CMakeLists.txt`, and `.github/workflows/*.yml`.

## Local Rule Sources

- No `.cursor/rules/` directory exists in this worktree.
- No `.cursorrules` file exists in this worktree.
- No `.github/copilot-instructions.md` file exists in this worktree.
- If any of those files appear later, merge their rules into this document.

## Project Summary

- Project: Ashlands
- Language: C11
- Stack: SDL2, SDL2_ttf, SDL2_image, SDL2_mixer, LuaJIT, OpenGL / GLES2
- CI targets: Linux, Windows, macOS, WebAssembly, Android
- Main binary: `ashlands`
- Main roots: `src/`, `include/`, `assets/`, `mods/`, `android/`, `web/`

## Important Paths

- `src/main.c`: CLI entry and argument parsing
- `src/engine.c` / `src/engine.h`: engine setup and main loop
- `src/ecs.c` / `src/ecs.h`: ECS storage
- `src/world.c` / `src/world.h`: world state, tiles, FOV, time
- `src/input.c` / `src/input.h`: SDL input translation
- `src/ui.c` / `src/ui.h`: HUD, logs, dialog overlays
- `src/render/`: renderer abstraction and backends
- `src/texgen/`: procedural texture generation and cache
- `src/lua_api.c` / `src/lua_api.h`: Lua integration
- `include/ashlands*.h`: shared types and portability wrappers
- `.github/workflows/`: CI and deploy workflows

## Build Commands

Prefer CI-parity commands.

### Native build

```bash
make -j$(nproc)
```

### Run locally

```bash
./ashlands
```

### CLI smoke checks

```bash
./ashlands --help
./ashlands --version
```

### Clean

```bash
make clean
```

### CMake build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Web build

```bash
make web
python3 -m http.server 8080 --directory build/web
```

### Android CI-parity build

Run from `android/` after the wrapper exists:

```bash
./gradlew assembleRelease
```

## Test And Lint Reality

- There is no dedicated unit test suite, `ctest`, or standalone test runner yet.
- There is no configured formatter or linter like `clang-format`, `clang-tidy`, or `cppcheck`.
- Current quality baseline: successful build, CLI smoke checks, clean diff, and clean CI.

### Recommended validation sequence

```bash
make clean
make -j$(nproc)
./ashlands --help
./ashlands --version
git diff --check
```

### Single test guidance

There is no real single-test command. If asked for one test, say that clearly and use the narrowest substitute:

- Rebuild one file: `make src/world.o`
- Rebuild one renderer file: `make src/render/render_3d.o`
- Rebuild one GL helper file: `make src/render/gl_api.o`
- Minimal smoke check: `./ashlands --help`
- Second smoke check: `./ashlands --version`

If a real test harness is added, update this file.

## Dependency Notes

- Linux CI installs `gcc`, `make`, `cmake`, SDL2 dev packages, LuaJIT headers, and Mesa GL.
- macOS CI uses Homebrew packages: `sdl2`, `sdl2_ttf`, `sdl2_image`, `sdl2_mixer`, `luajit`.
- Windows CI uses MSYS2 UCRT64 with GCC, CMake, Ninja, SDL2 packages, and LuaJIT.
- Android CI downloads SDL2 source, links repo `src/` and `include/` into the NDK tree, then runs Gradle.
- Web CI uses Emscripten and `make web`.

## Code Style Guide

### Language and portability

- Write pure C11.
- Keep changes portable across Linux, Windows, macOS, Web, and Android.
- Avoid `system()`, `fork()`, `popen()`, and platform-specific shell tricks.
- Prefer SDL abstractions or standard C library APIs for portable I/O.

### Includes and imports

- Include the file's own project header first when it has one.
- Then include related project headers, then standard library headers.
- Prefer wrappers from `include/`: `ashlands_sdl.h`, `ashlands_ttf.h`, `ashlands_gl.h`.
- Do not add new direct `#include <SDL2/...>` lines without a strong reason.

### Formatting

- Use 4-space indentation.
- Use K&R braces for functions and control flow.
- Match surrounding style exactly.
- Keep existing banner comments if the file already uses them.
- Do not reformat unrelated code.

### Naming

- Functions, locals, parameters, and struct fields use `snake_case`.
- Types use descriptive typedef names like `EngineState`, `WorldState`, `UIState`.
- Enum constants and macros use `UPPER_SNAKE_CASE`.
- Existing patterns matter: `g_` for globals, `s_` for file-local static state.
- Header guards use `ASHLANDS_*_H`.

### Types and constants

- Prefer `bool`, `uint32_t`, `uint64_t`, `size_t`, and explicit-width integer types.
- Reuse existing limits like `SAVE_PATH_LEN`, `ITEM_NAME_LEN`, `UI_LOG_MAX_LEN`, and `TEXGEN_NAME_LEN`.
- Keep public enums and structs consistent with `include/ashlands.h` and nearby headers.

### Error handling and resources

- Prefer early returns on invalid input, allocation failure, file failure, and init failure.
- Return `NULL` for pointer failures and `false` for boolean failures.
- Print human-readable diagnostics to `stderr` with subsystem prefixes like `[main]`, `[shader]`, `[render_3d]`, and `[gl]`.
- Include `SDL_GetError()` or `TTF_GetError()` when useful.
- Every `malloc` / `calloc` must have a matching `free` on all ownership paths.
- Pair SDL and OpenGL resources with the correct destroy/delete functions.

### Strings, rendering, and docs

- Use bounded functions such as `snprintf`.
- If using `strncpy`, explicitly null-terminate when needed.
- Target OpenGL 2.0 / GLSL 1.20 on desktop and GLES2-compatible paths on web/android.
- Desktop modern GL entry points should go through `src/render/gl_api.c` and `include/ashlands_gl.h`.
- Update `README.md` or `templates/README.md` when user-facing or Lua-facing behavior changes.

## Change Scope Rules

- Keep edits focused on the requested task.
- Avoid broad cleanup, file moves, or renames unless required.
- If a file is already very large, prefer a clean split over making it significantly larger.
- Preserve public APIs unless the task clearly requires an API change.

## CI And GitHub Guidance

Useful commands:

```bash
gh run list --limit 10
gh run view <run-id> --log-failed
git diff --check
```

- Main CI workflow: `.github/workflows/build.yml`
- Pages deploy workflow: `.github/workflows/build-and-deploy.yml`
- Strong completion state: local validation passes, changes are committed, pushed, and relevant Actions runs were checked.

## Required Agent Endgame

- Do not stop at local edits if the task is meant to be completed end-to-end.
- After code changes, validate locally, commit, push, and inspect GitHub Actions.
- If acting as a maintainer on the main repo, push to the expected upstream branch for the task.
- If acting as a contributor, push to a separate branch or personal fork, not directly to upstream `main`.
- Contributors must verify GitHub Actions on that branch or fork before handoff.
- If CI fails, inspect logs, fix the issue, push again, and re-check runs.
- In the final handoff, report the commit SHA, pushed branch, and Actions status.
