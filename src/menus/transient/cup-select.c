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
		entry = 0;
	}

	S_StartSound(NULL, sfx_s3k63);

	paused = false;

	S_StopMusicCredit();

	// Early fadeout to let the sound finish playing
	F_WipeStartScreen();
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	F_WipeEndScreen();
	F_RunWipe(wipe_level_toblack, wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

	if (cv_maxconnections.value < ssplayers+1)
		CV_SetValue(&cv_maxconnections, ssplayers+1);

	if (splitscreen != ssplayers)
	{
		splitscreen = ssplayers;
		SplitScreen_OnChange();
	}

	if (entry == 0)
	{
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

			CV_StealthSet(&cv_skin[0], skins[savedata.skin].name);

			// ...actually, let's do this last - Skin_OnChange has some return-early occasions
			// follower color
			CV_SetValue(&cv_followercolor[0], savedata.followercolor);
		}

		// Skip Bonus rounds.
		if (roundqueue.entries[entry].gametype != roundqueue.entries[0].gametype
			&& roundqueue.entries[entry].rankrestricted == false)
		{
			G_GetNextMap(); // updates position in the roundqueue
			entry = roundqueue.position-1;
		}
	}

	paused = false;

	SV_StartSinglePlayerServer(levellist.newgametype, levellist.netgame);

	M_ClearMenus(true);
	restoreMenu = &PLAY_CupSelectDef;

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

	M_StartCup(0);
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
				cupgrid.grandprix == true
				&& newcup->cachedlevels[0] == NEXTMAP_INVALID
			)
		)
		{
			S_StartSound(NULL, sfx_s3kb2);
			return;
		}

		if (cupgrid.grandprix == true)
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

			M_StartCup(0);
		}
		else if (count == 1 && levellist.levelsearch.timeattack == true)
		{
			currentMenu->transitionID = PLAY_TimeAttackDef.transitionID+1;
			M_LevelSelected(0);
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
