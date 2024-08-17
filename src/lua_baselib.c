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
/// \file  lua_baselib.c
/// \brief basic functions for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_local.h"
#include "p_setup.h" // So we can have P_SetupLevelSky
#include "p_slopes.h" // P_GetSlopeZAt
#include "z_zone.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_things.h" // R_Frame2Char etc
#include "m_random.h"
#include "s_sound.h"
#include "g_game.h"
#include "y_inter.h"
#include "hu_stuff.h"	// HU_AddChatText
#include "console.h"
#include "k_kart.h" // SRB2Kart
#include "k_battle.h"
#include "k_boss.h"
#include "k_collide.h"
#include "k_color.h"
#include "k_hud.h"
#include "d_netcmd.h" // IsPlayerAdmin
#include "k_menu.h" // Player Setup menu color stuff
#include "p_spec.h" // P_StartQuake
#include "i_system.h" // I_GetPreciseTime, I_GetPrecisePrecision
#include "hu_stuff.h" // for the cecho
#include "k_powerup.h"
#include "k_hitlag.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "taglist.h" // P_FindSpecialLineFromTag
#include "lua_hook.h" // hook_cmd_running errors

#define NOHUD if (hud_running)\
return luaL_error(L, "HUD rendering code should not call this function!");\
else if (hook_cmd_running)\
return luaL_error(L, "CMD building code should not call this function!");

boolean luaL_checkboolean(lua_State *L, int narg) {
	luaL_checktype(L, narg, LUA_TBOOLEAN);
	return lua_toboolean(L, narg);
}

// String concatination
static int lib_concat(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  char *r = NULL;
  size_t rl = 0,sl;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tolstring(L, -1, &sl);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
													 LUA_QL("__add"));
		r = Z_Realloc(r, rl+sl, PU_STATIC, NULL);
		M_Memcpy(r+rl, s, sl);
		rl += sl;
    lua_pop(L, 1);  /* pop result */
  }
  lua_pushlstring(L, r, rl);
  Z_Free(r);
	return 1;
}

// Wrapper for CONS_Printf
// Copied from base Lua code
static int lib_print(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  //HUDSAFE
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
													 LUA_QL("print"));
    if (i>1) CONS_Printf("\n");
    CONS_Printf("%s", s);
    lua_pop(L, 1);  /* pop result */
  }
	CONS_Printf("\n");
	return 0;
}

// Print stuff in the chat, or in the console if we can't.
static int lib_chatprint(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);	// retrieve string
	boolean sound = lua_optboolean(L, 2);	// retrieve sound boolean
	int len = strlen(str);

	if (str == NULL)	// error if we don't have a string!
		return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("chatprint"));

	if (len > 255)	// string is too long!!!
		return luaL_error(L, "String exceeds the 255 characters limit of the chat buffer.");

	HU_AddChatText(str, sound);
	return 0;
}

// Same as above, but do it for only one player.
static int lib_chatprintf(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	const char *str = luaL_checkstring(L, 2);	// retrieve string
	boolean sound = lua_optboolean(L, 3);	// sound?
	int len = strlen(str);
	player_t *plr;

	if (n < 2)
		return luaL_error(L, "chatprintf requires at least two arguments: player and text.");

	plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));	// retrieve player
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");
	if (plr != &players[consoleplayer])
		return 0;

	if (str == NULL)	// error if we don't have a string!
		return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("chatprintf"));

	if (len > 255)	// string is too long!!!
		return luaL_error(L, "String exceeds the 255 characters limit of the chat buffer.");

	HU_AddChatText(str, sound);
	return 0;
}

static const struct {
	const char *meta;
	const char *utype;
} meta2utype[] = {
	{META_STATE,        "state_t"},
	{META_MOBJINFO,     "mobjinfo_t"},
	{META_SFXINFO,      "sfxinfo_t"},
	{META_SKINCOLOR,    "skincolor_t"},
	{META_COLORRAMP,    "skincolor_t.ramp"},
	{META_SPRITEINFO,   "spriteinfo_t"},
	{META_PIVOTLIST,    "spriteframepivot_t[]"},
	{META_FRAMEPIVOT,   "spriteframepivot_t"},
	{META_PRECIPPROPS,  "precipprops_t"},

	{META_TAGLIST,      "taglist"},

	{META_MOBJ,         "mobj_t"},
	{META_MAPTHING,     "mapthing_t"},

	{META_PLAYER,       "player_t"},
	{META_TICCMD,       "ticcmd_t"},
	{META_SKIN,         "skin_t"},
	{META_SOUNDSID,     "skin_t.soundsid"},
	{META_SKINSPRITES,  "skin_t.sprites"},
	{META_SKINSPRITESLIST,  "skin_t.sprites[]"},

	{META_VERTEX,       "vertex_t"},
	{META_LINE,         "line_t"},
	{META_SIDE,         "side_t"},
	{META_SUBSECTOR,    "subsector_t"},
	{META_SECTOR,       "sector_t"},
	{META_FFLOOR,       "ffloor_t"},
#ifdef HAVE_LUA_SEGS
	{META_SEG,          "seg_t"},
	{META_NODE,         "node_t"},
#endif
	{META_SLOPE,        "slope_t"},
	{META_VECTOR2,      "vector2_t"},
	{META_VECTOR3,      "vector3_t"},
	{META_MAPHEADER,    "mapheader_t"},

	{META_POLYOBJ,      "polyobj_t"},
	{META_POLYOBJVERTICES, "polyobj_t.vertices"},
	{META_POLYOBJLINES, "polyobj_t.lines"},

	{META_CVAR,         "consvar_t"},

	{META_SECTORLINES,  "sector_t.lines"},
#ifdef MUTABLE_TAGS
	{META_SECTORTAGLIST, "sector_t.taglist"},
#endif
	{META_SIDENUM,      "line_t.sidenum"},
	{META_LINEARGS,     "line_t.args"},
	{META_LINESTRINGARGS, "line_t.stringargs"},

	{META_SECTORARGS,     "sector_t.args"},
	{META_SECTORSTRINGARGS, "sector_t.stringargs"},

	{META_THINGARGS,     "mapthing.args"},
	{META_THINGSTRINGARGS, "mapthing.stringargs"},
#ifdef HAVE_LUA_SEGS
	{META_NODEBBOX,     "node_t.bbox"},
	{META_NODECHILDREN, "node_t.children"},
#endif

	{META_BBOX,         "bbox"},

	{META_PATCH,        "patch_t"},
	{META_COLORMAP,     "colormap"},
	{META_CAMERA,       "camera_t"},

	{META_ACTION,       "action"},

	{META_LUABANKS,     "luabanks[]"},

	{META_ACTIVATOR,    "activator_t"},
	{NULL,              NULL}
};

// goes through the above list and returns the utype string for the userdata type
// returns "unknown" instead if we couldn't find the right userdata type
static const char *GetUserdataUType(lua_State *L)
{
	UINT8 i;
	lua_getmetatable(L, -1);

	for (i = 0; meta2utype[i].meta; i++)
	{
		luaL_getmetatable(L, meta2utype[i].meta);
		if (lua_rawequal(L, -1, -2))
		{
			lua_pop(L, 2);
			return meta2utype[i].utype;
		}
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	return "unknown";
}

// Return a string representing the type of userdata the given var is
// e.g. players[0] -> "player_t"
//   or players[0].powers -> "player_t.powers"
static int lib_userdataType(lua_State *L)
{
	lua_settop(L, 1); // pop everything except arg 1 (in case somebody decided to add more)
	luaL_checktype(L, 1, LUA_TUSERDATA);
	lua_pushstring(L, GetUserdataUType(L));
	return 1;
}

// Takes a metatable as first and only argument
// Only callable during script loading
static int lib_registerMetatable(lua_State *L)
{
	static UINT16 nextid = 1;

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");
	luaL_checktype(L, 1, LUA_TTABLE);

	if (nextid == 0)
		return luaL_error(L, "Too many metatables registered?! Please consider rewriting your script once you are sober again.\n");

	lua_getfield(L, LUA_REGISTRYINDEX, LREG_METATABLES); // 2
		// registry.metatables[metatable] = nextid
		lua_pushvalue(L, 1); // 3
			lua_pushnumber(L, nextid); // 4
		lua_settable(L, 2);

		// registry.metatables[nextid] = metatable
		lua_pushnumber(L, nextid); // 3
			lua_pushvalue(L, 1); // 4
		lua_settable(L, 2);
	lua_pop(L, 1);

	nextid++;

	return 0;
}

// Takes a string as only argument and returns the metatable
// associated to the userdata type this string refers to
// Returns nil if the string does not refer to a valid userdata type
static int lib_userdataMetatable(lua_State *L)
{
	UINT32 i;
	const char *udname = luaL_checkstring(L, 1);

	// Find internal metatable name
	for (i = 0; meta2utype[i].meta; i++)
		if (!strcmp(udname, meta2utype[i].utype))
		{
			luaL_getmetatable(L, meta2utype[i].meta);
			return 1;
		}

	lua_pushnil(L);
	return 1;
}

static int lib_isPlayerAdmin(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, IsPlayerAdmin(player-players));
	return 1;
}

static int lib_reserveLuabanks(lua_State *L)
{
	static boolean reserved = false;
	if (!lua_lumploading)
		return luaL_error(L, "luabanks[] cannot be reserved from within a hook or coroutine!");
	if (reserved)
		return luaL_error(L, "luabanks[] has already been reserved! Only one savedata-enabled mod at a time may use this feature.");
	reserved = true;
	LUA_PushUserdata(L, &luabanks, META_LUABANKS);
	return 1;
}

// M_MENU
//////////////

static int lib_pGetEffectiveFollowerColor(lua_State *L)
{
	UINT16 followercolor = (UINT16)luaL_checkinteger(L, 1);
	UINT16 playercolor = (UINT16)luaL_checkinteger(L, 2);
	lua_pushinteger(L, K_GetEffectiveFollowerColor(followercolor, NULL, playercolor, NULL)); // FIXME: follower / skin
	return 1;
}

// M_RANDOM
//////////////

// TODO: Lua needs a way to set RNG class, which will break compatibility.
// It will be more desireable to do it when RNG classes can be freeslotted.

static int lib_pRandomFixed(lua_State *L)
{
	NOHUD
	lua_pushfixed(L, P_RandomFixed(PR_UNDEFINED));
	demo_writerng = 2;
	return 1;
}

static int lib_pRandomByte(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_RandomByte(PR_UNDEFINED));
	demo_writerng = 2;
	return 1;
}

static int lib_pRandomKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	NOHUD
	lua_pushinteger(L, P_RandomKey(PR_UNDEFINED, a));
	demo_writerng = 2;
	return 1;
}

static int lib_pRandomRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);
	NOHUD
	if (b < a)
	{
		INT32 c = a;
		a = b;
		b = c;
	}
	lua_pushinteger(L, P_RandomRange(PR_UNDEFINED, a, b));
	demo_writerng = 2;
	return 1;
}

// Macros.
static int lib_pSignedRandom(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_SignedRandom(PR_UNDEFINED));
	demo_writerng = 2;
	return 1;
}

static int lib_pRandomChance(lua_State *L)
{
	fixed_t p = luaL_checkfixed(L, 1);
	NOHUD
	lua_pushboolean(L, P_RandomChance(PR_UNDEFINED, p));
	demo_writerng = 2;
	return 1;
}

// P_MAPUTIL
///////////////

static int lib_pAproxDistance(lua_State *L)
{
	fixed_t dx = luaL_checkfixed(L, 1);
	fixed_t dy = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, P_AproxDistance(dx, dy));
	return 1;
}

