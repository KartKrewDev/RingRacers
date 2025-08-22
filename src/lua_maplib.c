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
/// \file  lua_maplib.c
/// \brief game map library for Lua scripting

#include "doomdef.h"
#include "r_state.h"
#include "p_local.h"
#include "p_setup.h"
#include "z_zone.h"
#include "p_slopes.h"
#include "p_polyobj.h"
#include "r_main.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "lua_hook.h" // hook_cmd_running errors

#include "dehacked.h"
#include "fastcmp.h"
#include "doomstat.h"

enum sector_e {
	sector_valid = 0,
	sector_floorheight,
	sector_ceilingheight,
	sector_floorpic,
	sector_ceilingpic,
	sector_lightlevel,
	sector_floorlightlevel,
	sector_floorlightabsolute,
	sector_ceilinglightlevel,
	sector_ceilinglightabsolute,
	sector_special,
	sector_tag,
	sector_taglist,
	sector_thinglist,
	sector_heightsec,
	sector_camsec,
	sector_lines,
	sector_ffloors,
	sector_fslope,
	sector_cslope,
	sector_flags,
	sector_specialflags,
	sector_damagetype,
	sector_triggertag,
	sector_triggerer,
	sector_friction,
	sector_gravity,
	sector_botcontroller,
	sector_action,
	sector_args,
	sector_stringargs,
	sector_activation,
};

static const char *const sector_opt[] = {
	"valid",
	"floorheight",
	"ceilingheight",
	"floorpic",
	"ceilingpic",
	"lightlevel",
	"floorlightlevel",
	"floorlightabsolute",
	"ceilinglightlevel",
	"ceilinglightabsolute",
	"special",
	"tag",
	"taglist",
	"thinglist",
	"heightsec",
	"camsec",
	"lines",
	"ffloors",
	"f_slope",
	"c_slope",
	"flags",
	"specialflags",
	"damagetype",
	"triggertag",
	"triggerer",
	"friction",
	"gravity",
	"botcontroller",
	"action",
	"args"
	"stringargs",
	"activation",
	NULL};

enum subsector_e {
	subsector_valid = 0,
	subsector_sector,
	subsector_numlines,
	subsector_firstline,
	subsector_polyList
};

static const char *const subsector_opt[] = {
	"valid",
	"sector",
	"numlines",
	"firstline",
	"polyList",
	NULL};

enum line_e {
	line_valid = 0,
	line_v1,
	line_v2,
	line_dx,
	line_dy,
	line_angle,
	line_flags,
	line_activation,
	line_special,
	line_tag,
	line_taglist,
	line_args,
	line_stringargs,
	line_sidenum,
	line_frontside,
	line_backside,
	line_alpha,
	line_executordelay,
	line_slopetype,
	line_frontsector,
	line_backsector,
	line_polyobj,
	line_callcount
};

static const char *const line_opt[] = {
	"valid",
	"v1",
	"v2",
	"dx",
	"dy",
	"angle",
	"flags",
	"activation",
	"special",
	"tag",
	"taglist",
	"args",
	"stringargs",
	"sidenum",
	"frontside",
	"backside",
	"alpha",
	"executordelay",
	"slopetype",
	"frontsector",
	"backsector",
	"polyobj",
	"callcount",
	NULL};

enum side_e {
	side_valid = 0,
	side_textureoffset,
	side_rowoffset,
	side_toptexture,
	side_bottomtexture,
	side_midtexture,
	side_line,
	side_sector,
	side_special,
	side_repeatcnt,
};

static const char *const side_opt[] = {
	"valid",
	"textureoffset",
	"rowoffset",
	"toptexture",
	"bottomtexture",
	"midtexture",
	"line",
	"sector",
	"special",
	"repeatcnt",
	NULL};

enum vertex_e {
	vertex_valid = 0,
	vertex_x,
	vertex_y,
	vertex_floorz,
	vertex_floorzset,
	vertex_ceilingz,
	vertex_ceilingzset
};

static const char *const vertex_opt[] = {
	"valid",
	"x",
	"y",
	"floorz",
	"floorzset",
	"ceilingz",
	"ceilingzset",
	NULL};

enum ffloor_e {
	ffloor_valid = 0,
	ffloor_topheight,
	ffloor_toppic,
	ffloor_toplightlevel,
	ffloor_bottomheight,
	ffloor_bottompic,
	ffloor_tslope,
	ffloor_bslope,
	ffloor_sector,
	ffloor_fofflags,
	ffloor_flags,
	ffloor_master,
	ffloor_target,
	ffloor_next,
	ffloor_prev,
	ffloor_alpha,
	ffloor_blend,
	ffloor_bustflags,
	ffloor_busttype,
	ffloor_busttag,
	ffloor_sinkspeed,
	ffloor_friction,
	ffloor_bouncestrength,
};

static const char *const ffloor_opt[] = {
	"valid",
	"topheight",
	"toppic",
	"toplightlevel",
	"bottomheight",
	"bottompic",
	"t_slope",
	"b_slope",
	"sector", // secnum pushed as control sector userdata
	"fofflags",
	"flags",
	"master", // control linedef
	"target", // target sector
	"next",
	"prev",
	"alpha",
	"blend",
	"bustflags",
	"busttype",
	"busttag",
	"sinkspeed",
	"friction",
	"bouncestrength",
	NULL};

#ifdef HAVE_LUA_SEGS
enum seg_e {
	seg_valid = 0,
	seg_v1,
	seg_v2,
	seg_side,
	seg_offset,
	seg_angle,
	seg_sidedef,
	seg_linedef,
	seg_frontsector,
	seg_backsector,
	seg_polyseg
};

static const char *const seg_opt[] = {
	"valid",
	"v1",
	"v2",
	"side",
	"offset",
	"angle",
	"sidedef",
	"linedef",
	"frontsector",
	"backsector",
	"polyseg",
	NULL};

enum node_e {
	node_valid = 0,
	node_x,
	node_y,
	node_dx,
	node_dy,
	node_bbox,
	node_children,
};

static const char *const node_opt[] = {
	"valid",
	"x",
	"y",
	"dx",
	"dy",
	"bbox",
	"children",
	NULL};

enum nodechild_e {
	nodechild_valid = 0,
	nodechild_right,
	nodechild_left,
};

static const char *const nodechild_opt[] = {
	"valid",
	"right",
	"left",
	NULL};
#endif

enum bbox_e {
	bbox_valid = 0,
	bbox_top,
	bbox_bottom,
	bbox_left,
	bbox_right,
};

static const char *const bbox_opt[] = {
	"valid",
	"top",
	"bottom",
	"left",
	"right",
	NULL};

enum slope_e {
	slope_valid = 0,
	slope_o,
	slope_d,
	slope_zdelta,
	slope_normal,
	slope_zangle,
	slope_xydirection,
	slope_flags
};

static const char *const slope_opt[] = {
	"valid",
	"o",
	"d",
	"zdelta",
	"normal",
	"zangle",
	"xydirection",
	"flags",
	NULL};

// shared by both vector2_t and vector3_t
enum vector_e {
	vector_x = 0,
	vector_y,
	vector_z
};

static const char *const vector_opt[] = {
	"x",
	"y",
	"z",
	NULL};

enum activator_e {
	activator_valid = 0,
	activator_mo,
	activator_line,
	activator_side,
	activator_sector,
	activator_po
};

static const char *const activator_opt[] = {
	"valid",
	"mo",
	"line",
	"side",
	"sector",
	"po",
	NULL};
	
enum botcontroller_e {
	botcontroller_valid = 0,
	botcontroller_trick,
	botcontroller_flags,
	botcontroller_forceangle,
};

static const char *const botcontroller_opt[] = {
	"valid",
	"trick",
	"flags",
	"forceangle",
	NULL};

static const char *const array_opt[] ={"iterate",NULL};
static const char *const valid_opt[] ={"valid",NULL};

/////////////////////////////////////////////
// sector/subsector list iterate functions //
/////////////////////////////////////////////

// iterates through a sector's thinglist!
static int lib_iterateSectorThinglist(lua_State *L)
{
	mobj_t *state = NULL;
	mobj_t *thing = NULL;

	INLEVEL

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sector.thinglist() directly, use it as 'for rover in sector.thinglist do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	else
		return 0; // no thinglist to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		thing = thing->snext;
	}
	else
		thing = state; // state is used as the "start" of the thinglist

	if (thing)
	{
		LUA_PushUserdata(L, thing, META_MOBJ);
		return 1;
	}
	return 0;
}

// iterates through the ffloors list in a sector!
static int lib_iterateSectorFFloors(lua_State *L)
{
	ffloor_t *state = NULL;
	ffloor_t *ffloor = NULL;

	INLEVEL

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sector.ffloors() directly, use it as 'for rover in sector.ffloors do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	else
		return 0; // no ffloors to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
		ffloor = ffloor->next;
	}
	else
		ffloor = state; // state is used as the "start" of the ffloor list

	if (ffloor)
	{
		LUA_PushUserdata(L, ffloor, META_FFLOOR);
		return 1;
	}
	return 0;
}

