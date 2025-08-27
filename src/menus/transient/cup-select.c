// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/cup-select.c
/// \brief Cup Select

#include "../../i_time.h"
#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../f_finale.h" // F_WipeStartScreen
#include "../../v_video.h"
#include "../../k_grandprix.h"
#include "../../r_local.h" // SplitScreen_OnChange
#include "../../k_podium.h" // K_StartCeremony
#include "../../m_misc.h" // FIL_FileExists
#include "../../d_main.h" // D_ClearState
#include "../../m_cond.h" // Condition Sets

menuitem_t PLAY_CupSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_CupSelectHandler}, 0, 0},
};

menu_t PLAY_CupSelectDef = {
	sizeof(PLAY_CupSelect) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_CupSelect,
	0, 0,
	0, 0,
	0,
	NULL,
	2, 5,
	M_DrawCupSelect,
	NULL,
	M_CupSelectTick,
	NULL,
	NULL,
	NULL
};

struct cupgrid_s cupgrid;

static void M_StartCup(UINT8 entry)
{
	UINT8 ssplayers = cv_splitplayers.value-1;

	if (ssplayers > 0)
	{
		// Splitscreen is not accomodated with this recovery feature.
		entry = UINT8_MAX;
	}

	M_MenuToLevelPreamble(ssplayers, false);

	if (entry == UINT8_MAX)
	{
		entry = 0;
		memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));

		// read our dummy cvars

		grandprixinfo.gamespeed = min(KARTSPEED_HARD, cv_dummygpdifficulty.value);
		grandprixinfo.masterbots = (cv_dummygpdifficulty.value == 3);

		grandprixinfo.gp = true;
		grandprixinfo.initalize = true;
		grandprixinfo.cup = levellist.levelsearch.cup;

		// Populate the roundqueue
		memset(&roundqueue, 0, sizeof(struct roundqueue));
		G_GPCupIntoRoundQueue(levellist.levelsearch.cup, levellist.newgametype,
#if 0 // TODO: encore GP
			(boolean)cv_dummygpencore.value
#else
			false
#endif
		);
		roundqueue.position = roundqueue.roundnum = 1;
		roundqueue.netcommunicate = true; // relevant for future Online GP
	}
	else
	{
		// Silently change player setup
		{
			CV_StealthSetValue(&cv_playercolor[0], savedata.skincolor);

			// follower
			if (savedata.followerskin < 0 || savedata.followerskin >= numfollowers)
				CV_StealthSet(&cv_follower[0], "None");
			else
				CV_StealthSet(&cv_follower[0], followers[savedata.followerskin].name);

			// finally, call the skin[x] console command.
			// This will call SendNameAndColor which will synch everything we sent here and apply the changes!

			CV_StealthSet(&cv_skin[0], skins[savedata.skin]->name);

			// ...actually, let's do this last - Skin_OnChange has some return-early occasions
			// follower color
			CV_SetValue(&cv_followercolor[0], savedata.followercolor);
		}

		// Skip Bonus rounds.
		if (roundqueue.entries[entry].gametype != GT_RACE // roundqueue.entries[0].gametype
			&& roundqueue.entries[entry].rankrestricted == false)
		{
			G_GetNextMap(); // updates position in the roundqueue
			entry = roundqueue.position-1;
		}
	}

	M_ClearMenus(true);

	restoreMenu = &PLAY_CupSelectDef;
	restorelevellist = levellist;

	if (entry < roundqueue.size)
	{
		D_MapChange(
			roundqueue.entries[entry].mapnum + 1,
			roundqueue.entries[entry].gametype,
			roundqueue.entries[entry].encore,
			true,
			1,
			false,
			roundqueue.entries[entry].rankrestricted
		);
	}
	else if (entry == 0)
	{
		I_Error("M_StartCup: roundqueue is empty on startup!!");
	}
	else
	{
		if (K_StartCeremony() == false)
		{
			// Accomodate our buffoonery
			D_ClearState();
			M_StartControlPanel();

			M_StartMessage(
				"Grand Prix Backup",
				"The session is concluded!\n"
				"You exited a final Bonus Round,\n"
				"and the Podium failed to load.\n",
				NULL, MM_NOTHING, NULL, NULL
			);

			if (FIL_FileExists(gpbackup))
				remove(gpbackup);

			return;
		}
	}
}

