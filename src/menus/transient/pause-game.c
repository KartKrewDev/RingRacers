/// \file  menus/transient/pause-game.c
/// \brief In-game/pause menus

#include "../../k_menu.h"

// ESC pause menu
// Since there's no descriptions to each item, we'll use the descriptions as the names of the patches we want to draw for each option :)

menuitem_t PAUSE_Main[] =
{

	{IT_STRING | IT_CALL, "ADDONS", "M_ICOADD",
		NULL, {.routine = M_Addons}, 0, 0},

	{IT_STRING | IT_KEYHANDLER, "GAMETYPE", "M_ICOGAM",
		NULL, {.routine = M_HandlePauseMenuGametype}, 0, 0},

	{IT_STRING | IT_CALL, "CHANGE MAP", "M_ICOMAP",
		NULL, {.routine = M_LevelSelectInit}, 0, -1},

	{IT_STRING | IT_CALL, "RESTART MAP", "M_ICORE",
		NULL, {.routine = M_RestartMap}, 0, 0},

	{IT_STRING | IT_CALL, "TRY AGAIN", "M_ICORE",
		NULL, {.routine = M_TryAgain}, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_STRING | IT_CALL, "DISCORD REQUESTS", "M_ICODIS",
		NULL, {NULL}, 0, 0},
#endif

	{IT_STRING | IT_CALL, "RESUME GAME", "M_ICOUNP",
		NULL, {.routine = M_QuitPauseMenu}, 0, 0},

	{IT_STRING | IT_CALL, "SPECTATE", "M_ICOSPC",
		NULL, {.routine = M_ConfirmSpectate}, 0, 0},

	{IT_STRING | IT_CALL, "ENTER GAME", "M_ICOENT",
		NULL, {.routine = M_ConfirmEnterGame}, 0, 0},

	{IT_STRING | IT_CALL, "CANCEL JOIN", "M_ICOSPC",
		NULL, {.routine = M_ConfirmSpectate}, 0, 0},

	{IT_STRING | IT_SUBMENU, "JOIN OR SPECTATE", "M_ICOENT",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "PLAYER SETUP", "M_ICOCHR",
		NULL, {.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "OPTIONS", "M_ICOOPT",
		NULL, {.routine = M_InitOptions}, 0, 0},

	{IT_STRING | IT_CALL, "EXIT GAME", "M_ICOEXT",
		NULL, {.routine = M_EndGame}, 0, 0},
};

menu_t PAUSE_MainDef = {
	sizeof (PAUSE_Main) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_Main,
	0, 0,
	0, 0,
	1, 10,	// For transition with some menus!
	M_DrawPause,
	M_PauseTick,
	NULL,
	NULL,
	M_PauseInputs
};
