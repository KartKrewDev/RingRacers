/// \file  menus/play-local-race-difficulty.c
/// \brief difficulty selection -- see drace_e

#include "../k_menu.h"

menuitem_t PLAY_RaceDifficulty[] =
{
	// For GP
	{IT_STRING | IT_CVAR, "Difficulty", "Select the game difficulty",
		NULL, {.cvar = &cv_dummygpdifficulty}, 0, 0},

	// Match Race
	{IT_STRING | IT_CVAR, "Difficulty", "Select the game speed",
		NULL, {.cvar = &cv_dummykartspeed}, 0, 0},

	// DISABLE THAT OPTION OUTSIDE OF MATCH RACE
	{IT_STRING2 | IT_CVAR, "CPU", "Set the difficulty of CPU players.",
		NULL, {.cvar = &cv_dummymatchbots}, 0, 0},
	{IT_STRING2 | IT_CVAR, "Racers", "Sets the number of racers, including players and CPU.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING2 | IT_CVAR, "Encore", "Enable or disable Encore mode",
		NULL, {.cvar = &cv_dummygpencore}, 0, 0},

	// For GP
	{IT_STRING | IT_CALL, "Cup Select", "Go on and select a cup!", NULL, {.routine = M_LevelSelectInit}, 2, GT_RACE},

	// Match Race
	{IT_STRING | IT_CALL, "Map Select", "Go on and select a race track!", NULL, {.routine = M_LevelSelectInit}, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_RaceDifficultyDef = {
	sizeof(PLAY_RaceDifficulty) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_RaceDifficulty,
	0, 0,
	0, 0,
	1, 5,
	M_DrawRaceDifficulty,
	NULL,
	NULL,
	NULL,
	NULL
};
