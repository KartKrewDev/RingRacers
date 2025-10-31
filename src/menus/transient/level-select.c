// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/level-select.c
/// \brief Level Select

#include "../../i_time.h"
#include "../../k_menu.h"
#include "../../m_cond.h" // Condition Sets
#include "../../z_zone.h"
#include "../../s_sound.h"
#include "../../r_local.h" // SplitScreen_OnChange
#include "../../f_finale.h" // F_WipeStartScreen
#include "../../v_video.h"
#include "../../g_game.h" // G_GetBackupCupData
#include "../../p_saveg.h" // cupsavedata
#include "../../d_netcmd.h" // Handle_MapQueueSend

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
	0,
	NULL,
	2, 5,
	M_DrawLevelSelect,
	NULL,
	M_LevelSelectTick,
	NULL,
	NULL,
	NULL
};

levellist_t levellist;
levellist_t restorelevellist;

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
	// (grand prix can contain anything, we limit in a different way)
	if (levelsearch->grandprix == false
		&& (levelsearch->timeattack || levelsearch->tutorial || mapheaderinfo[mapnum]->typeoflevel)
		&& !(mapheaderinfo[mapnum]->typeoflevel & levelsearch->typeoflevel))
		return false;

	// Should the map be hidden?
	if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
		return false;

	// I don't know why, but some may have exceptions.
	if (levelsearch->timeattack && (mapheaderinfo[mapnum]->menuflags & LF2_NOTIMEATTACK))
		return false;

	cupheader_t *cup = (levelsearch->cup == &dummy_lostandfound) ? NULL : levelsearch->cup;

	// Don't permit cup when no cup requested (also no dupes in time attack)
	if (levelsearch->cupmode)
	{
		if ((!cup || levelsearch->timeattack)
			&& mapheaderinfo[mapnum]->cup != cup)
			return false;
	}

	// Finally, the most complex check: does the map have lock conditions?
	if (levelsearch->checklocked)
	{
		// All tutorial courses can be visited for the first time once the game has truly started.
		if (levelsearch->tutorial == false || M_GameTrulyStarted() == false)
		{
			// Check for visitation
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
			&& !(mapheaderinfo[mapnum]->records.mapvisited & MV_VISITED)
			&& !(cup && cup->cachedlevels[0] == mapnum))
				return false;
		}

		// Check for completion
		if ((mapheaderinfo[mapnum]->menuflags & LF2_FINISHNEEDED)
		&& !(mapheaderinfo[mapnum]->records.mapvisited & MV_BEATEN))
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

		const INT16 limit = (levelsearch->grandprix ? CUPCACHE_SPECIAL : CUPCACHE_PODIUM);
		for (i = 0; i < limit; i++)
		{
			if (!M_CanShowLevelInList(levelsearch->cup->cachedlevels[i], levelsearch))
				continue;
			count++;
		}

		return count;
	}

	for (i = 0; i < nummapheaders; i++)
	{
		if (!M_CanShowLevelInList(i, levelsearch))
			continue;
		count++;
	}

	return count;
}

