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
/// \file  lua_waypointslib.c
/// \brief wapoint structure library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"

#include "lua_script.h"
#include "lua_libs.h"

enum waypointvars {
	waypointvars_valid = 0,
	waypointvars_mobj,
	waypointvars_x,
	waypointvars_y,
	waypointvars_z,
	waypointvars_onaline,
	waypointvars_nextwaypoints,
	waypointvars_prevwaypoints,
	waypointvars_nextwaypointdistances,
	waypointvars_prevwaypointdistances,
	waypointvars_numnextwaypoints,
	waypointvars_numprevwaypoints,
};

static const char *const waypointvars_opt[] = {
	"valid",
	"mobj",
	"x",
	"y",
	"z",
	"onaline",
	"nextwaypoints",
	"prevwaypoints",
	"nextwaypointdistances",
	"prevwaypointdistances",
	"numnextwaypoints",
	"numprevwaypoints",
	NULL
};

#define RNOFIELD luaL_error(L, LUA_QL("waypoint_t") " has no field named " LUA_QS, field)
#define RNOSET luaL_error(L, LUA_QL("waypoint_t") " field " LUA_QS " cannot be set.", field)
#define RNOGET luaL_error(L, LUA_QL("waypoint_t") " field " LUA_QS " cannot be get.", field)

static int waypoint_get(lua_State *L)
{
	waypoint_t *waypoint = *((waypoint_t **)luaL_checkudata(L, 1, META_WAYPOINT));
	enum waypointvars field = luaL_checkoption(L, 2, NULL, waypointvars_opt);
	
	if (!waypoint)
	{
		switch (field)
		{
			case waypointvars_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "waypoint_t");
		}
	}
	
	// It could totally happen that some scripter just deletes the mobj,
	// which would have us check a null pointer, so we're checking against that
	// just in case.
	mobj_t *waypointMobj = NULL;
	if (waypoint->mobj != NULL && P_MobjWasRemoved(waypoint->mobj) == false)
		waypointMobj = waypoint->mobj;
		
	switch (field)
	{
		case waypointvars_valid:
			lua_pushboolean(L, true);
			break;
		case waypointvars_mobj:
			return RNOGET;
		case waypointvars_x:
			if (waypointMobj)
				lua_pushfixed(L, waypointMobj->x);
			else
				lua_pushnil(L);
			break;
		case waypointvars_y:
			if (waypointMobj)
				lua_pushinteger(L, waypointMobj->y);
			else
				lua_pushnil(L);
			break;
		case waypointvars_z:
			if (waypointMobj)
				lua_pushinteger(L, waypointMobj->z);
			else
				lua_pushnil(L);
			break;
		case waypointvars_onaline:
			lua_pushboolean(L, waypoint->onaline);
			break;
		case waypointvars_nextwaypoints:
			lua_createtable(L, waypoint->numnextwaypoints, 0);
			for (size_t i = 0; i < waypoint->numnextwaypoints; i++)
			{
				LUA_PushUserdata(L, waypoint->nextwaypoints[i], META_WAYPOINT);
				lua_rawseti(L, -2, 1 + i);
			}
			break;
		case waypointvars_prevwaypoints:
			lua_createtable(L, waypoint->numprevwaypoints, 0);
			for (size_t i = 0; i < waypoint->numprevwaypoints; i++)
			{
				LUA_PushUserdata(L, waypoint->prevwaypoints[i], META_WAYPOINT);
				lua_rawseti(L, -2, 1 + i);
			}
			break;
		case waypointvars_nextwaypointdistances:
			lua_createtable(L, waypoint->numnextwaypoints, 0);
			for (size_t i = 0; i < waypoint->numnextwaypoints; i++)
			{
				lua_pushinteger(L, waypoint->nextwaypointdistances[i]);
				lua_rawseti(L, -2, 1 + i);
			}
			break;
		case waypointvars_prevwaypointdistances:
			lua_createtable(L, waypoint->numprevwaypoints, 0);
			for (size_t i = 0; i < waypoint->numprevwaypoints; i++)
			{
				lua_pushinteger(L, waypoint->prevwaypointdistances[i]);
				lua_rawseti(L, -2, 1 + i);
			}
			break;
		case waypointvars_numnextwaypoints:
			lua_pushinteger(L, waypoint->numnextwaypoints);
			break;
		case waypointvars_numprevwaypoints:
			lua_pushinteger(L, waypoint->numprevwaypoints);
			break;
		default:
			return RNOFIELD;
	}

	return 1;
}

static int waypoint_set(lua_State *L)
{
	waypoint_t *waypoint = *((waypoint_t **)luaL_checkudata(L, 1, META_WAYPOINT));
	enum waypointvars field = luaL_checkoption(L, 2, waypointvars_opt[0], waypointvars_opt);

	if (!waypoint)
		return LUA_ErrInvalid(L, "waypoint_t");

	INLEVEL
	
	switch (field)
	{
		case waypointvars_mobj:
			return RNOSET;
		case waypointvars_x:
			return RNOSET;
		case waypointvars_y:
			return RNOSET;
		case waypointvars_z:
			return RNOSET;
		case waypointvars_onaline:
			return RNOSET;
		// A function should be used to set these instead.
		case waypointvars_nextwaypoints:
			return RNOSET;
		case waypointvars_prevwaypoints:
			return RNOSET;
		case waypointvars_nextwaypointdistances:
			return RNOSET;
		case waypointvars_prevwaypointdistances:
			return RNOSET;
		case waypointvars_numnextwaypoints:
			return RNOSET;
		case waypointvars_numprevwaypoints:
			return RNOSET;
		default:
			return RNOFIELD;
	}

	return 0;
}

#undef RNOSET
#undef RNOFIELD

static int waypoint_num(lua_State *L)
{
	waypoint_t *waypoint = *((waypoint_t **)luaL_checkudata(L, 1, META_WAYPOINT));
	
	if (!waypoint)
		return LUA_ErrInvalid(L, "waypoint_t");
	
	lua_pushinteger(L, K_GetWaypointHeapIndex(waypoint));
	return 1;
}

static int lib_iterateWaypoints(lua_State *L)
{
	size_t i;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateWaypoints);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = K_GetWaypointHeapIndex(*(waypoint_t **)luaL_checkudata(L, 1, META_WAYPOINT)) + 1;
	else
		i = 0;

	if (i < K_GetNumWaypoints())
	{
		LUA_PushUserdata(L, K_GetWaypointFromIndex(i), META_WAYPOINT);
		return 1;
	}

	return 0;
}

static int lib_getWaypoint(lua_State *L)
{
	const char *field;
	size_t i;

	// find waypoint by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i >= K_GetNumWaypoints())
			return luaL_error(L, "waypoints[] index %d out of range (0 - %d)", i, K_GetNumWaypoints()-1);
		LUA_PushUserdata(L, K_GetWaypointFromIndex(i), META_WAYPOINT);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateWaypoints);
		return 1;
	}

	return 0;
}

static int lib_numWaypoints(lua_State *L)
{
	lua_pushinteger(L, K_GetNumWaypoints());
	return 1;
}


int LUA_WaypointLib(lua_State *L)
{
	luaL_newmetatable(L, META_WAYPOINT);
		lua_pushcfunction(L, waypoint_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, waypoint_set);
		lua_setfield(L, -2, "__newindex");
		
		lua_pushcfunction(L, waypoint_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);
	
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getWaypoint);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numWaypoints);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "waypoints");

	return 0;
}
