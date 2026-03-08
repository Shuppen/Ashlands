LOCAL_PATH := $(call my-dir)
SDL_PATH := $(LOCAL_PATH)/SDL2

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_C_INCLUDES := $(SDL_PATH)/include $(LOCAL_PATH)/src $(LOCAL_PATH)/include
LOCAL_SRC_FILES := \
    src/main.c \
    src/engine.c \
    src/ecs.c \
    src/world.c \
    src/input.c \
    src/ui.c \
    src/dialog_ui.c \
    src/faction.c \
    src/item.c \
    src/lua_api.c \
    src/npc.c \
    src/quest.c \
    src/save.c \
    src/texgen/noise.c \
    src/texgen/texcache.c \
    src/texgen/texgen.c \
    src/render/camera.c \
    src/render/mesh.c \
    src/render/gl_api.c \
    src/render/render_3d_scene.c \
    src/render/shader.c \
    src/render/render.c \
    src/render/render_ascii.c \
    src/render/render_3d.c \
    src/procgen/dungeon.c
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -lGLESv2 -llog -landroid -lm
LOCAL_CFLAGS := -DPLATFORM_ANDROID -DGL_GLEXT_PROTOTYPES
include $(BUILD_SHARED_LIBRARY)

include $(SDL_PATH)/Android.mk
