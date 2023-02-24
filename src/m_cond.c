// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Vivian "toaster" Grannell.
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

#include "k_pwrlv.h"
#include "k_profiles.h"

gamedata_t *gamedata = NULL;
boolean netUnlocked[MAXUNLOCKABLES];

// Map triggers for linedef executors
// 32 triggers, one bit each
UINT32 unlocktriggers;

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
	UINT8 selection[2][MAXUNLOCKABLES + (CHALLENGEGRIDHEIGHT-1)];
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
		UINT8 majorpad = (CHALLENGEGRIDHEIGHT/2);
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
		(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT * sizeof(UINT8)),
		PU_STATIC, NULL);

	if (!gamedata->challengegrid)
	{
		I_Error("M_PopulateChallengeGrid: was not able to allocate grid");
	}

	memset(gamedata->challengegrid,
		MAXUNLOCKABLES,
		(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT * sizeof(UINT8)));

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

		if (nummajorunlocks > 0)
		{
			I_Error("M_PopulateChallengeGrid: was not able to populate %d large tiles (width %d)", nummajorunlocks, gamedata->challengegridwidth);
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
	UINT8 i, j, num, id, tempid, work;
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

void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2)
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
}

void M_ClearConditionSet(UINT8 set)
{
	if (conditionSets[set].numconditions)
	{
		Z_Free(conditionSets[set].condition);
		conditionSets[set].condition = NULL;
		conditionSets[set].numconditions = 0;
	}
	gamedata->achieved[set] = false;
}

// Clear ALL secrets.
void M_ClearSecrets(void)
{
	INT32 i;

	for (i = 0; i < nummapheaders; ++i)
	{
		mapheaderinfo[i]->mapvisited = 0;
	}

	for (i = 0; i < MAXEMBLEMS; ++i)
		gamedata->collected[i] = false;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
		gamedata->unlocked[i] = gamedata->unlockpending[i] = netUnlocked[i] = false;
	for (i = 0; i < MAXCONDITIONSETS; ++i)
		gamedata->achieved[i] = false;

	Z_Free(gamedata->challengegrid);
	gamedata->challengegrid = NULL;
	gamedata->challengegridwidth = 0;

	gamedata->timesBeaten = 0;

	// Re-unlock any always unlocked things
	M_UpdateUnlockablesAndExtraEmblems(false);
}

// ----------------------
// Condition set checking
// ----------------------

// See also M_GetConditionString
UINT8 M_CheckCondition(condition_t *cn)
{
	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x
			return (gamedata->totalplaytime >= (unsigned)cn->requirement);
		case UC_MATCHESPLAYED: // Requires any level completed >= x times
			return (gamedata->matchesplayed >= (unsigned)cn->requirement);
		case UC_POWERLEVEL: // Requires power level >= x on a certain gametype
		{
			UINT8 i;
			for (i = PROFILE_GUEST; i < PR_GetNumProfiles(); i++)
			{
				profile_t *p = PR_GetProfile(i);

				if (p->powerlevels[cn->extrainfo1] >= (unsigned)cn->requirement)
				{
					return true;
				}
			}

			return false;
		}
		case UC_GAMECLEAR: // Requires game beaten >= x times
			return (gamedata->timesBeaten >= (unsigned)cn->requirement);
		case UC_OVERALLTIME: // Requires overall time <= x
			return (M_GotLowEnoughTime(cn->requirement));
		case UC_MAPVISITED: // Requires map x to be visited
		case UC_MAPBEATEN: // Requires map x to be beaten
		case UC_MAPENCORE: // Requires map x to be beaten in encore
		{
			UINT8 mvtype = MV_VISITED;
			if (cn->type == UC_MAPBEATEN)
				mvtype = MV_BEATEN;
			else if (cn->type == UC_MAPENCORE)
				mvtype = MV_ENCORE;

			return ((cn->requirement < nummapheaders)
				&& (mapheaderinfo[cn->requirement])
				&& ((mapheaderinfo[cn->requirement]->mapvisited & mvtype) == mvtype));
		}
		case UC_MAPTIME: // Requires time on map <= x
			return (G_GetBestTime(cn->extrainfo1) <= (unsigned)cn->requirement);
		case UC_TRIGGER: // requires map trigger set
			return !!(unlocktriggers & (1 << cn->requirement));
		case UC_TOTALMEDALS: // Requires number of emblems >= x
			return (M_GotEnoughMedals(cn->requirement));
		case UC_EMBLEM: // Requires emblem x to be obtained
			return gamedata->collected[cn->requirement-1];
		case UC_UNLOCKABLE: // Requires unlockable x to be obtained
			return gamedata->unlocked[cn->requirement-1];
		case UC_CONDITIONSET: // requires condition set x to already be achieved
			return M_Achieved(cn->requirement-1);
	}
	return false;
}

