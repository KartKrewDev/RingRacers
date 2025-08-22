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
/// \file  lua_terrainlib.c
/// \brief terrain structure library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "k_terrain.h"

enum terrain {
	terrain_valid = 0,
	terrain_name,
	terrain_hash,
	terrain_splashid,
	terrain_footstepid,
	terrain_overlayid,
	terrain_friction,
	terrain_offroad,
	terrain_damagetype,
	terrain_trickpanel,
	terrain_speedpad,
	terrain_speedpadangle,
	terrain_springstrength,
	terrain_springstarcolor,
	terrain_outrun,
	terrain_floorclip,
	terrain_flags
};

enum splash {
	splash_valid = 0,
	splash_name,
	splash_hash,
	splash_mobjtype,
	splash_sfx,
	splash_scale,
	splash_color,
	splash_pushH,
	splash_pushV,
	splash_spread,
	splash_cone,
	splash_numparticles
};

enum footstep {
	footstep_valid = 0,
	footstep_name,
	footstep_hash,
	footstep_mobjtype,
	footstep_sfx,
	footstep_scale,
	footstep_color,
	footstep_pushH,
	footstep_pushV,
	footstep_spread,
	footstep_cone,
	footstep_sfxfreq,
	footstep_frequency,
	footstep_requiredspeed
};

enum overlay {
	overlay_valid = 0,
	overlay_name,
	overlay_hash,
	overlay_states,
	overlay_scale,
	overlay_color,
	overlay_speed
};

static const char *const terrain_opt[] = {
	"valid",
	"name",
	"hash",
	"splashid",
	"footstepid",
	"overlayid",
	"friction",
	"offroad",
	"damagetype",
	"trickpanel",
	"speedpad",
	"speedpadangle",
	"springstrength",
	"springstarcolor",
	"outrun",
	"floorclip",
	"flags",
	NULL
};

static const char *const splash_opt[] = {
	"valid",
	"name",
	"hash",
	"mobjtype",
	"sfx",
	"scale",
	"color",
	"pushh",
	"pushv",
	"spread",
	"cone",
	"numparticles",
	NULL
};

static const char *const footstep_opt[] = {
	"valid",
	"name",
	"hash",
	"mobjtype",
	"sfx",
	"scale",
	"color",
	"pushh",
	"pushv",
	"spread",
	"cone",
	"sfxfreq",
	"frequency",
	"requiredspeed",
	NULL
};

static const char *const overlay_opt[] = {
	"valid",
	"name",
	"hash",
	"states",
	"scale",
	"color",
	"speed",
	NULL
};

static int splash_get(lua_State *L)
{
	t_splash_t *splash = *((t_splash_t **)luaL_checkudata(L, 1, META_SPLASH));
	enum splash field = luaL_checkoption(L, 2, splash_opt[0], splash_opt);
	
	if (!splash)
	{
		switch (field)
		{
			case splash_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "t_splash_t");
		}
	}
	
	switch (field)
	{
	case splash_valid:
		lua_pushboolean(L, true);
		break;
	case splash_name:
		lua_pushstring(L, splash->name);
		break;
	case splash_hash:
		lua_pushnumber(L, splash->hash);
		break;
	case splash_mobjtype:
		lua_pushnumber(L, splash->mobjType);
		break;
	case splash_sfx:
		lua_pushnumber(L, splash->sfx);
		break;
	case splash_scale:
		lua_pushfixed(L, splash->scale);
		break;
	case splash_color:
		lua_pushnumber(L, splash->color);
		break;
	case splash_pushH:
		lua_pushfixed(L, splash->pushH);
		break;
	case splash_pushV:
		lua_pushfixed(L, splash->pushV);
		break;
	case splash_spread:
		lua_pushfixed(L, splash->spread);
		break;
	case splash_cone:
		lua_pushangle(L, splash->cone);
		break;
	case splash_numparticles:
		lua_pushnumber(L, splash->numParticles);
		break;
	}
	
	return 1;
}

static int splash_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("t_splash_t") " struct cannot be edited by Lua.");
}

static int splash_num(lua_State *L)
{
	t_splash_t *splash = *((t_splash_t **)luaL_checkudata(L, 1, META_SPLASH));
	
	// This should never happen.
	I_Assert(splash != NULL);
	
	lua_pushinteger(L, K_GetSplashHeapIndex(splash));
	return 1;
}

