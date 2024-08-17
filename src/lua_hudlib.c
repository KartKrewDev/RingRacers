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
/// \file  lua_hudlib.c
/// \brief custom HUD rendering library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "r_defs.h"
#include "r_local.h"
#include "r_fps.h"
#include "st_stuff.h"
#include "g_game.h"
#include "i_video.h" // rendermode
#include "p_local.h" // camera_t
#include "screen.h" // screen width/height
#include "m_random.h" // m_random
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "hu_stuff.h"
#include "k_hud.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"
#include "lua_hook.h"

#define HUDONLY if (!hud_running) return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

boolean hud_running = false;
static UINT8 hud_enabled[(hud_MAX/8)+1];

static UINT8 camnum = 1;

// must match enum hud in lua_hud.h
static const char *const hud_disable_options[] = {
	"stagetitle",
	"textspectator",
	"crosshair",
	"score",
	"time",
	"gametypeinfo",	// Bumpers / Karma / Laps depending on gametype
	"minimap",
	"item",
	"position",
	"check",		// "CHECK" f-zero indicator
	"minirankings",	// Gametype rankings to the left
	"battlerankingsbumpers",	// bumper drawer for battle. Useful if you want to make a custom battle gamemode without bumpers being involved.
	"wanted",
	"speedometer",
	"freeplay",
	"rankings",

	"intermissiontally",
	"intermissionmessages",
	NULL};

enum patch {
	patch_valid = 0,
	patch_width,
	patch_height,
	patch_leftoffset,
	patch_topoffset
};
static const char *const patch_opt[] = {
	"valid",
	"width",
	"height",
	"leftoffset",
	"topoffset",
	NULL};

// alignment types for v.drawString
enum align {
	align_left = 0,
	align_center,
	align_right,
	align_small,
	align_smallcenter,
	align_smallright,
	align_thin,
	align_thincenter,
	align_thinright
};
static const char *const align_opt[] = {
	"left",
	"center",
	"right",
	"small",
	"small-center",
	"small-right",
	"thin",
	"thin-center",
	"thin-right",
	NULL};

// width types for v.stringWidth
enum widtht {
	widtht_normal = 0,
	widtht_small,
	widtht_thin
};
static const char *const widtht_opt[] = {
	"normal",
	"small",
	"thin",
	NULL};

enum cameraf {
	camera_chase = 0,
	camera_aiming,
	camera_x,
	camera_y,
	camera_z,
	camera_angle,
	camera_subsector,
	camera_floorz,
	camera_ceilingz,
	camera_radius,
	camera_height,
	camera_momx,
	camera_momy,
	camera_momz,
	camera_pan,
	camera_pitch,
	camera_pnum
};


static const char *const camera_opt[] = {
	"chase",
	"aiming",
	"x",
	"y",
	"z",
	"angle",
	"subsector",
	"floorz",
	"ceilingz",
	"radius",
	"height",
	"momx",
	"momy",
	"momz",
	"pan",
	"pitch",
	"pnum",
	NULL};

static int colormap_get(lua_State *L)
{
	const UINT8 *colormap = *((UINT8 **)luaL_checkudata(L, 1, META_COLORMAP));
	UINT32 i = luaL_checkinteger(L, 2);
	if (i >= 256)
		return luaL_error(L, "colormap index %d out of range (0 - %d)", i, 255);
	lua_pushinteger(L, colormap[i]);
	return 1;
}

static int patch_get(lua_State *L)
{
	patch_t *patch = *((patch_t **)luaL_checkudata(L, 1, META_PATCH));
	enum patch field = luaL_checkoption(L, 2, NULL, patch_opt);

	// patches are invalidated when switching renderers
	if (!patch) {
		if (field == patch_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return LUA_ErrInvalid(L, "patch_t");
	}

	switch (field)
	{
	case patch_valid:
		lua_pushboolean(L, patch != NULL);
		break;
	case patch_width:
		lua_pushinteger(L, patch->width);
		break;
	case patch_height:
		lua_pushinteger(L, patch->height);
		break;
	case patch_leftoffset:
		lua_pushinteger(L, patch->leftoffset);
		break;
	case patch_topoffset:
		lua_pushinteger(L, patch->topoffset);
		break;
	}
	return 1;
}

static int patch_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("patch_t") " struct cannot be edited by Lua.");
}

