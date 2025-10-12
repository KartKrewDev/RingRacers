// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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
#include "k_battle.h"
#include "k_podium.h"
#include "m_random.h"
#include "r_things.h"
#include "fastcmp.h"
#include "byteptr.h"
#include "k_race.h"
#include "command.h"
#include "k_objects.h"
#include "m_cond.h"

// I was ALMOST tempted to start tearing apart all
// of the map loading code and turning it into C++
// and making it properly split between read-only
// and true level loading and clean up all of the
// global variable garbage it uses ... but I stopped
// myself. So here's code duplication hell instead.
static UINT32 g_rankCapsules_mapthingsPos[UINT16_MAX];
static size_t g_rankCapsules_nummapthings = 0;
static boolean g_rankCapsules_udmf = false;
static UINT32 g_rankCapsules_count = 0;

/*--------------------------------------------------
	static void RankCapsules_TextmapCount(size_t size)

		Counts the number of map things and records
		the structure positions, for the result of
		RankCapsules_CountFromMap.

	Input Arguments:-
		size - Length of the TEXTMAP lump.

	Return:-
		N/A
--------------------------------------------------*/
static UINT32 RankCapsules_TextmapCount(size_t size)
{
	const char *tkn = M_TokenizerRead(0);
	UINT8 brackets = 0;

	g_rankCapsules_nummapthings = 0;

	// Look for namespace at the beginning.
	if (!fastcmp(tkn, "namespace"))
	{
		return false;
	}

	// Check if namespace is valid.
	tkn = M_TokenizerRead(0);

	while ((tkn = M_TokenizerRead(0)) && M_TokenizerGetEndPos() < size)
	{
		// Avoid anything inside bracketed stuff, only look for external keywords.
		if (brackets)
		{
			if (fastcmp(tkn, "}"))
				brackets--;
		}
		else if (fastcmp(tkn, "{"))
			brackets++;
		// Check for valid fields.
		else if (fastcmp(tkn, "thing"))
			g_rankCapsules_mapthingsPos[g_rankCapsules_nummapthings++] = M_TokenizerGetEndPos();
	}

	if (brackets)
	{
		return false;
	}

	return true;
}

/*--------------------------------------------------
	static void RankCapsules_LoadTextmap(void)

		Loads UDMF map data for the result of
		RankCapsules_CountFromMap.
--------------------------------------------------*/
static void RankCapsules_LoadTextmap(void)
{
	size_t i;

	for (i = 0; i < g_rankCapsules_nummapthings; i++)
	{
		const char *param, *val;

		M_TokenizerSetEndPos(g_rankCapsules_mapthingsPos[i]);
		param = M_TokenizerRead(0);

		if (!fastcmp(param, "{"))
		{
			continue;
		}

		while (true)
		{
			param = M_TokenizerRead(0);

			if (fastcmp(param, "}"))
			{
				break;
			}

			val = M_TokenizerRead(1);

			if (fastcmp(param, "type"))
			{
				UINT16 type = atol(val);

				if (type == mobjinfo[MT_BATTLECAPSULE].doomednum
					|| type == mobjinfo[MT_CDUFO].doomednum)
				{
					g_rankCapsules_count++;
				}

				break;
			}
		}
	}
}

#if 0
/*--------------------------------------------------
	static void RankCapsules_LoadThingsLump(UINT8 *data)

		Loads binary map data for the result of
		RankCapsules_CountFromMap.

	Input Arguments:-
		data - Pointer to a THINGS lump.

	Return:-
		N/A
--------------------------------------------------*/
static void RankCapsules_LoadThingsLump(UINT8 *data)
{
	size_t i;

	for (i = 0; i < g_rankCapsules_nummapthings; i++)
	{
		UINT16 type = 0;

		data += 2; // x
		data += 2; // y

		data += 2; // angle
		type = READUINT16(data); // type
		type &= 4095;

		data += 2; // options

		if (type == mobjinfo[MT_BATTLECAPSULE].doomednum
			|| type == mobjinfo[MT_CDUFO].doomednum)
		{
			g_rankCapsules_count++;
		}
	}
}
#endif