UINT16 M_GetFirstLevelInList(UINT8 *i, levelsearch_t *levelsearch)
{
	UINT16 mapnum = NEXTMAP_INVALID;

	if (!levelsearch)
		return NEXTMAP_INVALID;

	if (levelsearch->cup && levelsearch->cup != &dummy_lostandfound)
	{
		if (levelsearch->checklocked && M_CupLocked(levelsearch->cup))
		{
			*i = CUPCACHE_PODIUM;
			return NEXTMAP_INVALID;
		}

		*i = 0;
		mapnum = NEXTMAP_INVALID;
		const INT16 limit = (levelsearch->grandprix ? CUPCACHE_SPECIAL : CUPCACHE_PODIUM);
		for (; *i < limit; (*i)++)
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
		const INT16 limit = (levelsearch->grandprix ? CUPCACHE_SPECIAL : CUPCACHE_PODIUM);
		for (; *i < limit; (*i)++)
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
	UINT16 m = levellist.mapcount-1;
	UINT16 dest = (6*levellist.cursor);

	if (dest < 3)
		dest = 3;

	if (m && dest > (6*m)-3)
		dest = (6*m)-3;

	dest *= 12;

	if (levellist.y != dest)
	{
		levellist.slide.start = I_GetTime();
		levellist.slide.dist = dest - levellist.y;
	}

	levellist.y = dest;
}

// Builds the level list we'll be using from the gametype we're choosing and send us to the apropriate menu.
// A gt of -1 means the menu is being restored.
boolean M_LevelListFromGametype(INT16 gt)
{
	static boolean first = true;
	UINT8 temp = 0;
	boolean invalidatedcursor = false;

	if (gt != -1)
	{
		if (first || gt != levellist.newgametype || levellist.guessgt != MAXGAMETYPES)
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
			if (!levellist.levelsearch.cupmode)
			{
				invalidatedcursor = (
					levellist.levelsearch.cup != NULL
					|| levellist.levelsearch.tutorial != (gt == GT_TUTORIAL)
				);
			}

			levellist.levelsearch.tutorial = (gt == GT_TUTORIAL);

			CV_SetValue(&cv_dummyspbattack, 0);
		}

		levellist.backMenu = currentMenu;

		if (gamestate == GS_MENU)
		{
			const char *music;
			void (*bgroutine)(void);

			if (gt == GT_SPECIAL)
			{
				music = "SSTAR3";
				bgroutine = M_DrawSealedBack;
			}
			else
			{
				music = levellist.backMenu->music;
				bgroutine = levellist.backMenu->bgroutine;
			}

			menu_t *remap_menus[] = {
				&PLAY_CupSelectDef,
				&PLAY_LevelSelectDef,
				&PLAY_TimeAttackDef,
				&PLAY_TAReplayDef,
				&PLAY_TAReplayGuestDef,
				&PLAY_TAGhostsDef,
				NULL
			};

			INT16 i, j;
			for (i = 0; remap_menus[i]; i++)
			{
				remap_menus[i]->music = music;
				remap_menus[i]->bgroutine = bgroutine;

				for (j = 0; j < remap_menus[i]->numitems; j++)
				{
					remap_menus[i]->menuitems[j].patch = \
						currentMenu->menuitems[itemOn].patch;
				}
			}
		}
	}

	// Obviously go to Cup Select in gametypes that have cups.
	// Use a really long level select in gametypes that don't use cups.

	if (levellist.levelsearch.cupmode)
	{
		PLAY_CupSelectDef.transitionID = PLAY_LevelSelectDef.transitionID;

		const boolean secondrowlocked = M_CupSecondRowLocked();
		if (cupgrid.cache_secondrowlocked != secondrowlocked)
		{
			cupgrid.cache_secondrowlocked = secondrowlocked;
			if (cupgrid.y || cupgrid.pageno)
			{
				// Prevent softlock, reset to start
				cupgrid.x = cupgrid.y = cupgrid.pageno = 0;
			}
		}

		levelsearch_t templevelsearch = levellist.levelsearch; // full copy
		size_t currentid = 0, highestunlockedid = 0;
		const size_t pagelen = sizeof(cupheader_t*) * (CUPMENU_COLUMNS * CUPMENU_ROWS);
		boolean foundany = false, currentvalid = false;
		size_t deltaid = 0;

		G_GetBackupCupData(
			templevelsearch.grandprix == true
			&& cv_splitplayers.value <= 1
		);

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

#define GRID_TIDYLOCKED(rewind) \
			currentid -= rewind; \
			memset(&cupgrid.builtgrid[currentid], 0, pagelen); \
			deltaid = 0;

		boolean lostandfoundready = true;
		// foundanythispage SHOULD start out as false... but if
		// nothing is unlocked, the first page should never be wiped!
		boolean foundanythispage = true;

		templevelsearch.cup = kartcupheaders;
		while (true)
		{
			// Handle reaching the end of the base-game cups.
			if (lostandfoundready == true
			&& (
				templevelsearch.cup == NULL
				|| templevelsearch.cup->id == basenumkartcupheaders
				)
			)
			{
				lostandfoundready = false;

				if (deltaid != 0 && foundanythispage == false)
				{
					GRID_TIDYLOCKED(deltaid);
				}

				size_t olddelta = deltaid;
				if (templevelsearch.grandprix == false)
				{
					cupheader_t *restore = templevelsearch.cup;

					templevelsearch.cup = &dummy_lostandfound;
					templevelsearch.checklocked = true;

					if (M_GetFirstLevelInList(&temp, &templevelsearch) != NEXTMAP_INVALID)
					{
						foundany = foundanythispage = true;
						GRID_INSERTCUP;
						highestunlockedid = currentid;

						if (Playing()
							? (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->cup == NULL)
							: (gt == -1 && levellist.levelsearch.cup == templevelsearch.cup))
						{
							GRID_FOCUSCUP;
						}

						currentid++;
						deltaid = currentid % (CUPMENU_COLUMNS * CUPMENU_ROWS);
					}

					templevelsearch.cup = restore;
				}

				// Lost and Found marks the transition point between base
				// and custom cups. Always force a page break between these
				// (unless LnF is the only "cup" on the page, for sanity).

				if (
					(deltaid == 0) // a new page already
					|| (olddelta == 0 && deltaid == 1) // LnF is first and only entry
				)
					; // this page layout is fine
				else
				{
					if (foundanythispage == false)
					{
						GRID_TIDYLOCKED(deltaid);
					}
					else
					{
						currentid += (CUPMENU_COLUMNS * CUPMENU_ROWS) - deltaid;
						deltaid = 0;
						foundanythispage = false;
					}
				}
			}

			if (templevelsearch.cup == NULL)
				break;

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
				foundanythispage = true;
				highestunlockedid = currentid;

				if (Playing()
					? (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->cup == templevelsearch.cup)
					: (cupsavedata.cup == templevelsearch.cup
						|| (gt == -1 && levellist.levelsearch.cup == templevelsearch.cup)))
				{
					GRID_FOCUSCUP;
				}
			}

			templevelsearch.cup = templevelsearch.cup->next;

			currentid++;
			deltaid = currentid % (CUPMENU_COLUMNS * CUPMENU_ROWS);

			if (secondrowlocked == true)
			{
				// If the second row is locked and you've reached it, skip onward.
				if (deltaid >= CUPMENU_COLUMNS)
				{
					currentid += (CUPMENU_COLUMNS * CUPMENU_ROWS) - deltaid;
					deltaid = 0;
				}
			}

			if (deltaid == 0)
			{
				if (foundanythispage == false)
				{
					GRID_TIDYLOCKED((CUPMENU_COLUMNS * CUPMENU_ROWS));
				}
				foundanythispage = false;
			}
		}

#undef GRID_INSERTCUP
#undef GRID_FOCUSCUP
#undef GRID_TIDYLOCKED

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

		PLAY_CupSelectDef.prevMenu = levellist.backMenu;
		PLAY_LevelSelectDef.prevMenu = &PLAY_CupSelectDef;

		if (gt != -1)
		{
			M_SetupNextMenu(&PLAY_CupSelectDef, false);
		}

		return true;
	}

	// Okay, just a list of maps then.

	levellist.levelsearch.cup = NULL;

	UINT16 test = M_GetFirstLevelInList(&temp, &levellist.levelsearch);

	if (test == NEXTMAP_INVALID)
	{
		return false;
	}

	// Reset position properly if you go back & forth between gametypes
	levellist.mapcount = M_CountLevelsToShowInList(&levellist.levelsearch);

	if (levellist.levelsearch.tutorial && levellist.levelsearch.checklocked)
	{
		// Find the first level we haven't played.
		UINT16 possiblecursor = 0;
		while (test < nummapheaders && (mapheaderinfo[test]->records.mapvisited & MV_BEATEN))
		{
			test = M_GetNextLevelInList(test, &temp, &levellist.levelsearch);
			possiblecursor++;
		}

		if (test < nummapheaders)
		{
			levellist.cursor = possiblecursor;
			invalidatedcursor = false;
		}
	}

	if (invalidatedcursor)
	{
		levellist.cursor = 0;
	}

	M_LevelSelectScrollDest();
	levellist.slide.start = 0;

	PLAY_LevelSelectDef.prevMenu = levellist.backMenu;

	if (gt != -1)
	{
		if (levellist.levelsearch.tutorial && levellist.mapcount == 1)
		{
			M_LevelSelected(0, true); // Skip the list!
		}
		else
		{
			M_SetupNextMenu(&PLAY_LevelSelectDef, false);
		}
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
			levellist.levelsearch.grandprix = false;
			levellist.levelsearch.timeattack = false;
			levellist.canqueue = true;

			if (gamestate == GS_MENU)
			{
				CV_StealthSet(&cv_kartbot, cv_dummymatchbots.string);
				CV_StealthSet(&cv_kartencore, (cv_dummygpencore.value == 1) ? "On" : "Auto");
				CV_StealthSet(&cv_kartspeed, cv_dummykartspeed.string);
			}

			break;
		case 1:
			levellist.levelsearch.grandprix = false;
			levellist.levelsearch.timeattack = true;
			levellist.canqueue = false;
			break;
		case 2:
			levellist.levelsearch.grandprix = true;
			levellist.levelsearch.timeattack = false;
			levellist.canqueue = false;
			break;
		case 3:
			levellist.levelsearch.grandprix = false;
			levellist.levelsearch.timeattack = false;
			levellist.canqueue = false;
			break;
		default:
			CONS_Alert(CONS_WARNING, "Bad level select init\n");
			return;
	}

	if (gt == -1)
	{
		gt = menugametype;
	}

	if (levellist.levelsearch.timeattack && gt == GT_RACE)
	{
		const INT32 skinid = R_SkinAvailableEx(cv_skin[0].string, false);
		if (skinid >= 0 && (skins[skinid]->flags & SF_HIVOLT))
		{
			M_StartMessage("A long-forgotten power...", "You are using a \x82prototype engine\x80.\nRecords will not be saved.", NULL, MM_NOTHING, NULL, NULL);
			S_StartSound(NULL, sfx_s3k81);
		}
	}

	if (!M_LevelListFromGametype(gt))
	{
		S_StartSound(NULL, sfx_s3kb2);
		M_StartMessage("Offline Play", va("No levels available for\n%s Mode!\n", gametypes[gt]->name), NULL, MM_NOTHING, NULL, NULL);
	}
}

