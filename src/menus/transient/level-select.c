/// \file  menus/transient/level-select.c
/// \brief Level Select

#include "../../k_menu.h"
#include "../../m_cond.h" // Condition Sets
#include "../../z_zone.h"
#include "../../s_sound.h"
#include "../../r_local.h" // SplitScreen_OnChange
#include "../../f_finale.h" // F_WipeStartScreen
#include "../../v_video.h"

cupheader_t dummy_lostandfound;

menuitem_t PLAY_LevelSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_LevelSelectHandler}, 0, 0},
};

menu_t PLAY_LevelSelectDef = {
	sizeof(PLAY_LevelSelect) / sizeof(menuitem_t),
	&PLAY_CupSelectDef,
	0,
	PLAY_LevelSelect,
	0, 0,
	0, 0,
	NULL,
	2, 5,
	M_DrawLevelSelect,
	M_LevelSelectTick,
	NULL,
	NULL,
	NULL
};

struct levellist_s levellist;

//
// M_CanShowLevelInList
//
// Determines whether to show a given map in the various level-select lists.
//
boolean M_CanShowLevelInList(INT16 mapnum, levelsearch_t *levelsearch)
{
	if (!levelsearch)
		return false;

	if (mapnum >= nummapheaders)
		return false;

	// Does the map exist?
	if (!mapheaderinfo[mapnum])
		return false;

	// Does the map have a name?
	if (!mapheaderinfo[mapnum]->lvlttl[0])
		return false;

	// Does the map have a LUMP?
	if (mapheaderinfo[mapnum]->lumpnum == LUMPERROR)
		return false;

	// Check for TOL (permits TEST RUN outside of time attack)
	if ((levelsearch->timeattack || mapheaderinfo[mapnum]->typeoflevel)
		&& !(mapheaderinfo[mapnum]->typeoflevel & levelsearch->typeoflevel))
		return false;

	// Should the map be hidden?
	if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
		return false;

	// I don't know why, but some may have exceptions.
	if (levelsearch->timeattack && (mapheaderinfo[mapnum]->menuflags & LF2_NOTIMEATTACK))
		return false;

	// Don't permit cup when no cup requested (also no dupes in time attack)
	if (levelsearch->cupmode)
	{
		cupheader_t *cup = (levelsearch->cup == &dummy_lostandfound) ? NULL : levelsearch->cup;

		if ((!cup || levelsearch->timeattack)
			&& mapheaderinfo[mapnum]->cup != cup)
			return false;
	}

	// Finally, the most complex check: does the map have lock conditions?
	if (levelsearch->checklocked)
	{
		// Check for completion
		if ((mapheaderinfo[mapnum]->menuflags & LF2_FINISHNEEDED)
		&& !(mapheaderinfo[mapnum]->mapvisited & MV_BEATEN))
			return false;

		// Check for unlock
		if (M_MapLocked(mapnum+1))
			return false;
	}

	// Survived our checks.
	return true;
}

UINT16 M_CountLevelsToShowInList(levelsearch_t *levelsearch)
{
	INT16 i, count = 0;

	if (!levelsearch)
		return 0;

	if (levelsearch->cup && levelsearch->cup != &dummy_lostandfound)
	{
		if (levelsearch->checklocked && M_CupLocked(levelsearch->cup))
			return 0;

		for (i = 0; i < CUPCACHE_MAX; i++)
		{
			if (!M_CanShowLevelInList(levelsearch->cup->cachedlevels[i], levelsearch))
				continue;
			count++;
		}

		return count;
	}

	for (i = 0; i < nummapheaders; i++)
		if (M_CanShowLevelInList(i, levelsearch))
			count++;

	return count;
}

UINT16 M_GetFirstLevelInList(UINT8 *i, levelsearch_t *levelsearch)
{
	INT16 mapnum = NEXTMAP_INVALID;

	if (!levelsearch)
		return NEXTMAP_INVALID;

	if (levelsearch->cup && levelsearch->cup != &dummy_lostandfound)
	{
		if (levelsearch->checklocked && M_CupLocked(levelsearch->cup))
		{
			*i = CUPCACHE_MAX;
			return NEXTMAP_INVALID;
		}

		*i = 0;
		mapnum = NEXTMAP_INVALID;
		for (; *i < CUPCACHE_MAX; (*i)++)
		{
			if (!M_CanShowLevelInList(levelsearch->cup->cachedlevels[*i], levelsearch))
				continue;
			mapnum = levelsearch->cup->cachedlevels[*i];
			break;
		}
	}
	else
	{
		for (mapnum = 0; mapnum < nummapheaders; mapnum++)
			if (M_CanShowLevelInList(mapnum, levelsearch))
				break;

		if (mapnum >= nummapheaders)
			mapnum = NEXTMAP_INVALID;
	}

	return mapnum;
}

