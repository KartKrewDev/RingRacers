/// \file  menus/play-local-race-time-attack.c
/// \brief Race Time Attack Menu

#include "../k_menu.h"
#include "../r_local.h" // SplitScreen_OnChange
#include "../s_sound.h"
#include "../f_finale.h" // F_WipeStartScreen
#include "../v_video.h"
#include "../d_main.h" // srb2home
#include "../m_misc.h" // M_MkdirEach

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

// autorecord demos for time attack
consvar_t cv_autorecord = CVAR_INIT ("autorecord", "Yes", 0, CV_YesNo, NULL);

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show Character"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_besttime  = CVAR_INIT ("ghost_besttime",  "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_bestlap   = CVAR_INIT ("ghost_bestlap",   "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_last      = CVAR_INIT ("ghost_last",      "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_guest     = CVAR_INIT ("ghost_guest",     "Show", CV_SAVE, ghost2_cons_t, NULL);
consvar_t cv_ghost_staff     = CVAR_INIT ("ghost_staff",     "Show", CV_SAVE, ghost2_cons_t, NULL);

// time attack stuff...
void M_HandleStaffReplay(INT32 choice)
{
	// @TODO:
	(void) choice;
}

void M_ReplayTimeAttack(INT32 choice)
{
	// @TODO:
	(void) choice;
}

void M_SetGuestReplay(INT32 choice)
{
	// @TODO:
	(void) choice;
}

void M_StartTimeAttack(INT32 choice)
{
	char *gpath;
	char nameofdemo[256];

	(void)choice;

	modeattacking = ATTACKING_TIME;

	if ((gametypes[levellist.newgametype]->rules & GTR_CIRCUIT)
		&& (mapheaderinfo[levellist.choosemap]->numlaps != 1))
	{
		modeattacking |= ATTACKING_LAP;
	}

	// Still need to reset devmode
	cht_debug = 0;
	emeralds = 0;

	if (demo.playback)
		G_StopDemo();
	if (metalrecording)
		G_StopMetalDemo();

	splitscreen = 0;
	SplitScreen_OnChange();

	S_StartSound(NULL, sfx_s3k63);

	paused = false;

	// Early fadeout to let the sound finish playing
	F_WipeStartScreen();
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	F_WipeEndScreen();
	F_RunWipe(wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

	SV_StartSinglePlayerServer(levellist.newgametype, false);

	gpath = va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s",
			srb2home, timeattackfolder);
	M_MkdirEach(gpath, M_PathParts(gpath) - 3, 0755);

	strcat(gpath, PATHSEP);
	strcat(gpath, G_BuildMapName(levellist.choosemap+1));

	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-last", gpath, cv_skin[0].string);

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	M_ClearMenus(true);
	D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_dummygpencore.value == 1), 1, 1, false, false);
}
