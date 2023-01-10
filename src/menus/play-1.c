/// \file  menus/play-1.c
/// \brief Play Menu

#include "../k_menu.h"

menuitem_t PLAY_MainMenu[] =
{
	{IT_STRING | IT_CALL, "Local Play", "Play only on this computer.",
		NULL, {.routine = M_SetupGametypeMenu}, 0, 0},

	{IT_STRING | IT_CALL, "Online", "Connect to other computers.",
		NULL, {.routine = M_MPOptSelectInit}, /*M_MPRoomSelectInit,*/ 0, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_MainDef = KARTGAMEMODEMENU(PLAY_MainMenu, &PLAY_CharSelectDef);
