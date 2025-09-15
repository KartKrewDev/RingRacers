// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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

// Please also see P_ArchiveMisc
struct gpRank_level_perplayer_t
{
	UINT8 position;
	UINT8 rings;
	UINT16 exp;
	UINT16 prisons;
	boolean gotSpecialPrize;
	gp_rank_e grade;
};

struct gpRank_level_t
{
	UINT16 id;
	INT32 event;
	UINT32 time;
	UINT16 totalExp;
	UINT16 totalPrisons;
	UINT16 continues;
	gpRank_level_perplayer_t perPlayer[MAXSPLITSCREENPLAYERS];
};

// Please remember to update P_ArchiveMisc
struct gpRank_t
{
	UINT8 numPlayers;
	UINT8 totalPlayers;

	UINT8 position;
	UINT16 skin;

	UINT32 winPoints;
	UINT32 totalPoints;

	UINT32 exp;
	UINT32 totalExp;

	UINT32 continuesUsed;

	UINT32 prisons;
	UINT32 totalPrisons;

	UINT32 rings;
	UINT32 totalRings;

	boolean specialWon;

	INT32 scorePosition;
	INT32 scoreGPPoints;
	INT32 scoreExp;
	INT32 scorePrisons;
	INT32 scoreRings;
	INT32 scoreContinues;
	INT32 scoreTotal;

	UINT8 numLevels;
	gpRank_level_t levels[ROUNDQUEUE_MAX];

#ifdef __cplusplus
	void Init(void);
	void Rejigger(UINT16 removedmap, UINT16 removedgt, UINT16 addedmap, UINT16 addedgt);
	void Update(void);
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

// gp_rank_e was once defined here, but moved to doomstat.h to prevent circular dependency

// 3rd place is neutral, anything below is a penalty
#define RANK_NEUTRAL_POSITION (3)

#define RANK_WEIGHT_POSITION (150)
#define RANK_WEIGHT_SCORE (100)
#define RANK_WEIGHT_EXP (100)
#define RANK_WEIGHT_PRISONS (100)
#define RANK_WEIGHT_RINGS (50)

#define RANK_CONTINUE_PENALTY_DIV (10) // 10% of the total grade
#define RANK_CONTINUE_PENALTY_START (0)

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
	void K_RejiggerGPRankData(gpRank_t *rankData, UINT16 removedmap, UINT16 removedgt, UINT16 addedmap, UINT16 addedgt)

		Recalculates rank requirements for overriden round.

	Input Arguments:-
		rankData - Pointer to struct that contains all
			of the information required to calculate GP rank.
		removedmap - Level ID for round extracted
		removedgt - Gametype ID for round extracted
		addedmap - Level ID for round extracted
		addedgt - Gametype ID for round extracted

	Return:-
		N/A
--------------------------------------------------*/

void K_RejiggerGPRankData(gpRank_t *rankData, UINT16 removedmap, UINT16 removedgt, UINT16 addedmap, UINT16 addedgt);


/*--------------------------------------------------
	void K_UpdateGPRank(gpRank_t *rankData)

		Updates the best ranking across all human
		players.
--------------------------------------------------*/

void K_UpdateGPRank(gpRank_t *rankData);


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
fixed_t K_SealedStarEntryRequirement(gpRank_t *rankData);
fixed_t K_CalculateGPPercent(gpRank_t *rankData);


/*--------------------------------------------------
	UINT16 K_GetGradeColor(gp_rank_e grade)

		Maps grades to skincolors for HUD purposes.

	Input Arguments:-
		grade - gp_rank_e representing an achieved ranking.

	Return:-
		skincolor ID representing the achieved grade.
--------------------------------------------------*/

UINT16 K_GetGradeColor(gp_rank_e grade);


/*--------------------------------------------------
	char K_GetGradeChar(gp_rank_e grade)

		Maps grades to a letter for strings.

	Input Arguments:-
		grade - gp_rank_e representing an achieved ranking.

	Return:-
		ASCII character for the grade.
--------------------------------------------------*/

char K_GetGradeChar(gp_rank_e grade);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
