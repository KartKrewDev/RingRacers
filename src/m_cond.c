// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Vivian "toastergrl" Grannell.
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2012-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cond.c
/// \brief Unlockable condition system for SRB2 version 2.1

#include "m_cond.h"
#include "m_random.h" // M_RandomKey
#include "doomstat.h"
#include "z_zone.h"

#include "hu_stuff.h" // CEcho
#include "v_video.h" // video flags

#include "g_game.h" // record info
#include "r_skins.h" // numskins
#include "k_follower.h"
#include "r_draw.h" // R_GetColorByName
#include "s_sound.h" // S_StartSound

#include "k_kart.h" // K_IsPLayerLosing
#include "k_grandprix.h" // grandprixinfo
#include "k_battle.h" // battleprisons
#include "k_specialstage.h" // specialstageinfo
#include "k_podium.h"
#include "k_pwrlv.h"
#include "k_profiles.h"

gamedata_t *gamedata = NULL;
boolean netUnlocked[MAXUNLOCKABLES];

// The meat of this system lies in condition sets
conditionset_t conditionSets[MAXCONDITIONSETS];

// Emblem locations
emblem_t emblemlocations[MAXEMBLEMS];

// Unlockables
unlockable_t unlockables[MAXUNLOCKABLES];

// Number of emblems
INT32 numemblems = 0;

// Create a new gamedata_t, for start-up
void M_NewGameDataStruct(void)
{
	gamedata = Z_Calloc(sizeof (gamedata_t), PU_STATIC, NULL);
	M_ClearSecrets();
	G_ClearRecords();
}

void M_PopulateChallengeGrid(void)
{
	UINT16 i, j;
	UINT16 numunlocks = 0, nummajorunlocks = 0, numempty = 0;
	UINT16 selection[2][MAXUNLOCKABLES + (CHALLENGEGRIDHEIGHT-1)];
	UINT16 majorcompact = 2;

	if (gamedata->challengegrid != NULL)
	{
		// todo tweak your grid if unlocks are changed
		return;
	}

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (!unlockables[i].conditionset)
		{
			continue;
		}

		if (unlockables[i].majorunlock)
		{
			selection[1][nummajorunlocks++] = i;
			//CONS_Printf(" found %d (LARGE)\n", selection[1][nummajorunlocks-1]);
			continue;
		}

		selection[0][numunlocks++] = i;
		//CONS_Printf(" found %d\n", selection[0][numunlocks-1]);
	}

	gamedata->challengegridwidth = 0;

	if (numunlocks + nummajorunlocks == 0)
	{
		return;
	}

	if (nummajorunlocks)
	{
		// Getting the number of 2-highs you can fit into two adjacent columns.
		UINT16 majorpad = (CHALLENGEGRIDHEIGHT/2);
		numempty = nummajorunlocks%majorpad;
		majorpad = (nummajorunlocks+(majorpad-1))/majorpad;

		gamedata->challengegridwidth = majorpad*2;
		numempty *= 4;

#if (CHALLENGEGRIDHEIGHT % 2)
		// One extra empty per column.
		numempty += gamedata->challengegridwidth;
#endif

		//CONS_Printf("%d major unlocks means width of %d, numempty of %d\n", nummajorunlocks, gamedata->challengegridwidth, numempty);
	}

	if (numunlocks > numempty)
	{
		// Getting the number of extra columns to store normal unlocks
		UINT16 temp = ((numunlocks - numempty) + (CHALLENGEGRIDHEIGHT-1))/CHALLENGEGRIDHEIGHT;
		gamedata->challengegridwidth += temp;
		majorcompact = 1;
		//CONS_Printf("%d normal unlocks means %d extra entries, additional width of %d\n", numunlocks, (numunlocks - numempty), temp);
	}
	else if (challengegridloops)
	{
		// Another case where offset large tiles are permitted.
		majorcompact = 1;
	}

	gamedata->challengegrid = Z_Malloc(
		(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT * sizeof(UINT16)),
		PU_STATIC, NULL);

	if (!gamedata->challengegrid)
	{
		I_Error("M_PopulateChallengeGrid: was not able to allocate grid");
	}

	for (i = 0; i < (gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT); ++i)
	{
		gamedata->challengegrid[i] = MAXUNLOCKABLES;
	}

	// Attempt to place all large tiles first.
	if (nummajorunlocks)
	{
		// You lose one from CHALLENGEGRIDHEIGHT because it is impossible to place a 2-high tile on the bottom row.
		// You lose one from the width if it doesn't loop.
		// You divide by two if the grid is so compacted that large tiles can't be in offset columns.
		UINT16 numspots = (gamedata->challengegridwidth - (challengegridloops ? 0 : majorcompact))
				* ((CHALLENGEGRIDHEIGHT-1) / majorcompact);
		// 0 is row, 1 is column
		INT16 quickcheck[numspots][2];

		// Prepare the easy-grab spots.
		for (i = 0; i < numspots; i++)
		{
			quickcheck[i][0] = i%(CHALLENGEGRIDHEIGHT-1);
			quickcheck[i][1] = majorcompact * i/(CHALLENGEGRIDHEIGHT-1);
		}

		// Place in random valid locations.
		while (nummajorunlocks > 0 && numspots > 0)
		{
			INT16 row, col;
			j = M_RandomKey(numspots);
			row = quickcheck[j][0];
			col =  quickcheck[j][1];

			// We always take from selection[1][] in order, but the PLACEMENT is still random.
			nummajorunlocks--;

			//CONS_Printf("--- %d (LARGE) placed at (%d, %d)\n", selection[1][nummajorunlocks], row, col);

			i = row + (col * CHALLENGEGRIDHEIGHT);
			gamedata->challengegrid[i] = gamedata->challengegrid[i+1] = selection[1][nummajorunlocks];
			if (col == gamedata->challengegridwidth-1)
			{
				i = row;
			}
			else
			{
				i += CHALLENGEGRIDHEIGHT;
			}
			gamedata->challengegrid[i] = gamedata->challengegrid[i+1] = selection[1][nummajorunlocks];
	
			if (nummajorunlocks == 0)
			{
				break;
			}

			for (i = 0; i < numspots; i++)
			{
quickcheckagain:
				if (abs((quickcheck[i][0]) - (row)) <= 1 // Row distance
					&& (abs((quickcheck[i][1]) - (col)) <= 1 // Column distance
					|| (quickcheck[i][1] == 0 && col == gamedata->challengegridwidth-1) // Wraparounds l->r
					|| (quickcheck[i][1] == gamedata->challengegridwidth-1 && col == 0))) // Wraparounds r->l
				{
					numspots--;  // Remove from possible indicies
					if (i == numspots)
						break;
					// Shuffle remaining so we can keep on using M_RandomKey
					quickcheck[i][0] = quickcheck[numspots][0];
					quickcheck[i][1] = quickcheck[numspots][1];
					// Woah there - we've gotta check the one that just got put in our place.
					goto quickcheckagain;
				}
				continue;
			}
		}

#if (CHALLENGEGRIDHEIGHT == 4)
		while (nummajorunlocks > 0)
		{
			UINT16 unlocktomoveup = MAXUNLOCKABLES;

			j = gamedata->challengegridwidth-1;

			// Attempt to fix our whoopsie.
			for (i = 0; i < j; i++)
			{
				if (gamedata->challengegrid[1 + (i*CHALLENGEGRIDHEIGHT)] != MAXUNLOCKABLES
					&& gamedata->challengegrid[(i*CHALLENGEGRIDHEIGHT)] == MAXUNLOCKABLES)
					break;
			}

			if (i == j)
			{
				break;
			}

			unlocktomoveup = gamedata->challengegrid[1 + (i*CHALLENGEGRIDHEIGHT)];

			if (i == 0
				&& challengegridloops
				&& (gamedata->challengegrid [1 + (j*CHALLENGEGRIDHEIGHT)]
					== gamedata->challengegrid[1]))
				;
			else
			{
				j = i + 1;
			}

			nummajorunlocks--;

			// Push one pair up.
			gamedata->challengegrid[(i*CHALLENGEGRIDHEIGHT)] = gamedata->challengegrid[(j*CHALLENGEGRIDHEIGHT)] = unlocktomoveup;
			// Wedge the remaining four underneath.
			gamedata->challengegrid[2 + (i*CHALLENGEGRIDHEIGHT)] = gamedata->challengegrid[2 + (j*CHALLENGEGRIDHEIGHT)] = selection[1][nummajorunlocks];
			gamedata->challengegrid[3 + (i*CHALLENGEGRIDHEIGHT)] = gamedata->challengegrid[3 + (j*CHALLENGEGRIDHEIGHT)] = selection[1][nummajorunlocks];
		}
#endif

		if (nummajorunlocks > 0)
		{
			UINT16 widthtoprint = gamedata->challengegridwidth;
			Z_Free(gamedata->challengegrid);
			gamedata->challengegrid = NULL;

			I_Error("M_PopulateChallengeGrid: was not able to populate %d large tiles (width %d)", nummajorunlocks, widthtoprint);
		}
	}

	numempty = 0;
	// Space out empty entries to pepper into unlock list
	for (i = 0; i < gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT; i++)
	{
		if (gamedata->challengegrid[i] < MAXUNLOCKABLES)
		{
			continue;
		}

		numempty++;
	}

	if (numunlocks > numempty)
	{
		gamedata->challengegridwidth = 0;
		Z_Free(gamedata->challengegrid);
		gamedata->challengegrid = NULL;

		I_Error("M_PopulateChallengeGrid: %d small unlocks vs %d empty spaces (%d gap)", numunlocks, numempty, (numunlocks-numempty));
	}

	//CONS_Printf(" %d unlocks vs %d empty spaces\n", numunlocks, numempty);

	while (numunlocks < numempty)
	{
		//CONS_Printf(" adding empty)\n");
		selection[0][numunlocks++] = MAXUNLOCKABLES;
	}

	// Fill the remaining spots with random ordinary unlocks (and empties).
	for (i = 0; i < gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT; i++)
	{
		if (gamedata->challengegrid[i] < MAXUNLOCKABLES)
		{
			continue;
		}

		j = M_RandomKey(numunlocks); // Get an entry
		gamedata->challengegrid[i] = selection[0][j]; // Set that entry
		//CONS_Printf(" %d placed at (%d, %d)\n", selection[0][j], i/CHALLENGEGRIDHEIGHT, i%CHALLENGEGRIDHEIGHT);
		numunlocks--; // Remove from possible indicies
		selection[0][j] = selection[0][numunlocks]; // Shuffle remaining so we can keep on using M_RandomKey

		if (numunlocks == 0)
		{
			break;
		}
	}
}

