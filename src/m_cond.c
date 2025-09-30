// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cond.c
/// \brief Challenges internals

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
#include "k_objects.h" // Obj_AllAncientGearsCollected

gamedata_t *gamedata = NULL;
boolean netUnlocked[MAXUNLOCKABLES];

// The meat of this system lies in condition sets
conditionset_t conditionSets[MAXCONDITIONSETS];

// Emblem locations
emblem_t emblemlocations[MAXEMBLEMS];

// Unlockables
unlockable_t unlockables[MAXUNLOCKABLES];

// Highest used emblem ID
INT32 numemblems = 0;

// The challenge that will truly let the games begin.
UINT16 gamestartchallenge = 600; // 601

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
		INT16 *quickcheck = Z_Calloc(sizeof(INT16) * 2 * numspots, PU_STATIC, NULL);

		// Prepare the easy-grab spots.
		for (i = 0; i < numspots; i++)
		{
			quickcheck[i * 2 + 0] = i%(CHALLENGEGRIDHEIGHT-1);
			quickcheck[i * 2 + 1] = majorcompact * i/(CHALLENGEGRIDHEIGHT-1);
		}

		// Place in random valid locations.
		while (nummajorunlocks > 0 && numspots > 0)
		{
			INT16 row, col;
			j = M_RandomKey(numspots);
			row = quickcheck[j * 2 + 0];
			col =  quickcheck[j * 2 + 1];

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
				if (abs((quickcheck[i * 2 + 0]) - (row)) <= 1 // Row distance
					&& (abs((quickcheck[i * 2 + 1]) - (col)) <= 1 // Column distance
					|| (quickcheck[i * 2 + 1] == 0 && col == gamedata->challengegridwidth-1) // Wraparounds l->r
					|| (quickcheck[i * 2 + 1] == gamedata->challengegridwidth-1 && col == 0))) // Wraparounds r->l
				{
					numspots--;  // Remove from possible indicies
					if (i == numspots)
						break;
					// Shuffle remaining so we can keep on using M_RandomKey
					quickcheck[i * 2 + 0] = quickcheck[numspots * 2 + 0];
					quickcheck[i * 2 + 1] = quickcheck[numspots * 2 + 1];
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

void M_SanitiseChallengeGrid(void)
{
	UINT8 seen[MAXUNLOCKABLES];
	UINT16 empty[MAXUNLOCKABLES + (CHALLENGEGRIDHEIGHT-1)];
	UINT16 i, j, numempty = 0;

	if (gamedata->challengegrid == NULL)
		return;

	memset(seen, 0, sizeof(seen));

	// Go through all spots to identify duplicates and absences.
	for (j = 0; j < gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT; j++)
	{
		i = gamedata->challengegrid[j];

		if (i >= MAXUNLOCKABLES || !unlockables[i].conditionset)
		{
			empty[numempty++] = j;
			continue;
		}

		if (seen[i] != 5) // Arbitrary cap greater than 4
		{
			seen[i]++;

			if (seen[i] == 1 || unlockables[i].majorunlock)
			{
				continue;
			}
		}

		empty[numempty++] = j;
	}

	// Go through unlockables to identify if any haven't been seen.
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (!unlockables[i].conditionset)
		{
			continue;
		}

		if (unlockables[i].majorunlock && seen[i] != 4)
		{
			// Probably not enough spots to retrofit.
			goto badgrid;
		}

		if (seen[i] != 0)
		{
			// Present on the challenge grid.
			continue;
		}

		if (numempty != 0)
		{
			// Small ones can be slotted in easy.
			j = empty[--numempty];
			gamedata->challengegrid[j] = i;
		}

		// Nothing we can do to recover.
		goto badgrid;
	}

	// Fill the remaining spots with empties.
	while (numempty != 0)
	{
		j = empty[--numempty];
		gamedata->challengegrid[j] = MAXUNLOCKABLES;
	}

	return;

badgrid:
	// Just remove everything and let it get regenerated.
	Z_Free(gamedata->challengegrid);
	gamedata->challengegrid = NULL;
	gamedata->challengegridwidth = 0;
}

static void M_ChallengeGridExtraDataAdjacencyRules(challengegridextradata_t *extradata, UINT16 adjacent)
{
	// Adjacent unlocked tile, permit hint/general key skip.
	if (gamedata->unlocked[adjacent] == true)
	{
		extradata->flags |= CHE_HINT;
	}
	// Adjacent locked small tile, prevent 10x key skip.
	else if (unlockables[adjacent].majorunlock == false)
	{
		extradata->flags &= ~CHE_ALLCLEAR;
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
			num = gamedata->challengegrid[id];
			if (num >= MAXUNLOCKABLES || unlockables[num].majorunlock == false || gamedata->unlocked[num] == true)
			{
				extradata[id].flags = CHE_NONE;
				continue;
			}

			// We only do this for locked large tiles, to reduce the
			// complexity of most standard tile challenge comparisons
			extradata[id].flags = CHE_ALLCLEAR;
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
						// CHE_ALLCLEAR has already been removed,
						// and CHE_HINT has already been applied,
						// so nothing more needs to be done here.
						continue;
					}
				}
				else if (work < MAXUNLOCKABLES)
				{
					M_ChallengeGridExtraDataAdjacencyRules(extradata+id, work);
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
						if (extradata[id].flags & CHE_HINT)
						{
							extradata[tempid].flags |= CHE_HINT;
						}
						if (!(extradata[id].flags & CHE_ALLCLEAR))
						{
							extradata[tempid].flags &= ~CHE_ALLCLEAR;
						}
						extradata[id].flags = CHE_CONNECTEDLEFT;
						id = tempid;
					}
					/*else
						CONS_Printf(" %d - %d to left of %d is invalid\n", work, tempid, id);*/
				}
				else if (work < MAXUNLOCKABLES)
				{
					M_ChallengeGridExtraDataAdjacencyRules(extradata+id, work);
				}
			}

			// Since we're not modifying id past this point, the conditions become much simpler.
			if (extradata[id].flags == CHE_HINT)
			{
				// CHE_ALLCLEAR has already been removed,
				// and CHE_HINT has already been applied,
				// so nothing more needs to be done here.
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
				else if (work < MAXUNLOCKABLES)
				{
					M_ChallengeGridExtraDataAdjacencyRules(extradata+id, work);
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
				else if (work < MAXUNLOCKABLES)
				{
					M_ChallengeGridExtraDataAdjacencyRules(extradata+id, work);
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
	UINT16 i;

	gamedata->totalplaytime = 0;
	gamedata->totalnetgametime = 0;
	gamedata->timeattackingtotaltime = 0;
	gamedata->spbattackingtotaltime = 0;
	for (i = 0; i < GDGT_MAX; ++i)
		gamedata->modeplaytime[i] = 0;
	gamedata->totalmenutime = 0;
	gamedata->totaltimestaringatstatistics = 0;
	gamedata->totalrings = 0;
	gamedata->totaltumbletime = 0;
	for (i = 0; i < GDGT_MAX; ++i)
		gamedata->roundsplayed[i] = 0;
	gamedata->timesBeaten = 0;

	gamedata->everloadedaddon = false;
	gamedata->everfinishedcredits = false;
	gamedata->eversavedreplay = false;
	gamedata->everseenspecial = false;
	gamedata->evercrashed = false;
	gamedata->chaokeytutorial = false;
	gamedata->majorkeyskipattempted = false;
	gamedata->enteredtutorialchallenge = false;
	gamedata->sealedswapalerted = false;
	gamedata->musicstate = GDMUSIC_NONE;

	gamedata->importprofilewins = false;

	// Skins only store stats, not progression metrics. Good to clear entirely here.

	for (i = 0; i < numskins; i++)
	{
		memset(&skins[i]->records, 0, sizeof(skins[i]->records));
	}

	unloaded_skin_t *unloadedskin, *nextunloadedskin = NULL;
	for (unloadedskin = unloadedskins; unloadedskin; unloadedskin = nextunloadedskin)
	{
		nextunloadedskin = unloadedskin->next;
		Z_Free(unloadedskin);
	}
	unloadedskins = NULL;

	// We retain exclusively the most important stuff from maps.

	UINT8 restoremapvisited;
	recordtimes_t restoretimeattack;
	recordtimes_t restorespbattack;

	for (i = 0; i < nummapheaders; i++)
	{
		restoremapvisited = mapheaderinfo[i]->records.mapvisited;
		restoretimeattack = mapheaderinfo[i]->records.timeattack;
		restorespbattack = mapheaderinfo[i]->records.spbattack;

		memset(&mapheaderinfo[i]->records, 0, sizeof(recorddata_t));

		mapheaderinfo[i]->records.mapvisited = restoremapvisited;
		mapheaderinfo[i]->records.timeattack = restoretimeattack;
		mapheaderinfo[i]->records.spbattack = restorespbattack;
	}

	unloaded_mapheader_t *unloadedmap;
	for (unloadedmap = unloadedmapheaders; unloadedmap; unloadedmap = unloadedmap->next)
	{
		restoremapvisited = unloadedmap->records.mapvisited;
		restoretimeattack = unloadedmap->records.timeattack;
		restorespbattack = unloadedmap->records.spbattack;

		memset(&unloadedmap->records, 0, sizeof(recorddata_t));

		unloadedmap->records.mapvisited = restoremapvisited;
		unloadedmap->records.timeattack = restoretimeattack;
		unloadedmap->records.spbattack = restorespbattack;
	}
}

void M_ClearSecrets(void)
{
	memset(gamedata->collected, 0, sizeof(gamedata->collected));
	memset(gamedata->unlocked, 0, sizeof(gamedata->unlocked));
	memset(gamedata->unlockpending, 0, sizeof(gamedata->unlockpending));
	memset(netUnlocked, 0, sizeof(netUnlocked));
	memset(gamedata->achieved, 0, sizeof(gamedata->achieved));

	Z_Free(gamedata->spraycans);
	gamedata->spraycans = NULL;
	gamedata->numspraycans = 0;
	gamedata->gotspraycans = 0;

	Z_Free(gamedata->prisoneggpickups);
	gamedata->prisoneggpickups = NULL;
	gamedata->numprisoneggpickups = 0;
	gamedata->thisprisoneggpickup = MAXCONDITIONSETS;
	gamedata->thisprisoneggpickup_cached = NULL;
	gamedata->thisprisoneggpickupgrabbed = false;

	UINT16 i, j;
	for (i = 0; i < nummapheaders; i++)
	{
		if (!mapheaderinfo[i])
			continue;

		mapheaderinfo[i]->records.mapvisited = 0;
		mapheaderinfo[i]->records.spraycan = MCAN_INVALID;

		mapheaderinfo[i]->cache_maplock = MAXUNLOCKABLES;

		for (j = 1; j < mapheaderinfo[i]->musname_size; j++)
		{
			mapheaderinfo[i]->cache_muslock[j-1] = MAXUNLOCKABLES;
		}
	}

	cupheader_t *cup;
	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		cup->cache_cuplock = MAXUNLOCKABLES;
	}

	for (i = 0; i < numskincolors; i++)
	{
		skincolors[i].cache_spraycan = UINT16_MAX;
	}

	memset(gamedata->sealedswaps, 0, sizeof(gamedata->sealedswaps));

	Z_Free(gamedata->challengegrid);
	gamedata->challengegrid = NULL;
	gamedata->challengegridwidth = 0;

	gamedata->pendingkeyrounds = 0;
	gamedata->pendingkeyroundoffset = 0;
	gamedata->keyspending = 0;

	gamedata->chaokeys = GDINIT_CHAOKEYS;
	gamedata->prisoneggstothispickup = GDINIT_PRISONSTOPRIZE;

	gamedata->tutorialdone = false;
	gamedata->playgroundroute = false;
	gamedata->finishedtutorialchallenge = false;

	gamedata->gonerlevel = GDGONER_INIT;
}

// For lack of a better idea on where to put this
static void M_Shuffle_UINT16(UINT16 *list, size_t len)
{
	size_t i;
	UINT16 temp;

	while (len > 1)
	{
		i = M_RandomKey(len);

		if (i == --len)
			continue;

		temp = list[i];
		list[i] = list[len];
		list[len] = temp;
	}
}

static void M_AssignSpraycans(void)
{
	// Very convenient I'm programming this on
	// the release date of "Bomb Rush Cyberfunk".
	// ~toast 180823 (committed a day later)

	// Init ordered list of skincolors
	UINT16 tempcanlist[MAXSKINCOLORS];
	UINT16 listlen = 0, prependlen = 0;

	UINT32 i, j;
	conditionset_t *c;
	condition_t *cn;

	UINT16 bonustocanmap = 0;

	// First, turn outstanding bonuses into existing uncollected Spray Cans.
	while (gamedata->gotspraycans < gamedata->numspraycans)
	{
		while (bonustocanmap < basenummapheaders)
		{
			if (mapheaderinfo[bonustocanmap]->records.spraycan != MCAN_BONUS)
			{
				bonustocanmap++;
				continue;
			}

			break;
		}

		if (bonustocanmap == basenummapheaders)
			break;

		mapheaderinfo[bonustocanmap]->records.spraycan = gamedata->gotspraycans;
		gamedata->spraycans[gamedata->gotspraycans].map = bonustocanmap;
		gamedata->gotspraycans++;
	}

	const UINT16 prependoffset = MAXSKINCOLORS-1;

	// None of the following accounts for cans being removed, only added...
	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];
		if (!c->numconditions)
			continue;

		for (j = 0; j < c->numconditions; ++j)
		{
			cn = &c->condition[j];
			if (cn->type != UC_SPRAYCAN)
				continue;

			// This will likely never support custom skincolors.
			if (cn->requirement >= SKINCOLOR_FIRSTFREESLOT) //numskincolors)
				continue;

			if (skincolors[cn->requirement].cache_spraycan != UINT16_MAX)
				continue;

			// Still invalid, just in case it isn't assigned one later
			skincolors[cn->requirement].cache_spraycan = UINT16_MAX-1;

			if (!cn->extrainfo1)
			{
				//CONS_Printf("DDD - Adding standard can color %d\n", cn->requirement);

				tempcanlist[listlen] = cn->requirement;
				listlen++;
				continue;
			}

			//CONS_Printf("DDD - Prepending early can color %d\n", cn->requirement);

			tempcanlist[prependoffset - prependlen] = cn->requirement;
			prependlen++;
		}
	}

	if (listlen)
	{
		// Swap the standard colours for random order
		M_Shuffle_UINT16(tempcanlist, listlen);
	}
	else if (!prependlen)
	{
		return;
	}

	if (prependlen)
	{
		// Swap the early colours for random order
		M_Shuffle_UINT16(tempcanlist + prependoffset - (prependlen - 1), prependlen);

		// Put at the front of the main list
		// (technically messes with the main order, but it
		// was LITERALLY just shuffled so it doesn't matter)
		i = 0;
		while (i < prependlen)
		{
			tempcanlist[listlen] = tempcanlist[i];
			tempcanlist[i] = tempcanlist[prependoffset - i];
			listlen++;
			i++;
		}
	}

	gamedata->spraycans = Z_Realloc(
		gamedata->spraycans,
		sizeof(candata_t) * (gamedata->numspraycans + listlen),
		PU_STATIC,
		NULL);

	for (i = 0; i < listlen; i++)
	{
		// Convert bonus pickups into Spray Cans if new ones have been added.
		while (bonustocanmap < basenummapheaders)
		{
			if (mapheaderinfo[bonustocanmap]->records.spraycan != MCAN_BONUS)
			{
				bonustocanmap++;
				continue;
			}

			gamedata->gotspraycans++;
			mapheaderinfo[bonustocanmap]->records.spraycan = gamedata->numspraycans;
			break;
		}
		gamedata->spraycans[gamedata->numspraycans].map = (
			(bonustocanmap == basenummapheaders)
				? NEXTMAP_INVALID
				: bonustocanmap
		);
		gamedata->spraycans[gamedata->numspraycans].col = tempcanlist[i];

		skincolors[tempcanlist[i]].cache_spraycan = gamedata->numspraycans;

		gamedata->numspraycans++;
	}
}