static int camera_get(lua_State *L)
{
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 1, META_CAMERA));
	enum cameraf field = luaL_checkoption(L, 2, NULL, camera_opt);

	// cameras should always be valid unless I'm a nutter
	I_Assert(cam != NULL);

	switch (field)
	{
	case camera_chase:
		lua_pushboolean(L, cam->chase);
		break;
	case camera_aiming:
		lua_pushinteger(L, cam->aiming);
		break;
	case camera_x:
		lua_pushinteger(L, cam->x);
		break;
	case camera_y:
		lua_pushinteger(L, cam->y);
		break;
	case camera_z:
		lua_pushinteger(L, cam->z);
		break;
	case camera_angle:
		lua_pushinteger(L, cam->angle);
		break;
	case camera_subsector:
		LUA_PushUserdata(L, cam->subsector, META_SUBSECTOR);
		break;
	case camera_floorz:
		lua_pushinteger(L, cam->floorz);
		break;
	case camera_ceilingz:
		lua_pushinteger(L, cam->ceilingz);
		break;
	case camera_radius:
		lua_pushinteger(L, cam->radius);
		break;
	case camera_height:
		lua_pushinteger(L, cam->height);
		break;
	case camera_momx:
		lua_pushinteger(L, cam->momx);
		break;
	case camera_momy:
		lua_pushinteger(L, cam->momy);
		break;
	case camera_momz:
		lua_pushinteger(L, cam->momz);
		break;
	case camera_pan:
		lua_pushinteger(L, cam->pan);
		break;
	case camera_pitch:
		lua_pushinteger(L, cam->pitch);
		break;
	case camera_pnum:
		lua_pushinteger(L, camnum);
		break;
	}
	return 1;
}

//
// lib_draw
//

static int libd_patchExists(lua_State *L)
{
	HUDONLY
	lua_pushboolean(L, W_LumpExists(luaL_checkstring(L, 1)));
	return 1;
}

static int libd_cachePatch(lua_State *L)
{
	HUDONLY
	LUA_PushUserdata(L, W_CachePatchLongName(luaL_checkstring(L, 1), PU_PATCH), META_PATCH);
	return 1;
}

// v.getSpritePatch(sprite, [frame, [angle, [rollangle]]])
static int libd_getSpritePatch(lua_State *L)
{
	UINT32 i; // sprite prefix
	UINT32 frame = 0; // 'A'
	UINT8 angle = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	HUDONLY

	if (lua_isnumber(L, 1)) // sprite number given, e.g. SPR_THOK
	{
		i = lua_tonumber(L, 1);
		if (i >= NUMSPRITES)
			return 0;
	}
	else if (lua_isstring(L, 1)) // sprite prefix name given, e.g. "THOK"
	{
		const char *name = lua_tostring(L, 1);
		for (i = 0; i < NUMSPRITES; i++)
			if (fastcmp(name, sprnames[i]))
				break;
		if (i >= NUMSPRITES)
			return 0;
	}
	else
		return 0;

	if (i == SPR_PLAY) // Use getSprite2Patch instead!
		return 0;

	sprdef = &sprites[i];

	// set frame number
	frame = luaL_optinteger(L, 2, 0);
	frame &= FF_FRAMEMASK; // ignore any bits that are not the actual frame, just in case
	if (frame >= sprdef->numframes)
		return 0;
	// set angle number
	sprframe = &sprdef->spriteframes[frame];
	angle = luaL_optinteger(L, 3, 1);

	// convert WAD editor angle numbers (1-8) to internal angle numbers (0-7)
	// keep 0 the same since we'll make it default to angle 1 (which is internally 0)
	// in case somebody didn't know that angle 0 really just maps all 8/16 angles to the same patch
	if (angle != 0)
		angle--;

	if (angle >= ((sprframe->rotate & SRF_3DGE) ? 16 : 8)) // out of range?
		return 0;

#ifdef ROTSPRITE
	if (lua_isnumber(L, 4))
	{
		// rotsprite?????
		angle_t rollangle = luaL_checkangle(L, 4);
		INT32 rot = R_GetRollAngle(rollangle);

		if (rot) {
			patch_t *rotsprite = Patch_GetRotatedSprite(sprframe, frame, angle, sprframe->flip & (1<<angle), true, &spriteinfo[i], rot);
			LUA_PushUserdata(L, rotsprite, META_PATCH);
			lua_pushboolean(L, false);
			lua_pushboolean(L, true);
			return 3;
		}
	}
#endif

	// push both the patch and it's "flip" value
	LUA_PushUserdata(L, W_CachePatchNum(sprframe->lumppat[angle], PU_SPRITE), META_PATCH);
	lua_pushboolean(L, (sprframe->flip & (1<<angle)) != 0);
	return 2;
}