// iterates through a subsector's polyList! (for polyobj_t)
static int lib_iterateSubSectorPolylist(lua_State *L)
{
	polyobj_t *state = NULL;
	polyobj_t *po = NULL;

	INLEVEL

	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call subsector.polyList() directly, use it as 'for polyobj in subsector.polyList do <block> end'.");

	if (!lua_isnil(L, 1))
		state = *((polyobj_t **)luaL_checkudata(L, 1, META_POLYOBJ));
	else
		return 0; // no polylist to iterate through sorry!

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	if (!lua_isnil(L, 1))
	{
		po = *((polyobj_t **)luaL_checkudata(L, 1, META_POLYOBJ));
		po = (polyobj_t *)(po->link.next);
	}
	else
		po = state; // state is used as the "start" of the polylist

	if (po)
	{
		LUA_PushUserdata(L, po, META_POLYOBJ);
		return 1;
	}
	return 0;
}

static int sector_iterate(lua_State *L)
{
	lua_pushvalue(L, lua_upvalueindex(1)); // iterator function, or the "generator"
	lua_pushvalue(L, lua_upvalueindex(2)); // state (used as the "start" of the list for our purposes
	lua_pushnil(L); // initial value (unused)
	return 3;
}

////////////////////
// sector.lines[] //
////////////////////

// sector.lines, i -> sector.lines[i]
// sector.lines.valid, for validity checking
//
// 25/9/19 Monster Iestyn
// Modified this and _num to use triple pointers, to allow for a new hack of mine involving offsetof
// this way we don't need to check frontsector or backsector of line #0 in the array
//
static int sectorlines_get(lua_State *L)
{
	line_t ***seclines = *((line_t ****)luaL_checkudata(L, 1, META_SECTORLINES));
	size_t i;
	size_t numoflines = 0;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		int field = luaL_checkoption(L, 2, NULL, valid_opt);
		if (!seclines || !(*seclines))
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed sector_t.lines doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

/* a snip from sector_t struct in r_defs.h, for reference
	size_t linecount;
	struct line_s **lines; // [linecount] size
*/
	// get the "linecount" by shifting our retrieved memory address of "lines" to where "linecount" is in the sector_t, then dereferencing the result
	// we need this to determine the array's actual size, and therefore also the maximum value allowed as an index
	// this only works if seclines is actually a pointer to a sector's lines member in memory, oh boy
	numoflines = *(size_t *)FIELDFROM (sector_t, seclines, lines,/* -> */linecount);

/* OLD HACK
	// check first linedef to figure which of its sectors owns this sector->lines pointer
	// then check that sector's linecount to get a maximum index
	//if (!(*seclines)[0])
		//return luaL_error(L, "no lines found!"); // no first linedef?????
	if ((*seclines)[0]->frontsector->lines == *seclines)
		numoflines = (*seclines)[0]->frontsector->linecount;
	else if ((*seclines)[0]->backsector && *seclines[0]->backsector->lines == *seclines) // check backsector exists first
		numoflines = (*seclines)[0]->backsector->linecount;
	//if neither sector has it then ???
*/

	if (!numoflines)
		return luaL_error(L, "no lines found!");

	i = (size_t)lua_tointeger(L, 2);
	if (i >= numoflines)
		return 0;
	LUA_PushUserdata(L, (*seclines)[i], META_LINE);
	return 1;
}

// #(sector.lines) -> sector.linecount
static int sectorlines_num(lua_State *L)
{
	line_t ***seclines = *((line_t ****)luaL_checkudata(L, 1, META_SECTORLINES));
	size_t numoflines = 0;

	if (!seclines || !(*seclines))
		return luaL_error(L, "accessed sector_t.lines doesn't exist anymore.");

	// see comments in the _get function above
	numoflines = *(size_t *)FIELDFROM (sector_t, seclines, lines,/* -> */linecount);
	lua_pushinteger(L, numoflines);
	return 1;
}

//////////////
// sector_t //
//////////////

// args, i -> args[i]
static int sectorargs_get(lua_State *L)
{
	INT32 *args = *((INT32**)luaL_checkudata(L, 1, META_SECTORARGS));
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUM_SCRIPT_ARGS)
		return luaL_error(L, LUA_QL("sector_t.args") " index cannot be %d", i);
	lua_pushinteger(L, args[i]);
	return 1;
}

// #args -> NUM_SCRIPT_ARGS
static int sectorargs_len(lua_State* L)
{
	lua_pushinteger(L, NUM_SCRIPT_ARGS);
	return 1;
}

// stringargs, i -> stringargs[i]
static int sectorstringargs_get(lua_State *L)
{
	char **stringargs = *((char***)luaL_checkudata(L, 1, META_SECTORSTRINGARGS));
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUM_SCRIPT_STRINGARGS)
		return luaL_error(L, LUA_QL("sector_t.stringargs") " index cannot be %d", i);
	lua_pushstring(L, stringargs[i]);
	return 1;
}

// #stringargs -> NUM_SCRIPT_STRINGARGS
static int sectorstringargs_len(lua_State *L)
{
	lua_pushinteger(L, NUM_SCRIPT_STRINGARGS);
	return 1;
}

