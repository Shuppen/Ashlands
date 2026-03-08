#ifndef ASHLANDS_TEXCACHE_H
#define ASHLANDS_TEXCACHE_H

void texcache_init(void);
void *texcache_get(const char *name);
void texcache_put(const char *name, void *texture);
void texcache_clear(void);

#endif
