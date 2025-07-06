// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-online-room-select.c
/// \brief MULTIPLAYER ROOM SELECT MENU

#include "../k_menu.h"
#include "../s_sound.h"
#include "../m_cond.h"

menuitem_t PLAY_MP_RoomSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_MPRoomSelect}, 0, 0},
};

menu_t PLAY_MP_RoomSelectDef = {
	sizeof (PLAY_MP_RoomSelect) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_RoomSelect,
	0, 0,
	0, 0,
	0,
	"NETMD2",
	0, 0,
	M_DrawMPRoomSelect,
	M_DrawEggaChannel,
	M_MPRoomSelectTick,
	NULL,
	NULL,
	NULL
};

void M_MPRoomSelect(INT32 choice)
{
	const UINT8 pid = 0;
	(void) choice;

	if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
	}
	else if (M_MenuConfirmPressed(pid))
	{
		M_ServersMenu(0);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_lr != 0)
	{
		mpmenu.room ^= 1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
}

void M_MPRoomSelectTick(void)
{
	mpmenu.ticker++;
}

void M_MPRoomSelectInit(INT32 choice)
{
	(void)choice;

	if (modifiedgame)
	{
		M_StartMessage("Server Browser & Add-Ons", M_GetText("You have add-ons loaded.\nYou won't be able to join netgames!\n\nTo play online, restart the game\nand don't load any addons.\n\n\"Dr. Robotnik's Ring Racers\" will\nautomatically add everything\nyou need when you join.\n"), NULL, MM_NOTHING, NULL, NULL);
	}

	// The following behaviour is affected by modifiedgame despite the above restriction.
	// It's a sanity check were that to be removed, wheither by us or by a modified client.
	// "wheither"? That typo rules, I'm keeping that ~toast 280823

	// thanks toaster - Tyron 2025-07-02

	mpmenu.room = (modifiedgame == true) ? 1 : 0;
	mpmenu.ticker = 0;
	mpmenu.servernum = 0;
	mpmenu.scrolln = 0;
	mpmenu.slide = 0;

#ifndef TESTERS
	if ((modifiedgame == true) || (M_SecretUnlocked(SECRET_ADDONS, true) == false))
	{
		M_ServersMenu(0);
		return;
	}
#endif // TESTERS

	M_SetupNextMenu(&PLAY_MP_RoomSelectDef, false);
}