static int lib_pClosestPointOnLine(lua_State *L)
{
	int n = lua_gettop(L);
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	vertex_t result;
	//HUDSAFE
	if (lua_isuserdata(L, 3)) // use a real linedef to get our points
	{
		line_t *line = *((line_t **)luaL_checkudata(L, 3, META_LINE));
		if (!line)
			return LUA_ErrInvalid(L, "line_t");
		P_ClosestPointOnLine(x, y, line, &result);
	}
	else // use custom coordinates of our own!
	{
		vertex_t v1, v2; // fake vertexes
		line_t junk; // fake linedef

		if (n < 6)
			return luaL_error(L, "arguments 3 to 6 not all given (expected 4 fixed-point integers)");

		v1.x = luaL_checkfixed(L, 3);
		v1.y = luaL_checkfixed(L, 4);
		v2.x = luaL_checkfixed(L, 5);
		v2.y = luaL_checkfixed(L, 6);

		junk.v1 = &v1;
		junk.v2 = &v2;
		junk.dx = v2.x - v1.x;
		junk.dy = v2.y - v1.y;
		P_ClosestPointOnLine(x, y, &junk, &result);
	}

	lua_pushfixed(L, result.x);
	lua_pushfixed(L, result.y);
	return 2;
}

static int lib_pPointOnLineSide(lua_State *L)
{
	int n = lua_gettop(L);
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	if (lua_isuserdata(L, 3)) // use a real linedef to get our points
	{
		line_t *line = *((line_t **)luaL_checkudata(L, 3, META_LINE));
		if (!line)
			return LUA_ErrInvalid(L, "line_t");
		lua_pushinteger(L, P_PointOnLineSide(x, y, line));
	}
	else // use custom coordinates of our own!
	{
		vertex_t v1, v2; // fake vertexes
		line_t junk; // fake linedef

		if (n < 6)
			return luaL_error(L, "arguments 3 to 6 not all given (expected 4 fixed-point integers)");

		v1.x = luaL_checkfixed(L, 3);
		v1.y = luaL_checkfixed(L, 4);
		v2.x = luaL_checkfixed(L, 5);
		v2.y = luaL_checkfixed(L, 6);

		junk.v1 = &v1;
		junk.v2 = &v2;
		junk.dx = v2.x - v1.x;
		junk.dy = v2.y - v1.y;
		lua_pushinteger(L, P_PointOnLineSide(x, y, &junk));
	}
	return 1;
}

// P_ENEMY
/////////////

static int lib_pCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckMeleeRange(actor));
	return 1;
}

static int lib_pJetbCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_JetbCheckMeleeRange(actor));
	return 1;
}

static int lib_pFaceStabCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_FaceStabCheckMeleeRange(actor));
	return 1;
}

static int lib_pSkimCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SkimCheckMeleeRange(actor));
	return 1;
}

static int lib_pCheckMissileRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckMissileRange(actor));
	return 1;
}

static int lib_pNewChaseDir(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_NewChaseDir(actor);
	return 0;
}

static int lib_pLookForPlayers(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t dist = (fixed_t)luaL_optinteger(L, 2, 0);
	boolean allaround = lua_optboolean(L, 3);
	boolean tracer = lua_optboolean(L, 4);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_LookForPlayers(actor, allaround, tracer, dist));
	return 1;
}

// P_MOBJ
////////////

static int lib_pSpawnMobj(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	mobjtype_t type = luaL_checkinteger(L, 4);
	NOHUD
	INLEVEL
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnMobj(x, y, z, type), META_MOBJ);
	return 1;
}

static int lib_pSpawnMobjFromMobj(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	mobjtype_t type = luaL_checkinteger(L, 5);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnMobjFromMobj(actor, x, y, z, type), META_MOBJ);
	return 1;
}

static int lib_pSpawnMobjFromMobjUnscaled(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	mobjtype_t type = luaL_checkinteger(L, 5);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnMobjFromMobjUnscaled(actor, x, y, z, type), META_MOBJ);
	return 1;
}

static int lib_pRemoveMobj(lua_State *L)
{
	mobj_t *th = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!th)
		return LUA_ErrInvalid(L, "mobj_t");
	if (th->player)
		return luaL_error(L, "Attempt to remove player mobj with P_RemoveMobj.");
	P_RemoveMobj(th);
	return 0;
}

// P_IsValidSprite2 technically doesn't exist, and probably never should... but too much would need to be exposed to allow this to be checked by other methods.

static int lib_pIsValidSprite2(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	UINT8 spr2 = (UINT8)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, (mobj->skin && (((skin_t *)mobj->skin)->sprites[spr2].numframes)));
	return 1;
}

// P_SpawnLockOn doesn't exist either, but we want to expose making a local mobj without encouraging hacks.

static int lib_pSpawnLockOn(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *lockon = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	statenum_t state = luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!lockon)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (state >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", state, NUMSTATES-1);
#if 0
	if (P_IsPartyPlayer(player)) // Only display it on your own view.
	{
		mobj_t *visual = P_SpawnMobj(lockon->x, lockon->y, lockon->z, MT_LOCKON); // positioning, flip handled in P_SceneryThinker
		P_SetTarget(&visual->target, lockon);
		visual->renderflags |= RF_DONTDRAW;
		P_SetMobjStateNF(visual, state);
	}
#else
	CONS_Alert(CONS_WARNING, "TODO: P_SpawnLockOn is deprecated\n");
#endif
	return 0;
}

static int lib_pSpawnMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *dest = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnMissile(source, dest, type), META_MOBJ);
	return 1;
}

static int lib_pSpawnXYZMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *dest = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 3);
	fixed_t x = luaL_checkfixed(L, 4);
	fixed_t y = luaL_checkfixed(L, 5);
	fixed_t z = luaL_checkfixed(L, 6);
	NOHUD
	INLEVEL
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnXYZMissile(source, dest, type, x, y, z), META_MOBJ);
	return 1;
}

static int lib_pSpawnPointMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t xa = luaL_checkfixed(L, 2);
	fixed_t ya = luaL_checkfixed(L, 3);
	fixed_t za = luaL_checkfixed(L, 4);
	mobjtype_t type = luaL_checkinteger(L, 5);
	fixed_t x = luaL_checkfixed(L, 6);
	fixed_t y = luaL_checkfixed(L, 7);
	fixed_t z = luaL_checkfixed(L, 8);
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnPointMissile(source, xa, ya, za, type, x, y, z), META_MOBJ);
	return 1;
}

static int lib_pSpawnAlteredDirectionMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	fixed_t x = luaL_checkfixed(L, 3);
	fixed_t y = luaL_checkfixed(L, 4);
	fixed_t z = luaL_checkfixed(L, 5);
	INT32 shiftingAngle = (INT32)luaL_checkinteger(L, 5);
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnAlteredDirectionMissile(source, type, x, y, z, shiftingAngle), META_MOBJ);
	return 1;
}

static int lib_pColorTeamMissile(lua_State *L)
{
	mobj_t *missile = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *source = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	NOHUD
	INLEVEL
	if (!missile)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!source)
		return LUA_ErrInvalid(L, "player_t");
	P_ColorTeamMissile(missile, source);
	return 0;
}

static int lib_pSPMAngle(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	angle_t angle = luaL_checkangle(L, 3);
	UINT8 allowaim = (UINT8)luaL_optinteger(L, 4, 0);
	UINT32 flags2 = (UINT32)luaL_optinteger(L, 5, 0);
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SPMAngle(source, type, angle, allowaim, flags2), META_MOBJ);
	return 1;
}

static int lib_pSpawnPlayerMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	UINT32 flags2 = (UINT32)luaL_optinteger(L, 3, 0);
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	LUA_PushUserdata(L, P_SpawnPlayerMissile(source, type, flags2), META_MOBJ);
	return 1;
}

static int lib_pMobjFlip(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushinteger(L, P_MobjFlip(mobj));
	return 1;
}

static int lib_pGetMobjGravity(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushfixed(L, P_GetMobjGravity(mobj));
	return 1;
}

static int lib_pFlashPal(lua_State *L)
{
	player_t *pl = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT16 type = (UINT16)luaL_checkinteger(L, 2);
	UINT16 duration = (UINT16)luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!pl)
		return LUA_ErrInvalid(L, "player_t");
	P_FlashPal(pl, type, duration);
	return 0;
}

static int lib_pGetClosestAxis(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_GetClosestAxis(source), META_MOBJ);
	return 1;
}

static int lib_pSpawnParaloop(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	fixed_t radius = luaL_checkfixed(L, 4);
	INT32 number = (INT32)luaL_checkinteger(L, 5);
	mobjtype_t type = luaL_checkinteger(L, 6);
	angle_t rotangle = luaL_checkangle(L, 7);
	statenum_t nstate = luaL_optinteger(L, 8, S_NULL);
	boolean spawncenter = lua_optboolean(L, 9);
	NOHUD
	INLEVEL
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	if (nstate >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", nstate, NUMSTATES-1);
	P_SpawnParaloop(x, y, z, radius, number, type, nstate, rotangle, spawncenter);
	return 0;
}

static int lib_pBossTargetPlayer(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	boolean closest = lua_optboolean(L, 2);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_BossTargetPlayer(actor, closest));
	return 1;
}

static int lib_pSupermanLook4Players(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SupermanLook4Players(actor));
	return 1;
}

static int lib_pSetScale(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t newscale = luaL_checkfixed(L, 2);
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (newscale < FRACUNIT/100)
		newscale = FRACUNIT/100;
	P_SetScale(mobj, newscale);
	return 0;
}

static int lib_pInsideANonSolidFFloor(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	lua_pushboolean(L, P_InsideANonSolidFFloor(mobj, rover));
	return 1;
}

static int lib_pCheckDeathPitCollide(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckDeathPitCollide(mo));
	return 1;
}

static int lib_pCheckSolidLava(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	lua_pushboolean(L, P_CheckSolidLava(mo, rover));
	return 1;
}

static int lib_pMaceRotate(lua_State *L)
{
	mobj_t *center = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	INT32 baserot = luaL_checkinteger(L, 2);
	INT32 baseprevrot = luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!center)
		return LUA_ErrInvalid(L, "mobj_t");
	P_MaceRotate(center, baserot, baseprevrot);
	return 0;
}

static int lib_pCreateFloorSpriteSlope(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, (pslope_t *)P_CreateFloorSpriteSlope(mobj), META_SLOPE);
	return 1;
}

static int lib_pRemoveFloorSpriteSlope(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RemoveFloorSpriteSlope(mobj);
	return 1;
}

static int lib_pRailThinker(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_RailThinker(mobj));
	P_RestoreTMStruct(ptm);
	return 1;
}

static int lib_pXYMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_XYMovement(actor);
	P_RestoreTMStruct(ptm);
	return 0;
}

static int lib_pRingXYMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RingXYMovement(actor);
	P_RestoreTMStruct(ptm);
	return 0;
}

static int lib_pSceneryXYMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SceneryXYMovement(actor);
	P_RestoreTMStruct(ptm);
	return 0;
}

static int lib_pZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_ZMovement(actor));
	P_CheckPosition(actor, actor->x, actor->y, NULL);
	P_RestoreTMStruct(ptm);
	return 1;
}

static int lib_pRingZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RingZMovement(actor);
	P_CheckPosition(actor, actor->x, actor->y, NULL);
	P_RestoreTMStruct(ptm);
	return 0;
}

static int lib_pSceneryZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SceneryZMovement(actor));
	P_CheckPosition(actor, actor->x, actor->y, NULL);
	P_RestoreTMStruct(ptm);
	return 1;
}