void M_UpdateChallengeGridExtraData(challengegridextradata_t *extradata)
{
	UINT16 i, j, num, id, tempid, work;
	boolean idchange;

	if (gamedata->challengegrid == NULL)
	{
		return;
	}

	if (extradata == NULL)
	{
		return;
	}

	//CONS_Printf(" --- \n");

	// Pre-wipe flags.
	for (i = 0; i < gamedata->challengegridwidth; i++)
	{
		for (j = 0; j < CHALLENGEGRIDHEIGHT; j++)
		{
			id = (i * CHALLENGEGRIDHEIGHT) + j;
			extradata[id].flags = CHE_NONE;
		}
	}

	// Populate extra data.
	for (i = 0; i < gamedata->challengegridwidth; i++)
	{
		for (j = 0; j < CHALLENGEGRIDHEIGHT; j++)
		{
			id = (i * CHALLENGEGRIDHEIGHT) + j;
			num = gamedata->challengegrid[id];
			idchange = false;

			// Empty spots in the grid are always unconnected.
			if (num >= MAXUNLOCKABLES)
			{
				continue;
			}

			// Check the spot above.
			if (j > 0)
			{
				tempid = (i * CHALLENGEGRIDHEIGHT) + (j - 1);
				work = gamedata->challengegrid[tempid];
				if (work == num)
				{
					extradata[id].flags = CHE_CONNECTEDUP;

					// Get the id to write extra hint data to.
					// This check is safe because extradata's order of population
					if (extradata[tempid].flags & CHE_CONNECTEDLEFT)
					{
						extradata[id].flags |= CHE_CONNECTEDLEFT;
						//CONS_Printf(" %d - %d above %d is invalid, check to left\n", num, tempid, id);
						if (i > 0)
						{
							tempid -= CHALLENGEGRIDHEIGHT;
						}
						else
						{
							tempid = ((gamedata->challengegridwidth - 1) * CHALLENGEGRIDHEIGHT) + j - 1;
						}
					}
					/*else
						CONS_Printf(" %d - %d above %d is valid\n", num, tempid, id);*/

					id = tempid;
					idchange = true;

					if (extradata[id].flags == CHE_HINT)
					{
						continue;
					}
				}
				else if (work < MAXUNLOCKABLES && gamedata->unlocked[work])
				{
					extradata[id].flags = CHE_HINT;
				}
			}

			// Check the spot to the left.
			{
				if (i > 0)
				{
					tempid = ((i - 1) * CHALLENGEGRIDHEIGHT) + j;
				}
				else
				{
					tempid = ((gamedata->challengegridwidth - 1) * CHALLENGEGRIDHEIGHT) + j;
				}
				work = gamedata->challengegrid[tempid];

				if (work == num)
				{
					if (!idchange && (i > 0 || challengegridloops))
					{
						//CONS_Printf(" %d - %d to left of %d is valid\n", work, tempid, id);
						// If we haven't already updated our id, it's the one to our left.
						if (extradata[id].flags == CHE_HINT)
						{
							extradata[tempid].flags = CHE_HINT;
						}
						extradata[id].flags = CHE_CONNECTEDLEFT;
						id = tempid;
					}
					/*else
						CONS_Printf(" %d - %d to left of %d is invalid\n", work, tempid, id);*/
				}
				else if (work < MAXUNLOCKABLES && gamedata->unlocked[work])
				{
					extradata[id].flags = CHE_HINT;
					continue;
				}
			}

			// Since we're not modifying id past this point, the conditions become much simpler.
			if ((extradata[id].flags & (CHE_HINT|CHE_DONTDRAW)) == CHE_HINT)
			{
				continue;
			}

			// Check the spot below.
			if (j < CHALLENGEGRIDHEIGHT-1)
			{
				tempid = (i * CHALLENGEGRIDHEIGHT) + (j + 1);
				work = gamedata->challengegrid[tempid];

				if (work == num)
				{
					;
				}
				else if (work < MAXUNLOCKABLES && gamedata->unlocked[work])
				{
					extradata[id].flags = CHE_HINT;
					continue;
				}
			}

			// Check the spot to the right.
			{
				if (i < (gamedata->challengegridwidth - 1))
				{
					tempid = ((i + 1) * CHALLENGEGRIDHEIGHT) + j;
				}
				else
				{
					tempid = j;
				}
				work = gamedata->challengegrid[tempid];

				if (work == num)
				{
					;
				}
				else if (work < MAXUNLOCKABLES && gamedata->unlocked[work])
				{
					extradata[id].flags = CHE_HINT;
					continue;
				}
			}
		}
	}
}

void M_AddRawCondition(UINT16 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2, char *stringvar)
{
	condition_t *cond;
	UINT32 num, wnum;

	I_Assert(set < MAXCONDITIONSETS);

	wnum = conditionSets[set].numconditions;
	num = ++conditionSets[set].numconditions;

	conditionSets[set].condition = Z_Realloc(conditionSets[set].condition, sizeof(condition_t)*num, PU_STATIC, 0);

	cond = conditionSets[set].condition;

	cond[wnum].id = id;
	cond[wnum].type = c;
	cond[wnum].requirement = r;
	cond[wnum].extrainfo1 = x1;
	cond[wnum].extrainfo2 = x2;
	cond[wnum].stringvar = stringvar;
}

