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

#ifndef K_RANK_H
#define K_RANK_H

#include "doomdef.h"
#include "doomstat.h"

// Please also see P_ArchiveMisc
struct gpRank_level_perplayer_t
{
	uint8_t position;
	uint8_t rings;
	uint16_t exp;
	uint16_t prisons;
	dboolean gotSpecialPrize;
	gp_rank_e grade;
};

struct gpRank_level_t
{
	uint16_t id;
	int32_t event;
	uint32_t time;
	uint16_t totalExp;
	uint16_t totalPrisons;
	uint16_t continues;
	gpRank_level_perplayer_t perPlayer[MAXSPLITSCREENPLAYERS];
};

// Please remember to update P_ArchiveMisc
struct gpRank_t
{
	uint8_t numPlayers;
	uint8_t totalPlayers;

	uint8_t position;
	uint16_t skin;

	uint32_t winPoints;
	uint32_t totalPoints;

	uint32_t exp;
	uint32_t totalExp;

	uint32_t continuesUsed;

	uint32_t prisons;
	uint32_t totalPrisons;

	uint32_t rings;
	uint32_t totalRings;

	dboolean specialWon;

	int32_t scorePosition;
	int32_t scoreGPPoints;
	int32_t scoreExp;
	int32_t scorePrisons;
	int32_t scoreRings;
	int32_t scoreContinues;
	int32_t scoreTotal;

	uint8_t numLevels;
	gpRank_level_t levels[ROUNDQUEUE_MAX];

#ifdef __cplusplus
	void Init(void);
	void Rejigger(uint16_t removedmap, uint16_t removedgt, uint16_t addedmap, uint16_t addedgt);
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
	void K_RejiggerGPRankData(gpRank_t *rankData, uint16_t removedmap, uint16_t removedgt, uint16_t addedmap, uint16_t addedgt)

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

void K_RejiggerGPRankData(gpRank_t *rankData, uint16_t removedmap, uint16_t removedgt, uint16_t addedmap, uint16_t addedgt);


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
	uint16_t K_GetGradeColor(gp_rank_e grade)

		Maps grades to skincolors for HUD purposes.

	Input Arguments:-
		grade - gp_rank_e representing an achieved ranking.

	Return:-
		skincolor ID representing the achieved grade.
--------------------------------------------------*/

uint16_t K_GetGradeColor(gp_rank_e grade);


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
