// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-gameplay-1.c
/// \brief Gameplay Options -- see gopt_e

#include "../k_menu.h"
#include "../m_cond.h"

menuitem_t OPTIONS_Gameplay[] =
{

	{IT_HEADER, "Race...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Game Speed", "Gear for the next map.",
		NULL, {.cvar = &cv_kartspeed}, 0, 0},

	{IT_STRING | IT_CVAR, "Base Lap Count", "How many laps must be completed per race.",
		NULL, {.cvar = &cv_numlaps}, 0, 0},

	{IT_STRING | IT_CVAR, "Frantic Items", "Make item odds crazier with more powerful items!",
		NULL, {.cvar = &cv_kartfrantic}, 0, 0},

	{IT_STRING | IT_CVAR, "Encore Mode", "Play in Encore Mode next map.",
		NULL, {.cvar = &cv_kartencore}, 0, 0},

	{IT_STRING | IT_CVAR, "Exit Countdown", "How long players have to finish after 1st place finishes.",
		NULL, {.cvar = &cv_countdowntime}, 0, 0},

	{IT_STRING | IT_CVAR, "Last Place Explodes", "Once the standings are complete, give the race an explosive finish!",
		NULL, {.cvar = &cv_karteliminatelast}, 0, 0},


	{IT_HEADER, "Battle...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Time Limit", "Time limit for Battle rounds.",
		NULL, {.cvar = &cv_timelimit}, 0, 0},

	{IT_STRING | IT_CVAR, "Point Limit", "How many strikes it takes to win a Battle.",
		NULL, {.cvar = &cv_pointlimit}, 0, 0},

	{IT_STRING | IT_CVAR, "Starting Bumpers", "How many bumpers players start with in Battle.",
		NULL, {.cvar = &cv_kartbumpers}, 0, 0},


	{IT_SPACE | IT_DYBIGSPACE, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Random Item Toggles...", "Which items appear in your games.",
		NULL, {.submenu = &OPTIONS_GameplayItemsDef}, 0, 0},

};

static void init_routine(void)
{
	OPTIONS_Gameplay[gopt_encore].status = M_SecretUnlocked(SECRET_ENCORE, true) ? IT_STRING | IT_CVAR : IT_DISABLED;
}

menu_t OPTIONS_GameplayDef = {
	sizeof (OPTIONS_Gameplay) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Gameplay,
	48, 80,
	SKINCOLOR_SCARLET, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	init_routine,
	NULL,
	NULL,
};
