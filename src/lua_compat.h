#ifndef ASHLANDS_LUA_COMPAT_H
#define ASHLANDS_LUA_COMPAT_H

#include <stddef.h>

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)

struct lua_State;
typedef struct lua_State lua_State;
typedef long long lua_Integer;
typedef int (*lua_CFunction)(lua_State *L);

typedef struct luaL_Reg {
    const char *name;
    lua_CFunction func;
} luaL_Reg;

#ifndef LUA_NOREF
#define LUA_NOREF (-2)
#endif

#ifndef LUA_REFNIL
#define LUA_REFNIL (-1)
#endif

#ifndef LUA_TTABLE
#define LUA_TTABLE 5
#endif

#ifndef LUA_REGISTRYINDEX
#define LUA_REGISTRYINDEX (-1001000)
#endif

#ifndef lua_absindex
#define lua_absindex(L, idx) (idx)
#endif

#ifndef lua_rawlen
#define lua_rawlen(L, idx) lua_objlen((L), (idx))
#endif

#ifndef lua_pop
#define lua_pop(L, n) ((void)(L), (void)(n))
#endif

static inline int lua_gettop(lua_State *L) {
    (void)L;
    return 0;
}

static inline void lua_setglobal(lua_State *L, const char *name) {
    (void)L;
    (void)name;
}

static inline void lua_getglobal(lua_State *L, const char *name) {
    (void)L;
    (void)name;
}

static inline void lua_getfield(lua_State *L, int index, const char *key) {
    (void)L;
    (void)index;
    (void)key;
}

static inline void lua_pushnil(lua_State *L) {
    (void)L;
}

static inline int lua_next(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline void lua_pushboolean(lua_State *L, int value) {
    (void)L;
    (void)value;
}

static inline void lua_pushinteger(lua_State *L, lua_Integer value) {
    (void)L;
    (void)value;
}

static inline void lua_pushstring(lua_State *L, const char *value) {
    (void)L;
    (void)value;
}

static inline void lua_pushvalue(lua_State *L, int index) {
    (void)L;
    (void)index;
}

static inline void lua_pushcfunction(lua_State *L, lua_CFunction fn) {
    (void)L;
    (void)fn;
}

static inline int lua_error(lua_State *L) {
    (void)L;
    return 0;
}

static inline void lua_createtable(lua_State *L, int narr, int nrec) {
    (void)L;
    (void)narr;
    (void)nrec;
}

static inline void lua_rawgeti(lua_State *L, int index, int n) {
    (void)L;
    (void)index;
    (void)n;
}

static inline void lua_rawseti(lua_State *L, int index, int n) {
    (void)L;
    (void)index;
    (void)n;
}

static inline int lua_objlen(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline int lua_isfunction(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline int lua_istable(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline int lua_isnumber(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline int lua_isboolean(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline int lua_isstring(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline int lua_isnil(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 1;
}

static inline const char *lua_tostring(lua_State *L, int index) {
    (void)L;
    (void)index;
    return NULL;
}

static inline int lua_toboolean(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline lua_Integer lua_tointeger(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline double lua_tonumber(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0.0;
}

static inline void luaL_checktype(lua_State *L, int index, int type) {
    (void)L;
    (void)index;
    (void)type;
}

static inline const char *luaL_checkstring(lua_State *L, int index) {
    (void)L;
    (void)index;
    return "";
}

static inline lua_Integer luaL_checkinteger(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0;
}

static inline double luaL_checknumber(lua_State *L, int index) {
    (void)L;
    (void)index;
    return 0.0;
}

static inline const char *luaL_optstring(lua_State *L, int index, const char *def) {
    (void)L;
    (void)index;
    return def;
}

static inline lua_Integer luaL_optinteger(lua_State *L, int index, lua_Integer def) {
    (void)L;
    (void)index;
    return def;
}

static inline double luaL_optnumber(lua_State *L, int index, double def) {
    (void)L;
    (void)index;
    return def;
}

static inline int luaL_ref(lua_State *L, int table_index) {
    (void)L;
    (void)table_index;
    return LUA_NOREF;
}

static inline void luaL_unref(lua_State *L, int table_index, int ref) {
    (void)L;
    (void)table_index;
    (void)ref;
}

static inline lua_State *luaL_newstate(void) {
    return NULL;
}

static inline void luaL_openlibs(lua_State *L) {
    (void)L;
}

static inline int luaL_dofile(lua_State *L, const char *filename) {
    (void)L;
    (void)filename;
    return -1;
}

static inline int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc) {
    (void)L;
    (void)nargs;
    (void)nresults;
    (void)errfunc;
    return -1;
}

static inline void lua_close(lua_State *L) {
    (void)L;
}

#else

#if defined(__has_include)
#if __has_include(<lua.h>)
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#elif __has_include(<luajit-2.1/lua.h>)
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>
#elif __has_include(<lua5.1/lua.h>)
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#else
#error "Lua headers not found"
#endif
#else
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

#ifndef lua_absindex
#define lua_absindex(L, idx) \
    (((idx) > 0 || (idx) <= LUA_REGISTRYINDEX) ? (idx) : lua_gettop(L) + (idx) + 1)
#endif

#ifndef lua_rawlen
#define lua_rawlen(L, idx) lua_objlen((L), (idx))
#endif

#endif

#endif