void M_ClearConditionSet(UINT16 set)
{
	if (conditionSets[set].numconditions)
	{
		while (conditionSets[set].numconditions > 0)
		{
			--conditionSets[set].numconditions;
			Z_Free(conditionSets[set].condition[conditionSets[set].numconditions].stringvar);
		}

		Z_Free(conditionSets[set].condition);
		conditionSets[set].condition = NULL;
	}
	gamedata->achieved[set] = false;
}

// Clear ALL secrets.
void M_ClearStats(void)
{
	UINT8 i;
	gamedata->totalplaytime = 0;
	gamedata->totalrings = 0;
	for (i = 0; i < GDGT_MAX; ++i)
		gamedata->roundsplayed[i] = 0;
	gamedata->timesBeaten = 0;

	gamedata->everloadedaddon = false;
	gamedata->eversavedreplay = false;
	gamedata->everseenspecial = false;
	gamedata->evercrashed = false;
	gamedata->musicflags = 0;

	gamedata->importprofilewins = false;
}

void M_ClearSecrets(void)
{
	INT32 i;

	for (i = 0; i < MAXEMBLEMS; ++i)
		gamedata->collected[i] = false;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
		gamedata->unlocked[i] = gamedata->unlockpending[i] = netUnlocked[i] = false;
	for (i = 0; i < MAXCONDITIONSETS; ++i)
		gamedata->achieved[i] = false;

	Z_Free(gamedata->challengegrid);
	gamedata->challengegrid = NULL;
	gamedata->challengegridwidth = 0;

	gamedata->pendingkeyrounds = 0;
	gamedata->pendingkeyroundoffset = 0;
	gamedata->keyspending = 0;
	gamedata->chaokeys = 3; // Start with 3 !!
}

// ----------------------
// Condition set checking
// ----------------------

void M_UpdateConditionSetsPending(void)
{
	UINT32 i, j;
	conditionset_t *c;
	condition_t *cn;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];
		if (!c->numconditions)
			continue;

		for (j = 0; j < c->numconditions; ++j)
		{
			cn = &c->condition[j];
			if (cn->stringvar == NULL)
				continue;

			switch (cn->type)
			{
				case UCRP_ISCHARACTER:
				{
					cn->requirement = R_SkinAvailable(cn->stringvar);

					if (cn->requirement < 0)
					{
						CONS_Alert(CONS_WARNING, "UCRP_ISCHARACTER: Invalid character %s for condition ID %d", cn->stringvar, cn->id+1);
						return;
					}

					Z_Free(cn->stringvar);
					cn->stringvar = NULL;

					break;
				}

				case UCRP_WETPLAYER:
				{
					if (cn->extrainfo1)
					{
						char *l;

						for (l = cn->stringvar; *l != '\0'; l++)
						{
							*l = tolower(*l);
						}

						cn->extrainfo1 = 0;
					}
					break;
				}

				default:
					break;
			}
		}

		
	}
}

boolean M_NotFreePlay(player_t *player)
{
	UINT8 i;

	if (K_CanChangeRules(true) == false)
	{
		// Rounds with direction are never FREE PLAY.
		return true;
	}

	if (battleprisons)
	{
		// Prison Break is battle's FREE PLAY.
		return false;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			continue;
		}

		if (player == &players[i])
		{
			continue;
		}

		return true;
	}

	return false;
}

// See also M_GetConditionString
boolean M_CheckCondition(condition_t *cn, player_t *player)
{
	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x
			return (gamedata->totalplaytime >= (unsigned)cn->requirement);
		case UC_ROUNDSPLAYED: // Requires any level completed >= x times
		{
			if (cn->extrainfo1 == GDGT_MAX)
			{
				UINT8 i;
				UINT32 sum = 0;

				for (i = 0; i < GDGT_MAX; i++)
				{
					sum += gamedata->roundsplayed[i];
				}

				return (sum >= (unsigned)cn->requirement);
			}
			return (gamedata->roundsplayed[cn->extrainfo1] >= (unsigned)cn->requirement);
		}
		case UC_TOTALRINGS: // Requires grabbing >= x rings
			return (gamedata->totalrings >= (unsigned)cn->requirement);
		case UC_GAMECLEAR: // Requires game beaten >= x times
			return (gamedata->timesBeaten >= (unsigned)cn->requirement);
		case UC_OVERALLTIME: // Requires overall time <= x
			return (M_GotLowEnoughTime(cn->requirement));
		case UC_MAPVISITED: // Requires map x to be visited
		case UC_MAPBEATEN: // Requires map x to be beaten
		case UC_MAPENCORE: // Requires map x to be beaten in encore
		case UC_MAPSPBATTACK: // Requires map x to be beaten in SPB Attack
		{
			UINT8 mvtype = MV_VISITED;
			if (cn->type == UC_MAPBEATEN)
				mvtype = MV_BEATEN;
			else if (cn->type == UC_MAPENCORE)
				mvtype = MV_ENCORE;
			else if (cn->type == UC_MAPSPBATTACK)
				mvtype = MV_SPBATTACK;

			return ((cn->requirement < nummapheaders)
				&& (mapheaderinfo[cn->requirement])
				&& ((mapheaderinfo[cn->requirement]->records.mapvisited & mvtype) == mvtype));
		}
		case UC_MAPTIME: // Requires time on map <= x
			return (G_GetBestTime(cn->extrainfo1) <= (unsigned)cn->requirement);

		case UC_ALLCHAOS:
		case UC_ALLSUPER:
		case UC_ALLEMERALDS:
		{
			cupheader_t *cup;
			UINT16 ret = 0;
			UINT8 i;

			if (gamestate == GS_LEVEL)
				return false; // this one could be laggy with many cups available

			for (cup = kartcupheaders; cup; cup = cup->next)
			{
				if (cup->emeraldnum == 0)
					continue;

				i = cn->requirement;
				for (i = cn->requirement; i < KARTGP_MAX; i++)
				{
					if (cup->windata[i].got_emerald == true)
						break;
				}

				if (i == KARTGP_MAX)
					continue;

				ret |= 1<<(cup->emeraldnum-1);
			}

			if (cn->type == UC_ALLCHAOS)
				return ALLCHAOSEMERALDS(ret);
			if (cn->type == UC_ALLSUPER)
				return ALLSUPEREMERALDS(ret);
			return ALLEMERALDS(ret);
		}

		case UC_TOTALMEDALS: // Requires number of emblems >= x
			return (M_GotEnoughMedals(cn->requirement));
		case UC_EMBLEM: // Requires emblem x to be obtained
			return gamedata->collected[cn->requirement-1];
		case UC_UNLOCKABLE: // Requires unlockable x to be obtained
			return gamedata->unlocked[cn->requirement-1];
		case UC_CONDITIONSET: // requires condition set x to already be achieved
			return M_Achieved(cn->requirement-1);

		case UC_ADDON:
			return ((gamedata->everloadedaddon == true)
				&& M_SecretUnlocked(SECRET_ADDONS, true));
		case UC_REPLAY:
			return (gamedata->eversavedreplay == true);
		case UC_CRASH:
			if (gamedata->evercrashed)
			{
				gamedata->musicflags |= GDMUSIC_LOSERCLUB;
				return true;
			}
			return false;

		// Just for string building
		case UC_AND:
		case UC_COMMA:
			return true;

		case UCRP_PREFIX_GRANDPRIX:
			return (grandprixinfo.gp == true);
		case UCRP_PREFIX_BONUSROUND:
			return ((grandprixinfo.gp == true) && (grandprixinfo.eventmode == GPEVENT_BONUS));
		case UCRP_PREFIX_TIMEATTACK:
			return (modeattacking != ATTACKING_NONE);
		case UCRP_PREFIX_PRISONBREAK:
			return ((gametyperules & GTR_PRISONS) && battleprisons);
		case UCRP_PREFIX_SEALEDSTAR:
			return (specialstageinfo.valid == true);

		case UCRP_PREFIX_ISMAP:
		case UCRP_ISMAP:
			return (gamemap == cn->requirement+1);
		case UCRP_ISCHARACTER:
			return (player->skin == cn->requirement);
		case UCRP_ISENGINECLASS:
			return (player->skin < numskins
				&& R_GetEngineClass(
					skins[player->skin].kartspeed,
					skins[player->skin].kartweight,
					skins[player->skin].flags
				) == (unsigned)cn->requirement);
		case UCRP_ISDIFFICULTY:
			if (grandprixinfo.gp == false)
				return (gamespeed >= cn->requirement);
			if (cn->requirement == KARTGP_MASTER)
				return (grandprixinfo.masterbots == true);
			return (grandprixinfo.gamespeed >= cn->requirement);

		case UCRP_PODIUMCUP:
			if (K_PodiumRanking() == false)
				return false;
			if (grandprixinfo.cup == NULL
				|| grandprixinfo.cup->id != cn->requirement)
				return false;
			if (cn->extrainfo2)
				return (K_PodiumGrade() >= (unsigned)cn->requirement);
			if (cn->extrainfo1 != 0)
				return (player->position != 0
					&& player->position <= cn->extrainfo1);
			return true;
		case UCRP_PODIUMEMERALD:
		case UCRP_PODIUMPRIZE:
			return (K_PodiumRanking() == true
				&& grandprixinfo.rank.specialWon == true);

		case UCRP_FINISHCOOL:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay(player)
				&& !K_IsPlayerLosing(player));
		case UCRP_FINISHALLPRISONS:
			return (battleprisons
				&& !(player->pflags & PF_NOCONTEST)
				//&& M_NotFreePlay(player)
				&& numtargets >= maptargets);
		case UCRP_NOCONTEST:
			return (player->pflags & PF_NOCONTEST);
		case UCRP_FINISHPLACE:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay(player)
				&& player->position != 0
				&& player->position <= cn->requirement);
		case UCRP_FINISHPLACEEXACT:
			return (player->exiting 
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay(player)
				&& player->position == cn->requirement);
		case UCRP_FINISHTIME:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				//&& M_NotFreePlay(player)
				&& player->realtime <= (unsigned)cn->requirement);
		case UCRP_FINISHTIMEEXACT:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				//&& M_NotFreePlay(player)
				&& player->realtime/TICRATE == (unsigned)cn->requirement/TICRATE);
		case UCRP_FINISHTIMELEFT:
			return (timelimitintics
				&& player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& !K_CanChangeRules(false) // too easy to change cv_timelimit
				&& player->realtime < timelimitintics
				&& (timelimitintics + extratimeintics + secretextratime - player->realtime) >= (unsigned)cn->requirement);

		case UCRP_TRIGGER: // requires map trigger set
			return !!(player->roundconditions.unlocktriggers & (1 << cn->requirement));

		case UCRP_FALLOFF:
			return (player->roundconditions.fell_off == (cn->requirement == 1));
		case UCRP_TOUCHOFFROAD:
			return (player->roundconditions.touched_offroad == (cn->requirement == 1));
		case UCRP_TOUCHSNEAKERPANEL:
			return (player->roundconditions.touched_sneakerpanel == (cn->requirement == 1));
		case UCRP_RINGDEBT:
			return (!(gametyperules & GTR_SPHERES) && (player->roundconditions.debt_rings == (cn->requirement == 1)));

		case UCRP_TRIPWIREHYUU:
			return (player->roundconditions.tripwire_hyuu);
		case UCRP_SPBNEUTER:
			return (player->roundconditions.spb_neuter);
		case UCRP_LANDMINEDUNK:
			return (player->roundconditions.landmine_dunk);
		case UCRP_HITMIDAIR:
			return (player->roundconditions.hit_midair);

		case UCRP_WETPLAYER:
			return (((player->roundconditions.wet_player & cn->requirement) == 0)
				&& !player->roundconditions.fell_off); // Levels with water tend to texture their pits as water too
	}
	return false;
}

