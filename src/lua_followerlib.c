// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by John "JTE" Muniz.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_followerlib.c
/// \brief player follower structure library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "k_follower.h"
#include "r_skins.h"
#include "sounds.h"

#include "lua_script.h"
#include "lua_libs.h"

enum follower {
	follower_valid = 0,
	follower_name,
	follower_icon,
	follower_category,
	follower_defaultcolor,
	follower_mode,
	follower_scale,
	follower_bubblescale,
	follower_atangle,
	follower_dist,
	follower_height,
	follower_zoffs,
	follower_horzlag,
	follower_vertlag,
	follower_anglelag,
	follower_bobamp,
	follower_bobspeed,
	// states
	follower_idlestate,
	follower_followstate,
	follower_hurtstate,
	follower_winstate,
	follower_losestate,
	follower_hitconfirmstate,
	follower_hitconfirmtime,
	follower_ringstate,
	follower_ringtime,
	//
	follower_hornsound,
};
static const char *const follower_opt[] = {
	"valid",
	"name",
	"icon",
	"category",
	"defaultcolor",
	"mode",
	"scale",
	"bubblescale",
	"atangle",
	"dist",
	"height",
	"zoffs",
	"horzlag",
	"vertlag",
	"anglelag",
	"bobamp",
	"bobspeed",
	// states
	"idlestate",
	"followstate",
	"hurtstate",
	"winstate",
	"losestate",
	"hitconfirmstate",
	"hitconfirmtime",
	"ringstate",
	"ringtime",
	//
	"hornsound",
	NULL
};

#define UNIMPLEMENTED luaL_error(L, LUA_QL("follower_t") " field " LUA_QS " is not implemented for Lua and cannot be accessed.", follower_opt[field])

static int follower_get(lua_State *L)
{
	follower_t *follower = *((follower_t **)luaL_checkudata(L, 1, META_FOLLOWER));
	enum follower field = luaL_checkoption(L, 2, NULL, follower_opt);

	// followers are always valid, only added, never removed
	I_Assert(follower != NULL);

	switch (field)
	{
	case follower_valid:
		lua_pushboolean(L, follower != NULL);
		break;
	case follower_name:
		lua_pushstring(L, follower->name);
		break;
	case follower_icon:
		lua_pushstring(L, follower->icon);
		break;
	case follower_category:
		// This would require me to expose followercategory_t as well
		// Not doing that for now, so this has no use.
		return UNIMPLEMENTED;
	case follower_defaultcolor:
		lua_pushinteger(L, follower->defaultcolor);
		break;
	case follower_mode:
		lua_pushinteger(L, follower->mode);
		break;
	case follower_scale:
		lua_pushfixed(L, follower->scale);
		break;
	case follower_bubblescale:
		lua_pushfixed(L, follower->bubblescale);
		break;
	case follower_atangle:
		lua_pushangle(L, follower->atangle);
		break;
	case follower_dist:
		lua_pushfixed(L, follower->dist);
		break;
	case follower_height:
		lua_pushfixed(L, follower->height);
		break;
	case follower_zoffs:
		lua_pushfixed(L, follower->zoffs);
		break;
	case follower_horzlag:
		lua_pushfixed(L, follower->horzlag);
		break;
	case follower_vertlag:
		lua_pushfixed(L, follower->vertlag);
		break;
	case follower_anglelag:
		lua_pushfixed(L, follower->anglelag);
		break;
	case follower_bobamp:
		lua_pushfixed(L, follower->bobamp);
		break;
	case follower_bobspeed:
		lua_pushinteger(L, follower->bobspeed);
		break;
	case follower_idlestate:
		lua_pushinteger(L, follower->idlestate);
		break;
	case follower_followstate:
		lua_pushinteger(L, follower->followstate);
		break;
	case follower_hurtstate:
		lua_pushinteger(L, follower->hurtstate);
		break;
	case follower_winstate:
		lua_pushinteger(L, follower->winstate);
		break;
	case follower_losestate:
		lua_pushinteger(L, follower->losestate);
		break;
	case follower_hitconfirmstate:
		lua_pushinteger(L, follower->hitconfirmstate);
		break;
	case follower_hitconfirmtime:
		lua_pushinteger(L, follower->hitconfirmtime);
		break;
	case follower_ringstate:
		lua_pushinteger(L, follower->ringstate);
		break;
	case follower_ringtime:
		lua_pushinteger(L, follower->ringtime);
		break;
	case follower_hornsound:
		lua_pushinteger(L, follower->hornsound);
		break;
	}
	return 1;
}

static int follower_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("follower_t") " struct cannot be edited by Lua.");
}

static int follower_num(lua_State *L)
{
	follower_t *follower = *((follower_t **)luaL_checkudata(L, 1, META_FOLLOWER));

	// followers are always valid, only added, never removed
	I_Assert(follower != NULL);

	lua_pushinteger(L, follower-followers);
	return 1;
}

static int lib_iterateFollowers(lua_State *L)
{
	INT32 i;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateFollowers);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = (INT32)(*((follower_t **)luaL_checkudata(L, 1, META_FOLLOWER)) - followers) + 1;
	else
		i = 0;

	// followers are always valid, only added, never removed
	if (i < numfollowers)
	{
		LUA_PushUserdata(L, &followers[i], META_FOLLOWER);
		return 1;
	}

	return 0;
}

static int lib_getFollower(lua_State *L)
{
	const char *field;
	INT32 i;

	// find follower by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= MAXFOLLOWERS)
			return luaL_error(L, "followers[] index %d out of range (0 - %d)", i, MAXFOLLOWERS-1);
		if (i >= numfollowers)
			return 0;
		LUA_PushUserdata(L, &followers[i], META_FOLLOWER);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateFollowers);
		return 1;
	}

	// find follower by name
	i = K_FollowerAvailable(field);
	if (i != -1)
	{
		LUA_PushUserdata(L, &followers[i], META_FOLLOWER);
		return 1;
	}

	return 0;
}

static int lib_numFollowers(lua_State *L)
{
	lua_pushinteger(L, numfollowers);
	return 1;
}

int LUA_FollowerLib(lua_State *L)
{
	luaL_newmetatable(L, META_FOLLOWER);
		lua_pushcfunction(L, follower_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, follower_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, follower_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getFollower);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numFollowers);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "followers");

	return 0;
}