static int sector_get(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	enum sector_e field = luaL_checkoption(L, 2, sector_opt[0], sector_opt);
	INT16 i;

	if (!sector)
	{
		if (field == sector_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case sector_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case sector_floorheight:
		lua_pushfixed(L, sector->floorheight);
		return 1;
	case sector_ceilingheight:
		lua_pushfixed(L, sector->ceilingheight);
		return 1;
	case sector_floorpic: // floorpic
	{
		levelflat_t *levelflat = &levelflats[sector->floorpic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case sector_ceilingpic: // ceilingpic
	{
		levelflat_t *levelflat = &levelflats[sector->ceilingpic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case sector_lightlevel:
		lua_pushinteger(L, sector->lightlevel);
		return 1;
	case sector_floorlightlevel:
		lua_pushinteger(L, sector->floorlightlevel);
		return 1;
	case sector_floorlightabsolute:
		lua_pushboolean(L, sector->floorlightabsolute);
		return 1;
	case sector_ceilinglightlevel:
		lua_pushinteger(L, sector->ceilinglightlevel);
		return 1;
	case sector_ceilinglightabsolute:
		lua_pushboolean(L, sector->ceilinglightabsolute);
		return 1;
	case sector_special:
		lua_pushinteger(L, sector->special);
		return 1;
	case sector_tag:
		lua_pushinteger(L, (UINT16)Tag_FGet(&sector->tags));
		return 1;
	case sector_taglist:
		LUA_PushUserdata(L, &sector->tags, META_SECTORTAGLIST);
		return 1;
	case sector_thinglist: // thinglist
		lua_pushcfunction(L, lib_iterateSectorThinglist);
		LUA_PushUserdata(L, sector->thinglist, META_MOBJ);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateSectorThinglist and sector->thinglist as upvalues for the function
		return 1;
	case sector_heightsec: // heightsec - fake floor heights
		if (sector->heightsec < 0)
			return 0;
		LUA_PushUserdata(L, &sectors[sector->heightsec], META_SECTOR);
		return 1;
	case sector_camsec: // camsec - camera clipping heights
		if (sector->camsec < 0)
			return 0;
		LUA_PushUserdata(L, &sectors[sector->camsec], META_SECTOR);
		return 1;
	case sector_lines: // lines
		LUA_PushUserdata(L, &sector->lines, META_SECTORLINES); // push the address of the "lines" member in the struct, to allow our hacks in sectorlines_get/_num to work
		return 1;
	case sector_ffloors: // ffloors
		lua_pushcfunction(L, lib_iterateSectorFFloors);
		LUA_PushUserdata(L, sector->ffloors, META_FFLOOR);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateFFloors and sector->ffloors as upvalues for the function
		return 1;
	case sector_fslope: // f_slope
		LUA_PushUserdata(L, sector->f_slope, META_SLOPE);
		return 1;
	case sector_cslope: // c_slope
		LUA_PushUserdata(L, sector->c_slope, META_SLOPE);
		return 1;
	case sector_flags: // flags
		lua_pushinteger(L, sector->flags);
		return 1;
	case sector_specialflags: // specialflags
		lua_pushinteger(L, sector->specialflags);
		return 1;
	case sector_damagetype: // damagetype
		lua_pushinteger(L, (UINT8)sector->damagetype);
		return 1;
	case sector_triggertag: // triggertag
		lua_pushinteger(L, (INT16)sector->triggertag);
		return 1;
	case sector_triggerer: // triggerer
		lua_pushinteger(L, (UINT8)sector->triggerer);
		return 1;
	case sector_friction: // friction
		lua_pushfixed(L, sector->friction);
		return 1;
	case sector_gravity: // gravity
		lua_pushfixed(L, sector->gravity);
		return 1;
	case sector_botcontroller: // botController
		LUA_PushUserdata(L, &sector->botController, META_BOTCONTROLLER);
		return 1;
	case sector_action: // action
		lua_pushinteger(L, (INT16)sector->action);
		return 1;
	case sector_args:
		LUA_PushUserdata(L, sector->args, META_SECTORARGS);
		return 1;
	case sector_stringargs:
		LUA_PushUserdata(L, sector->stringargs, META_SECTORSTRINGARGS);
		return 1;
	case sector_activation: // activation
		lua_pushinteger(L, sector->activation);
		return 1;
	}
	return 0;
}

static int sector_set(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	enum sector_e field = luaL_checkoption(L, 2, sector_opt[0], sector_opt);

	if (!sector)
		return luaL_error(L, "accessed sector_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter sector_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter sector_t in CMD building code!");

	switch(field)
	{
	case sector_valid: // valid
	case sector_thinglist: // thinglist
	case sector_heightsec: // heightsec
	case sector_camsec: // camsec
	case sector_lines: // lines
	case sector_ffloors: // ffloors
	case sector_fslope: // f_slope
	case sector_cslope: // c_slope
	case sector_friction: // friction
	case sector_args: // args
	case sector_stringargs: // stringargs
	default:
		return luaL_error(L, "sector_t field " LUA_QS " cannot be set.", sector_opt[field]);
	case sector_floorheight: { // floorheight
		boolean flag;
		tm_t ptm = g_tm;
		fixed_t lastpos = sector->floorheight;
		sector->floorheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->floorheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_RestoreTMStruct(ptm);
		break;
	}
	case sector_ceilingheight: { // ceilingheight
		boolean flag;
		tm_t ptm = g_tm;
		fixed_t lastpos = sector->ceilingheight;
		sector->ceilingheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			sector->ceilingheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_RestoreTMStruct(ptm);
		break;
	}
	case sector_floorpic:
		sector->floorpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case sector_ceilingpic:
		sector->ceilingpic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case sector_lightlevel:
		sector->lightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_floorlightlevel:
		sector->floorlightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_floorlightabsolute:
		sector->floorlightabsolute = luaL_checkboolean(L, 3);
		break;
	case sector_ceilinglightlevel:
		sector->ceilinglightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_ceilinglightabsolute:
		sector->ceilinglightabsolute = luaL_checkboolean(L, 3);
		break;
	case sector_special:
		sector->special = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_tag:
		Tag_SectorFSet((UINT32)(sector - sectors), (INT16)luaL_checkinteger(L, 3));
		break;
	case sector_taglist:
		return LUA_ErrSetDirectly(L, "sector_t", "taglist");
	case sector_flags:
		sector->flags = luaL_checkinteger(L, 3);
		CheckForReverseGravity |= (sector->flags & MSF_GRAVITYFLIP);
		break;
	case sector_specialflags:
		sector->specialflags = luaL_checkinteger(L, 3);
		break;
	case sector_damagetype:
		sector->damagetype = (UINT8)luaL_checkinteger(L, 3);
		break;
	case sector_triggertag:
		sector->triggertag = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_triggerer:
		sector->triggerer = (UINT8)luaL_checkinteger(L, 3);
		break;
	case sector_gravity:
		sector->gravity = luaL_checkfixed(L, 3);
		break;
	case sector_action:
		sector->action = (INT16)luaL_checkinteger(L, 3);
		break;
	case sector_activation:
		sector->activation = luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

static int sector_num(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	lua_pushinteger(L, sector-sectors);
	return 1;
}

/////////////////
// subsector_t //
/////////////////

static int subsector_get(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	enum subsector_e field = luaL_checkoption(L, 2, subsector_opt[0], subsector_opt);

	if (!subsector)
	{
		if (field == subsector_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed subsector_t doesn't exist anymore.");
	}

	switch(field)
	{
	case subsector_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case subsector_sector:
		LUA_PushUserdata(L, subsector->sector, META_SECTOR);
		return 1;
	case subsector_numlines:
		lua_pushinteger(L, subsector->numlines);
		return 1;
	case subsector_firstline:
		lua_pushinteger(L, subsector->firstline);
		return 1;
	case subsector_polyList: // polyList
		lua_pushcfunction(L, lib_iterateSubSectorPolylist);
		LUA_PushUserdata(L, subsector->polyList, META_POLYOBJ);
		lua_pushcclosure(L, sector_iterate, 2); // push lib_iterateSubSectorPolylist and subsector->polyList as upvalues for the function
		return 1;
	}
	return 0;
}

static int subsector_num(lua_State *L)
{
	subsector_t *subsector = *((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR));
	lua_pushinteger(L, subsector-subsectors);
	return 1;
}

////////////
// line_t //
////////////

// args, i -> args[i]
static int lineargs_get(lua_State *L)
{
	INT32 *args = *((INT32**)luaL_checkudata(L, 1, META_LINEARGS));
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUM_SCRIPT_ARGS)
		return luaL_error(L, LUA_QL("line_t.args") " index cannot be %d", i);
	lua_pushinteger(L, args[i]);
	return 1;
}

// #args -> NUM_SCRIPT_ARGS
static int lineargs_len(lua_State* L)
{
	lua_pushinteger(L, NUM_SCRIPT_ARGS);
	return 1;
}

// stringargs, i -> stringargs[i]
static int linestringargs_get(lua_State *L)
{
	char **stringargs = *((char***)luaL_checkudata(L, 1, META_LINESTRINGARGS));
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUM_SCRIPT_STRINGARGS)
		return luaL_error(L, LUA_QL("line_t.stringargs") " index cannot be %d", i);
	lua_pushstring(L, stringargs[i]);
	return 1;
}

// #stringargs -> NUM_SCRIPT_STRINGARGS
static int linestringargs_len(lua_State *L)
{
	lua_pushinteger(L, NUM_SCRIPT_STRINGARGS);
	return 1;
}

static int line_get(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	enum line_e field = luaL_checkoption(L, 2, line_opt[0], line_opt);

	if (!line)
	{
		if (field == line_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed line_t doesn't exist anymore.");
	}

	switch(field)
	{
	case line_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case line_v1:
		LUA_PushUserdata(L, line->v1, META_VERTEX);
		return 1;
	case line_v2:
		LUA_PushUserdata(L, line->v2, META_VERTEX);
		return 1;
	case line_dx:
		lua_pushfixed(L, line->dx);
		return 1;
	case line_dy:
		lua_pushfixed(L, line->dy);
		return 1;
	case line_angle:
		lua_pushangle(L, line->angle);
		return 1;
	case line_flags:
		lua_pushinteger(L, line->flags);
		return 1;
	case line_activation:
		lua_pushinteger(L, line->activation);
		return 1;
	case line_special:
		lua_pushinteger(L, line->special);
		return 1;
	case line_tag:
		// HELLO
		// THIS IS LJ SONIC
		// HOW IS YOUR DAY?
		// BY THE WAY WHEN 2.3 OR 3.0 OR 4.0 OR SRB3 OR SRB4 OR WHATEVER IS OUT
		// YOU SHOULD REMEMBER TO CHANGE THIS SO IT ALWAYS RETURNS A UNSIGNED VALUE
		// HAVE A NICE DAY
		//
		//
		//
		//
		// you are ugly
		lua_pushinteger(L, Tag_FGet(&line->tags));
		return 1;
	case line_taglist:
		LUA_PushUserdata(L, &line->tags, META_TAGLIST);
		return 1;
	case line_args:
		LUA_PushUserdata(L, line->args, META_LINEARGS);
		return 1;
	case line_stringargs:
		LUA_PushUserdata(L, line->stringargs, META_LINESTRINGARGS);
		return 1;
	case line_sidenum:
		LUA_PushUserdata(L, line->sidenum, META_SIDENUM);
		return 1;
	case line_frontside: // frontside
		LUA_PushUserdata(L, &sides[line->sidenum[0]], META_SIDE);
		return 1;
	case line_backside: // backside
		if (line->sidenum[1] == 0xffff)
			return 0;
		LUA_PushUserdata(L, &sides[line->sidenum[1]], META_SIDE);
		return 1;
	case line_alpha:
		lua_pushfixed(L, line->alpha);
		return 1;
	case line_executordelay:
		lua_pushinteger(L, line->executordelay);
		return 1;
	case line_slopetype:
		switch(line->slopetype)
		{
		case ST_HORIZONTAL:
			lua_pushliteral(L, "horizontal");
			break;
		case ST_VERTICAL:
			lua_pushliteral(L, "vertical");
			break;
		case ST_POSITIVE:
			lua_pushliteral(L, "positive");
			break;
		case ST_NEGATIVE:
			lua_pushliteral(L, "negative");
			break;
		}
		return 1;
	case line_frontsector:
		LUA_PushUserdata(L, line->frontsector, META_SECTOR);
		return 1;
	case line_backsector:
		LUA_PushUserdata(L, line->backsector, META_SECTOR);
		return 1;
	case line_polyobj:
		LUA_PushUserdata(L, line->polyobj, META_POLYOBJ);
		return 1;
	case line_callcount:
		lua_pushinteger(L, line->callcount);
		return 1;
	}
	return 0;
}

static int line_num(lua_State *L)
{
	line_t *line = *((line_t **)luaL_checkudata(L, 1, META_LINE));
	lua_pushinteger(L, line-lines);
	return 1;
}

////////////////////
// line.sidenum[] //
////////////////////

static int sidenum_get(lua_State *L)
{
	UINT16 *sidenum = *((UINT16 **)luaL_checkudata(L, 1, META_SIDENUM));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		int field = luaL_checkoption(L, 2, NULL, valid_opt);
		if (!sidenum)
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed line_t doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	i = lua_tointeger(L, 2);
	if (i < 0 || i > 1)
		return 0;
	lua_pushinteger(L, sidenum[i]);
	return 1;
}

////////////
// side_t //
////////////

static int side_get(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	enum side_e field = luaL_checkoption(L, 2, side_opt[0], side_opt);

	if (!side)
	{
		if (field == side_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case side_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case side_textureoffset:
		lua_pushfixed(L, side->textureoffset);
		return 1;
	case side_rowoffset:
		lua_pushfixed(L, side->rowoffset);
		return 1;
	case side_toptexture:
		lua_pushinteger(L, side->toptexture);
		return 1;
	case side_bottomtexture:
		lua_pushinteger(L, side->bottomtexture);
		return 1;
	case side_midtexture:
		lua_pushinteger(L, side->midtexture);
		return 1;
	case side_line:
		LUA_PushUserdata(L, side->line, META_LINE);
		return 1;
	case side_sector:
		LUA_PushUserdata(L, side->sector, META_SECTOR);
		return 1;
	case side_special:
		lua_pushinteger(L, side->special);
		return 1;
	case side_repeatcnt:
		lua_pushinteger(L, side->repeatcnt);
		return 1;
	}
	return 0;
}

static int side_set(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	enum side_e field = luaL_checkoption(L, 2, side_opt[0], side_opt);

	if (!side)
	{
		if (field == side_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed side_t doesn't exist anymore.");
	}

	switch(field)
	{
	case side_valid: // valid
	case side_line:
	case side_sector:
	case side_special:
	default:
		return luaL_error(L, "side_t field " LUA_QS " cannot be set.", side_opt[field]);
	case side_textureoffset:
		side->textureoffset = luaL_checkfixed(L, 3);
		break;
	case side_rowoffset:
		side->rowoffset = luaL_checkfixed(L, 3);
		break;
	case side_toptexture:
		side->toptexture = luaL_checkinteger(L, 3);
		break;
	case side_bottomtexture:
		side->bottomtexture = luaL_checkinteger(L, 3);
		break;
	case side_midtexture:
		side->midtexture = luaL_checkinteger(L, 3);
		break;
	case side_repeatcnt:
		side->repeatcnt = luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

static int side_num(lua_State *L)
{
	side_t *side = *((side_t **)luaL_checkudata(L, 1, META_SIDE));
	lua_pushinteger(L, side-sides);
	return 1;
}

//////////////
// vertex_t //
//////////////

static int vertex_get(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	enum vertex_e field = luaL_checkoption(L, 2, vertex_opt[0], vertex_opt);

	if (!vertex)
	{
		if (field == vertex_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed vertex_t doesn't exist anymore.");
	}

	switch(field)
	{
	case vertex_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case vertex_x:
		lua_pushfixed(L, vertex->x);
		return 1;
	case vertex_y:
		lua_pushfixed(L, vertex->y);
		return 1;
	case vertex_floorzset:
		lua_pushboolean(L, vertex->floorzset);
		return 1;
	case vertex_ceilingzset:
		lua_pushboolean(L, vertex->ceilingzset);
		return 1;
	case vertex_floorz:
		lua_pushfixed(L, vertex->floorz);
		return 1;
	case vertex_ceilingz:
		lua_pushfixed(L, vertex->ceilingz);
		return 1;
	}
	return 0;
}

static int vertex_num(lua_State *L)
{
	vertex_t *vertex = *((vertex_t **)luaL_checkudata(L, 1, META_VERTEX));
	lua_pushinteger(L, vertex-vertexes);
	return 1;
}

#ifdef HAVE_LUA_SEGS

///////////
// seg_t //
///////////

static int seg_get(lua_State *L)
{
	seg_t *seg = *((seg_t **)luaL_checkudata(L, 1, META_SEG));
	enum seg_e field = luaL_checkoption(L, 2, seg_opt[0], seg_opt);

	if (!seg)
	{
		if (field == seg_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed seg_t doesn't exist anymore.");
	}

	switch(field)
	{
	case seg_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case seg_v1:
		LUA_PushUserdata(L, seg->v1, META_VERTEX);
		return 1;
	case seg_v2:
		LUA_PushUserdata(L, seg->v2, META_VERTEX);
		return 1;
	case seg_side:
		lua_pushinteger(L, seg->side);
		return 1;
	case seg_offset:
		lua_pushfixed(L, seg->offset);
		return 1;
	case seg_angle:
		lua_pushangle(L, seg->angle);
		return 1;
	case seg_sidedef:
		LUA_PushUserdata(L, seg->sidedef, META_SIDE);
		return 1;
	case seg_linedef:
		LUA_PushUserdata(L, seg->linedef, META_LINE);
		return 1;
	case seg_frontsector:
		LUA_PushUserdata(L, seg->frontsector, META_SECTOR);
		return 1;
	case seg_backsector:
		LUA_PushUserdata(L, seg->backsector, META_SECTOR);
		return 1;
	case seg_polyseg:
		LUA_PushUserdata(L, seg->polyseg, META_POLYOBJ);
		return 1;
	}
	return 0;
}

static int seg_num(lua_State *L)
{
	seg_t *seg = *((seg_t **)luaL_checkudata(L, 1, META_SEG));
	lua_pushinteger(L, seg-segs);
	return 1;
}

////////////
// node_t //
////////////

static int node_get(lua_State *L)
{
	node_t *node = *((node_t **)luaL_checkudata(L, 1, META_NODE));
	enum node_e field = luaL_checkoption(L, 2, node_opt[0], node_opt);

	if (!node)
	{
		if (field == node_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed node_t doesn't exist anymore.");
	}

	switch(field)
	{
	case node_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case node_x:
		lua_pushfixed(L, node->x);
		return 1;
	case node_y:
		lua_pushfixed(L, node->y);
		return 1;
	case node_dx:
		lua_pushfixed(L, node->x);
		return 1;
	case node_dy:
		lua_pushfixed(L, node->x);
		return 1;
	case node_bbox:
		LUA_PushUserdata(L, node->bbox, META_NODEBBOX);
		return 1;
	case node_children:
		LUA_PushUserdata(L, node->children, META_NODECHILDREN);
		return 1;
	}
	return 0;
}

static int node_num(lua_State *L)
{
	node_t *node = *((node_t **)luaL_checkudata(L, 1, META_NODE));
	lua_pushinteger(L, node-nodes);
	return 1;
}

///////////////
// node.bbox //
///////////////

/*
// node.bbox[i][j]: i = 0 or 1, j = 0 1 2 or 3
// NOTE: 2D arrays are NOT double pointers,
//       the second bbox will be directly after the first in memory (hence the way the bbox is pushed here)
// this function handles the [i] part, bbox_get handles the [j] part
static int nodebbox_get(lua_State *L)
{
	fixed_t *bbox = *((fixed_t **)luaL_checkudata(L, 1, META_NODEBBOX));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		int field = luaL_checkoption(L, 2, NULL, valid_opt);
		if (!bbox)
		{
			if (field == 0) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed node_t doesn't exist anymore.");
		} else if (field == 0) {
			lua_pushboolean(L, 1);
			return 1;
		}
	}

	i = lua_tointeger(L, 2);
	if (i < 0 || i > 1)
		return 0;
	LUA_PushUserdata(L, bbox + i*4*sizeof(fixed_t), META_BBOX);
	return 1;
}
*/
static int nodebbox_call(lua_State *L)
{
	fixed_t *bbox = *((fixed_t **)luaL_checkudata(L, 1, META_NODEBBOX));
	int i, j;
	int n = lua_gettop(L);

	if (!bbox)
		return luaL_error(L, "accessed node bbox doesn't exist anymore.");
	if (n < 3)
		return luaL_error(L, "arguments 2 and/or 3 not given (expected node.bbox(child, coord))");
	// get child
	if (!lua_isnumber(L, 2)) {
		enum nodechild_e field = luaL_checkoption(L, 2, nodechild_opt[0], nodechild_opt);
		switch (field) {
			case nodechild_right: i = 0; break;
			case nodechild_left:  i = 1; break;
			default:
				return luaL_error(L, "invalid node child \"%s\".", lua_tostring(L, 2));
		}
	}
	else {
		i = lua_tointeger(L, 2);
		if (i < 0 || i > 1)
			return 0;
	}
	// get bbox coord
	if (!lua_isnumber(L, 3)) {
		enum bbox_e field = luaL_checkoption(L, 3, bbox_opt[0], bbox_opt);
		switch (field) {
			case bbox_top:    j = BOXTOP;    break;
			case bbox_bottom: j = BOXBOTTOM; break;
			case bbox_left:   j = BOXLEFT;   break;
			case bbox_right:  j = BOXRIGHT;  break;
			default:
				return luaL_error(L, "invalid bbox coordinate \"%s\".", lua_tostring(L, 3));
		}
	}
	else {
		j = lua_tointeger(L, 3);
		if (j < 0 || j > 3)
			return 0;
	}
	lua_pushinteger(L, bbox[i*4 + j]);
	return 1;
}

/////////////////////
// node.children[] //
/////////////////////

// node.children[i]: i = 0 or 1
static int nodechildren_get(lua_State *L)
{
	UINT16 *children = *((UINT16 **)luaL_checkudata(L, 1, META_NODECHILDREN));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		enum nodechild_e field = luaL_checkoption(L, 2, nodechild_opt[0], nodechild_opt);
		if (!children)
		{
			if (field == nodechild_valid) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed node_t doesn't exist anymore.");
		} else if (field == nodechild_valid) {
			lua_pushboolean(L, 1);
			return 1;
		} else switch (field) {
			case nodechild_right: i = 0; break;
			case nodechild_left:  i = 1; break;
			default:              return 0;
		}
	}
	else {
		i = lua_tointeger(L, 2);
		if (i < 0 || i > 1)
			return 0;
	}
	lua_pushinteger(L, children[i]);
	return 1;
}
#endif

//////////
// bbox //
//////////

// bounding box (aka fixed_t array with four elements)
// NOTE: may be useful for polyobjects or other things later
static int bbox_get(lua_State *L)
{
	fixed_t *bbox = *((fixed_t **)luaL_checkudata(L, 1, META_BBOX));
	int i;
	lua_settop(L, 2);
	if (!lua_isnumber(L, 2))
	{
		enum bbox_e field = luaL_checkoption(L, 2, bbox_opt[0], bbox_opt);
		if (!bbox)
		{
			if (field == bbox_valid) {
				lua_pushboolean(L, 0);
				return 1;
			}
			return luaL_error(L, "accessed bbox doesn't exist anymore.");
		} else if (field == bbox_valid) {
			lua_pushboolean(L, 1);
			return 1;
		} else switch (field) {
			case bbox_top:    i = BOXTOP;    break;
			case bbox_bottom: i = BOXBOTTOM; break;
			case bbox_left:   i = BOXLEFT;   break;
			case bbox_right:  i = BOXRIGHT;  break;
			default:          return 0;
		}
	}
	else {
		i = lua_tointeger(L, 2);
		if (i < 0 || i > 3)
			return 0;
	}
	lua_pushinteger(L, bbox[i]);
	return 1;
}

///////////////
// sectors[] //
///////////////

static int lib_iterateSectors(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sectors.iterate() directly, use it as 'for sector in sectors.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((sector_t **)luaL_checkudata(L, 1, META_SECTOR)) - sectors)+1;
	if (i < numsectors)
	{
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	return 0;
}

static int lib_getSector(lua_State *L)
{
	INLEVEL
	if (lua_isnumber(L, 2))
	{
		size_t i = lua_tointeger(L, 2);
		if (i >= numsectors)
			return 0;
		LUA_PushUserdata(L, &sectors[i], META_SECTOR);
		return 1;
	}
	return 0;
}

static int lib_numsectors(lua_State *L)
{
	lua_pushinteger(L, numsectors);
	return 1;
}

//////////////////
// subsectors[] //
//////////////////

static int lib_iterateSubsectors(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call subsectors.iterate() directly, use it as 'for subsector in subsectors.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((subsector_t **)luaL_checkudata(L, 1, META_SUBSECTOR)) - subsectors)+1;
	if (i < numsubsectors)
	{
		LUA_PushUserdata(L, &subsectors[i], META_SUBSECTOR);
		return 1;
	}
	return 0;
}

static int lib_getSubsector(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsubsectors)
			return 0;
		LUA_PushUserdata(L, &subsectors[i], META_SUBSECTOR);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSubsectors);
		return 1;
	}
	return 0;
}

static int lib_numsubsectors(lua_State *L)
{
	lua_pushinteger(L, numsubsectors);
	return 1;
}

/////////////
// lines[] //
/////////////

static int lib_iterateLines(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call lines.iterate() directly, use it as 'for line in lines.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((line_t **)luaL_checkudata(L, 1, META_LINE)) - lines)+1;
	if (i < numlines)
	{
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	return 0;
}

static int lib_getLine(lua_State *L)
{
	INLEVEL
	if (lua_isnumber(L, 2))
	{
		size_t i = lua_tointeger(L, 2);
		if (i >= numlines)
			return 0;
		LUA_PushUserdata(L, &lines[i], META_LINE);
		return 1;
	}
	return 0;
}

static int lib_numlines(lua_State *L)
{
	lua_pushinteger(L, numlines);
	return 1;
}

/////////////
// sides[] //
/////////////

static int lib_iterateSides(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call sides.iterate() directly, use it as 'for side in sides.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((side_t **)luaL_checkudata(L, 1, META_SIDE)) - sides)+1;
	if (i < numsides)
	{
		LUA_PushUserdata(L, &sides[i], META_SIDE);
		return 1;
	}
	return 0;
}

static int lib_getSide(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsides)
			return 0;
		LUA_PushUserdata(L, &sides[i], META_SIDE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSides);
		return 1;
	}
	return 0;
}

static int lib_numsides(lua_State *L)
{
	lua_pushinteger(L, numsides);
	return 1;
}

////////////////
// vertexes[] //
////////////////

static int lib_iterateVertexes(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call vertexes.iterate() directly, use it as 'for vertex in vertexes.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((vertex_t **)luaL_checkudata(L, 1, META_VERTEX)) - vertexes)+1;
	if (i < numvertexes)
	{
		LUA_PushUserdata(L, &vertexes[i], META_VERTEX);
		return 1;
	}
	return 0;
}

static int lib_getVertex(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numvertexes)
			return 0;
		LUA_PushUserdata(L, &vertexes[i], META_VERTEX);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateVertexes);
		return 1;
	}
	return 0;
}

static int lib_numvertexes(lua_State *L)
{
	lua_pushinteger(L, numvertexes);
	return 1;
}

#ifdef HAVE_LUA_SEGS

////////////
// segs[] //
////////////

static int lib_iterateSegs(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call segs.iterate() directly, use it as 'for seg in segs.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((seg_t **)luaL_checkudata(L, 1, META_SEG)) - segs)+1;
	if (i < numsegs)
	{
		LUA_PushUserdata(L, &segs[i], META_SEG);
		return 1;
	}
	return 0;
}

static int lib_getSeg(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numsegs)
			return 0;
		LUA_PushUserdata(L, &segs[i], META_SEG);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSegs);
		return 1;
	}
	return 0;
}

static int lib_numsegs(lua_State *L)
{
	lua_pushinteger(L, numsegs);
	return 1;
}

/////////////
// nodes[] //
/////////////

static int lib_iterateNodes(lua_State *L)
{
	size_t i = 0;
	INLEVEL
	if (lua_gettop(L) < 2)
		return luaL_error(L, "Don't call nodes.iterate() directly, use it as 'for node in nodes.iterate do <block> end'.");
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (size_t)(*((node_t **)luaL_checkudata(L, 1, META_NODE)) - nodes)+1;
	if (i < numsegs)
	{
		LUA_PushUserdata(L, &nodes[i], META_NODE);
		return 1;
	}
	return 0;
}

static int lib_getNode(lua_State *L)
{
	int field;
	INLEVEL
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		size_t i = lua_tointeger(L, 1);
		if (i >= numnodes)
			return 0;
		LUA_PushUserdata(L, &nodes[i], META_NODE);
		return 1;
	}
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateNodes);
		return 1;
	}
	return 0;
}

static int lib_numnodes(lua_State *L)
{
	lua_pushinteger(L, numnodes);
	return 1;
}
#endif

//////////////
// ffloor_t //
//////////////

static INT32 P_GetOldFOFFlags(ffloor_t *fflr)
{
	INT32 result = 0;
	if (fflr->fofflags & FOF_EXISTS)
		result |= FF_OLD_EXISTS;
	if (fflr->fofflags & FOF_BLOCKPLAYER)
		result |= FF_OLD_BLOCKPLAYER;
	if (fflr->fofflags & FOF_BLOCKOTHERS)
		result |= FF_OLD_BLOCKOTHERS;
	if (fflr->fofflags & FOF_RENDERSIDES)
		result |= FF_OLD_RENDERSIDES;
	if (fflr->fofflags & FOF_RENDERPLANES)
		result |= FF_OLD_RENDERPLANES;
	if (fflr->fofflags & FOF_SWIMMABLE)
		result |= FF_OLD_SWIMMABLE;
	if (fflr->fofflags & FOF_NOSHADE)
		result |= FF_OLD_NOSHADE;
	if (fflr->fofflags & FOF_CUTSOLIDS)
		result |= FF_OLD_CUTSOLIDS;
	if (fflr->fofflags & FOF_CUTEXTRA)
		result |= FF_OLD_CUTEXTRA;
	if (fflr->fofflags & FOF_CUTSPRITES)
		result |= FF_OLD_CUTSPRITES;
	if (fflr->fofflags & FOF_BOTHPLANES)
		result |= FF_OLD_BOTHPLANES;
	if (fflr->fofflags & FOF_EXTRA)
		result |= FF_OLD_EXTRA;
	if (fflr->fofflags & FOF_TRANSLUCENT)
		result |= FF_OLD_TRANSLUCENT;
	if (fflr->fofflags & FOF_FOG)
		result |= FF_OLD_FOG;
	if (fflr->fofflags & FOF_INVERTPLANES)
		result |= FF_OLD_INVERTPLANES;
	if (fflr->fofflags & FOF_ALLSIDES)
		result |= FF_OLD_ALLSIDES;
	if (fflr->fofflags & FOF_INVERTSIDES)
		result |= FF_OLD_INVERTSIDES;
	if (fflr->fofflags & FOF_DOUBLESHADOW)
		result |= FF_OLD_DOUBLESHADOW;
	if (fflr->fofflags & FOF_FLOATBOB)
		result |= FF_OLD_FLOATBOB;
	if (fflr->fofflags & FOF_NORETURN)
		result |= FF_OLD_NORETURN;
	if (fflr->fofflags & FOF_CRUMBLE)
		result |= FF_OLD_CRUMBLE;
	if (fflr->bustflags & FB_ONLYBOTTOM)
		result |= FF_OLD_SHATTERBOTTOM;
	if (fflr->fofflags & FOF_GOOWATER)
		result |= FF_OLD_GOOWATER;
	if (fflr->fofflags & FOF_MARIO)
		result |= FF_OLD_MARIO;
	if (fflr->fofflags & FOF_BUSTUP)
		result |= FF_OLD_BUSTUP;
	if (fflr->fofflags & FOF_QUICKSAND)
		result |= FF_OLD_QUICKSAND;
	if (fflr->fofflags & FOF_PLATFORM)
		result |= FF_OLD_PLATFORM;
	if (fflr->fofflags & FOF_REVERSEPLATFORM)
		result |= FF_OLD_REVERSEPLATFORM;
	if (fflr->fofflags & FOF_INTANGIBLEFLATS)
		result |= FF_OLD_INTANGIBLEFLATS;
	if (fflr->busttype == BT_TOUCH)
		result |= FF_OLD_SHATTER;
	if (fflr->busttype == BT_SPINBUST)
		result |= FF_OLD_SPINBUST;
	if (fflr->busttype == BT_STRONG)
		result |= FF_OLD_STRONGBUST;
	if (fflr->fofflags & FF_OLD_RIPPLE)
		result |= FOF_RIPPLE;
	if (fflr->fofflags & FF_OLD_COLORMAPONLY)
		result |= FOF_COLORMAPONLY;
	return result;
}

static int ffloor_get(lua_State *L)
{
	ffloor_t *ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	enum ffloor_e field = luaL_checkoption(L, 2, ffloor_opt[0], ffloor_opt);
	INT16 i;

	if (!ffloor)
	{
		if (field == ffloor_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed ffloor_t doesn't exist anymore.");
	}

	switch(field)
	{
	case ffloor_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case ffloor_topheight:
		lua_pushfixed(L, *ffloor->topheight);
		return 1;
	case ffloor_toppic: { // toppic
		levelflat_t *levelflat = &levelflats[*ffloor->toppic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case ffloor_toplightlevel:
		lua_pushinteger(L, *ffloor->toplightlevel);
		return 1;
	case ffloor_bottomheight:
		lua_pushfixed(L, *ffloor->bottomheight);
		return 1;
	case ffloor_bottompic: { // bottompic
		levelflat_t *levelflat = &levelflats[*ffloor->bottompic];
		for (i = 0; i < 8; i++)
			if (!levelflat->name[i])
				break;
		lua_pushlstring(L, levelflat->name, i);
		return 1;
	}
	case ffloor_tslope:
		LUA_PushUserdata(L, *ffloor->t_slope, META_SLOPE);
		return 1;
	case ffloor_bslope:
		LUA_PushUserdata(L, *ffloor->b_slope, META_SLOPE);
		return 1;
	case ffloor_sector:
		LUA_PushUserdata(L, &sectors[ffloor->secnum], META_SECTOR);
		return 1;
	case ffloor_fofflags:
		lua_pushinteger(L, ffloor->fofflags);
		return 1;
	case ffloor_flags:
		lua_pushinteger(L, P_GetOldFOFFlags(ffloor));
		return 1;
	case ffloor_master:
		LUA_PushUserdata(L, ffloor->master, META_LINE);
		return 1;
	case ffloor_target:
		LUA_PushUserdata(L, ffloor->target, META_SECTOR);
		return 1;
	case ffloor_next:
		LUA_PushUserdata(L, ffloor->next, META_FFLOOR);
		return 1;
	case ffloor_prev:
		LUA_PushUserdata(L, ffloor->prev, META_FFLOOR);
		return 1;
	case ffloor_alpha:
		lua_pushinteger(L, ffloor->alpha);
		return 1;
	case ffloor_blend:
		lua_pushinteger(L, ffloor->blend);
		return 1;
	case ffloor_bustflags:
		lua_pushinteger(L, ffloor->bustflags);
		return 1;
	case ffloor_busttype:
		lua_pushinteger(L, ffloor->busttype);
		return 1;
	case ffloor_busttag:
		lua_pushinteger(L, ffloor->busttag);
		return 1;
	case ffloor_sinkspeed:
		lua_pushfixed(L, ffloor->sinkspeed);
		return 1;
	case ffloor_friction:
		lua_pushfixed(L, ffloor->friction);
		return 1;
	case ffloor_bouncestrength:
		lua_pushfixed(L, ffloor->bouncestrength);
		return 1;
	}
	return 0;
}

static void P_SetOldFOFFlags(ffloor_t *fflr, oldffloortype_e oldflags)
{
	ffloortype_e originalflags = fflr->fofflags;
	fflr->fofflags = 0;
	if (oldflags & FF_OLD_EXISTS)
		fflr->fofflags |= FOF_EXISTS;
	if (oldflags & FF_OLD_BLOCKPLAYER)
		fflr->fofflags |= FOF_BLOCKPLAYER;
	if (oldflags & FF_OLD_BLOCKOTHERS)
		fflr->fofflags |= FOF_BLOCKOTHERS;
	if (oldflags & FF_OLD_RENDERSIDES)
		fflr->fofflags |= FOF_RENDERSIDES;
	if (oldflags & FF_OLD_RENDERPLANES)
		fflr->fofflags |= FOF_RENDERPLANES;
	if (oldflags & FF_OLD_SWIMMABLE)
		fflr->fofflags |= FOF_SWIMMABLE;
	if (oldflags & FF_OLD_NOSHADE)
		fflr->fofflags |= FOF_NOSHADE;
	if (oldflags & FF_OLD_CUTSOLIDS)
		fflr->fofflags |= FOF_CUTSOLIDS;
	if (oldflags & FF_OLD_CUTEXTRA)
		fflr->fofflags |= FOF_CUTEXTRA;
	if (oldflags & FF_OLD_CUTSPRITES)
		fflr->fofflags |= FOF_CUTSPRITES;
	if (oldflags & FF_OLD_BOTHPLANES)
		fflr->fofflags |= FOF_BOTHPLANES;
	if (oldflags & FF_OLD_EXTRA)
		fflr->fofflags |= FOF_EXTRA;
	if (oldflags & FF_OLD_TRANSLUCENT)
		fflr->fofflags |= FOF_TRANSLUCENT;
	if (oldflags & FF_OLD_FOG)
		fflr->fofflags |= FOF_FOG;
	if (oldflags & FF_OLD_INVERTPLANES)
		fflr->fofflags |= FOF_INVERTPLANES;
	if (oldflags & FF_OLD_ALLSIDES)
		fflr->fofflags |= FOF_ALLSIDES;
	if (oldflags & FF_OLD_INVERTSIDES)
		fflr->fofflags |= FOF_INVERTSIDES;
	if (oldflags & FF_OLD_DOUBLESHADOW)
		fflr->fofflags |= FOF_DOUBLESHADOW;
	if (oldflags & FF_OLD_FLOATBOB)
		fflr->fofflags |= FOF_FLOATBOB;
	if (oldflags & FF_OLD_NORETURN)
		fflr->fofflags |= FOF_NORETURN;
	if (oldflags & FF_OLD_CRUMBLE)
		fflr->fofflags |= FOF_CRUMBLE;
	if (oldflags & FF_OLD_GOOWATER)
		fflr->fofflags |= FOF_GOOWATER;
	if (oldflags & FF_OLD_MARIO)
		fflr->fofflags |= FOF_MARIO;
	if (oldflags & FF_OLD_BUSTUP)
		fflr->fofflags |= FOF_BUSTUP;
	if (oldflags & FF_OLD_QUICKSAND)
		fflr->fofflags |= FOF_QUICKSAND;
	if (oldflags & FF_OLD_PLATFORM)
		fflr->fofflags |= FOF_PLATFORM;
	if (oldflags & FF_OLD_REVERSEPLATFORM)
		fflr->fofflags |= FOF_REVERSEPLATFORM;
	if (oldflags & FF_OLD_RIPPLE)
		fflr->fofflags |= FOF_RIPPLE;
	if (oldflags & FF_OLD_COLORMAPONLY)
		fflr->fofflags |= FOF_COLORMAPONLY;
	if (originalflags & FOF_BOUNCY)
		fflr->fofflags |= FOF_BOUNCY;
	if (originalflags & FOF_SPLAT)
		fflr->fofflags |= FOF_SPLAT;

	if (oldflags & FF_OLD_SHATTER)
		fflr->busttype = BT_TOUCH;
	else if (oldflags & FF_OLD_SPINBUST)
		fflr->busttype = BT_SPINBUST;
	else if (oldflags & FF_OLD_STRONGBUST)
		fflr->busttype = BT_STRONG;
	else
		fflr->busttype = BT_REGULAR;

	if (oldflags & FF_OLD_SHATTERBOTTOM)
		fflr->bustflags |= FB_ONLYBOTTOM;
	else
		fflr->bustflags &= ~FB_ONLYBOTTOM;
}

static int ffloor_set(lua_State *L)
{
	ffloor_t *ffloor = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	enum ffloor_e field = luaL_checkoption(L, 2, ffloor_opt[0], ffloor_opt);

	if (!ffloor)
		return luaL_error(L, "accessed ffloor_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter ffloor_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter ffloor_t in CMD building code!");

	switch(field)
	{
	case ffloor_valid: // valid
	case ffloor_tslope: // t_slope
	case ffloor_bslope: // b_slope
	case ffloor_sector: // sector
	case ffloor_master: // master
	case ffloor_target: // target
	case ffloor_next: // next
	case ffloor_prev: // prev
	default:
		return luaL_error(L, "ffloor_t field " LUA_QS " cannot be set.", ffloor_opt[field]);
	case ffloor_topheight: { // topheight
		boolean flag;
		fixed_t lastpos = *ffloor->topheight;
		tm_t ptm = g_tm;
		sector_t *sector = &sectors[ffloor->secnum];
		sector->ceilingheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			*ffloor->topheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_RestoreTMStruct(ptm);
		break;
	}
	case ffloor_toppic:
		*ffloor->toppic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case ffloor_toplightlevel:
		*ffloor->toplightlevel = (INT16)luaL_checkinteger(L, 3);
		break;
	case ffloor_bottomheight: { // bottomheight
		boolean flag;
		fixed_t lastpos = *ffloor->bottomheight;
		tm_t ptm = g_tm;
		sector_t *sector = &sectors[ffloor->secnum];
		sector->floorheight = luaL_checkfixed(L, 3);
		flag = P_CheckSector(sector, true);
		if (flag && sector->numattached)
		{
			*ffloor->bottomheight = lastpos;
			P_CheckSector(sector, true);
		}
		P_RestoreTMStruct(ptm);
		break;
	}
	case ffloor_bottompic:
		*ffloor->bottompic = P_AddLevelFlatRuntime(luaL_checkstring(L, 3));
		break;
	case ffloor_fofflags: {
		ffloortype_e oldflags = ffloor->fofflags; // store FOF's old flags
		ffloor->fofflags = luaL_checkinteger(L, 3);
		if (ffloor->fofflags != oldflags)
			ffloor->target->moved = true; // reset target sector's lightlist
		break;
	}
	case ffloor_flags: {
		ffloortype_e oldflags = ffloor->fofflags; // store FOF's old flags
		busttype_e oldbusttype = ffloor->busttype;
		ffloorbustflags_e oldbustflags = ffloor->bustflags;
		oldffloortype_e newflags = luaL_checkinteger(L, 3);
		P_SetOldFOFFlags(ffloor, newflags);
		if (ffloor->fofflags != oldflags || ffloor->busttype != oldbusttype || ffloor->bustflags != oldbustflags)
			ffloor->target->moved = true; // reset target sector's lightlist
		break;
	}
	case ffloor_alpha:
		ffloor->alpha = (INT32)luaL_checkinteger(L, 3);
		break;
	case ffloor_blend:
		ffloor->blend = (INT32)luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

//////////////
// pslope_t //
//////////////

static int slope_get(lua_State *L)
{
	pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
	enum slope_e field = luaL_checkoption(L, 2, slope_opt[0], slope_opt);

	if (!slope)
	{
		if (field == slope_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed pslope_t doesn't exist anymore.");
	}

	switch(field)
	{
	case slope_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case slope_o: // o
		LUA_PushUserdata(L, &slope->o, META_VECTOR3);
		return 1;
	case slope_d: // d
		LUA_PushUserdata(L, &slope->d, META_VECTOR2);
		return 1;
	case slope_zdelta: // zdelta
		lua_pushfixed(L, slope->zdelta);
		return 1;
	case slope_normal: // normal
		LUA_PushUserdata(L, &slope->normal, META_VECTOR3);
		return 1;
	case slope_zangle: // zangle
		lua_pushangle(L, slope->zangle);
		return 1;
	case slope_xydirection: // xydirection
		lua_pushangle(L, slope->xydirection);
		return 1;
	case slope_flags: // flags
		lua_pushinteger(L, slope->flags);
		return 1;
	}
	return 0;
}

static int slope_set(lua_State *L)
{
	pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
	enum slope_e field = luaL_checkoption(L, 2, slope_opt[0], slope_opt);

	if (!slope)
		return luaL_error(L, "accessed pslope_t doesn't exist anymore.");

	if (hud_running)
		return luaL_error(L, "Do not alter pslope_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter pslope_t in CMD building code!");

	switch(field) // todo: reorganize this shit
	{
	case slope_valid: // valid
	case slope_d: // d
	case slope_flags: // flags
	case slope_normal: // normal
	default:
		return luaL_error(L, "pslope_t field " LUA_QS " cannot be set.", slope_opt[field]);
	case slope_o: { // o
		luaL_checktype(L, 3, LUA_TTABLE);

		lua_getfield(L, 3, "x");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 1);
		}
		if (!lua_isnil(L, -1))
			slope->o.x = luaL_checkfixed(L, -1);
		else
			slope->o.x = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "y");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 2);
		}
		if (!lua_isnil(L, -1))
			slope->o.y = luaL_checkfixed(L, -1);
		else
			slope->o.y = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "z");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 3);
		}
		if (!lua_isnil(L, -1))
			slope->o.z = luaL_checkfixed(L, -1);
		else
			slope->o.z = 0;
		lua_pop(L, 1);
		break;
	}
	case slope_zdelta: { // zdelta, this is temp until i figure out wtf to do
		slope->zdelta = luaL_checkfixed(L, 3);
		slope->zangle = R_PointToAngle2(0, 0, FRACUNIT, -slope->zdelta);
		P_CalculateSlopeNormal(slope);
		break;
	}
	case slope_zangle: { // zangle
		angle_t zangle = luaL_checkangle(L, 3);
		if (zangle == ANGLE_90 || zangle == ANGLE_270)
			return luaL_error(L, "invalid zangle for slope!");
		slope->zangle = zangle;
		slope->zdelta = -FINETANGENT(((slope->zangle+ANGLE_90)>>ANGLETOFINESHIFT) & 4095);
		P_CalculateSlopeNormal(slope);
		break;
	}
	case slope_xydirection: // xydirection
		slope->xydirection = luaL_checkangle(L, 3);
		slope->d.x = -FINECOSINE((slope->xydirection>>ANGLETOFINESHIFT) & FINEMASK);
		slope->d.y = -FINESINE((slope->xydirection>>ANGLETOFINESHIFT) & FINEMASK);
		P_CalculateSlopeNormal(slope);
		break;
	}
	return 0;
}

