// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2022 by Sonic Team Junior.
// Copyright (C) 2016 by John "JTE" Muniz.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hud.h
/// \brief HUD enable/disable flags for Lua scripting

#ifndef __LUA_HUD_H__
#define __LUA_HUD_H__

#include "lua_hudlib_drawlist.h"

#ifdef __cplusplus
extern "C" {
#endif

enum hud {
	hud_stagetitle = 0,
	hud_textspectator,
	hud_crosshair,
	// Singleplayer / Co-op
	hud_score,
	hud_time,
	hud_gametypeinfo,
	hud_minimap,
	hud_item,
	hud_position,
	hud_names,			// online nametags
	hud_check,			// "CHECK" f-zero indicator
	hud_minirankings,	// Rankings to the left
	hud_battlebumpers,	// mini rankings battle bumpers.
	hud_wanted,
	hud_speedometer,
	hud_freeplay,
	hud_rankings,		// Tab rankings
	hud_rings,			// Rings and Spheres HUD element

	// Intermission
	hud_intermissiontally,
	hud_intermissionmessages,
	hud_MAX
};

extern boolean hud_running;

boolean LUA_HudEnabled(enum hud option);

void LUA_SetHudHook(int hook, huddrawlist_h list);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LUA_HUD_H__