static int lib_pPlayerZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_PlayerZMovement(actor);
	P_CheckPosition(actor, actor->x, actor->y, NULL);
	P_RestoreTMStruct(ptm);
	return 0;
}

// P_USER
////////////

static int lib_pAddPlayerScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 amount = (UINT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_AddPlayerScore(player, amount);
	return 0;
}

static int lib_pPlayerInPain(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_PlayerInPain(player));
	return 1;
}

static int lib_pResetPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ResetPlayer(player);
	return 0;
}

static int lib_pPlayerFullbright(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_PlayerFullbright(player));
	return 1;
}


static int lib_pIsObjectInGoop(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_IsObjectInGoop(mo));
	return 1;
}

static int lib_pIsObjectOnGround(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_IsObjectOnGround(mo));
	return 1;
}

static int lib_pInQuicksand(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_InQuicksand(mo));
	return 1;
}

static int lib_pSetObjectMomZ(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t value = luaL_checkfixed(L, 2);
	boolean relative = lua_optboolean(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SetObjectMomZ(mo, value, relative);
	return 0;
}

static int lib_pSpawnGhostMobj(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnGhostMobj(mobj), META_MOBJ);
	return 1;
}

static int lib_pSpawnFakeShadow(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	UINT8 offset = (UINT8)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnFakeShadow(mobj, offset), META_MOBJ);
	return 1;
}

static int lib_pGivePlayerRings(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 num_rings = (INT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, P_GivePlayerRings(player, num_rings));
	return 1;
}

static int lib_pGivePlayerLives(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 numlives = (INT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_GivePlayerLives(player, numlives);
	return 0;
}

static int lib_pMovePlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	tm_t ptm = g_tm;
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_MovePlayer(player);
	P_RestoreTMStruct(ptm);
	return 0;
}

static int lib_pDoPlayerExit(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	pflags_t flags = luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoPlayerExit(player, flags);
	return 0;
}

static int lib_pInstaThrust(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = luaL_checkangle(L, 2);
	fixed_t move = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_InstaThrust(mo, angle, move);
	return 0;
}

static int lib_pReturnThrustX(lua_State *L)
{
	angle_t angle;
	fixed_t move;
	if (lua_isnil(L, 1) || lua_isuserdata(L, 1))
		lua_remove(L, 1); // ignore mobj as arg1
	angle = luaL_checkangle(L, 1);
	move = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, P_ReturnThrustX(NULL, angle, move));
	return 1;
}

static int lib_pReturnThrustY(lua_State *L)
{
	angle_t angle;
	fixed_t move;
	if (lua_isnil(L, 1) || lua_isuserdata(L, 1))
		lua_remove(L, 1); // ignore mobj as arg1
	angle = luaL_checkangle(L, 1);
	move = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, P_ReturnThrustY(NULL, angle, move));
	return 1;
}

static int lib_pNukeEnemies(lua_State *L)
{
	mobj_t *inflictor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t radius = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!inflictor || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_NukeEnemies(inflictor, source, radius);
	return 0;
}

// P_MAP
///////////

static int lib_pCheckPosition(lua_State *L)
{
	tm_t ptm = g_tm;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckPosition(thing, x, y, NULL));
	LUA_PushUserdata(L, g_tm.thing, META_MOBJ);
	P_RestoreTMStruct(ptm);
	return 2;
}

static int lib_pTryMove(lua_State *L)
{
	tm_t ptm = g_tm;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	boolean allowdropoff = lua_optboolean(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_TryMove(thing, x, y, allowdropoff, NULL));
	LUA_PushUserdata(L, g_tm.thing, META_MOBJ);
	P_RestoreTMStruct(ptm);
	return 2;
}

static int lib_pMove(lua_State *L)
{
	tm_t ptm = g_tm;
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t speed = luaL_checkfixed(L, 2);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_Move(actor, speed));
	LUA_PushUserdata(L, g_tm.thing, META_MOBJ);
	P_RestoreTMStruct(ptm);
	return 2;
}

static int lib_pTeleportMove(lua_State *L)
{
	tm_t ptm = g_tm;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_Deprecated(L, "P_TeleportMove", "P_SetOrigin\" or \"P_MoveOrigin");
	lua_pushboolean(L, P_MoveOrigin(thing, x, y, z));
	LUA_PushUserdata(L, g_tm.thing, META_MOBJ);
	P_RestoreTMStruct(ptm);
	return 2;
}

static int lib_pSetOrigin(lua_State *L)
{
	tm_t ptm = g_tm;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SetOrigin(thing, x, y, z));
	LUA_PushUserdata(L, g_tm.thing, META_MOBJ);
	P_RestoreTMStruct(ptm);
	return 2;
}

static int lib_pMoveOrigin(lua_State *L)
{
	tm_t ptm = g_tm;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_MoveOrigin(thing, x, y, z));
	LUA_PushUserdata(L, g_tm.thing, META_MOBJ);
	P_RestoreTMStruct(ptm);
	return 2;
}

static int lib_pSetAngle(lua_State *L)
{
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t newValue = luaL_checkangle(L, 2);
	NOHUD
	INLEVEL

	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	//P_SetAngle(thing, newValue);
	thing->angle = thing->old_angle = newValue;
	return 0;
}

static int lib_pSetPitch(lua_State *L)
{
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t newValue = luaL_checkangle(L, 2);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	//P_SetPitch(thing, newValue);
	thing->pitch = thing->old_pitch = newValue;
	return 0;
}

static int lib_pSetRoll(lua_State *L)
{
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t newValue = luaL_checkangle(L, 2);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	//P_SetRoll(thing, newValue);
	thing->roll = thing->old_roll = newValue;
	return 0;
}

static int lib_pSlideMove(lua_State *L)
{
	/*
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SlideMove(mo);
	*/
	LUA_UsageWarning(L, "FIXME: P_SlideMove needs updated to use result from P_TryMove");
	(void)L;
	return 0;
}

static int lib_pBounceMove(lua_State *L)
{
	/*
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_BounceMove(mo);
	*/
	LUA_UsageWarning(L, "FIXME: P_BounceMove needs updated to use result from P_TryMove");
	(void)L;
	return 0;
}

static int lib_pCheckSight(lua_State *L)
{
	mobj_t *t1 = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *t2 = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	//HUDSAFE?
	INLEVEL
	if (!t1 || !t2)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckSight(t1, t2));
	return 1;
}

static int lib_pTraceBlockingLines(lua_State *L)
{
	mobj_t *t1 = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *t2 = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	//HUDSAFE?
	INLEVEL
	if (!t1 || !t2)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_TraceBlockingLines(t1, t2));
	return 1;
}

static int lib_pCheckHoopPosition(lua_State *L)
{
	mobj_t *hoopthing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	fixed_t radius = luaL_checkfixed(L, 5);
	NOHUD
	INLEVEL
	if (!hoopthing)
		return LUA_ErrInvalid(L, "mobj_t");
	P_CheckHoopPosition(hoopthing, x, y, z, radius);
	return 0;
}

static int lib_pRadiusAttack(lua_State *L)
{
	mobj_t *spot = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t damagedist = luaL_checkfixed(L, 3);
	UINT8 damagetype = luaL_optinteger(L, 4, 0);
	boolean sightcheck = lua_opttrueboolean(L, 5);
	NOHUD
	INLEVEL
	if (!spot || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RadiusAttack(spot, source, damagedist, damagetype, sightcheck);
	return 0;
}

static int lib_pFloorzAtPos(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	fixed_t height = luaL_checkfixed(L, 4);
	//HUDSAFE
	INLEVEL
	lua_pushfixed(L, P_FloorzAtPos(x, y, z, height));
	return 1;
}

static int lib_pCeilingzAtPos(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	fixed_t height = luaL_checkfixed(L, 4);
	//HUDSAFE
	INLEVEL
	lua_pushfixed(L, P_CeilingzAtPos(x, y, z, height));
	return 1;
}

static int lib_pDoSpring(lua_State *L)
{
	mobj_t *spring = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *object = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!spring || !object)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_DoSpring(spring, object));
	return 1;
}

// P_INTER
////////////

static int lib_pDamageMobj(lua_State *L)
{
	mobj_t *target = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)), *inflictor = NULL, *source = NULL;
	INT32 damage;
	UINT8 damagetype;
	NOHUD
	INLEVEL
	if (!target)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	damage = (INT32)luaL_optinteger(L, 4, 1);
	damagetype = (UINT8)luaL_optinteger(L, 5, 0);
	lua_pushboolean(L, P_DamageMobj(target, inflictor, source, damage, damagetype));
	return 1;
}

static int lib_pKillMobj(lua_State *L)
{
	mobj_t *target = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)), *inflictor = NULL, *source = NULL;
	UINT8 damagetype;
	NOHUD
	INLEVEL
	if (!target)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	damagetype = (UINT8)luaL_optinteger(L, 4, 0);
	P_KillMobj(target, inflictor, source, damagetype);
	return 0;
}

static int lib_pPlayerRingBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 num_rings = (INT32)luaL_optinteger(L, 2, -1);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (num_rings == -1)
		num_rings = player->rings;
	P_PlayerRingBurst(player, num_rings);
	return 0;
}

static int lib_pPlayRinglossSound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *player = NULL;
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsPartyPlayer(player))
		P_PlayRinglossSound(source);
	return 0;
}

static int lib_pPlayDeathSound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *player = NULL;
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsPartyPlayer(player))
		P_PlayDeathSound(source);
	return 0;
}

static int lib_pPlayVictorySound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *player = NULL;
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsPartyPlayer(player))
		P_PlayVictorySound(source);
	return 0;
}

static int lib_pCanPickupItem(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT8 weapon = (UINT8)luaL_optinteger(L, 2, 0);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_CanPickupItem(player, weapon));
	return 1;
}

// P_SPEC
////////////

static int lib_pThrust(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = luaL_checkangle(L, 2);
	fixed_t move = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_Thrust(mo, angle, move);
	return 0;
}

static int lib_pSetMobjStateNF(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	statenum_t state = luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (state >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", state, NUMSTATES-1);
	if (mobj->player && state == S_NULL)
		return luaL_error(L, "Attempt to remove player mobj with S_NULL.");
	lua_pushboolean(L, P_SetMobjStateNF(mobj, state));
	return 1;
}

static int lib_pExplodeMissile(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_ExplodeMissile(mo);
	return 0;
}

static int lib_pMobjTouchingSectorSpecial(lua_State *L)
{
	mobj_t *mo = *((mobj_t**)luaL_checkudata(L, 1, META_MOBJ));
	INT32 section = (INT32)luaL_checkinteger(L, 2);
	INT32 number = (INT32)luaL_checkinteger(L, 3);
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_MobjTouchingSectorSpecial(mo, section, number), META_SECTOR);
	return 1;
}

static int lib_pMobjTouchingSectorSpecialFlag(lua_State *L)
{
	mobj_t *mo = *((mobj_t**)luaL_checkudata(L, 1, META_MOBJ));
	sectorspecialflags_t flag = (INT32)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_MobjTouchingSectorSpecialFlag(mo, flag), META_SECTOR);
	return 1;
}

static int lib_pPlayerTouchingSectorSpecial(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 section = (INT32)luaL_checkinteger(L, 2);
	INT32 number = (INT32)luaL_checkinteger(L, 3);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, P_PlayerTouchingSectorSpecial(player, section, number), META_SECTOR);
	return 1;
}

static int lib_pPlayerTouchingSectorSpecialFlag(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	sectorspecialflags_t flag = (INT32)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, P_PlayerTouchingSectorSpecialFlag(player, flag), META_SECTOR);
	return 1;
}