///////////////
// vector*_t //
///////////////

static int vector2_get(lua_State *L)
{
	vector2_t *vec = *((vector2_t **)luaL_checkudata(L, 1, META_VECTOR2));
	enum vector_e field = luaL_checkoption(L, 2, vector_opt[0], vector_opt);

	if (!vec)
		return luaL_error(L, "accessed vector2_t doesn't exist anymore.");

	switch(field)
	{
		case vector_x: lua_pushfixed(L, vec->x); return 1;
		case vector_y: lua_pushfixed(L, vec->y); return 1;
		default: break;
	}

	return 0;
}

static int vector3_get(lua_State *L)
{
	vector3_t *vec = *((vector3_t **)luaL_checkudata(L, 1, META_VECTOR3));
	enum vector_e field = luaL_checkoption(L, 2, vector_opt[0], vector_opt);

	if (!vec)
		return luaL_error(L, "accessed vector3_t doesn't exist anymore.");

	switch(field)
	{
		case vector_x: lua_pushfixed(L, vec->x); return 1;
		case vector_y: lua_pushfixed(L, vec->y); return 1;
		case vector_z: lua_pushfixed(L, vec->z); return 1;
		default: break;
	}

	return 0;
}

/////////////////////
// mapheaderinfo[] //
/////////////////////

