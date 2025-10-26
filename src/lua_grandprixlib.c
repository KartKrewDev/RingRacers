// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Freaky Mutant Man.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by John "JTE" Muniz.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_grandprixlib.c
/// \brief Grand Prix, cup and rank info for Lua scripting.

#include "doomdef.h"
#include "fastcmp.h"
#include "doomstat.h"
#include "k_grandprix.h"
#include "k_rank.h"
#include "g_game.h"

#include "lua_script.h"
#include "lua_libs.h"

#define UNIMPLEMENTED luaL_error(L, LUA_QL("cupheader_t") " field " LUA_QS " is not implemented for Lua and cannot be accessed.", cup_opt[field])
#define RNOFIELDGP luaL_error(L, LUA_QL("grandprixinfo") " has no field named " LUA_QS, field)
#define RNOFIELDCH luaL_error(L, LUA_QL("cupheader_t") " has no field named " LUA_QS, field)
#define RNOFIELDGR luaL_error(L, LUA_QL("gprank_t") " has no field named " LUA_QS, field)
#define RNOFIELDGRL luaL_error(L, LUA_QL("gprank_level_t") " has no field named " LUA_QS, field)
#define RNOFIELDGRLP luaL_error(L, LUA_QL("gprank_level_perplayer_t") " has no field named " LUA_QS, field)
#define RNOFIELDRQ luaL_error(L, LUA_QL("roundqueue") " has no field named " LUA_QS, field)
#define RNOFIELDRE luaL_error(L, LUA_QL("roundentry_t") " has no field named " LUA_QS, field)
#define GPERR luaL_error(L, LUA_QL("grandprixinfo") " field " LUA_QS " cannot be accessed while grandprixinfo.gp is false.", grandprix_opt[field])
#define ROUNDCUEERR luaL_error(L, LUA_QL("roundqueue") " field " LUA_QS " cannot be accessed while roundqueue.size is 0.", grandprix_opt[field])

enum grandprix {
	grandprix_gp = 0,
	grandprix_cup,
	grandprix_gamespeed,
	grandprix_encore,
	grandprix_masterbots,
	grandprix_initialize,
	grandprix_initalize,
	grandprix_wonround,
	grandprix_eventmode,
	grandprix_specialdamage,
	grandprix_rank,
};

enum cup {
	cup_valid = 0,
	cup_id,
	cup_monitor,
	cup_name,
	cup_namehash,
	cup_realname,
	cup_icon,
	cup_levellist,
	cup_cachedlevels,
	cup_numlevels,
	cup_numbonus,
	cup_emeraldnum,
	cup_playcredits,
	cup_hintcondition,
	cup_cache_cuplock,
	cup_windata,
	cup_next,
};

enum gprank {
	gprank_valid = 0,
	gprank_numplayers,
	gprank_totalplayers,
	gprank_position,
	gprank_skin,
	gprank_winpoints,
	gprank_totalpoints,
	gprank_exp,
	gprank_totalexp,
	gprank_continuesused,
	gprank_prisons,
	gprank_totalprisons,
	gprank_rings,
	gprank_totalrings,
	gprank_specialwon,
	gprank_scoreposition,
	gprank_scoregppoints,
	gprank_scoreexp,
	gprank_scoreprisons,
	gprank_scorerings,
	gprank_scorecontinues,
	gprank_scoretotal,
	gprank_numlevels,
	gprank_levels,
};

enum gprank_level {
	gprank_level_valid = 0,
	gprank_level_id,
	gprank_level_event,
	gprank_level_time,
	gprank_level_totalexp,
	gprank_level_totalprisons,
	gprank_level_continues,
	gprank_level_perplayer,
};

enum gprank_level_perplayer {
	gprank_level_perplayer_valid = 0,
	gprank_level_perplayer_position,
	gprank_level_perplayer_rings,
	gprank_level_perplayer_exp,
	gprank_level_perplayer_prisons,
	gprank_level_perplayer_gotspecialprize,
	gprank_level_perplayer_grade,
};

enum roundcue { // named "roundcue" to avoid overlap with actual roundqueue struct
	roundcue_size = 0, // placed first since we'll be checking this to see if the roundqueue is currently in use
	roundcue_roundnum,
	roundcue_position,
	roundcue_netcommunicate,
	roundcue_writetextmap,
	roundcue_snapshotmaps,
	roundcue_entries,
};