// v.getSprite2Patch(skin, sprite, [super?,] [frame, [angle, [rollangle]]])
static int libd_getSprite2Patch(lua_State *L)
{
	INT32 i; // skin number
	playersprite_t j; // sprite2 prefix
	UINT32 frame = 0; // 'A'
	UINT8 angle = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	boolean super = false; // add FF_SPR2SUPER to sprite2 if true
	HUDONLY

	// get skin first!
	if (lua_isnumber(L, 1)) // find skin by number
	{
		i = lua_tonumber(L, 1);
		if (i < 0 || i >= MAXSKINS)
			return luaL_error(L, "skin number %d out of range (0 - %d)", i, MAXSKINS-1);
		if (i >= (demo.playback ? demo.numskins : numskins))
			return 0;
	}
	else // find skin by name
	{
		const char *name = luaL_checkstring(L, 1);
		i = R_SkinAvailable(name);
		if (i == -1)
			return 0;
	}

	if (demo.playback)
		i = demo.skinlist[i].mapping;

	lua_remove(L, 1); // remove skin now

	if (lua_isnumber(L, 1)) // sprite number given, e.g. SPR2_STIL
	{
		j = lua_tonumber(L, 1);
		if (j & FF_SPR2SUPER) // e.g. SPR2_STIL|FF_SPR2SUPER
		{
			super = true;
			j &= ~FF_SPR2SUPER; // remove flag so the next check doesn't fail
		}
		if (j >= free_spr2)
			return 0;
	}
	else if (lua_isstring(L, 1)) // sprite prefix name given, e.g. "STND"
	{
		const char *name = lua_tostring(L, 1);
		for (j = 0; j < free_spr2; j++)
			if (fastcmp(name, spr2names[j]))
				break;
		// if you want super flags you'll have to use the optional boolean following this
		if (j >= free_spr2)
			return 0;
	}
	else
		return 0;

	if (lua_isboolean(L, 2)) // optional boolean for superness
	{
		super = lua_toboolean(L, 2); // note: this can override FF_SPR2SUPER from sprite number
		lua_remove(L, 2); // remove
	}
	// if it's not boolean then just assume it's the frame number

	if (super)
		j |= FF_SPR2SUPER;

	j = P_GetSkinSprite2(&skins[i], j, NULL); // feed skin and current sprite2 through to change sprite2 used if necessary

	sprdef = &skins[i].sprites[j];

	// set frame number
	frame = luaL_optinteger(L, 2, 0);
	frame &= FF_FRAMEMASK; // ignore any bits that are not the actual frame, just in case
	if (frame >= sprdef->numframes)
		return 0;
	// set angle number
	sprframe = &sprdef->spriteframes[frame];
	angle = luaL_optinteger(L, 3, 1);

	// convert WAD editor angle numbers (1-8) to internal angle numbers (0-7)
	// keep 0 the same since we'll make it default to angle 1 (which is internally 0)
	// in case somebody didn't know that angle 0 really just maps all 8/16 angles to the same patch
	if (angle != 0)
		angle--;

	if (angle >= ((sprframe->rotate & SRF_3DGE) ? 16 : 8)) // out of range?
		return 0;

#ifdef ROTSPRITE
	if (lua_isnumber(L, 4))
	{
		// rotsprite?????
		angle_t rollangle = luaL_checkangle(L, 4);
		INT32 rot = R_GetRollAngle(rollangle);

		if (rot) {
			patch_t *rotsprite = Patch_GetRotatedSprite(sprframe, frame, angle, sprframe->flip & (1<<angle), true, &skins[i].sprinfo[j], rot);
			LUA_PushUserdata(L, rotsprite, META_PATCH);
			lua_pushboolean(L, false);
			lua_pushboolean(L, true);
			return 3;
		}
	}
#endif

	// push both the patch and it's "flip" value
	LUA_PushUserdata(L, W_CachePatchNum(sprframe->lumppat[angle], PU_SPRITE), META_PATCH);
	lua_pushboolean(L, (sprframe->flip & (1<<angle)) != 0);
	return 2;
}