static UINT16 cupselecttutorial_hack = NEXTMAP_INVALID;

static void M_GPTutorialResponse(INT32 choice)
{
	if (choice != MA_YES)
		return;

	multiplayer = true;

	restoreMenu = &PLAY_CupSelectDef;
	restorelevellist = levellist;

	// mild hack
	levellist.newgametype = GT_TUTORIAL;
	levellist.netgame = false;
	M_MenuToLevelPreamble(0, false);

	D_MapChange(
		cupselecttutorial_hack+1,
		levellist.newgametype,
		false,
		true,
		1,
		false,
		false
	);

	M_ClearMenus(true);
}

static boolean M_GPTutorialRecommendation(cupheader_t *cup)
{
	// Only applies to GP.
	if (levellist.levelsearch.grandprix == false)
		return false;

	// Does this not have a Tutorial Recommendation?
	if (cup->cache_cuplock >= MAXUNLOCKABLES
	|| cup->hintcondition == MAXCONDITIONSETS
	|| !M_Achieved(cup->hintcondition))
		return false;

	// Does the thing have no condition?
	const UINT16 condition = unlockables[cup->cache_cuplock].conditionset;
	if (condition == 0)
		return false;

	const conditionset_t *c = &conditionSets[condition-1];
	UINT32 i;
	INT32 mapnum = NEXTMAP_INVALID;

	// Identify the map to visit/beat.
	for (i = 0; i < c->numconditions; ++i)
	{
		if (c->condition[i].type < UC_MAPVISITED || c->condition[i].type > UC_MAPBEATEN)
			continue;
		mapnum = c->condition[i].requirement;
		if (mapnum < 0 || mapnum >= nummapheaders)
			continue;
		if (!mapheaderinfo[mapnum])
			continue;
		if (G_GuessGametypeByTOL(mapheaderinfo[mapnum]->typeoflevel) != GT_TUTORIAL)
			continue;
		break;
	}

	// Didn't find one?
	if (i == c->numconditions)
		return false;

	// Not unlocked?
	if (M_MapLocked(mapnum+1))
	{
		M_StartMessage(
			"Recommended Learning",
			"This Cup will test skills that\n"
			"a ""\x86""currently locked ""\x87""Tutorial\x80 teaches.\n"
			"\n"
			"Come back when you've made progress elsewhere.",
			NULL, MM_NOTHING, NULL, NULL
		);
		return true;
	}

	// This is kind of a hack.
	cupselecttutorial_hack = mapnum;

	M_StartMessage(
		"Recommended Learning",
		va(
		"This Cup will test skills that\n"
		"the ""\x87""%s Tutorial\x80 teaches.\n"
		"\n"
		"%s\n",
		mapheaderinfo[mapnum]->menuttl,
		(setup_numplayers > 1
			? "You're encouraged to play it later."
			: "Would you like to play it now?"
		)),
		setup_numplayers > 1 ? NULL : M_GPTutorialResponse,
		setup_numplayers > 1 ? MM_NOTHING : MM_YESNO,
		setup_numplayers > 1 ? NULL : "Yes, let's go",
		setup_numplayers > 1 ? "Got it!" : "Not right now"
	);

	return true;
}

static void M_GPBackup(INT32 choice)
{
	if (choice == MA_YES)
	{
		G_LoadGame();

		if (savedata.lives != 0)
		{
			M_StartCup(roundqueue.position-1);
		}

		return;
	}

	M_StartCup(UINT8_MAX);
}

