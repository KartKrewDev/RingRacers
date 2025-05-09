// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_podium.h
/// \brief Grand Prix podium cutscene

#ifndef __K_PODIUM__
#define __K_PODIUM__

#include "doomtype.h"
#include "doomstat.h" // gp_rank_e
#include "d_event.h"
#include "p_mobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------
	boolean K_PodiumSequence(void);

		Returns whenver or not we are in the podium
		cutscene mode.

	Input Arguments:-
		N/A

	Return:-
		true if we're in GS_CEREMONY, otherwise false.
--------------------------------------------------*/

boolean K_PodiumSequence(void);


/*--------------------------------------------------
	boolean K_PodiumRanking(void);

		Returns whenver or not we are in the podium
		final state.

	Input Arguments:-
		N/A

	Return:-
		true if we're in GS_CEREMONY, otherwise false.
--------------------------------------------------*/

boolean K_PodiumRanking(void);


/*--------------------------------------------------
	boolean K_PodiumGrade(void)

		Returns the podium grade.

	Input Arguments:-
		N/A

	Return:-
		gp_rank_e constant if we're in GS_CEREMONY, otherwise 0.
--------------------------------------------------*/

gp_rank_e K_PodiumGrade(void);


/*--------------------------------------------------
	boolean K_PodiumHasEmerald(void)

		Returns whether the Emerald or Prize was collected.

	Input Arguments:-
		N/A

	Return:-
		true if the Emerald/Prize was collected during the GP, otherwise false.
--------------------------------------------------*/

boolean K_PodiumHasEmerald(void);


/*--------------------------------------------------
	UINT8 K_GetPodiumPosition(player_t *player);

		Calculates what the player's position would
		be at the final standings.

	Input Arguments:-
		player - The player to do the calculation for.

	Return:-
		The player's final position, as a number
		between 1 and MAXPLAYERS.
--------------------------------------------------*/

UINT8 K_GetPodiumPosition(player_t *player);


/*--------------------------------------------------
	void K_InitializePodiumWaypoint(player_t *const player);

		Sets a bot's current waypoint to one matching
		their final podium position.

	Input Arguments:-
		player - The podium bot to update.

	Return:-
		N/A
--------------------------------------------------*/

void K_InitializePodiumWaypoint(player_t *const player);


/*--------------------------------------------------
	void K_UpdatePodiumWaypoints(player_t *const player);

		Helps a bot move along a predetermined path by
		updating their current and next waypoints as
		they move. Intended for the podium sequence.

	Input Arguments:-
		player - The podium bot to update.

	Return:-
		N/A
--------------------------------------------------*/

void K_UpdatePodiumWaypoints(player_t *const player);


/*--------------------------------------------------
	boolean K_StartCeremony(void);

		Loads the podium map and changes the gamestate
		to the podium cutscene mode.

	Input Arguments:-
		N/A

	Return:-
		true if successful, otherwise false. Can fail
		if there is no podium map defined.
--------------------------------------------------*/

boolean K_StartCeremony(void);


/*--------------------------------------------------
	void K_FinishCeremony(void);

		Called at the end of the podium cutscene,
		displays the ranking screen and starts
		accepting input.
--------------------------------------------------*/

void K_FinishCeremony(void);


/*--------------------------------------------------
	void K_ResetCeremony(void);

		Called on level load, to reset all of the
		podium variables.
--------------------------------------------------*/

void K_ResetCeremony(void);


/*--------------------------------------------------
	void K_CeremonyTicker(boolean run);

		Ticker function to be ran during the podium
		cutscene mode gamestate. Handles updating
		the camera.

	Input Arguments:-
		run - Set to true when we're running a
			new game frame.

	Return:-
		N/A
--------------------------------------------------*/

void K_CeremonyTicker(boolean run);


/*--------------------------------------------------
	void K_CeremonyDrawer(void);

		Handles the ranking screen and other HUD for
		the podium cutscene.
--------------------------------------------------*/

void K_CeremonyDrawer(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_PODIUM__
