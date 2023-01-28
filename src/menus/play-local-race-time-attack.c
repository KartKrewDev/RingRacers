/// \file  menus/play-local-race-time-attack.c
/// \brief Race Time Attack Menu

#include "../k_menu.h"
#include "../r_local.h" // SplitScreen_OnChange
#include "../s_sound.h"
#include "../f_finale.h" // F_WipeStartScreen
#include "../v_video.h"
#include "../d_main.h" // srb2home
#include "../m_misc.h" // M_MkdirEach
#include "../z_zone.h" // Z_StrDup/Z_Free

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


typedef enum
{
	tareplay_besttime = 0,
	tareplay_bestlap,
	tareplay_gap1,
	tareplay_last,
	tareplay_guest,
	tareplay_staff,
	tareplay_gap2,
	tareplay_back
} tareplay_e;

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

typedef enum
{
	taguest_save = 0,
	taguest_besttime,
	taguest_bestlap,
	taguest_last,
	taguest_gap1,
	taguest_delete,
	taguest_gap2,
	taguest_back
} taguest_e;

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

typedef enum
{
	taghost_besttime = 0,
	taghost_bestlap,
	taghost_last,
	taghost_guest,
	taghost_staff,
	taghost_gap1,
	taghost_back
} taghost_e;

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
void M_PrepareTimeAttack(INT32 choice)
{
	(void) choice;

	if (levellist.guessgt != MAXGAMETYPES)
	{
		levellist.newgametype = levellist.guessgt;
		if (!(gametypes[levellist.newgametype]->tol & mapheaderinfo[levellist.choosemap]->typeoflevel))
		{
			INT32 guess = G_GuessGametypeByTOL(mapheaderinfo[levellist.choosemap]->typeoflevel);
			if (guess != -1)
				levellist.newgametype = guess;
		}
	}

	{
		// see also p_setup.c's P_LoadRecordGhosts
		char *gpath = Z_StrDup(va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(levellist.choosemap+1)));
		UINT8 active;

		if (!gpath)
			return;

		CV_StealthSetValue(&cv_dummystaff, 0);

		active = false;
		PLAY_TimeAttack[ta_guest].status = IT_DISABLED;
		PLAY_TimeAttack[ta_replay].status = IT_DISABLED;
		PLAY_TimeAttack[ta_ghosts].status = IT_DISABLED;

		// Check if file exists, if not, disable options
		PLAY_TAReplay[tareplay_besttime].status =
			PLAY_TAReplayGuest[taguest_besttime].status = IT_DISABLED;
		if (FIL_FileExists(va("%s-%s-time-best.lmp", gpath, cv_skin[0].string)))
		{
			PLAY_TAReplay[tareplay_besttime].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_besttime].status = IT_STRING|IT_CALL;
			active |= (1|2|4);
		}

		PLAY_TAReplay[tareplay_bestlap].status =
			PLAY_TAReplayGuest[taguest_bestlap].status =
			PLAY_TAGhosts[taghost_bestlap].status = IT_DISABLED;
		if ((gametypes[levellist.newgametype]->rules & GTR_CIRCUIT)
			&& (mapheaderinfo[levellist.choosemap]->numlaps > 1)
			&& FIL_FileExists(va("%s-%s-lap-best.lmp", gpath, cv_skin[0].string)))
		{
			PLAY_TAReplay[tareplay_bestlap].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_bestlap].status = IT_STRING|IT_CALL;
			PLAY_TAGhosts[taghost_bestlap].status = IT_STRING|IT_CVAR;
			active |= (1|2|4);
		}

		PLAY_TAReplay[tareplay_last].status =
			PLAY_TAReplayGuest[taguest_last].status = IT_DISABLED;
		if (FIL_FileExists(va("%s-%s-last.lmp", gpath, cv_skin[0].string)))
		{
			PLAY_TAReplay[tareplay_last].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_last].status = IT_STRING|IT_CALL;
			active |= (1|2|4);
		}

		PLAY_TAReplay[tareplay_guest].status =
			PLAY_TAGhosts[taghost_guest].status =
			PLAY_TAReplayGuest[taguest_delete].status = IT_DISABLED;
		if (FIL_FileExists(va("%s-guest.lmp", gpath)))
		{
			PLAY_TAReplay[tareplay_guest].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_delete].status = IT_STRING|IT_CALL;
			PLAY_TAGhosts[taghost_guest].status = IT_STRING|IT_CVAR;
			active |= (1|2|4);
		}

		PLAY_TAReplay[tareplay_staff].status =
			PLAY_TAGhosts[taghost_staff].status = IT_DISABLED;
#ifdef STAFFGHOSTS
		CV_SetValue(&cv_dummystaff, 1);
		if (cv_dummystaff.value)
		{
			PLAY_TAReplay[tareplay_staff].status = IT_STRING|IT_KEYHANDLER;
			PLAY_TAGhosts[taghost_staff].status = IT_STRING|IT_CVAR;
			CV_StealthSetValue(&cv_dummystaff, 1);
			active |= 1|4;
		}
#endif //#ifdef STAFFGHOSTS

		if (active & 1)
			PLAY_TimeAttack[ta_replay].status = IT_STRING|IT_SUBMENU;
		else if (PLAY_TimeAttackDef.lastOn == ta_replay)
			PLAY_TimeAttackDef.lastOn = ta_start;
		if (active & 2)
			PLAY_TimeAttack[ta_guest].status = IT_STRING|IT_SUBMENU;
		else if (PLAY_TimeAttackDef.lastOn == ta_guest)
			PLAY_TimeAttackDef.lastOn = ta_start;
		//if (active & 4) -- for possible future use
			PLAY_TimeAttack[ta_ghosts].status = IT_STRING|IT_SUBMENU;
		/*else if (PLAY_TimeAttackDef.lastOn == ta_ghosts)
			PLAY_TimeAttackDef.lastOn = ta_start;*/

		Z_Free(gpath);
	}
}

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
