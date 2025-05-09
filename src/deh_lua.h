// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_lua.h
/// \brief Lua SOC library

#ifndef __DEH_LUA_H__
#define __DEH_LUA_H__

#ifdef __cplusplus
extern "C" {
#endif

boolean LUA_SetLuaAction(void *state, const char *actiontocompare);
const char *LUA_GetActionName(void *action);
void LUA_SetActionByName(void *state, const char *actiontocompare);
size_t LUA_GetActionNumByName(const char *actiontocompare);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