/*--------------------------------------------------
	static boolean RankCapsules_LoadMapData(const virtres_t *virt)

		Loads either UDMF or binary map data, for the
		result of RankCapsules_CountFromMap.

	Input Arguments:-
		virt - Pointer to the map's virtual resource.

	Return:-
		true if we could successfully load the map data,
		otherwise false.
--------------------------------------------------*/
static boolean RankCapsules_LoadMapData(const virtres_t *virt)
{
	virtlump_t *virtthings = NULL;

	// Count map data.
	if (g_rankCapsules_udmf) // Count how many entries for each type we got in textmap.
	{
		virtlump_t *textmap = vres_Find(virt, "TEXTMAP");
		M_TokenizerOpen((char *)textmap->data, textmap->size);
		if (!RankCapsules_TextmapCount(textmap->size))
		{
			M_TokenizerClose();
			return false;
		}
	}
	else
	{
		virtthings = vres_Find(virt, "THINGS");

		if (!virtthings)
		{
			return false;
		}

		// Traditional doom map format just assumes the number of elements from the lump sizes.
		g_rankCapsules_nummapthings = virtthings->size / (5 * sizeof (INT16));
	}

	// Load map data.
	if (g_rankCapsules_udmf)
	{
		RankCapsules_LoadTextmap();
		M_TokenizerClose();
	}
	else
	{
#if 0
		RankCapsules_LoadThingsLump(virtthings->data);
#else
		CONS_Printf("binary maps SMELL!!!!!\n");
#endif
	}

	return true;
}

/*--------------------------------------------------
	static UINT32 RankCapsules_CountFromMap(INT32 cuplevelnum)

		Counts the number of capsules in a map, without
		needing to fully load it.

	Input Arguments:-
		cuplevelnum - Map ID to identify Prison Eggs in

	Return:-
		Number of MT_BATTLECAPSULE instances found.
--------------------------------------------------*/
static UINT32 RankCapsules_CountFromMap(const INT32 cupLevelNum)
{
	lumpnum_t lp = mapheaderinfo[cupLevelNum]->lumpnum;
	virtres_t *virt = NULL;

	if (lp == LUMPERROR)
	{
		return 0;
	}

	virt = vres_GetMap(lp);
	if (virt == NULL)
	{
		return 0;
	}

	virtlump_t *textmap = vres_Find(virt, "TEXTMAP");

	g_rankCapsules_udmf = (textmap != NULL);
	g_rankCapsules_count = 0;

	if (RankCapsules_LoadMapData(virt) == false)
	{
		g_rankCapsules_count = 0;
	}

	vres_Free(virt);

	return g_rankCapsules_count;
}

/*--------------------------------------------------
	void K_InitGrandPrixRank(gpRank_t *rankData)

		See header file for description.
--------------------------------------------------*/
void gpRank_t::Init(void)
{
	UINT8 numHumans = 0;
	UINT32 exp = 0;
	INT32 i;

	memset(this, 0, sizeof(gpRank_t));
	skin = MAXSKINS;

	if (grandprixinfo.cup == nullptr)
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
	numPlayers = numHumans;
	totalPlayers = K_GetGPPlayerCount(numHumans);

	// Initialize to the neutral value.
	position = RANK_NEUTRAL_POSITION;

	// Calculate total of points
	// (Should this account for all coop players?)
	for (i = 0; i < numHumans; i++)
	{
		totalPoints += grandprixinfo.cup->numlevels * K_CalculateGPRankPoints(EXP_MAX, i+1, totalPlayers);
	}

	totalRings = grandprixinfo.cup->numlevels * numHumans * 20;

	for (i = 0; i < grandprixinfo.cup->numlevels; i++)
	{
		const INT32 cupLevelNum = grandprixinfo.cup->cachedlevels[i];
		if (cupLevelNum < nummapheaders && mapheaderinfo[cupLevelNum] != NULL)
		{
			exp += EXP_TARGET;
		}
	}

	for (i = 0; i < numHumans; i++)
	{
		totalExp += exp;
	}

	// Search through all of the cup's bonus levels
	// for an accurate count of how many capsules they have.
	for (i = 0; i < grandprixinfo.cup->numbonus; i++)
	{
		const INT32 cupLevelNum = grandprixinfo.cup->cachedlevels[CUPCACHE_BONUS + i];
		if (cupLevelNum < nummapheaders && mapheaderinfo[cupLevelNum] != NULL)
		{
			totalPrisons += RankCapsules_CountFromMap(cupLevelNum);
		}
	}
}

