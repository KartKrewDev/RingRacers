// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_respawn.h
/// \brief Respawning logic

#ifndef __K_RESPAWN__
#define __K_RESPAWN__

#include "k_waypoint.h"
#include "d_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RESPAWN_DIST 1024
#define RESPAWN_TIME 48
#define RESPAWNST_NONE 0x00
#define RESPAWNST_MOVE 0x01
#define RESPAWNST_DROP 0x02

/*--------------------------------------------------
	fixed_t K_RespawnOffset(player_t *player, boolean flip);

		Updates the player's flip flags, and returns a
		Z offset for respawning.

	Input Arguments:-
		player - Player to preform this for.
		flip - false for normal, true for gravity flip.

	Return:-
		Z position offset.
--------------------------------------------------*/

fixed_t K_RespawnOffset(player_t *player, boolean flip);

/*--------------------------------------------------
	void K_RespawnAtWaypoint(player_t *player, waypoint_t *waypoint)

		Updates a player's respawn variables to go to the provided waypoint.

	Input Arguments:-
		player - Player to preform for.
		waypoint - Waypoint to respawn to.

	Return:-
		None
--------------------------------------------------*/
void K_RespawnAtWaypoint(player_t *player, waypoint_t *waypoint);

/*--------------------------------------------------
	void K_DoFault(player_t *player);

		Faults the specified player.

	Input Arguments:-
		player - Player to preform this for.

	Return:-
		None
--------------------------------------------------*/

void K_DoFault(player_t *player);


/*--------------------------------------------------
	void K_DoIngameRespawn(player_t *player);

		Starts the respawning animation for the specified player,
		updating their respawn variables in preparation.

	Input Arguments:-
		player - Player to preform this for.

	Return:-
		None
--------------------------------------------------*/

void K_DoIngameRespawn(player_t *player);


/*--------------------------------------------------
	size_t K_NextRespawnWaypointIndex(waypoint_t *waypoint);

		Returns the index for the next respawn waypoint.

	Input Arguments:-
		waypoint - Waypoint to look past.

	Return:-
		An table index for waypoint_t -> nextwaypoints.
--------------------------------------------------*/

size_t K_NextRespawnWaypointIndex(waypoint_t *waypoint);


/*--------------------------------------------------
	void K_RespawnChecker(player_t *player);

		Thinker for the respawning animation.

	Input Arguments:-
		player - Player to preform this for.

	Return:-
		None
--------------------------------------------------*/

void K_RespawnChecker(player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
