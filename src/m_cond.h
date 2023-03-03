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
/// \file  m_cond.h
/// \brief Unlockable condition system for SRB2 version 2.1

#ifndef __M_COND_H__
#define __M_COND_H__

#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------
// Typedefs
// --------

// DEHackEd structure for each is listed after the condition type
// [required] <optional>
typedef enum
{
	UC_PLAYTIME,		// PLAYTIME [tics]
	UC_MATCHESPLAYED,	// SRB2Kart: MATCHESPLAYED [x played]
	UC_TOTALRINGS,		// TOTALRINGS [x collected]
	UC_POWERLEVEL,		// SRB2Kart: POWERLEVEL [power level to reach] [gametype, "0" for race, "1" for battle]
	UC_GAMECLEAR,		// GAMECLEAR <x times>
	UC_OVERALLTIME,		// OVERALLTIME [time to beat, tics]
	UC_MAPVISITED,		// MAPVISITED [map]
	UC_MAPBEATEN,		// MAPBEATEN [map]
	UC_MAPENCORE,		// MAPENCORE [map]
	UC_MAPTIME,			// MAPTIME [map] [time to beat, tics]
	UC_TRIGGER,			// TRIGGER [trigger number]
	UC_TOTALMEDALS,		// TOTALMEDALS [number of emblems]
	UC_EMBLEM,			// EMBLEM [emblem number]
	UC_UNLOCKABLE,		// UNLOCKABLE [unlockable number]
	UC_CONDITIONSET,	// CONDITIONSET [condition set number]

	UC_CRASH,			// Hee ho !

	 // Just for string building
	UC_AND,
	UC_COMMA,

	UCRP_REQUIRESPLAYING, // All conditions below this can only be checked if (Playing() && gamestate == GS_LEVEL).

	UCRP_PREFIX_GRANDPRIX = UCRP_REQUIRESPLAYING, // GRAND PRIX:
	UCRP_PREFIX_BONUSROUND, // BONUS ROUND:
	UCRP_PREFIX_TIMEATTACK, // TIME ATTACK:
	UCRP_PREFIX_BREAKTHECAPSULES, // BREAK THE CAPSULES:
	UCRP_PREFIX_SEALEDSTAR, // SEALED STAR:

	UCRP_PREFIX_ISMAP, // name of [map]:
	UCRP_ISMAP, // gamemap == [map]

	UCRP_ISCHARACTER, // character == [skin]

	UCRP_FINISHCOOL, // Finish in good standing
	UCRP_FINISHALLCAPSULES, // Break all capsules
	UCRP_NOCONTEST, // No Contest

	UCRP_FINISHPLACE, // Finish at least [place]
	UCRP_FINISHPLACEEXACT, // Finish at [place] exactly

	UCRP_FINISHTIME, // Finish <= [time, tics]
	UCRP_FINISHTIMEEXACT, // Finish == [time, tics]
	UCRP_FINISHTIMELEFT, // Finish with at least [time, tics] to spare
} conditiontype_t;

// Condition Set information
struct condition_t
{
	UINT32 id;           /// <- The ID of this condition.
	                     ///    In an unlock condition, all conditions with the same ID
	                     ///    must be true to fulfill the unlockable requirements.
	                     ///    Only one ID set needs to be true, however.
	conditiontype_t type;/// <- The type of condition
	INT32 requirement;   /// <- The requirement for this variable.
	INT16 extrainfo1;    /// <- Extra information for the condition when needed.
	INT16 extrainfo2;    /// <- Extra information for the condition when needed.
	char *pendingstring; /// oooohhh my god i hate loading order for SOC VS skins
};
struct conditionset_t
{
	UINT32 numconditions;   /// <- number of conditions.
	condition_t *condition; /// <- All conditionals to be checked.
};

// Emblem information
#define ET_GLOBAL		0	// Emblem with a position in space
#define ET_MAP			1	// Beat the map
#define ET_TIME			2	// Get the time
//#define ET_DEVTIME 3 // Time, but the value is tied to a Time Trial demo, not pre-defined

// Global emblem flags
#define GE_NOTMEDAL		1	// Doesn't count towards number of medals
#define GE_TIMED		2	// Disappears after var time

// Map emblem flags
#define ME_ENCORE		1	// Achieve in Encore

struct emblem_t
{
	UINT8 type;			///< Emblem type
	INT16 tag;			///< Tag of emblem mapthing
	char *level;		///< Level on which this emblem can be found.
	INT16 levelCache;	///< Stored G_MapNumber()+1 result
	UINT8 sprite;		///< emblem sprite to use, 0 - 25
	UINT16 color;		///< skincolor to use
	INT32 flags;		///< GE or ME constants
	INT32 var;			///< If needed, specifies extra information
	char *stringVar;	///< String version
};

// Unlockable information
struct unlockable_t
{
	char name[64];
	char *icon;
	UINT16 color;
	UINT8 conditionset;
	INT16 type;
	INT16 variable;
	char *stringVar;
	INT16 stringVarCache;
	UINT8 majorunlock;
};