enum roundentry {
	roundentry_valid = 0,
	roundentry_mapnum,
	roundentry_gametype,
	roundentry_encore,
	roundentry_rankrestricted,
	roundentry_overridden,
};

static const char *const grandprix_opt[] = {
	"gp",
	"cup",
	"gamespeed",
	"encore",
	"masterbots",
	"initialize",
	"initalize",
	"wonround",
	"eventmode",
	"specialdamage",
	"rank",
	NULL
};

static const char *const cup_opt[] = {
	"valid",
	"id",
	"monitor",
	"name",
	"namehash",
	"realname",
	"icon",
	"levellist",
	"cachedlevels",
	"numlevels",
	"numbonus",
	"emeraldnum",
	"playcredits",
	"hintcondition",
	"cache_cuplock",
	"windata",
	"next",
	NULL
};

static const char *const gprank_opt[] = {
	"valid",
	"numplayers",
	"totalplayers",
	"position",
	"skin",
	"winpoints",
	"totalpoints",
	"exp",
	"totalexp",
	"continuesused",
	"prisons",
	"totalprisons",
	"rings",
	"totalrings",
	"specialwon",
	"scoreposition",
	"scoregppoints",
	"scoreexp",
	"scoreprisons",
	"scorerings",
	"scorecontinues",
	"scoretotal",
	"numlevels",
	"levels",
	NULL
};

static const char *const gprank_level_opt[] = {
	"valid",
	"id",
	"event",
	"time",
	"totalexp",
	"totalprisons",
	"continues",
	"perplayer",
	NULL
};

static const char *const gprank_level_perplayer_opt[] = {
	"valid",
	"position",
	"rings",
	"exp",
	"prisons",
	"gotspecialprize",
	"grade",
	NULL
};

static const char *const roundcue_opt[] = {
	"size",
	"roundnum",
	"position",
	"netcommunicate",
	"writetextmap",
	"snapshotmaps",
	"entries",
	NULL
};

static const char *const roundentry_opt[] = {
	"valid",
	"mapnum",
	"gametype",
	"encore",
	"rankrestricted",
	"overridden",
	NULL
};

static int grandprix_get(lua_State *L)
{
	enum grandprix field = luaL_checkoption(L, 2, grandprix_opt[0], grandprix_opt);
	
	// Don't return any grandprixinfo values while not in a GP.
	if (!grandprixinfo.gp)
	{
		switch (field)
		{
			case grandprix_gp:
				lua_pushboolean(L, false);
				return 1;
			default:
				return GPERR;
		}
	}
	
	switch (field)
	{
	case grandprix_gp:
		lua_pushboolean(L, grandprixinfo.gp);
		break;
	case grandprix_cup:
		LUA_PushUserdata(L, grandprixinfo.cup, META_CUP);
		break;
	case grandprix_gamespeed:
		lua_pushnumber(L, grandprixinfo.gamespeed);
		break;
	case grandprix_encore:
		lua_pushboolean(L, grandprixinfo.encore);
		break;
	case grandprix_masterbots:
		lua_pushboolean(L, grandprixinfo.masterbots);
		break;
	case grandprix_initialize:
	case grandprix_initalize: // when the struct misspelled the variable...
		lua_pushboolean(L, grandprixinfo.initalize);
		break;
	case grandprix_wonround:
		lua_pushboolean(L, grandprixinfo.wonround);
		break;
	case grandprix_eventmode:
		lua_pushnumber(L, grandprixinfo.eventmode);
		break;
	case grandprix_specialdamage:
		lua_pushnumber(L, grandprixinfo.specialDamage);
		break;
	case grandprix_rank:
		LUA_PushUserdata(L, &grandprixinfo.rank, META_GPRANK);
		break;
	default:
		return RNOFIELDGP;
	}
	
	return 1;
}

static int grandprix_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("grandprixinfo") " struct cannot be edited by Lua.");
}

