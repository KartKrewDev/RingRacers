// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
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
#include "doomstat.h"
#include "z_zone.h"

#include "hu_stuff.h" // CEcho
#include "v_video.h" // video flags

#include "g_game.h" // record info
#include "r_skins.h" // numskins
#include "r_draw.h" // R_GetColorByName

#include "k_pwrlv.h"
#include "k_profiles.h"

gamedata_t *gamedata = NULL;

// Map triggers for linedef executors
// 32 triggers, one bit each
UINT32 unlocktriggers;

// The meat of this system lies in condition sets
conditionset_t conditionSets[MAXCONDITIONSETS];

// Emblem locations
emblem_t emblemlocations[MAXEMBLEMS];

// Extra emblems
extraemblem_t extraemblems[MAXEXTRAEMBLEMS];

// Unlockables
unlockable_t unlockables[MAXUNLOCKABLES];

// Number of emblems and extra emblems
INT32 numemblems = 0;
INT32 numextraemblems = 0;

// Create a new gamedata_t, for start-up
void M_NewGameDataStruct(void)
{
	gamedata = Z_Calloc(sizeof (gamedata_t), PU_STATIC, NULL);
	M_ClearSecrets();
	G_ClearRecords();
}

void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2)
{
	condition_t *cond;
	UINT32 num, wnum;

	I_Assert(set && set <= MAXCONDITIONSETS);

	wnum = conditionSets[set - 1].numconditions;
	num = ++conditionSets[set - 1].numconditions;

	conditionSets[set - 1].condition = Z_Realloc(conditionSets[set - 1].condition, sizeof(condition_t)*num, PU_STATIC, 0);

	cond = conditionSets[set - 1].condition;

	cond[wnum].id = id;
	cond[wnum].type = c;
	cond[wnum].requirement = r;
	cond[wnum].extrainfo1 = x1;
	cond[wnum].extrainfo2 = x2;
}

void M_ClearConditionSet(UINT8 set)
{
	if (conditionSets[set - 1].numconditions)
	{
		Z_Free(conditionSets[set - 1].condition);
		conditionSets[set - 1].condition = NULL;
		conditionSets[set - 1].numconditions = 0;
	}
	gamedata->achieved[set - 1] = false;
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
	for (i = 0; i < MAXEXTRAEMBLEMS; ++i)
		gamedata->extraCollected[i] = false;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
		gamedata->unlocked[i] = false;
	for (i = 0; i < MAXCONDITIONSETS; ++i)
		gamedata->achieved[i] = false;

	gamedata->timesBeaten = 0;

	// Re-unlock any always unlocked things
	M_UpdateUnlockablesAndExtraEmblems(false);
}

// ----------------------
// Condition set checking
// ----------------------
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
		case UC_TOTALEMBLEMS: // Requires number of emblems >= x
			return (M_GotEnoughEmblems(cn->requirement));
		case UC_EMBLEM: // Requires emblem x to be obtained
			return gamedata->collected[cn->requirement-1];
		case UC_EXTRAEMBLEM: // Requires extra emblem x to be obtained
			return gamedata->extraCollected[cn->requirement-1];
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

void M_CheckUnlockConditions(void)
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
	char cechoText[992] = "";
	UINT8 cechoLines = 0;

	M_CheckUnlockConditions();

	if (!loud)
	{
		// Just in case they aren't to sync
		M_CheckLevelEmblems();
		M_CompletionEmblems();
	}

	// Go through extra emblems
	for (i = 0; i < numextraemblems; ++i)
	{
		if (gamedata->extraCollected[i] || !extraemblems[i].conditionset)
		{
			continue;
		}

		if ((gamedata->extraCollected[i] = M_Achieved(extraemblems[i].conditionset - 1)) == false)
		{
			continue;
		}

		if (loud)
		{
			strcat(cechoText, va("Got \"%s\" medal!\n", extraemblems[i].name));
		}
		++cechoLines;
	}

	// Fun part: if any of those unlocked we need to go through the
	// unlock conditions AGAIN just in case an emblem reward was reached
	if (cechoLines)
		M_CheckUnlockConditions();

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (gamedata->unlocked[i] || !unlockables[i].conditionset)
		{
			continue;
		}

		if ((gamedata->unlocked[i] = M_Achieved(unlockables[i].conditionset - 1)) == false)
		{
			continue;
		}

		if (unlockables[i].nocecho)
		{
			continue;
		}

		if (loud)
		{
			strcat(cechoText, va("\"%s\" unlocked!\n", unlockables[i].name));
		}
		++cechoLines;
	}

	// Announce
	if (cechoLines && loud)
	{
#ifdef DEVELOP
		// todo make debugmode
		CONS_Printf("%s\n", cechoText);
#endif
		return true;
	}
	return false;
}

