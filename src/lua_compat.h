#ifndef ASHLANDS_LUA_COMPAT_H
#define ASHLANDS_LUA_COMPAT_H

#ifdef PLATFORM_WEB

struct lua_State;
typedef struct lua_State lua_State;

#ifndef LUA_NOREF
#define LUA_NOREF (-2)
#endif

#ifndef LUA_REFNIL
#define LUA_REFNIL (-1)
#endif

#else

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef lua_absindex
#define lua_absindex(L, idx) \
    (((idx) > 0 || (idx) <= LUA_REGISTRYINDEX) ? (idx) : lua_gettop(L) + (idx) + 1)
#endif

#ifndef lua_rawlen
#define lua_rawlen(L, idx) lua_objlen((L), (idx))
#endif

#endif

#endif