void M_MenuToLevelPreamble(UINT8 ssplayers, boolean nowipe)
{
	cht_debug = 0;

	if (demo.playback)
		G_StopDemo();

	if (cv_maxconnections.value < ssplayers+1)
		CV_SetValue(&cv_maxconnections, ssplayers+1);

	if (splitscreen != ssplayers)
	{
		splitscreen = ssplayers;
		SplitScreen_OnChange();
	}

	paused = false;

	S_StopMusicCredit();

	if (!nowipe)
	{
		S_StartSound(NULL, sfx_s3k63);

		// Early fadeout to let the sound finish playing
		F_WipeStartScreen();
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
		F_WipeEndScreen();
		F_RunWipe(wipe_level_toblack, wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);
	}

	SV_StartSinglePlayerServer(levellist.newgametype, levellist.netgame);
}

INT16 M_LevelFromScrolledList(INT16 add)
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

	return map;
}

void M_LevelSelected(INT16 add, boolean menuupdate)
{
	INT16 map = M_LevelFromScrolledList(add);

	if (map >= nummapheaders)
	{
		// This shouldn't happen
		return;
	}

	levellist.choosemap = map;

	if (levellist.levelsearch.timeattack)
	{
		restorelevellist = levellist;

		M_PrepareTimeAttack(menuupdate);

		if (menuupdate)
		{
			S_StartSound(NULL, sfx_s3k63);

			PLAY_TimeAttackDef.lastOn = ta_start;
			PLAY_TimeAttackDef.prevMenu = currentMenu;
			M_SetupNextMenu(&PLAY_TimeAttackDef, false);
		}
	}
	else
	{
		if (gamestate == GS_MENU)
		{
			multiplayer = true;

			M_MenuToLevelPreamble(
				(levellist.levelsearch.tutorial
					? 0
					: cv_splitplayers.value-1
				),
				(
					(cv_kartencore.value == 1)
					&& (gametypes[levellist.newgametype]->rules & GTR_ENCORE)
				)
			);

			restoreMenu = (levellist.netgame)
				? &PLAY_MP_OptSelectDef
				: currentMenu;

			restorelevellist = levellist;
		}

		D_MapChange(
			levellist.choosemap+1,
			levellist.newgametype,
			(cv_kartencore.value == 1),
			true,
			1,
			false,
			false
		);

		M_ClearMenus(true);
	}
}

