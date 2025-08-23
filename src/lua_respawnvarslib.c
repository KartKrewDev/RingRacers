// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by John "JTE" Muniz.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_respawnvarslib.c
/// \brief player respawn variables library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "lua_hook.h" // hook_cmd_running errors

enum respawnvars {
	respawnvars_state = 0,
	respawnvars_waypoint,
	respawnvars_pointx,
	respawnvars_pointy,
	respawnvars_pointz,
	respawnvars_pointangle,
	respawnvars_flip,
	respawnvars_timer,
	respawnvars_airtimer,
	respawnvars_distanceleft,
	respawnvars_dropdash,
	respawnvars_truedeath,
	respawnvars_manual,
	respawnvars_fromringshooter,
	respawnvars_init,
	respawnvars_fast,
	respawnvars_returnspeed,
};

static const char *const respawnvars_opt[] = {
	"state",
	"waypoint",
	"pointx",
	"pointy",
	"pointz",
	"pointangle",
	"flip",
	"timer",
	"airtimer",
	"distanceleft",
	"dropdash",
	"truedeath",
	"manual",
	"fromringshooter",
	"init",
	"fast",
	"returnspeed",
	NULL
};

#define RNOFIELD luaL_error(L, LUA_QL("respawnvars_t") " has no field named " LUA_QS, field)
#define RUNIMPLEMENTED luaL_error(L, LUA_QL("respawnvars_t") " unimplemented field " LUA_QS " cannot be read or set.", field)

static int respawn_get(lua_State *L)
{
	respawnvars_t *rsp = *((respawnvars_t **)luaL_checkudata(L, 1, META_RESPAWN));
	enum respawnvars field = luaL_checkoption(L, 2, NULL, respawnvars_opt);
	
	if (!rsp)
		return LUA_ErrInvalid(L, "player_t");
	
	switch (field)
	{
		case respawnvars_state:
			lua_pushinteger(L, rsp->state);
			break;
		case respawnvars_waypoint:
			LUA_PushUserdata(L, rsp->wp, META_WAYPOINT);
			break;
		case respawnvars_pointx:
			lua_pushfixed(L, rsp->pointx);
			break;
		case respawnvars_pointy:
			lua_pushfixed(L, rsp->pointy);
			break;
		case respawnvars_pointz:
			lua_pushfixed(L, rsp->pointz);
			break;
		case respawnvars_pointangle:
			lua_pushangle(L, rsp->pointangle);
			break;
		case respawnvars_flip:
			lua_pushboolean(L, rsp->flip);
			break;
		case respawnvars_timer:
			lua_pushinteger(L, rsp->timer);
			break;
		case respawnvars_airtimer:
			lua_pushinteger(L, rsp->airtimer);
			break;
		case respawnvars_distanceleft:
			lua_pushinteger(L, rsp->distanceleft);
			break;
		case respawnvars_dropdash:
			lua_pushinteger(L, rsp->dropdash);
			break;
		case respawnvars_truedeath:
			lua_pushboolean(L, rsp->truedeath);
			break;
		case respawnvars_manual:
			lua_pushboolean(L, rsp->manual);
			break;
		case respawnvars_fromringshooter:
			lua_pushboolean(L, rsp->fromRingShooter);
			break;
		case respawnvars_init:
			lua_pushboolean(L, rsp->init);
			break;
		case respawnvars_fast:
			lua_pushboolean(L, rsp->fast);
			break;
		case respawnvars_returnspeed:
			lua_pushfixed(L, rsp->returnspeed);
			break;
		default:
			return RNOFIELD;
	}

	return 1;
}

static int respawn_set(lua_State *L)
{
	respawnvars_t *rsp = *((respawnvars_t **)luaL_checkudata(L, 1, META_RESPAWN));
	enum respawnvars field = luaL_checkoption(L, 2, respawnvars_opt[0], respawnvars_opt);

	if (!rsp)
		return LUA_ErrInvalid(L, "respawnvars_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in CMD building code!");
	
	switch (field)
	{
		case respawnvars_state:
			rsp->state = (UINT8)luaL_checkinteger(L, 3);
			break;
		case respawnvars_waypoint:
			rsp->wp = *((waypoint_t **)luaL_checkudata(L, 3, META_WAYPOINT));
			break;
		case respawnvars_pointx:
			rsp->pointx = luaL_checkfixed(L, 3);
			break;
		case respawnvars_pointy:
			rsp->pointy = luaL_checkfixed(L, 3);
			break;
		case respawnvars_pointz:
			rsp->pointz = luaL_checkfixed(L, 3);
			break;
		case respawnvars_pointangle:
			rsp->pointangle = luaL_checkangle(L, 3);
			break;
		case respawnvars_flip:
			rsp->flip = luaL_checkboolean(L, 3);
			break;
		case respawnvars_timer:
			rsp->timer = (tic_t)luaL_checkinteger(L, 3);
			break;
		case respawnvars_airtimer:
			rsp->airtimer = (tic_t)luaL_checkinteger(L, 3);
			break;
		case respawnvars_distanceleft:
			rsp->distanceleft = (UINT32)luaL_checkinteger(L, 3);
			break;
		case respawnvars_dropdash:
			rsp->dropdash = (tic_t)luaL_checkinteger(L, 3);
			break;
		case respawnvars_truedeath:
			rsp->truedeath = luaL_checkboolean(L, 3);
			break;
		case respawnvars_manual:
			rsp->manual = luaL_checkboolean(L, 3);
			break;
		case respawnvars_fromringshooter:
			rsp->fromRingShooter = luaL_checkboolean(L, 3);
			break;
		case respawnvars_init:
			rsp->init = luaL_checkboolean(L, 3);
			break;
		case respawnvars_fast:
			rsp->fast = luaL_checkboolean(L, 3);
			break;
		case respawnvars_returnspeed:
			rsp->returnspeed = luaL_checkfixed(L, 3);
			break;
		default:
			return RNOFIELD;
	}

	return 0;
}

#undef RNOFIELD
#undef RUNIMPLEMENTED

int LUA_RespawnVarsLib(lua_State *L)
{
	luaL_newmetatable(L, META_RESPAWN);
		lua_pushcfunction(L, respawn_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, respawn_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	return 0;
}