typedef enum
{
	SECRET_NONE = 0,			// Does nil, useful as a default only

	// One step above bragging rights
	SECRET_EXTRAMEDAL,			// Extra medal for your counter

	// Level restrictions
	SECRET_CUP,					// Permit access to entire cup (overrides SECRET_MAP)
	SECRET_MAP,					// Permit access to single map

	// Player restrictions
	SECRET_SKIN,				// Permit this character
	SECRET_FOLLOWER,			// Permit this follower

	// Difficulty restrictions
	SECRET_HARDSPEED,			// Permit Hard gamespeed
	SECRET_MASTERMODE,			// Permit Master Mode bots in GP
	SECRET_ENCORE,				// Permit Encore option

	// Menu restrictions
	SECRET_TIMEATTACK,			// Permit Time attack
	SECRET_BREAKTHECAPSULES,	// Permit SP Capsule attack
	SECRET_SPECIALATTACK,		// Permit Special attack (You're blue now!)

	// Option restrictions
	SECRET_ONLINE,				// Permit netplay (ankle-high barrier to jumping in the deep end)
	SECRET_ADDONS,				// Permit menu addfile
	SECRET_EGGTV,				// Permit replay playback menu
	SECRET_SOUNDTEST,			// Permit Sound Test
	SECRET_ALTTITLE,			// Permit alternate titlescreen

	// Assist restrictions
	SECRET_ITEMFINDER,			// Permit locating in-level secrets
} secrettype_t;

// If you have more secrets than these variables allow in your game,
// you seriously need to get a life.
#define MAXCONDITIONSETS UINT8_MAX
#define MAXEMBLEMS       512
#define MAXUNLOCKABLES   MAXCONDITIONSETS

#define CHALLENGEGRIDHEIGHT 4
#ifdef DEVELOP
#define CHALLENGEGRIDLOOPWIDTH 3
#else
#define CHALLENGEGRIDLOOPWIDTH (BASEVIDWIDTH/16)
#endif
#define challengegridloops (gamedata->challengegridwidth >= CHALLENGEGRIDLOOPWIDTH)

#define GDCRASH_LAST		0x01
#define GDCRASH_ANY			0x02
#define GDCRASH_LOSERCLUB	0x04

// This is the largest number of 9s that will fit in UINT32.
#define GDMAX_RINGS 999999999

// GAMEDATA STRUCTURE
// Everything that would get saved in gamedata.dat
struct gamedata_t
{
	// WHENEVER OR NOT WE'RE READY TO SAVE
	boolean loaded;
	boolean deferredsave;

	// CONDITION SETS ACHIEVED
	boolean achieved[MAXCONDITIONSETS];

	// EMBLEMS COLLECTED
	boolean collected[MAXEMBLEMS];

	// UNLOCKABLES UNLOCKED
	boolean unlocked[MAXUNLOCKABLES];
	boolean unlockpending[MAXUNLOCKABLES];

	// CHALLENGE GRID
	UINT16 challengegridwidth;
	UINT8 *challengegrid;

	// # OF TIMES THE GAME HAS BEEN BEATEN
	UINT32 timesBeaten;

	// PLAY TIME
	UINT32 totalplaytime;
	UINT32 matchesplayed;
	UINT32 totalrings;

	// Funny
	UINT8 crashflags;
};

extern gamedata_t *gamedata;

// Netsynced functional alternative to gamedata->unlocked
extern boolean netUnlocked[MAXUNLOCKABLES];

extern conditionset_t conditionSets[MAXCONDITIONSETS];
extern emblem_t emblemlocations[MAXEMBLEMS];
extern unlockable_t unlockables[MAXUNLOCKABLES];

extern INT32 numemblems;

extern UINT32 unlocktriggers;

void M_NewGameDataStruct(void);

// Challenges menu stuff
void M_PopulateChallengeGrid(void);

struct challengegridextradata_t
{
	UINT8 flags;
	UINT8 flip;
};

void M_UpdateChallengeGridExtraData(challengegridextradata_t *extradata);

#define CHE_NONE          0
#define CHE_HINT          1
#define CHE_CONNECTEDLEFT (1<<1)
#define CHE_CONNECTEDUP   (1<<2)
#define CHE_DONTDRAW (CHE_CONNECTEDLEFT|CHE_CONNECTEDUP)

char *M_BuildConditionSetString(UINT8 unlockid);
#define DESCRIPTIONWIDTH 170

// Condition set setup
void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2, char *pendingstring);
void M_UpdateConditionSetsPending(void);

// Clearing secrets
void M_ClearConditionSet(UINT8 set);
void M_ClearSecrets(void);

// Updating conditions and unlockables
boolean M_CheckCondition(condition_t *cn, player_t *player);
boolean M_UpdateUnlockablesAndExtraEmblems(boolean loud);
UINT8 M_GetNextAchievedUnlock(void);
UINT8 M_CheckLevelEmblems(void);
UINT8 M_CompletionEmblems(void);

// Checking unlockable status
boolean M_CheckNetUnlockByID(UINT8 unlockid);
boolean M_SecretUnlocked(INT32 type, boolean local);
boolean M_CupLocked(cupheader_t *cup);
boolean M_MapLocked(INT32 mapnum);
INT32 M_CountMedals(boolean all, boolean extraonly);

// Emblem shit
emblem_t *M_GetLevelEmblems(INT32 mapnum);
skincolornum_t M_GetEmblemColor(emblem_t *em);
const char *M_GetEmblemPatch(emblem_t *em, boolean big);

// If you're looking to compare stats for unlocks or what not, use these
// They stop checking upon reaching the target number so they
// should be (theoretically?) slightly faster.
UINT8 M_GotEnoughMedals(INT32 number);
UINT8 M_GotLowEnoughTime(INT32 tictime);

INT32 M_UnlockableSkinNum(unlockable_t *unlock);
INT32 M_UnlockableFollowerNum(unlockable_t *unlock);
cupheader_t *M_UnlockableCup(unlockable_t *unlock);
UINT16 M_UnlockableMapNum(unlockable_t *unlock);

INT32 M_EmblemSkinNum(emblem_t *emblem);
UINT16 M_EmblemMapNum(emblem_t *emblem);

#define M_Achieved(a) ((a) >= MAXCONDITIONSETS || gamedata->achieved[a])

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __M_COND_H__