static int lib_iterateSplashes(lua_State *L)
{
	size_t i;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateSplashes);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = K_GetSplashHeapIndex(*(t_splash_t **)luaL_checkudata(L, 1, META_SPLASH)) + 1;
	else
		i = 0;

	// terrains are always valid, only added, never removed
	if (i < K_GetNumSplashDefs())
	{
		LUA_PushUserdata(L, K_GetSplashByIndex(i), META_SPLASH);
		return 1;
	}

	return 0;
}

static int lib_getSplash(lua_State *L)
{
	const char *field;
	size_t i;

	// find terrain by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i >= K_GetNumSplashDefs())
			return luaL_error(L, "splashes[] index %d out of range (0 - %d)", i, K_GetNumSplashDefs()-1);
		LUA_PushUserdata(L, K_GetSplashByIndex(i), META_SPLASH);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateSplashes);
		return 1;
	}

	// find terrain by name
	t_splash_t *byname = K_GetSplashByName(field);
	if (byname != NULL)
	{
		LUA_PushUserdata(L, byname, META_SPLASH);
		return 1;
	}

	return 0;
}

static int lib_numSplashes(lua_State *L)
{
	lua_pushinteger(L, K_GetNumSplashDefs());
	return 1;
}

static int footstep_get(lua_State *L)
{
	t_footstep_t *footstep = *((t_footstep_t **)luaL_checkudata(L, 1, META_FOOTSTEP));
	enum footstep field = luaL_checkoption(L, 2, footstep_opt[0], footstep_opt);
	
	if (!footstep)
	{
		switch (field)
		{
			case footstep_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "t_footstep_t");
		}
	}
	
	switch (field)
	{
	case footstep_valid:
		lua_pushboolean(L, true);
		break;
	case footstep_name:
		lua_pushstring(L, footstep->name);
		break;
	case footstep_hash:
		lua_pushnumber(L, footstep->hash);
		break;
	case footstep_mobjtype:
		lua_pushnumber(L, footstep->mobjType);
		break;
	case footstep_sfx:
		lua_pushnumber(L, footstep->sfx);
		break;
	case footstep_scale:
		lua_pushfixed(L, footstep->scale);
		break;
	case footstep_color:
		lua_pushnumber(L, footstep->color);
		break;
	case footstep_pushH:
		lua_pushfixed(L, footstep->pushH);
		break;
	case footstep_pushV:
		lua_pushfixed(L, footstep->pushV);
		break;
	case footstep_spread:
		lua_pushfixed(L, footstep->spread);
		break;
	case footstep_cone:
		lua_pushangle(L, footstep->cone);
		break;
	case footstep_sfxfreq:
		lua_pushnumber(L, footstep->sfxFreq);
		break;
	case footstep_frequency:
		lua_pushnumber(L, footstep->frequency);
		break;
	case footstep_requiredspeed:
		lua_pushfixed(L, footstep->requiredSpeed);
		break;
	}
	
	return 1;
}

static int footstep_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("t_footstep_t") " struct cannot be edited by Lua.");
}

static int footstep_num(lua_State *L)
{
	t_footstep_t *footstep = *((t_footstep_t **)luaL_checkudata(L, 1, META_FOOTSTEP));
	
	// This should never happen.
	I_Assert(footstep != NULL);
	
	lua_pushinteger(L, K_GetFootstepHeapIndex(footstep));
	return 1;
}

static int lib_iterateFootsteps(lua_State *L)
{
	size_t i;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateFootsteps);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = K_GetFootstepHeapIndex(*(t_footstep_t **)luaL_checkudata(L, 1, META_FOOTSTEP)) + 1;
	else
		i = 0;

	// footsteps are always valid, only added, never removed
	if (i < K_GetNumFootstepDefs())
	{
		LUA_PushUserdata(L, K_GetFootstepByIndex(i), META_FOOTSTEP);
		return 1;
	}

	return 0;
}

static int lib_getFootstep(lua_State *L)
{
	const char *field;
	size_t i;

	// find footstep by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i >= K_GetNumFootstepDefs())
			return luaL_error(L, "footsteps[] index %d out of range (0 - %d)", i, K_GetNumFootstepDefs()-1);
		LUA_PushUserdata(L, K_GetFootstepByIndex(i), META_FOOTSTEP);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateFootsteps);
		return 1;
	}

	// find footstep by name
	t_footstep_t *byname = K_GetFootstepByName(field);
	if (byname != NULL)
	{
		LUA_PushUserdata(L, byname, META_FOOTSTEP);
		return 1;
	}

	return 0;
}