static boolean M_CheckConditionSet(conditionset_t *c, player_t *player)
{
	UINT32 i;
	UINT32 lastID = 0;
	condition_t *cn;
	boolean achievedSoFar = true;

	for (i = 0; i < c->numconditions; ++i)
	{
		cn = &c->condition[i];

		// If the ID is changed and all previous statements of the same ID were true
		// then this condition has been successfully achieved
		if (lastID && lastID != cn->id && achievedSoFar)
			return true;

		// Skip future conditions with the same ID if one fails, for obvious reasons
		if (lastID && lastID == cn->id && !achievedSoFar)
			continue;

		// Skip entries that are JUST for string building
		if (cn->type == UC_AND || cn->type == UC_COMMA)
			continue;

		lastID = cn->id;

		if ((player != NULL) != (cn->type >= UCRP_REQUIRESPLAYING))
		{
			//CONS_Printf("skipping %s:%u:%u (%s)\n", sizeu1(c-conditionSets), cn->id, i, player ? "player exists" : "player does not exist");
			achievedSoFar = false;
			continue;
		}

		achievedSoFar = M_CheckCondition(cn, player);
		//CONS_Printf("%s:%u:%u - %u is %s\n", sizeu1(c-conditionSets), cn->id, i, cn->type, achievedSoFar ? "true" : "false");
	}

	return achievedSoFar;
}

static char *M_BuildConditionTitle(UINT16 map)
{
	char *title, *ref;

	if (((mapheaderinfo[map]->menuflags & LF2_FINISHNEEDED)
	// the following is intentionally not MV_BEATEN, just in case the title is for "Finish a round on X"
	&& !(mapheaderinfo[map]->records.mapvisited & MV_VISITED))
	|| M_MapLocked(map+1))
		return Z_StrDup("???");

	title = ref = G_BuildMapTitle(map+1);

	if (!title)
		I_Error("M_BuildConditionTitle: out of memory");

	while (*ref != '\0')
	{
		*ref = toupper(*ref);
		ref++;
	}

	return title;
}

static const char *M_GetNthType(UINT8 position)
{
	if (position == 1)
		return "st";
	if (position == 2)
		return "nd";
	if (position == 3)
		return "rd";
	return "th";
}

