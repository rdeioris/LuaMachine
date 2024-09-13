// Copyright 2024 - Roberto De Ioris

#pragma once

#if LUAMACHINE_LUA53
#include "ThirdParty/lua/lua.hpp"
#define LUAMACHINE_RETURN_ERROR(L, Fmt, ...) return luaL_error(L, Fmt, ##__VA_ARGS__)
#elif LUAMACHINE_LUAU
#include "ThirdParty/luau/Compiler/luacode.h"
#include "ThirdParty/luau/VM/lualib.h"

#define lua_pushglobaltable(L) lua_pushvalue(L,LUA_GLOBALSINDEX)

#define luaL_ref lua_ref
#define luaL_unref(L, PlaceHolder, Ref) lua_unref(L, Ref) 

#undef lua_pushcfunction
#define lua_pushcfunction(L, fn) lua_pushcclosurek(L, fn, "", 0, NULL)

int lua_isinteger(lua_State* L, int index);
void lua_seti(lua_State* L, int index, lua_Integer i);
#ifndef LUA_EXTRASPACE
#define LUA_EXTRASPACE (sizeof(void*))
#endif
void* lua_getextraspace(lua_State* L);

void lua_len(lua_State* L, int i);
lua_Integer luaL_len(lua_State* L, int i);

#define LUAMACHINE_RETURN_ERROR(L, Fmt, ...) luaL_error(L, Fmt, ##__VA_ARGS__)
#endif