static void M_InitPrisonEggPickups(void)
{
	// Init ordered list of skincolors
	UINT16 temppickups[MAXCONDITIONSETS];
	UINT16 listlen = 0;

	UINT32 i, j;
	conditionset_t *c;
	condition_t *cn;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		// Optimisation - unlike Spray Cans, these are rebuilt every game launch/savedata wipe.
		// Therefore, we don't need to re-store the ones that have been achieved.
		if (gamedata->achieved[i])
			continue;

		c = &conditionSets[i];
		if (!c->numconditions)
			continue;

		for (j = 0; j < c->numconditions; ++j)
		{
			cn = &c->condition[j];
			if (cn->type != UC_PRISONEGGCD)
				continue;

			temppickups[listlen] = i;
			listlen++;
			break;
		}
	}

	if (!listlen)
	{
		return;
	}

	// This list doesn't need to be shuffled because it's always being randomly grabbed.
	// (Unlike Spray Cans, you don't know which CD you miss out on.)

	gamedata->prisoneggpickups = Z_Realloc(
		gamedata->prisoneggpickups,
		sizeof(UINT16) * listlen,
		PU_STATIC,
		NULL);

	while (gamedata->numprisoneggpickups < listlen)
	{
		gamedata->prisoneggpickups[gamedata->numprisoneggpickups]
			= temppickups[gamedata->numprisoneggpickups];
		gamedata->numprisoneggpickups++;
	}

	M_UpdateNextPrisonEggPickup();
}

void M_UpdateNextPrisonEggPickup(void)
{
	UINT16 i, j, swap;

	conditionset_t *c;
	condition_t *cn;

#ifdef DEVELOP
	extern consvar_t cv_debugprisoncd;
#endif

cacheprisoneggpickup:

	// Check if the current roll is fine
	gamedata->thisprisoneggpickup_cached = NULL;
	if (gamedata->thisprisoneggpickup < MAXCONDITIONSETS)
	{
#ifdef DEVELOP
		if (cv_debugprisoncd.value)
			CONS_Printf("CACHE TEST: thisprisoneggpickup is set to %u\n", gamedata->thisprisoneggpickup);
#endif

		if (gamedata->achieved[gamedata->thisprisoneggpickup] == false)
		{
			c = &conditionSets[gamedata->thisprisoneggpickup];
			if (c->numconditions)
			{
				for (j = 0; j < c->numconditions; ++j)
				{
					cn = &c->condition[j];
					if (cn->type != UC_PRISONEGGCD)
						continue;

					if (cn->requirement < nummapheaders && M_MapLocked(cn->requirement+1))
						continue;

					// Good! Attach the cache.
					gamedata->thisprisoneggpickup_cached = cn;

#ifdef DEVELOP
					if (cv_debugprisoncd.value)
						CONS_Printf(" successfully set to cn!\n");
#endif

					break;
				}
			}
		}

		if (gamedata->thisprisoneggpickup_cached == NULL)
		{
			gamedata->thisprisoneggpickup = MAXCONDITIONSETS;
			gamedata->thisprisoneggpickupgrabbed = false;
		}
	}

	if (gamedata->numprisoneggpickups && gamedata->thisprisoneggpickup >= MAXCONDITIONSETS)
	{
#ifdef DEVELOP
		if (cv_debugprisoncd.value)
			CONS_Printf(" Invalid thisprisoneggpickup, rolling a random one...\n");
#endif

		UINT16 gettableprisoneggpickups = 0;

		for (i = 0; i < gamedata->numprisoneggpickups; i++)
		{
			if (gamedata->achieved[gamedata->prisoneggpickups[i]] == false)
			{
				c = &conditionSets[gamedata->prisoneggpickups[i]];
				if (c->numconditions)
				{
					for (j = 0; j < c->numconditions; ++j)
					{
						cn = &c->condition[j];
						if (cn->type != UC_PRISONEGGCD)
							continue;

						// Locked associated map? Keep in the rear end dimension!
						if (cn->requirement < nummapheaders && M_MapLocked(cn->requirement+1))
							break; // not continue intentionally

						// Okay, this should be available.
						// Bring to the front!
						if (i != gettableprisoneggpickups)
						{
							swap = gamedata->prisoneggpickups[gettableprisoneggpickups];
							gamedata->prisoneggpickups[gettableprisoneggpickups] =
								gamedata->prisoneggpickups[i];
							gamedata->prisoneggpickups[i] = swap;
						}

						gettableprisoneggpickups++;

						break;
					}

					if (j < c->numconditions)
						continue;
				}
			}
		}

		if (gettableprisoneggpickups != 0)
		{
			gamedata->thisprisoneggpickup =
				gamedata->prisoneggpickups[
					M_RandomKey(gettableprisoneggpickups)
				];

#ifdef DEVELOP
			if (cv_debugprisoncd.value)
				CONS_Printf(" Selected %u, trying again...\n", gamedata->thisprisoneggpickup);
#endif

			goto cacheprisoneggpickup;
		}
	}

#ifdef DEVELOP
	if (cv_debugprisoncd.value)
		CONS_Printf("thisprisoneggpickup = %u (MAXCONDITIONSETS is %u)\n", gamedata->thisprisoneggpickup, MAXCONDITIONSETS);

#if 0
	// If all drops are collected, just force the first valid one.
	// THIS DOESN'T ACTUALLY WORK IF ALL PRISON PRIZES HAVE BEEN REDEEMED AND THE GAME IS RELAUNCHED, so it is not reliable enough to expose as a debugging tool
	if (cv_debugprisoncd.value && gamedata->thisprisoneggpickup_cached == NULL)
	{
		for (i = 0; gamedata->thisprisoneggpickup_cached == NULL &&
			i < gamedata->numprisoneggpickups; i++)
		{
			c = &conditionSets[gamedata->prisoneggpickups[i]];
			if (c->numconditions)
			{
				for (j = 0; j < c->numconditions; ++j)
				{
					cn = &c->condition[j];
					if (cn->type != UC_PRISONEGGCD)
						continue;

					gamedata->thisprisoneggpickup = gamedata->prisoneggpickups[i];
					gamedata->thisprisoneggpickup_cached = cn;
					break;
				}
			}
		}
	}
#endif

#endif // DEVELOP
}

