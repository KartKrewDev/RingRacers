// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-1.c
/// \brief Play Menu

#include "../k_menu.h"
#include "../m_cond.h"

menuitem_t PLAY_MainMenu[] =
{
	{IT_STRING | IT_CALL, "Local Play", "Play only on this computer.",
		"MENUI002", {.routine = M_SetupGametypeMenu}, 0, 0},

	{IT_STRING | IT_CALL, "Online", NULL,
		"MENUI009", {.routine = M_MPOptSelectInit}, /*M_MPRoomSelectInit,*/ 0, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_MainDef = KARTGAMEMODEMENU(PLAY_MainMenu, &PLAY_CharSelectDef);

void M_SetupPlayMenu(INT32 choice)
{
#ifdef TESTERS
	(void)choice;
#else
	if (choice != -1)
		PLAY_MainDef.prevMenu = currentMenu;

	// Enable/disable online play.
	if (!M_SecretUnlocked(SECRET_ONLINE, true))
	{
		PLAY_MainMenu[1].status = IT_TRANSTEXT2 | IT_CALL;
		PLAY_MainMenu[1].tooltip = "You'll need experience to play over the internet!";
	}
	else
	{
		PLAY_MainMenu[1].status = IT_STRING | IT_CALL;
		PLAY_MainMenu[1].tooltip = "Connect to other computers over the internet.";
	}

	if (choice != -1)
		M_SetupNextMenu(&PLAY_MainDef, false);
#endif
}