static int lib_getMapheaderinfo(lua_State *L)
{
	// i -> mapheaderinfo[i-1]

	//int field;
	lua_settop(L, 2);
	lua_remove(L, 1); // dummy userdata table is unused.
	if (lua_isnumber(L, 1))
	{
		INT32 i = lua_tointeger(L, 1)-1;
		if (i < 0 || i >= nummapheaders)
			return 0;
		LUA_PushUserdata(L, mapheaderinfo[i], META_MAPHEADER);
		//CONS_Printf(mapheaderinfo[i]->lvlttl);
		return 1;
	}/*
	field = luaL_checkoption(L, 1, NULL, array_opt);
	switch(field)
	{
	case 0: // iterate
		lua_pushcfunction(L, lib_iterateSubsectors);
		return 1;
	}*/
	return 0;
}

static int lib_nummapheaders(lua_State *L)
{
	lua_pushinteger(L, nummapheaders);
	return 1;
}

/////////////////
// mapheader_t //
/////////////////

static int mapheaderinfo_get(lua_State *L)
{
	mapheader_t *header = *((mapheader_t **)luaL_checkudata(L, 1, META_MAPHEADER));
	const char *field = luaL_checkstring(L, 2);
	if (fastcmp(field,"lvlttl"))
		lua_pushstring(L, header->lvlttl);
	else if (fastcmp(field,"menuttl"))
		lua_pushstring(L, header->menuttl);
	else if (fastcmp(field,"zonttl"))
		lua_pushstring(L, header->zonttl);
	else if (fastcmp(field,"actnum"))
		lua_pushinteger(L, header->actnum);
	else if (fastcmp(field,"typeoflevel"))
		lua_pushinteger(L, header->typeoflevel);
	else if (fastcmp(field,"keywords"))
		lua_pushstring(L, header->keywords);
	else if (fastcmp(field,"musname")) // we create a table here because it saves us from a userdata nightmare
	{
		UINT8 i;
		lua_createtable(L, header->musname_size, 0);
		for (i = 0; i < header->musname_size; i++)
		{
			lua_pushstring(L, header->musname[i]);
			lua_rawseti(L, -2, 1 + i);
		}
	}
	else if (fastcmp(field,"encoremusname")) // we create a table here because it saves us from a userdata nightmare
	{
		UINT8 i;
		lua_createtable(L, header->encoremusname_size, 0);
		for (i = 0; i < header->encoremusname_size; i++)
		{
			lua_pushstring(L, header->encoremusname[i]);
			lua_rawseti(L, -2, 1 + i);
		}
	}
	else if (fastcmp(field,"associatedmus")) // we create a table here because it saves us from a userdata nightmare
	{
		UINT8 i;
		lua_createtable(L, header->associatedmus_size, 0);
		for (i = 0; i < header->associatedmus_size; i++)
		{
			lua_pushstring(L, header->associatedmus[i]);
			lua_rawseti(L, -2, 1 + i);
		}
	}
	else if (fastcmp(field,"mustrack"))
		lua_pushinteger(L, header->mustrack);
	else if (fastcmp(field,"muspos"))
		lua_pushinteger(L, header->muspos);
	else if (fastcmp(field,"musname_size"))
		lua_pushinteger(L, header->musname_size);
	else if (fastcmp(field,"encoremusname_size"))
		lua_pushinteger(L, header->encoremusname_size);
	else if (fastcmp(field,"associatedmus_size"))
		lua_pushinteger(L, header->associatedmus_size);
	else if (fastcmp(field,"weather"))
		lua_pushinteger(L, header->weather);
	else if (fastcmp(field,"skytexture"))
		lua_pushstring(L, header->skytexture);
	else if (fastcmp(field,"skybox_scalex"))
		lua_pushinteger(L, header->skybox_scalex);
	else if (fastcmp(field,"skybox_scaley"))
		lua_pushinteger(L, header->skybox_scaley);
	else if (fastcmp(field,"skybox_scalez"))
		lua_pushinteger(L, header->skybox_scalez);
	else if (fastcmp(field,"runsoc"))
		lua_pushstring(L, header->runsoc);
	else if (fastcmp(field,"scriptname"))
		lua_pushstring(L, header->scriptname);
	else if (fastcmp(field,"precutscenenum"))
		lua_pushinteger(L, header->precutscenenum);
	else if (fastcmp(field,"cutscenenum"))
		lua_pushinteger(L, header->cutscenenum);
	else if (fastcmp(field,"palette"))
		lua_pushinteger(L, header->palette);
	else if (fastcmp(field,"numlaps"))
		lua_pushinteger(L, header->numlaps);
	else if (fastcmp(field,"lapspersection"))
		lua_pushinteger(L, header->lapspersection);
	else if (fastcmp(field,"levelselect"))
		lua_pushinteger(L, header->levelselect);
	else if (fastcmp(field,"levelflags"))
		lua_pushinteger(L, header->levelflags);
	else if (fastcmp(field,"menuflags"))
		lua_pushinteger(L, header->menuflags);
	else if (fastcmp(field,"mobj_scale"))
		lua_pushfixed(L, header->mobj_scale);
	else if (fastcmp(field, "gravity"))
		lua_pushfixed(L, header->gravity);
	else {
		// Read custom vars now
		// (note: don't include the "LUA." in your lua scripts!)
		UINT8 j = 0;
		for (;j < header->numCustomOptions && !fastcmp(field, header->customopts[j].option); ++j);

		if(j < header->numCustomOptions)
			lua_pushstring(L, header->customopts[j].value);
		else
			lua_pushnil(L);
	}
	return 1;
}