static void M_MenuQueueStopSend(INT32 ch)
{
	(void)ch;

	memset(&menuqueue, 0, sizeof(struct menuqueue));

	menuqueue.clearing = false;
}

static void M_MenuQueueSelectedLocal(void)
{
	UINT8 i = 0;

	for (; i < menuqueue.size; i++)
	{
		G_MapIntoRoundQueue(
			menuqueue.entries[i].mapnum,
			menuqueue.entries[i].gametype,
			menuqueue.entries[i].encore,
			false
		);
	}

	roundqueue.netcommunicate = true;

	if (gamestate == GS_MENU)
	{
		levellist.choosemap = roundqueue.entries[0].mapnum;

		multiplayer = true;

		M_MenuToLevelPreamble(
			cv_splitplayers.value-1,
			(
				roundqueue.entries[0].encore
				&& (gametypes[roundqueue.entries[0].gametype]->rules & GTR_ENCORE)
			)
		);

		restoreMenu = (levellist.netgame)
			? &PLAY_MP_OptSelectDef
			: currentMenu;

		restorelevellist = levellist;

		roundqueue.position = roundqueue.roundnum = 1;

		D_MapChange(
			roundqueue.entries[0].mapnum + 1,
			roundqueue.entries[0].gametype,
			roundqueue.entries[0].encore,
			true,
			1,
			false,
			roundqueue.entries[0].rankrestricted
		);

		M_MenuQueueStopSend(MA_NONE);

		M_ClearMenus(true);
	}
	else
	{
		if (gamestate == GS_LEVEL && roundqueue.position == 0)
		{
			menuqueue.sending = UINT8_MAX;
		}
		else
		{
			S_StartSound(NULL, sfx_chchng);

			M_MenuQueueStopSend(MA_NONE);

			M_ClearMenus(true);
		}
	}
}