void K_InitGrandPrixRank(gpRank_t *rankData)
{
	rankData->Init();
}

/*--------------------------------------------------
	void K_RejiggerGPRankData(gpRank_t *rankData, UINT16 removedmap, UINT16 removedgt, UINT16 addedmap, UINT16 addedgt)

		See header file for description.
--------------------------------------------------*/
void gpRank_t::Rejigger(UINT16 removedmap, UINT16 removedgt, UINT16 addedmap, UINT16 addedgt)
{
	INT32 i;
	UINT32 deltaPoints = 0;

	if ((removedgt == GT_RACE) != (addedgt == GT_RACE))
	{
		for (i = 0; i < numPlayers; i++)
		{
			deltaPoints += K_CalculateGPRankPoints(EXP_MAX, i + 1, totalPlayers);
		}
		if (addedgt == GT_RACE)
			totalPoints += deltaPoints;
		else if (totalPoints > deltaPoints)
			totalPoints -= deltaPoints;
		else
			totalPoints = 0;
	}

	INT32 deltaExp = 0;
	INT32 deltaPrisons = 0;
	INT32 deltaRings = 0;

	if (removedmap < nummapheaders && mapheaderinfo[removedmap] != NULL
	&& removedgt < numgametypes && gametypes[removedgt])
	{
		if (removedgt == GT_RACE)
		{
			deltaExp -= EXP_TARGET;
		}
		if ((gametypes[removedgt]->rules & GTR_SPHERES) == 0)
		{
			deltaRings -= 20 * numPlayers;
		}
		if (gametypes[removedgt]->rules & GTR_PRISONS)
		{
			deltaPrisons -= RankCapsules_CountFromMap(removedmap);
		}
	}

	if (addedmap < nummapheaders && mapheaderinfo[addedmap] != NULL
	&& addedgt < numgametypes && gametypes[addedgt])
	{
		if (addedgt == GT_RACE)
		{
			deltaExp += EXP_TARGET;
		}
		if ((gametypes[addedgt]->rules & GTR_SPHERES) == 0)
		{
			deltaRings += 20 * numPlayers;
		}
		if (gametypes[addedgt]->rules & GTR_PRISONS)
		{
			deltaPrisons += RankCapsules_CountFromMap(addedmap);
		}
	}

	if (deltaExp)
	{
		deltaExp += totalExp;
		if (deltaExp > 0)
			totalExp = deltaExp;
		else
			totalExp = 0;
	}

	if (deltaPrisons)
	{
		deltaPrisons += totalPrisons;
		if (deltaPrisons > 0)
			totalPrisons = deltaPrisons;
		else
			totalPrisons = 0;
	}

	if (deltaRings)
	{
		deltaRings += totalRings;
		if (totalRings > 0)
			totalRings = deltaRings;
		else
			totalRings = 0;
	}
}

void K_RejiggerGPRankData(gpRank_t *rankData, UINT16 removedmap, UINT16 removedgt, UINT16 addedmap, UINT16 addedgt)
{
	rankData->Rejigger(removedmap, removedgt, addedmap, addedgt);
}

