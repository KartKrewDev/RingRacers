/// \file  menus/transient/cup-select.c
/// \brief Cup Select

#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../f_finale.h" // F_WipeStartScreen
#include "../../v_video.h"
#include "../../k_grandprix.h"
#include "../../r_local.h" // SplitScreen_OnChange

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
	M_CupSelectTick,
	NULL,
	NULL,
	NULL
};

struct cupgrid_s cupgrid;

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
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (menucmd[pid].dpad_ud > 0)
	{
		cupgrid.y++;
		if (cupgrid.y >= CUPMENU_ROWS)
			cupgrid.y = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		cupgrid.y--;
		if (cupgrid.y < 0)
			cupgrid.y = CUPMENU_ROWS-1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (M_MenuConfirmPressed(pid) /*|| M_MenuButtonPressed(pid, MBT_START)*/)
	{
		UINT16 count;
		cupheader_t *newcup = cupgrid.builtgrid[CUPMENU_CURSORID];
		cupheader_t *oldcup = levellist.levelsearch.cup;

		M_SetMenuDelay(pid);

		levellist.levelsearch.cup = newcup;
		count = M_CountLevelsToShowInList(&levellist.levelsearch);

		if ((!newcup)
			|| (count == 0)
			|| (cupgrid.grandprix == true && newcup->cachedlevels[0] == NEXTMAP_INVALID))
		{
			S_StartSound(NULL, sfx_s3kb2);
			return;
		}

		if (cupgrid.grandprix == true)
		{
			UINT8 ssplayers = cv_splitplayers.value-1;

			S_StartSound(NULL, sfx_s3k63);

			// Early fadeout to let the sound finish playing
			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();
			F_RunWipe(wipe_level_toblack, wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

			memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));

			if (cv_maxconnections.value < ssplayers+1)
				CV_SetValue(&cv_maxconnections, ssplayers+1);

			if (splitscreen != ssplayers)
			{
				splitscreen = ssplayers;
				SplitScreen_OnChange();
			}

			// read our dummy cvars

			grandprixinfo.gamespeed = min(KARTSPEED_HARD, cv_dummygpdifficulty.value);
			grandprixinfo.masterbots = (cv_dummygpdifficulty.value == 3);

			grandprixinfo.gp = true;
			grandprixinfo.initalize = true;
			grandprixinfo.cup = newcup;

			// Populate the roundqueue
			memset(&roundqueue, 0, sizeof(struct roundqueue));
			G_GPCupIntoRoundQueue(newcup, levellist.newgametype, (boolean)cv_dummygpencore.value);
			roundqueue.position = roundqueue.roundnum = 1;

			paused = false;

			// Don't restart the server if we're already in a game lol
			if (gamestate == GS_MENU)
			{
				SV_StartSinglePlayerServer(levellist.newgametype, levellist.netgame);
			}

			D_MapChange(
				roundqueue.entries[0].mapnum + 1,
				roundqueue.entries[0].gametype,
				roundqueue.entries[0].encore,
				true,
				1,
				false,
				roundqueue.entries[0].rankrestricted
			);

			M_ClearMenus(true);

			restoreMenu = &PLAY_CupSelectDef;
		}
		else if (count == 1 && levellist.levelsearch.timeattack == true)
		{
			PLAY_TimeAttackDef.transitionID = currentMenu->transitionID+1;
			M_LevelSelected(0);
		}
		else
		{
			// Keep cursor position if you select the same cup again, reset if it's a different cup
			if (oldcup != newcup || levellist.cursor >= count)
			{
				levellist.cursor = 0;
			}

			levellist.mapcount = count;
			M_LevelSelectScrollDest();
			levellist.y = levellist.dest;

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