static int libd_draw(lua_State *L)
{
	INT32 x, y, flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	patch = *((patch_t **)luaL_checkudata(L, 3, META_PATCH));
	if (!patch)
		return LUA_ErrInvalid(L, "patch_t");
	flags = luaL_optinteger(L, 4, 0);
	if (!lua_isnoneornil(L, 5))
		colormap = *((UINT8 **)luaL_checkudata(L, 5, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDraw(list, x, y, patch, flags, colormap);
	else
		V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, flags, patch, colormap);
	return 0;
}

static int libd_drawScaled(lua_State *L)
{
	fixed_t x, y, scale;
	INT32 flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	scale = luaL_checkinteger(L, 3);
	if (scale < 0)
		return luaL_error(L, "negative scale");
	patch = *((patch_t **)luaL_checkudata(L, 4, META_PATCH));
	if (!patch)
		return LUA_ErrInvalid(L, "patch_t");
	flags = luaL_optinteger(L, 5, 0);
	if (!lua_isnoneornil(L, 6))
		colormap = *((UINT8 **)luaL_checkudata(L, 6, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawScaled(list, x, y, scale, patch, flags, colormap);
	else
		V_DrawFixedPatch(x, y, scale, flags, patch, colormap);
	return 0;
}

static int libd_drawStretched(lua_State *L)
{
	fixed_t x, y, hscale, vscale;
	INT32 flags;
	patch_t *patch;
	UINT8 *colormap = NULL;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	hscale = luaL_checkinteger(L, 3);
	if (hscale < 0)
		return luaL_error(L, "negative horizontal scale");
	vscale = luaL_checkinteger(L, 4);
	if (vscale < 0)
		return luaL_error(L, "negative vertical scale");
	patch = *((patch_t **)luaL_checkudata(L, 5, META_PATCH));
	flags = luaL_optinteger(L, 6, 0);
	if (!lua_isnoneornil(L, 7))
		colormap = *((UINT8 **)luaL_checkudata(L, 7, META_COLORMAP));

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawStretched(list, x, y, hscale, vscale, patch, flags, colormap);
	else
		V_DrawStretchyFixedPatch(x, y, hscale, vscale, flags, patch, colormap);
	return 0;
}

// KART: draw patch on minimap from x, y coordinates on the map
// Sal: Let's please just merge the relevant info into the actual function, and have Lua call that...
// JugadorXEI: hey, sure.
static int libd_drawOnMinimap(lua_State *L)
{
	fixed_t x, y, scale;	// coordinates of the object
	patch_t *patch;	// patch we want to draw
	UINT8 *colormap = NULL;	// do we want to colormap this patch?
	boolean centered;	// the patch is centered and doesn't need readjusting on x/y coordinates.
	huddrawlist_h list;

	// variables used to replicate k_kart's mmap drawer:
	patch_t *AutomapPic;

	// variables used for actually drawing the icon:
	position_t amnumpos;
	INT32 minimapflags;
	fixed_t amxpos, amypos;
	INT32 mm_x, mm_y;
	fixed_t patchw, patchh;

	HUDONLY	// only run this function in hud hooks
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	scale = luaL_checkinteger(L, 3);
	patch = *((patch_t **)luaL_checkudata(L, 4, META_PATCH));
	if (!lua_isnoneornil(L, 5))
		colormap = *((UINT8 **)luaL_checkudata(L, 5, META_COLORMAP));
	centered = lua_optboolean(L, 6);
	
	// Draw the HUD only when playing in a level.
	// hu_stuff needs this, unlike st_stuff.
	if (gamestate != GS_LEVEL)
		return 0;

	if (R_GetViewNumber() != 0)
		return 0;
	
	AutomapPic = minimapinfo.minimap_pic;
	if (!AutomapPic)
	{
		return 0; // no pic, just get outta here
	}
	
	// Handle offsets and stuff.
	mm_x = MINI_X;
	mm_y = MINI_Y - SHORT(AutomapPic->topoffset);
	
	if (encoremode)
	{
		mm_x += SHORT(AutomapPic->leftoffset);
	}
	else
	{
		mm_x -= SHORT(AutomapPic->leftoffset);
	}

	// Minimap flags:
	minimapflags = K_GetMinimapSplitFlags(false)|K_GetMinimapTransFlags(false);

	// scale patch coords
	patchw = (SHORT(patch->width) / 2) * scale;
	patchh = (SHORT(patch->height) / 2) * scale;

	if (centered)
		patchw = patchh = 0;	// patch is supposedly already centered, don't butt in.

	amnumpos = K_GetKartObjectPosToMinimapPos(x, y);

	amxpos = amnumpos.x + (mm_x<<FRACBITS) - patchw;
	amypos = amnumpos.y + (mm_y<<FRACBITS) - patchh;

	// and NOW we can FINALLY DRAW OUR GOD DAMN PATCH :V
	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawScaled(list, amxpos, amypos, scale, patch, minimapflags, colormap);
	else
		V_DrawFixedPatch(amxpos, amypos, scale, minimapflags, patch, colormap);
	return 0;
}

static int libd_drawNum(lua_State *L)
{
	INT32 x, y, flags, num;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = luaL_checkinteger(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawNum(list, x, y, num, flags);
	else
		V_DrawTallNum(x, y, flags, num);
	return 0;
}

static int libd_drawPaddedNum(lua_State *L)
{
	INT32 x, y, flags, num, digits;
	huddrawlist_h list;

	HUDONLY
	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = labs(luaL_checkinteger(L, 3));
	digits = luaL_optinteger(L, 4, 2);
	flags = luaL_optinteger(L, 5, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawPaddedNum(list, x, y, num, digits, flags);
	else
		V_DrawPaddedTallNum(x, y, flags, num, digits);
	return 0;
}

static int libd_drawPingNum(lua_State *L)
{
	INT32 x, y, flags, num;
	UINT8 *colormap = NULL;
	huddrawlist_h list;
	HUDONLY
	x = luaL_checkfixed(L, 1);
	y = luaL_checkfixed(L, 2);
	num = luaL_checkinteger(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.
	if (!lua_isnoneornil(L, 5))
		colormap = *((UINT8 **)luaL_checkudata(L, 5, META_COLORMAP));

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawPingNum(list, x, y, flags, num, colormap);
	else
		V_DrawPingNum(x, y, flags, num, colormap);
	return 0;
}

static int libd_drawFill(lua_State *L)
{
	huddrawlist_h list;
	INT32 x = luaL_optinteger(L, 1, 0);
	INT32 y = luaL_optinteger(L, 2, 0);
	INT32 w = luaL_optinteger(L, 3, BASEVIDWIDTH);
	INT32 h = luaL_optinteger(L, 4, BASEVIDHEIGHT);
	INT32 c = luaL_optinteger(L, 5, 31);

	HUDONLY

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawFill(list, x, y, w, h, c);
	else
		V_DrawFill(x, y, w, h, c);
	return 0;
}

static int libd_fadeScreen(lua_State *L)
{
	UINT16 color = luaL_checkinteger(L, 1);
	UINT8 strength = luaL_checkinteger(L, 2);
	const UINT8 maxstrength = ((color & 0xFF00) ? 32 : 10);
	huddrawlist_h list;

	HUDONLY

	if (!strength)
		return 0;

	if (strength > maxstrength)
		return luaL_error(L, "%s fade strength %d out of range (0 - %d)", ((color & 0xFF00) ? "COLORMAP" : "TRANSMAP"), strength, maxstrength);

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (strength == maxstrength) // Allow as a shortcut for drawfill...
	{
		if (LUA_HUD_IsDrawListValid(list))
			LUA_HUD_AddDrawFill(list, 0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, ((color & 0xFF00) ? 31 : color));
		else
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, ((color & 0xFF00) ? 31 : color));
		return 0;
	}

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddFadeScreen(list, color, strength);
	else
		V_DrawFadeScreen(color, strength);
	return 0;
}

static int libd_drawString(lua_State *L)
{
	huddrawlist_h list;
	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, 0);
	enum align align = luaL_checkoption(L, 5, "left", align_opt);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	HUDONLY

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	// okay, sorry, this is kind of ugly
	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawString(list, x, y, str, flags, align);
	else
	switch(align)
	{
	// hu_font
	case align_left:
		V_DrawString(x, y, flags, str);
		break;
	case align_center:
		V_DrawCenteredString(x, y, flags, str);
		break;
	case align_right:
		V_DrawRightAlignedString(x, y, flags, str);
		break;
	// hu_font, 0.5x scale
	case align_small:
		V_DrawSmallString(x, y, flags, str);
		break;
	case align_smallcenter:
		V_DrawCenteredSmallString(x, y, flags, str);
		break;
	case align_smallright:
		V_DrawRightAlignedSmallString(x, y, flags, str);
		break;
	// tny_font
	case align_thin:
		V_DrawThinString(x, y, flags, str);
		break;
	case align_thincenter:
		V_DrawCenteredThinString(x, y, flags, str);
		break;
	case align_thinright:
		V_DrawRightAlignedThinString(x, y, flags, str);
		break;
	}
	return 0;
}

static int libd_drawTitleCardString(lua_State *L)
{

	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, 0);
	boolean rightalign = lua_optboolean(L, 5);
	INT32 timer = luaL_optinteger(L, 6, 0);
	INT32 threshold = luaL_optinteger(L, 7, 0);
	boolean p4 = lua_optboolean(L, 8);
	huddrawlist_h list;

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	HUDONLY
	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawTitleCardString(list, x, y, flags, str, rightalign, timer, threshold, p4);
	else
		V_DrawTitleCardString(x, y, str, flags, rightalign, timer, threshold, p4);
	return 0;
}

static int libd_drawKartString(lua_State *L)
{
	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, 0);
	huddrawlist_h list;

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	HUDONLY
	lua_getfield(L, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");
	list = (huddrawlist_h) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (LUA_HUD_IsDrawListValid(list))
		LUA_HUD_AddDrawKartString(list, x, y, str, flags);
	else
		V_DrawTimerString(x, y, flags, str);
	return 0;
}

static int libd_titleCardStringWidth(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	boolean p4 = lua_optboolean(L, 2);
	HUDONLY

	lua_pushinteger(L, V_TitleCardStringWidth(str, p4));
	return 1;
}

static int libd_stringWidth(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	INT32 flags = luaL_optinteger(L, 2, 0);
	enum widtht widtht = luaL_checkoption(L, 3, "normal", widtht_opt);

	HUDONLY
	switch(widtht)
	{
	case widtht_normal: // hu_font
		lua_pushinteger(L, V_StringWidth(str, flags));
		break;
	case widtht_small: // hu_font, 0.5x scale
		lua_pushinteger(L, V_SmallStringWidth(str, flags));
		break;
	case widtht_thin: // tny_font
		lua_pushinteger(L, V_ThinStringWidth(str, flags));
		break;
	}
	return 1;
}

static int libd_getColormap(lua_State *L)
{
	INT32 skinnum = TC_DEFAULT;
	skincolornum_t color = luaL_optinteger(L, 2, 0);
	UINT8* colormap = NULL;
	HUDONLY
	if (lua_isnoneornil(L, 1))
		; // defaults to TC_DEFAULT
	else if (lua_type(L, 1) == LUA_TNUMBER) // skin number
	{
		skinnum = (INT32)luaL_checkinteger(L, 1);
		if (skinnum >= MAXSKINS)
			return luaL_error(L, "skin number %d is out of range (>%d)", skinnum, MAXSKINS-1);
		else if (skinnum < 0 && skinnum > TC_DEFAULT)
			return luaL_error(L, "translation colormap index is out of range");
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 1);
		INT32 i = R_SkinAvailable(skinname);
		if (i != -1) // if -1, just default to TC_DEFAULT as above
			skinnum = i;
	}

	if (demo.playback && skinnum >= 0)
		skinnum = demo.skinlist[skinnum].mapping;

	// all was successful above, now we generate the colormap at last!

	colormap = R_GetTranslationColormap(skinnum, color, GTC_CACHE);
	LUA_PushUserdata(L, colormap, META_COLORMAP); // push as META_COLORMAP userdata, specifically for patches to use!
	return 1;
}

static int libd_getStringColormap(lua_State *L)
{
	INT32 flags = luaL_checkinteger(L, 1);
	UINT8* colormap = NULL;
	HUDONLY
	colormap = V_GetStringColormap(flags & V_CHARCOLORMASK);
	if (colormap) {
		LUA_PushUserdata(L, colormap, META_COLORMAP); // push as META_COLORMAP userdata, specifically for patches to use!
		return 1;
	}
	return 0;
}

static int libd_width(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.width); // push screen width
	return 1;
}

static int libd_height(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.height); // push screen height
	return 1;
}