static int lib_numFootsteps(lua_State *L)
{
	lua_pushinteger(L, K_GetNumFootstepDefs());
	return 1;
}

static int overlay_get(lua_State *L)
{
	t_overlay_t *overlay = *((t_overlay_t **)luaL_checkudata(L, 1, META_OVERLAY));
	enum overlay field = luaL_checkoption(L, 2, overlay_opt[0], overlay_opt);
	
	if (!overlay)
	{
		switch (field)
		{
			case overlay_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "t_overlay_t");
		}
	}
	
	switch (field)
	{
	case overlay_valid:
		lua_pushboolean(L, true);
		break;
	case overlay_name:
		lua_pushstring(L, overlay->name);
		break;
	case overlay_hash:
		lua_pushnumber(L, overlay->hash);
		break;
	case overlay_states:
		lua_createtable(L, TOV__MAX, 0);
		for (size_t i = 0; i < TOV__MAX; i++)
		{
			lua_pushinteger(L, overlay->states[i]);
			lua_rawseti(L, -2, 1 + i);
		}
		break;
	case overlay_scale:
		lua_pushboolean(L, overlay->scale);
		break;
	case overlay_color:
		lua_pushnumber(L, overlay->color);
		break;
	case overlay_speed:
		lua_pushfixed(L, overlay->speed);
		break;
	}

	return 1;
}

static int overlay_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("t_overlay_t") " struct cannot be edited by Lua.");
}

static int overlay_num(lua_State *L)
{
	t_overlay_t *overlay = *((t_overlay_t **)luaL_checkudata(L, 1, META_OVERLAY));
	
	// This should never happen.
	I_Assert(overlay != NULL);
	
	lua_pushinteger(L, K_GetOverlayHeapIndex(overlay));
	return 1;
}

static int lib_iterateOverlays(lua_State *L)
{
	size_t i;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateOverlays);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = K_GetOverlayHeapIndex(*(t_overlay_t **)luaL_checkudata(L, 1, META_OVERLAY)) + 1;
	else
		i = 0;

	// overlays are always valid, only added, never removed
	if (i < K_GetNumOverlayDefs())
	{
		LUA_PushUserdata(L, K_GetOverlayByIndex(i), META_OVERLAY);
		return 1;
	}

	return 0;
}

static int lib_getOverlay(lua_State *L)
{
	const char *field;
	size_t i;

	// find overlay by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		// Making a special condition for this as the game contains no overlays by default.
		if (K_GetNumOverlayDefs() == 0)
			return luaL_error(L, "no overlays available in overlays[]");
		
		i = luaL_checkinteger(L, 2);
		if (i >= K_GetNumOverlayDefs())
			return luaL_error(L, "overlays[] index %d out of range (0 - %d)", i, K_GetNumOverlayDefs()-1);
		LUA_PushUserdata(L, K_GetOverlayByIndex(i), META_OVERLAY);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateOverlays);
		return 1;
	}

	// find overlay by name
	t_overlay_t *byname = K_GetOverlayByName(field);
	if (byname != NULL)
	{
		LUA_PushUserdata(L, byname, META_OVERLAY);
		return 1;
	}

	return 0;
}

static int lib_numOverlays(lua_State *L)
{
	lua_pushinteger(L, K_GetNumOverlayDefs());
	return 1;
}