UINT16 M_GetNextLevelInList(UINT16 mapnum, UINT8 *i, levelsearch_t *levelsearch)
{
	if (!levelsearch)
		return NEXTMAP_INVALID;

	if (levelsearch->cup && levelsearch->cup != &dummy_lostandfound)
	{
		mapnum = NEXTMAP_INVALID;
		(*i)++;
		for (; *i < CUPCACHE_MAX; (*i)++)
		{
			if (!M_CanShowLevelInList(levelsearch->cup->cachedlevels[*i], levelsearch))
				continue;
			mapnum = levelsearch->cup->cachedlevels[*i];
			break;
		}
	}
	else
	{
		mapnum++;
		while (!M_CanShowLevelInList(mapnum, levelsearch) && mapnum < nummapheaders)
			mapnum++;
	}

	return mapnum;
}

void M_LevelSelectScrollDest(void)
{
	UINT16 m = M_CountLevelsToShowInList(&levellist.levelsearch)-1;

	levellist.dest = (6*levellist.cursor);

	if (levellist.dest < 3)
		levellist.dest = 3;

	if (levellist.dest > (6*m)-3)
		levellist.dest = (6*m)-3;
}

// Builds the level list we'll be using from the gametype we're choosing and send us to the apropriate menu.
// A gt of -1 means the menu is being restored.
boolean M_LevelListFromGametype(INT16 gt)
{
	static boolean first = true;
	UINT8 temp = 0;

	if (gt != -1 && (first || gt != levellist.newgametype || levellist.guessgt != MAXGAMETYPES))
	{
		if (first)
		{
			cupgrid.cappages = 0;
			cupgrid.builtgrid = NULL;
			dummy_lostandfound.cachedlevels[0] = NEXTMAP_INVALID;

			first = false;
		}

		levellist.newgametype = gt;

		levellist.levelsearch.typeoflevel = G_TOLFlag(gt);
		if (levellist.levelsearch.timeattack == true && gt == GT_SPECIAL)
		{
			// Sneak in an extra.
			levellist.levelsearch.typeoflevel |= G_TOLFlag(GT_VERSUS);
			levellist.guessgt = gt;
		}
		else
		{
			levellist.guessgt = MAXGAMETYPES;
		}

		levellist.levelsearch.cupmode = (!(gametypes[gt]->rules & GTR_NOCUPSELECT));

		CV_SetValue(&cv_dummyspbattack, 0);
	}

	// Obviously go to Cup Select in gametypes that have cups.
	// Use a really long level select in gametypes that don't use cups.

	if (levellist.levelsearch.cupmode)
	{
		levelsearch_t templevelsearch = levellist.levelsearch; // full copy
		size_t currentid = 0, highestunlockedid = 0;
		const size_t pagelen = sizeof(cupheader_t*) * (CUPMENU_COLUMNS * CUPMENU_ROWS);
		boolean foundany = false, currentvalid = false;

		templevelsearch.cup = kartcupheaders;

#if 0
		// Make sure there's valid cups before going to this menu. -- rip sweet prince
		if (templevelsearch.cup == NULL)
			I_Error("Can you really call this a racing game, I didn't recieve any Cups on my pillow or anything");
#endif

		if (cupgrid.cappages == 0)
		{
			cupgrid.cappages = 2;
			cupgrid.builtgrid = Z_Malloc(
				cupgrid.cappages * pagelen,
				PU_STATIC,
				NULL);

			if (!cupgrid.builtgrid)
			{
				I_Error("M_LevelListFromGametype: Not enough memory to allocate builtgrid");
			}
		}
		memset(cupgrid.builtgrid, 0, cupgrid.cappages * pagelen);

		// The following doubles the size of the buffer if necessary.
#define GRID_INSERTCUP \
			if ((currentid * sizeof(cupheader_t*)) >= cupgrid.cappages * pagelen) \
			{ \
				const size_t firstlen = cupgrid.cappages * pagelen; \
				cupgrid.builtgrid = Z_Realloc(cupgrid.builtgrid, \
					firstlen * 2, \
					PU_STATIC, NULL); \
				\
				if (!cupgrid.builtgrid) \
				{ \
					I_Error("M_LevelListFromGametype: Not enough memory to reallocate builtgrid"); \
				} \
				\
				cupgrid.cappages *= 2; \
			} \
			\
			cupgrid.builtgrid[currentid] = templevelsearch.cup;

#define GRID_FOCUSCUP \
					cupgrid.x = currentid % CUPMENU_COLUMNS; \
					cupgrid.y = (currentid / CUPMENU_COLUMNS) % CUPMENU_ROWS; \
					cupgrid.pageno = currentid / (CUPMENU_COLUMNS * CUPMENU_ROWS); \
					currentvalid = true;

		while (templevelsearch.cup)
		{
			templevelsearch.checklocked = false;
			if (!M_CountLevelsToShowInList(&templevelsearch))
			{
				// No valid maps, skip.
				templevelsearch.cup = templevelsearch.cup->next;
				continue;
			}

			foundany = true;

			GRID_INSERTCUP;

			templevelsearch.checklocked = true;
			if (M_GetFirstLevelInList(&temp, &templevelsearch) != NEXTMAP_INVALID)
			{
				highestunlockedid = currentid;

				if (Playing()
					? (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->cup == templevelsearch.cup)
					: (gt == -1 && levellist.levelsearch.cup == templevelsearch.cup))
				{
					GRID_FOCUSCUP;
				}
			}

			currentid++;
			templevelsearch.cup = templevelsearch.cup->next;
		}

		// Lost and found, a simplified version of the above loop.
		if (cupgrid.grandprix == false)
		{
			templevelsearch.cup = &dummy_lostandfound;
			templevelsearch.checklocked = true;

			if (M_GetFirstLevelInList(&temp, &levellist.levelsearch) != NEXTMAP_INVALID)
			{
				foundany = true;
				GRID_INSERTCUP;
				highestunlockedid = currentid;

				if (Playing()
					? (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->cup == NULL)
					: (gt == -1 && levellist.levelsearch.cup == templevelsearch.cup))
				{
					GRID_FOCUSCUP;
				}

				currentid++;
			}

			templevelsearch.cup = NULL;
		}

#undef GRID_INSERTCUP
#undef GRID_FOCUSCUP

		if (foundany == false)
		{
			return false;
		}

		if (currentvalid == false)
		{
			levellist.levelsearch.cup = NULL;
		}

		cupgrid.numpages = (highestunlockedid / (CUPMENU_COLUMNS * CUPMENU_ROWS)) + 1;
		if (cupgrid.pageno >= cupgrid.numpages)
		{
			cupgrid.pageno = 0;
		}

		if (gt != -1)
		{
			PLAY_CupSelectDef.prevMenu = currentMenu;
			PLAY_LevelSelectDef.prevMenu = &PLAY_CupSelectDef;
			M_SetupNextMenu(&PLAY_CupSelectDef, false);
		}

		return true;
	}

	// Okay, just a list of maps then.

	if (M_GetFirstLevelInList(&temp, &levellist.levelsearch) == NEXTMAP_INVALID)
	{
		return false;
	}

	// Reset position properly if you go back & forth between gametypes
	if (levellist.levelsearch.cup)
	{
		levellist.cursor = 0;
		levellist.levelsearch.cup = NULL;
	}

	M_LevelSelectScrollDest();
	levellist.y = levellist.dest;

	if (gt != -1)
	{
		PLAY_LevelSelectDef.prevMenu = currentMenu;
		M_SetupNextMenu(&PLAY_LevelSelectDef, false);
	}

	return true;
}