static int libd_dupx(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dupx); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdupx); // push fixed point scale (position scale)
	return 2;
}

static int libd_dupy(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, vid.dupy); // push integral scale (patch scale)
	lua_pushfixed(L, vid.fdupy); // push fixed point scale (position scale)
	return 2;
}

static int libd_renderer(lua_State *L)
{
	HUDONLY
	switch (rendermode) {
		case render_opengl: lua_pushliteral(L, "opengl");   break; // OpenGL renderer
		case render_soft:   lua_pushliteral(L, "software"); break; // Software renderer
		default:            lua_pushliteral(L, "none");     break; // render_none (for dedicated), in case there's any reason this should be run
	}
	return 1;
}

// M_RANDOM
//////////////

static int libd_RandomFixed(lua_State *L)
{
	HUDONLY
	lua_pushfixed(L, M_RandomFixed());
	return 1;
}

static int libd_RandomByte(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, M_RandomByte());
	return 1;
}

static int libd_RandomKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);

	HUDONLY
	if (a > 65536)
		LUA_UsageWarning(L, "v.RandomKey: range > 65536 is undefined behavior");
	lua_pushinteger(L, M_RandomKey(a));
	return 1;
}

static int libd_RandomRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);

	HUDONLY
	if (b < a) {
		INT32 c = a;
		a = b;
		b = c;
	}
	if ((b-a+1) > 65536)
		LUA_UsageWarning(L, "v.RandomRange: range > 65536 is undefined behavior");
	lua_pushinteger(L, M_RandomRange(a, b));
	return 1;
}

