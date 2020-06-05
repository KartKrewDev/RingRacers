// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
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

#define RESPAWN_DIST 1024
#define RESPAWN_TIME 48
#define RESPAWNST_NONE 0
#define RESPAWNST_MOVE 1
#define RESPAWNST_DROP 2

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
	void K_RespawnChecker(player_t *player);

		Thinker for the respawning animation.

	Input Arguments:-
		player - Player to preform this for.

	Return:-
		None
--------------------------------------------------*/

void K_RespawnChecker(player_t *player);

#endif