// See also M_CheckCondition
static const char *M_GetConditionString(condition_t *cn)
{
	INT32 i;
	char *title = NULL;
	const char *work = NULL;

	// If this function returns NULL, it stops building the condition and just does ???'s.

#define BUILDCONDITIONTITLE(i) (M_BuildConditionTitle(i))

	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x

			return va("play for %i:%02i:%02i",
				G_TicsToHours(cn->requirement),
				G_TicsToMinutes(cn->requirement, false),
				G_TicsToSeconds(cn->requirement));

		case UC_ROUNDSPLAYED: // Requires any level completed >= x times

			if (cn->extrainfo1 == GDGT_MAX)
				work = "";
			else if (cn->extrainfo1 != GDGT_RACE && cn->extrainfo1 != GDGT_BATTLE // Base gametypes
				&& (cn->extrainfo1 != GDGT_CUSTOM || M_SecretUnlocked(SECRET_ADDONS, true) == false) // Custom is visible at 0 if addons are unlocked
				&& gamedata->roundsplayed[cn->extrainfo1] == 0)
					work = " ???";
			else switch (cn->extrainfo1)
			{
				case GDGT_RACE:
					work = " Race";
					break;
				case GDGT_PRISONS:
					work = " Prison";
					break;
				case GDGT_BATTLE:
					work = " Battle";
					break;
				case GDGT_SPECIAL:
					work = " Special";
					break;
				case GDGT_CUSTOM:
					work = " custom gametype";
					break;
				default:
					return va("INVALID GAMETYPE CONDITION \"%d:%d:%d\"", cn->type, cn->extrainfo1, cn->requirement);
			}

			return va("play %d%s Round%s", cn->requirement, work,
				(cn->requirement == 1 ? "" : "s"));

		case UC_TOTALRINGS: // Requires collecting >= x rings
			if (cn->requirement >= 1000000)
				return va("collect %u,%03u,%03u Rings", (cn->requirement/1000000), (cn->requirement/1000)%1000, (cn->requirement%1000));
			if (cn->requirement >= 1000)
				return va("collect %u,%03u Rings", (cn->requirement/1000), (cn->requirement%1000));
			return va("collect %u Rings", cn->requirement);

		case UC_GAMECLEAR: // Requires game beaten >= x times
			if (cn->requirement > 1)
				return va("beat game %d times", cn->requirement);
			else
				return va("beat the game");

		case UC_OVERALLTIME: // Requires overall time <= x
			return va("get overall time of %i:%02i:%02i",
				G_TicsToHours(cn->requirement),
				G_TicsToMinutes(cn->requirement, false),
				G_TicsToSeconds(cn->requirement));

		case UC_MAPVISITED: // Requires map x to be visited
		case UC_MAPBEATEN: // Requires map x to be beaten
		case UC_MAPENCORE: // Requires map x to be beaten in encore
		case UC_MAPSPBATTACK: // Requires map x to be beaten in SPB Attack
		{
			const char *prefix = "";

			if (cn->requirement >= nummapheaders || !mapheaderinfo[cn->requirement])
				return va("INVALID MAP CONDITION \"%d:%d\"", cn->type, cn->requirement);

			title = BUILDCONDITIONTITLE(cn->requirement);

			if (cn->type == UC_MAPSPBATTACK)
				prefix = (M_SecretUnlocked(SECRET_SPBATTACK, true) ? "SPB ATTACK: " : "???: ");
			else if (cn->type == UC_MAPENCORE)
				prefix = (M_SecretUnlocked(SECRET_ENCORE, true) ? "ENCORE MODE: " : "???: ");

			work = "finish a round on";
			if (cn->type == UC_MAPVISITED)
				work = "visit";
			else if (cn->type == UC_MAPSPBATTACK)
				work = "conquer";

			work = va("%s%s %s%s",
				prefix,
				work,
				title,
				(cn->type == UC_MAPENCORE) ? " in Encore Mode" : "");
			Z_Free(title);
			return work;
		}

		case UC_MAPTIME: // Requires time on map <= x
		{
			if (cn->extrainfo1 >= nummapheaders || !mapheaderinfo[cn->extrainfo1])
				return va("INVALID MAP CONDITION \"%d:%d:%d\"", cn->type, cn->extrainfo1, cn->requirement);

			title = BUILDCONDITIONTITLE(cn->extrainfo1);
			work = va("beat %s in %i:%02i.%02i", title,
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement),
				G_TicsToCentiseconds(cn->requirement));

			Z_Free(title);
			return work;
		}

		case UC_ALLCHAOS:
		case UC_ALLSUPER:
		case UC_ALLEMERALDS:
		{
			const char *chaostext, *speedtext = "";

			if (!gamedata->everseenspecial)
				return NULL;

			if (cn->type == UC_ALLCHAOS)
				chaostext = "7 Chaos";
			else if (cn->type == UC_ALLSUPER)
				chaostext = "7 Super";
			else
				chaostext = "14";

			/*if (cn->requirement == KARTSPEED_NORMAL) -- Emeralds can not be collected on Easy
			{
				speedtext = " on Normal difficulty";
			}
			else*/
			if (cn->requirement == KARTSPEED_HARD)
			{
				speedtext = " on Hard difficulty";
			}
			else if (cn->requirement == KARTGP_MASTER)
			{
				if (M_SecretUnlocked(SECRET_MASTERMODE, true))
					speedtext = " on Master difficulty";
				else
					speedtext = " on ???";
			}

			return va("GRAND PRIX: collect all %s Emeralds%s", chaostext, speedtext);
		}

		case UC_TOTALMEDALS: // Requires number of emblems >= x
			return va("get %d medals", cn->requirement);

		case UC_EMBLEM: // Requires emblem x to be obtained
		{
			INT32 checkLevel;

			i = cn->requirement-1;
			checkLevel = M_EmblemMapNum(&emblemlocations[i]);

			if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
				return va("INVALID MEDAL MAP \"%d:%d\"", cn->requirement, checkLevel);

			title = BUILDCONDITIONTITLE(checkLevel);
			switch (emblemlocations[i].type)
			{
				case ET_MAP:
					work = "";
					if (emblemlocations[i].flags & ME_SPBATTACK)
						work = (M_SecretUnlocked(SECRET_SPBATTACK, true) ? "SPB ATTACK: " : "???: ");
					else if (emblemlocations[i].flags & ME_ENCORE)
						work = (M_SecretUnlocked(SECRET_ENCORE, true) ? "ENCORE MODE: " : "???: ");

					work = va("%s%s %s",
						work,
						(emblemlocations[i].flags & ME_SPBATTACK) ? "conquer" : "finish a round on",
						title);
					break;
				case ET_TIME:
					if (emblemlocations[i].color <= 0 || emblemlocations[i].color >= numskincolors)
					{
						Z_Free(title);
						return va("INVALID MEDAL COLOR \"%d:%d\"", cn->requirement, checkLevel);
					}
					work = va("TIME ATTACK: get the %s Medal for %s", skincolors[emblemlocations[i].color].name, title);
					break;
				case ET_GLOBAL:
				{
					const char *astr, *colorstr, *medalstr;

					if (emblemlocations[i].flags & GE_NOTMEDAL)
					{
						astr = "a ";
						colorstr = "";
						medalstr = "secret";
					}
					else if (emblemlocations[i].color <= 0 || emblemlocations[i].color >= numskincolors)
					{
						Z_Free(title);
						return va("INVALID MEDAL COLOR \"%d:%d:%d\"", cn->requirement, emblemlocations[i].tag, checkLevel);
					}
					else
					{
						astr = "the ";
						colorstr = skincolors[emblemlocations[i].color].name;
						medalstr = " Medal";
					}

					if (emblemlocations[i].flags & GE_TIMED)
					{
						work = va("%s: find %s%s%s before %i:%02i.%02i",
							title, astr, colorstr, medalstr,
							G_TicsToMinutes(emblemlocations[i].var, true),
							G_TicsToSeconds(emblemlocations[i].var),
							G_TicsToCentiseconds(emblemlocations[i].var));
					}
					else
					{
						work = va("%s: find %s%s%s",
							title, astr, colorstr, medalstr);
					}
					break;
				}
				default:
					work = va("find a secret in %s", title);
					break;
			}

			Z_Free(title);
			return work;
		}
		case UC_UNLOCKABLE: // Requires unlockable x to be obtained
			return va("get \"%s\"",
				gamedata->unlocked[cn->requirement-1]
				? unlockables[cn->requirement-1].name
				: "???");

		case UC_ADDON:
			if (!M_SecretUnlocked(SECRET_ADDONS, true))
				return NULL;
			return "load a custom addon into \"Dr. Robotnik's Ring Racers\"";
		case UC_REPLAY:
			return "save a replay after finishing a round";
		case UC_CRASH:
			if (gamedata->evercrashed)
				return "launch \"Dr. Robotnik's Ring Racers\" again after a game crash";
			return NULL;

		case UC_AND:
			return "&";
		case UC_COMMA:
			return ",";

		case UCRP_PREFIX_GRANDPRIX:
			return "GRAND PRIX:";
		case UCRP_PREFIX_BONUSROUND:
			return "BONUS ROUND:";
		case UCRP_PREFIX_TIMEATTACK:
			if (!M_SecretUnlocked(SECRET_TIMEATTACK, true))
				return NULL;
			return "TIME ATTACK:";
		case UCRP_PREFIX_PRISONBREAK:
			return "PRISON BREAK:";
		case UCRP_PREFIX_SEALEDSTAR:
			if (!gamedata->everseenspecial)
				return NULL;
			return "SEALED STARS:";

		case UCRP_PREFIX_ISMAP:
			if (cn->requirement >= nummapheaders || !mapheaderinfo[cn->requirement])
				return va("INVALID MAP CONDITION \"%d:%d\":", cn->type, cn->requirement);

			title = BUILDCONDITIONTITLE(cn->requirement);
			work = va("%s:", title);
			Z_Free(title);
			return work;
		case UCRP_ISMAP:
			if (cn->requirement >= nummapheaders || !mapheaderinfo[cn->requirement])
				return va("INVALID MAP CONDITION \"%d:%d\"", cn->type, cn->requirement);

			title = BUILDCONDITIONTITLE(cn->requirement);
			work = va("on %s", title);
			Z_Free(title);
			return work;
		case UCRP_ISCHARACTER:
			if (cn->requirement < 0 || !skins[cn->requirement].realname[0])
				return va("INVALID CHAR CONDITION \"%d:%d\"", cn->type, cn->requirement);
			work = (R_SkinUsable(-1, cn->requirement, false))
				? skins[cn->requirement].realname
				: "???";
			return va("as %s", work);
		case UCRP_ISENGINECLASS:
			return va("with engine class %c", 'A' + cn->requirement);
		case UCRP_ISDIFFICULTY:
		{
			const char *speedtext = "";

			if (cn->requirement == KARTSPEED_NORMAL)
			{
				speedtext = "on Normal difficulty";
			}
			else if (cn->requirement == KARTSPEED_HARD)
			{
				speedtext = "on Hard difficulty";
			}
			else if (cn->requirement == KARTGP_MASTER)
			{
				if (M_SecretUnlocked(SECRET_MASTERMODE, true))
					speedtext = "on Master difficulty";
				else
					speedtext = "on ???";
			}

			return speedtext;
		}

		case UCRP_PODIUMCUP:
		{
			cupheader_t *cup;
			const char *completetype = "complete", *orbetter = "";

			if (cn->extrainfo2)
			{
				switch (cn->requirement)
				{
					case GRADE_E: { completetype = "get grade E"; break; }
					case GRADE_D: { completetype = "get grade D"; break; }
					case GRADE_C: { completetype = "get grade C"; break; }
					case GRADE_B: { completetype = "get grade B"; break; }
					case GRADE_A: { completetype = "get grade A"; break; }
					case GRADE_S: { completetype = "get grade S"; break; }
					default: { break; }
				}

				if (cn->requirement < GRADE_S)
					orbetter = " or better in";
				else
					orbetter = " in";
			}
			else if (cn->extrainfo1 == 0)
				;
			else if (cn->extrainfo1 == 1)
				completetype = "get Gold in";
			else
			{
				if (cn->extrainfo1 == 2)
					completetype = "get Silver";
				else if (cn->extrainfo1 == 3)
					completetype = "get Bronze";
				orbetter = " or better in";
			}

			for (cup = kartcupheaders; cup; cup = cup->next)
			{
				if (cup->id != cn->requirement)
					continue;
				return va("%s%s %s CUP",
					completetype, orbetter,
					(M_CupLocked(cup) ? "???" : cup->name)
				);
			}
			return va("INVALID CUP CONDITION \"%d:%d\"", cn->type, cn->requirement);
		}
		case UCRP_PODIUMEMERALD:
			if (!gamedata->everseenspecial)
				return "???";
			return "collect the Emerald";
		case UCRP_PODIUMPRIZE:
			if (!gamedata->everseenspecial)
				return "???";
			return "collect the prize";

		case UCRP_FINISHCOOL:
			return "finish in good standing";
		case UCRP_FINISHALLPRISONS:
			return "break every prison";
		case UCRP_NOCONTEST:
			return "NO CONTEST";
		case UCRP_FINISHPLACE:
		case UCRP_FINISHPLACEEXACT:
			return va("finish in %d%s%s", cn->requirement, M_GetNthType(cn->requirement),
				((cn->type == UCRP_FINISHPLACE && cn->requirement > 1)
					? " or better" : ""));
		case UCRP_FINISHTIME:
			return va("finish in %i:%02i.%02i",
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement),
				G_TicsToCentiseconds(cn->requirement));
		case UCRP_FINISHTIMEEXACT:
			return va("finish in exactly %i:%02i.XX",
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement));
		case UCRP_FINISHTIMELEFT:
			return va("finish with %i:%02i.%02i remaining",
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement),
				G_TicsToCentiseconds(cn->requirement));

		case UCRP_TRIGGER:
			return "do something special";

		case UCRP_FALLOFF:
			return (cn->requirement == 1) ? "fall off the course" : "without falling off";
		case UCRP_TOUCHOFFROAD:
			return (cn->requirement == 1) ? "touch offroad" : "without touching any offroad";
		case UCRP_TOUCHSNEAKERPANEL:
			return (cn->requirement == 1) ? "touch a Sneaker Panel" : "without touching any Sneaker Panels";
		case UCRP_RINGDEBT:
			return (cn->requirement == 1) ? "go into Ring debt" : "without going into Ring debt";

		case UCRP_TRIPWIREHYUU:
			return "go through Tripwire after getting snared by Hyudoro";
		case UCRP_SPBNEUTER:
			return "shock a Self Propelled Bomb into submission";
		case UCRP_LANDMINEDUNK:
			return "dunk a Landmine on another racer's head";
		case UCRP_HITMIDAIR:
			return "hit another racer with a projectile while you're both in the air";

		case UCRP_WETPLAYER:
			return va("without %s %s",
				(cn->requirement & MFE_TOUCHWATER) ? "touching any" : "going into",
				cn->stringvar);

		default:
			break;
	}
	// UC_MAPTRIGGER and UC_CONDITIONSET are explicitly very hard to support proper descriptions for
	return va("UNSUPPORTED CONDITION \"%d\"", cn->type);

