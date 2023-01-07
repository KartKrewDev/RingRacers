/// \file  menus/extras-1.c
/// \brief Extras Menu

#include "../k_menu.h"

menuitem_t EXTRAS_Main[] =
{

	{IT_STRING | IT_CALL, "Addons", "Add files to customize your experience.",
		NULL, {.routine = M_Addons}, 0, 0},

	{IT_STRING | IT_CALL, "Challenges", "View the requirements for some of the secret content you can unlock!",
		NULL, {.routine = M_Challenges}, 0, 0},

	{IT_STRING | IT_CALL, "Replay Hut", "Play the replays you've saved throughout your many races & battles!",
		NULL, {.routine = M_ReplayHut}, 0, 0},

	{IT_STRING | IT_CALL, "Statistics", "Look back on some of your greatest achievements such as your playtime and wins!",
		NULL, {.routine = M_Statistics}, 0, 0},
};

// the extras menu essentially reuses the options menu stuff
menu_t EXTRAS_MainDef = {
	sizeof (EXTRAS_Main) / sizeof (menuitem_t),
	&MainDef,
	0,
	EXTRAS_Main,
	0, 0,
	0, 0,
	2, 5,
	M_DrawExtras,
	M_ExtrasTick,
	NULL,
	NULL,
	M_ExtrasInputs
};