static int cup_get(lua_State *L)
{
	cupheader_t *cup = *((cupheader_t **)luaL_checkudata(L, 1, META_CUP));
	enum cup field = luaL_checkoption(L, 2, cup_opt[0], cup_opt);
	
	if (!cup)
	{
		switch (field)
		{
			case cup_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "cupheader_t");
		}
	}
	
	switch (field)
	{
	case cup_valid:
		lua_pushboolean(L, true);
		break;
	case cup_id:
		lua_pushnumber(L, cup->id);
		break;
	case cup_monitor:
		lua_pushnumber(L, cup->monitor);
		break;
	case cup_name:
		lua_pushstring(L, cup->name);
		break;
	case cup_namehash:
		return UNIMPLEMENTED;
		break;
	case cup_realname:
		lua_pushstring(L, cup->realname);
		break;
	case cup_icon:
		lua_pushstring(L, cup->icon);
		break;
	case cup_levellist:
		lua_createtable(L, ((cup->numlevels) + (cup->numbonus)), 0);
		for (size_t i = 0; i < ((cup->numlevels) + (cup->numbonus)); i++)
		{
			lua_pushstring(L, cup->levellist[i]);
			lua_rawseti(L, -2, 1 + i);
		}
		break;
	case cup_cachedlevels:
		lua_createtable(L, ((cup->numlevels) + (cup->numbonus)), 0);
		for (size_t i = 0; i < CUPCACHE_MAX; i++)
		{
			lua_pushnumber(L, (cup->cachedlevels[i])+1);
			lua_rawseti(L, -2, 1 + i);
		}
		break;
	case cup_numlevels:
		lua_pushnumber(L, cup->numlevels);
		break;
	case cup_numbonus:
		lua_pushnumber(L, cup->numbonus);
		break;
	case cup_emeraldnum:
		lua_pushnumber(L, cup->emeraldnum);
		break;
	case cup_playcredits:
		lua_pushboolean(L, cup->playcredits);
		break;
	case cup_hintcondition:
		lua_pushnumber(L, cup->hintcondition);
		break;
	case cup_cache_cuplock:
		return UNIMPLEMENTED;
		break;
	case cup_windata:
		return UNIMPLEMENTED;
		break;
	case cup_next:
		return UNIMPLEMENTED;
		break;
	default:
		return RNOFIELDCH;
	}
	
	return 1;
}

static int cup_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("cupheader_t") " struct cannot be edited by Lua.");
}

static int gprank_get(lua_State *L)
{
	gpRank_t *gprank = *((gpRank_t **)luaL_checkudata(L, 1, META_GPRANK));
	enum gprank field = luaL_checkoption(L, 2, gprank_opt[0], gprank_opt);
	
	if (!gprank)
	{
		switch (field)
		{
			case gprank_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "gprank_t");
		}
	}
	
	switch (field)
	{
	case gprank_valid:
		lua_pushboolean(L, true);
		break;
	case gprank_numplayers:
		lua_pushnumber(L, gprank->numPlayers);
		break;
	case gprank_totalplayers:
		lua_pushnumber(L, gprank->totalPlayers);
		break;
	case gprank_position:
		lua_pushnumber(L, gprank->position);
		break;
	case gprank_skin:
		lua_pushnumber(L, gprank->skin);
		break;
	case gprank_winpoints:
		lua_pushnumber(L, gprank->winPoints);
		break;
	case gprank_totalpoints:
		lua_pushnumber(L, gprank->totalPoints);
		break;
	case gprank_exp:
		lua_pushnumber(L, gprank->exp);
		break;
	case gprank_totalexp:
		lua_pushnumber(L, gprank->totalExp);
		break;
	case gprank_continuesused:
		lua_pushnumber(L, gprank->continuesUsed);
		break;
	case gprank_prisons:
		lua_pushnumber(L, gprank->prisons);
		break;
	case gprank_totalprisons:
		lua_pushnumber(L, gprank->totalPrisons);
		break;
	case gprank_rings:
		lua_pushnumber(L, gprank->rings);
		break;
	case gprank_totalrings:
		lua_pushnumber(L, gprank->totalRings);
		break;
	case gprank_specialwon:
		lua_pushboolean(L, gprank->specialWon);
		break;
	case gprank_scoreposition:
		lua_pushnumber(L, gprank->scorePosition);
		break;
	case gprank_scoregppoints:
		lua_pushnumber(L, gprank->scoreGPPoints);
		break;
	case gprank_scoreexp:
		lua_pushnumber(L, gprank->scoreExp);
		break;
	case gprank_scoreprisons:
		lua_pushnumber(L, gprank->scorePrisons);
		break;
	case gprank_scorerings:
		lua_pushnumber(L, gprank->scoreRings);
		break;
	case gprank_scorecontinues:
		lua_pushnumber(L, gprank->scoreContinues);
		break;
	case gprank_scoretotal:
		lua_pushnumber(L, gprank->scoreTotal);
		break;
	case gprank_numlevels:
		lua_pushnumber(L, gprank->numLevels);
		break;
	case gprank_levels:
		lua_createtable(L, ((grandprixinfo.cup->numlevels) + (grandprixinfo.cup->numbonus)), 0);
		for (size_t i = 0; i < ((grandprixinfo.cup->numlevels) + (grandprixinfo.cup->numbonus)); i++)
		{
			LUA_PushUserdata(L, &gprank->levels[i], META_GPRANKLEVEL);
			lua_rawseti(L, -2, 1 + i);
		}
		break;
	default:
		return RNOFIELDGR;
	}
	
	return 1;
}

