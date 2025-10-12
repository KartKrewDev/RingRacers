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
/// \file  lua_itemroulettelib.c
/// \brief player item roulette structure library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "d_player.h"
#include "z_zone.h"
#include "k_roulette.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "lua_hook.h" // hook_cmd_running errors

enum itemroulette {
	itemroulette_valid = 0,
	itemroulette_active,
	itemroulette_itemlist,
	itemroulette_playing,
	itemroulette_exiting,
	itemroulette_preexpdist,
	itemroulette_dist,
	itemroulette_basedist,
	itemroulette_firstdist,
	itemroulette_seconddist,
	itemroulette_secondtofirst,
	itemroulette_index,
	itemroulette_sound,
	itemroulette_speed,
	itemroulette_tics,
	itemroulette_elapsed,
	itemroulette_eggman,
	itemroulette_ringbox,
	itemroulette_autoroulette,
	itemroulette_reserved,
	itemroulette_popcorn
};
static const char *const itemroulette_opt[] = {
	"valid",
	"active",
	"itemlist",
	"playing",
	"exiting",
	"preexpdist",
	"dist",
	"basedist",
	"firstdist",
	"seconddist",
	"secondtofirst",
	"index",
	"sound",
	"speed",
	"tics",
	"elapsed",
	"eggman",
	"ringbox",
	"autoroulette",
	"reserved",
	"popcorn",
	NULL
};

static int itemroulette_get(lua_State *L)
{
	itemroulette_t *itemroulette = *((itemroulette_t **)luaL_checkudata(L, 1, META_ITEMROULETTE));
	enum itemroulette field = luaL_checkoption(L, 2, itemroulette_opt[0], itemroulette_opt);

	// if this is null, welcome to parking garage rally circuit
	I_Assert(itemroulette != NULL);
	
	switch (field)
	{
	case itemroulette_valid:
		lua_pushboolean(L, itemroulette != NULL);
		break;
	case itemroulette_active:
		lua_pushboolean(L, itemroulette->active);
		break;
	case itemroulette_itemlist:
		LUA_PushUserdata(L, &itemroulette->itemList, META_ITEMROULETTE_ITEMLIST);	
		break;
	case itemroulette_playing:
		lua_pushinteger(L, itemroulette->playing);
		break;
	case itemroulette_exiting:
		lua_pushinteger(L, itemroulette->exiting);
		break;
	case itemroulette_preexpdist:
		lua_pushinteger(L, itemroulette->preexpdist);
		break;
	case itemroulette_dist:
		lua_pushinteger(L, itemroulette->dist);
		break;
	case itemroulette_basedist:
		lua_pushinteger(L, itemroulette->baseDist);
		break;
	case itemroulette_firstdist:
		lua_pushinteger(L, itemroulette->firstDist);
		break;
	case itemroulette_seconddist:
		lua_pushinteger(L, itemroulette->secondDist);
		break;
	case itemroulette_secondtofirst:
		lua_pushinteger(L, itemroulette->secondToFirst);
		break;
	case itemroulette_index:
		lua_pushinteger(L, itemroulette->index);
		break;
	case itemroulette_sound:
		lua_pushinteger(L, itemroulette->sound);
		break;
	case itemroulette_speed:
		lua_pushinteger(L, itemroulette->speed);
		break;
	case itemroulette_tics:
		lua_pushinteger(L, itemroulette->tics);
		break;
	case itemroulette_elapsed:
		lua_pushinteger(L, itemroulette->elapsed);
		break;
	case itemroulette_eggman:
		lua_pushboolean(L, itemroulette->eggman);
		break;
	case itemroulette_ringbox:
		lua_pushboolean(L, itemroulette->ringbox);
		break;
	case itemroulette_autoroulette:
		lua_pushboolean(L, itemroulette->autoroulette);
		break;
	case itemroulette_reserved:
		lua_pushinteger(L, itemroulette->reserved);
		break;
	case itemroulette_popcorn:
		lua_pushinteger(L, itemroulette->popcorn);
		break;
	}

	return 1;
}

#define NOSET luaL_error(L, LUA_QL("itemroulette_t") " field " LUA_QS " should not be set directly.", itemroulette_opt[field])
#define UNIMPLEMENTED luaL_error(L, LUA_QL("itemroulette_t") " field " LUA_QS " is not implemented for Lua and cannot be accessed.", itemroulette_opt[field])
#define NOSETITEMLIST luaL_error(L, LUA_QL("itemroulette_t") " field " LUA_QS " should not be set directly. Use " LUA_QL("K_AddItemToReel") ", " LUA_QL("K_PushToRouletteItemList") ", or " LUA_QL("K_StartItemRoulette") ", or " LUA_QL("K_StopRoulette") " instead.", itemroulette_opt[field])

