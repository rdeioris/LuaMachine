// Copyright 2024-2025 - Roberto De Ioris

#pragma once

#include "ThirdParty/lua/lua.hpp"
#define LUAMACHINE_RETURN_ERROR(L, Fmt, ...) return luaL_error(L, Fmt, ##__VA_ARGS__)