static UINT8 M_CheckConditionSet(conditionset_t *c)
{
	UINT32 i;
	UINT32 lastID = 0;
	condition_t *cn;
	UINT8 achievedSoFar = true;

	for (i = 0; i < c->numconditions; ++i)
	{
		cn = &c->condition[i];

		// If the ID is changed and all previous statements of the same ID were true
		// then this condition has been successfully achieved
		if (lastID && lastID != cn->id && achievedSoFar)
			return true;

		// Skip future conditions with the same ID if one fails, for obvious reasons
		else if (lastID && lastID == cn->id && !achievedSoFar)
			continue;

		lastID = cn->id;
		achievedSoFar = M_CheckCondition(cn);
	}

	return achievedSoFar;
}

static char *M_BuildConditionTitle(UINT16 map)
{
	char *title, *ref;

	if (((mapheaderinfo[map]->menuflags & LF2_FINISHNEEDED)
	// the following is intentionally not MV_BEATEN, just in case the title is for "Finish a round on X"
	&& !(mapheaderinfo[map]->mapvisited & MV_VISITED))
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

// See also M_CheckCondition
static const char *M_GetConditionString(condition_t *cn)
{
	INT32 i;
	char *title = NULL;
	const char *work = NULL;

#define BUILDCONDITIONTITLE(i) (M_BuildConditionTitle(i))

	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x
			return va("Play for %i:%02i:%02i",
				G_TicsToHours(cn->requirement),
				G_TicsToMinutes(cn->requirement, false),
				G_TicsToSeconds(cn->requirement));
		case UC_MATCHESPLAYED: // Requires any level completed >= x times
			return va("Play %d matches", cn->requirement);
		case UC_POWERLEVEL: // Requires power level >= x on a certain gametype
			return va("Get a PWR of %d in %s", cn->requirement,
				(cn->extrainfo1 == PWRLV_RACE)
				? "Race"
				: "Battle");
		case UC_GAMECLEAR: // Requires game beaten >= x times
			if (cn->requirement > 1)
				return va("Beat game %d times", cn->requirement);
			else
				return va("Beat the game");
		case UC_OVERALLTIME: // Requires overall time <= x
			return va("Get overall time of %i:%02i:%02i",
				G_TicsToHours(cn->requirement),
				G_TicsToMinutes(cn->requirement, false),
				G_TicsToSeconds(cn->requirement));
		case UC_MAPVISITED: // Requires map x to be visited
		case UC_MAPBEATEN: // Requires map x to be beaten
		case UC_MAPENCORE: // Requires map x to be beaten in encore
		{
			if (cn->requirement >= nummapheaders || !mapheaderinfo[cn->requirement])
				return va("INVALID MAP CONDITION \"%d:%d\"", cn->type, cn->requirement);

			title = BUILDCONDITIONTITLE(cn->requirement);
			work = va("%s %s%s",
				(cn->type == UC_MAPVISITED) ? "Visit" : "Finish a round on",
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
			work = va("Beat %s in %i:%02i.%02i", title,
				G_TicsToMinutes(cn->requirement, true),
				G_TicsToSeconds(cn->requirement),
				G_TicsToCentiseconds(cn->requirement));

			Z_Free(title);
			return work;
		}
		case UC_TOTALMEDALS: // Requires number of emblems >= x
			return va("Get %d medals", cn->requirement);
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
					work = va("Beat %s", title);
					break;
				case ET_TIME:
					if (emblemlocations[i].color <= 0 || emblemlocations[i].color >= numskincolors)
					{
						Z_Free(title);
						return va("INVALID MEDAL COLOR \"%d:%d\"", cn->requirement, checkLevel);
					}
					work = va("Get the %s Medal for %s", skincolors[emblemlocations[i].color].name, title);
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
						work = va("Find %s%s%s in %s before %i:%02i.%02i",
							astr, colorstr, medalstr, title,
							G_TicsToMinutes(emblemlocations[i].var, true),
							G_TicsToSeconds(emblemlocations[i].var),
							G_TicsToCentiseconds(emblemlocations[i].var));
					}
					else
					{
						work = va("Find %s%s%s in %s",
							astr, colorstr, medalstr, title);
					}
					break;
				}
				default:
					work = va("Find a secret in %s", title);
					break;
			}

			Z_Free(title);
			return work;
		}
		case UC_UNLOCKABLE: // Requires unlockable x to be obtained
			return va("Get \"%s\"",
				gamedata->unlocked[cn->requirement-1]
				? unlockables[cn->requirement-1].name
				: "???");
		default:
			break;
	}
	// UC_MAPTRIGGER and UC_CONDITIONSET are explicitly very hard to support proper descriptions for
	return va("UNSUPPORTED CONDITION \"%d\"", cn->type);