static void M_PrecacheLevelLocks(void)
{
	UINT16 i, j;

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		switch (unlockables[i].type)
		{
			// SECRET_SKIN, SECRET_COLOR, SECRET_FOLLOWER are instantiated too late to use
			case SECRET_MAP:
			{
				UINT16 map = M_UnlockableMapNum(&unlockables[i]);
				if (map < nummapheaders
					&& mapheaderinfo[map])
				{
					if (mapheaderinfo[map]->cache_maplock != MAXUNLOCKABLES)
						CONS_Alert(CONS_ERROR, "Unlockable %u: Too many SECRET_MAPs associated with Level %s\n", i+1, mapheaderinfo[map]->lumpname);
					mapheaderinfo[map]->cache_maplock = i;
				}
				break;
			}

			case SECRET_ALTMUSIC:
			{
				UINT16 map = M_UnlockableMapNum(&unlockables[i]);
				const char *tempstr = NULL;

				if (map < nummapheaders
					&& mapheaderinfo[map])
				{
					UINT8 greatersize = max(mapheaderinfo[map]->musname_size, mapheaderinfo[map]->encoremusname_size);
					for (j = 1; j < greatersize; j++)
					{
						if (mapheaderinfo[map]->cache_muslock[j - 1] != MAXUNLOCKABLES)
						{
							continue;
						}

						mapheaderinfo[map]->cache_muslock[j - 1] = i;

						UINT8 positionid = 0;

						if (mapheaderinfo[map]->cup)
						{
							for (positionid = 0; positionid < CUPCACHE_PODIUM; positionid++)
							{
								if (mapheaderinfo[map]->cup->cachedlevels[positionid] != map)
									continue;
								break;
							}

							if (positionid < CUPCACHE_PODIUM)
							{
								char prefix = 'R';
								if (positionid >= CUPCACHE_BONUS)
								{
									positionid -= (CUPCACHE_BONUS);
									prefix = 'B';
								}

								tempstr = va(
									"Music: %s CUP %c%u %c",
									mapheaderinfo[map]->cup->realname,
									prefix,
									positionid + 1,
									'A' + j // :D ?
								);
							}
						}

						if (tempstr == NULL)
						{
							UINT16 mapcheck;
							for (mapcheck = 0; mapcheck < map; mapcheck++)
							{
								if (!mapheaderinfo[mapcheck] || mapheaderinfo[mapcheck]->cup != NULL)
									continue;
								if (mapheaderinfo[mapcheck]->menuflags & LF2_HIDEINMENU)
									continue;
								if (((mapheaderinfo[mapcheck]->typeoflevel & TOL_TUTORIAL) == TOL_TUTORIAL)
									!= ((mapheaderinfo[map]->typeoflevel & TOL_TUTORIAL) == TOL_TUTORIAL))
									continue;

								// We don't check for locked, because the levels exist
								positionid++;
							}

							tempstr = va(
								"Music: %s #%u %c",
								(mapheaderinfo[map]->typeoflevel & TOL_TUTORIAL) ? "Tutorial" : "Lost & Found",
								positionid + 1,
								'A' + j // :D ?
							);
						}

						break;
					}
					if (j == greatersize)
						CONS_Alert(CONS_ERROR, "Unlockable %u: Too many SECRET_ALTMUSICs associated with Level %s\n", i+1, mapheaderinfo[map]->lumpname);
				}
				else
				{
					CONS_Alert(CONS_ERROR, "Unlockable %u: Invalid levelname %s for SECRET_ALTMUSIC\n", i+1, unlockables[i].stringVar);
				}

				if (tempstr == NULL)
				{
					tempstr = va("INVALID MUSIC UNLOCK %u", i+1);
				}

				strlcpy(unlockables[i].name, tempstr, sizeof (unlockables[i].name));

				break;
			}

			case SECRET_CUP:
			{
				cupheader_t *cup = M_UnlockableCup(&unlockables[i]);
				if (cup)
				{
					if (cup->cache_cuplock != MAXUNLOCKABLES)
						CONS_Alert(CONS_ERROR, "Unlockable %u: Too many SECRET_CUPs associated with Cup %s\n", i+1, cup->name);
					cup->cache_cuplock = i;
					break;
				}
				break;
			}

			default:
				break;
		}
	}
}

void M_FinaliseGameData(void)
{
	//M_PopulateChallengeGrid(); -- This can be done lazily when we actually need it

	// Precache as many unlockables as is meaningfully feasible
	M_PrecacheLevelLocks();

	// Place the spraycans, which CAN'T be done lazily.
	M_AssignSpraycans();

	// You could probably do the Prison Egg Pickups lazily, but it'd be a lagspike mid-combat.
	M_InitPrisonEggPickups();

	// Don't consider loaded until it's a success!
	// It used to do this much earlier, but this would cause the gamedata
	// to save over itself when it I_Errors from corruption,  which can
	// accidentally delete players' legitimate data if the code ever has
	// any tiny mistakes!
	gamedata->loaded = true;

	// Silent update unlockables in case they're out of sync with conditions
	M_UpdateUnlockablesAndExtraEmblems(false, true);
}

void M_SetNetUnlocked(void)
{
	UINT16 i;

	// Use your gamedata as baseline
	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		netUnlocked[i] = gamedata->unlocked[i];
	}

	if (!dedicated)
	{
		return;
	}

	// Dedicated spoiler password - tournament mode equivalent.
	if (usedTourney)
	{
		for (i = 0; i < MAXUNLOCKABLES; i++)
		{
			if (unlockables[i].conditionset == CH_FURYBIKE)
				continue;

			netUnlocked[i] = true;
		}

		return;
	}

	// Okay, now it's dedicated first-week spoilerless behaviour.
	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (netUnlocked[i])
			continue;

		switch (unlockables[i].type)
		{
			case SECRET_CUP:
			{
				// Give the first seven Cups for free.
				cupheader_t *cup = M_UnlockableCup(&unlockables[i]);
				if (cup && cup->id < 7)
					netUnlocked[i] = true;

				break;
			}
			case SECRET_ADDONS:
			{
				netUnlocked[i] = true;
				break;
			}
			default:
			{
				// Most stuff isn't given to dedis for free
				break;
			}
		}
	}
}

// ----------------------
// Condition set checking
// ----------------------

