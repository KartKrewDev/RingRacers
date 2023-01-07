/// \file  menus/play-local-race-time-attack.c
/// \brief Race Time Attack Menu

#include "../k_menu.h"

// see ta_e
menuitem_t PLAY_TimeAttack[] =
{
	{IT_STRING | IT_SUBMENU, "Replay...", NULL, NULL, {.submenu = &PLAY_TAReplayDef}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Guest...", NULL, NULL, {.submenu = &PLAY_TAReplayGuestDef}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Ghosts...", NULL, NULL, {.submenu = &PLAY_TAGhostsDef}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Start", NULL, NULL, {.routine = M_StartTimeAttack}, 0, 0},
};

menu_t PLAY_TimeAttackDef = {
	sizeof(PLAY_TimeAttack) / sizeof(menuitem_t),
	&PLAY_LevelSelectDef,
	0,
	PLAY_TimeAttack,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};


menuitem_t PLAY_TAReplay[] =
{
	{IT_STRING | IT_CALL, "Replay Best Time", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Best Lap", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Last", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Guest", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Staff", NULL, NULL, {.routine = M_HandleStaffReplay}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Back", NULL, NULL, {.submenu = &PLAY_TimeAttackDef}, 0, 0},
};

menu_t PLAY_TAReplayDef = {
	sizeof(PLAY_TAReplay) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAReplay,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};

menuitem_t PLAY_TAReplayGuest[] =
{
	{IT_HEADERTEXT|IT_HEADER, "Save as guest...", NULL, NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "Best Time", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},
	{IT_STRING | IT_CALL, "Best Lap", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},
	{IT_STRING | IT_CALL, "Last Run", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Delete Guest", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Back", NULL, NULL, {.submenu = &PLAY_TimeAttackDef}, 0, 0},

};

menu_t PLAY_TAReplayGuestDef = {
	sizeof(PLAY_TAReplayGuest) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAReplayGuest,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};

menuitem_t PLAY_TAGhosts[] =
{
	{IT_STRING | IT_CVAR, "Best Time", NULL, NULL, {.cvar = &cv_ghost_besttime}, 0, 0},
	{IT_STRING | IT_CVAR, "Best Lap", NULL, NULL, {.cvar = &cv_ghost_bestlap}, 0, 0},
	{IT_STRING | IT_CVAR, "Last", NULL, NULL, {.cvar = &cv_ghost_last}, 0, 0},
	{IT_DISABLED, "Guest", NULL, NULL, {.cvar = &cv_ghost_guest}, 0, 0},
	{IT_DISABLED, "Staff", NULL, NULL, {.cvar = &cv_ghost_staff}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Back", NULL, NULL, {.submenu = &PLAY_TimeAttackDef}, 0, 0},
};

menu_t PLAY_TAGhostsDef = {
	sizeof(PLAY_TAGhosts) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAGhosts,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};
