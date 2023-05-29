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
#include "k_rank.h"

static struct podiumData_s
{
	boolean ranking;
	gpRank_t rank;
	gp_rank_e grade;
	UINT8 state;
	UINT8 delay;
	UINT8 fade;
} podiumData;

#define PODIUM_STATES (9) // TODO: enum when this actually gets made

/*--------------------------------------------------
	boolean K_PodiumSequence(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumSequence(void)
{
	return (gamestate == GS_CEREMONY);
}

/*--------------------------------------------------
	boolean K_PodiumRanking(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumRanking(void)
{
	return (gamestate == GS_CEREMONY && podiumData.ranking == true);
}

/*--------------------------------------------------
	boolean K_PodiumGrade(void)

		See header file for description.
--------------------------------------------------*/
gp_rank_e K_PodiumGrade(void)
{
	if (K_PodiumSequence() == false)
	{
		return 0;
	}

	return podiumData.grade;
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
			if (playeringame[i])
			{
				if (players[i].lives < 1)
					players[i].lives = 1;

				if (players[i].bot)
					players[i].spectator = false;
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

	// Play the noise now
	M_UpdateUnlockablesAndExtraEmblems(true, true);
	G_SaveGameData();
}

/*--------------------------------------------------
	void K_ResetCeremony(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetCeremony(void)
{
	UINT8 i;

	memset(&podiumData, 0, sizeof(struct podiumData_s));

	if (K_PodiumSequence() == false)
	{
		return;
	}

	// Establish rank and grade for this play session.
	podiumData.rank = grandprixinfo.rank;
	podiumData.grade = K_CalculateGPGrade(&podiumData.rank);

	if (!grandprixinfo.cup)
	{
		return;
	}

	// Write grade, position, and emerald-having-ness for later sessions!
	i = (grandprixinfo.masterbots) ? KARTGP_MASTER : grandprixinfo.gamespeed;

	if ((grandprixinfo.cup->windata[i].best_placement == 0) // First run
		|| (podiumData.rank.position < grandprixinfo.cup->windata[i].best_placement)) // Later, better run
	{
		grandprixinfo.cup->windata[i].best_placement = podiumData.rank.position;

		// The following will not occour in unmodified builds, but pre-emptively sanitise gamedata if someone just changes MAXPLAYERS and calls it a day
		if (grandprixinfo.cup->windata[i].best_placement > 0x0F)
			grandprixinfo.cup->windata[i].best_placement = 0x0F;
	}

	if (podiumData.grade > grandprixinfo.cup->windata[i].best_grade)
		grandprixinfo.cup->windata[i].best_grade = podiumData.grade;

	if (i != KARTSPEED_EASY && podiumData.rank.specialWon == true)
		grandprixinfo.cup->windata[i].got_emerald = true;

	// Save before playing the noise
	G_SaveGameData();
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
		else
		{
			if (podiumData.state < PODIUM_STATES)
			{
				podiumData.delay++;

				if (podiumData.delay > TICRATE/2)
				{
					podiumData.state++;
					podiumData.delay = 0;
				}
			}
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

	if (podiumData.ranking == false || podiumData.state < PODIUM_STATES)
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
		char gradeChar = '?';
		INT32 x = 64;
		INT32 y = 48;
		INT32 i;

		switch (podiumData.grade)
		{
			case GRADE_E: { gradeChar = 'E'; break; }
			case GRADE_D: { gradeChar = 'D'; break; }
			case GRADE_C: { gradeChar = 'C'; break; }
			case GRADE_B: { gradeChar = 'B'; break; }
			case GRADE_A: { gradeChar = 'A'; break; }
			case GRADE_S: { gradeChar = 'S'; break; }
			default: { break; }
		}

		V_DrawFadeScreen(0xFF00, podiumData.fade);

		for (i = 0; i <= podiumData.state; i++)
		{
			switch (i)
			{
				case 1:
				{
					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("POS: %d / %d", podiumData.rank.position, RANK_NEUTRAL_POSITION)
					);
					break;
				}
				case 2:
				{
					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("PTS: %d / %d", podiumData.rank.winPoints, podiumData.rank.totalPoints)
					);
					break;
				}
				case 3:
				{
					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("LAPS: %d / %d", podiumData.rank.laps, podiumData.rank.totalLaps)
					);
					break;
				}
				case 4:
				{
					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("CONTINUES: %d", podiumData.rank.continuesUsed)
					);
					break;
				}
				case 5:
				{
					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("PRISONS: %d / %d", podiumData.rank.prisons, podiumData.rank.totalPrisons)
					);
					break;
				}
				case 6:
				{
					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("RINGS: %d / %d", podiumData.rank.rings, podiumData.rank.totalRings)
					);
					break;
				}
				case 7:
				{
					const char *emeraldstr = "???";
					if (gamedata->everseenspecial == true)
					{
						emeraldstr =
							(grandprixinfo.gp == true
							&& grandprixinfo.cup != NULL
							&& grandprixinfo.cup->emeraldnum > 0)
								? "EMERALD"
								: "PRIZE";
					}

					V_DrawString(x, y, V_ALLOWLOWERCASE,
						va("%s: %s",
							emeraldstr,
							(podiumData.rank.specialWon == true) ? "YES" : "NO")
					);
					break;
				}
				case 8:
				{
					V_DrawString(x, y + 10, V_YELLOWMAP|V_ALLOWLOWERCASE,
						va(" ** FINAL GRADE: %c", gradeChar)
					);
					break;
				}
				case 9:
				{
					V_DrawThinString(2, BASEVIDHEIGHT - 10, V_SNAPTOBOTTOM|V_SNAPTOLEFT|V_6WIDTHSPACE|V_ALLOWLOWERCASE,
						"Press some button type deal to continue"
					);
					break;
				}
			}

			y += 10;
		}
	}

	if (timeinmap < 16)
	{
		// Level fade-in
		V_DrawCustomFadeScreen(((levelfadecol == 0) ? "FADEMAP1" : "FADEMAP0"), 31-(timeinmap*2));
	}
}
