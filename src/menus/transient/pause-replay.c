// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/pause-replay.c
/// \brief Replay popup menu

#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../p_tick.h" // leveltime
#include "../../i_time.h"
#include "../../r_main.h" // R_ExecuteSetViewSize
#include "../../p_local.h" // P_InitCameraCmd
#include "../../d_main.h" // D_StartTitle
#include "../../k_credits.h"
#include "../../g_demo.h"
#include "../../k_director.h"

static void M_PlaybackTick(void);

// This is barebones, just toggle director on all screens.
static void M_PlaybackToggleDirector(INT32 choice)
{
	(void)choice;

	UINT8 i;
	for (i = 0; i <= r_splitscreen; ++i)
	{
		K_ToggleDirector(i, !K_DirectorIsEnabled(i));
	}
}

menuitem_t PAUSE_PlaybackMenu[] =
{
	{IT_CALL   | IT_STRING, "Hide Menu",			NULL, "M_PHIDE",	{.routine = M_SelectableClearMenus},	  0, 0},

	{IT_CALL   | IT_STRING, "Restart",				NULL, "M_PRSTRT",	{.routine = M_PlaybackRewind},			 20, 0},
	{IT_CALL   | IT_STRING, "Rewind 10 seconds",	NULL, "M_PREW",		{.routine = M_PlaybackRewind},			 36, 0},
	{IT_CALL   | IT_STRING, "Pause",				NULL, "M_PPAUSE",	{.routine = M_PlaybackPause},			 52, 0},
	{IT_CALL   | IT_STRING, "Fast-Forward",			NULL, "M_PFFWD",	{.routine = M_PlaybackFastForward},		 68, 0},
	{IT_CALL   | IT_STRING, "Resume",				NULL, "M_PRESUM",	{.routine = M_PlaybackPause},			 52, 0},
	{IT_CALL   | IT_STRING, "Advance Frame",		NULL, "M_PFADV",	{.routine = M_PlaybackAdvance},			 68, 0},

	{IT_ARROWS | IT_STRING, "View Count",			NULL, "M_PVIEWS",	{.routine = M_PlaybackSetViews},		 88, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		104, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 2",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		120, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 3",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		136, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 4",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		152, 0},

	{IT_CALL   | IT_STRING, "Toggle Director",		NULL, "UN_IC11A",	{.routine = M_PlaybackToggleDirector},	172, 0},
	{IT_CALL   | IT_STRING, "Toggle Free Camera",	NULL, "M_PVIEWS",	{.routine = M_PlaybackToggleFreecam},	188, 0},
	{IT_CALL   | IT_STRING, "Stop Playback",		NULL, "M_PEXIT",	{.routine = M_PlaybackQuit},			204, 0},
};

menu_t PAUSE_PlaybackMenuDef = {
	sizeof (PAUSE_PlaybackMenu) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_PlaybackMenu,
	BASEVIDWIDTH/2 - 96, 2,
	0, 0,
	MBF_UD_LR_FLIPPED,
	NULL,
	0, 0,
	M_DrawPlaybackMenu,
	NULL,
	M_PlaybackTick,
	NULL,
	NULL,
	NULL
};

void M_EndModeAttackRun(void)
{
	// End recording / playback.
	// Why not check demo.recording?
	// Because for recording, this may be called from G_AfterIntermission.
	// And before this function is called, G_SaveDemo is called, which sets demo.recording to false.
	// Don't need to check demo.playback; G_CheckDemoStatus is safe to call even outside of demos.
	// Check modeattacking because this function is recursively called (read on for an explanation).
	if (modeattacking)
	{
		// This must be called for both playback and
		// recording, because it both finishes playback and
		// frees ghost data.
		G_CheckDemoStatus();

		// What does G_CheckDemoStatus do? Here's the answer!

		// Playback:
		// - Clears everything, including demo state and modeattacking.
		// - It then calls the current function (M_EndModeAttackRun) AGAIN (after everything was cleared), so return.
		if (!modeattacking)
			return;

		// Recording:
		// - Only saves the demo and clears the demo state.
		// - Now we need to clear the rest of the gamestate ourself!
	}

	// Playback:
	// - modeattacking is always false, so calling this returns to the menu.
	// - Because modeattacking is false, also clears demo.attract.
	//
	// Recording:
	// - modeattacking is still true and this function call preserves that.
	Command_ExitGame_f();

	if (!modeattacking)
		return;

	// The rest of this is relevant for recording ONLY.

	if (nextmapoverride != 0)
	{
		M_StartMessage(
			"Secret Exit",
			va(
				"No finish time was recorded.\n"
				"Secrets don't work in Attack modes!\n"
				"Try again in %s.\n",
				(gametype == GT_RACE)
					? "Grand Prix or Match Race"
					: "Grand Prix"
			),
			NULL, MM_NOTHING, NULL, NULL
		);
	}

	// Command_ExitGame_f didn't clear this, so now we do.
	modeattacking = ATTACKING_NONE;

	// Return to the menu.
	D_ClearState();
	M_StartControlPanel();
	// This removes any lingering optionsmenu.profile pointers,
	// so that when returning to char select, it will show the normal
	// stat grid instead of the profile one (this is also most likely a bad way to fix this)
	M_ResetOptions();
}

// Replay Playback Menu

tic_t playback_last_menu_interaction_leveltime = 0;

static void M_PlaybackTick(void)
{
	INT16 i;

	if (leveltime - playback_last_menu_interaction_leveltime >= 6*TICRATE)
		playback_last_menu_interaction_leveltime = leveltime - 6*TICRATE;

	// Toggle items
	if (paused)
	{
		PAUSE_PlaybackMenu[playback_pause].status = PAUSE_PlaybackMenu[playback_fastforward].status = IT_DISABLED;
		PAUSE_PlaybackMenu[playback_resume].status = PAUSE_PlaybackMenu[playback_advanceframe].status = IT_CALL|IT_STRING;

		if (itemOn >= playback_pause && itemOn <= playback_fastforward)
			itemOn += playback_resume - playback_pause;
	}
	else
	{
		PAUSE_PlaybackMenu[playback_pause].status = PAUSE_PlaybackMenu[playback_fastforward].status = IT_CALL|IT_STRING;
		PAUSE_PlaybackMenu[playback_resume].status = PAUSE_PlaybackMenu[playback_advanceframe].status = IT_DISABLED;

		if (itemOn >= playback_resume && itemOn <= playback_advanceframe)
			itemOn -= playback_resume - playback_pause;
	}

	if (modeattacking)
	{
		for (i = playback_viewcount; i <= playback_director; i++)
			PAUSE_PlaybackMenu[i].status = IT_DISABLED;

		PAUSE_PlaybackMenu[playback_freecam].mvar1 = 88;
		PAUSE_PlaybackMenu[playback_quit].mvar1 = 104;

		currentMenu->x = BASEVIDWIDTH/2 - 60;
	}
	else
	{
		PAUSE_PlaybackMenu[playback_viewcount].status = IT_ARROWS|IT_STRING;
		PAUSE_PlaybackMenu[playback_director].status = IT_ARROWS|IT_STRING;
		PAUSE_PlaybackMenu[playback_freecam].status = IT_CALL|IT_STRING;

		for (i = 0; i <= r_splitscreen; i++)
			PAUSE_PlaybackMenu[playback_view1+i].status = IT_ARROWS|IT_STRING;
		for (i = r_splitscreen+1; i < 4; i++)
			PAUSE_PlaybackMenu[playback_view1+i].status = IT_DISABLED;

		PAUSE_PlaybackMenu[playback_freecam].mvar1 = 188;
		PAUSE_PlaybackMenu[playback_quit].mvar1 = 204;

		//currentMenu->x = BASEVIDWIDTH/2 - 102;
		currentMenu->x = BASEVIDWIDTH/2 - 104;
	}
}

void M_SetPlaybackMenuPointer(void)
{
	itemOn = playback_pause;
}

void M_PlaybackRewind(INT32 choice)
{
	const tic_t curleveltime = leveltime;

	if (choice == playback_rewind)
	{
		demo.simplerewind = (paused ? DEMO_REWIND_PAUSE : DEMO_REWIND_RESUME);
		menuactive = false;
	}

	G_DoPlayDemo(NULL); // Restart the current demo

	if (demo.simplerewind)
	{
		if (curleveltime > 10*TICRATE)
		{
			g_fast_forward = curleveltime - (10 * TICRATE);
			g_fast_forward_clock_stop = INFTICS; //I_GetTime() + 2 * TICRATE; -- maybe?
		}
		else
		{
			if (demo.simplerewind == DEMO_REWIND_PAUSE)
			{
				paused = true;
				S_PauseAudio();
			}
			demo.simplerewind = DEMO_REWIND_OFF;
		}
		menuactive = true;
	}
	else
	{
		M_ClearMenus(true);
	}
}

void M_PlaybackPause(INT32 choice)
{
	(void)choice;

	paused = !paused;

	if (paused)
		S_PauseAudio();
	else
		S_ResumeAudio();

	CV_SetValue(&cv_playbackspeed, 1);
}

void M_PlaybackFastForward(INT32 choice)
{
	(void)choice;

	CV_SetValue(&cv_playbackspeed, cv_playbackspeed.value == 1 ? 4 : 1);
}

void M_PlaybackAdvance(INT32 choice)
{
	(void)choice;

	paused = false;
	TryRunTics(1);
	paused = true;
}

void M_PlaybackSetViews(INT32 choice)
{
	if (choice > 0)
	{
		if (r_splitscreen < 3)
			G_AdjustView(r_splitscreen + 2, 0, true);
	}
	else if (r_splitscreen)
	{
		if (choice == 0)
		{
			G_SyncDemoParty(displayplayers[r_splitscreen], r_splitscreen - 1);
		}
		else
		{
			G_SyncDemoParty(consoleplayer, 0);
		}
	}
}

void M_PlaybackAdjustView(INT32 choice)
{
	G_AdjustView(itemOn - playback_viewcount, (choice > 0) ? 1 : -1, true);
}

// this one's rather tricky
void M_PlaybackToggleFreecam(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);

	// remove splitscreen:
	splitscreen = 0;
	R_ExecuteSetViewSize();

	UINT8 i;
	for (i = 0; i <= r_splitscreen; ++i)
	{
		P_ToggleDemoCamera(i);
	}
}

void M_PlaybackQuit(INT32 choice)
{
	(void)choice;
	G_StopDemo();

	if (modeattacking)
		M_EndModeAttackRun();
	else if (restoreMenu)
		M_StartControlPanel();
	else
		D_StartTitle();
}
