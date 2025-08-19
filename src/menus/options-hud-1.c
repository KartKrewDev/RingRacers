// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-hud-1.c
/// \brief HUD Options

#include "../k_menu.h"
#include "../r_main.h"	// cv_showhud
#include "../v_video.h" // cv_constextsize

menuitem_t OPTIONS_HUD[] =
{

	{IT_STRING | IT_CVAR, "Show HUD (F3)", "Toggles the Heads-Up display. Great for taking screenshots!",
		NULL, {.cvar = &cv_showhud}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Speedometer", "Choose which speed unit to display on the speedometer.",
		NULL, {.cvar = &cv_kartspeedometer}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Show FPS", "Displays the framerate in the lower right corner of the screen.",
		NULL, {.cvar = &cv_ticrate}, 0, 0},

	{IT_STRING | IT_CVAR, "Show \"FOCUS LOST\"", "Displays \"FOCUS LOST\" when the game cannot accept inputs.",
		NULL, {.cvar = &cv_showfocuslost}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Race Splits", "Display time comparisons during races. Next = closest leading player.",
		NULL, {.cvar = &cv_racesplits}, 0, 0},

	{IT_STRING | IT_CVAR, "Time Attack Splits", "Display time comparisons during Time Attack. Next = closest leading player.",
		NULL, {.cvar = &cv_attacksplits}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Online Chat Options...", "Visual options for the online chat box.",
		NULL, {.submenu = &OPTIONS_HUDOnlineDef}, 0, 0},
};

menu_t OPTIONS_HUDDef = {
	sizeof (OPTIONS_HUD) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_HUD,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
