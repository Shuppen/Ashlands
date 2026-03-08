cmake_minimum_required(VERSION 3.16)
project(Ashlands VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Options --------------------------------------------------------
option(ASHLANDS_ENABLE_LTO "Enable link-time optimisation" OFF)

# --- Find dependencies ----------------------------------------------
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# SDL2 extension libs (not always in CMake modules)
find_library(SDL2_TTF_LIB NAMES SDL2_ttf)
find_library(SDL2_IMAGE_LIB NAMES SDL2_image)
find_library(SDL2_MIXER_LIB NAMES SDL2_mixer)
find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
    PATHS /usr/include
          /usr/local/include
          /opt/homebrew/include
          /opt/homebrew/opt/sdl2_ttf/include
          /ucrt64/include
          /mingw64/include
    PATH_SUFFIXES SDL2 SDL2_ttf)

# LuaJIT
find_library(LUAJIT_LIB NAMES luajit-5.1 luajit)
find_path(LUAJIT_INCLUDE_DIR lua.h
    PATHS /usr/include/luajit-2.1
           /usr/local/include/luajit-2.1
          /opt/homebrew/include/luajit-2.1
           /usr/include/lua5.1
          /ucrt64/include/luajit-2.1
          /mingw64/include/luajit-2.1
    PATH_SUFFIXES luajit-2.1 lua5.1)

set(ASHLANDS_PLATFORM_LIBS)
if(UNIX AND NOT APPLE)
    list(APPEND ASHLANDS_PLATFORM_LIBS m dl)
elseif(APPLE)
    list(APPEND ASHLANDS_PLATFORM_LIBS m)
endif()

set(ASHLANDS_SDL2_LINK_LIBS)
if(TARGET SDL2::SDL2main AND WIN32)
    list(APPEND ASHLANDS_SDL2_LINK_LIBS SDL2::SDL2main)
endif()

if(TARGET SDL2::SDL2)
    list(APPEND ASHLANDS_SDL2_LINK_LIBS SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    list(APPEND ASHLANDS_SDL2_LINK_LIBS SDL2::SDL2-static)
elseif(SDL2_LIBRARIES)
    list(APPEND ASHLANDS_SDL2_LINK_LIBS ${SDL2_LIBRARIES})
else()
    message(FATAL_ERROR "SDL2 library target was not found")
endif()

set(ASHLANDS_OPENGL_LIBS)
if(TARGET OpenGL::GL)
    list(APPEND ASHLANDS_OPENGL_LIBS OpenGL::GL)
elseif(OPENGL_LIBRARIES)
    list(APPEND ASHLANDS_OPENGL_LIBS ${OPENGL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenGL library target was not found")
endif()

foreach(required_lib IN ITEMS SDL2_TTF_LIB SDL2_IMAGE_LIB SDL2_MIXER_LIB LUAJIT_LIB)
    if(NOT ${required_lib})
        message(FATAL_ERROR "Missing required dependency: ${required_lib}")
    endif()
endforeach()

if(NOT SDL2_TTF_INCLUDE_DIR)
    message(FATAL_ERROR "Missing required dependency: SDL2_TTF_INCLUDE_DIR")
endif()

if(NOT LUAJIT_INCLUDE_DIR)
    message(FATAL_ERROR "Missing required dependency: LUAJIT_INCLUDE_DIR")
endif()

# --- Source files ---------------------------------------------------
set(ASHLANDS_SOURCES
    src/main.c
    src/engine.c
    src/ecs.c
    src/world.c
    src/input.c
    src/ui.c
    src/dialog_ui.c
    src/faction.c
    src/item.c
    src/lua_api.c
    src/npc.c
    src/quest.c
    src/save.c
    src/texgen/noise.c
    src/texgen/texcache.c
    src/texgen/texgen.c
    src/render/camera.c
    src/render/mesh.c
    src/render/gl_api.c
    src/render/render_3d_scene.c
    src/render/shader.c
    src/render/render.c
    src/render/render_ascii.c
    src/render/render_3d.c
    src/procgen/dungeon.c
)

# --- Executable -----------------------------------------------------
add_executable(ashlands ${ASHLANDS_SOURCES})

target_include_directories(ashlands PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_TTF_INCLUDE_DIR}
    ${LUAJIT_INCLUDE_DIR}
)

target_link_libraries(ashlands PRIVATE
    ${ASHLANDS_SDL2_LINK_LIBS}
    ${SDL2_TTF_LIB}
    ${SDL2_IMAGE_LIB}
    ${SDL2_MIXER_LIB}
    ${LUAJIT_LIB}
    ${ASHLANDS_OPENGL_LIBS}
    ${ASHLANDS_PLATFORM_LIBS}
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