// Copy-pasted and edited from G_GPCupIntoRoundQueue
void M_CupQueueHandler(cupheader_t *cup)
{
	UINT8 i, levelindex = 0, bonusindex = 0;
	UINT8 bonusmodulo = max(1, (cup->numlevels+1)/(cup->numbonus+1));
	UINT16 cupLevelNum;
	INT32 gtcheck;

	// We shouldn't get to this point while there's rounds queued, but if we do, get outta there.
	if (roundqueue.size)
	{
		return;
	}

	menuqueue.size = 0;

	// Levels are added to the queue in the following pattern.
	// For 5 Race rounds and 2 Bonus rounds, the most common case:
	//    race - race - BONUS - race - race - BONUS - race
	// The system is flexible enough to permit other arrangements.
	// However, we just want to keep the pacing even & consistent.
	while (levelindex < cup->numlevels)
	{
		memset(menuqueue.entries+menuqueue.size, 0, sizeof(roundentry_t));

		// Fill like two or three Race maps.
		for (i = 0; i < bonusmodulo; i++)
		{
			cupLevelNum = cup->cachedlevels[levelindex];

			if (cupLevelNum >= nummapheaders)
			{
				// Just skip the map if it's invalid.
				continue;
			}

			if ((mapheaderinfo[cupLevelNum]->typeoflevel & TOL_RACE) == TOL_RACE)
			{
				gtcheck = GT_RACE;
			}
			else
			{
				gtcheck = mapheaderinfo[cupLevelNum]->typeoflevel;
			}

			menuqueue.entries[menuqueue.size].mapnum = cupLevelNum;
			menuqueue.entries[menuqueue.size].gametype = gtcheck;
			menuqueue.entries[menuqueue.size].encore = (cv_kartencore.value == 1);

			menuqueue.size++;

			levelindex++;
			if (levelindex >= cup->numlevels)
				break;
		}

		// Attempt to add an interstitial Battle round.
		// If we're in singleplayer Match Race, just skip this.
		if ((levelindex < cup->numlevels
			&& bonusindex < cup->numbonus) && (levellist.netgame || (cv_splitplayers.value > 1) || netgame))
		{
			cupLevelNum = cup->cachedlevels[CUPCACHE_BONUS + bonusindex];

			if (cupLevelNum < nummapheaders)
			{
				if ((mapheaderinfo[cupLevelNum]->typeoflevel & TOL_BATTLE) == TOL_BATTLE)
				{
					gtcheck = GT_BATTLE;
				}
				else
				{
					gtcheck = mapheaderinfo[cupLevelNum]->typeoflevel;
				}
				// In the case of Bonus rounds, we simply skip invalid maps.
				menuqueue.entries[menuqueue.size].mapnum = cupLevelNum;
				menuqueue.entries[menuqueue.size].gametype = gtcheck;
				menuqueue.entries[menuqueue.size].encore = (cv_kartencore.value == 1);

				menuqueue.size++;
			}

			bonusindex++;
		}
	}
}