// Macros.
static int libd_SignedRandom(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, M_SignedRandom());
	return 1;
}

static int libd_RandomChance(lua_State *L)
{
	fixed_t p = luaL_checkfixed(L, 1);
	HUDONLY
	lua_pushboolean(L, M_RandomChance(p));
	return 1;
}

// 30/10/18 Lat': Get st_translucency's value for HUD rendering as a normal V_xxTRANS int
// Could as well be thrown in global vars for ease of access but I guess it makes sense for it to be a HUD fn
static int libd_getlocaltransflag(lua_State *L)
{
	HUDONLY
	lua_pushinteger(L, (10-st_translucency)*V_10TRANS);
	return 1;
}

// Return the time elapsed for the previous frame, in tics.
static int libd_getDeltaTime(lua_State *L)
{
	HUDONLY
	lua_pushfixed(L, renderdeltatics);
	return 1;
}

static luaL_Reg lib_draw[] = {
	// cache
	{"patchExists", libd_patchExists},
	{"cachePatch", libd_cachePatch},
	{"getSpritePatch", libd_getSpritePatch},
	{"getSprite2Patch", libd_getSprite2Patch},
	{"getColormap", libd_getColormap},
	{"getStringColormap", libd_getStringColormap},
	// drawing
	{"draw", libd_draw},
	{"drawScaled", libd_drawScaled},
	{"drawStretched", libd_drawStretched},
	{"drawNum", libd_drawNum},
	{"drawPaddedNum", libd_drawPaddedNum},
	{"drawPingNum", libd_drawPingNum},
	{"drawFill", libd_drawFill},
	{"fadeScreen", libd_fadeScreen},
	{"drawString", libd_drawString},
	{"drawTitleCardString", libd_drawTitleCardString},
	{"drawKartString", libd_drawKartString},
	// misc
	{"stringWidth", libd_stringWidth},
	{"titleCardStringWidth", libd_titleCardStringWidth},
	// m_random
	{"RandomFixed",libd_RandomFixed},
	{"RandomByte",libd_RandomByte},
	{"RandomKey",libd_RandomKey},
	{"RandomRange",libd_RandomRange},
	{"SignedRandom",libd_SignedRandom}, // MACRO
	{"RandomChance",libd_RandomChance}, // MACRO
	// properties
	{"width", libd_width},
	{"height", libd_height},
	{"dupx", libd_dupx},
	{"dupy", libd_dupy},
	{"renderer", libd_renderer},
	{"localTransFlag", libd_getlocaltransflag},
	{"drawOnMinimap", libd_drawOnMinimap},
	{"getDeltaTime", libd_getDeltaTime},
	{NULL, NULL}
};

