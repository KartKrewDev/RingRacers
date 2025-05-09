// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/manual.c
/// \brief Manual

#include "../../k_menu.h"
#include "../../s_sound.h"

menuitem_t MISC_Manual[] = {
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL00", NULL, NULL, {.routine = M_HandleImageDef}, 0, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL01", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL02", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL03", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL04", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL05", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL06", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL07", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL08", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL09", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL10", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL11", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL12", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL99", NULL, NULL, {.routine = M_HandleImageDef}, 0, 0},
};

menu_t MISC_ManualDef = IMAGEDEF(MISC_Manual);

// Handles the ImageDefs.  Just a specialized function that
// uses left and right movement.
void M_HandleImageDef(INT32 choice)
{
	const UINT8 pid = 0;
	boolean exitmenu = false;
	(void) choice;

	if (menucmd[pid].dpad_lr > 0)
	{
		if (itemOn >= (INT16)(currentMenu->numitems-1))
			return;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		itemOn++;
	}
	else if (menucmd[pid].dpad_lr < 0)
	{
		if (!itemOn)
			return;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		itemOn--;
	}
	else if (M_MenuConfirmPressed(pid) || M_MenuButtonPressed(pid, MBT_X) || M_MenuButtonPressed(pid, MBT_Y))
	{
		exitmenu = true;
		M_SetMenuDelay(pid);
	}


	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

// Opening manual
#ifdef TODONEWMANUAL
void M_Manual(INT32 choice)
{
	(void)choice;

	MISC_ManualDef.prevMenu = (choice == INT32_MAX ? NULL : currentMenu);
	M_SetupNextMenu(&MISC_ManualDef, true);
}
#endif