static int lib_pFindLowestFloorSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindLowestFloorSurrounding(sector));
	return 1;
}

static int lib_pFindHighestFloorSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindHighestFloorSurrounding(sector));
	return 1;
}

static int lib_pFindNextHighestFloor(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	fixed_t currentheight;
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	// defaults to floorheight of sector arg
	currentheight = (fixed_t)luaL_optinteger(L, 2, sector->floorheight);
	lua_pushfixed(L, P_FindNextHighestFloor(sector, currentheight));
	return 1;
}

static int lib_pFindNextLowestFloor(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	fixed_t currentheight;
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	// defaults to floorheight of sector arg
	currentheight = (fixed_t)luaL_optinteger(L, 2, sector->floorheight);
	lua_pushfixed(L, P_FindNextLowestFloor(sector, currentheight));
	return 1;
}

static int lib_pFindLowestCeilingSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindLowestCeilingSurrounding(sector));
	return 1;
}

static int lib_pFindHighestCeilingSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindHighestCeilingSurrounding(sector));
	return 1;
}

static int lib_pFindSpecialLineFromTag(lua_State *L)
{
	INT16 special = (INT16)luaL_checkinteger(L, 1);
	INT16 line = (INT16)luaL_checkinteger(L, 2);
	INT32 start = (INT32)luaL_optinteger(L, 3, -1);
	NOHUD
	INLEVEL
	lua_pushinteger(L, P_FindSpecialLineFromTag(special, line, start));
	return 1;
}

static int lib_pSwitchWeather(lua_State *L)
{
	INT32 weathernum = (INT32)luaL_checkinteger(L, 1);
	player_t *user = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2)) // if a player, setup weather for only the player, otherwise setup weather for all players
		user = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	if (!user) // global
		globalweather = weathernum;
	if (!user || P_IsPartyPlayer(user))
		P_SwitchWeather(weathernum);
	return 0;
}

static int lib_pLinedefExecute(lua_State *L)
{
	INT32 tag = (INT16)luaL_checkinteger(L, 1);
	mobj_t *actor = NULL;
	sector_t *caller = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		actor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		caller = *((sector_t **)luaL_checkudata(L, 3, META_SECTOR));
	P_LinedefExecute(tag, actor, caller);
	return 0;
}

static int lib_pSpawnLightningFlash(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	NOHUD
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	P_SpawnLightningFlash(sector);
	return 0;
}

static int lib_pFadeLight(lua_State *L)
{
	INT16 tag = (INT16)luaL_checkinteger(L, 1);
	INT32 destvalue = (INT32)luaL_checkinteger(L, 2);
	INT32 speed = (INT32)luaL_checkinteger(L, 3);
	boolean ticbased = lua_optboolean(L, 4);
	boolean force = lua_optboolean(L, 5);
	boolean relative = lua_optboolean(L, 6);
	NOHUD
	INLEVEL
	P_FadeLight(tag, destvalue, speed, ticbased, force, relative);
	return 0;
}

static int lib_pSetupLevelSky(lua_State *L)
{
	const char *skytexname = luaL_checkstring(L, 1);
	player_t *user = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2)) // if a player, setup sky for only the player, otherwise setup sky for all players
		user = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	if (!user) // global
		P_SetupLevelSky(skytexname, true);
	else if (P_IsPartyPlayer(user))
		P_SetupLevelSky(skytexname, false);
	return 0;
}

// Shhh, P_SetSkyboxMobj doesn't actually exist yet.
static int lib_pSetSkyboxMobj(lua_State *L)
{
	int n = lua_gettop(L);
	mobj_t *mo = NULL;
	player_t *user = NULL;
	int w = 0;

	NOHUD
	INLEVEL
	if (!lua_isnil(L,1)) // nil leaves mo as NULL to remove the skybox rendering.
	{
		mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)); // otherwise it is a skybox mobj.
		if (!mo)
			return LUA_ErrInvalid(L, "mobj_t");
	}

	if (n == 1)
		;
	else if (lua_isuserdata(L, 2))
		user = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	else if (lua_isnil(L, 2))
		w = 0;
	else if (lua_isboolean(L, 2))
	{
		if (lua_toboolean(L, 2))
			w = 1;
		else
			w = 0;
	}
	else
		w = luaL_optinteger(L, 2, 0);

	if (n > 2 && lua_isuserdata(L, 3))
	{
		user = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!user)
			return LUA_ErrInvalid(L, "player_t");
	}

	if (w > 1 || w < 0)
		return luaL_error(L, "skybox mobj index %d is out of range for P_SetSkyboxMobj argument #2 (expected 0 or 1)", w);

#if 0
	if (!user || P_IsPartyPlayer(user))
		skyboxmo[w] = mo;
#else
	CONS_Alert(CONS_WARNING, "TODO: P_SetSkyboxMobj is unimplemented\n");
#endif
	return 0;
}

// Shhh, neither does P_StartQuake.
static int lib_pStartQuake(lua_State *L)
{
	tic_t q_time = (tic_t)luaL_checkinteger(L, 1);
	fixed_t q_intensity = luaL_checkfixed(L, 2);
	fixed_t q_radius = luaL_optinteger(L, 3, 512*FRACUNIT);

	static mappoint_t q_epicenter = {0,0,0};
	boolean q_epicenter_set = false;

	NOHUD
	INLEVEL

	if (!lua_isnoneornil(L, 4))
	{
		mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 4, META_MOBJ));

		if (mobj != NULL)
		{
			q_epicenter.x = mobj->x;
			q_epicenter.y = mobj->y;
			q_epicenter.z = mobj->z;
			q_epicenter_set = true;
		}
		else
		{
			luaL_checktype(L, 4, LUA_TTABLE);

			lua_getfield(L, 4, "x");
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				lua_rawgeti(L, 4, 1);
			}
			if (!lua_isnil(L, -1))
				q_epicenter.x = luaL_checkinteger(L, -1);
			else
				q_epicenter.x = 0;
			lua_pop(L, 1);

			lua_getfield(L, 4, "y");
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				lua_rawgeti(L, 4, 2);
			}
			if (!lua_isnil(L, -1))
				q_epicenter.y = luaL_checkinteger(L, -1);
			else
				q_epicenter.y = 0;
			lua_pop(L, 1);

			lua_getfield(L, 4, "z");
			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				lua_rawgeti(L, 4, 3);
			}
			if (!lua_isnil(L, -1))
				q_epicenter.z = luaL_checkinteger(L, -1);
			else
				q_epicenter.z = 0;
			lua_pop(L, 1);

			q_epicenter_set = true;
		}
	}

	P_StartQuake(q_time, q_intensity, q_radius, q_epicenter_set ? &q_epicenter : NULL);
	return 0;
}

static int lib_evCrumbleChain(lua_State *L)
{
	sector_t *sec = NULL;
	ffloor_t *rover = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2))
	{
		if (!lua_isnil(L, 1))
		{
			sec = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
			if (!sec)
				return LUA_ErrInvalid(L, "sector_t");
		}
		rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	}
	else
		rover = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	EV_CrumbleChain(sec, rover);
	return 0;
}

static int lib_evStartCrumble(lua_State *L)
{
	sector_t *sec = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	boolean floating = lua_optboolean(L, 3);
	player_t *player = NULL;
	fixed_t origalpha;
	boolean crumblereturn = lua_optboolean(L, 6);
	NOHUD
	if (!sec)
		return LUA_ErrInvalid(L, "sector_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	if (!lua_isnone(L, 4) && lua_isuserdata(L, 4))
	{
		player = *((player_t **)luaL_checkudata(L, 4, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!lua_isnone(L,5))
		origalpha = luaL_checkfixed(L, 5);
	else
		origalpha = rover->alpha;
	lua_pushboolean(L, EV_StartCrumble(sec, rover, floating, player, origalpha, crumblereturn) != 0);
	return 0;
}

// P_SLOPES
////////////

static int lib_pGetZAt(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	//HUDSAFE
	if (lua_isnil(L, 1))
	{
		fixed_t z = luaL_checkfixed(L, 4);
		lua_pushfixed(L, P_GetZAt(NULL, x, y, z));
	}
	else
	{
		pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
		lua_pushfixed(L, P_GetSlopeZAt(slope, x, y));
	}

	return 1;
}

static int lib_pButteredSlope(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	P_ButteredSlope(mobj);
	return 0;
}

// R_DEFS
////////////

static int lib_rPointToAngle(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushangle(L, R_PointToAngle(x, y));
	return 1;
}

static int lib_rPointToAnglePlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	//HUDSAFE
	lua_pushangle(L, R_PointToAnglePlayer(player, x, y));
	return 1;
}

static int lib_rPointToAngle2(lua_State *L)
{
	fixed_t px2 = luaL_checkfixed(L, 1);
	fixed_t py2 = luaL_checkfixed(L, 2);
	fixed_t px1 = luaL_checkfixed(L, 3);
	fixed_t py1 = luaL_checkfixed(L, 4);
	//HUDSAFE
	lua_pushangle(L, R_PointToAngle2(px2, py2, px1, py1));
	return 1;
}

static int lib_rPointToDist(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, R_PointToDist(x, y));
	return 1;
}

static int lib_rPointToDist2(lua_State *L)
{
	fixed_t px2 = luaL_checkfixed(L, 1);
	fixed_t py2 = luaL_checkfixed(L, 2);
	fixed_t px1 = luaL_checkfixed(L, 3);
	fixed_t py1 = luaL_checkfixed(L, 4);
	//HUDSAFE
	lua_pushfixed(L, R_PointToDist2(px2, py2, px1, py1));
	return 1;
}

static int lib_rPointInSubsector(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	INLEVEL
	LUA_PushUserdata(L, R_PointInSubsector(x, y), META_SUBSECTOR);
	return 1;
}

static int lib_rPointInSubsectorOrNil(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	subsector_t *sub = R_PointInSubsectorOrNull(x, y);
	//HUDSAFE
	INLEVEL
	if (sub)
		LUA_PushUserdata(L, sub, META_SUBSECTOR);
	else
		lua_pushnil(L);
	return 1;
}

// R_THINGS
////////////

static int lib_rChar2Frame(lua_State *L)
{
	const char *p = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_Char2Frame(*p)); // first character only
	return 1;
}

static int lib_rFrame2Char(lua_State *L)
{
	UINT8 ch = (UINT8)luaL_checkinteger(L, 1);
	char c[2] = "";
	//HUDSAFE

	c[0] = R_Frame2Char(ch);
	c[1] = 0;

	lua_pushstring(L, c);
	lua_pushinteger(L, c[0]);
	return 2;
}

// R_SetPlayerSkin technically doesn't exist either, although it's basically just SetPlayerSkin and SetPlayerSkinByNum handled in one place for convenience
static int lib_rSetPlayerSkin(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 i = -1, j = -1;
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");

	j = (player-players);

	if (lua_isnoneornil(L, 2))
		return luaL_error(L, "argument #2 not given (expected number or string)");
	else if (lua_type(L, 2) == LUA_TNUMBER) // skin number
	{
		INT32 skincount = (demo.playback ? demo.numskins : numskins);
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= skincount)
			return luaL_error(L, "skin %d (argument #2) out of range (0 - %d)", i, skincount-1);
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 2);
		i = R_SkinAvailable(skinname);
		if (i == -1)
			return luaL_error(L, "skin %s (argument 2) is not loaded", skinname);
	}

	if (!R_SkinUsable(j, i, false))
		return luaL_error(L, "skin %d (argument 2) not usable - check with R_SkinUsable(player_t, skin) first.", i);
	SetPlayerSkinByNum(j, i);
	return 0;
}