#undef BUILDCONDITIONTITLE
}

char *M_BuildConditionSetString(UINT16 unlockid)
{
	conditionset_t *c = NULL;
	UINT32 lastID = 0;
	condition_t *cn;
	size_t len = 1024, worklen;
	static char message[1024] = "";
	const char *work = NULL;
	size_t max = 0, maxatstart = 0, start = 0, i;
	boolean stopasap = false;

	message[0] = '\0';

	if (unlockid >= MAXUNLOCKABLES)
	{
		return NULL;
	}

	if (!unlockables[unlockid].conditionset)
	{
		return NULL;
	}

	if (gamedata->unlocked[unlockid] == true && M_Achieved(unlockables[unlockid].conditionset - 1) == false)
	{
		message[0] = '\x86'; // the following text will be grey
		message[1] = '\0';
		len--;
	}

	c = &conditionSets[unlockables[unlockid].conditionset-1];

	for (i = 0; i < c->numconditions; ++i)
	{
		cn = &c->condition[i];

		if (i > 0 && (cn->type != UC_COMMA))
		{
			if (lastID != cn->id)
			{
				worklen = 4;
				strncat(message, "\nOR ", len);
			}
			else
			{
				worklen = 1;
				strncat(message, " ", len);
			}
			len -= worklen;
		}

		lastID = cn->id;

		work = M_GetConditionString(cn);
		if (work == NULL)
		{
			stopasap = true;
			work = "???";
		}
		worklen = strlen(work);

		strncat(message, work, len);
		len -= worklen;

		if (stopasap)
		{
			break;
		}
	}

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, etc. but who cares.
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
			maxatstart = max;
		}
		else if (message[i] == '\n')
		{
			start = 0;
			max = 0;
			maxatstart = 0;
			continue;
		}
		else if (message[i] & 0x80)
			continue;
		else
			max += 8;

		// Start trying to wrap if presumed length exceeds the space we have on-screen.
		if (max >= DESCRIPTIONWIDTH && start > 0)
		{
			message[start] = '\n';
			max -= maxatstart;
			start = 0;
		}
	}

	// Valid sentence capitalisation handling.
	{
		// Finds the first : character, indicating the end of the prefix.
		for (i = 0; message[i]; i++)
		{
			if (message[i] != ':')
				continue;
			i++;
			break;
		}

		// If we didn't find a prefix, just start from the first character again.
		if (!message[i])
			i = 0;

		// Okay, now make the first non-whitespace character after the prefix a capital.
		// Doesn't matter if !isalpha() - toupper is a no-op.
		for (; message[i]; i++)
		{
			if ((message[i] & 0x80) || isspace(message[i]))
				continue;
			message[i] = toupper(message[i]);
			break;
		}
	}

	return message;
}

