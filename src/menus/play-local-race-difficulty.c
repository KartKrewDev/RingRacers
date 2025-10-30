// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-local-race-difficulty.c
/// \brief difficulty selection -- see drace_e

#include "../i_time.h"
#include "../k_menu.h"
#include "../m_cond.h" // Condition Sets
#include "../s_sound.h"
#include "../sounds.h"

boolean interceptedDefaultDifficulty = false;

menuitem_t PLAY_RaceDifficulty[] =
{
	// For GP
	{IT_STRING | IT_CVAR, "Difficulty", "Select the game difficulty.",
		"MENUI004", {.cvar = &cv_dummygpdifficulty}, 0, 0},

	// Match Race
	{IT_STRING | IT_CVAR, "Difficulty", "Select the game speed.",
		"MENUI005", {.cvar = &cv_dummykartspeed}, 0, 0},

	// DISABLE THAT OPTION OUTSIDE OF MATCH RACE
	{IT_STRING2 | IT_CVAR, "CPU", "Set the difficulty of CPU players.",
		"MENUI005", {.cvar = &cv_dummymatchbots}, 0, 0},
	{IT_STRING2 | IT_CVAR, "Racers", "Sets the number of racers, including players and CPU.",
		"MENUI005", {.cvar = &cv_maxplayers}, 0, 0},

	{IT_PATCH | IT_SPACE, NULL, NULL,
		"ITEMTOGG", {NULL}, 222, MBT_Y},
	{IT_PATCH | IT_SPACE, NULL, NULL,
		"ENCRTOGG", {.cvar = &cv_dummygpencore}, 264, MBT_Z},

	// For GP
	{IT_STRING | IT_CALL, "Cup Select", "Go on and select a cup!", "MENUI004", {.routine = M_LevelSelectInit}, 2, GT_RACE},

	// Match Race
	{IT_STRING | IT_CALL, "Map Select", "Go on and select a course!", "MENUI005", {.routine = M_LevelSelectInit}, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_anim_t g_drace_timer = {0, -1};

static void tick_routine(void)
{
	if (g_drace_timer.dist == -1 || I_GetTime() - g_drace_timer.start < 4)
	{
		return;
	}

	S_StartSound(NULL, sfx_s23b);

	switch (g_drace_timer.dist)
	{
		case drace_mritems:
			M_SetupNextMenu(&OPTIONS_GameplayItemsDef, false);
			optionsmenu.ticker = 0;
			M_OptionsTick();
			break;

		case drace_encore:
			CV_AddValue(&cv_dummygpencore, 1);
			break;
	}

	g_drace_timer.dist = -1;
}

static boolean input_routine(INT32 ch)
{
	if (g_drace_timer.dist != -1)
	{
		return true;
	}

	UINT8 pid = 0;
	(void)ch;

	int i;
	for (i = 0; i < currentMenu->numitems; ++i)
	{
		const menuitem_t *it = &currentMenu->menuitems[i];
		if ((it->status & IT_DISPLAY) == IT_PATCH && M_MenuButtonPressed(pid, it->mvar2))
		{
			g_drace_timer.start = I_GetTime();
			g_drace_timer.dist = i;
			S_StartSound(NULL, sfx_s3k5b);
			return true;
		}
	}

	return false;
}

menu_t PLAY_RaceDifficultyDef = {
	sizeof(PLAY_RaceDifficulty) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_RaceDifficulty,
	0, 0,
	drace_boxend, 0,
	0,
	NULL,
	1, 5,
	M_DrawRaceDifficulty,
	NULL,
	tick_routine,
	NULL,
	NULL,
	input_routine
};

void M_SetupDifficultyOptions(INT32 choice)
{
	PLAY_RaceDifficulty[drace_gpdifficulty].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrkartspeed].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrcpu].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrracers].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mritems].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_encore].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_cupselect].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mapselect].status = IT_DISABLED;

	if (!interceptedDefaultDifficulty)
	{
		if (M_SecretUnlocked(SECRET_HARDSPEED, true))
		{
			CV_SetValue(&cv_dummygpdifficulty, KARTSPEED_NORMAL);
			CV_SetValue(&cv_dummykartspeed, KARTSPEED_NORMAL);
		}
		else
		{
			CV_SetValue(&cv_dummygpdifficulty, KARTSPEED_EASY);
			CV_SetValue(&cv_dummykartspeed, KARTSPEED_EASY);
		}
	}

	interceptedDefaultDifficulty = true;

	if (choice)		// Match Race
	{
		PLAY_RaceDifficulty[drace_mrkartspeed].status = IT_STRING|IT_CVAR; // Kart Speed
		PLAY_RaceDifficulty[drace_mrcpu].status = IT_STRING2|IT_CVAR;	// CPUs on/off
		PLAY_RaceDifficulty[drace_mrracers].status = IT_STRING2|IT_CVAR;	// CPU amount
		PLAY_RaceDifficulty[drace_mapselect].status = IT_STRING|IT_CALL;	// Level Select (Match Race)
		PLAY_RaceDifficultyDef.lastOn = drace_mapselect;	// Select map select by default.

		PLAY_RaceDifficulty[drace_mritems].status = IT_PATCH|IT_SPACE;	// Item Toggles

		if (M_SecretUnlocked(SECRET_ENCORE, true))
		{
			PLAY_RaceDifficulty[drace_encore].status = IT_PATCH|IT_SPACE;	// Encore on/off
		}
	}
	else			// GP
	{
		PLAY_RaceDifficulty[drace_gpdifficulty].status = IT_STRING|IT_CVAR; // Difficulty
		PLAY_RaceDifficulty[drace_cupselect].status = IT_STRING|IT_CALL;	// Level Select (GP)
		PLAY_RaceDifficultyDef.lastOn = drace_cupselect;	// Select cup select by default.
	}
}