/////////////////
// activator_t //
/////////////////

static int activator_get(lua_State *L)
{
	activator_t *activator = *((activator_t **)luaL_checkudata(L, 1, META_ACTIVATOR));
	enum activator_e field = luaL_checkoption(L, 2, activator_opt[0], activator_opt);

	if (activator == NULL)
	{
		if (field == activator_valid)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		return luaL_error(L, "accessed activator_t doesn't exist anymore.");
	}

	switch (field)
	{
		case activator_valid:
			lua_pushboolean(L, 1);
			return 1;

		case activator_mo:
			LUA_PushUserdata(L, activator->mo, META_MOBJ);
			return 1;

		case activator_line:
			LUA_PushUserdata(L, activator->line, META_LINE);
			return 1;

		case activator_side:
			lua_pushinteger(L, activator->side);
			return 1;

		case activator_sector:
			LUA_PushUserdata(L, activator->sector, META_SECTOR);
			return 1;

		case activator_po:
			LUA_PushUserdata(L, activator->po, META_POLYOBJ);
			return 1;

		default:
			break;
	}

	return 0;
}

/////////////////////
// botcontroller_t //
/////////////////////

static int botcontroller_get(lua_State *L)
{
	botcontroller_t *botcontroller = *((botcontroller_t **)luaL_checkudata(L, 1, META_BOTCONTROLLER));
	enum botcontroller_e field = luaL_checkoption(L, 2, botcontroller_opt[0], botcontroller_opt);

	if (!botcontroller)
	{
		if (field == botcontroller_valid) {
			lua_pushboolean(L, false);
			return 1;
		}
		return luaL_error(L, "accessed botcontroller_t doesn't exist anymore.");
	}

	switch (field)
	{
		case botcontroller_valid:
			lua_pushboolean(L, true);
			return 1;

		case botcontroller_trick:
			lua_pushinteger(L, botcontroller->trick);
			return 1;

		case botcontroller_flags:
			lua_pushinteger(L, botcontroller->flags);
			return 1;

		case botcontroller_forceangle:
			lua_pushangle(L, botcontroller->forceAngle);
			return 1;

		default:
			break;
	}

	return 0;
}

