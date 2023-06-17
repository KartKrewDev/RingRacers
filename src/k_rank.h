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

// Please also see P_ArchiveMisc
struct gpRank_t
{
	UINT8 players;
	UINT8 totalPlayers;

	UINT8 position;
	UINT8 skin;

	UINT32 winPoints;
	UINT32 totalPoints;

	UINT32 laps;
	UINT32 totalLaps;

	UINT32 continuesUsed;

	UINT32 prisons;
	UINT32 totalPrisons;

	UINT32 rings;
	UINT32 totalRings;

	boolean specialWon;
};

// gp_rank_e was once defined here, but moved to doomstat.h to prevent circular dependency

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
	void K_UpdateGPRank(void)

		Updates the best ranking across all human
		players.

	Input Arguments:-
		N/A

	Return:-
		N/A
--------------------------------------------------*/
void K_UpdateGPRank(void);


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


/*--------------------------------------------------
	UINT16 K_GetGradeColor(gp_rank_e grade)

		Maps grades to skincolors for HUD purposes.

	Input Arguments:-
		grade - gp_rank_e representing an achieved ranking.

	Return:-
		skincolor ID representing the achieved grade.
--------------------------------------------------*/
UINT16 K_GetGradeColor(gp_rank_e grade);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