static boolean M_CheckUnlockConditions(player_t *player)
{
	INT32 i;
	conditionset_t *c;
	boolean ret;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];
		if (!c->numconditions || gamedata->achieved[i])
			continue;

		if ((gamedata->achieved[i] = (M_CheckConditionSet(c, player))) != true)
			continue;

		ret = true;
	}

	return ret;
}

boolean M_UpdateUnlockablesAndExtraEmblems(boolean loud, boolean doall)
{
	UINT16 i = 0, response = 0, newkeys = 0;

	if (!gamedata)
	{
		// Don't attempt to write/check anything.
		return false;
	}

	if (!loud)
	{
		// Just in case they aren't to sync
		// Done first so that emblems are ready before check
		M_CheckLevelEmblems();
		M_CompletionEmblems();
		doall = true;
	}

	if (gamedata->deferredconditioncheck == true)
	{
		// Handle deferred all-condition checks
		gamedata->deferredconditioncheck = false;
		doall = true;
	}

	if (doall)
	{
		response = M_CheckUnlockConditions(NULL);

		if (gamedata->pendingkeyrounds == 0
			|| (gamedata->chaokeys >= GDMAX_CHAOKEYS))
		{
			gamedata->keyspending = 0;
		}
		else while ((gamedata->keyspending + gamedata->chaokeys) < GDMAX_CHAOKEYS
			&& ((gamedata->pendingkeyrounds + gamedata->pendingkeyroundoffset)/GDCONVERT_ROUNDSTOKEY) > gamedata->keyspending)
		{
			gamedata->keyspending++;
			newkeys++;
			response |= true;
		}
	}

	if (!demo.playback && Playing() && (gamestate == GS_LEVEL || K_PodiumRanking() == true))
	{
		for (i = 0; i <= splitscreen; i++)
		{
			if (!playeringame[g_localplayers[i]])
				continue;
			if (players[g_localplayers[i]].spectator)
				continue;
			if (!doall && players[g_localplayers[i]].roundconditions.checkthisframe == false)
				continue;
			response |= M_CheckUnlockConditions(&players[g_localplayers[i]]);
			players[g_localplayers[i]].roundconditions.checkthisframe = false;
		}
	}

	if (loud && response == 0)
	{
		return false;
	}

	response = 0;

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (gamedata->unlocked[i] || !unlockables[i].conditionset)
		{
			continue;
		}

		if (gamedata->unlocked[i] == true
			|| gamedata->unlockpending[i] == true)
		{
			continue;
		}

		if (M_Achieved(unlockables[i].conditionset - 1) == false)
		{
			continue;
		}

		gamedata->unlockpending[i] = true;
		response++;
	}

	response += newkeys;

	// Announce
	if (response != 0)
	{
		if (loud)
		{
			S_StartSound(NULL, sfx_achiev);
		}
		return true;
	}
	return false;
}

UINT16 M_GetNextAchievedUnlock(void)
{
	UINT16 i;

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (gamedata->unlocked[i] || !unlockables[i].conditionset)
		{
			continue;
		}

		if (gamedata->unlocked[i] == true)
		{
			continue;
		}

		if (gamedata->unlockpending[i] == false)
		{
			continue;
		}

		return i;
	}

	if (gamedata->keyspending != 0)
	{
		return PENDING_CHAOKEYS;
	}

	return MAXUNLOCKABLES;
}

// Emblem unlocking shit
UINT16 M_CheckLevelEmblems(void)
{
	INT32 i;
	INT32 valToReach;
	INT16 tag;
	INT16 levelnum;
	boolean res;
	UINT16 somethingUnlocked = 0;

	// Update Score, Time, Rings emblems
	for (i = 0; i < numemblems; ++i)
	{
		INT32 checkLevel;

		if (emblemlocations[i].type < ET_TIME || gamedata->collected[i])
			continue;

		checkLevel = M_EmblemMapNum(&emblemlocations[i]);

		if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
			continue;

		levelnum = checkLevel;
		valToReach = emblemlocations[i].var;
		tag = emblemlocations[i].tag;

		switch (emblemlocations[i].type)
		{
			case ET_TIME: // Requires time on map <= x
				if (tag > 0)
				{
					if (tag > mapheaderinfo[checkLevel]->ghostCount
					|| mapheaderinfo[checkLevel]->ghostBrief[tag-1] == NULL)
						continue;

					res = (G_GetBestTime(levelnum) <= mapheaderinfo[checkLevel]->ghostBrief[tag-1]->time);
				}
				else
				{
					res = (G_GetBestTime(levelnum) <= (unsigned)valToReach);
				}
				break;
			default: // unreachable but shuts the compiler up.
				continue;
		}

		gamedata->collected[i] = res;
		if (res)
			++somethingUnlocked;
	}
	return somethingUnlocked;
}

UINT16 M_CompletionEmblems(void) // Bah! Duplication sucks, but it's for a separate print when awarding emblems and it's sorta different enough.
{
	INT32 i;
	INT32 embtype;
	INT16 levelnum;
	boolean res;
	UINT16 somethingUnlocked = 0;
	UINT8 flags;

	for (i = 0; i < numemblems; ++i)
	{
		INT32 checkLevel;

		if (emblemlocations[i].type != ET_MAP || gamedata->collected[i])
			continue;

		checkLevel = M_EmblemMapNum(&emblemlocations[i]);

		if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
			continue;

		levelnum = checkLevel;
		embtype = emblemlocations[i].flags;
		flags = MV_BEATEN;

		if (embtype & ME_ENCORE)
			flags |= MV_ENCORE;

		if (embtype & ME_SPBATTACK)
			flags |= MV_SPBATTACK;

		res = ((mapheaderinfo[levelnum]->records.mapvisited & flags) == flags);

		gamedata->collected[i] = res;
		if (res)
			++somethingUnlocked;
	}

	return somethingUnlocked;
}

// -------------------
// Quick unlock checks
// -------------------

boolean M_CheckNetUnlockByID(UINT16 unlockid)
{
	if (unlockid >= MAXUNLOCKABLES
		|| !unlockables[unlockid].conditionset)
	{
		return true; // default permit
	}

	if (netgame)
	{
		return netUnlocked[unlockid];
	}

	return gamedata->unlocked[unlockid];
}

boolean M_SecretUnlocked(INT32 type, boolean local)
{
	INT32 i;

#if 0
	(void)type;
	(void)i;
	return false; // for quick testing
#else

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != type)
			continue;
		if ((local && gamedata->unlocked[i])
			|| (!local && M_CheckNetUnlockByID(i)))
			continue;
		return false;
	}

	return true;

#endif //if 0
}

boolean M_CupLocked(cupheader_t *cup)
{
	UINT16 i;

	// Don't lock maps in dedicated servers.
	// That just makes hosts' lives hell.
	if (dedicated)
		return false;

	// No skipping over any part of your marathon.
	if (marathonmode)
		return false;

	if (!cup)
		return false;

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != SECRET_CUP)
			continue;
		if (M_UnlockableCup(&unlockables[i]) != cup)
			continue;
		return !M_CheckNetUnlockByID(i);
	}

	return false;
}