/*--------------------------------------------------
	void K_UpdateGPRank(gpRank_t *rankData)

		See header file for description.
--------------------------------------------------*/
void gpRank_t::Update(void)
{
	if (nextmapoverride != 0)
	{
		// This level does not matter if the roundqueue entry will be overridden
		return;
	}

	if (numLevels >= ROUNDQUEUE_MAX)
	{
		CONS_Alert(CONS_ERROR, "gpRank_t::Update(): Too many courses recorded in rank, discarding this round");
		return;
	}

	gpRank_level_t *const lvl = &levels[numLevels];

	prisons += numtargets;

	position = MAXPLAYERS;
	skin = MAXSKINS;

	lvl->id = gamemap;

	if (grandprixinfo.gp == true)
	{
		lvl->event = grandprixinfo.eventmode;
	}
	else
	{
		lvl->event = (gametype != GT_RACE) ? GPEVENT_BONUS : GPEVENT_NONE;
	}

	lvl->time = UINT32_MAX;

	lvl->totalExp = EXP_TARGET;
	lvl->totalPrisons = maptargets;

	UINT8 i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false
			|| players[i].spectator == true
			|| players[i].bot == true)
		{
			continue;
		}

		player_t *const player = &players[i];

		UINT8 podiumPosition = K_GetPodiumPosition(player);
		if (podiumPosition < position) // port priority
		{
			position = podiumPosition;
			skin = player->skin;
		}
	}

	for (i = 0; i < numPlayers; i++)
	{
		if (playeringame[g_localplayers[i]] == false
			|| players[g_localplayers[i]].spectator == true
			|| players[g_localplayers[i]].bot == true)
		{
			continue;
		}

		const player_t *player = &players[g_localplayers[i]];
		gpRank_level_perplayer_t *const dta = &lvl->perPlayer[i];

		if (player->realtime < lvl->time)
		{
			lvl->time = player->realtime;
		}

		dta->position = player->tally.position;
		dta->rings = player->tally.rings;
		dta->exp = player->tally.exp;
		dta->prisons = player->tally.prisons;
		dta->gotSpecialPrize = !!!(player->pflags & PF_NOCONTEST);
		dta->grade = static_cast<gp_rank_e>(player->tally.rank);
	}

	numLevels++;
}

void K_UpdateGPRank(gpRank_t *rankData)
{
	rankData->Update();
}

gp_rank_e K_CalculateGPGrade(gpRank_t *rankData)
{
	INT32 retGrade = GRADE_E;

	{
		extern consvar_t cv_debugrank;

		if (cv_debugrank.value >= 2)
		{
			return static_cast<gp_rank_e>(GRADE_E + (cv_debugrank.value - 2));
		}
	}


	fixed_t percent = K_CalculateGPPercent(rankData);

	static const fixed_t gradePercents[GRADE_A] = {
		 7*FRACUNIT/20,		// D: 35% or higher
		10*FRACUNIT/20,		// C: 50% or higher
		14*FRACUNIT/20,		// B: 70% or higher
		17*FRACUNIT/20		// A: 85% or higher
	};

	const fixed_t upgraderequirement = 370*FRACUNIT/400;

	// If our last map was Special, check for "uncredited" continues to offset the rank bump.
	fixed_t hiddenpercent = percent;
	gpRank_level_t *lastgrade = &rankData->levels[rankData->numLevels - 1];

	if (rankData->specialWon)
	{
		hiddenpercent -= FRACUNIT / RANK_CONTINUE_PENALTY_DIV * lastgrade->continues;
	}

	for (retGrade = GRADE_E; retGrade < GRADE_A; retGrade++)
	{
		if (percent < gradePercents[retGrade])
		{
			break;
		}
	}

	if (rankData->specialWon == true && hiddenpercent >= upgraderequirement)
	{
		// Winning the Special Stage gives you
		// a free grade increase.
		retGrade++;
	}

	return static_cast<gp_rank_e>(retGrade);
}

fixed_t K_SealedStarEntryRequirement(gpRank_t *rankData) // Sealed Star entry is hard at first and then gets easy, avoid newbie accidents
{
	fixed_t entry = 370*FRACUNIT/400;

	if (gamedata->everseenspecial)
		entry -= 20*FRACUNIT/400; // Goes down to 350 for good once seen

	if (grandprixinfo.masterbots && grandprixinfo.rank.position <= 1) // Master Mode Sealed Star entry only requires 1st place
		entry = K_CalculateGPPercent(rankData);

	return entry;
}