static int itemroulette_set(lua_State *L)
{
	itemroulette_t *itemroulette = *((itemroulette_t **)luaL_checkudata(L, 1, META_ITEMROULETTE));
	enum itemroulette field = luaL_checkoption(L, 2, itemroulette_opt[0], itemroulette_opt);

	// if this is null, welcome to parking garage rally circuit
	I_Assert(itemroulette != NULL);

	INLEVEL

	if (hud_running)
		return luaL_error(L, "Do not alter itemroulette_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter itemroulette_t in CMD building code!");

	switch(field)
	{
	case itemroulette_valid:
		return NOSET;
	case itemroulette_active:
		itemroulette->active = luaL_checkboolean(L, 3);
		break;
	case itemroulette_itemlist:
		return NOSETITEMLIST;
	case itemroulette_playing:
		itemroulette->playing = luaL_checkinteger(L, 3);
		break;
	case itemroulette_exiting:
		itemroulette->exiting = luaL_checkinteger(L, 3);
		break;
	case itemroulette_preexpdist:
		itemroulette->preexpdist = luaL_checkinteger(L, 3);
		break;
	case itemroulette_dist:
		itemroulette->dist = luaL_checkinteger(L, 3);
		break;
	case itemroulette_basedist:
		itemroulette->baseDist = luaL_checkinteger(L, 3);
		break;
	case itemroulette_firstdist:
		itemroulette->firstDist = luaL_checkinteger(L, 3);
		break;
	case itemroulette_seconddist:
		itemroulette->secondDist = luaL_checkinteger(L, 3);
		break;
	case itemroulette_secondtofirst:
		itemroulette->secondToFirst = luaL_checkinteger(L, 3);
		break;
	case itemroulette_index:
		itemroulette->index = luaL_checkinteger(L, 3);
		break;
	case itemroulette_sound:
		itemroulette->sound = luaL_checkinteger(L, 3);
		break;
	case itemroulette_speed:
		itemroulette->speed = luaL_checkinteger(L, 3);
		break;
	case itemroulette_tics:
		itemroulette->tics = luaL_checkinteger(L, 3);
		break;
	case itemroulette_elapsed:
		itemroulette->elapsed = luaL_checkinteger(L, 3);
		break;
	case itemroulette_eggman:
		itemroulette->eggman = luaL_checkboolean(L, 3);
		break;
	case itemroulette_ringbox:
		itemroulette->ringbox = luaL_checkboolean(L, 3);
		break;
	case itemroulette_autoroulette:
		itemroulette->autoroulette = luaL_checkboolean(L, 3);
		break;
	case itemroulette_reserved:
		itemroulette->reserved = luaL_checkinteger(L, 3);
		break;
	case itemroulette_popcorn:
		itemroulette->popcorn = luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

#undef NOSET
#undef NOSETITEMLIST
#undef UNIMPLEMENTED

// itemlist, i -> itemlist[i]
static int itemrouletteitemlist_get(lua_State *L)
{
	itemlist_t *itemlist = *((itemlist_t **)luaL_checkudata(L, 1, META_ITEMROULETTE_ITEMLIST));
	size_t index = luaL_checkint(L, 2);
	
	if (!itemlist)
		return LUA_ErrInvalid(L, "itemroulette_t.itemlist_t");

	if (index == 0 || index > itemlist->len) {
		return luaL_error(L, LUA_QL("itemroulette_t.itemlist_t") " index cannot be %d", index);
	}
	lua_pushinteger(L, itemlist->items[index-1]);
	return 1;
}

static int itemrouletteitemlist_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("itemroulette_t.itemlist_t") " struct cannot be edited by Lua.");
}

// #itemlist -> itemList.len
static int itemrouletteitemlist_len(lua_State* L)
{
	itemlist_t *itemlist = *((itemlist_t **)luaL_checkudata(L, 1, META_ITEMROULETTE_ITEMLIST));
	lua_pushinteger(L, itemlist->len);
	return 1;
}

int LUA_ItemRouletteLib(lua_State *L)
{
	luaL_newmetatable(L, META_ITEMROULETTE);
		lua_pushcfunction(L, itemroulette_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, itemroulette_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);
	
	luaL_newmetatable(L, META_ITEMROULETTE_ITEMLIST);
		lua_pushcfunction(L, itemrouletteitemlist_get);
		lua_setfield(L, -2, "__index");
		
		lua_pushcfunction(L, itemrouletteitemlist_set);
		lua_setfield(L, -2, "__newindex");
		
		lua_pushcfunction(L, itemrouletteitemlist_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	return 0;
}