static int gprank_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("gprank_t") " struct cannot be edited by Lua.");
}

static int gprank_level_get(lua_State *L)
{
	gpRank_level_t *gprank_level = *((gpRank_level_t **)luaL_checkudata(L, 1, META_GPRANKLEVEL));
	enum gprank_level field = luaL_checkoption(L, 2, gprank_level_opt[0], gprank_level_opt);
	
	if (!gprank_level)
	{
		switch (field)
		{
			case gprank_level_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "gprank_level_t");
		}
	}
	
	switch (field)
	{
	case gprank_level_valid:
		lua_pushboolean(L, true);
		break;
	case gprank_level_id:
		lua_pushnumber(L, gprank_level->id);
		break;
	case gprank_level_event:
		lua_pushnumber(L, gprank_level->event);
		break;
	case gprank_level_time:
		lua_pushnumber(L, gprank_level->time);
		break;
	case gprank_level_totalexp:
		lua_pushnumber(L, gprank_level->totalExp);
		break;
	case gprank_level_totalprisons:
		lua_pushnumber(L, gprank_level->totalPrisons);
		break;
	case gprank_level_continues:
		lua_pushnumber(L, gprank_level->continues);
		break;
	case gprank_level_perplayer:
		lua_createtable(L, grandprixinfo.rank.numPlayers, 0);
		for (size_t i = 0; i < grandprixinfo.rank.numPlayers; i++)
		{
			LUA_PushUserdata(L, &gprank_level->perPlayer[i], META_GPRANKLEVELPERPLAYER);
			lua_rawseti(L, -2, 1 + i);
		}
		break;
	default:
		return RNOFIELDGRL;
	}
	
	return 1;
}

static int gprank_level_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("gprank_level_t") " struct cannot be edited by Lua.");
}

static int gprank_level_perplayer_get(lua_State *L)
{
	// "perplaya" to avoid shadowed declaration
	gpRank_level_perplayer_t *gprank_level_perplaya = *((gpRank_level_perplayer_t **)luaL_checkudata(L, 1, META_GPRANKLEVELPERPLAYER));
	enum gprank_level_perplayer field = luaL_checkoption(L, 2, gprank_level_perplayer_opt[0], gprank_level_perplayer_opt);
	
	if (!gprank_level_perplaya)
	{
		switch (field)
		{
			case gprank_level_perplayer_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "gprank_level_perplayer_t");
		}
	}
	
	switch (field)
	{
	case gprank_level_perplayer_valid:
		lua_pushboolean(L, true);
		break;
	case gprank_level_perplayer_position:
		lua_pushnumber(L, gprank_level_perplaya->position);
		break;
	case gprank_level_perplayer_rings:
		lua_pushnumber(L, gprank_level_perplaya->rings);
		break;
	case gprank_level_perplayer_exp:
		lua_pushnumber(L, gprank_level_perplaya->exp);
		break;
	case gprank_level_perplayer_prisons:
		lua_pushnumber(L, gprank_level_perplaya->prisons);
		break;
	case gprank_level_perplayer_gotspecialprize:
		lua_pushboolean(L, gprank_level_perplaya->gotSpecialPrize);
		break;
	case gprank_level_perplayer_grade:
		lua_pushnumber(L, gprank_level_perplaya->grade);
		break;
	default:
		return RNOFIELDGRLP;
	}
	
	return 1;
}

