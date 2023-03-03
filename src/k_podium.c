// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_podium.c
/// \brief Grand Prix podium cutscene

#include "k_podium.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_time.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_threads.h"
#include "dehacked.h"
#include "g_input.h"
#include "console.h"
#include "m_random.h"
#include "m_misc.h" // moviemode functionality
#include "y_inter.h"
#include "m_cond.h"
#include "p_local.h"
#include "p_setup.h"
#include "st_stuff.h" // hud hiding
#include "fastcmp.h"

#include "lua_hud.h"
#include "lua_hook.h"

#include "k_menu.h"
#include "k_grandprix.h"

static boolean s_podiumDone = false;

/*--------------------------------------------------
	boolean K_PodiumSequence(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumSequence(void)
{
	return (gamestate == GS_CEREMONY);
}

/*--------------------------------------------------
	boolean K_StartCeremony(void)

		See header file for description.
--------------------------------------------------*/
boolean K_StartCeremony(void)
{
	INT32 podiumMapNum = nummapheaders;
	INT32 i;

	if (grandprixinfo.gp == false)
	{
		return false;
	}

	if (podiummap
		&& ((podiumMapNum = G_MapNumber(podiummap)) < nummapheaders)
		&& mapheaderinfo[podiumMapNum]
		&& mapheaderinfo[podiumMapNum]->lumpnum != LUMPERROR)
	{
		P_SetTarget(&titlemapcam.mobj, NULL);

		gamemap = podiumMapNum+1;

		maptol = mapheaderinfo[gamemap-1]->typeoflevel;
		globalweather = mapheaderinfo[gamemap-1]->weather;

		// Make sure all of the GAME OVER'd players can spawn
		// and be present for the podium
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator && !players[i].bot)
			{
				players[i].lives = max(1, players[i].lives);
			}
		}

		G_DoLoadLevelEx(false, GS_CEREMONY);

		r_splitscreen = 0; // Only one screen for the ceremony
		R_ExecuteSetViewSize();
		return true;
	}

	return false;
}

/*--------------------------------------------------
	void K_FinishCeremony(void)

		See header file for description.
--------------------------------------------------*/
void K_FinishCeremony(void)
{
	if (K_PodiumSequence() == false)
	{
		return;
	}

	s_podiumDone = true;
}

/*--------------------------------------------------
	void K_ResetCeremony(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetCeremony(void)
{
	s_podiumDone = false;
}

/*--------------------------------------------------
	void K_CeremonyTicker(boolean run)

		See header file for description.
--------------------------------------------------*/
void K_CeremonyTicker(boolean run)
{
	(void)run;

	// don't trigger if doing anything besides idling
	if (gameaction != ga_nothing || gamestate != GS_CEREMONY)
	{
		return;
	}

	P_TickAltView(&titlemapcam);

	if (titlemapcam.mobj != NULL)
	{
		camera[0].x = titlemapcam.mobj->x;
		camera[0].y = titlemapcam.mobj->y;
		camera[0].z = titlemapcam.mobj->z;
		camera[0].angle = titlemapcam.mobj->angle;
		camera[0].aiming = titlemapcam.mobj->pitch;
		camera[0].subsector = titlemapcam.mobj->subsector;
	}
}

/*--------------------------------------------------
	boolean K_CeremonyResponder(event_t *event)

		See header file for description.
--------------------------------------------------*/
boolean K_CeremonyResponder(event_t *event)
{
	INT32 key = event->data1;

	if (s_podiumDone == false)
	{
		return false;
	}

	// remap virtual keys (mouse & joystick buttons)
	switch (key)
	{
		case KEY_MOUSE1:
			key = KEY_ENTER;
			break;
		case KEY_MOUSE1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_JOY1:
		case KEY_JOY1 + 2:
			key = KEY_ENTER;
			break;
		case KEY_JOY1 + 3:
			key = 'n';
			break;
		case KEY_JOY1 + 1:
			key = KEY_BACKSPACE;
			break;
		case KEY_HAT1:
			key = KEY_UPARROW;
			break;
		case KEY_HAT1 + 1:
			key = KEY_DOWNARROW;
			break;
		case KEY_HAT1 + 2:
			key = KEY_LEFTARROW;
			break;
		case KEY_HAT1 + 3:
			key = KEY_RIGHTARROW;
			break;
	}

	if (event->type != ev_keydown)
	{
		return false;
	}

	if (key != KEY_ESCAPE && key != KEY_ENTER && key != KEY_BACKSPACE)
	{
		return false;
	}

	return true;
}

/*--------------------------------------------------
	void K_CeremonyDrawer(void)

		See header file for description.
--------------------------------------------------*/
void K_CeremonyDrawer(void)
{
	if (s_podiumDone == true)
	{
		V_DrawFadeScreen(0xFF00, 16);
		V_DrawCenteredString(BASEVIDWIDTH / 2, 64, 0, "STUFF GOES HERE");
	}

	if (timeinmap < 16)
	{
		// Level fade-in
		V_DrawCustomFadeScreen(((levelfadecol == 0) ? "FADEMAP1" : "FADEMAP0"), 31-(timeinmap*2));
	}
}
