/// \file  menus/play-local-1.c
/// \brief Local Play, gamemode selection menu

#include "../k_menu.h"

menuitem_t PLAY_GamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Race", "A contest to see who's the fastest of them all!",
		NULL, {.routine = M_SetupRaceMenu}, 0, 0},

	{IT_STRING | IT_CALL, "Battle", "It's last kart standing in this free-for-all!",
		"MENIMG00", {.routine = M_LevelSelectInit}, 0, GT_BATTLE},

	{IT_STRING | IT_CALL, "Capsules", "Bust up all of the capsules in record time!",
		NULL, {.routine = M_LevelSelectInit}, 1, GT_BATTLE},

	{IT_STRING | IT_CALL, "Special", "Strike your target and secure the prize!",
		NULL, {.routine = M_LevelSelectInit}, 1, GT_SPECIAL},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_GamemodesDef = KARTGAMEMODEMENU(PLAY_GamemodesMenu, &PLAY_MainDef);
