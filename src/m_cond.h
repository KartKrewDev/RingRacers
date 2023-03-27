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
	UC_ROUNDSPLAYED,	// ROUNDSPLAYED [x played]
	UC_TOTALRINGS,		// TOTALRINGS [x collected]

	UC_POWERLEVEL,		// SRB2Kart: POWERLEVEL [power level to reach] [gametype, "0" for race, "1" for battle]

	UC_GAMECLEAR,		// GAMECLEAR <x times>
	UC_OVERALLTIME,		// OVERALLTIME [time to beat, tics]

	UC_MAPVISITED,		// MAPVISITED [map]
	UC_MAPBEATEN,		// MAPBEATEN [map]
	UC_MAPENCORE,		// MAPENCORE [map]
	UC_MAPSPBATTACK,	// MAPSPBATTACK [map]
	UC_MAPTIME,			// MAPTIME [map] [time to beat, tics]

	UC_ALLCHAOS,		// ALLCHAOS [minimum difficulty]
	UC_ALLSUPER,		// ALLSUPER [minimum difficulty]
	UC_ALLEMERALDS,		// ALLEMERALDS [minimum difficulty]

	UC_TOTALMEDALS,		// TOTALMEDALS [number of emblems]
	UC_EMBLEM,			// EMBLEM [emblem number]

	UC_UNLOCKABLE,		// UNLOCKABLE [unlockable number]
	UC_CONDITIONSET,	// CONDITIONSET [condition set number]

	UC_ADDON,			// Ever loaded a custom file?
	UC_REPLAY,			// Save a replay
	UC_CRASH,			// Hee ho !

	 // Just for string building
	UC_AND,
	UC_COMMA,

	UCRP_REQUIRESPLAYING, // All conditions below this can only be checked if (Playing() && gamestate == GS_LEVEL).

	UCRP_PREFIX_GRANDPRIX = UCRP_REQUIRESPLAYING, // GRAND PRIX:
	UCRP_PREFIX_BONUSROUND, // BONUS ROUND:
	UCRP_PREFIX_TIMEATTACK, // TIME ATTACK:
	UCRP_PREFIX_PRISONBREAK, // PRISON BREAK:
	UCRP_PREFIX_SEALEDSTAR, // SEALED STAR:

	UCRP_PREFIX_ISMAP, // name of [map]:
	UCRP_ISMAP, // gamemap == [map]

	UCRP_ISCHARACTER, // character == [skin]
	UCRP_ISENGINECLASS, // engine class [class]
	UCRP_ISDIFFICULTY, // difficulty >= [difficulty]

	UCRP_PODIUMCUP, // cup == [cup] [optional: >= grade OR place]
	UCRP_PODIUMEMERALD, // Get to podium sequence with that cup's emerald
	UCRP_PODIUMPRIZE, // Get to podium sequence with that cup's bonus (alternate string version of UCRP_PODIUMEMERALD

	UCRP_FINISHCOOL, // Finish in good standing
	UCRP_FINISHALLPRISONS, // Break all prisons
	UCRP_NOCONTEST, // No Contest

	UCRP_FINISHPLACE, // Finish at least [place]
	UCRP_FINISHPLACEEXACT, // Finish at [place] exactly

	UCRP_FINISHTIME, // Finish <= [time, tics]
	UCRP_FINISHTIMEEXACT, // Finish == [time, tics]
	UCRP_FINISHTIMELEFT, // Finish with at least [time, tics] to spare

	UCRP_TRIGGER,	// Map execution trigger [id]

	UCRP_FALLOFF, // Fall off (or don't)
	UCRP_TOUCHOFFROAD, // Touch offroad (or don't)
	UCRP_TOUCHSNEAKERPANEL, // Either touch sneaker panel (or don't)
	UCRP_RINGDEBT, // Go into debt (or don't)

	UCRP_TRIPWIREHYUU, // Go through tripwire with Hyudoro
	UCRP_SPBNEUTER, // Kill an SPB with Lightning
	UCRP_LANDMINEDUNK, // huh? you died? that's weird. all i did was try to hug you...
	UCRP_HITMIDAIR, // Hit another player mid-air with a kartfielditem

	UCRP_WETPLAYER, // Don't touch [fluid]
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
	char *stringvar;     /// <- Extra z-allocated string for the condition when needed
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
#define ME_SPBATTACK	2	// Achieve in SPB Attack

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
	SECRET_PRISONBREAK,			// Permit SP Prison attack
	SECRET_SPECIALATTACK,		// Permit Special attack (You're blue now!)
	SECRET_SPBATTACK,			// Permit SPB mode of Time attack

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

#define GDMUSIC_LOSERCLUB	0x01

// This is the largest number of 9s that will fit in UINT32 and UINT16 respectively.
#define GDMAX_RINGS 999999999
#define GDMAX_CHAOKEYS 9999

#ifdef DEVELOP
#define GDCONVERT_ROUNDSTOKEY 20
#else
#define GDCONVERT_ROUNDSTOKEY 50
#endif

typedef enum {
	GDGT_RACE,
	GDGT_BATTLE,
	GDGT_PRISONS,
	GDGT_SPECIAL,
	GDGT_CUSTOM,
	GDGT_MAX
} roundsplayed_t;

// GAMEDATA STRUCTURE
// Everything that would get saved in gamedata.dat
struct gamedata_t
{
	// WHENEVER OR NOT WE'RE READY TO SAVE
	boolean loaded;
	boolean deferredsave;
	boolean deferredconditioncheck;

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
	UINT32 roundsplayed[GDGT_MAX];
	UINT32 totalrings;

	// Chao Key condition bypass
	UINT32 pendingkeyrounds;
	UINT8 pendingkeyroundoffset;
	UINT8 keyspending;
	UINT16 chaokeys;

	// SPECIFIC SPECIAL EVENTS
	boolean everloadedaddon;
	boolean eversavedreplay;
	boolean everseenspecial;
	boolean evercrashed;
	UINT8 musicflags;
};

extern gamedata_t *gamedata;

// Netsynced functional alternative to gamedata->unlocked
extern boolean netUnlocked[MAXUNLOCKABLES];

extern conditionset_t conditionSets[MAXCONDITIONSETS];
extern emblem_t emblemlocations[MAXEMBLEMS];
extern unlockable_t unlockables[MAXUNLOCKABLES];

extern INT32 numemblems;

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
void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2, char *stringvar);
void M_UpdateConditionSetsPending(void);

// Clearing secrets
void M_ClearConditionSet(UINT8 set);
void M_ClearSecrets(void);
void M_ClearStats(void);

boolean M_NotFreePlay(player_t *player);

// Updating conditions and unlockables
boolean M_CheckCondition(condition_t *cn, player_t *player);
boolean M_UpdateUnlockablesAndExtraEmblems(boolean loud, boolean doall);

#define PENDING_CHAOKEYS (UINT16_MAX-1)
UINT16 M_GetNextAchievedUnlock(void);

UINT8 M_CheckLevelEmblems(void);
UINT8 M_CompletionEmblems(void);

// Checking unlockable status
boolean M_CheckNetUnlockByID(UINT8 unlockid);
boolean M_SecretUnlocked(INT32 type, boolean local);
boolean M_CupLocked(cupheader_t *cup);
boolean M_MapLocked(UINT16 mapnum);
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
