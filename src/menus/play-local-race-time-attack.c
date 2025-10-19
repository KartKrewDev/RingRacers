// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-local-race-time-attack.c
/// \brief Race Time Attack Menu

#include "../k_menu.h"
#include "../r_local.h" // SplitScreen_OnChange
#include "../s_sound.h"
#include "../f_finale.h" // F_WipeStartScreen
#include "../v_video.h"
#include "../d_main.h" // srb2home
#include "../m_misc.h" // M_MkdirEach
#include "../s_sound.h" // S_StartSound
#include "../z_zone.h" // Z_StrDup/Z_Free
#include "../m_cond.h"
#include "../console.h" // CON_ToggleOff

struct timeattackmenu_s timeattackmenu;

void M_TimeAttackTick(void)
{
	timeattackmenu.ticker++;
	if (timeattackmenu.spbflicker > 0)
	{
		timeattackmenu.spbflicker--;
	}
}

boolean M_EncoreAttackTogglePermitted(void)
{
	if ((gametypes[levellist.newgametype]->rules & GTR_ENCORE) == 0) //levellist.newgametype != GT_RACE
		return false;

	return M_SecretUnlocked(SECRET_SPBATTACK, true);
}

boolean M_TimeAttackInputs(INT32 ch)
{
	const UINT8 pid = 0;
	const boolean buttonR = M_MenuButtonPressed(pid, MBT_R);
	(void) ch;

	if (buttonR && M_EncoreAttackTogglePermitted())
	{
		CV_AddValue(&cv_dummyspbattack, 1);
		timeattackmenu.spbflicker = TICRATE/6;
		if (cv_dummyspbattack.value)
		{
			S_StartSound(NULL, sfx_s3k9f);
			S_StopSoundByID(NULL, sfx_s3k92);
		}
		else
		{
			S_StartSound(NULL, sfx_s3k92);
			S_StopSoundByID(NULL, sfx_s3k9f);
		}

		return true;
	}

	if (menucmd[pid].dpad_lr != 0
		&& !((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
			|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	)
	{
		if (menucmd[pid].dpad_lr > 0)
		{
			levellist.cursor++;
			if (levellist.cursor >= levellist.mapcount)
			{
				M_LevelSelectCupSwitch(true, false);
				levellist.cursor = 0;
			}
		}
		else
		{
			levellist.cursor--;
			if (levellist.cursor < 0)
			{
				M_LevelSelectCupSwitch(false, false);
				levellist.cursor = levellist.mapcount-1;
			}
		}

		M_LevelSelectScrollDest();
		levellist.slide.start = 0;

		M_LevelSelected(levellist.cursor, false);
		itemOn = ta_start;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);

		return true;
	}

	return false;
}

// see ta_e
menuitem_t PLAY_TimeAttack[] =
{
	{IT_STRING | IT_SUBMENU, "Replay...", NULL, "MENUI006", {.submenu = &PLAY_TAReplayDef}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Guest...", NULL, "MENUI006", {.submenu = &PLAY_TAReplayGuestDef}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Ghosts...", NULL, "MENUI006", {.submenu = &PLAY_TAGhostsDef}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Start", NULL, "MENUI006", {.routine = M_StartTimeAttack}, 0, 0},
};

menu_t PLAY_TimeAttackDef = {
	sizeof(PLAY_TimeAttack) / sizeof(menuitem_t),
	&PLAY_LevelSelectDef,
	0,
	PLAY_TimeAttack,
	0, 0,
	0, 0,
	0,
	NULL,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	M_TimeAttackTick,
	NULL,
	NULL,
	M_TimeAttackInputs
};


typedef enum
{
	tareplay_header = 0,
	tareplay_besttime,
	tareplay_bestlap,
	tareplay_last,
	tareplay_gap1,
	tareplay_guest,
	tareplay_staff,
	tareplay_gap2,
	tareplay_back
} tareplay_e;

menuitem_t PLAY_TAReplay[] =
{
	{IT_HEADERTEXT|IT_HEADER, "<!>", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Best Time", NULL, "MENUI006", {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Best Lap", NULL, "MENUI006", {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Last", NULL, "MENUI006", {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Guest", NULL, "MENUI006", {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_ARROWS, "Replay Staff", NULL, "MENUI006", {.routine = M_HandleStaffReplay}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Back", NULL, "MENUI006", {.submenu = &PLAY_TimeAttackDef}, 0, 0},
};

menu_t PLAY_TAReplayDef = {
	sizeof(PLAY_TAReplay) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAReplay,
	0, 0,
	0, 0,
	0,
	NULL,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	M_TimeAttackTick,
	NULL,
	NULL,
	NULL
};

typedef enum
{
	taguest_header = 0,
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
	{IT_HEADERTEXT|IT_HEADER, "Save as guest...", NULL, "MENUI006", {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "Best Time", NULL, "MENUI006", {.routine = M_SetGuestReplay}, 0, 0},
	{IT_STRING | IT_CALL, "Best Lap", NULL, "MENUI006", {.routine = M_SetGuestReplay}, 0, 0},
	{IT_STRING | IT_CALL, "Last Run", NULL, "MENUI006", {.routine = M_SetGuestReplay}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Delete Guest", NULL, "MENUI006", {.routine = M_SetGuestReplay}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Back", NULL, "MENUI006", {.submenu = &PLAY_TimeAttackDef}, 0, 0},

};

menu_t PLAY_TAReplayGuestDef = {
	sizeof(PLAY_TAReplayGuest) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAReplayGuest,
	0, 0,
	0, 0,
	0,
	NULL,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	M_TimeAttackTick,
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
	{IT_STRING | IT_CVAR, "Best Time", NULL, "MENUI006", {.cvar = &cv_ghost_besttime}, 0, 0},
	{IT_STRING | IT_CVAR, "Best Lap", NULL, "MENUI006", {.cvar = &cv_ghost_bestlap}, 0, 0},
	{IT_STRING | IT_CVAR, "Last", NULL, "MENUI006", {.cvar = &cv_ghost_last}, 0, 0},
	{IT_DISABLED, "Guest", NULL, "MENUI006", {.cvar = &cv_ghost_guest}, 0, 0},
	{IT_DISABLED, "Staff", NULL, "MENUI006", {.cvar = &cv_ghost_staff}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Back", NULL, "MENUI006", {.submenu = &PLAY_TimeAttackDef}, 0, 0},
};

menu_t PLAY_TAGhostsDef = {
	sizeof(PLAY_TAGhosts) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAGhosts,
	0, 0,
	0, 0,
	0,
	NULL,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	M_TimeAttackTick,
	NULL,
	NULL,
	NULL
};

// See also G_UpdateRecordReplays
const char *M_GetRecordMode(void)
{
	if (cv_dummyspbattack.value)
	{
		return "spb-";
	}

	const INT32 skinid = R_SkinAvailableEx(cv_skin[0].string, false);
	if (skinid >= 0 && (skins[skinid]->flags & SF_HIVOLT))
	{
		return "hivolt-";
	}

	return "";
}

void CV_SPBAttackChanged(void);
void CV_SPBAttackChanged(void)
{
	G_UpdateTimeStickerMedals(levellist.choosemap, false);

	// Menu options
	{
		// see also p_setup.c's P_LoadRecordGhosts
		char *gpath = Z_StrDup(va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(levellist.choosemap+1)));
		const char *modeprefix = M_GetRecordMode();
		UINT8 active;

		if (!gpath)
			return;


		active = false;
		PLAY_TimeAttack[ta_guest].status = IT_DISABLED;
		PLAY_TimeAttack[ta_replay].status = IT_DISABLED;
		PLAY_TimeAttack[ta_ghosts].status = IT_DISABLED;

		PLAY_TAReplay[tareplay_header].status = IT_DISABLED;

		// Check if file exists, if not, disable options
		PLAY_TAReplay[tareplay_besttime].status =
			PLAY_TAReplayGuest[taguest_besttime].status = IT_DISABLED;
		if (FIL_FileExists(va("%s-%s-%stime-best.lmp", gpath, cv_skin[0].string, modeprefix)))
		{
			PLAY_TAReplay[tareplay_besttime].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_besttime].status = IT_STRING|IT_CALL;
			active |= (1|2|4|8);
		}
		else if (PLAY_TAReplayGuestDef.lastOn == taguest_besttime)
			PLAY_TAReplayGuestDef.lastOn = taguest_back;

		PLAY_TAReplay[tareplay_bestlap].status =
			PLAY_TAReplayGuest[taguest_bestlap].status =
			PLAY_TAGhosts[taghost_bestlap].status = IT_DISABLED;
		if ((gametypes[levellist.newgametype]->rules & GTR_CIRCUIT)
			&& (mapheaderinfo[levellist.choosemap]->numlaps != 1)
			&& FIL_FileExists(va("%s-%s-%slap-best.lmp", gpath, cv_skin[0].string, modeprefix)))
		{
			PLAY_TAReplay[tareplay_bestlap].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_bestlap].status = IT_STRING|IT_CALL;
			PLAY_TAGhosts[taghost_bestlap].status = IT_STRING|IT_CVAR;
			active |= (1|2|4|8);
		}
		else if (PLAY_TAReplayGuestDef.lastOn == taguest_bestlap)
			PLAY_TAReplayGuestDef.lastOn = taguest_back;

		PLAY_TAReplay[tareplay_last].status =
			PLAY_TAReplayGuest[taguest_last].status = IT_DISABLED;
		if (FIL_FileExists(va("%s-%s-%slast.lmp", gpath, cv_skin[0].string, modeprefix)))
		{
			PLAY_TAReplay[tareplay_last].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_last].status = IT_STRING|IT_CALL;
			active |= (1|2|4|8);
		}
		else if (PLAY_TAReplayGuestDef.lastOn == taguest_last)
			PLAY_TAReplayGuestDef.lastOn = taguest_back;

		PLAY_TAReplay[tareplay_guest].status =
			PLAY_TAGhosts[taghost_guest].status =
			PLAY_TAReplayGuest[taguest_delete].status = IT_DISABLED;
		if (FIL_FileExists(va("%s-%sguest.lmp", gpath, modeprefix)))
		{
			PLAY_TAReplay[tareplay_guest].status = IT_STRING|IT_CALL;
			PLAY_TAReplayGuest[taguest_delete].status = IT_STRING|IT_CALL;
			PLAY_TAGhosts[taghost_guest].status = IT_STRING|IT_CVAR;
			active |= (1|2|4);
		}
		else if (PLAY_TAReplayGuestDef.lastOn == taguest_delete)
			PLAY_TAReplayGuestDef.lastOn = taguest_back;

		PLAY_TAReplay[tareplay_staff].status =
			PLAY_TAGhosts[taghost_staff].status = IT_DISABLED;
		if (mapheaderinfo[levellist.choosemap]->ghostCount > 0 && !modeprefix[0])
		{
			extern CV_PossibleValue_t dummystaff_cons_t[];
			dummystaff_cons_t[1].value = mapheaderinfo[levellist.choosemap]->ghostCount-1;

			// LAST MINUTE SANITY
			static UINT16 laststaffmap = NEXTMAP_INVALID;
			if (laststaffmap != levellist.choosemap || cv_dummystaff.value < dummystaff_cons_t[0].value)
			{
				laststaffmap = levellist.choosemap;
				CV_SetValue(&cv_dummystaff, dummystaff_cons_t[0].value);
			}
			else if (cv_dummystaff.value > dummystaff_cons_t[1].value)
			{
				CV_SetValue(&cv_dummystaff, dummystaff_cons_t[1].value);
			}

			PLAY_TAReplay[tareplay_staff].status = IT_STRING|IT_ARROWS;
			PLAY_TAGhosts[taghost_staff].status = IT_STRING|IT_CVAR;
			active |= 1|4;
		}

		if (currentMenu == &PLAY_TimeAttackDef)
			PLAY_TimeAttackDef.lastOn = itemOn;

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

		if ((active & 8) && M_EncoreAttackTogglePermitted())
		{
			PLAY_TAReplay[tareplay_header].status = IT_HEADER;
			PLAY_TAReplay[tareplay_header].text = cv_dummyspbattack.value ? "SPB Attack..." : "Time Attack...";
		}

		if (currentMenu == &PLAY_TimeAttackDef)
			itemOn = PLAY_TimeAttackDef.lastOn;

		Z_Free(gpath);
	}
}

/// time attack stuff...
void M_PrepareTimeAttack(boolean menuupdate)
{
	if (menuupdate)
	{
		timeattackmenu.ticker = 0;
	}

	// Gametype guess
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

	if (cv_dummyspbattack.value
	&& (levellist.levelsearch.timeattack == false || !M_EncoreAttackTogglePermitted()))
	{
		CV_StealthSetValue(&cv_dummyspbattack, 0);

		if (!menuupdate)
		{
			timeattackmenu.spbflicker = TICRATE/6;
			S_StartSound(NULL, sfx_s3k92);
			S_StopSoundByID(NULL, sfx_s3k9f);
		}
	}

	// Menu options / Time-sticker medals
	CV_SPBAttackChanged();
}

void M_HandleStaffReplay(INT32 choice)
{
	if (choice == 2)
	{
		mapheader_t *mapheader;
		staffbrief_t *staffbrief;
		restoreMenu = &PLAY_TAReplayDef;

		M_ClearMenusNoTitle(true);
		demo.loadfiles = false;
		demo.ignorefiles = true; // Just assume that record attack replays have the files needed

		mapheader = mapheaderinfo[levellist.choosemap];
		staffbrief = mapheader->ghostBrief[cv_dummystaff.value];

		G_DoPlayDemoEx("", (staffbrief->wad << 16) | staffbrief->lump);
		return;
	}

	M_ChangeCvarDirect(choice, &cv_dummystaff);
}

void M_ReplayTimeAttack(INT32 choice)
{
	menudemo_t menudemo = {0};
	const char *which = NULL;
	const char *modeprefix = M_GetRecordMode();

	switch (choice)
	{
		default:
		case tareplay_besttime:
			which = "time-best";
			break;
		case tareplay_bestlap:
			which = "lap-best";
			break;
		case tareplay_last:
			which = "last";
			break;
		case tareplay_guest:
			sprintf(menudemo.filepath, "%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%sguest.lmp", srb2home, timeattackfolder, G_BuildMapName(levellist.choosemap+1), modeprefix);
			break;
	}

	if (which)
	{
		sprintf(menudemo.filepath, "%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s%s.lmp", srb2home, timeattackfolder, G_BuildMapName(levellist.choosemap+1), cv_skin[0].string, modeprefix, which);
	}

	G_LoadDemoInfo(&menudemo, /*allownonmultiplayer*/ true);

	if (menudemo.type != MD_LOADED || menudemo.addonstatus > 0)
	{
		// Do nothing because the demo can't be played
		S_StartSound(NULL, sfx_tmxerr);
		M_StartMessage("Invalid Replay", "Replay cannot be played on this version of the game", NULL, MM_NOTHING, NULL, "Back");
		return;
	}

	restoreMenu = &PLAY_TAReplayDef;

	M_ClearMenusNoTitle(true);
	demo.loadfiles = false;
	demo.ignorefiles = true; // Just assume that record attack replays have the files needed

	G_DoPlayDemo(menudemo.filepath);
}

static const char *TA_GuestReplay_Str = NULL;

static void M_WriteGuestReplay(INT32 ch)
{
	char *gpath, *rguest;

	UINT8 *buf;
	size_t len = 0;

	if (ch != MA_YES)
		return;

	gpath = Z_StrDup(va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(levellist.choosemap+1)));

	const char *modeprefix = M_GetRecordMode();

	if (TA_GuestReplay_Str != NULL)
	{
		len = FIL_ReadFile(va("%s-%s-%s%s.lmp", gpath, cv_skin[0].string, modeprefix, TA_GuestReplay_Str), &buf);
		if (!len)
		{
			M_StartMessage("Guest Replay", "Replay to copy no longer exists!", NULL, MM_NOTHING, NULL, NULL);
			Z_Free(gpath);
			return;
		}
	}

	rguest = Z_StrDup(va("%s-%sguest.lmp", gpath, modeprefix));

	if (FIL_FileExists(rguest))
	{
		remove(rguest);
	}

	if (len)
	{
		FIL_WriteFile(rguest, buf, len);
	}

	Z_Free(rguest);
	Z_Free(gpath);

	M_PrepareTimeAttack(false);
	M_SetupNextMenu(&PLAY_TimeAttackDef, false);

	// TODO the following isn't showing up and I'm not sure why
	//M_StartMessage("Guest Replay", va("Guest replay data %s.", (len ? "saved" : "erased")), NULL, MM_NOTHING, NULL, NULL);
}


void M_SetGuestReplay(INT32 choice)
{
	switch (choice)
	{
		case taguest_besttime:
			TA_GuestReplay_Str = "time-best";
			break;
		case taguest_bestlap:
			TA_GuestReplay_Str = "lap-best";
			break;
		case taguest_last:
			TA_GuestReplay_Str = "last";
			break;

		case taguest_delete:
		default:
			TA_GuestReplay_Str = NULL;
			break;
	}

	const char *modeprefix = M_GetRecordMode();

	if (FIL_FileExists(va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%sguest.lmp", srb2home, timeattackfolder, G_BuildMapName(levellist.choosemap+1), modeprefix)))
	{
		M_StartMessage("Guest Replay", va("Are you sure you want to\n%s the guest replay data?\n", (TA_GuestReplay_Str != NULL ? "overwrite" : "delete")), &M_WriteGuestReplay, MM_YESNO, NULL, NULL);
	}
	else if (TA_GuestReplay_Str != NULL)
	{
		M_WriteGuestReplay(MA_YES);
	}
}

void M_StartTimeAttack(INT32 choice)
{
	char *gpath;
	char nameofdemo[256];
	const char *modeprefix = M_GetRecordMode();

	(void)choice;

	modeattacking = ATTACKING_TIME;

	if ((gametypes[levellist.newgametype]->rules & GTR_CIRCUIT)
		&& (mapheaderinfo[levellist.choosemap]->numlaps != 1))
	{
		modeattacking |= ATTACKING_LAP;
	}

	if (cv_dummyspbattack.value)
	{
		if (levellist.newgametype == GT_RACE)
		{
			modeattacking |= ATTACKING_SPB;
		}

		if (gamestate == GS_MENU)
		{
			encoremode = true; // guarantees short wipe
		}
	}

	// DON'T SOFTLOCK
	CON_ToggleOff();

	M_MenuToLevelPreamble(0, (gamestate != GS_MENU || cv_dummyspbattack.value == 1));

	gpath = va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s",
			srb2home, timeattackfolder);
	M_MkdirEach(gpath, M_PathParts(gpath) - 3, 0755);

	strcat(gpath, PATHSEP);
	strcat(gpath, G_BuildMapName(levellist.choosemap+1));

	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-%slast", gpath, cv_skin[0].string, modeprefix);

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	restoreMenu = &PLAY_TimeAttackDef;

	D_MapChange(
		levellist.choosemap+1,
		levellist.newgametype,
		(cv_dummyspbattack.value == 1),
		true,
		1,
		false,
		false
	);

	M_ClearMenusNoTitle(true);

	G_UpdateTimeStickerMedals(levellist.choosemap, true);
}
