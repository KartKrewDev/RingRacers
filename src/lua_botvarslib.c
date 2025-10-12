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
/// \file  lua_botvarslib.c
/// \brief player botvars structure library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"

#include "lua_script.h"
#include "lua_libs.h"

enum botvars {
	botvars_valid = 0,
	botvars_style,
	botvars_difficulty,
	botvars_diffincrease,
	botvars_rival,
	botvars_foe,
	botvars_rubberband,
	botvars_bumpslow,
	botvars_itemdelay,
	botvars_itemconfirm,
	botvars_turnconfirm,
	botvars_spindashconfirm,
	botvars_respawnconfirm,
	botvars_roulettepriority,
	botvars_roulettetimeout,
	botvars_predictionerror,
	botvars_recentdeflection,
	botvars_lastangle
};

static const char *const botvars_opt[] = {
	"valid",
	"style",
	"difficulty",
	"diffincrease",
	"rival",
	"foe",
	"rubberband",
	"bumpslow",
	"itemdelay",
	"itemconfirm",
	"turnconfirm",
	"spindashconfirm",
	"respawnconfirm",
	"roulettepriority",
	"roulettetimeout",
	"predictionerror",
	"recentdeflection",
	"lastangle",
	NULL
};

#define UNIMPLEMENTED luaL_error(L, LUA_QL("botvars_t") " field " LUA_QS " is not implemented for Lua and cannot be accessed.", follower_opt[field])

static int botvars_get(lua_State *L)
{
	botvars_t *botvars = *((botvars_t **)luaL_checkudata(L, 1, META_BOTVARS));
	enum botvars field = luaL_checkoption(L, 2, NULL, botvars_opt);

	// This is a property that always exists in a player.
	I_Assert(botvars != NULL);

	switch (field)
	{
	case botvars_valid:
		lua_pushboolean(L, botvars != NULL);
		break;
	case botvars_style:
		lua_pushinteger(L, botvars->style);
		break;
	case botvars_difficulty:
		lua_pushinteger(L, botvars->difficulty);
		break;
	case botvars_diffincrease:
		lua_pushinteger(L, botvars->diffincrease);
		break;
	case botvars_rival:
		lua_pushboolean(L, botvars->rival);
		break;
	case botvars_foe:
		lua_pushboolean(L, botvars->foe);
		break;
	case botvars_rubberband:
		lua_pushfixed(L, botvars->rubberband);
		break;
	case botvars_bumpslow:
		lua_pushinteger(L, botvars->bumpslow);
		break;
	case botvars_itemdelay:
		lua_pushinteger(L, botvars->itemdelay);
		break;
	case botvars_itemconfirm:
		lua_pushinteger(L, botvars->itemconfirm);
		break;
	case botvars_turnconfirm:
		lua_pushinteger(L, botvars->turnconfirm);
		break;
	case botvars_spindashconfirm:
		lua_pushinteger(L, botvars->spindashconfirm);
		break;
	case botvars_respawnconfirm:
		lua_pushinteger(L, botvars->respawnconfirm);
		break;
	case botvars_roulettepriority:
		lua_pushinteger(L, botvars->roulettePriority);
		break;
	case botvars_roulettetimeout:
		lua_pushinteger(L, botvars->rouletteTimeout);
		break;
	case botvars_predictionerror:
		lua_pushinteger(L, botvars->predictionError);
		break;
	case botvars_recentdeflection:
		lua_pushinteger(L, botvars->recentDeflection);
		break;
	case botvars_lastangle:
		lua_pushinteger(L, botvars->lastAngle);
		break;
	}
	return 1;
}

#define NOSET luaL_error(L, LUA_QL("itemroulette_t") " field " LUA_QS " should not be set directly.", botvars_opt[field])

static int botvars_set(lua_State *L)
{
	botvars_t *botvars = *((botvars_t **)luaL_checkudata(L, 1, META_BOTVARS));
	enum botvars field = luaL_checkoption(L, 2, botvars_opt[0], botvars_opt);

	// This is a property that always exists in a player.
	I_Assert(botvars != NULL);

	INLEVEL

	switch(field)
	{
	case botvars_valid:
		return NOSET;
	case botvars_style:
		botvars->style = luaL_checkinteger(L, 3);
		break;
	case botvars_difficulty:
		botvars->difficulty = luaL_checkinteger(L, 3);
		break;
	case botvars_diffincrease:
		botvars->diffincrease = luaL_checkinteger(L, 3);
		break;
	case botvars_rival:
		botvars->rival = luaL_checkboolean(L, 3);
		break;
	case botvars_foe:
		botvars->foe = luaL_checkboolean(L, 3);
		break;
	case botvars_rubberband:
		botvars->rubberband = luaL_checkfixed(L, 3);
		break;
	case botvars_bumpslow:
		botvars->bumpslow = luaL_checkinteger(L, 3);
		break;
	case botvars_itemdelay:
		botvars->itemdelay = luaL_checkinteger(L, 3);
		break;
	case botvars_itemconfirm:
		botvars->itemconfirm = luaL_checkinteger(L, 3);
		break;
	case botvars_turnconfirm:
		botvars->turnconfirm = luaL_checkinteger(L, 3);
		break;
	case botvars_spindashconfirm:
		botvars->spindashconfirm = luaL_checkinteger(L, 3);
		break;
	case botvars_respawnconfirm:
		botvars->respawnconfirm = luaL_checkinteger(L, 3);
		break;
	case botvars_roulettepriority:
		botvars->roulettePriority = luaL_checkinteger(L, 3);
		break;
	case botvars_roulettetimeout:
		botvars->rouletteTimeout = luaL_checkinteger(L, 3);
		break;
	case botvars_predictionerror:
		botvars->predictionError = luaL_checkangle(L, 3);
		break;
	case botvars_recentdeflection:
		botvars->recentDeflection = luaL_checkangle(L, 3);
		break;
	case botvars_lastangle:
		botvars->lastAngle = luaL_checkangle(L, 3);
		break;
	}
	return 0;
}

#undef NOSET

int LUA_BotVarsLib(lua_State *L)
{
	luaL_newmetatable(L, META_BOTVARS);
		lua_pushcfunction(L, botvars_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, botvars_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	return 0;
}
