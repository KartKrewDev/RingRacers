// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-local-race-1.c
/// \brief Race Mode Menu

#include "../k_menu.h"
#include "../m_cond.h" // Condition Sets

menuitem_t PLAY_RaceGamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Grand Prix", "Compete for the best rank over five rounds!",
		"MENUI004", {.routine = M_SetupDifficultySelect}, 0, 0},

	{IT_STRING | IT_CALL, "Match Race", "Play by your own rules in a specialized, single race!",
		"MENUI005", {.routine = M_SetupDifficultySelect}, 1, 0},

	{IT_STRING | IT_CALL, "Time Attack", "Race versus your best at \x87Gear 3\x80 - can you handle the heat?",
		"MENUI006", {.routine = M_LevelSelectInit}, 1, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_RaceGamemodesDef = KARTGAMEMODEMENU(PLAY_RaceGamemodesMenu, &PLAY_GamemodesDef);

void M_SetupRaceMenu(INT32 choice)
{
	if (choice != -1)
		PLAY_RaceGamemodesDef.prevMenu = currentMenu;

	// Time Attack disabled
	PLAY_RaceGamemodesMenu[2].status = IT_DISABLED;

	// Time Attack is 1P only
	if (cv_splitplayers.value <= 1
	 && M_SecretUnlocked(SECRET_TIMEATTACK, true))
	{
		PLAY_RaceGamemodesMenu[2].status = IT_STRING | IT_CALL;
	}

	if (choice != -1)
		M_SetupNextMenu(&PLAY_RaceGamemodesDef, false);
}