void M_UpdateConditionSetsPending(void)
{
	UINT32 i, j, k;
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
				case UC_CHARACTERWINS:
				case UCRP_ISCHARACTER:
				case UCRP_MAKERETIRE:
				{
					cn->requirement = R_SkinAvailableEx(cn->stringvar, false);

					if (cn->requirement < 0)
					{
						CONS_Alert(CONS_WARNING, "UC TYPE %u: Invalid character %s for condition ID %d", cn->type, cn->stringvar, cn->id+1);
						continue;
					}

					Z_Free(cn->stringvar);
					cn->stringvar = NULL;

					break;
				}

				case UCRP_HASFOLLOWER:
				{
					// match deh_soc readfollower()
					for (k = 0; cn->stringvar[k]; k++)
					{
						if (cn->stringvar[k] == '_')
							cn->stringvar[k] = ' ';
					}

					cn->requirement = K_FollowerAvailable(cn->stringvar);

					if (cn->requirement < 0)
					{
						CONS_Alert(CONS_WARNING, "UC TYPE %u: Invalid character %s for condition ID %d", cn->type, cn->stringvar, cn->id+1);
						continue;
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

boolean M_NotFreePlay(void)
{
	UINT8 i;
	UINT8 nump = 0;

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

		nump++;

		if (nump > 1)
		{
			return true;
		}
	}

	return false;
}

UINT16 M_CheckCupEmeralds(UINT8 difficulty)
{
	if (difficulty == 0)
		return 0;

	if (difficulty >= KARTGP_MAX)
		difficulty = KARTGP_MASTER;

	cupheader_t *cup;
	UINT16 ret = 0, seen = 0;

	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		// Don't use custom material
		if (cup->id >= basenumkartcupheaders)
			break;

		// Does it not *have* an emerald?
		if (cup->emeraldnum == 0 || cup->emeraldnum > 14)
			continue;

		UINT16 emerald = 1<<(cup->emeraldnum-1);

		// Only count the first reference.
		if (seen & emerald)
			continue;

		// We've seen it, prevent future repetitions.
		seen |= emerald;

		// Did you actually get it?
		if (cup->windata[difficulty].got_emerald == false)
			continue;

		// Wa hoo !
		ret |= emerald;
	}

	return ret;
}

// See also M_GetConditionString
boolean M_CheckCondition(condition_t *cn, player_t *player)
{
	switch (cn->type)
	{
		case UC_NONE:
			return false;
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
		case UC_TOTALTUMBLETIME: // Requires total tumbling time >= x
			return (gamedata->totaltumbletime >= (unsigned)cn->requirement);
		case UC_GAMECLEAR: // Requires game beaten >= x times
			return (gamedata->timesBeaten >= (unsigned)cn->requirement);
		case UC_OVERALLTIME: // Requires overall time <= x
			return (M_GotLowEnoughTime(cn->requirement));
		case UC_MAPVISITED: // Requires map x to be visited
		case UC_MAPBEATEN: // Requires map x to be beaten
		case UC_MAPENCORE: // Requires map x to be beaten in encore
		case UC_MAPSPBATTACK: // Requires map x to be beaten in SPB Attack
		case UC_MAPMYSTICMELODY: // Mystic Melody on map x's Ancient Shrine
		{
			UINT8 mvtype = MV_VISITED;
			if (cn->type == UC_MAPBEATEN)
				mvtype = MV_BEATEN;
			else if (cn->type == UC_MAPENCORE)
				mvtype = MV_ENCORE;
			else if (cn->type == UC_MAPSPBATTACK)
				mvtype = MV_SPBATTACK;
			else if (cn->type == UC_MAPMYSTICMELODY)
				mvtype = MV_MYSTICMELODY;

			return ((cn->requirement < nummapheaders)
				&& (mapheaderinfo[cn->requirement])
				&& ((mapheaderinfo[cn->requirement]->records.mapvisited & mvtype) == mvtype));
		}
		case UC_MAPTIME: // Requires time on map <= x
			return (G_GetBestTime(cn->extrainfo1) <= (unsigned)cn->requirement);

		case UC_CHARACTERWINS:
			if (cn->requirement < 0)
				return false;

			return (skins[cn->requirement]->records.wins >= (UINT32)cn->extrainfo1);

		case UC_ALLCUPRECORDS:
		{
			if (gamestate == GS_LEVEL)
				return false; // this one could be laggy with many cups available

			INT32 requiredid = cn->requirement;
			if (requiredid == -1) // stop at all basegame cup
				requiredid = basenumkartcupheaders;

			UINT8 difficulty = cn->extrainfo2;
			if (difficulty > KARTGP_MASTER)
				difficulty = KARTGP_MASTER;

			cupheader_t *cup;
			for (cup = kartcupheaders; cup; cup = cup->next)
			{
				// Ok, achieved up to the desired cup.
				if (cup->id == requiredid)
					return true;

				cupwindata_t *windata = &cup->windata[difficulty];

				// Did you actually get it?
				if (windata->best_placement == 0)
					return false;

				// Sufficient placement?
				if (cn->extrainfo1 && windata->best_placement > cn->extrainfo1)
					return false;
			}

			// If we ended up here, check we were looking for all cups achieved.
			return (requiredid == basenumkartcupheaders);
		}


		case UC_ALLCHAOS:
		case UC_ALLSUPER:
		case UC_ALLEMERALDS:
		{
			UINT16 ret = 0;

			if (gamestate == GS_LEVEL)
				return false; // this one could be laggy with many cups available

			ret = M_CheckCupEmeralds(cn->requirement);

			if (cn->type == UC_ALLCHAOS)
				return ALLCHAOSEMERALDS(ret);
			if (cn->type == UC_ALLSUPER)
				return ALLSUPEREMERALDS(ret);
			return ALLEMERALDS(ret);
		}

		case UC_TOTALMEDALS: // Requires number of emblems >= x
			return (M_GotEnoughMedals(cn->requirement));
		case UC_EMBLEM: // Requires emblem x to be obtained
		{
			INT32 i = cn->requirement-1;
			if (i >= 0 && i < numemblems && emblemlocations[i].type != ET_NONE)
				return gamedata->collected[cn->requirement-1];
			return false;
		}
		case UC_UNLOCKABLE: // Requires unlockable x to be obtained
			return gamedata->unlocked[cn->requirement-1];
		case UC_CONDITIONSET: // requires condition set x to already be achieved
			return M_Achieved(cn->requirement-1);

		case UC_UNLOCKPERCENT:
		{
			// Don't let netgame sessions intefere
			// or have this give a performance hit
			// (This is formulated this way to
			// perfectly eclipse M_CheckNetUnlockByID)
			if (netgame || demo.playback || Playing())
				return false;

			UINT16 i, unlocked = cn->extrainfo2, total = 0;

			// Special case for maps
			if (cn->extrainfo1 == SECRET_MAP)
			{
				for (i = 0; i < basenummapheaders; i++)
				{
					if (!mapheaderinfo[i] || mapheaderinfo[i]->menuflags & (LF2_HIDEINMENU))
						continue;

					total++;

					// Check for completion
					if ((mapheaderinfo[i]->menuflags & LF2_FINISHNEEDED)
					&& !(mapheaderinfo[i]->records.mapvisited & MV_BEATEN))
						continue;

					// Check for unlock
					if (M_MapLocked(i+1))
						continue;

					unlocked++;
				}
			}
			// Special case for SECRET_COLOR
			else if (cn->extrainfo1 == SECRET_COLOR)
			{
				total = gamedata->numspraycans;
				unlocked = gamedata->gotspraycans;
			}
			// Special case for raw Challenge count
			else if (cn->extrainfo1 == SECRET_NONE)
			{
				for (i = 0; i < MAXUNLOCKABLES; i++)
				{
					if (unlockables[i].type == SECRET_NONE)
						continue;

					total++;

					if (M_Achieved(unlockables[i].conditionset - 1) == false)
						continue;

					unlocked++;
				}
			}
			else
			{
				for (i = 0; i < MAXUNLOCKABLES; i++)
				{
					if (unlockables[i].type != cn->extrainfo1)
						continue;

					total++;

					if (gamedata->unlocked[i] == false)
						continue;

					unlocked++;
				}
			}

			if (!total)
				return false;

			// No need to do a pesky divide
			return ((100 * unlocked) >= (total * cn->requirement));
		}

		case UC_ADDON:
			return ((gamedata->everloadedaddon == true)
				&& M_SecretUnlocked(SECRET_ADDONS, true));
		case UC_CREDITS:
			return (gamedata->everfinishedcredits == true);
		case UC_REPLAY:
			return (gamedata->eversavedreplay == true);
		case UC_CRASH:
			if (gamedata->evercrashed)
			{
				if (gamedata->musicstate < GDMUSIC_LOSERCLUB)
					gamedata->musicstate = GDMUSIC_LOSERCLUB;
				return true;
			}
			return false;
		case UC_TUTORIALSKIP:
			return (gamedata->finishedtutorialchallenge == true);
		case UC_TUTORIALDONE:
			return (gamedata->tutorialdone == true);
		case UC_PLAYGROUND:
			return (gamedata->playgroundroute == true);
		case UC_PASSWORD:
			return (cn->stringvar == NULL);

		case UC_SPRAYCAN:
		{
			if (cn->requirement <= 0
			|| cn->requirement >= numskincolors)
				return false;

			UINT16 can_id = skincolors[cn->requirement].cache_spraycan;

			if (can_id >= gamedata->numspraycans)
				return false;

			return (gamedata->spraycans[can_id].map < nummapheaders);
		}

		case UC_PRISONEGGCD:
			return ((gamedata->thisprisoneggpickupgrabbed == true) && (cn == gamedata->thisprisoneggpickup_cached));

		// Just for string building
		case UC_AND:
		case UC_THEN:
		case UC_COMMA:
		case UC_DESCRIPTIONOVERRIDE:
			return true;

		case UCRP_PREFIX_GRANDPRIX:
			return (grandprixinfo.gp == true);
		case UCRP_PREFIX_BONUSROUND:
			return ((grandprixinfo.gp == true) && (grandprixinfo.eventmode == GPEVENT_BONUS));
		case UCRP_PREFIX_TIMEATTACK:
			return (modeattacking != ATTACKING_NONE && !(skins[player->skin]->flags & SF_HIVOLT));
		case UCRP_PREFIX_PRISONBREAK:
			return ((gametyperules & GTR_PRISONS) && battleprisons);
		case UCRP_PREFIX_SEALEDSTAR:
			return (specialstageinfo.valid == true);

		case UCRP_PREFIX_ISMAP:
		case UCRP_ISMAP:
			return (gamemap == cn->requirement+1);
		case UCRP_ISCHARACTER:
			return (
				player->roundconditions.switched_skin == false
				&& player->skin == cn->requirement
			);
		case UCRP_ISENGINECLASS:
			return (player->roundconditions.switched_skin == false
				&& player->skin < numskins
				&& R_GetEngineClass(
					skins[player->skin]->kartspeed,
					skins[player->skin]->kartweight,
					skins[player->skin]->flags
				) == (unsigned)cn->requirement);
		case UCRP_HASFOLLOWER:
			return (cn->requirement != -1 && player->followerskin == cn->requirement);
		case UCRP_ISDIFFICULTY:
			if (grandprixinfo.gp == false)
				return false;
			if (cn->requirement == KARTGP_MASTER)
				return (grandprixinfo.masterbots == true);
			return (grandprixinfo.gamespeed >= cn->requirement);
		case UCRP_ISGEAR:
			return (gamespeed >= cn->requirement);

		case UCRP_PODIUMCUP:
			if (grandprixinfo.gp == false || K_PodiumSequence() == false)
				return false;
			if (grandprixinfo.cup == NULL
				|| (
					cn->requirement != -1 // Any
					&& grandprixinfo.cup->id != cn->requirement
				)
			)
				return false;
			if (cn->extrainfo2 != 0)
				return (K_PodiumGrade() >= cn->extrainfo1);
			if (cn->extrainfo1 != 0)
				return K_GetPodiumPosition(player) <= cn->extrainfo1;
			return true;
		case UCRP_PODIUMEMERALD:
		case UCRP_PODIUMPRIZE:
			return (grandprixinfo.gp == true
				&& K_PodiumSequence() == true
				&& grandprixinfo.rank.specialWon == true);
		case UCRP_PODIUMNOCONTINUES:
			return (grandprixinfo.gp == true
				&& K_PodiumSequence() == true
				&& grandprixinfo.rank.continuesUsed == 0);

		case UCRP_FINISHCOOL:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay()
				&& !K_IsPlayerLosing(player));
		case UCRP_FINISHPERFECT:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay()
				&& (gamespeed != KARTSPEED_EASY)
				&& (player->tally.active == true)
				&& (player->tally.totalExp > 0) // Only true if not Time Attack
				&& (
					(player->tally.exp >= player->tally.totalExp)
					|| (K_InRaceDuel() && player->duelscore == DUELWINNINGSCORE)
				));
		case UCRP_FINISHALLPRISONS:
			return (battleprisons
				&& !(player->pflags & PF_NOCONTEST)
				//&& M_NotFreePlay()
				&& numtargets >= maptargets);
		case UCRP_SURVIVE:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST));
		case UCRP_NOCONTEST:
			return (player->pflags & PF_NOCONTEST);

		case UCRP_SMASHUFO:
			return (
				specialstageinfo.valid == true
				&& (
					P_MobjWasRemoved(specialstageinfo.ufo)
					|| specialstageinfo.ufo->health <= 1
				)
			);
		case UCRP_CHASEDBYSPB:
			// The PERFECT implementation would check spbplace, iterate over trackercap, etc.
			// But the game already has this handy-dandy SPB signal for us...
			// It's only MAYBE invalid in modded context. And mods can already cheat...
			return ((player->pflags & PF_RINGLOCK) == PF_RINGLOCK);
		case UCRP_MAPDESTROYOBJECTS:
			return (
				gamemap == cn->requirement+1
				&& numchallengedestructibles == UINT16_MAX
			);

		case UCRP_MAKERETIRE:
		{
			// You can't "make" someone retire in coop.
			if (K_Cooperative() == true)
			{
				return false;
			}

			// The following is basically UCRP_FINISHCOOL,
			// but without the M_NotFreePlay check since this
			// condition is already dependent on other players.
			if ((player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& !K_IsPlayerLosing(player)) == false)
			{
				return false;
			}

			UINT8 i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] == false)
				{
					continue;
				}

				// This player is ME!
				if (player == players+i)
				{
					continue;
				}

				// This player didn't NO CONTEST.
				if (!(players[i].pflags & PF_NOCONTEST))
				{
					continue;
				}

				// This player doesn't have the right skin.
				if (players[i].skin != cn->requirement)
				{
					continue;
				}

				// Okay, the right player is dead!
				break;
			}

			return (i != MAXPLAYERS);
		}

		case UCRP_FINISHPLACE:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay()
				&& player->position != 0
				&& player->position <= cn->requirement);
		case UCRP_FINISHPLACEEXACT:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay()
				&& player->position == cn->requirement);
		case UCRP_FINISHGRADE:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& M_NotFreePlay()
				&& (player->tally.active == true)
				&& (player->tally.state >= TALLY_ST_GRADE_APPEAR)
				&& (player->tally.state <= TALLY_ST_DONE)
				&& (player->tally.rank >= cn->requirement));
		case UCRP_FINISHTIME:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& (!battleprisons || numtargets >= maptargets)
				//&& M_NotFreePlay()
				&& player->realtime <= (unsigned)cn->requirement);
		case UCRP_FINISHTIMEEXACT:
			return (player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& (!battleprisons || numtargets >= maptargets)
				//&& M_NotFreePlay()
				&& player->realtime/TICRATE == (unsigned)cn->requirement/TICRATE);
		case UCRP_FINISHTIMELEFT:
			return (timelimitintics
				&& player->exiting
				&& !(player->pflags & PF_NOCONTEST)
				&& (!battleprisons || numtargets >= maptargets)
				&& !K_CanChangeRules(false) // too easy to change cv_timelimit
				&& player->realtime < timelimitintics
				&& (timelimitintics + extratimeintics + secretextratime - player->realtime) >= (unsigned)cn->requirement);

		case UCRP_RINGS:
			return (player->hudrings >= cn->requirement);
		case UCRP_RINGSEXACT:
			return (player->hudrings == cn->requirement);

		case UCRP_SPEEDOMETER:
			return (player->roundconditions.maxspeed >= cn->requirement);
		case UCRP_DRAFTDURATION:
			return (player->roundconditions.continuousdraft_best >= ((tic_t)cn->requirement)*TICRATE);
		case UCRP_GROWCONSECUTIVEBEAMS:
			return (player->roundconditions.best_consecutive_grow_lasers >= cn->requirement);

		case UCRP_TRIGGER: // requires map trigger set
			return !!(player->roundconditions.unlocktriggers & (1 << cn->requirement));

		case UCRP_FALLOFF:
			return ((cn->requirement == 1 || player->exiting || (player->pflags & PF_NOCONTEST))
				&& player->roundconditions.fell_off == (cn->requirement == 1));
		case UCRP_TOUCHOFFROAD:
			return ((cn->requirement == 1 || player->exiting || (player->pflags & PF_NOCONTEST))
				&& player->roundconditions.touched_offroad == (cn->requirement == 1));
		case UCRP_TOUCHSNEAKERPANEL:
			return ((cn->requirement == 1 || player->exiting || (player->pflags & PF_NOCONTEST))
				&& player->roundconditions.touched_sneakerpanel == (cn->requirement == 1));
		case UCRP_RINGDEBT:
			return (!(gametyperules & GTR_SPHERES)
				&& (cn->requirement == 1 || player->exiting || (player->pflags & PF_NOCONTEST))
				&& (player->roundconditions.debt_rings == (cn->requirement == 1)));
		case UCRP_FAULTED:
			return ((cn->requirement == 1 || player->latestlap >= 1)
				&& (player->roundconditions.faulted == (cn->requirement == 1)));

		case UCRP_TRIPWIREHYUU:
			return (player->roundconditions.tripwire_hyuu);
		case UCRP_WHIPHYUU:
			return (player->roundconditions.whip_hyuu);
		case UCRP_SPBNEUTER:
			return (player->roundconditions.spb_neuter);
		case UCRP_LANDMINEDUNK:
			return (player->roundconditions.landmine_dunk);
		case UCRP_HITMIDAIR:
			return (player->roundconditions.hit_midair);
		case UCRP_HITDRAFTERLOOKBACK:
			return (player->roundconditions.hit_drafter_lookback);
		case UCRP_GIANTRACERSHRUNKENORBI:
			return (player->roundconditions.giant_foe_shrunken_orbi);
		case UCRP_RETURNMARKTOSENDER:
			return (player->roundconditions.returntosender_mark);
		case UCRP_ALLANCIENTGEARS:
			return Obj_AllAncientGearsCollected();

		case UCRP_TRACKHAZARD:
		{
			if (!(gametyperules & GTR_CIRCUIT))
			{
				// Prison Break/Versus

				if (!player->exiting && cn->requirement == 0)
					return false;

				return (((player->roundconditions.hittrackhazard[0] & 1) == 1) == (cn->requirement == 1));
			}

			INT16 requiredlap = cn->extrainfo1;

			if (requiredlap < 0)
			{
				// Prevents lowered numlaps from activating it
				// (this also handles exiting, for all-laps situations)
				requiredlap = max(mapheaderinfo[gamemap-1]->numlaps, numlaps);
			}

			// cn->requirement is used as an offset here
			// so if you need to get hit on lap x, the
			// condition can fire while that lap is active
			// but if you need to NOT get hit on lap X,
			// it only fires once the lap is complete
			if (player->latestlap <= (requiredlap - cn->requirement))
				return false;

			UINT8 requiredbit = 1<<(requiredlap & 7);
			requiredlap /= 8;

			if (cn->extrainfo1 == -1)
			{
				if (cn->requirement == 0)
				{
					// The "don't get hit on any lap" check is trivial.
					for (; requiredlap > 0; requiredlap--)
					{
						if (player->roundconditions.hittrackhazard[requiredlap] != 0)
							return false;
					}

					return (player->roundconditions.hittrackhazard[0] == 0);
				}

				// The following is my attempt at a major optimisation.
				// The naive version was MAX_LAP bools, which is ridiculous.

				// Check the highest relevant byte for all necessary bits.
				// We only do this if an == 0xFF/0xFE check wouldn't satisfy.
				if (requiredbit != (1<<7))
				{
					// Last bit MAYBE not needed, POSITION doesn't count.
					const UINT8 finalbit = (requiredlap == 0) ? 1 : 0;
					while (requiredbit != finalbit)
					{
						if (!(player->roundconditions.hittrackhazard[requiredlap] & requiredbit))
							return false;
						requiredbit /= 2;
					}

					if (requiredlap == 0)
						return true;

					requiredlap--;
				}

				// All bytes between the top and the bottom need to be checked for saturation.
				for (; requiredlap > 0; requiredlap--)
				{
					if (player->roundconditions.hittrackhazard[requiredlap] != 0xFF)
						return false;
				}

				// Last bit not needed, POSITION doesn't count.
				return (player->roundconditions.hittrackhazard[0] == 0xFE);
			}

			return (((player->roundconditions.hittrackhazard[requiredlap] & requiredbit) == requiredbit) == (cn->requirement == 1));
		}

		case UCRP_TARGETATTACKMETHOD:
			return (player->roundconditions.targetdamaging == (targetdamaging_t)cn->requirement);

		case UCRP_GACHABOMMISER:
			return (
				player->roundconditions.targetdamaging == UFOD_GACHABOM
				&& player->roundconditions.gachabom_miser != 0xFF
			);

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
		if (cn->type == UC_AND || cn->type == UC_THEN || cn->type == UC_COMMA || cn->type == UC_DESCRIPTIONOVERRIDE)
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

	if ((!(mapheaderinfo[map]->menuflags & LF2_NOVISITNEEDED)
	// the following is intentionally not MV_BEATEN, just in case the title is for "Finish a round on X"
	&& !(mapheaderinfo[map]->records.mapvisited & MV_VISITED))
	|| M_MapLocked(map+1))
		return Z_StrDup("???");

	if (mapheaderinfo[map]->menuttl[0])
	{
		if (mapheaderinfo[map]->typeoflevel & TOL_TUTORIAL)
		{
			// Intentionally not forced uppercase
			return Z_StrDup(va("the %s Tutorial", mapheaderinfo[map]->menuttl));
		}
		title = ref = Z_StrDup(mapheaderinfo[map]->menuttl);
	}
	else
	{
		title = ref = G_BuildMapTitle(map+1);
	}

	if (!title)
		I_Error("M_BuildConditionTitle: out of memory");

	while (*ref != '\0')
	{
		*ref = toupper(*ref);
		ref++;
	}

	return title;
}