boolean M_LevelSelectCupSwitch(boolean next, boolean skipones)
{
	levelsearch_t templevelsearch = levellist.levelsearch;

	while (1)
	{
		if (next)
		{
			// Next
			if (++cupgrid.x >= CUPMENU_COLUMNS)
			{
				cupgrid.x = 0;
				if (++cupgrid.y >= CUPMENU_ROWS)
				{
					cupgrid.y = 0;
					if (++cupgrid.pageno >= cupgrid.numpages)
					{
						cupgrid.pageno = 0;
					}
				}
			}
		}
		else
		{
			// Prev
			if (cupgrid.x == 0)
			{
				cupgrid.x = CUPMENU_COLUMNS;
				if (cupgrid.y == 0)
				{
					cupgrid.y = CUPMENU_ROWS;
					if (cupgrid.pageno == 0)
					{
						cupgrid.pageno = cupgrid.numpages;
					}
					cupgrid.pageno--;
				}
				cupgrid.y--;
			}
			cupgrid.x--;
		}

		templevelsearch.cup = cupgrid.builtgrid[CUPMENU_CURSORID];

		if (templevelsearch.cup == levellist.levelsearch.cup)
		{
			return false;
		}

		if (!templevelsearch.cup)
		{
			continue;
		}

		UINT16 count = M_CountLevelsToShowInList(&templevelsearch);

		if (count == 0
		// The following isn't ideal, but in addition to the
		// necessary programming work being extremely annoying,
		// I also just think being forced to switch between
		// Time Attack single-course views and multi-course
		// selections would just plain kind of look bad.
		// ~toast 250124 (ON A PLANE BACK FROM MAGFEST WOOOOOOOO)
		|| (skipones && count == 1))
		{
			continue;
		}

		levellist.levelsearch = templevelsearch;

		levellist.mapcount = count;
		if (levellist.cursor >= count)
			levellist.cursor = count-1;

		M_LevelSelectScrollDest();
		levellist.slide.start = 0;

		return true;
	}
}