static int lib_rSkinUsable(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 i = -1, j = -1;
	if (player)
		j = (player-players);
	else if (netgame || multiplayer)
		return luaL_error(L, "player_t (argument #1) must be provided in multiplayer games");
	if (lua_isnoneornil(L, 2))
		return luaL_error(L, "argument #2 not given (expected number or string)");
	else if (lua_type(L, 2) == LUA_TNUMBER) // skin number
	{
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= numskins)
			return luaL_error(L, "skin %d (argument #2) out of range (0 - %d)", i, numskins-1);
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 2);
		i = R_SkinAvailable(skinname);
		if (i == -1)
			return luaL_error(L, "skin %s (argument 2) is not loaded", skinname);
	}

	lua_pushboolean(L, R_SkinUsable(j, i, false));
	return 1;
}

// R_DATA
////////////

static int lib_rCheckTextureNumForName(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_CheckTextureNumForName(name));
	return 1;
}

static int lib_rTextureNumForName(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_TextureNumForName(name));
	return 1;
}

// R_DRAW
////////////
static int lib_rGetColorByName(lua_State *L)
{
	const char* colorname = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_GetColorByName(colorname));
	return 1;
}

static int lib_rGetSuperColorByName(lua_State *L)
{
	const char* colorname = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_GetSuperColorByName(colorname));
	return 1;
}

// Lua exclusive function, returns the name of a color from the SKINCOLOR_ constant.
// SKINCOLOR_GREEN > "Green" for example
static int lib_rGetNameByColor(lua_State *L)
{
	UINT16 colornum = (UINT16)luaL_checkinteger(L, 1);
	if (!colornum || colornum >= numskincolors)
		return luaL_error(L, "skincolor %d out of range (1 - %d).", colornum, numskincolors-1);
	lua_pushstring(L, skincolors[colornum].name);
	return 1;
}

// S_SOUND
////////////
static int GetValidSoundOrigin(lua_State *L, void **origin)
{
	const char *type;

	lua_settop(L, 1);
	type = GetUserdataUType(L);

	if (fasticmp(type, "mobj_t"))
	{
		*origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (!(*origin))
			return LUA_ErrInvalid(L, "mobj_t");
		return 1;
	}
	else if (fasticmp(type, "sector_t"))
	{
		*origin = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
		if (!(*origin))
			return LUA_ErrInvalid(L, "sector_t");

		*origin = &((sector_t *)(*origin))->soundorg;
		return 1;
	}

	return LUA_ErrInvalid(L, "mobj_t/sector_t");
}

static int lib_sStartSound(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	player_t *player = NULL;
	//NOHUD

	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);

	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return 0;
	if (!player || P_IsPartyPlayer(player))
	{
		if (hud_running || hook_cmd_running)
			origin = NULL;	// HUD rendering and CMD building startsound shouldn't have an origin, just remove it instead of having a retarded error.

		S_StartSound(origin, sound_id);
	}
	return 0;
}

static int lib_sStartSoundAtVolume(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	INT32 volume = (INT32)luaL_checkinteger(L, 3);
	player_t *player = NULL;
	//NOHUD

	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);
	if (!lua_isnone(L, 4) && lua_isuserdata(L, 4))
	{
		player = *((player_t **)luaL_checkudata(L, 4, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return LUA_ErrInvalid(L, "mobj_t/sector_t");

	if (!player || P_IsPartyPlayer(player))
		S_StartSoundAtVolume(origin, sound_id, volume);
	return 0;
}

static int lib_sStopSound(lua_State *L)
{
	void *origin = NULL;
	//NOHUD
	if (!GetValidSoundOrigin(L, &origin))
		return LUA_ErrInvalid(L, "mobj_t/sector_t");

	S_StopSound(origin);
	return 0;
}

static int lib_sStopSoundByID(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	//NOHUD

	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return LUA_ErrInvalid(L, "mobj_t/sector_t");

	S_StopSoundByID(origin, sound_id);
	return 0;
}

static int lib_sOriginPlaying(lua_State *L)
{
	void *origin = NULL;
	//NOHUD
	INLEVEL
	if (!GetValidSoundOrigin(L, &origin))
		return LUA_ErrInvalid(L, "mobj_t/sector_t");
	lua_pushboolean(L, S_OriginPlaying(origin));
	return 1;
}

static int lib_sIdPlaying(lua_State *L)
{
	sfxenum_t id = luaL_checkinteger(L, 1);
	//NOHUD
	if (id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", id, NUMSFX-1);
	lua_pushboolean(L, S_IdPlaying(id));
	return 1;
}

static int lib_sSoundPlaying(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t id = luaL_checkinteger(L, 2);
	//NOHUD
	INLEVEL
	if (id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", id, NUMSFX-1);
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return LUA_ErrInvalid(L, "mobj_t/sector_t");
	lua_pushboolean(L, S_SoundPlaying(origin, id));
	return 1;
}

// This doesn't really exist, but we're providing it as a handy netgame-safe wrapper for stuff that should be locally handled.

static int lib_sStartMusicCaption(lua_State *L)
{
	player_t *player = NULL;
	const char *caption = luaL_checkstring(L, 1);
	UINT16 lifespan = (UINT16)luaL_checkinteger(L, 2);
	//HUDSAFE
	//INLEVEL

	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}

	if (lifespan && (!player || P_IsPartyPlayer(player)))
	{
		strlcpy(S_sfx[sfx_None].caption, caption, sizeof(S_sfx[sfx_None].caption));
		S_StartCaption(sfx_None, -1, lifespan);
	}
	return 0;
}

static int lib_sShowMusicCredit(lua_State *L)
{
	player_t *player = NULL;
	//HUDSAFE
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsPartyPlayer(player))
		S_ShowMusicCredit();
	return 0;
}

// G_GAME
////////////

// Copypasted from lib_cvRegisterVar :]
static int lib_gAddGametype(lua_State *L)
{
	const char *k;
	lua_Integer i;

	gametype_t *newgametype = NULL;

	const char *gtname = NULL;
	const char *gtconst = NULL;
	const char *gppic = NULL;
	const char *gppicmini = NULL;
	UINT32 newgtrules = 0;
	UINT32 newgttol = 0;
	INT32 newgtpointlimit = 0;
	INT32 newgttimelimit = 0;
	UINT8 newgtinttype = 0;
	SINT8 newgtspeed = KARTSPEED_AUTO;
	INT16 j;

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1); // Clear out all other possible arguments, leaving only the first one.

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	// Ran out of gametype slots
	if (numgametypes == GT_LASTFREESLOT)
	{
		I_Error("Out of Gametype Freeslots while allocating \"%s\"\nLoad less addons to fix this.", gtname);
	}

#define FIELDERROR(f, e) luaL_error(L, "bad value for " LUA_QL(f) " in table passed to " LUA_QL("G_AddGametype") " (%s)", e);
#define TYPEERROR(f, t) FIELDERROR(f, va("%s expected, got %s", lua_typename(L, t), luaL_typename(L, -1)))

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		// stack: gametype table, key/index, value
		//               1            2        3
		i = 0;
		k = NULL;
		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2);
		else if (lua_isstring(L, 2))
			k = lua_tostring(L, 2);

		// Sorry, no gametype rules as key names.
		if (i == 1 || (k && fasticmp(k, "name"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("name", LUA_TSTRING)
			gtname = Z_StrDup(lua_tostring(L, 3));
		} else if (i == 2 || (k && fasticmp(k, "identifier"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("identifier", LUA_TSTRING)
			gtconst = Z_StrDup(lua_tostring(L, 3));
		} else if (i == 3 || (k && fasticmp(k, "rules"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("rules", LUA_TNUMBER)
			newgtrules = (UINT32)lua_tointeger(L, 3);
		} else if (i == 4 || (k && fasticmp(k, "typeoflevel"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("typeoflevel", LUA_TNUMBER)
			newgttol = (UINT32)lua_tointeger(L, 3);
		} else if (i == 5 || (k && fasticmp(k, "intermissiontype"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("intermissiontype", LUA_TNUMBER)
			newgtinttype = (int)lua_tointeger(L, 3);
		} else if (i == 6 || (k && fasticmp(k, "defaultpointlimit"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("defaultpointlimit", LUA_TNUMBER)
			newgtpointlimit = (INT32)lua_tointeger(L, 3);
		} else if (i == 7 || (k && fasticmp(k, "defaulttimelimit"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("defaulttimelimit", LUA_TNUMBER)
			newgttimelimit = (INT32)lua_tointeger(L, 3);
		} else if (i == 8 || (k && fasticmp(k, "gppic"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("gppic", LUA_TSTRING)
			gppic = lua_tostring(L, 3);
		} else if (i == 9 || (k && fasticmp(k, "gppicmini"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("gppicmini", LUA_TSTRING)
			gppicmini = lua_tostring(L, 3);
		} else if (i == 10 || (k && fasticmp(k, "speed"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("speed", LUA_TNUMBER)
			newgtspeed = (UINT32)lua_tointeger(L, 3);
			if (newgtspeed < KARTSPEED_AUTO || newgtspeed > KARTSPEED_HARD)
			{
				newgtspeed = KARTSPEED_AUTO;
			}
		}
		lua_pop(L, 1);
	}

#undef FIELDERROR
#undef TYPEERROR

	if (gtname == NULL)
		return luaL_error(L, "Custom gametype must have a name");

	if (strlen(gtname) >= MAXGAMETYPELENGTH)
		return luaL_error(L, "Custom gametype \"%s\"'s name must be %d long at most", gtname, MAXGAMETYPELENGTH-1);

	for (j = 0; j < numgametypes; j++)
		if (!strcmp(gtname, gametypes[j]->name))
			break;

	if (j < numgametypes)
		return luaL_error(L, "Custom gametype \"%s\"'s name is already in use", gtname);

	// pop gametype table
	lua_pop(L, 1);

	// Add the new gametype
	newgametype = Z_Calloc(sizeof (gametype_t), PU_STATIC, NULL);
	if (!newgametype)
	{
		I_Error("Out of memory allocating gametype \"%s\"", gtname);
	}

	if (gtconst == NULL)
		gtconst = gtname;

	newgametype->name = gtname;
	newgametype->rules = newgtrules;
	newgametype->constant = G_PrepareGametypeConstant(gtconst);
	newgametype->tol = newgttol;
	newgametype->intermission = newgtinttype;
	newgametype->pointlimit = newgtpointlimit;
	newgametype->timelimit = newgttimelimit;
	newgametype->speed = newgtspeed;

	if (gppic != NULL)
	{
		// Calloc means only set if valid
		strlcpy(newgametype->gppic, gppic, 9);
	}

	if (gppicmini != NULL)
	{
		// Calloc means only set if valid
		strlcpy(newgametype->gppicmini, gppicmini, 9);
	}

	gametypes[numgametypes++] = newgametype;

	// done
	CONS_Printf("Added gametype %s\n", gtname);
	return 0;
}

static int Lcheckmapnumber (lua_State *L, int idx, const char *fun)
{
	if (ISINLEVEL)
		return luaL_optinteger(L, idx, gamemap);
	else
	{
		if (lua_isnoneornil(L, idx))
		{
			return luaL_error(L,
					"%s can only be used without a parameter while in a level.",
					fun
			);
		}
		else
			return luaL_checkinteger(L, idx);
	}
}

static int lib_gBuildMapName(lua_State *L)
{
	INT32 map = Lcheckmapnumber(L, 1, "G_BuildMapName");
	//HUDSAFE
	lua_pushstring(L, G_BuildMapName(map));
	return 1;
}

static int lib_gBuildMapTitle(lua_State *L)
{
	INT32 map = Lcheckmapnumber(L, 1, "G_BuildMapTitle");
	char *name;
	if (map < 1 || map > nummapheaders)
	{
		return luaL_error(L,
				"map ID %d out of range (1 - %d)",
				map,
				nummapheaders
		);
	}
	name = G_BuildMapTitle(map);
	lua_pushstring(L, name);
	Z_Free(name);
	return 1;
}

static void
Lpushdim (lua_State *L, int c, struct searchdim *v)
{
	int i;
	lua_createtable(L, c, 0);/* I guess narr is numeric indices??? */
	for (i = 0; i < c; ++i)
	{
		lua_createtable(L, 0, 2);/* and hashed indices (field)... */
			lua_pushnumber(L, v[i].pos);
			lua_setfield(L, -2, "pos");

			lua_pushnumber(L, v[i].siz);
			lua_setfield(L, -2, "siz");
		lua_rawseti(L, -2, 1 + i);
	}
}

/*
I decided to make this return a table because userdata
is scary and tables let the user set their own fields.
*/
/*
Returns:

[1] => map number
[2] => map title
[3] => search frequency table

The frequency table is unsorted. It has the following format:

{
	['mapnum'],

	['matchd'] => matches in map title string
	['keywhd'] => matches in map keywords

	The above two tables have the following format:

	{
		['pos'] => offset from start of string
		['siz'] => length of match
	}...

	['total'] => the total matches
}...
*/
static int lib_gFindMap(lua_State *L)
{
	const char *query = luaL_checkstring(L, 1);

	INT32 map;
	char *realname;
	INT32 frc;
	mapsearchfreq_t *frv;

	INT32 i;

	map = G_FindMap(query, &realname, &frv, &frc);

	lua_settop(L, 0);

	lua_pushnumber(L, map);
	lua_pushstring(L, realname);

	lua_createtable(L, frc, 0);
	for (i = 0; i < frc; ++i)
	{
		lua_createtable(L, 0, 4);
			lua_pushnumber(L, frv[i].mapnum);
			lua_setfield(L, -2, "mapnum");

			Lpushdim(L, frv[i].matchc, frv[i].matchd);
			lua_setfield(L, -2, "matchd");

			Lpushdim(L, frv[i].keywhc, frv[i].keywhd);
			lua_setfield(L, -2, "keywhd");

			lua_pushnumber(L, frv[i].total);
			lua_setfield(L, -2, "total");
		lua_rawseti(L, -2, 1 + i);
	}

	G_FreeMapSearch(frv, frc);
	Z_Free(realname);

	return 3;
}

/*
Returns:

[1] => map number
[2] => map title
*/
static int lib_gFindMapByNameOrCode(lua_State *L)
{
	const char *query = luaL_checkstring(L, 1);
	INT32 map;
	char *realname;
	map = G_FindMapByNameOrCode(query, &realname);
	lua_pushnumber(L, map);
	if (map)
	{
		lua_pushstring(L, realname);
		Z_Free(realname);
		return 2;
	}
	else
		return 1;
}

static int lib_gDoReborn(lua_State *L)
{
	INT32 playernum = luaL_checkinteger(L, 1);
	NOHUD
	INLEVEL
	if (playernum >= MAXPLAYERS)
		return luaL_error(L, "playernum %d out of range (0 - %d)", playernum, MAXPLAYERS-1);
	G_DoReborn(playernum);
	return 0;
}

// Another Lua function that doesn't actually exist!
// Sets nextmapoverride & skipstats without instantly ending the level, for instances where other sources should be exiting the level, like normal signposts.
static int lib_gSetCustomExitVars(lua_State *L)
{
	int n = lua_gettop(L); // Num arguments
	NOHUD
	INLEVEL

	// LUA EXTENSION: Custom exit like support
	// Supported:
	//	G_SetCustomExitVars();			[reset to defaults]
	//	G_SetCustomExitVars(int)		[nextmap override only]
	//	G_SetCustomExitVars(nil, int)	[skipstats only]
	//	G_SetCustomExitVars(int, int)	[both of the above]

	nextmapoverride = 0;
	skipstats = 0;

	if (n >= 1)
	{
		nextmapoverride = (INT16)luaL_optinteger(L, 1, 0);
		skipstats = (INT16)luaL_optinteger(L, 2, 0);
	}

	return 0;
}

static int lib_gExitLevel(lua_State *L)
{
	int n = lua_gettop(L); // Num arguments
	NOHUD
	// Moved this bit to G_SetCustomExitVars
	if (n >= 1) // Don't run the reset to defaults option
		lib_gSetCustomExitVars(L);
	G_BeginLevelExit();
	G_FinishExitLevel();
	return 0;
}

static int lib_gGametypeUsesLives(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeUsesLives());
	return 1;
}

static int lib_gGametypeAllowsRetrying(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeAllowsRetrying());
	return 1;
}

static int lib_gGametypeHasTeams(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeHasTeams());
	return 1;
}

static int lib_gGametypeHasSpectators(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeHasSpectators());
	return 1;
}

static int lib_gTicsToHours(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToHours(rtic));
	return 1;
}

static int lib_gTicsToMinutes(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	boolean rfull = lua_optboolean(L, 2);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToMinutes(rtic, rfull));
	return 1;
}

static int lib_gTicsToSeconds(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToSeconds(rtic));
	return 1;
}

static int lib_gTicsToCentiseconds(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToCentiseconds(rtic));
	return 1;
}

static int lib_gTicsToMilliseconds(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToMilliseconds(rtic));
	return 1;
}

// K_HUD
////////////

static int lib_kAddMessage(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);
	boolean interrupt = lua_optboolean(L, 2);
	boolean persist = lua_optboolean(L, 3);
	INLEVEL
	if (msg == NULL)
		return luaL_error(L, "argument #1 not given (expected string)");
	K_AddMessage(msg, interrupt, persist);	
	return 0;
}

static int lib_kAddMessageForPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	const char *msg = luaL_checkstring(L, 2);
	boolean interrupt = lua_optboolean(L, 3);
	boolean persist = lua_optboolean(L, 4);
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (msg == NULL)
		return luaL_error(L, "argument #2 not given (expected string)");
	K_AddMessageForPlayer(player, msg, interrupt, persist);
	return 0;
}

static int lib_kClearPersistentMessages(lua_State *L)
{
	INLEVEL
	K_ClearPersistentMessages();
	lua_pushnil(L);
	return 0;
}

static int lib_kClearPersistentMessageForPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_ClearPersistentMessageForPlayer(player);
	return 0;
}

// K_KART
////////////

// Seriously, why weren't those exposed before?
static int lib_kAttackSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayAttackTaunt: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	K_PlayAttackTaunt(mobj);
	return 0;
}

static int lib_kBoostSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayBoostTaunt: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	K_PlayBoostTaunt(mobj);
	return 0;
}

static int lib_kOvertakeSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayOvertakeSound: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	K_PlayOvertakeSound(mobj);
	return 0;
}

static int lib_kPainSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *other = NULL;
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayPainSound: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		other = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	K_PlayPainSound(mobj, other);
	return 0;
}

static int lib_kHitEmSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *other = NULL;
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayHitEmSound: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		other = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	K_PlayHitEmSound(mobj, other);
	return 0;
}

static int lib_kTryHurtSoundExchange(lua_State *L)
{
	mobj_t *victim = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *attacker = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	if (!victim->player)
		return luaL_error(L, "K_TryHurtSoundExchange: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	K_TryHurtSoundExchange(victim, attacker);
	return 0;
}

static int lib_kGloatSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayPowerGloatSound: mobj_t isn't a player object.");	//Nothing bad would happen if we let it run the func, but telling why it ain't doing anything is helpful.
	K_PlayPowerGloatSound(mobj);
	return 0;
}

static int lib_kLossSound(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));	// let's require a mobj for consistency with the other functions
	sfxenum_t sfx_id;
	NOHUD
	if (!mobj->player)
		return luaL_error(L, "K_PlayLossSound: mobj_t isn't a player object.");

	sfx_id = ((skin_t *)mobj->skin)->soundsid[S_sfx[sfx_klose].skinsound];
	S_StartSound(mobj, sfx_id);
	return 0;
}