static const char *M_GetConditionCharacter(INT32 skin, boolean directlyrequires)
{
	// First we check for direct unlock.
	boolean permitname = R_SkinUsable(-1, skin, false);

	if (permitname == false && directlyrequires == false)
	{
		// If there's no direct unlock, we CAN check for if the
		// character is the Rival of somebody we DO have unlocked...

		UINT8 i, j;
		for (i = 0; i < numskins; i++)
		{
			if (i == skin)
				continue;

			if (R_SkinUsable(-1, i, false) == false)
				continue;

			for (j = 0; j < SKINRIVALS; j++)
			{
				const char *rivalname = skins[i]->rivals[j];
				INT32 rivalnum = R_SkinAvailableEx(rivalname, false);

				if (rivalnum != skin)
					continue;

				// We can see this character as a Rival!
				break;
			}

			if (j == SKINRIVALS)
				continue;

			// "break" our way up the nesting...
			break;
		}

		// We stopped before the end, we can see it!
		if (i != numskins)
			permitname = true;
	}

	return (permitname)
		? skins[skin]->realname
		: "???";
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

	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x
			return va("play the game for %i:%02i:%02i",
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

			return va("clear %d%s Round%s", cn->requirement, work,
				(cn->requirement == 1 ? "" : "s"));

		case UC_TOTALRINGS: // Requires collecting >= x rings
			if (cn->requirement >= 1000000)
				return va("collect %u,%03u,%03u Rings", (cn->requirement/1000000), (cn->requirement/1000)%1000, (cn->requirement%1000));
			if (cn->requirement >= 1000)
				return va("collect %u,%03u Rings", (cn->requirement/1000), (cn->requirement%1000));
			return va("collect %u Rings", cn->requirement);

		case UC_TOTALTUMBLETIME:
			return va("tumble through the air for %i:%02i.%02i",
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement),
				G_TicsToCentiseconds(cn->requirement));

		case UC_GAMECLEAR: // Requires game beaten >= x times
			if (cn->requirement > 1)
				return va("beat the game %d times", cn->requirement);
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
		case UC_MAPMYSTICMELODY: // Mystic Melody on map x's Ancient Shrine
		{
			const char *prefix = "";

			if (cn->requirement >= nummapheaders || !mapheaderinfo[cn->requirement])
				return va("INVALID MAP CONDITION \"%d:%d\"", cn->type, cn->requirement);

			title = M_BuildConditionTitle(cn->requirement);

			if (cn->type == UC_MAPSPBATTACK)
				prefix = (M_SecretUnlocked(SECRET_SPBATTACK, true) ? "SPB ATTACK: " : "???: ");
			else if (cn->type == UC_MAPENCORE)
				prefix = (M_SecretUnlocked(SECRET_ENCORE, true) ? "ENCORE MODE: " : "???: ");

			work = "finish a round on";
			if (cn->type == UC_MAPVISITED)
				work = "visit";
			else if (cn->type == UC_MAPSPBATTACK)
				work = "conquer";
			else if (cn->type == UC_MAPMYSTICMELODY)
				work = "play a melody for the ancient shrine in";

			work = va("%s%s %s",
				prefix,
				work,
				title);
			Z_Free(title);
			return work;
		}

		case UC_MAPTIME: // Requires time on map <= x
		{
			if (cn->extrainfo1 >= nummapheaders || !mapheaderinfo[cn->extrainfo1])
				return va("INVALID MAP CONDITION \"%d:%d:%d\"", cn->type, cn->extrainfo1, cn->requirement);

			title = M_BuildConditionTitle(cn->extrainfo1);
			work = va("beat %s in %i:%02i.%02i", title,
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement),
				G_TicsToCentiseconds(cn->requirement));

			Z_Free(title);
			return work;
		}

		case UC_CHARACTERWINS:
		{
			if (cn->requirement < 0 || !skins[cn->requirement]->realname[0])
				return va("INVALID CHAR CONDITION \"%d:%d:%d\"", cn->type, cn->requirement, cn->extrainfo1);
			work = M_GetConditionCharacter(cn->requirement, true);
			return va("win %d Round%s as %s",
				cn->extrainfo1,
				cn->extrainfo1 == 1 ? "" : "s",
				work);
		}

		case UC_ALLCUPRECORDS:
		{
			const char *completetype = "Complete", *orbetter = "", *specialtext = NULL, *speedtext = "";

			if (cn->extrainfo1 == 0)
				;
			else if (cn->extrainfo1 == 1)
				completetype = "get Gold over";
			else
			{
				if (cn->extrainfo1 == 2)
					completetype = "get Silver";
				else if (cn->extrainfo1 == 3)
					completetype = "get Bronze";
				orbetter = " or better over";
			}

			if (cn->extrainfo2 == KARTSPEED_NORMAL)
			{
				speedtext = " on Intense";
			}
			else if (cn->extrainfo2 == KARTSPEED_HARD)
			{
				speedtext = " on Vicious";
			}
			else if (cn->extrainfo2 == KARTGP_MASTER)
			{
				if (M_SecretUnlocked(SECRET_MASTERMODE, true))
					speedtext = " on Master";
				else
					speedtext = " on ???";
			}

			if (cn->requirement == -1)
				specialtext = "every Cup";
			else if (M_CupSecondRowLocked() == true && cn->requirement+1 >= CUPMENU_COLUMNS)
				specialtext = "the first ??? Cups";

			if (specialtext != NULL)
				return va("GRAND PRIX: %s%s %s%s", completetype, orbetter, specialtext, speedtext);

			return va("GRAND PRIX: %s%s the first %d Cups%s", completetype, orbetter, cn->requirement, speedtext);
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
			else if (M_CupSecondRowLocked() == true)
				return NULL;
			else if (cn->type == UC_ALLSUPER)
				chaostext = "7 Super";
			else
				chaostext = "14";

			/*if (cn->requirement == KARTSPEED_NORMAL) -- Emeralds can not be collected on Easy
			{
				speedtext = " on Normal";
			}
			else*/
			if (cn->requirement == KARTSPEED_HARD)
			{
				speedtext = " on Vicious";
			}
			else if (cn->requirement == KARTGP_MASTER)
			{
				if (M_SecretUnlocked(SECRET_MASTERMODE, true))
					speedtext = " on Master";
				else
					speedtext = " on ???";
			}

			return va("GRAND PRIX: collect all %s Emeralds%s", chaostext, speedtext);
		}

		case UC_TOTALMEDALS: // Requires number of emblems >= x
			return va("get %d Medals", cn->requirement);

		case UC_EMBLEM: // Requires emblem x to be obtained
		{
			INT32 checkLevel = NEXTMAP_INVALID;

			i = cn->requirement-1;

			if (i >= 0 && i < numemblems)
				checkLevel = M_EmblemMapNum(&emblemlocations[i]);

			if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel] || emblemlocations[i].type == ET_NONE)
				return va("INVALID MEDAL MAP \"%d:%d\"", cn->requirement, checkLevel);

			title = M_BuildConditionTitle(checkLevel);
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

		case UC_CONDITIONSET:
			return va("INVALID CN RECURSION \"%d\"", cn->requirement);

		case UC_UNLOCKABLE: // Requires unlockable x to be obtained
			return va("get %s",
				gamedata->unlocked[cn->requirement-1]
				? unlockables[cn->requirement-1].name
				: "???");

		case UC_UNLOCKPERCENT:
		{
			boolean checkavailable = false;

			switch (cn->extrainfo1)
			{
				case SECRET_NONE:
					work = "completion";
					break;
				case SECRET_EXTRAMEDAL:
					work = "of Challenge Medals";
					break;
				case SECRET_CUP:
					work = "of Cups";
					break;
				case SECRET_MAP:
					work = "of Courses";
					break;
				case SECRET_ALTMUSIC:
					work = "of alternate music";
					checkavailable = true;
					break;
				case SECRET_SKIN:
					work = "of Characters";
					checkavailable = true;
					break;
				case SECRET_FOLLOWER:
					work = "of Followers";
					checkavailable = true;
					break;
				case SECRET_COLOR:
					work = (gamedata->gotspraycans == 0) ? "of ???" : "of Spray Cans";
					//checkavailable = true;
					break;
				default:
					return va("INVALID CHALLENGE FOR PERCENT \"%d\"", cn->requirement);
			}

			if (checkavailable == true)
			{
				for (i = 0; i < MAXUNLOCKABLES; ++i)
				{
					if (unlockables[i].type != cn->extrainfo1)
						continue;
					if (gamedata->unlocked[i] == false)
						continue;
					break;
				}

				if (i == MAXUNLOCKABLES)
					work = "of ???";
			}

			return va("CHALLENGES: get %u%% %s", cn->requirement, work);
		}

		case UC_ADDON:
			if (!M_SecretUnlocked(SECRET_ADDONS, true))
				return NULL;
			return "load a custom addon";
		case UC_CREDITS:
			return "watch the developer credits all the way from start to finish";
		case UC_REPLAY:
			return "save a replay after finishing a round";
		case UC_CRASH:
			if (gamedata->evercrashed)
				return "re-launch the game after a crash";
			return NULL;
		case UC_TUTORIALSKIP:
			return "successfully skip the Tutorial";
		case UC_TUTORIALDONE:
			return "complete the Tutorial";
		case UC_PLAYGROUND:
			return "pick the Playground";
		case UC_PASSWORD:
			return "enter a secret password";

		case UC_SPRAYCAN:
		{
			if (cn->requirement <= 0
			|| cn->requirement >= numskincolors)
				return va("INVALID SPRAYCAN COLOR \"%d\"", cn->requirement);

			UINT16 can_id = skincolors[cn->requirement].cache_spraycan;

			if (can_id >= gamedata->numspraycans)
				return va("INVALID SPRAYCAN ID \"%d:%u\"",
					cn->requirement,
					skincolors[cn->requirement].cache_spraycan
				);

			if (can_id == 0)
				return "grab a Spray Can"; // Special case for the head of the list

			if (gamedata->spraycans[0].map >= nummapheaders)
				return NULL; // Don't tease that there are many until you have one

			return va("grab %d Spray Cans", can_id + 1);
		}

		case UC_PRISONEGGCD:
			// :butterfly: "alternatively you could say 'grab a hot toooon' or 'smooth beeat'"
			return "GRAND PRIX: grab a certain prize from a random Prison Egg";

		case UC_AND:
			return "&";
		case UC_THEN:
			return "then";
		case UC_COMMA:
			return ",";
		case UC_DESCRIPTIONOVERRIDE:
			return cn->stringvar;

		case UCRP_PREFIX_BONUSROUND:
			//return "BONUS ROUND:"; -- our final testers bounced off this, just fallthru to GRAND PRIX instead
		case UCRP_PREFIX_GRANDPRIX:
			return "GRAND PRIX:";
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

			title = M_BuildConditionTitle(cn->requirement);
			work = va("%s:", title);
			Z_Free(title);
			return work;
		case UCRP_ISMAP:
			if (cn->requirement >= nummapheaders || !mapheaderinfo[cn->requirement])
				return va("INVALID MAP CONDITION \"%d:%d\"", cn->type, cn->requirement);

			title = M_BuildConditionTitle(cn->requirement);
			work = va("on %s", title);
			Z_Free(title);
			return work;
		case UCRP_ISCHARACTER:
			if (cn->requirement < 0 || !skins[cn->requirement]->realname[0])
				return va("INVALID CHAR CONDITION \"%d:%d\"", cn->type, cn->requirement);
			work = M_GetConditionCharacter(cn->requirement, true);
			return va("as %s", work);
		case UCRP_ISENGINECLASS:
			return va("with engine class %c", 'A' + cn->requirement);
		case UCRP_HASFOLLOWER:
			if (cn->requirement < 0 || !followers[cn->requirement].name[0])
				return va("INVALID FOLLOWER CONDITION \"%d:%d\"", cn->type, cn->requirement);
			work = (K_FollowerUsable(cn->requirement))
				? followers[cn->requirement].name
				: "???";
			return va("with %s in tow", work);
		case UCRP_ISDIFFICULTY:
		{
			const char *speedtext = "";

			if (cn->requirement == KARTSPEED_NORMAL)
			{
				speedtext = "on Intense";
			}
			else if (cn->requirement == KARTSPEED_HARD)
			{
				speedtext = "on Vicious";
			}
			else if (cn->requirement == KARTGP_MASTER)
			{
				if (M_SecretUnlocked(SECRET_MASTERMODE, true))
					speedtext = "on Master";
				else
					speedtext = "on ???";
			}

			return speedtext;
		}
		case UCRP_ISGEAR:
			return va("in Gear %d", cn->requirement + 1);

		case UCRP_PODIUMCUP:
		{
			cupheader_t *cup;
			const char *completetype = "complete", *orbetter = "";

			if (cn->extrainfo2)
			{
				switch (cn->extrainfo1)
				{
					case GRADE_E: { completetype = "get grade E"; break; }
					case GRADE_D: { completetype = "get grade D"; break; }
					case GRADE_C: { completetype = "get grade C"; break; }
					case GRADE_B: { completetype = "get grade B"; break; }
					case GRADE_A: { completetype = "get grade A"; break; }
					case GRADE_S: { completetype = "get grade S"; break; }
					default: { break; }
				}

				if (cn->extrainfo1 < GRADE_S)
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

			if (cn->requirement == -1)
			{
				return va("%s%s any Cup",
					completetype, orbetter
				);
			}

			for (cup = kartcupheaders; cup; cup = cup->next)
			{
				if (cup->id != cn->requirement)
					continue;
				return va("%s%s %s CUP",
					completetype, orbetter,
					(M_CupLocked(cup) ? "???" : cup->realname)
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
		case UCRP_PODIUMNOCONTINUES:
			return "without using any continues";

		case UCRP_FINISHCOOL:
			return "finish in good standing";
		case UCRP_FINISHPERFECT:
			return "finish a perfect round (excluding Gear 1)";
		case UCRP_FINISHALLPRISONS:
			return "break every Prison Egg";
		case UCRP_SURVIVE:
			return "survive";
		case UCRP_NOCONTEST:
			return "NO CONTEST";

		case UCRP_SMASHUFO:
			if (!gamedata->everseenspecial)
				return NULL;
			return "smash the UFO Catcher";
		case UCRP_CHASEDBYSPB:
			return "while chased by a Self-Propelled Bomb";
		case UCRP_MAPDESTROYOBJECTS:
		{
			if (cn->stringvar == NULL)
				return va("INVALID DESTROY CONDITION \"%d\"", cn->type);

			title = M_BuildConditionTitle(cn->requirement);
			work = va("%s: destroy all the %s", title, cn->stringvar);
			Z_Free(title);
			return work;
		}

		case UCRP_MAKERETIRE:
		{
			if (cn->requirement < 0 || !skins[cn->requirement]->realname[0])
				return va("INVALID CHAR CONDITION \"%d:%d\"", cn->type, cn->requirement);

			work = M_GetConditionCharacter(cn->requirement, false);
			return va("make %s retire", work);
		}

		case UCRP_FINISHPLACE:
		case UCRP_FINISHPLACEEXACT:
			return va("finish in %d%s%s", cn->requirement, M_GetNthType(cn->requirement),
				((cn->type == UCRP_FINISHPLACE && cn->requirement > 1)
					? " or better" : ""));
		case UCRP_FINISHGRADE:
		{
			char gradeletter = '?';
			const char *orbetter = "";

			switch (cn->requirement)
			{
				case GRADE_E: { gradeletter = 'E'; break; }
				case GRADE_D: { gradeletter = 'D'; break; }
				case GRADE_C: { gradeletter = 'C'; break; }
				case GRADE_B: { gradeletter = 'B'; break; }
				case GRADE_A: { gradeletter = 'A'; break; }
				default: { break; }
			}

			if (cn->requirement < GRADE_A)
				orbetter = " or better";

			return va("get grade %c%s",
				gradeletter, orbetter
			);
		}
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

		case UCRP_RINGS:
			if (cn->requirement != 20)
				return va("with at least %d Rings", cn->requirement);
			// FALLTHRU
		case UCRP_RINGSEXACT:
			return va("with exactly %d Rings", cn->requirement);

		case UCRP_SPEEDOMETER:
			return va("reach %s%u%% on the speedometer",
				(cn->requirement == 999)
					? "" : "at least ",
				cn->requirement
			);
		case UCRP_DRAFTDURATION:
			return va("consistently tether off other racers for %u seconds", cn->requirement);
		case UCRP_GROWCONSECUTIVEBEAMS:
			return va("touch the blue beams from your own Shrink at least %u times before returning to normal size", cn->requirement);

		case UCRP_TRIGGER:
			return "do something special";

		case UCRP_FALLOFF:
			return (cn->requirement == 1) ? "fall off the course" : "don't fall off the course";
		case UCRP_TOUCHOFFROAD:
			return (cn->requirement == 1) ? "touch offroad" : "don't touch any offroad";
		case UCRP_TOUCHSNEAKERPANEL:
			return (cn->requirement == 1) ? "touch a Sneaker Panel" : "don't touch any Sneaker Panels";
		case UCRP_RINGDEBT:
			return (cn->requirement == 1) ? "go into Ring debt" : "don't go into Ring debt";
		case UCRP_FAULTED:
			return (cn->requirement == 1) ? "FAULT during POSITION" : "don't FAULT during POSITION";

		case UCRP_TRIPWIREHYUU:
			return "go through Tripwire while afflicted by Hyudoro";
		case UCRP_WHIPHYUU:
			return "Insta-Whip a racer while afflicted by Hyudoro";
		case UCRP_SPBNEUTER:
			return "shock a Self-Propelled Bomb into submission";
		case UCRP_LANDMINEDUNK:
			return "dunk a Land Mine on another racer's head";
		case UCRP_HITMIDAIR:
			return "hit another racer with a projectile while you're both in the air";
		case UCRP_HITDRAFTERLOOKBACK:
			return "hit a racer tethering off you while looking back at them";
		case UCRP_GIANTRACERSHRUNKENORBI:
			return "hit a giant racer with a shrunken Orbinaut";
		case UCRP_RETURNMARKTOSENDER:
			return "when cursed with Eggmark, blow up the racer responsible";
		case UCRP_ALLANCIENTGEARS:
			return "collect all Ancient Gears";

		case UCRP_TRACKHAZARD:
		{
			work = (cn->requirement == 1) ? "touch a course hazard" : "don't touch any course hazards";
			if (cn->extrainfo1 == -1)
				return va("%s%s", work, (cn->requirement == 1) ? " on every lap" : "");
			if (cn->extrainfo1 == -2)
				return va("%s on the final lap", work);
			if (cn->extrainfo1 == 0)
				return va("%s during POSITION", work);
			return va("%s on lap %u", work, cn->extrainfo1);
		}

		case UCRP_TARGETATTACKMETHOD:
		{
			work = NULL;

			switch (cn->requirement)
			{
				// See targetdamaging_t
				case UFOD_BOOST:
					work = "boost power";
					break;
				case UFOD_WHIP:
					work = "Insta-Whip";
					break;
				case UFOD_BANANA:
					work = "Bananas";
					break;
				case UFOD_ORBINAUT:
					work = "Orbinauts";
					break;
				case UFOD_JAWZ:
					work = "Jawz";
					break;
				case UFOD_SPB:
					work = "Self-Propelled Bombs";
					break;
				case UFOD_GACHABOM:
					work = "Gachabom";
					break;
				default:
					break;
			}

			if (work == NULL)
				return va("INVALID ATTACK CONDITION \"%d:%d\"", cn->type, cn->requirement);

			return va("using only %s", work);
		}

		case UCRP_GACHABOMMISER:
			return "using exactly one Gachabom repeatedly";

		case UCRP_WETPLAYER:
			return va("without %s %s",
				(cn->requirement & MFE_TOUCHWATER) ? "touching any" : "going into",
				(cn->stringvar) ? cn->stringvar : "water");

		default:
			break;
	}
	// UC_MAPTRIGGER and the like are explicitly very hard to support proper descriptions for
	return va("UNSUPPORTED CONDITION \"%d\"", cn->type);
}

char *M_BuildConditionSetString(UINT16 unlockid)
{
	condition_t *cn;
	size_t len = 1024, worklen;
	static char message[1024] = "";
	const char *work = NULL;
	size_t i;

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

	struct conditionset_traverser_s {
		conditionset_t *c;
		size_t i;
		UINT32 lastID;
		UINT8 stopasap;
	};

	struct conditionset_traverser_s current = {0};
	current.c = &conditionSets[unlockables[unlockid].conditionset-1];
	current.i = current.lastID = current.stopasap = 0;

	struct conditionset_traverser_s restore = {0};
	restore.c = NULL;

	do {
		if (current.stopasap == UINT8_MAX)
		{
			// Sentinel value for recursion
			current.stopasap = 0;
		}
		else if (restore.c)
		{
			// De-recur
			current = restore;
			current.i++;

			restore.c = NULL;
		}

		for (; current.i < current.c->numconditions; ++current.i)
		{
			cn = &current.c->condition[current.i];

			if (current.i > 0)
			{
				worklen = 0;
				if (current.lastID != cn->id)
				{
					current.stopasap = 0;
					worklen = 6;
					strncat(message, " - OR ", len);
				}
				else if (current.stopasap == 0 && cn->type != UC_COMMA)
				{
					worklen = 1;
					strncat(message, " ", len);
				}
				len -= worklen;
			}

			current.lastID = cn->id;

			if (current.stopasap == 1)
			{
				// Secret challenge -- show unrelated condition IDs
				continue;
			}

			if (cn->type == UC_CONDITIONSET
			&& restore.c == NULL
			&& cn->requirement
			&& cn->requirement <= MAXCONDITIONSETS)
			{
				// Conditionset description! Can only recursion once at a time
				restore = current;
				current.c = &conditionSets[cn->requirement-1];
				current.i = current.lastID = 0;
				current.stopasap = UINT8_MAX;
				break;
			}

			work = M_GetConditionString(cn);
			if (work == NULL)
			{
				current.stopasap = 1;
				if (message[0] && message[1])
					work = "???";
				else
					work = "(Find other secrets to learn about this...)";
			}
			else if (cn->type == UC_DESCRIPTIONOVERRIDE)
			{
				current.stopasap = 2;
			}
			worklen = strlen(work);

			strncat(message, work, len);
			len -= worklen;

			if (current.stopasap == 2)
			{
				// Description override - hide all further ones
				break;
			}
		}
	} while (restore.c);

	if (message[0] == '\0')
	{
		return NULL;
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

		// Okay, now make the first non-whitespace character after this a capital.
		// Doesn't matter if !isalpha() - toupper is a no-op.
		// (If the first loop hit the string's end, the message[i] check keeps us safe)
		for (; message[i]; i++)
		{
			if ((message[i] & 0x80) || isspace(message[i]))
				continue;
			message[i] = toupper(message[i]);
			break;
		}

		// Also do this for the prefix.
		// This might seem redundant, but "the Controls Tutorial:" is a possible prefix!
		for (i = 0; message[i]; i++)
		{
			if ((message[i] & 0x80) || isspace(message[i]))
				continue;
			message[i] = toupper(message[i]);
			break;
		}
	}

	if (usedTourney && unlockables[unlockid].conditionset == CH_FURYBIKE && gamedata->unlocked[unlockid] == false)
	{
		strcpy(message, "Power shrouds this challenge in darkness... (Return here without Tournament Mode!)\0");
	}

	// Finally, do a clean wordwrap!
	return V_ScaledWordWrap(
		DESCRIPTIONWIDTH << FRACBITS,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0,
		TINY_FONT,
		message
	);
}

static boolean M_CheckUnlockConditions(player_t *player)
{
	UINT32 i;
	conditionset_t *c;
	boolean ret = false;

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

		M_UpdateNextPrisonEggPickup();

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

	if (!demo.playback && Playing() && (gamestate == GS_LEVEL || K_PodiumSequence() == true))
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

	// Announce
	if (response != 0)
	{
		if (loud)
		{
			S_StartSound(NULL, sfx_achiev);
		}
		return true;
	}

	if (newkeys != 0)
	{
		if (loud)
		{
			S_StartSound(NULL, sfx_keygen);
		}
		return true;
	}

	return false;
}

UINT16 M_GetNextAchievedUnlock(boolean canskipchaokeys)
{
	UINT16 i;

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (!unlockables[i].conditionset)
		{
			// Not worthy of consideration
			continue;
		}

		if (gamedata->unlocked[i] == true)
		{
			// Already unlocked, no need to engage
			continue;
		}

		if (gamedata->unlockpending[i] == false)
		{
			// Not unlocked AND not pending, which means chao keys can be used on something
			canskipchaokeys = false;
			continue;
		}

		return i;
	}

	if (canskipchaokeys == true)
	{
		// Okay, we're skipping chao keys - let's just insta-digest them.

		if (gamedata->chaokeys + gamedata->keyspending < GDMAX_CHAOKEYS)
		{
			gamedata->chaokeys += gamedata->keyspending;
			gamedata->pendingkeyroundoffset =
				(gamedata->pendingkeyroundoffset + gamedata->pendingkeyrounds)
				% GDCONVERT_ROUNDSTOKEY;

		}
		else
		{
			gamedata->chaokeys = GDMAX_CHAOKEYS;
			gamedata->pendingkeyroundoffset = 0;
		}

		gamedata->keyspending = 0;
		gamedata->pendingkeyrounds = 0;
	}
	else if (gamedata->keyspending != 0)
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
				else if (tag < 0 && tag > AUTOMEDAL_MAX)
				{
					// Use auto medal times for emblem tags, see AUTOMEDAL_ in m_cond.h
					int index = -tag - 1; // 0 is Platinum, 3 is Bronze
					tic_t time = mapheaderinfo[checkLevel]->automedaltime[index];

					res = (G_GetBestTime(levelnum) <= time);
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

boolean M_GameTrulyStarted(void)
{
	// Fail safe
	if (gamedata == NULL)
		return false;

	// Not set
	if (gamestartchallenge >= MAXUNLOCKABLES)
		return true;

	// An unfortunate sidestep, but sync is important.
	if (netgame)
		return true;

	// Okay, we can check to see if this challenge has been achieved.
	/*return (
		gamedata->unlockpending[gamestartchallenge]
		|| gamedata->unlocked[gamestartchallenge]
	);*/
	// Actually, on second thought, let's let the Goner Setup play one last time
	// The above is used in M_StartControlPanel instead
	return (gamedata->gonerlevel == GDGONER_DONE);
}

boolean M_GameAboutToStart(void)
{
	// Fail safe
	if (gamedata == NULL)
		return false;

	// Not set
	if (gamestartchallenge >= MAXUNLOCKABLES)
		return true;

	// Pending unlocked, but not unlocked
	return (
		gamedata->unlockpending[gamestartchallenge]
		&& !gamedata->unlocked[gamestartchallenge]
	);
}

boolean M_CheckNetUnlockByID(UINT16 unlockid)
{
	if (unlockid >= MAXUNLOCKABLES
		|| !unlockables[unlockid].conditionset)
	{
		return true; // default permit
	}

	if (netgame || demo.playback)
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
	// No skipping over any part of your marathon.
	if (marathonmode)
		return false;

	if (!cup)
		return false;

#if 0 // perfect uncached behaviour
	UINT16 i;

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != SECRET_CUP)
			continue;
		if (M_UnlockableCup(&unlockables[i]) != cup)
			continue;
		return !M_CheckNetUnlockByID(i);
	}
#else
	if (cup->cache_cuplock < MAXUNLOCKABLES)
		return !M_CheckNetUnlockByID(cup->cache_cuplock);
#endif

	return false;
}

boolean M_CupSecondRowLocked(void)
{
	// The following was pre-optimised for cached behaviour.
	// It would need a refactor if the cache system were to
	// change, maybe to iterate over unlockable_t instead.
	cupheader_t *cup;
	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		// Only important for the second row.
		if ((cup->id % (CUPMENU_COLUMNS * CUPMENU_ROWS)) < CUPMENU_COLUMNS)
			continue;

		// Only important for ones that can be locked.
		if (cup->cache_cuplock == MAXUNLOCKABLES)
			continue;

		// If it's NOT unlocked, can't be used as proof of unlock.
		if (!M_CheckNetUnlockByID(cup->cache_cuplock))
			continue;

		// Okay, at least one cup on the second row is unlocked!
		return false;
	}

	return true;
}