void M_CupSelectHandler(INT32 choice)
{
	const UINT8 pid = 0;

	(void)choice;

	if (menucmd[pid].dpad_lr > 0)
	{
		cupgrid.x++;
		if (cupgrid.x >= CUPMENU_COLUMNS)
		{
			cupgrid.x = 0;
			cupgrid.pageno++;
			if (cupgrid.pageno >= cupgrid.numpages)
				cupgrid.pageno = 0;
		}
		cupgrid.xslide.start = I_GetTime();
		cupgrid.xslide.dist = 42;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_lr < 0)
	{
		cupgrid.x--;
		if (cupgrid.x < 0)
		{
			cupgrid.x = CUPMENU_COLUMNS-1;
			if (cupgrid.pageno == 0)
				cupgrid.pageno = cupgrid.numpages-1;
			else
				cupgrid.pageno--;
		}
		cupgrid.xslide.start = I_GetTime();
		cupgrid.xslide.dist = -42;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (cupgrid.cache_secondrowlocked == true)
		; // No up/down for you!
	else if (menucmd[pid].dpad_ud > 0)
	{
		cupgrid.y++;
		if (cupgrid.y >= CUPMENU_ROWS)
		{
			cupgrid.y = 0;
			cupgrid.yslide.dist = 8;
		}
		else
			cupgrid.yslide.dist = 44;
		cupgrid.yslide.start = I_GetTime();
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		cupgrid.y--;
		if (cupgrid.y < 0)
		{
			cupgrid.y = CUPMENU_ROWS-1;
			cupgrid.yslide.dist = -8;
		}
		else
			cupgrid.yslide.dist = -44;
		cupgrid.yslide.start = I_GetTime();
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (M_MenuConfirmPressed(pid) /*|| M_MenuButtonPressed(pid, MBT_START)*/)
	{
		UINT16 count;
		cupheader_t *newcup = cupgrid.builtgrid[CUPMENU_CURSORID];
		cupheader_t *oldcup = levellist.levelsearch.cup;

		M_SetMenuDelay(pid);

		if (!newcup)
		{
			S_StartSound(NULL, sfx_s3kb2);
			return;
		}

		levellist.levelsearch.cup = newcup;
		count = M_CountLevelsToShowInList(&levellist.levelsearch);

		if (count == 0
			|| (
				levellist.levelsearch.grandprix == true
				&& newcup->cachedlevels[0] == NEXTMAP_INVALID
			)
		)
		{
			if (!M_GPTutorialRecommendation(newcup))
				S_StartSound(NULL, sfx_s3kb2);
			return;
		}

		if (levellist.levelsearch.grandprix == true)
		{
			if (newcup == cupsavedata.cup
			&& FIL_FileExists(gpbackup))
			{
				M_StartMessage(
					"Grand Prix Backup",
					"A progress backup was found.\n"
					"Do you want to resurrect your\n"
					"last Grand Prix session?\n",
					M_GPBackup,
					MM_YESNO,
					"Yes, let's try again",
					"No, start from Round 1"
				);

				return;
			}

			M_StartCup(UINT8_MAX);
		}
		else if (count == 1 && levellist.levelsearch.timeattack == true)
		{
			currentMenu->transitionID = PLAY_TimeAttackDef.transitionID+1;
			M_LevelSelected(0, true);
		}
		else
		{
			currentMenu->transitionID = PLAY_LevelSelectDef.transitionID;

			// Keep cursor position if you select the same cup again, reset if it's a different cup
			if (oldcup != newcup || levellist.cursor >= count)
			{
				levellist.cursor = 0;
			}

			levellist.mapcount = count;
			M_LevelSelectScrollDest();
			levellist.slide.start = 0;

			M_SetupNextMenu(&PLAY_LevelSelectDef, false);
			S_StartSound(NULL, sfx_s3k63);
		}
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

void M_CupSelectTick(void)
{
	cupgrid.previewanim++;
}