static int terrain_get(lua_State *L)
{
	terrain_t *terrain = *((terrain_t **)luaL_checkudata(L, 1, META_TERRAIN));
	enum terrain field = luaL_checkoption(L, 2, terrain_opt[0], terrain_opt);
	
	if (!terrain)
	{
		switch (field)
		{
			case terrain_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "terrain_t");
		}
	}
	
	switch (field)
	{
	case terrain_valid:
		lua_pushboolean(L, true);
		break;
	case terrain_name:
		lua_pushstring(L, terrain->name);
		break;
	case terrain_hash:
		lua_pushnumber(L, terrain->hash);
		break;
	case terrain_splashid:
		lua_pushnumber(L, terrain->splashID);
		break;
	case terrain_footstepid:
		lua_pushnumber(L, terrain->footstepID);
		break;
	case terrain_overlayid:
		lua_pushnumber(L, terrain->overlayID);
		break;
	case terrain_friction:
		lua_pushfixed(L, terrain->friction);
		break;
	case terrain_offroad:
		lua_pushfixed(L, terrain->offroad);
		break;
	case terrain_damagetype:
		lua_pushnumber(L, terrain->damageType);
		break;
	case terrain_trickpanel:
		lua_pushfixed(L, terrain->trickPanel);
		break;
	case terrain_speedpad:
		lua_pushfixed(L, terrain->speedPad);
		break;
	case terrain_speedpadangle:
		lua_pushangle(L, terrain->speedPadAngle);
		break;
	case terrain_springstrength:
		lua_pushfixed(L, terrain->springStrength);
		break;
	case terrain_springstarcolor:
		lua_pushnumber(L, terrain->springStarColor);
		break;
	case terrain_outrun:
		lua_pushfixed(L, terrain->outrun);
		break;
	case terrain_floorclip:
		lua_pushfixed(L, terrain->floorClip);
		break;
	case terrain_flags:
		lua_pushnumber(L, terrain->flags);
		break;
	}

	return 1;
}

static int terrain_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("terrain_t") " struct cannot be edited by Lua.");
}

static int terrain_num(lua_State *L)
{
	terrain_t *terrain = *((terrain_t **)luaL_checkudata(L, 1, META_TERRAIN));
	
	// This should never happen.
	I_Assert(terrain != NULL);
	
	lua_pushinteger(L, K_GetTerrainHeapIndex(terrain));
	return 1;
}

static int lib_iterateTerrains(lua_State *L)
{
	size_t i;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateTerrains);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
		i = K_GetTerrainHeapIndex(*(terrain_t **)luaL_checkudata(L, 1, META_TERRAIN)) + 1;
	else
		i = 0;

	// terrains are always valid, only added, never removed
	if (i < K_GetNumTerrainDefs())
	{
		LUA_PushUserdata(L, K_GetTerrainByIndex(i), META_TERRAIN);
		return 1;
	}

	return 0;
}

static int lib_getTerrain(lua_State *L)
{
	const char *field;
	size_t i;

	// find terrain by number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i >= K_GetNumTerrainDefs())
			return luaL_error(L, "terrains[] index %d out of range (0 - %d)", i, K_GetNumTerrainDefs()-1);
		LUA_PushUserdata(L, K_GetTerrainByIndex(i), META_TERRAIN);
		return 1;
	}

	field = luaL_checkstring(L, 2);

	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateTerrains);
		return 1;
	}

	// find terrain by name
	terrain_t *byname = K_GetTerrainByName(field);
	if (byname != NULL)
	{
		LUA_PushUserdata(L, byname, META_TERRAIN);
		return 1;
	}

	return 0;
}

static int lib_numTerrains(lua_State *L)
{
	lua_pushinteger(L, K_GetNumTerrainDefs());
	return 1;
}

int LUA_TerrainLib(lua_State *L)
{	
	luaL_newmetatable(L, META_SPLASH);
		lua_pushcfunction(L, splash_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, splash_set);
		lua_setfield(L, -2, "__newindex");
		
		lua_pushcfunction(L, splash_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);
	
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSplash);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numSplashes);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "splashes");
	
	luaL_newmetatable(L, META_FOOTSTEP);
		lua_pushcfunction(L, footstep_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, footstep_set);
		lua_setfield(L, -2, "__newindex");
		
		lua_pushcfunction(L, footstep_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getFootstep);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numFootsteps);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "footsteps");
	
	luaL_newmetatable(L, META_OVERLAY);
		lua_pushcfunction(L, overlay_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, overlay_set);
		lua_setfield(L, -2, "__newindex");
		
		lua_pushcfunction(L, overlay_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);
	
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getOverlay);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numOverlays);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "overlays");

	luaL_newmetatable(L, META_TERRAIN);
		lua_pushcfunction(L, terrain_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, terrain_set);
		lua_setfield(L, -2, "__newindex");
		
		lua_pushcfunction(L, terrain_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);
	
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getTerrain);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numTerrains);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "terrains");

	return 0;
}