/// \file  menus/extras-replay-hut.c
/// \brief Extras Menu: Replay Hut

#include "../k_menu.h"

// extras menu: replay hut
menuitem_t EXTRAS_ReplayHut[] =
{
	{IT_KEYHANDLER|IT_NOTHING, "", "",			// Dummy menuitem for the replay list
		NULL, {.routine = M_HandleReplayHutList}, 0, 0},

	{IT_NOTHING, "", "",						// Dummy for handling wrapping to the top of the menu..
		NULL, {NULL}, 0, 0},
};

menu_t EXTRAS_ReplayHutDef =
{
	sizeof (EXTRAS_ReplayHut)/sizeof (menuitem_t),
	&EXTRAS_MainDef,
	0,
	EXTRAS_ReplayHut,
	30, 80,
	0, 0,
	0, 0,
	M_DrawReplayHut,
	NULL,
	NULL,
	M_QuitReplayHut,
	NULL
};

menuitem_t EXTRAS_ReplayStart[] =
{
	{IT_CALL |IT_STRING,  "Load Addons and Watch", NULL,
		NULL, {.routine = M_HutStartReplay}, 0, 0},

	{IT_CALL |IT_STRING,  "Load Without Addons", NULL,
		NULL, {.routine = M_HutStartReplay}, 10, 0},

	{IT_CALL |IT_STRING,  "Watch Replay", NULL,
		NULL, {.routine = M_HutStartReplay}, 10, 0},

	{IT_SUBMENU |IT_STRING,  "Go Back", NULL,
		NULL, {.submenu = &EXTRAS_ReplayHutDef}, 30, 0},
};


menu_t EXTRAS_ReplayStartDef =
{
	sizeof (EXTRAS_ReplayStart)/sizeof (menuitem_t),
	&EXTRAS_ReplayHutDef,
	0,
	EXTRAS_ReplayStart,
	27, 80,
	0, 0,
	0, 0,
	M_DrawReplayStartMenu,
	NULL,
	NULL,
	NULL,
	NULL
};
