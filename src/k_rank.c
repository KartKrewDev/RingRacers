// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_rank.c
/// \brief Grand Prix mode ranking

#include "k_rank.h"
#include "k_grandprix.h"
#include "k_specialstage.h"
#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "k_bot.h"
#include "k_kart.h"
#include "m_random.h"
#include "r_things.h"

gpRank_t g_gpRank = {0};

/*--------------------------------------------------
	void K_InitGrandPrixRank(gpRank_t *rankData)

		See header file for description.
--------------------------------------------------*/
void K_InitGrandPrixRank(gpRank_t *rankData)
{
	UINT8 numHumans = 0;
	UINT32 laps = 0;
	INT32 i;

	memset(rankData, 0, sizeof(gpRank_t));

	if (grandprixinfo.cup == NULL)
	{
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (numHumans < MAXSPLITSCREENPLAYERS && players[i].spectator == false)
			{
				numHumans++;
			}
		}
	}

	// Calculate players 
	rankData->players = numHumans;
	rankData->totalPlayers = K_GetGPPlayerCount(numHumans);

	// Initialize to the neutral value.
	rankData->position = RANK_NEUTRAL_POSITION;

	// Calculate total of points
	// (Should this account for all coop players?)
	for (i = 0; i < numHumans; i++)
	{
		rankData->totalPoints += grandprixinfo.cup->numlevels * K_CalculateGPRankPoints(i + 1, rankData->totalPlayers);
	}

	rankData->totalRings = grandprixinfo.cup->numlevels * numHumans * 20;

	if (grandprixinfo.masterbots == true)
	{
		rankData->difficultyTarget = MAXBOTDIFFICULTY;
	}
	else
	{
		rankData->difficultyTarget = min(
			MAXBOTDIFFICULTY,
			K_BotStartingDifficulty(grandprixinfo.gamespeed) + ((grandprixinfo.cup->numlevels + 1) / 2)
		);
	}

	for (i = 0; i < grandprixinfo.cup->numlevels; i++)
	{
		const INT32 cupLevelNum = grandprixinfo.cup->cachedlevels[i];
		if (cupLevelNum < nummapheaders && mapheaderinfo[cupLevelNum] != NULL)
		{
			laps += mapheaderinfo[cupLevelNum]->numlaps;
		}
	}

	// +1, since 1st place laps are worth 2 pts.
	for (i = 0; i < numHumans+1; i++)
	{
		rankData->totalLaps += laps;
	}

	// Total capsules will need to be calculated as you enter the bonus stages...
}

/*--------------------------------------------------
	gp_rank_e K_CalculateGPGrade(gpRank_t *rankData)

		See header file for description.
--------------------------------------------------*/
gp_rank_e K_CalculateGPGrade(gpRank_t *rankData)
{
	static const fixed_t gradePercents[GRADE_A] = {
		 7*FRACUNIT/20,		// D: 35% or higher
		10*FRACUNIT/20,		// C: 50% or higher
		14*FRACUNIT/20,		// B: 70% or higher
		17*FRACUNIT/20		// A: 85% or higher
	};

	gp_rank_e retGrade = GRADE_E;

	const INT32 positionWeight = 1500;
	const INT32 pointsWeight = 1000;
	const INT32 lapsWeight = 1000;
	const INT32 capsulesWeight = 1000;
	const INT32 ringsWeight = 500;
	const INT32 difficultyWeight = 200;
	const INT32 total = positionWeight + pointsWeight + lapsWeight + capsulesWeight + ringsWeight + difficultyWeight;
	const INT32 continuesPenalty = 200;

	INT32 ours = 0;
	fixed_t percent = 0;

	if (rankData->position > 0)
	{
		const INT32 sc = (rankData->position - 1);
		const INT32 loser = (RANK_NEUTRAL_POSITION - 1);
		ours += ((loser - sc) * positionWeight) / loser;
	}

	if (rankData->totalPoints > 0)
	{
		ours += (rankData->winPoints * pointsWeight) / rankData->totalPoints;
	}

	if (rankData->totalLaps > 0)
	{
		ours += (rankData->laps * lapsWeight) / rankData->totalLaps;
	}

	if (rankData->totalCapsules > 0)
	{
		ours += (rankData->capsules * capsulesWeight) / rankData->totalCapsules;
	}

	if (rankData->totalRings > 0)
	{
		ours += (rankData->rings * ringsWeight) / rankData->totalRings;
	}

	if (rankData->difficultyTarget > 0)
	{
		ours += (rankData->difficulty * difficultyWeight) / rankData->difficultyTarget;
	}

	ours -= rankData->continuesUsed * continuesPenalty;

	percent = FixedDiv(ours, total);

	for (retGrade = 0; retGrade < GRADE_A; retGrade++)
	{
		if (percent < gradePercents[retGrade])
		{
			break;
		}
	}

	if (rankData->specialWon == true)
	{
		// Winning the Special Stage gives you
		// a free grade increase.
		retGrade++;
	}

	return retGrade;
}