static int lib_draw_ref;

//
// lib_hud
//

// enable vanilla HUD element
static int lib_hudenable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] |= 1<<(option%8);
	return 0;
}

// disable vanilla HUD element
static int lib_huddisable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] &= ~(1<<(option%8));
	return 0;
}

// 30/10/18: Lat': How come this wasn't here before?
static int lib_hudenabled(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	if (hud_enabled[option/8] & (1<<(option%8)))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}

// add a HUD element for rendering
extern int lib_hudadd(lua_State *L);

static luaL_Reg lib_hud[] = {
	{"enable", lib_hudenable},
	{"disable", lib_huddisable},
	{"enabled", lib_hudenabled},
	{"add", lib_hudadd},
	{NULL, NULL}
};

//
//
//

int LUA_HudLib(lua_State *L)
{
	memset(hud_enabled, 0xff, (hud_MAX/8)+1);

	lua_newtable(L);
	luaL_register(L, NULL, lib_draw);
	lib_draw_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	luaL_newmetatable(L, META_COLORMAP);
		lua_pushcfunction(L, colormap_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_newmetatable(L, META_PATCH);
		lua_pushcfunction(L, patch_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, patch_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	luaL_newmetatable(L, META_CAMERA);
		lua_pushcfunction(L, camera_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L,1);

	luaL_register(L, "hud", lib_hud);
	return 0;
}

boolean LUA_HudEnabled(enum hud option)
{
	if (!gL || hud_enabled[option/8] & (1<<(option%8)))
		return true;
	return false;
}

void LUA_SetHudHook(int hook, huddrawlist_h list)
{
	lua_getref(gL, lib_draw_ref);

	lua_pushlightuserdata(gL, list);
	lua_setfield(gL, LUA_REGISTRYINDEX, "HUD_DRAW_LIST");

	switch (hook)
	{
		case HUD_HOOK(game):
			camnum = R_GetViewNumber();

			LUA_PushUserdata(gL, stplyr, META_PLAYER);
			LUA_PushUserdata(gL, &camera[camnum], META_CAMERA);

			camnum++; // for compatibility
			break;

		case HUD_HOOK(titlecard):
			LUA_PushUserdata(gL, stplyr, META_PLAYER);
			lua_pushinteger(gL, lt_ticker);
			lua_pushinteger(gL, (lt_endtime + TICRATE));
			break;
	}
}