// Note: Pain, Death and Victory are already exposed.

static int lib_kIsPlayerLosing(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, K_IsPlayerLosing(player));
	return 1;
}

static int lib_kIsPlayerWanted(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, K_IsPlayerWanted(player));
	return 1;
}

static int lib_kKartBouncing(lua_State *L)
{
	mobj_t *mobj1 = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *mobj2 = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	if (!mobj1)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!mobj2)
		return LUA_ErrInvalid(L, "mobj_t");
	K_KartBouncing(mobj1, mobj2);
	return 0;
}

static int lib_kMatchGenericExtraFlags(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *master = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!master)
		return LUA_ErrInvalid(L, "mobj_t");
	K_MatchGenericExtraFlags(mo, master);
	return 0;
}

static int lib_kDoInstashield(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_DoInstashield(player);
	return 0;
}

static int lib_kSpawnBattlePoints(lua_State *L)
{
	player_t *source = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	player_t *victim = NULL;
	UINT8 amount = (UINT8)luaL_checkinteger(L, 3);
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "player_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		victim = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	K_SpawnBattlePoints(source, victim, amount);
	return 0;
}

static int lib_kSpinPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *inflictor = NULL;
	mobj_t *source = NULL;
	INT32 type = (INT32)luaL_optinteger(L, 3, 0);
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	K_SpinPlayer(player, inflictor, source, type);
	return 0;
}

static int lib_kTumblePlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *inflictor = NULL;
	mobj_t *source = NULL;
	boolean soften = false;
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	soften = lua_optboolean(L, 4);
	K_TumblePlayer(player, inflictor, source, soften);
	return 0;
}

static int lib_kStumblePlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_StumblePlayer(player);
	return 0;
}

static int lib_kExplodePlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *source = NULL;
	mobj_t *inflictor = NULL;
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	lua_pushinteger(L, K_ExplodePlayer(player, inflictor, source));
	return 1;
}

static int lib_kTakeBumpersFromPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	player_t *victim = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	UINT8 amount = (UINT8)luaL_optinteger(L, 3, 1);
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!victim)
		return LUA_ErrInvalid(L, "player_t");
	K_TakeBumpersFromPlayer(player, victim, amount);
	return 0;
}

static int lib_kSpawnMineExplosion(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	UINT8 color = (UINT8)luaL_optinteger(L, 2, SKINCOLOR_KETCHUP);
	tic_t delay = (tic_t)luaL_optinteger(L, 3, 0);
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	K_SpawnMineExplosion(source, color, delay);
	return 0;
}

static int lib_kSpawnBoostTrail(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_SpawnBoostTrail(player);
	return 0;
}

static int lib_kSpawnSparkleTrail(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	K_SpawnSparkleTrail(mo);
	return 0;
}

static int lib_kSpawnWipeoutTrail(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	K_SpawnWipeoutTrail(mo);
	return 0;
}

static int lib_kDriftDustHandling(lua_State *L)
{
	mobj_t *spawner = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!spawner)
		return LUA_ErrInvalid(L, "mobj_t");
	K_DriftDustHandling(spawner);
	return 0;
}

static int lib_kDoSneaker(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 type = luaL_optinteger(L, 2, 0);
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_DoSneaker(player, type);
	return 0;
}

static int lib_kDoPogoSpring(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t vertispeed = (fixed_t)luaL_optinteger(L, 2, 0);
	UINT8 sound = (UINT8)luaL_optinteger(L, 3, 1);
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	K_DoPogoSpring(mo, vertispeed, sound);
	return 0;
}