// Emblem unlocking shit
UINT8 M_CheckLevelEmblems(void)
{
	INT32 i;
	INT32 valToReach;
	INT16 levelnum;
	UINT8 res;
	UINT8 somethingUnlocked = 0;

	// Update Score, Time, Rings emblems
	for (i = 0; i < numemblems; ++i)
	{
		INT32 checkLevel;

		if (emblemlocations[i].type < ET_TIME || gamedata->collected[i])
			continue;

		checkLevel = G_MapNumber(emblemlocations[i].level);

		if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
			continue;

		levelnum = checkLevel;
		valToReach = emblemlocations[i].var;

		switch (emblemlocations[i].type)
		{
			case ET_TIME: // Requires time on map <= x
				res = (G_GetBestTime(levelnum) <= (unsigned)valToReach);
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

		if (emblemlocations[i].type < ET_TIME || gamedata->collected[i])
			continue;

		checkLevel = G_MapNumber(emblemlocations[i].level);

		if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
			continue;

		levelnum = checkLevel;
		embtype = emblemlocations[i].var;
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
UINT8 M_AnySecretUnlocked(void)
{
	INT32 i;

#ifdef DEVELOP
	if (1)
		return true;	
#endif	
	
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (!unlockables[i].nocecho && gamedata->unlocked[i])
			return true;
	}
	return false;
}

UINT8 M_SecretUnlocked(INT32 type)
{
	INT32 i;

	if (dedicated)
		return true;

#if 0
	(void)type;
	(void)i;
	return false; // for quick testing
#else

#ifdef DEVELOP
#define CHADYES true
#else
#define CHADYES false
#endif

	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type == type && gamedata->unlocked[i] != CHADYES)
			return !CHADYES;
	}
	return CHADYES;

#undef CHADYES
#endif //if 0
}

UINT8 M_MapLocked(INT32 mapnum)
{
#ifdef DEVELOP
	(void)mapnum;
	return false;
#else
	// Don't lock maps in dedicated servers.
	// That just makes hosts' lives hell.
	if (dedicated)
		return false;
	
	if (!mapheaderinfo[mapnum-1] || mapheaderinfo[mapnum-1]->unlockrequired < 0)
		return false;

	if (!gamedata->unlocked[mapheaderinfo[mapnum-1]->unlockrequired])
		return true;

	return false;
#endif
}

INT32 M_CountEmblems(void)
{
	INT32 found = 0, i;
	for (i = 0; i < numemblems; ++i)
	{
		if (gamedata->collected[i])
			found++;
	}
	for (i = 0; i < numextraemblems; ++i)
	{
		if (gamedata->extraCollected[i])
			found++;
	}
	return found;
}

// --------------------------------------
// Quick functions for calculating things
// --------------------------------------

// Theoretically faster than using M_CountEmblems()
// Stops when it reaches the target number of emblems.
UINT8 M_GotEnoughEmblems(INT32 number)
{
	INT32 i, gottenemblems = 0;
	for (i = 0; i < numemblems; ++i)
	{
		if (gamedata->collected[i])
			if (++gottenemblems >= number) return true;
	}
	for (i = 0; i < numextraemblems; ++i)
	{
		if (gamedata->extraCollected[i])
			if (++gottenemblems >= number) return true;
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

	if (unlock->stringVar && strcmp(unlock->stringVar, ""))
	{
		// Get the skin from the string.
		INT32 skinnum = R_SkinAvailable(unlock->stringVar);
		if (skinnum != -1)
		{
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

// ----------------
// Misc Emblem shit
// ----------------

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
		map = mapnum;
		i = numemblems;
	}

	while (--i >= 0)
	{
		INT32 checkLevel = G_MapNumber(emblemlocations[i].level);

		if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
			continue;

		if (checkLevel == map)
			return &emblemlocations[i];
	}
	return NULL;
}

skincolornum_t M_GetEmblemColor(emblem_t *em)
{
	if (!em || em->color >= numskincolors)
		return SKINCOLOR_NONE;
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

skincolornum_t M_GetExtraEmblemColor(extraemblem_t *em)
{
	if (!em || em->color >= numskincolors)
		return SKINCOLOR_NONE;
	return em->color;
}

const char *M_GetExtraEmblemPatch(extraemblem_t *em, boolean big)
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
