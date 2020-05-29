// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hud.h
/// \brief HUD enable/disable flags for Lua scripting

enum hud {
	hud_stagetitle = 0,
	hud_textspectator,

	hud_time,
<<<<<<< HEAD
	hud_gametypeinfo,
	hud_minimap,
	hud_item,
	hud_position,
	hud_check,			// "CHECK" f-zero indicator
	hud_minirankings,	// Rankings to the left
	hud_battlebumpers,	// mini rankings battle bumpers.
	hud_battlefullscreen,	// battle huge text (WAIT, WIN, LOSE ...) + karma comeback time
	hud_battlecomebacktimer,	// comeback timer in battlefullscreen. separated for ease of use.
	hud_wanted,
	hud_speedometer,
	hud_freeplay,
	hud_rankings,		// Tab rankings

=======
	hud_rings,
	hud_lives,
	// Match / CTF / Tag / Ringslinger
	hud_weaponrings,
	hud_powerstones,
	hud_teamscores,
	// NiGHTS mode
	hud_nightslink,
	hud_nightsdrill,
	hud_nightsspheres,
	hud_nightsscore,
	hud_nightstime,
	hud_nightsrecords,
	// TAB scores overlays
	hud_rankings,
	hud_coopemeralds,
	hud_tokens,
	hud_tabemblems,
	// Intermission
	hud_intermissiontally,
	hud_intermissionmessages,
>>>>>>> srb2/next
	hud_MAX
};

extern boolean hud_running;

boolean LUA_HudEnabled(enum hud option);

void LUAh_GameHUD(player_t *stplyr);
void LUAh_ScoresHUD(void);
void LUAh_TitleHUD(void);
void LUAh_TitleCardHUD(player_t *stplayr);
void LUAh_IntermissionHUD(void);
