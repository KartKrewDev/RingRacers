// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2012-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cond.h
/// \brief Unlockable condition system for SRB2 version 2.1

#include "doomdef.h"

// --------
// Typedefs
// --------

// DEHackEd structure for each is listed after the condition type
// [required] <optional>
typedef enum
{
	UC_PLAYTIME,		// PLAYTIME [tics]
	UC_MATCHESPLAYED,	// SRB2Kart: MATCHESPLAYED [x played]
	UC_POWERLEVEL,		// SRB2Kart: POWERLEVEL [power level to reach] [gametype, "0" for race, "1" for battle]
	UC_GAMECLEAR,		// GAMECLEAR <x times>
	UC_OVERALLTIME,		// OVERALLTIME [time to beat, tics]
	UC_MAPVISITED,		// MAPVISITED [map number]
	UC_MAPBEATEN,		// MAPBEATEN [map number]
	UC_MAPENCORE,		// MAPENCORE [map number]
	UC_MAPTIME,			// MAPTIME [map number] [time to beat, tics]
	UC_TRIGGER,			// TRIGGER [trigger number]
	UC_TOTALEMBLEMS,	// TOTALEMBLEMS [number of emblems]
	UC_EMBLEM,			// EMBLEM [emblem number]
	UC_UNLOCKABLE,		// UNLOCKABLE [unlockable number]
	UC_CONDITIONSET,	// CONDITIONSET [condition set number]
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
};
struct conditionset_t
{
	UINT32 numconditions;   /// <- number of conditions.
	condition_t *condition; /// <- All conditionals to be checked.
};

// Emblem information
#define ET_GLOBAL  0 // Emblem with a position in space
#define ET_MAP     1 // Beat the map
#define ET_TIME    2 // Get the time
//#define ET_DEVTIME 3 // Time, but the value is tied to a Time Trial demo, not pre-defined

// Global emblem flags
// (N/A to Kart yet)
//#define GE_OH 1

// Map emblem flags
#define ME_ENCORE 1

struct emblem_t
{
	UINT8 type;      ///< Emblem type
	INT16 tag;       ///< Tag of emblem mapthing
	char * level;     ///< Level on which this emblem can be found.
	UINT8 sprite;    ///< emblem sprite to use, 0 - 25
	UINT16 color;    ///< skincolor to use
	INT32 var;       ///< If needed, specifies information on the target amount to achieve (or target skin)
	char *stringVar; ///< String version
	char hint[110];  ///< Hint for emblem hints menu
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
	UINT8 majorunlock;
};

#define SECRET_NONE			 0 // Does nil.  Use with levels locked by UnlockRequired
#define SECRET_HEADER		 1 // Does nothing on its own, just serves as a header for the menu

#define SECRET_SKIN			 2 // Allow this character to be selected
#define SECRET_FOLLOWER		 3 // Allow this follower to be selected

#define SECRET_EXTRAEMBLEM	 4 // Extra Emblems (formerly extraemblem_t)

#define SECRET_TIMEATTACK	 5 // Enables Time Attack on the main menu
#define SECRET_BREAKTHECAPSULES	6 // Enables Break the Capsules on the main menu
#define SECRET_SOUNDTEST	 7 // Sound Test
#define SECRET_CREDITS		 8 // Enables Credits

#define SECRET_ITEMFINDER	 9 // Enables Item Finder/Emblem Radar
#define SECRET_EMBLEMHINTS	10 // Enables Emblem Hints

#define SECRET_ENCORE		11 // Enables Encore mode cvar
#define SECRET_HARDSPEED	12 // Enables Hard gamespeed
#define SECRET_HELLATTACK	13 // Map Hell in record attack

#define SECRET_PANDORA		14 // Enables Pandora's Box

// If you have more secrets than these variables allow in your game,
// you seriously need to get a life.
#define MAXCONDITIONSETS UINT8_MAX
#define MAXEMBLEMS       512
#define MAXUNLOCKABLES   MAXCONDITIONSETS

#define CHALLENGEGRIDHEIGHT 5
#ifdef DEVELOP
#define CHALLENGEGRIDLOOPWIDTH 3
#else
#define CHALLENGEGRIDLOOPWIDTH (BASEVIDWIDTH/16)
#endif
#define challengegridloops (gamedata->challengegridwidth >= CHALLENGEGRIDLOOPWIDTH)

// GAMEDATA STRUCTURE
// Everything that would get saved in gamedata.dat
struct gamedata_t
{
	// WHENEVER OR NOT WE'RE READY TO SAVE
	boolean loaded;

	// CONDITION SETS ACHIEVED
	boolean achieved[MAXCONDITIONSETS];

	// EMBLEMS COLLECTED
	boolean collected[MAXEMBLEMS];

	// UNLOCKABLES UNLOCKED
	boolean unlocked[MAXUNLOCKABLES];

	// CHALLENGE GRID
	UINT16 challengegridwidth;
	UINT8 *challengegrid;

	// # OF TIMES THE GAME HAS BEEN BEATEN
	UINT32 timesBeaten;

	// PLAY TIME
	UINT32 totalplaytime;
	UINT32 matchesplayed;
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
void M_PopulateChallengeGrid(void);
UINT8 *M_ChallengeGridExtraData(void);
#define CHE_NONE          0
#define CHE_HINT          1
#define CHE_CONNECTEDLEFT 2
#define CHE_CONNECTEDUP   4
#define CHE_DONTDRAW (CHE_CONNECTEDLEFT|CHE_CONNECTEDUP)

// Condition set setup
void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2);

// Clearing secrets
void M_ClearConditionSet(UINT8 set);
void M_ClearSecrets(void);

// Updating conditions and unlockables
void M_CheckUnlockConditions(void);
UINT8 M_CheckCondition(condition_t *cn);
boolean M_UpdateUnlockablesAndExtraEmblems(boolean silent);
UINT8 M_GetNextAchievedUnlock(void);
UINT8 M_CheckLevelEmblems(void);
UINT8 M_CompletionEmblems(void);

// Checking unlockable status
boolean M_CheckNetUnlockByID(UINT8 unlockid);
boolean M_SecretUnlocked(INT32 type, boolean local);
boolean M_MapLocked(INT32 mapnum);
INT32 M_CountEmblems(void);

// Emblem shit
emblem_t *M_GetLevelEmblems(INT32 mapnum);
skincolornum_t M_GetEmblemColor(emblem_t *em);
const char *M_GetEmblemPatch(emblem_t *em, boolean big);

// If you're looking to compare stats for unlocks or what not, use these
// They stop checking upon reaching the target number so they
// should be (theoretically?) slightly faster.
UINT8 M_GotEnoughEmblems(INT32 number);
UINT8 M_GotLowEnoughTime(INT32 tictime);

INT32 M_UnlockableSkinNum(unlockable_t *unlock);
INT32 M_UnlockableFollowerNum(unlockable_t *unlock);
INT32 M_EmblemSkinNum(emblem_t *emblem);

#define M_Achieved(a) ((a) >= MAXCONDITIONSETS || gamedata->achieved[a])
