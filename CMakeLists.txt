cmake_minimum_required(VERSION 3.16)
project(Ashlands VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Options --------------------------------------------------------
option(ASHLANDS_ENABLE_LTO "Enable link-time optimisation" OFF)

# --- Find dependencies ----------------------------------------------
find_package(SDL2   REQUIRED)
find_package(OpenGL REQUIRED)

# SDL2 extension libs (not always in CMake modules)
find_library(SDL2_TTF_LIB   SDL2_ttf)
find_library(SDL2_IMAGE_LIB SDL2_image)
find_library(SDL2_MIXER_LIB SDL2_mixer)

# LuaJIT
find_library(LUAJIT_LIB luajit-5.1 NAMES luajit)
find_path(LUAJIT_INCLUDE_DIR lua.h
    PATHS /usr/include/luajit-2.1
          /usr/local/include/luajit-2.1
          /usr/include/lua5.1)

# --- Source files ---------------------------------------------------
set(ASHLANDS_SOURCES
    src/main.c
    src/engine.c
    src/ecs.c
    src/world.c
    src/input.c
    src/ui.c
    src/lua_api.c
    src/render/camera.c
    src/render/render_ascii.c
    src/procgen/dungeon.c
)

# --- Executable -----------------------------------------------------
add_executable(ashlands ${ASHLANDS_SOURCES})

target_include_directories(ashlands PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${LUAJIT_INCLUDE_DIR}
)

target_link_libraries(ashlands
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIB}
    ${SDL2_IMAGE_LIB}
    ${SDL2_MIXER_LIB}
    ${LUAJIT_LIB}
    ${OPENGL_LIBRARIES}
    m
    dl
)

if(ASHLANDS_ENABLE_LTO)
    set_property(TARGET ashlands PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# --- Compiler warnings ----------------------------------------------
target_compile_options(ashlands PRIVATE
    $<$<C_COMPILER_ID:GNU>:   -Wall -Wextra -Wno-unused-parameter>
    $<$<C_COMPILER_ID:Clang>: -Wall -Wextra -Wno-unused-parameter>
)

# --- Install --------------------------------------------------------
install(TARGETS ashlands DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/ashlands/assets)
install(DIRECTORY mods/   DESTINATION share/ashlands/mods)