#undef BUILDCONDITIONTITLE
}

//#define ACHIEVEDBRITE

char *M_BuildConditionSetString(UINT8 unlockid)
{
	conditionset_t *c = NULL;
	UINT32 lastID = 0;
	condition_t *cn;
#ifdef ACHIEVEDBRITE
	boolean achieved = false;
#endif
	size_t len = 1024, worklen;
	static char message[1024] = "";
	const char *work = NULL;
	size_t max = 0, start = 0, strlines = 0, i;

	message[0] = '\0';

	if (unlockid >= MAXUNLOCKABLES)
	{
		return NULL;
	}

	if (!unlockables[unlockid].conditionset)
	{
		return NULL;
	}

	c = &conditionSets[unlockables[unlockid].conditionset-1];

	for (i = 0; i < c->numconditions; ++i)
	{
		cn = &c->condition[i];

		if (i > 0)
		{
			worklen = 3;
			if (lastID == cn->id)
			{
				strncat(message, "\n& ", len);
			}
			else
			{
				strncat(message, "\nOR ", len);
				worklen++;
			}
			len -= worklen;
		}
		lastID = cn->id;

#ifdef ACHIEVEDBRITE
		achieved = M_CheckCondition(cn);

		if (achieved)
		{
			strncat(message, "\0x82", len);
			len--;
		}
#endif

		work = M_GetConditionString(cn);
		worklen = strlen(work);

		strncat(message, work, len);
		len -= worklen;

#ifdef ACHIEVEDBRITE
		if (achieved)
		{
			strncat(message, "\0x80", len);
			len--;
		}
#endif
	}

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, etc. but who cares.
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
		}
		else if (message[i] == '\n')
		{
			strlines = i;
			start = 0;
			max = 0;
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
			max -= (start-strlines)*8;
			strlines = start;
			start = 0;
		}
	}

	return message;
}

static void M_CheckUnlockConditions(void)
{
	INT32 i;
	conditionset_t *c;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];
		if (!c->numconditions || gamedata->achieved[i])
			continue;

		gamedata->achieved[i] = (M_CheckConditionSet(c));
	}
}

boolean M_UpdateUnlockablesAndExtraEmblems(boolean loud)
{
	INT32 i;
	UINT8 response = 0;

	if (!loud)
	{
		// Just in case they aren't to sync
		// Done first so that emblems are ready before check
		M_CheckLevelEmblems();
		M_CompletionEmblems();
	}

	M_CheckUnlockConditions();

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
	if (response)
	{
		if (loud)
		{
			S_StartSound(NULL, sfx_ncitem);
		}
		return true;
	}
	return false;
}

UINT8 M_GetNextAchievedUnlock(void)
{
	UINT8 i;

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

	return MAXUNLOCKABLES;
}

// Emblem unlocking shit
UINT8 M_CheckLevelEmblems(void)
{
	INT32 i;
	INT32 valToReach;
	INT16 tag;
	INT16 levelnum;
	UINT8 res;
	UINT8 somethingUnlocked = 0;

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

UINT8 M_CompletionEmblems(void) // Bah! Duplication sucks, but it's for a separate print when awarding emblems and it's sorta different enough.
{
	INT32 i;
	INT32 embtype;
	INT16 levelnum;
	UINT8 res;
	UINT8 somethingUnlocked = 0;
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

		res = ((mapheaderinfo[levelnum]->mapvisited & flags) == flags);

		gamedata->collected[i] = res;
		if (res)
			++somethingUnlocked;
	}

	return somethingUnlocked;
}

// -------------------
// Quick unlock checks
// -------------------

boolean M_CheckNetUnlockByID(UINT8 unlockid)
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
	UINT8 i;

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

boolean M_MapLocked(INT32 mapnum)
{
	UINT8 i;

	// Don't lock maps in dedicated servers.
	// That just makes hosts' lives hell.
	if (dedicated)
		return false;

	// No skipping over any part of your marathon.
	if (marathonmode)
		return false;

	if (!mapnum || mapnum > nummapheaders)
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
UINT8 M_GotEnoughMedals(INT32 number)
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

UINT8 M_GotLowEnoughTime(INT32 tictime)
{
	INT32 curtics = 0;
	INT32 i;

	for (i = 0; i < nummapheaders; ++i)
	{
		if (!mapheaderinfo[i] || (mapheaderinfo[i]->menuflags & LF2_NOTIMEATTACK))
			continue;

		if (!mapheaderinfo[i]->mainrecord || !mapheaderinfo[i]->mainrecord->time)
			return false;
		else if ((curtics += mapheaderinfo[i]->mainrecord->time) > tictime)
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
			while (cup)
			{
				if (!strcmp(cup->name, unlock->stringVar))
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