static void M_MenuQueueResponse(INT32 ch)
{
	M_ClearMenus(false);

	if (ch != MA_YES)
		return;

	if (!(server || (IsPlayerAdmin(consoleplayer))))
		return;

	if (!(gamestate == GS_LEVEL && roundqueue.position == 0))
		return;

	SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

// Ripped out of LevelSelectHandler for use in cup queueing from cupselect.c
void M_LevelConfirmHandler(void)
{
	// Starting immediately OR importing queue

	while ((menuqueue.size + roundqueue.size) > ROUNDQUEUE_MAX)
			menuqueue.size--;

	if (!levellist.canqueue || !menuqueue.size)
	{
		M_LevelSelected(levellist.cursor, true);
	}
	else if (netgame)
	{
		menuqueue.anchor = roundqueue.size;
		menuqueue.sending = 1;

		M_StartMessage("Queueing Rounds",
			va(M_GetText(
			"Attempting to send %d Round%s...\n"
			"\n"
			"If this is taking longer than you\n"
			"expect, exit out of this message.\n"
			), menuqueue.size, (menuqueue.size == 1 ? "" : "s")
			), &M_MenuQueueStopSend, MM_NOTHING,
			NULL,
			"This is taking too long..."
		);
	}
	else
	{
		M_MenuQueueSelectedLocal();
	}
}

static void M_ClearQueueResponse(INT32 ch)
{
	if (ch != MA_YES)
		return;

	if (!(server || (IsPlayerAdmin(consoleplayer))))
		return;

	S_StartSound(NULL, sfx_slip);

	if (!netgame)
		memset(&roundqueue, 0, sizeof(struct roundqueue));
	if (netgame && (roundqueue.size != 0))
	{
		menuqueue.clearing = true;
		Handle_MapQueueSend(0, ROUNDQUEUE_CMD_CLEAR, false);
		M_StartMessage("Clearing Rounds",
			va(M_GetText(
			"Attempting to clear %d Round%s...\n"
			"\n"
			"If this is taking longer than you\n"
			"expect, exit out of this message.\n"
			), roundqueue.size, (roundqueue.size == 1 ? "" : "s")
			), &M_MenuQueueStopSend, MM_NOTHING,
			NULL,
			"This is taking too long..."
		);
	}
}

// Ripped out of LevelSelectHandler for use in queue clearing from cupselect.c
void M_ClearQueueHandler(void)
{
	while ((menuqueue.size + roundqueue.size) > ROUNDQUEUE_MAX)
		menuqueue.size--;

	if (menuqueue.size)
	{
		S_StartSound(NULL, sfx_shldls);
		menuqueue.size--;
	}
	else if (roundqueue.size)
	{
		M_StartMessage("Queue Clearing",
			va(M_GetText(
			"There %s %d Round%s of play queued.\n"
			"\n"
			"Do you want to empty the queue?\n"
			),
			(roundqueue.size == 1 ? "is" : "are"),
			roundqueue.size,
			(roundqueue.size == 1 ? "" : "s")
			), &M_ClearQueueResponse, MM_YESNO,
			"Time to start fresh",
			"Not right now"
		);
	}
}

void M_LevelSelectHandler(INT32 choice)
{
	const UINT8 pid = 0;

	(void)choice;

	if (I_GetTime() - levellist.slide.start < M_LEVELLIST_SLIDETIME)
	{
		return;
	}

	if (menuqueue.sending)
	{
		return;
	}

	if (menucmd[pid].dpad_ud > 0)
	{
		levellist.cursor++;
		if (levellist.cursor >= levellist.mapcount)
			levellist.cursor = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		levellist.cursor--;
		if (levellist.cursor < 0)
			levellist.cursor = levellist.mapcount-1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (levellist.levelsearch.cup == NULL)
		; // Mode with no cup? No left/right input for you!
	else if (menucmd[pid].dpad_lr != 0)
	{
		if (M_LevelSelectCupSwitch(
			(menucmd[pid].dpad_lr > 0),
			levellist.levelsearch.timeattack)
		)
		{
			S_StartSound(NULL, sfx_s3k5b);
			M_SetMenuDelay(pid);
		}
	}

	M_LevelSelectScrollDest();

	if (M_MenuConfirmPressed(pid))
	{
		M_SetMenuDelay(pid);

		M_LevelConfirmHandler();
	}
	else if (levellist.canqueue && M_MenuButtonPressed(pid, MBT_Z))
	{
		// Adding to queue

		INT16 map = NEXTMAP_INVALID;

		M_SetMenuDelay(pid);

		if ((roundqueue.size + menuqueue.size) < ROUNDQUEUE_MAX)
		{
			map = M_LevelFromScrolledList(levellist.cursor);
		}

		if (map < nummapheaders)
		{
			memset(menuqueue.entries+menuqueue.size, 0, sizeof(roundentry_t));
			menuqueue.entries[menuqueue.size].mapnum = map;
			menuqueue.entries[menuqueue.size].gametype = levellist.newgametype;
			menuqueue.entries[menuqueue.size].encore = (cv_kartencore.value == 1);

			menuqueue.size++;

			S_StartSound(NULL, sfx_s3k4a);
		}
		else
		{
			S_StartSound(NULL, sfx_s3kb2);
		}
	}
	else if (levellist.canqueue && M_MenuExtraPressed(pid))
	{
		M_ClearQueueHandler();
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
	if (menuqueue.clearing)
	{
		if (roundqueue.size != 0)
			return;
		menuqueue.clearing = false;
		if (!menuqueue.cupqueue)
			M_StopMessage(MA_NONE);
		else
			menuqueue.cupqueue = false;
	}

	if (!menuqueue.sending)
		return;

	if ((menuqueue.sending <= menuqueue.size) // Sending
		&& (roundqueue.size >= menuqueue.anchor)) // Didn't get it wiped
	{
		if (!netgame)
			return;

		const UINT8 idcount = (roundqueue.size - menuqueue.anchor);

		if (idcount == menuqueue.sending-1)
		{
			Handle_MapQueueSend(
				menuqueue.entries[idcount].mapnum,
				menuqueue.entries[idcount].gametype,
				menuqueue.entries[idcount].encore
			);
		}
		if (idcount >= menuqueue.sending-1)
		{
			menuqueue.sending++;
		}
		return;
	}

	if (menuqueue.size)
	{
		S_StartSound(NULL, sfx_chchng);

		if (roundqueue.position || gamestate != GS_LEVEL)
		{
			M_StopMessage(MA_NONE);
		}
		else
		{
			M_StartMessage("Round Queue",
				va(M_GetText(
				"You just queued %d Round%s of play.\n"
				"\n"
				"Do you want to skip the current\n"
				"course and start them immediately?\n"
				), menuqueue.size, (menuqueue.size == 1 ? "" : "s")
				), &M_MenuQueueResponse, MM_YESNO,
				"Let's get going!",
				"Not right now"
			);
		}
	}

	M_MenuQueueStopSend(MA_NONE);
}