static int gprank_level_perplayer_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("gprank_level_perplayer") " struct cannot be edited by Lua.");
}

static int roundcue_get(lua_State *L)
{
	enum roundcue field = luaL_checkoption(L, 2, roundcue_opt[0], roundcue_opt);
	
	// Don't return any grandprixinfo values while not in a GP.
	if (!roundqueue.size)
	{
		switch (field)
		{
			case roundcue_size:
				lua_pushboolean(L, false);
				return 1;
			default:
				return ROUNDCUEERR;
		}
	}
	
	switch (field)
	{
	case roundcue_size:
		lua_pushnumber(L, roundqueue.size);
		break;
	case roundcue_roundnum:
		lua_pushnumber(L, roundqueue.roundnum);
		break;
	case roundcue_position:
		lua_pushnumber(L, roundqueue.position);
		break;
	case roundcue_netcommunicate:
		return UNIMPLEMENTED;
		break;
	case roundcue_writetextmap:
		return UNIMPLEMENTED;
		break;
	case roundcue_snapshotmaps:
		lua_pushboolean(L, roundqueue.snapshotmaps);
		break;
	case roundcue_entries:
		lua_createtable(L, roundqueue.size, 0);
		for (size_t i = 0; i < roundqueue.size; i++)
		{
			LUA_PushUserdata(L, &roundqueue.entries[i], META_ROUNDENTRY);
			lua_rawseti(L, -2, 1 + i);
		}
		break;
	default:
		return RNOFIELDRQ;
	}
	
	return 1;
}

static int roundcue_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("roundqueue") " struct cannot be edited by Lua.");
}

static int roundentry_get(lua_State *L)
{
	roundentry_t *roundentry = *((roundentry_t **)luaL_checkudata(L, 1, META_ROUNDENTRY));
	enum roundentry field = luaL_checkoption(L, 2, roundentry_opt[0], roundentry_opt);
	
	if (!roundentry)
	{
		switch (field)
		{
			case roundentry_valid:
				lua_pushboolean(L, false);
				return 1;
			default:
				return LUA_ErrInvalid(L, "roundentry_t");
		}
	}
	
	switch (field)
	{
	case roundentry_valid:
		lua_pushboolean(L, true);
		break;
	case roundentry_mapnum:
		lua_pushnumber(L, roundentry->mapnum);
		break;
	case roundentry_gametype:
		lua_pushnumber(L, roundentry->gametype);
		break;
	case roundentry_encore:
		lua_pushboolean(L, roundentry->encore);
		break;
	case roundentry_rankrestricted:
		lua_pushboolean(L, roundentry->rankrestricted);
		break;
	case roundentry_overridden:
		lua_pushboolean(L, roundentry->overridden);
		break;
	default:
		return RNOFIELDRE;
	}
	
	return 1;
}

static int roundentry_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("roundentry_t") " struct cannot be edited by Lua.");
}

#undef UNIMPLEMENTED 
#undef RNOFIELDGP 
#undef RNOFIELDCH 
#undef RNOFIELDGR 
#undef RNOFIELDGRL 
#undef RNOFIELDGRLP 
#undef RNOFIELDRQ 
#undef RNOFIELDRE 
#undef GPERR
#undef ROUNDCUEERR

static int lib_numCupheaders(lua_State *L)
{
	lua_pushinteger(L, numkartcupheaders);
	return 1;
}

// There was, in fact, a better thing to do here - thanks toaster
#define GETCUPERR UINT16_MAX

// copied and edited from G_MapNumber
static UINT16 LUA_GetCupByNum(UINT16 cupnum)
{
	cupheader_t *checkcup;
	// find by cup id
	if (cupnum != GETCUPERR)
	{
		if (cupnum >= numkartcupheaders)
			return GETCUPERR; // id outta range
		for (checkcup = kartcupheaders; checkcup->id <= numkartcupheaders; checkcup = checkcup->next)
		{
			if (checkcup->id != cupnum)
				continue;
			else
				break;
			return GETCUPERR; // id invalid
		}
		return checkcup->id;
	}

	return GETCUPERR;
}

