/// \file  menus/options-gameplay-item-toggles.c
/// \brief Random Item Toggles

#include "../k_menu.h"
#include "../s_sound.h"

menuitem_t OPTIONS_GameplayItems[] =
{
	// Mostly handled by the drawing function.
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Rings",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Self-Propelled Bombs",	NULL, {.routine = M_HandleItemToggles}, KITEM_SPB, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Eggman Marks",			NULL, {.routine = M_HandleItemToggles}, KITEM_EGGMAN, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Toggle All",			NULL, {.routine = M_HandleItemToggles}, 0, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers",				NULL, {.routine = M_HandleItemToggles}, KITEM_SNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLESNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Rocket Sneakers",		NULL, {.routine = M_HandleItemToggles}, KITEM_ROCKETSNEAKER, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas",				NULL, {.routine = M_HandleItemToggles}, KITEM_BANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Proximity Mines",		NULL, {.routine = M_HandleItemToggles}, KITEM_MINE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts",				NULL, {.routine = M_HandleItemToggles}, KITEM_ORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts x4",			NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Land Mines",			NULL, {.routine = M_HandleItemToggles}, KITEM_LANDMINE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz",					NULL, {.routine = M_HandleItemToggles}, KITEM_JAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz x2",				NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ballhogs",				NULL, {.routine = M_HandleItemToggles}, KITEM_BALLHOG, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Drop Targets",			NULL, {.routine = M_HandleItemToggles}, KITEM_DROPTARGET, sfx_s258},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Lightning Shields",		NULL, {.routine = M_HandleItemToggles}, KITEM_LIGHTNINGSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bubble Shields",		NULL, {.routine = M_HandleItemToggles}, KITEM_BUBBLESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Flame Shields",			NULL, {.routine = M_HandleItemToggles}, KITEM_FLAMESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Hyudoros",				NULL, {.routine = M_HandleItemToggles}, KITEM_HYUDORO, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Invinciblity",			NULL, {.routine = M_HandleItemToggles}, KITEM_INVINCIBILITY, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Grow",					NULL, {.routine = M_HandleItemToggles}, KITEM_GROW, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Shrink",				NULL, {.routine = M_HandleItemToggles}, KITEM_SHRINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pogo Springs",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_POGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Kitchen Sinks",			NULL, {.routine = M_HandleItemToggles}, KITEM_KITCHENSINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0}
};

menu_t OPTIONS_GameplayItemsDef = {
	sizeof (OPTIONS_GameplayItems) / sizeof (menuitem_t),
	&OPTIONS_GameplayDef,
	0,
	OPTIONS_GameplayItems,
	14, 40,
	SKINCOLOR_SCARLET, 0,
	NULL,
	2, 5,
	M_DrawItemToggles,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

void M_HandleItemToggles(INT32 choice)
{
	const INT32 width = 8, height = 4;
	INT32 column = itemOn/height, row = itemOn%height;
	INT16 next;
	UINT8 i;
	boolean exitmenu = false;
	const UINT8 pid = 0;

	(void) choice;

	if (menucmd[pid].dpad_lr > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		column++;
		if (((column*height)+row) >= currentMenu->numitems)
			column = 0;
		next = min(((column*height)+row), currentMenu->numitems-1);
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_lr < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		column--;
		if (column < 0)
			column = width-1;
		if (((column*height)+row) >= currentMenu->numitems)
			column--;
		next = max(((column*height)+row), 0);
		if (next >= currentMenu->numitems)
			next = currentMenu->numitems-1;
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		row = (row+1) % height;
		if (((column*height)+row) >= currentMenu->numitems)
			row = 0;
		next = min(((column*height)+row), currentMenu->numitems-1);
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		row = (row-1) % height;
		if (row < 0)
			row = height-1;
		if (((column*height)+row) >= currentMenu->numitems)
			row--;
		next = max(((column*height)+row), 0);
		if (next >= currentMenu->numitems)
			next = currentMenu->numitems-1;
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{
		M_SetMenuDelay(pid);
		if (currentMenu->menuitems[itemOn].mvar1 == 255)
		{
			//S_StartSound(NULL, sfx_s26d);
			if (!shitsfree)
			{
				shitsfree = TICRATE;
				S_StartSound(NULL, sfx_itfree);
			}
		}
		else
		if (currentMenu->menuitems[itemOn].mvar1 == 0)
		{
			INT32 v = cv_items[0].value;
			S_StartSound(NULL, sfx_s1b4);
			for (i = 0; i < NUMKARTRESULTS-1; i++)
			{
				if (cv_items[i].value == v)
					CV_AddValue(&cv_items[i], 1);
			}
		}
		else
		{
			if (currentMenu->menuitems[itemOn].mvar2)
			{
				S_StartSound(NULL, currentMenu->menuitems[itemOn].mvar2);
			}
			else
			{
				S_StartSound(NULL, sfx_s1ba);
			}
			CV_AddValue(&cv_items[currentMenu->menuitems[itemOn].mvar1-1], 1);
		}
	}

	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);
		exitmenu = true;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}
