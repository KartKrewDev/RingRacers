// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_race.h
/// \brief Race Mode specific code.

#ifndef __K_RACE__
#define __K_RACE__

#include "r_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern line_t *finishBeamLine;

#define FINISHLINEBEAM_SPACING (48*mapobjectscale)

/*--------------------------------------------------
	void K_ClearFinishBeamLine(void);

		Clears variables for finishBeamLine.
		Separate from K_FreeFinishBeamLine since this
		needs called when PU_LEVEL is freed.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/

void K_ClearFinishBeamLine(void);


/*--------------------------------------------------
	boolean K_GenerateFinishBeamLine(void);

		Finds pre-placed "beam points" to create a finish line out of,
		or tries to automatically create it from a finish linedef in the map.
		The result is stored in the "finishBeamLine" variable.

	Input Arguments:-
		None

	Return:-
		True if successful, otherwise false.
--------------------------------------------------*/

boolean K_GenerateFinishBeamLine(void);


/*--------------------------------------------------
	void K_RunFinishLineBeam(void);

		Updates the finish line beam effect.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/

void K_RunFinishLineBeam(void);


/*--------------------------------------------------
	UINT8 K_RaceLapCount(INT16 mapNum);

		Returns the effective final lap count of the race.

	Input Arguments:-
		mapNum - The level to count laps for, 0-indexed.

	Return:-
		The lap count to finish.
--------------------------------------------------*/

UINT8 K_RaceLapCount(INT16 mapNum);

void K_SpawnFinishEXP(player_t *player, UINT16 exp);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
