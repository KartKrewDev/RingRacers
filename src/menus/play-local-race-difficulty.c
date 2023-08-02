/// \file  menus/play-local-race-difficulty.c
/// \brief difficulty selection -- see drace_e

#include "../k_menu.h"
#include "../m_cond.h" // Condition Sets

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

	{IT_STRING2 | IT_CVAR, "Encore", "Enable or disable Encore mode",
		"MENUI005", {.cvar = &cv_dummygpencore}, 0, 0},

	// For GP
	{IT_STRING | IT_CALL, "Cup Select", "Go on and select a cup!", "MENUI004", {.routine = M_LevelSelectInit}, 2, GT_RACE},

	// Match Race
	{IT_STRING | IT_CALL, "Map Select", "Go on and select a race track!", "MENUI005", {.routine = M_LevelSelectInit}, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_RaceDifficultyDef = {
	sizeof(PLAY_RaceDifficulty) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_RaceDifficulty,
	0, 0,
	0, 0,
	0,
	NULL,
	1, 5,
	M_DrawRaceDifficulty,
	NULL,
	NULL,
	NULL,
	NULL
};

consvar_t cv_dummygpdifficulty = CVAR_INIT ("dummygpdifficulty", "Normal", CV_HIDDEN, gpdifficulty_cons_t, NULL);
consvar_t cv_dummykartspeed = CVAR_INIT ("dummykartspeed", "Normal", CV_HIDDEN, dummykartspeed_cons_t, NULL);
consvar_t cv_dummygpencore = CVAR_INIT ("dummygpencore", "Off", CV_HIDDEN, CV_OnOff, NULL);

static CV_PossibleValue_t dummymatchbots_cons_t[] = {
	{0, "Off"},
	{1, "Lv.1"},
	{2, "Lv.2"},
	{3, "Lv.3"},
	{4, "Lv.4"},
	{5, "Lv.5"},
	{6, "Lv.6"},
	{7, "Lv.7"},
	{8, "Lv.8"},
	{9, "Lv.9"},
	{10, "Lv.10"},
	{11, "Lv.11"},
	{12, "Lv.12"},
	{13, "Lv.MAX"},
	{0, NULL}
};
consvar_t cv_dummymatchbots = CVAR_INIT ("dummymatchbots", "Off", CV_HIDDEN, dummymatchbots_cons_t, NULL);

void M_SetupDifficultyOptions(INT32 choice)
{
	PLAY_RaceDifficulty[drace_gpdifficulty].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrkartspeed].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrcpu].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrracers].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_encore].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_cupselect].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mapselect].status = IT_DISABLED;

	if (choice)		// Match Race
	{
		PLAY_RaceDifficulty[drace_mrkartspeed].status = IT_STRING|IT_CVAR; // Kart Speed
		PLAY_RaceDifficulty[drace_mrcpu].status = IT_STRING2|IT_CVAR;	// CPUs on/off
		PLAY_RaceDifficulty[drace_mrracers].status = IT_STRING2|IT_CVAR;	// CPU amount
		PLAY_RaceDifficulty[drace_mapselect].status = IT_STRING|IT_CALL;	// Level Select (Match Race)
		PLAY_RaceDifficultyDef.lastOn = drace_mapselect;	// Select map select by default.

		if (M_SecretUnlocked(SECRET_ENCORE, true))
		{
			PLAY_RaceDifficulty[drace_encore].status = IT_STRING2|IT_CVAR;	// Encore on/off
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
