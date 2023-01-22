/// \file  menus/play-local-race-1.c
/// \brief Race Mode Menu

#include "../k_menu.h"
#include "../m_cond.h" // Condition Sets

menuitem_t PLAY_RaceGamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Grand Prix", "Compete for the best rank over five races!",
		NULL, {.routine = M_SetupDifficultySelect}, 0, 0},

	{IT_STRING | IT_CALL, "Match Race", "Play by your own rules in a specialized, single race!",
		"MENIMG01", {.routine = M_SetupDifficultySelect}, 1, 0},

	{IT_STRING | IT_CALL, "Time Attack", "Record your best time on any track!",
		NULL, {.routine = M_LevelSelectInit}, 1, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_RaceGamemodesDef = KARTGAMEMODEMENU(PLAY_RaceGamemodesMenu, &PLAY_GamemodesDef);

void M_SetupRaceMenu(INT32 choice)
{
	(void)choice;

	PLAY_RaceGamemodesDef.prevMenu = currentMenu;

	// Time Attack disabled
	PLAY_RaceGamemodesMenu[2].status = IT_DISABLED;

	// Time Attack is 1P only
	if (cv_splitplayers.value <= 1
	 && M_SecretUnlocked(SECRET_TIMEATTACK, true))
	{
		PLAY_RaceGamemodesMenu[2].status = IT_STRING | IT_CALL;
	}

	M_SetupNextMenu(&PLAY_RaceGamemodesDef, false);
}