static int lib_kKillBananaChain(lua_State *L)
{
	mobj_t *banana = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *inflictor = NULL;
	mobj_t *source = NULL;
	NOHUD
	if (!banana)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	K_KillBananaChain(banana, inflictor, source);
	return 0;
}

static int lib_kRepairOrbitChain(lua_State *L)
{
	mobj_t *orbit = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!orbit)
		return LUA_ErrInvalid(L, "mobj_t");
	K_RepairOrbitChain(orbit);
	return 0;
}

static int lib_kFindJawzTarget(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *source = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	//HUDSAFE
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!source)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, K_FindJawzTarget(actor, source, ANGLE_45), META_PLAYER);
	return 1;
}

static int lib_kGetKartDriftSparkValue(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, K_GetKartDriftSparkValue(player));
	return 1;
}

static int lib_kKartUpdatePosition(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_KartUpdatePosition(player);
	return 0;
}

static int lib_kPopPlayerShield(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_PopPlayerShield(player);
	return 0;
}

static int lib_kDropHnextList(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_DropHnextList(player);
	return 0;
}

static int lib_kDropItems(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_DropItems(player);
	return 0;
}

static int lib_kStripItems(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_StripItems(player);
	return 0;
}

static int lib_kStripOther(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_StripOther(player);
	return 0;
}

static int lib_kMomentumToFacing(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_MomentumToFacing(player);
	return 0;
}

static int lib_kGetKartSpeed(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean doboostpower = lua_optboolean(L, 2);
	boolean dorubberbanding = lua_optboolean(L, 3);
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushfixed(L, K_GetKartSpeed(player, doboostpower, dorubberbanding));
	return 1;
}

static int lib_kGetKartAccel(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushfixed(L, K_GetKartAccel(player));
	return 1;
}

static int lib_kGetKartFlashing(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, K_GetKartFlashing(player));
	return 1;
}

static int lib_kGetItemPatch(lua_State *L)
{
	UINT8 item = (UINT8)luaL_optinteger(L, 1, KITEM_NONE);
	boolean tiny = lua_optboolean(L, 2);
	//HUDSAFE
	lua_pushstring(L, K_GetItemPatch(item, tiny));
	return 1;
}

static int lib_kGetCollideAngle(lua_State *L)
{
	mobj_t *t1 = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *t2 = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	//HUDSAFE
	if (!t1)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!t2)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushinteger(L, K_GetCollideAngle(t1, t2));
	return 1;
}

static int lib_kAddHitLag(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	tic_t tics = (tic_t)luaL_checkinteger(L, 2);
	boolean fromdamage = lua_opttrueboolean(L, 3);
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	K_AddHitLag(mo, tics, fromdamage);
	return 0;
}


static int lib_kPowerUpRemaining(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	kartitems_t powerup = luaL_checkinteger(L, 2);
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, K_PowerUpRemaining(player, powerup));
	return 1;
}

static int lib_kGivePowerUp(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	kartitems_t powerup = luaL_checkinteger(L, 2);
	tic_t time = (tic_t)luaL_checkinteger(L, 3);
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_GivePowerUp(player, powerup, time);
	return 0;
}

static int lib_kDropPowerUps(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	K_DropPowerUps(player);
	return 0;
}


static int lib_kInitBossHealthBar(lua_State *L)
{
	const char *enemyname = luaL_checkstring(L, 1);
	const char *subtitle = luaL_checkstring(L, 2);
	sfxenum_t titlesound = luaL_checkinteger(L, 3);
	fixed_t pinchmagnitude = luaL_checkfixed(L, 4);
	UINT8 divisions = (UINT8)luaL_checkinteger(L, 5);
	NOHUD
	K_InitBossHealthBar(enemyname, subtitle, titlesound, pinchmagnitude, divisions);
	return 0;
}

static int lib_kUpdateBossHealthBar(lua_State *L)
{
	fixed_t magnitude = luaL_checkfixed(L, 1);
	tic_t jitterlen = (tic_t)luaL_checkinteger(L, 2);
	NOHUD
	K_UpdateBossHealthBar(magnitude, jitterlen);
	return 0;
}

static int lib_kDeclareWeakspot(lua_State *L)
{
	mobj_t *spot = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	spottype_t spottype = luaL_checkinteger(L, 2);
	UINT16 color = luaL_checkinteger(L, 3);
	boolean minimap = lua_optboolean(L, 4);
	NOHUD
	if (!spot)
		return LUA_ErrInvalid(L, "mobj_t");
	K_DeclareWeakspot(spot, spottype, color, minimap);
	return 0;
}

static int lib_vsGetArena(lua_State *L)
{
	INT32 bossindex = luaL_checkinteger(L, 1);
	//HUDSAFE
	LUA_PushUserdata(L, VS_GetArena(bossindex), META_MOBJ);
	return 1;
}

static int lib_vsPredictAroundArena(lua_State *L)
{
	mobj_t *arena = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *movingobject = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t magnitude = luaL_checkfixed(L, 3);
	fixed_t mompoint = luaL_checkangle(L, 4);
	fixed_t radiussubtract = luaL_checkfixed(L, 5);
	boolean forcegoaround = lua_optboolean(L, 6);
	fixed_t radiusdeltafactor = luaL_optinteger(L, 7, FRACUNIT); //optfixed?
	NOHUD
	if (!arena || !movingobject)
		return LUA_ErrInvalid(L, "mobj_t");

	fixed_t *result = VS_PredictAroundArena(arena, movingobject, magnitude, mompoint, radiussubtract, forcegoaround, radiusdeltafactor);

	lua_pushfixed(L, result[0]);
	lua_pushfixed(L, result[1]);
	return 2;
}

static int lib_vsRandomPointOnArena(lua_State *L)
{
	mobj_t *arena = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t radiussubtract = luaL_checkfixed(L, 2);
	NOHUD
	if (!arena)
		return LUA_ErrInvalid(L, "mobj_t");

	fixed_t *result = VS_RandomPointOnArena(arena, radiussubtract);

	lua_pushfixed(L, result[0]);
	lua_pushfixed(L, result[1]);
	return 2;
}

static int lib_getTimeMicros(lua_State *L)
{
	lua_pushinteger(L, I_GetPreciseTime() / (I_GetPrecisePrecision() / 1000000));
	return 1;
}

static int lib_startTitlecardCecho(lua_State *L)
{
	player_t *player = lua_isnil(L, 1) ? NULL : *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	const char *str = luaL_checkstring(L, 2);
	boolean interrupt = lua_optboolean(L, 3);

	HU_DoTitlecardCEcho(player, str, interrupt);

	return 1;
}

