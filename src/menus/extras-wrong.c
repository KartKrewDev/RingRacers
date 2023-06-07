/// \file  menus/extras-wrong.c
/// \brief Wrongwarp

#include "../k_menu.h"

struct wrongwarp_s wrongwarp;

static menuitem_t MISC_WrongWarpMenu[] =
{
	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

void M_WrongWarp(INT32 choice)
{
	(void)choice;

	wrongwarp.ticker = 0;

	MISC_WrongWarpDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_WrongWarpDef, false);
}

static void M_WrongWarpTick(void)
{
	wrongwarp.ticker++;
}

static boolean M_WrongWarpInputs(INT32 ch)
{
	(void)ch;

	if (wrongwarp.ticker < TICRATE/2)
		return true;

	return false;
}

menu_t MISC_WrongWarpDef = {
	sizeof (MISC_WrongWarpMenu)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_WrongWarpMenu,
	0, 0,
	0, 0,
	MBF_SOUNDLESS,
	"YEAWAY",
	98, 0,
	M_DrawWrongWarp,
	M_WrongWarpTick,
	NULL,
	NULL,
	M_WrongWarpInputs,
};