// Init level select for use in local play using the last choice we made.
// For the online MP version used to START HOSTING A GAME, see M_MPSetupNetgameMapSelect()
// (We still use this one midgame)

void M_LevelSelectInit(INT32 choice)
{
	INT32 gt = currentMenu->menuitems[itemOn].mvar2;

	(void)choice;

	// Make sure this is reset as we'll only be using this function for offline games!
	levellist.netgame = false;
	levellist.levelsearch.checklocked = true;

	switch (currentMenu->menuitems[itemOn].mvar1)
	{
		case 0:
			cupgrid.grandprix = false;
			levellist.levelsearch.timeattack = false;
			break;
		case 1:
			cupgrid.grandprix = false;
			levellist.levelsearch.timeattack = true;
			break;
		case 2:
			cupgrid.grandprix = true;
			levellist.levelsearch.timeattack = false;
			break;
		default:
			CONS_Alert(CONS_WARNING, "Bad level select init\n");
			return;
	}

	if (gt == -1)
	{
		gt = menugametype;
	}

	if (!M_LevelListFromGametype(gt))
	{
		S_StartSound(NULL, sfx_s3kb2);
		M_StartMessage(va("No levels available for\n%s Mode!\n\nPress (B)\n", gametypes[gt]->name), NULL, MM_NOTHING);
	}
}