// copied and edited from G_MapNumber
static UINT16 LUA_GetCupByName(const char * name)
{
	cupheader_t *checkcup;
	
	UINT32 hash = quickncasehash(name, MAXCUPNAME);

	// find by cup name/realname
	for (checkcup = kartcupheaders; checkcup != NULL; checkcup = checkcup->next)
	{
		if (hash != checkcup->namehash)
			continue;
		
		if (strcasecmp(checkcup->name, name) != 0)
			continue;

		return checkcup->id;
	}

	return GETCUPERR;
}

static int lib_iterateCups(lua_State *L)
{
	INT32 i = -1;
	cupheader_t *tempcup = kartcupheaders;

	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateCups);
		return 1;
	}

	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.

	if (!lua_isnil(L, 1))
	{
		i = ((*((cupheader_t **)luaL_checkudata(L, 1, META_CUP)))->id) + 1;
	}
	else
		i = 0;
	
	for (tempcup = kartcupheaders; tempcup->id < numkartcupheaders; tempcup = tempcup->next)
	{
		if (tempcup->next == NULL)
			break;
		
		if (tempcup->id >= i)
			break;
	}

	// cups are always valid, only added, never removed
	if (i < numkartcupheaders)
	{
		LUA_PushUserdata(L, tempcup, META_CUP);
		return 1;
	}

	return 0;
}

// Shamelessly copied and edited from lua_waypointslib.c (with thanks to JugadorXEI)
static int lib_getCupheader(lua_State *L)
{
	const char *field;
	size_t i;
	cupheader_t *checkcup;
	UINT16 getResult = GETCUPERR;
	
	// find cup by id number
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		i = luaL_checkinteger(L, 2);
		if (i > numkartcupheaders)
			return luaL_error(L, "cupheader_t id %d out of loaded range (0 - %d)", i, numkartcupheaders);
		getResult = LUA_GetCupByNum(i);
		if (getResult == GETCUPERR)
			return luaL_error(L, "cupheader_t id %d invalid", i);
		for (checkcup = kartcupheaders; checkcup->id < numkartcupheaders; checkcup = checkcup->next)
		{
			if (checkcup->id != getResult)
				continue;
			else
				break;
			
			return luaL_error(L, "cupheader_t id %d invalid (LUA_GetCupByNum failed?)", i);
		}
		LUA_PushUserdata(L, checkcup, META_CUP);
		return 1;
	}
	
	field = luaL_checkstring(L, 2);
	
	// special function iterate
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iterateCups);
		return 1;
	}
	
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		getResult = LUA_GetCupByName(field);
		if (getResult == GETCUPERR)
			return luaL_error(L, "no cupheader_t with name %s", field);
	}
	
	// If, after all this...
	if (getResult == GETCUPERR)
		return luaL_error(L, "internal failure in lua_grandprixlib.c???");
	
	for (checkcup = kartcupheaders; checkcup->id < numkartcupheaders; checkcup = checkcup->next)
	{
		if (checkcup->id != getResult)
			continue;
		else
			break;
		
		return luaL_error(L, "cupheader_t id %d invalid (LUA_GetCupByName failed?)", i);
	}
	
	LUA_PushUserdata(L, checkcup, META_CUP);
	return 1;
}

int LUA_GrandPrixLib(lua_State *L)
{
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, grandprix_get);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, grandprix_set);
			lua_setfield(L, -2, "__newindex");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "grandprixinfo");
	
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, roundcue_get);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, roundcue_set);
			lua_setfield(L, -2, "__newindex");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "roundqueue");

	luaL_newmetatable(L, META_CUP);
		lua_pushcfunction(L, cup_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, cup_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_GPRANK);
		lua_pushcfunction(L, gprank_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, gprank_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_GPRANKLEVEL);
		lua_pushcfunction(L, gprank_level_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, gprank_level_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_GPRANKLEVELPERPLAYER);
		lua_pushcfunction(L, gprank_level_perplayer_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, gprank_level_perplayer_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_ROUNDENTRY);
		lua_pushcfunction(L, roundentry_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, roundentry_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getCupheader);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numCupheaders);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "cups");
	
	return 0;
}