void M_SetupDifficultySelect(INT32 choice)
{
	(void)choice;

	// setup the difficulty menu and then remove choices depending on choice
	PLAY_RaceDifficultyDef.prevMenu = currentMenu;

	M_SetupDifficultyOptions(currentMenu->menuitems[itemOn].mvar1);

	M_SetupNextMenu(&PLAY_RaceDifficultyDef, false);
}

void Dummygpdifficulty_OnChange(void);
void Dummygpdifficulty_OnChange(void)
{
	const char *tooltip = NULL;

	switch (cv_dummygpdifficulty.value)
	{
		case KARTSPEED_EASY:
			tooltip = "Low-stakes racing at \x83Gear 1""\x80"". \x83No placement requirements\x80.";
			break;
		case KARTSPEED_NORMAL:
			tooltip = "Aim for the prize at\x82 Gear 2\x80. Place \x82""within the top half\x80 to advance!";
			break;
		case KARTSPEED_HARD:
			tooltip = "Challenge fierce competition at\x87 Gear 3\x80. For\x87 thrill-seekers!";
			break;
		case KARTGP_MASTER:
			tooltip = "Let's go crazy! Take on brutal CPUs at\x87 Gear 3\x80: for\x85 lunatics only!";
			break;
	}

	PLAY_RaceDifficulty[drace_gpdifficulty].tooltip = tooltip;
}
void DummyKartSpeed_OnChange(void);
void DummyKartSpeed_OnChange(void)
{
	const char *tooltip = NULL;

	switch (cv_dummykartspeed.value)
	{
		case KARTSPEED_EASY:
			tooltip = "Boosts are reduced for ""\x83""casual play""\x80""! The ""\x83""main event""\x80"" when ""\x83""inviting friends""\x80"" over!";
			break;
		case KARTSPEED_NORMAL:
			tooltip = "Designed around ""\x82""competition""\x80"". Even playing from behind ""\x82""requires good driving""\x80"".";
			break;
		case KARTSPEED_HARD:
			tooltip = "An ""\x87""extra ""\x80""kick for various challenges. ""\x85""Not as tightly balanced""\x80"", but fun in bursts.";
			break;
	}

	PLAY_RaceDifficulty[drace_mrkartspeed].tooltip = tooltip;
}