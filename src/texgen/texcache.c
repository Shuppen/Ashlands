#include "texcache.h"

#include "../../include/ashlands_gl.h"

#include <stdlib.h>
#include <string.h>

typedef struct TexCacheEntry {
    char *name;
    void *texture;
    struct TexCacheEntry *next;
} TexCacheEntry;

static TexCacheEntry *s_texcache = NULL;

static void texcache_destroy_payload(const char *name, void *texture) {
    if (!name || !texture) {
        return;
    }

    if (strncmp(name, "sdl:", 4) == 0) {
        SDL_DestroyTexture((SDL_Texture *)texture);
        return;
    }

    if (strncmp(name, "gl:", 3) == 0) {
        GLuint id = *(GLuint *)texture;
        if (id) {
            glDeleteTextures(1, &id);
        }
        free(texture);
    }
}

void texcache_init(void) {
    texcache_clear();
}

void *texcache_get(const char *name) {
    TexCacheEntry *entry = s_texcache;

    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry->texture;
        }
        entry = entry->next;
    }

    return NULL;
}

void texcache_put(const char *name, void *texture) {
    TexCacheEntry *entry = s_texcache;

    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            texcache_destroy_payload(entry->name, entry->texture);
            entry->texture = texture;
            return;
        }
        entry = entry->next;
    }

    entry = calloc(1, sizeof(*entry));
    if (!entry) {
        return;
    }

    entry->name = malloc(strlen(name) + 1);
    if (!entry->name) {
        free(entry);
        return;
    }

    strcpy(entry->name, name);
    entry->texture = texture;
    entry->next = s_texcache;
    s_texcache = entry;
}

void texcache_clear(void) {
    TexCacheEntry *entry = s_texcache;

    while (entry) {
        TexCacheEntry *next = entry->next;
        texcache_destroy_payload(entry->name, entry->texture);
        free(entry->name);
        free(entry);
        entry = next;
    }

    s_texcache = NULL;
}