int LUA_MapLib(lua_State *L)
{
	luaL_newmetatable(L, META_SECTORLINES);
		lua_pushcfunction(L, sectorlines_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sectorlines_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SECTOR);
		lua_pushcfunction(L, sector_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sector_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, sector_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SECTORARGS);
		lua_pushcfunction(L, sectorargs_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sectorargs_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SECTORSTRINGARGS);
		lua_pushcfunction(L, sectorstringargs_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, sectorstringargs_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SUBSECTOR);
		lua_pushcfunction(L, subsector_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, subsector_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_LINE);
		lua_pushcfunction(L, line_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, line_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_LINEARGS);
		lua_pushcfunction(L, lineargs_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, lineargs_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_LINESTRINGARGS);
		lua_pushcfunction(L, linestringargs_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, linestringargs_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SIDENUM);
		lua_pushcfunction(L, sidenum_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SIDE);
		lua_pushcfunction(L, side_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, side_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, side_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_VERTEX);
		lua_pushcfunction(L, vertex_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, vertex_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_FFLOOR);
		lua_pushcfunction(L, ffloor_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, ffloor_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);

#ifdef HAVE_LUA_SEGS
	luaL_newmetatable(L, META_SEG);
		lua_pushcfunction(L, seg_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, seg_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_NODE);
		lua_pushcfunction(L, node_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, node_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_NODEBBOX);
		//lua_pushcfunction(L, nodebbox_get);
		//lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, nodebbox_call);
		lua_setfield(L, -2, "__call");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_NODECHILDREN);
		lua_pushcfunction(L, nodechildren_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
#endif

	luaL_newmetatable(L, META_BBOX);
		lua_pushcfunction(L, bbox_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_SLOPE);
		lua_pushcfunction(L, slope_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, slope_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_VECTOR2);
		lua_pushcfunction(L, vector2_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_VECTOR3);
		lua_pushcfunction(L, vector3_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_MAPHEADER);
		lua_pushcfunction(L, mapheaderinfo_get);
		lua_setfield(L, -2, "__index");

		//lua_pushcfunction(L, mapheaderinfo_num);
		//lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_ACTIVATOR);
		lua_pushcfunction(L, activator_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, META_BOTCONTROLLER);
		lua_pushcfunction(L, botcontroller_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	

	LUA_PushTaggableObjectArray(L, "sectors",
			lib_iterateSectors,
			lib_getSector,
			lib_numsectors,
			tags_sectors,
			&numsectors, &sectors,
			sizeof (sector_t), META_SECTOR);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSubsector);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsubsectors);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "subsectors");

	LUA_PushTaggableObjectArray(L, "lines",
			lib_iterateLines,
			lib_getLine,
			lib_numlines,
			tags_lines,
			&numlines, &lines,
			sizeof (line_t), META_LINE);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSide);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsides);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "sides");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getVertex);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numvertexes);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "vertexes");

#ifdef HAVE_LUA_SEGS
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getSeg);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numsegs);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "segs");

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getNode);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numnodes);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "nodes");
#endif

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getMapheaderinfo);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_nummapheaders);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "mapheaderinfo");
	return 0;
}