static luaL_Reg lib[] = {
	{"print", lib_print},
	{"chatprint", lib_chatprint},
	{"chatprintf", lib_chatprintf},
	{"userdataType", lib_userdataType},
	{"registerMetatable", lib_registerMetatable},
	{"userdataMetatable", lib_userdataMetatable},
	{"IsPlayerAdmin", lib_isPlayerAdmin},
	{"reserveLuabanks", lib_reserveLuabanks},

	// m_random
	{"P_RandomFixed",lib_pRandomFixed},
	{"P_RandomByte",lib_pRandomByte},
	{"P_RandomKey",lib_pRandomKey},
	{"P_RandomRange",lib_pRandomRange},
	{"P_SignedRandom",lib_pSignedRandom}, // MACRO
	{"P_RandomChance",lib_pRandomChance}, // MACRO

	// p_maputil
	{"P_AproxDistance",lib_pAproxDistance},
	{"P_ClosestPointOnLine",lib_pClosestPointOnLine},
	{"P_PointOnLineSide",lib_pPointOnLineSide},

	// p_enemy
	{"P_CheckMeleeRange", lib_pCheckMeleeRange},
	{"P_JetbCheckMeleeRange", lib_pJetbCheckMeleeRange},
	{"P_FaceStabCheckMeleeRange", lib_pFaceStabCheckMeleeRange},
	{"P_SkimCheckMeleeRange", lib_pSkimCheckMeleeRange},
	{"P_CheckMissileRange", lib_pCheckMissileRange},
	{"P_NewChaseDir", lib_pNewChaseDir},
	{"P_LookForPlayers", lib_pLookForPlayers},

	// p_mobj
	// don't add P_SetMobjState or P_SetPlayerMobjState, use "mobj.state = S_NEWSTATE" instead.
	{"P_SpawnMobj",lib_pSpawnMobj},
	{"P_SpawnMobjFromMobj",lib_pSpawnMobjFromMobj},
	{"P_SpawnMobjFromMobjUnscaled",lib_pSpawnMobjFromMobjUnscaled},
	{"P_RemoveMobj",lib_pRemoveMobj},
	{"P_IsValidSprite2", lib_pIsValidSprite2},
	{"P_SpawnLockOn", lib_pSpawnLockOn},
	{"P_SpawnMissile",lib_pSpawnMissile},
	{"P_SpawnXYZMissile",lib_pSpawnXYZMissile},
	{"P_SpawnPointMissile",lib_pSpawnPointMissile},
	{"P_SpawnAlteredDirectionMissile",lib_pSpawnAlteredDirectionMissile},
	{"P_ColorTeamMissile",lib_pColorTeamMissile},
	{"P_SPMAngle",lib_pSPMAngle},
	{"P_SpawnPlayerMissile",lib_pSpawnPlayerMissile},
	{"P_MobjFlip",lib_pMobjFlip},
	{"P_GetMobjGravity",lib_pGetMobjGravity},
	{"P_FlashPal",lib_pFlashPal},
	{"P_GetClosestAxis",lib_pGetClosestAxis},
	{"P_SpawnParaloop",lib_pSpawnParaloop},
	{"P_BossTargetPlayer",lib_pBossTargetPlayer},
	{"P_SupermanLook4Players",lib_pSupermanLook4Players},
	{"P_SetScale",lib_pSetScale},
	{"P_InsideANonSolidFFloor",lib_pInsideANonSolidFFloor},
	{"P_CheckDeathPitCollide",lib_pCheckDeathPitCollide},
	{"P_CheckSolidLava",lib_pCheckSolidLava},
	{"P_MaceRotate",lib_pMaceRotate},
	{"P_CreateFloorSpriteSlope",lib_pCreateFloorSpriteSlope},
	{"P_RemoveFloorSpriteSlope",lib_pRemoveFloorSpriteSlope},
	{"P_RailThinker",lib_pRailThinker},
	{"P_XYMovement",lib_pXYMovement},
	{"P_RingXYMovement",lib_pRingXYMovement},
	{"P_SceneryXYMovement",lib_pSceneryXYMovement},
	{"P_ZMovement",lib_pZMovement},
	{"P_RingZMovement",lib_pRingZMovement},
	{"P_SceneryZMovement",lib_pSceneryZMovement},
	{"P_PlayerZMovement",lib_pPlayerZMovement},

	// p_user
	{"P_AddPlayerScore",lib_pAddPlayerScore},
	{"P_PlayerInPain",lib_pPlayerInPain},
	{"P_ResetPlayer",lib_pResetPlayer},
	{"P_PlayerFullbright",lib_pPlayerFullbright},
	{"P_IsObjectInGoop",lib_pIsObjectInGoop},
	{"P_IsObjectOnGround",lib_pIsObjectOnGround},
	{"P_InQuicksand",lib_pInQuicksand},
	{"P_SetObjectMomZ",lib_pSetObjectMomZ},
	{"P_SpawnGhostMobj",lib_pSpawnGhostMobj},
	{"P_SpawnFakeShadow",lib_pSpawnFakeShadow},
	{"P_GivePlayerRings",lib_pGivePlayerRings},
	{"P_GivePlayerLives",lib_pGivePlayerLives},
	{"P_MovePlayer",lib_pMovePlayer},
	{"P_DoPlayerExit",lib_pDoPlayerExit},
	{"P_InstaThrust",lib_pInstaThrust},
	{"P_ReturnThrustX",lib_pReturnThrustX},
	{"P_ReturnThrustY",lib_pReturnThrustY},
	{"P_NukeEnemies",lib_pNukeEnemies},
	{"K_GetEffectiveFollowerColor",lib_pGetEffectiveFollowerColor},

	// p_map
	{"P_CheckPosition",lib_pCheckPosition},
	{"P_TryMove",lib_pTryMove},
	{"P_Move",lib_pMove},
	{"P_TeleportMove",lib_pTeleportMove},
	{"P_SetOrigin",lib_pSetOrigin},
	{"P_MoveOrigin",lib_pMoveOrigin},
	{"P_SetAngle",lib_pSetAngle},
	{"P_SetPitch",lib_pSetPitch},
	{"P_SetRoll",lib_pSetRoll},
	{"P_SlideMove",lib_pSlideMove},
	{"P_BounceMove",lib_pBounceMove},
	{"P_CheckSight", lib_pCheckSight},
	{"P_TraceBlockingLines", lib_pTraceBlockingLines},
	{"P_CheckHoopPosition",lib_pCheckHoopPosition},
	{"P_RadiusAttack",lib_pRadiusAttack},
	{"P_FloorzAtPos",lib_pFloorzAtPos},
	{"P_CeilingzAtPos",lib_pCeilingzAtPos},
	{"P_DoSpring",lib_pDoSpring},

	// p_inter
	{"P_DamageMobj",lib_pDamageMobj},
	{"P_KillMobj",lib_pKillMobj},
	{"P_PlayerRingBurst",lib_pPlayerRingBurst},
	{"P_PlayRinglossSound",lib_pPlayRinglossSound},
	{"P_PlayDeathSound",lib_pPlayDeathSound},
	{"P_PlayVictorySound",lib_pPlayVictorySound},
	{"P_CanPickupItem",lib_pCanPickupItem},

	// p_spec
	{"P_Thrust",lib_pThrust},
	{"P_SetMobjStateNF",lib_pSetMobjStateNF},
	{"P_ExplodeMissile",lib_pExplodeMissile},
	{"P_MobjTouchingSectorSpecial",lib_pMobjTouchingSectorSpecial},
	{"P_MobjTouchingSectorSpecialFlag",lib_pMobjTouchingSectorSpecialFlag},
	{"P_PlayerTouchingSectorSpecial",lib_pPlayerTouchingSectorSpecial},
	{"P_PlayerTouchingSectorSpecialFlag",lib_pPlayerTouchingSectorSpecialFlag},
	{"P_FindLowestFloorSurrounding",lib_pFindLowestFloorSurrounding},
	{"P_FindHighestFloorSurrounding",lib_pFindHighestFloorSurrounding},
	{"P_FindNextHighestFloor",lib_pFindNextHighestFloor},
	{"P_FindNextLowestFloor",lib_pFindNextLowestFloor},
	{"P_FindLowestCeilingSurrounding",lib_pFindLowestCeilingSurrounding},
	{"P_FindHighestCeilingSurrounding",lib_pFindHighestCeilingSurrounding},
	{"P_FindSpecialLineFromTag",lib_pFindSpecialLineFromTag},
	{"P_SwitchWeather",lib_pSwitchWeather},
	{"P_LinedefExecute",lib_pLinedefExecute},
	{"P_SpawnLightningFlash",lib_pSpawnLightningFlash},
	{"P_FadeLight",lib_pFadeLight},
	{"P_SetupLevelSky",lib_pSetupLevelSky},
	{"P_SetSkyboxMobj",lib_pSetSkyboxMobj},
	{"P_StartQuake",lib_pStartQuake},
	{"EV_CrumbleChain",lib_evCrumbleChain},
	{"EV_StartCrumble",lib_evStartCrumble},

	// p_slopes
	{"P_GetZAt",lib_pGetZAt},
	{"P_ButteredSlope",lib_pButteredSlope},

	// r_defs
	{"R_PointToAngle",lib_rPointToAngle},
	{"R_PointToAnglePlayer", lib_rPointToAnglePlayer},
	{"R_PointToAngle2",lib_rPointToAngle2},
	{"R_PointToDist",lib_rPointToDist},
	{"R_PointToDist2",lib_rPointToDist2},
	{"R_PointInSubsector",lib_rPointInSubsector},
	{"R_PointInSubsectorOrNil",lib_rPointInSubsectorOrNil},

	// r_things (sprite)
	{"R_Char2Frame",lib_rChar2Frame},
	{"R_Frame2Char",lib_rFrame2Char},
	{"R_SetPlayerSkin",lib_rSetPlayerSkin},
	{"R_SkinUsable",lib_rSkinUsable},

	// r_data
	{"R_CheckTextureNumForName",lib_rCheckTextureNumForName},
	{"R_TextureNumForName",lib_rTextureNumForName},

	// r_draw
	{"R_GetColorByName", lib_rGetColorByName},
	{"R_GetSuperColorByName", lib_rGetSuperColorByName},
	{"R_GetNameByColor", lib_rGetNameByColor},

	// s_sound
	{"S_StartSound",lib_sStartSound},
	{"S_StartSoundAtVolume",lib_sStartSoundAtVolume},
	{"S_StopSound",lib_sStopSound},
	{"S_StopSoundByID",lib_sStopSoundByID},
	{"S_OriginPlaying",lib_sOriginPlaying},
	{"S_IdPlaying",lib_sIdPlaying},
	{"S_SoundPlaying",lib_sSoundPlaying},
	{"S_StartMusicCaption", lib_sStartMusicCaption},
	{"S_ShowMusicCredit",lib_sShowMusicCredit},

	// g_game
	{"G_AddGametype", lib_gAddGametype},
	{"G_BuildMapName",lib_gBuildMapName},
	{"G_BuildMapTitle",lib_gBuildMapTitle},
	{"G_FindMap",lib_gFindMap},
	{"G_FindMapByNameOrCode",lib_gFindMapByNameOrCode},
	{"G_DoReborn",lib_gDoReborn},
	{"G_SetCustomExitVars",lib_gSetCustomExitVars},
	{"G_ExitLevel",lib_gExitLevel},
	{"G_GametypeUsesLives",lib_gGametypeUsesLives},
	{"G_GametypeAllowsRetrying",lib_gGametypeAllowsRetrying},
	{"G_GametypeHasTeams",lib_gGametypeHasTeams},
	{"G_GametypeHasSpectators",lib_gGametypeHasSpectators},
	{"G_TicsToHours",lib_gTicsToHours},
	{"G_TicsToMinutes",lib_gTicsToMinutes},
	{"G_TicsToSeconds",lib_gTicsToSeconds},
	{"G_TicsToCentiseconds",lib_gTicsToCentiseconds},
	{"G_TicsToMilliseconds",lib_gTicsToMilliseconds},
	{"getTimeMicros",lib_getTimeMicros},

	// k_hud
	{"K_AddMessage", lib_kAddMessage},
	{"K_AddMessageForPlayer", lib_kAddMessageForPlayer},
	{"K_ClearPersistentMessages", lib_kClearPersistentMessages},
	{"K_ClearPersistentMessageForPlayer", lib_kClearPersistentMessageForPlayer},

	// k_kart
	{"K_PlayAttackTaunt", lib_kAttackSound},
	{"K_PlayBoostTaunt", lib_kBoostSound},
	{"K_PlayPowerGloatSound", lib_kGloatSound},
	{"K_PlayOvertakeSound", lib_kOvertakeSound},
	{"K_PlayLossSound", lib_kLossSound},
	{"K_PlayPainSound", lib_kPainSound},
	{"K_PlayHitEmSound", lib_kHitEmSound},
	{"K_TryHurtSoundExchange", lib_kTryHurtSoundExchange},
	{"K_IsPlayerLosing",lib_kIsPlayerLosing},
	{"K_IsPlayerWanted",lib_kIsPlayerWanted},
	{"K_KartBouncing",lib_kKartBouncing},
	{"K_MatchGenericExtraFlags",lib_kMatchGenericExtraFlags},
	{"K_DoInstashield",lib_kDoInstashield},
	{"K_SpawnBattlePoints",lib_kSpawnBattlePoints},
	{"K_SpinPlayer",lib_kSpinPlayer},
	{"K_TumblePlayer",lib_kTumblePlayer},
	{"K_StumblePlayer",lib_kStumblePlayer},
	{"K_ExplodePlayer",lib_kExplodePlayer},
	{"K_TakeBumpersFromPlayer",lib_kTakeBumpersFromPlayer},
	{"K_SpawnMineExplosion",lib_kSpawnMineExplosion},
	{"K_SpawnBoostTrail",lib_kSpawnBoostTrail},
	{"K_SpawnSparkleTrail",lib_kSpawnSparkleTrail},
	{"K_SpawnWipeoutTrail",lib_kSpawnWipeoutTrail},
	{"K_DriftDustHandling",lib_kDriftDustHandling},
	{"K_DoSneaker",lib_kDoSneaker},
	{"K_DoPogoSpring",lib_kDoPogoSpring},
	{"K_KillBananaChain",lib_kKillBananaChain},
	{"K_RepairOrbitChain",lib_kRepairOrbitChain},
	{"K_FindJawzTarget",lib_kFindJawzTarget},
	{"K_GetKartDriftSparkValue",lib_kGetKartDriftSparkValue},
	{"K_KartUpdatePosition",lib_kKartUpdatePosition},
	{"K_PopPlayerShield",lib_kPopPlayerShield},
	{"K_DropHnextList",lib_kDropHnextList},
	{"K_DropItems",lib_kDropItems},
	{"K_StripItems",lib_kStripItems},
	{"K_StripOther",lib_kStripOther},
	{"K_MomentumToFacing",lib_kMomentumToFacing},
	{"K_GetKartSpeed",lib_kGetKartSpeed},
	{"K_GetKartAccel",lib_kGetKartAccel},
	{"K_GetKartFlashing",lib_kGetKartFlashing},
	{"K_GetItemPatch",lib_kGetItemPatch},

	{"K_GetCollideAngle",lib_kGetCollideAngle},
	{"K_AddHitLag",lib_kAddHitLag},

	// k_powerup
	{"K_PowerUpRemaining",lib_kPowerUpRemaining},
	{"K_GivePowerUp",lib_kGivePowerUp},
	{"K_DropPowerUps",lib_kDropPowerUps},

	// k_boss
	{"K_InitBossHealthBar", lib_kInitBossHealthBar},
	{"K_UpdateBossHealthBar", lib_kUpdateBossHealthBar},
	{"K_DeclareWeakspot", lib_kDeclareWeakspot},
	{"VS_GetArena", lib_vsGetArena},
	{"VS_PredictAroundArena", lib_vsPredictAroundArena},
	{"VS_RandomPointOnArena", lib_vsRandomPointOnArena},

	// hu_stuff technically?
	{"HU_DoTitlecardCEcho", lib_startTitlecardCecho},

	{NULL, NULL}
};

int LUA_BaseLib(lua_State *L)
{
	// Set metatable for string
	lua_pushliteral(L, "");  // dummy string
	lua_getmetatable(L, -1);  // get string metatable
	lua_pushcfunction(L,lib_concat); // push concatination function
	lua_setfield(L,-2,"__add"); // ... store it as mathematical addition
	lua_pop(L, 2); // pop metatable and dummy string

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);

	// Set global functions
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}