boolean M_MapLocked(UINT16 mapnum)
{
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

#if 0 // perfect uncached behaviour
	UINT16 i;

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type != SECRET_MAP)
			continue;
		if (M_UnlockableMapNum(&unlockables[i]) != mapnum-1)
			continue;
		return !M_CheckNetUnlockByID(i);
	}
#else
	if (mapheaderinfo[mapnum-1]->cache_maplock < MAXUNLOCKABLES)
		return !M_CheckNetUnlockByID(mapheaderinfo[mapnum-1]->cache_maplock);
#endif

	return false;
}

INT32 M_CountMedals(boolean all, boolean extraonly)
{
	INT32 found = 0, i;
	if (!extraonly)
	{
		for (i = 0; i < numemblems; ++i)
		{
			// Not init in SOC
			if (emblemlocations[i].type == ET_NONE)
				continue;

			// Not explicitly a medal
			if ((emblemlocations[i].type == ET_GLOBAL)
				&& (emblemlocations[i].flags & GE_NOTMEDAL))
				continue;

			// Not getting the counter, and not collected
			if (!all && !gamedata->collected[i])
				continue;

			// Don't count Platinums in the overall count, so you can get 101% going for them
			if (all
				&& (emblemlocations[i].type == ET_TIME)
				&& (emblemlocations[i].tag == AUTOMEDAL_PLATINUM))
				continue;

			// Relevant, add to da counter
			found++;
		}
	}

	// Above but for extramedals
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
		// Not init in SOC
		if (emblemlocations[i].type == ET_NONE)
			continue;

		// Not explicitly a medal
		if ((emblemlocations[i].type == ET_GLOBAL)
			&& (emblemlocations[i].flags & GE_NOTMEDAL))
			continue;

		// Not collected
		if (!gamedata->collected[i])
			continue;

		// Add to counter. Hit our threshold?
		if (++gottenmedals < number)
			continue;

		// We did!
		return true;
	}

	// Above but for extramedals
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

	// Didn't hit our counter!
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

		if (!mapheaderinfo[i]->records.timeattack.time)
			return false;
		if ((curtics += mapheaderinfo[i]->records.timeattack.time) > tictime)
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
		skinnum = R_SkinAvailableEx(unlock->stringVar, false);
		if (skinnum != -1)
		{
			if (skinnum >= MAXSKINUNAVAILABLE)
			{
				CONS_Alert(CONS_WARNING,"Unlockable ID %s: Skin %s (id %d) is greater than %u, and will not be locked in this session.", sizeu1((unlock-unlockables)+1), unlock->stringVar, skinnum, MAXSKINUNAVAILABLE);
			}

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

INT32 M_UnlockableColorNum(unlockable_t *unlock)
{
	if (unlock->type != SECRET_COLOR)
	{
		// This isn't a color unlockable...
		return -1;
	}

	if (unlock->stringVar && unlock->stringVar[0])
	{
		skincolornum_t colornum = SKINCOLOR_NONE;

		if (unlock->stringVarCache != -1)
		{
			return unlock->stringVarCache;
		}

		// Get the skin from the string.
		colornum = R_GetColorByName(unlock->stringVar);
		if (colornum != SKINCOLOR_NONE)
		{
			unlock->stringVarCache = colornum;
			return colornum;
		}
	}

	if (unlock->variable > SKINCOLOR_NONE && unlock->variable < numskincolors)
	{
		// Use the number directly.
		return unlock->variable;
	}

	// Invalid color unlockable.
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
	if (unlock->type != SECRET_MAP && unlock->type != SECRET_ALTMUSIC)
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
	if (emblem->levelCache == NEXTMAP_INVALID && emblem->level)
	{
		UINT16 result = G_MapNumber(emblem->level);

		if (result >= nummapheaders)
			return result;

		emblem->levelCache = result;
		Z_Free(emblem->level);
		emblem->level = NULL;
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
		if (emblemlocations[i].type == ET_NONE)
			continue;

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

boolean M_UseAlternateTitleScreen(void)
{
	extern consvar_t cv_alttitle;
	return cv_alttitle.value && M_SecretUnlocked(SECRET_ALTTITLE, true);
}

INT32 M_GameDataGameType(INT32 lgametype, boolean lbattleprisons)
{
	INT32 playtimemode = GDGT_CUSTOM;
	if (lgametype == GT_RACE)
		playtimemode = GDGT_RACE;
	else if (lgametype == GT_BATTLE)
		playtimemode = lbattleprisons ? GDGT_PRISONS : GDGT_BATTLE;
	else if (lgametype == GT_SPECIAL || lgametype == GT_VERSUS)
		playtimemode = GDGT_SPECIAL;

	return playtimemode;
}