void M_LevelSelected(INT16 add)
{
	UINT8 i = 0;
	INT16 map = M_GetFirstLevelInList(&i, &levellist.levelsearch);

	while (add > 0)
	{
		map = M_GetNextLevelInList(map, &i, &levellist.levelsearch);

		if (map >= nummapheaders)
		{
			break;
		}

		add--;
	}

	if (map >= nummapheaders)
	{
		// This shouldn't happen
		return;
	}

	levellist.choosemap = map;

	if (levellist.levelsearch.timeattack)
	{
		S_StartSound(NULL, sfx_s3k63);

		M_PrepareTimeAttack(0);

		PLAY_TimeAttackDef.lastOn = ta_start;
		PLAY_TimeAttackDef.prevMenu = currentMenu;
		M_SetupNextMenu(&PLAY_TimeAttackDef, false);
	}
	else
	{
		if (gamestate == GS_MENU)
		{
			UINT8 ssplayers = cv_splitplayers.value-1;

			netgame = false;
			multiplayer = true;

			strncpy(connectedservername, cv_servername.string, MAXSERVERNAME);

			// Still need to reset devmode
			cht_debug = 0;

			if (demo.playback)
				G_StopDemo();
			if (metalrecording)
				G_StopMetalDemo();

				/*if (levellist.choosemap == 0)
					levellist.choosemap = G_RandMap(G_TOLFlag(levellist.newgametype), -1, 0, 0, false, NULL);*/

			if (cv_maxconnections.value < ssplayers+1)
				CV_SetValue(&cv_maxconnections, ssplayers+1);

			if (splitscreen != ssplayers)
			{
				splitscreen = ssplayers;
				SplitScreen_OnChange();
			}

			S_StartSound(NULL, sfx_s3k63);

			paused = false;

			// Early fadeout to let the sound finish playing
			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();
			F_RunWipe(wipe_level_toblack, wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

			SV_StartSinglePlayerServer(levellist.newgametype, levellist.netgame);

			CV_StealthSet(&cv_kartbot, cv_dummymatchbots.string);
			CV_StealthSet(&cv_kartencore, (cv_dummygpencore.value == 1) ? "On" : "Auto");
			CV_StealthSet(&cv_kartspeed, (cv_dummykartspeed.value == KARTSPEED_NORMAL) ? "Auto" : cv_dummykartspeed.string);

			D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_kartencore.value == 1), 1, 1, false, false);

			if (levellist.netgame == true)
			{
				restoreMenu = &PLAY_MP_OptSelectDef;
			}
			else
			{
				restoreMenu = &PLAY_RaceDifficultyDef;
			}
		}
		else
		{
			// directly do the map change
			D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_kartencore.value == 1), 1, 1, false, false);
		}

		M_ClearMenus(true);
	}
}

void M_LevelSelectHandler(INT32 choice)
{
	INT16 maxlevels = M_CountLevelsToShowInList(&levellist.levelsearch);
	const UINT8 pid = 0;

	(void)choice;

	if (levellist.y != levellist.dest)
	{
		return;
	}

	if (menucmd[pid].dpad_ud > 0)
	{
		levellist.cursor++;
		if (levellist.cursor >= maxlevels)
			levellist.cursor = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		levellist.cursor--;
		if (levellist.cursor < 0)
			levellist.cursor = maxlevels-1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	M_LevelSelectScrollDest();

	if (M_MenuConfirmPressed(pid) /*|| M_MenuButtonPressed(pid, MBT_START)*/)
	{
		M_SetMenuDelay(pid);

		PLAY_TimeAttackDef.transitionID = currentMenu->transitionID;
		M_LevelSelected(levellist.cursor);
	}
	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

void M_LevelSelectTick(void)
{

	INT16 dist = levellist.dest - levellist.y;

	if (abs(dist) == 1)	// cheating to avoid off by 1 errors with divisions.
		levellist.y = levellist.dest;
	else
		levellist.y += dist/2;
}
