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

static struct podiumData_s
{
	boolean ranking;
	UINT8 fade;
} podiumData;

/*--------------------------------------------------
	boolean K_PodiumSequence(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumSequence(void)
{
	return (gamestate == GS_CEREMONY);
}

/*--------------------------------------------------
	UINT8 K_GetPodiumPosition(player_t *player)

		See header file for description.
--------------------------------------------------*/
UINT8 K_GetPodiumPosition(player_t *player)
{
	UINT8 position = 1;
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *other = NULL;
		if (playeringame[i] == false)
		{
			continue;
		}

		other = &players[i];
		if (other->spectator == true)
		{
			continue;
		}

		if (other->score > player->score)
		{
			// Final score is the important part.
			position++;
		}
		else if (other->score == player->score)
		{
			if (other->bot == false && player->bot == true)
			{
				// Bots are never as important as players.
				position++;
			}
			else if (i < player - players)
			{
				// Port priority is the final tie breaker.
				position++;
			}
		}
	}

	return position;
}

/*--------------------------------------------------
	static void K_SetPodiumWaypoint(player_t *const player, waypoint_t *const waypoint)

		Changes the player's current and next waypoints, for
		use during the podium sequence.

	Input Arguments:-
		player - The player to update the waypoints of.
		waypoint - The new current waypoint.

	Return:-
		None
--------------------------------------------------*/
static void K_SetPodiumWaypoint(player_t *const player, waypoint_t *const waypoint)
{
	// Set the new waypoint.
	player->currentwaypoint = waypoint;

	if ((waypoint == NULL)
		|| (waypoint->nextwaypoints == NULL)
		|| (waypoint->numnextwaypoints == 0U))
	{
		// No waypoint, or no next waypoint.
		player->nextwaypoint = NULL;
		return;
	}

	// Simply use the first available next waypoint.
	// No need for split paths in these cutscenes.
	player->nextwaypoint = waypoint->nextwaypoints[0];
}

/*--------------------------------------------------
	void K_InitializePodiumWaypoint(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_InitializePodiumWaypoint(player_t *const player)
{
	if ((player != NULL) && (player->mo != NULL))
	{
		player->position = K_GetPodiumPosition(player);

		if (player->position > 0 && player->position <= MAXPLAYERS)
		{
			// Initialize our first waypoint to the one that
			// matches our position.
			K_SetPodiumWaypoint(player, K_GetWaypointFromID(player->position));
		}
		else
		{
			// None does, so remove it if we happen to have one.
			K_SetPodiumWaypoint(player, NULL);
		}
	}
}

/*--------------------------------------------------
	void K_UpdatePodiumWaypoints(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_UpdatePodiumWaypoints(player_t *const player)
{
	if ((player != NULL) && (player->mo != NULL))
	{
		if (player->currentwaypoint != NULL)
		{
			const fixed_t xydist = P_AproxDistance(
				player->mo->x - player->currentwaypoint->mobj->x,
				player->mo->y - player->currentwaypoint->mobj->y
			);
			const fixed_t xyzdist = P_AproxDistance(
				xydist,
				player->mo->z - player->currentwaypoint->mobj->z
			);
			//const fixed_t speed = P_AproxDistance(player->mo->momx, player->mo->momy);

			if (xyzdist <= player->mo->radius + player->currentwaypoint->mobj->radius)
			{
				// Reached waypoint, go to the next waypoint.
				K_SetPodiumWaypoint(player, player->nextwaypoint);
			}
		}
	}
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

		G_SetGametype(GT_RACE);
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

	podiumData.ranking = true;
}

/*--------------------------------------------------
	void K_ResetCeremony(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetCeremony(void)
{
	memset(&podiumData, 0, sizeof(struct podiumData_s));
}

/*--------------------------------------------------
	void K_CeremonyTicker(boolean run)

		See header file for description.
--------------------------------------------------*/
void K_CeremonyTicker(boolean run)
{
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

	if (podiumData.ranking == false)
	{
		return;
	}

	if (run == true)
	{
		if (podiumData.fade < 16)
		{
			podiumData.fade++;
		}
	}
}

/*--------------------------------------------------
	boolean K_CeremonyResponder(event_t *event)

		See header file for description.
--------------------------------------------------*/
boolean K_CeremonyResponder(event_t *event)
{
	INT32 key = event->data1;

	if (podiumData.ranking == false || podiumData.fade < 16)
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
	if (podiumData.ranking == true)
	{
		V_DrawFadeScreen(0xFF00, podiumData.fade);
		V_DrawCenteredString(BASEVIDWIDTH / 2, 64, 0, "STUFF GOES HERE");
	}

	if (timeinmap < 16)
	{
		// Level fade-in
		V_DrawCustomFadeScreen(((levelfadecol == 0) ? "FADEMAP1" : "FADEMAP0"), 31-(timeinmap*2));
	}
}
