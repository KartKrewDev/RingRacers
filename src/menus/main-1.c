/// \file  menus/main-1.c
/// \brief Main Menu

// ==========================================================================
// ORGANIZATION START.
// ==========================================================================
// Note: Never should we be jumping from one category of menu options to another
//       without first going to the Main Menu.
// Note: Ignore the above if you're working with the Pause menu.
// Note: (Prefix)_MainMenu should be the target of all Main Menu options that
//       point to submenus.

#include "../k_menu.h"

menuitem_t MainMenu[] =
{
	{IT_STRING | IT_CALL, "Play",
		"Cut to the chase and start the race!", NULL,
		{.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Extras",
		"Check out some bonus features.", "MENUI001",
		{.routine = M_InitExtras}, 0, 0},

	{IT_STRING, "Options",
		"Configure your controls, settings, and preferences.", NULL,
		{.routine = M_InitOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Quit",
		"Exit \"Dr. Robotnik's Ring Racers\".", NULL,
		{.routine = M_QuitSRB2}, 0, 0},
};

menu_t MainDef = KARTGAMEMODEMENU(MainMenu, NULL);