boolean M_MapLocked(UINT16 mapnum)
{
	UINT16 i;

	// Don't lock maps in dedicated servers.
	// That just makes hosts' lives hell.
	if (dedicated)
		return false;

	// No skipping over any part of your marathon.
	if (marathonmode)
		return false;

	if (mapnum == 0 || mapnum > nummapheaders)
		return false;
	
	if (!mapheaderinfo[mapnum-1])
		return false;

	if (mapheaderinfo[mapnum-1]->cup)
	{
		return M_CupLocked(mapheaderinfo[mapnum-1]->cup);
	}

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != SECRET_MAP)
			continue;
		if (M_UnlockableMapNum(&unlockables[i]) != mapnum-1)
			continue;
		return !M_CheckNetUnlockByID(i);
	}

	return false;
}

INT32 M_CountMedals(boolean all, boolean extraonly)
{
	INT32 found = 0, i;
	if (!extraonly)
	{
		for (i = 0; i < numemblems; ++i)
		{
			if ((emblemlocations[i].type == ET_GLOBAL)
				&& (emblemlocations[i].flags & GE_NOTMEDAL))
				continue;
			if (!all && !gamedata->collected[i])
				continue;
			found++;
		}
	}
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != SECRET_EXTRAMEDAL)
			continue;
		if (!all && !gamedata->unlocked[i])
			continue;
		found++;
	}
	return found;
}

// --------------------------------------
// Quick functions for calculating things
// --------------------------------------

// Theoretically faster than using M_CountMedals()
// Stops when it reaches the target number of medals.
boolean M_GotEnoughMedals(INT32 number)
{
	INT32 i, gottenmedals = 0;
	for (i = 0; i < numemblems; ++i)
	{
		if (!gamedata->collected[i])
			continue;
		if (++gottenmedals < number)
			continue;
		return true;
	}
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != SECRET_EXTRAMEDAL)
			continue;
		if (!gamedata->unlocked[i])
			continue;
		if (++gottenmedals < number)
			continue;
		return true;
	}
	return false;
}

boolean M_GotLowEnoughTime(INT32 tictime)
{
	INT32 curtics = 0;
	INT32 i;

	for (i = 0; i < nummapheaders; ++i)
	{
		if (!mapheaderinfo[i] || (mapheaderinfo[i]->menuflags & LF2_NOTIMEATTACK))
			continue;

		if (!mapheaderinfo[i]->records.time)
			return false;
		if ((curtics += mapheaderinfo[i]->records.time) > tictime)
			return false;
	}
	return true;
}

// Gets the skin number for a SECRET_SKIN unlockable.
INT32 M_UnlockableSkinNum(unlockable_t *unlock)
{
	if (unlock->type != SECRET_SKIN)
	{
		// This isn't a skin unlockable...
		return -1;
	}

	if (unlock->stringVar && unlock->stringVar[0])
	{
		INT32 skinnum;

		if (unlock->stringVarCache != -1)
		{
			return unlock->stringVarCache;
		}

		// Get the skin from the string.
		skinnum = R_SkinAvailable(unlock->stringVar);
		if (skinnum != -1)
		{
			unlock->stringVarCache = skinnum;
			return skinnum;
		}
	}

	if (unlock->variable >= 0 && unlock->variable < numskins)
	{
		// Use the number directly.
		return unlock->variable;
	}

	// Invalid skin unlockable.
	return -1;
}

// Gets the skin number for a SECRET_FOLLOWER unlockable.
INT32 M_UnlockableFollowerNum(unlockable_t *unlock)
{
	if (unlock->type != SECRET_FOLLOWER)
	{
		// This isn't a follower unlockable...
		return -1;
	}

	if (unlock->stringVar && unlock->stringVar[0])
	{
		INT32 skinnum;
		size_t i;
		char testname[SKINNAMESIZE+1];

		if (unlock->stringVarCache != -1)
		{
			return unlock->stringVarCache;
		}

		// match deh_soc readfollower()
		for (i = 0; unlock->stringVar[i]; i++)
		{
			testname[i] = unlock->stringVar[i];
			if (unlock->stringVar[i] == '_')
				testname[i] = ' ';
		}
		testname[i] = '\0';

		// Get the skin from the string.
		skinnum = K_FollowerAvailable(testname);
		if (skinnum != -1)
		{
			unlock->stringVarCache = skinnum;
			return skinnum;
		}
	}

	if (unlock->variable >= 0 && unlock->variable < numfollowers)
	{
		// Use the number directly.
		return unlock->variable;
	}

	// Invalid follower unlockable.
	return -1;
}

cupheader_t *M_UnlockableCup(unlockable_t *unlock)
{
	cupheader_t *cup = kartcupheaders;
	INT16 val = unlock->variable-1;

	if (unlock->type != SECRET_CUP)
	{
		// This isn't a cup unlockable...
		return NULL;
	}

	if (unlock->stringVar && unlock->stringVar[0])
	{
		if (unlock->stringVarCache == -1)
		{
			// Get the cup from the string.
			UINT32 hash = quickncasehash(unlock->stringVar, MAXCUPNAME);
			while (cup)
			{
				if (hash == cup->namehash && !strcmp(cup->name, unlock->stringVar))
					break;
				cup = cup->next;
			}

			if (cup)
			{
				unlock->stringVarCache = cup->id;
			}
			return cup;
		}

		val = unlock->stringVarCache;
	}
	else if (val == -1)
	{
		return NULL;
	}

	// Use the number directly.
	while (cup)
	{
		if (cup->id == val)
			break;
		cup = cup->next;
	}

	return cup;
}

UINT16 M_UnlockableMapNum(unlockable_t *unlock)
{
	if (unlock->type != SECRET_MAP)
	{
		// This isn't a map unlockable...
		return NEXTMAP_INVALID;
	}

	if (unlock->stringVar && unlock->stringVar[0])
	{
		if (unlock->stringVarCache == -1)
		{
			INT32 result = G_MapNumber(unlock->stringVar);

			if (result >= nummapheaders)
				return result;

			unlock->stringVarCache = result;
		}

		return unlock->stringVarCache;
	}

	return NEXTMAP_INVALID;
}

// ----------------
// Misc Emblem shit
// ----------------

UINT16 M_EmblemMapNum(emblem_t *emblem)
{
	if (emblem->levelCache == NEXTMAP_INVALID)
	{
		UINT16 result = G_MapNumber(emblem->level);

		if (result >= nummapheaders)
			return result;

		emblem->levelCache = result;
	}

	return emblem->levelCache;
}

// Returns pointer to an emblem if an emblem exists for that level.
// Pass -1 mapnum to continue from last emblem.
// NULL if not found.
// note that this goes in reverse!!
emblem_t *M_GetLevelEmblems(INT32 mapnum)
{
	static INT32 map = -1;
	static INT32 i = -1;

	if (mapnum > 0)
	{
		map = mapnum-1;
		i = numemblems;
	}

	while (--i >= 0)
	{
		INT32 checkLevel = M_EmblemMapNum(&emblemlocations[i]);

		if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
			continue;

		if (checkLevel != map)
			continue;

		return &emblemlocations[i];
	}
	return NULL;
}

skincolornum_t M_GetEmblemColor(emblem_t *em)
{
	if (!em || !em->color || em->color >= numskincolors)
		return SKINCOLOR_GOLD;
	return em->color;
}

const char *M_GetEmblemPatch(emblem_t *em, boolean big)
{
	static char pnamebuf[7];

	if (!big)
		strcpy(pnamebuf, "GOTITn");
	else
		strcpy(pnamebuf, "EMBMn0");

	I_Assert(em->sprite >= 'A' && em->sprite <= 'Z');

	if (!big)
		pnamebuf[5] = em->sprite;
	else
		pnamebuf[4] = em->sprite;

	return pnamebuf;
}