/*--------------------------------------------------
	gp_rank_e K_CalculateGPGrade(gpRank_t *rankData)

		See header file for description.
--------------------------------------------------*/
fixed_t K_CalculateGPPercent(gpRank_t *rankData)
{
	rankData->scorePosition = 0;
	rankData->scoreGPPoints = 0;
	rankData->scoreExp = 0;
	rankData->scorePrisons = 0;
	rankData->scoreRings = 0;
	rankData->scoreContinues = 0;
	rankData->scoreTotal = 0;

	const INT32 expWeight = (rankData->totalExp > 0) ? RANK_WEIGHT_EXP : 0;
	const INT32 prisonsWeight = (rankData->totalPrisons > 0) ? RANK_WEIGHT_PRISONS : 0;

	const INT32 total = RANK_WEIGHT_POSITION + expWeight + prisonsWeight + RANK_WEIGHT_RINGS;
	const INT32 continuesPenalty = total / RANK_CONTINUE_PENALTY_DIV;

	if (rankData->position > 0)
	{
		const INT32 sc = (rankData->position - 1);
		const INT32 loser = (RANK_NEUTRAL_POSITION - 1);
		rankData->scorePosition += ((loser - sc) * RANK_WEIGHT_POSITION) / loser;
	}

	if (rankData->totalPoints > 0)
	{
		rankData->scoreGPPoints += (rankData->winPoints * RANK_WEIGHT_SCORE) / rankData->totalPoints;
	}

	if (rankData->totalExp > 0)
	{
		rankData->scoreExp += (std::min(rankData->exp, rankData->totalExp) * expWeight) / rankData->totalExp;
	}

	if (rankData->totalPrisons > 0)
	{
		rankData->scorePrisons += (rankData->prisons * prisonsWeight) / rankData->totalPrisons;
	}

	if (rankData->totalRings > 0)
	{
		rankData->scoreRings += (rankData->rings * RANK_WEIGHT_RINGS) / rankData->totalRings;
	}

	rankData->scoreContinues -= (rankData->continuesUsed - RANK_CONTINUE_PENALTY_START) * continuesPenalty;

	rankData->scoreTotal =
		rankData->scorePosition +
		// rankData->scoreGPPoints +
		rankData->scoreExp +
		rankData->scorePrisons +
		rankData->scoreRings +
		rankData->scoreContinues;

	if (rankData->scoreTotal < 0)
		rankData->scoreTotal = 0;

	const fixed_t percent = FixedDiv(rankData->scoreTotal, total);

	return percent;
}

/*--------------------------------------------------
	UINT16 K_GetGradeColor(gp_rank_e grade)

		See header file for description.
--------------------------------------------------*/
UINT16 K_GetGradeColor(gp_rank_e grade)
{
	switch (grade)
	{
		case GRADE_E:
			return SKINCOLOR_BLUE;
		case GRADE_D:
			return SKINCOLOR_TURTLE;
		case GRADE_C:
			return SKINCOLOR_ORANGE;
		case GRADE_B:
			return SKINCOLOR_RED;
		case GRADE_A:
			return SKINCOLOR_MAGENTA;
		case GRADE_S:
			return SKINCOLOR_PIGEON;
		default:
			break;
	}

	return SKINCOLOR_NONE;
}

/*--------------------------------------------------
	char K_GetGradeChar(gp_rank_e grade)

		See header file for description.
--------------------------------------------------*/
char K_GetGradeChar(gp_rank_e grade)
{
	switch (grade)
	{
		case GRADE_E:
			return 'E';
		case GRADE_D:
			return 'D';
		case GRADE_C:
			return 'C';
		case GRADE_B:
			return 'B';
		case GRADE_A:
			return 'A';
		case GRADE_S:
			return 'S';
		default:
			break;
	}

	return '?';
}

