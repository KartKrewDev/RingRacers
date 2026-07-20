// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef LUA_PROFILE_H
#define LUA_PROFILE_H

#include "blua/lua.h"
#include "doomtype.h"
#include "typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lua_timer_t;

void LUA_ResetTicTimers(void);

lua_timer_t *LUA_BeginFunctionTimer(lua_State *L, int fn_idx, const char *name);
void LUA_EndFunctionTimer(lua_timer_t *timer);

void LUA_RenderTimers(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*LUA_PROFILE_H*/
