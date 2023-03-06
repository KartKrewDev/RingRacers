// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_rank.h
/// \brief Grand Prix mode ranking

#ifndef __K_RANK__
#define __K_RANK__

#include "doomdef.h"
#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gpRank_t
{
	UINT8 players;
	UINT8 totalPlayers;

	UINT8 position;

	UINT32 winPoints;
	UINT32 totalPoints;

	UINT32 laps;
	UINT32 totalLaps;

	UINT32 continuesUsed;

	UINT32 capsules;
	UINT32 totalCapsules;

	UINT32 rings;
	UINT32 totalRings;

	boolean specialWon;
};

typedef enum
{
	GRADE_E,
	GRADE_D,
	GRADE_C,
	GRADE_B,
	GRADE_A,
	GRADE_S
} gp_rank_e;

// 3rd place is neutral, anything below is a penalty
#define RANK_NEUTRAL_POSITION (3)

/*--------------------------------------------------
	void K_InitGrandPrixRank(gpRank_t *rankData);

		Calculates rank requirements for a GP session.

	Input Arguments:-
		rankData - Pointer to struct that contains all
			of the information required to calculate GP rank.

	Return:-
		N/A
--------------------------------------------------*/

void K_InitGrandPrixRank(gpRank_t *rankData);


/*--------------------------------------------------
	gp_rank_e K_CalculateGPGrade(gpRank_t *rankData);

		Calculates the player's grade using the
		variables from gpRank.

	Input Arguments:-
		rankData - struct containing existing rank data.

	Return:-
		gp_rank_e representing the total grade.
--------------------------------------------------*/

gp_rank_e K_CalculateGPGrade(gpRank_t *rankData);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
