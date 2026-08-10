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
/// \file  m_cond.h
/// \brief Challenges internals

#ifndef M_COND_H
#define M_COND_H

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
	UC_NONE,

	UC_PLAYTIME,		// PLAYTIME [tics]
	UC_ROUNDSPLAYED,	// ROUNDSPLAYED [x played]
	UC_TOTALRINGS,		// TOTALRINGS [x collected]
	UC_TOTALTUMBLETIME,	// TOTALTUMBLETIME [tics]

	UC_GAMECLEAR,		// GAMECLEAR <x times>
	UC_OVERALLTIME,		// OVERALLTIME [time to beat, tics]

	UC_MAPVISITED,		// MAPVISITED [map]
	UC_MAPBEATEN,		// MAPBEATEN [map]
	UC_MAPENCORE,		// MAPENCORE [map]
	UC_MAPSPBATTACK,	// MAPSPBATTACK [map]
	UC_MAPMYSTICMELODY,	// MAPMYSTICMELODY [map]
	UC_MAPTIME,			// MAPTIME [map] [time to beat, tics]

	UC_CHARACTERWINS,	// CHARACTERWINS [character] [x rounds]

	UC_ALLCUPRECORDS,	// ALLCUPRECORDS [cup to complete up to] [minimum position] [minimum difficulty]

	UC_ALLCHAOS,		// ALLCHAOS [minimum difficulty]
	UC_ALLSUPER,		// ALLSUPER [minimum difficulty]
	UC_ALLEMERALDS,		// ALLEMERALDS [minimum difficulty]

	UC_TOTALMEDALS,		// TOTALMEDALS [number of emblems]
	UC_EMBLEM,			// EMBLEM [emblem number]

	UC_UNLOCKABLE,		// UNLOCKABLE [unlockable number]
	UC_CONDITIONSET,	// CONDITIONSET [condition set number]

	UC_UNLOCKPERCENT,	// Unlock <x percent> of [unlockable type]

	UC_ADDON,			// Ever loaded a custom file?
	UC_CREDITS,			// Finish watching the credits
	UC_REPLAY,			// Save a replay
	UC_CRASH,			// Hee ho !
	UC_TUTORIALSKIP,	// Complete the Tutorial Challenge
	UC_TUTORIALDONE,	// Complete the Tutorial at all
	UC_PLAYGROUND,		// Go to the playground instead..?

	UC_PASSWORD,		// Type in something funny

	UC_SPRAYCAN,		// Grab a spraycan

	UC_PRISONEGGCD,		// Grab a CD from a Prison Egg

	 // Just for string building
	UC_AND,
	UC_THEN,
	UC_COMMA,
	UC_DESCRIPTIONOVERRIDE,

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
	UCRP_HASFOLLOWER, // follower == [followerskin]
	UCRP_ISDIFFICULTY, // GP difficulty >= [difficulty]
	UCRP_ISGEAR, // gear speed >= [speed]

	UCRP_PODIUMCUP, // cup == [cup] [optional: >= grade OR place]
	UCRP_PODIUMEMERALD, // Get to podium sequence with that cup's emerald
	UCRP_PODIUMPRIZE, // Get to podium sequence with that cup's bonus (alternate string version of UCRP_PODIUMEMERALD
	UCRP_PODIUMNOCONTINUES, // Get to podium sequence without any continues

	UCRP_FINISHCOOL, // Finish in good standing
	UCRP_FINISHPERFECT, // Finish a perfect race
	UCRP_FINISHALLPRISONS, // Break all prisons
	UCRP_SURVIVE, // Survive
	UCRP_NOCONTEST, // No Contest

	UCRP_SMASHUFO, // Smash the UFO Catcher
	UCRP_CHASEDBYSPB, // Chased by SPB
	UCRP_MAPDESTROYOBJECTS, // LEVELNAME: Destroy all [object names] -- CAUTION: You have to add to the level's header too to get them successfully tracked!

	UCRP_MAKERETIRE, // Make another player of [skin] No Contest

	UCRP_FINISHPLACE, // Finish at least [place]
	UCRP_FINISHPLACEEXACT, // Finish at [place] exactly

	UCRP_FINISHGRADE, // Finish with at least grade [grade]

	UCRP_FINISHTIME, // Finish <= [time, tics]
	UCRP_FINISHTIMEEXACT, // Finish == [time, tics]
	UCRP_FINISHTIMELEFT, // Finish with at least [time, tics] to spare

	UCRP_RINGS, // >= [rings]
	UCRP_RINGSEXACT, // == [rings]

	UCRP_SPEEDOMETER, // >= [percentage]
	UCRP_DRAFTDURATION, // >= [time, seconds]
	UCRP_GROWCONSECUTIVEBEAMS, // touch more than n times consecutively

	UCRP_TRIGGER,	// Map execution trigger [id]

	UCRP_FALLOFF, // Fall off (or don't)
	UCRP_TOUCHOFFROAD, // Touch offroad (or don't)
	UCRP_TOUCHSNEAKERPANEL, // Either touch sneaker panel (or don't)
	UCRP_RINGDEBT, // Go into debt (or don't)
	UCRP_FAULTED, // FAULT

	UCRP_TRIPWIREHYUU, // Go through tripwire with Hyudoro
	UCRP_WHIPHYUU, // Use Insta-Whip with Hyudoro
	UCRP_SPBNEUTER, // Kill an SPB with Lightning
	UCRP_LANDMINEDUNK, // huh? you died? that's weird. all i did was try to hug you...
	UCRP_HITMIDAIR, // Hit another player mid-air with a kartfielditem
	UCRP_HITDRAFTERLOOKBACK, // Hit a player that's behind you, while looking back at them, and they're drafting off you
	UCRP_GIANTRACERSHRUNKENORBI, // Hit a giant racer with a shrunken Orbinaut
	UCRP_RETURNMARKTOSENDER, // Hit the player responsible for Eggman Marking you with that explosion
	UCRP_ALLANCIENTGEARS, // Collect all Ancient Gears in a map

	UCRP_TRACKHAZARD, // (Don't) get hit by a track hazard (maybe specific lap)

	UCRP_TARGETATTACKMETHOD, // Break targets/UFO using only one method
	UCRP_GACHABOMMISER, // Break targets/UFO using exactly one Gachabom repeatedly

	UCRP_WETPLAYER, // Don't touch [strictness] [fluid]
} conditiontype_t;

// Condition Set information
struct condition_t
{
	uint32_t id;           /// <- The ID of this condition.
	                     ///    In an unlock condition, all conditions with the same ID
	                     ///    must be true to fulfill the unlockable requirements.
	                     ///    Only one ID set needs to be true, however.
	conditiontype_t type;/// <- The type of condition
	int32_t requirement;   /// <- The requirement for this variable.
	int16_t extrainfo1;    /// <- Extra information for the condition when needed.
	int16_t extrainfo2;    /// <- Extra information for the condition when needed.
	char *stringvar;     /// <- Extra z-allocated string for the condition when needed
};
struct conditionset_t
{
	uint32_t numconditions;   /// <- number of conditions.
	condition_t *condition; /// <- All conditionals to be checked.
};

// Emblem information
#define ET_NONE			0	// Empty slot
#define ET_GLOBAL		1	// Emblem with a position in space
#define ET_MAP			2	// Beat the map
#define ET_TIME			3	// Get the time

// Global emblem flags
#define GE_NOTMEDAL		1	// Doesn't count towards number of medals
#define GE_TIMED		2	// Disappears after var time
#define GE_FOLLOWER		4	// Takes on the appearance of a Follower in (string)var2

// Map emblem flags
#define ME_ENCORE		1	// Achieve in Encore
#define ME_SPBATTACK	2	// Achieve in SPB Attack

// Automedal SOC tags
#define AUTOMEDAL_MAX       -5 // just in case any more are ever added
#define AUTOMEDAL_BRONZE    -4
#define AUTOMEDAL_SILVER    -3
#define AUTOMEDAL_GOLD      -2
#define AUTOMEDAL_PLATINUM  -1

struct emblem_t
{
	uint8_t type;			///< Emblem type
	int16_t tag;			///< Tag of emblem mapthing
	char *level;		///< Level on which this emblem can be found.
	int16_t levelCache;	///< Stored G_MapNumber()+1 result
	uint8_t sprite;		///< emblem sprite to use, 0 - 25
	uint16_t color;		///< skincolor to use
	int32_t flags;		///< GE or ME constants
	int32_t var;			///< If needed, specifies extra information
	int32_t var2;			///< Ditto
	char *stringVar;	///< String version
	char *stringVar2;	///< Ditto
};

// Unlockable information
struct unlockable_t
{
	char name[64];
	char *icon;
	uint16_t color;
	uint16_t conditionset;
	int16_t type;
	int16_t variable;
	char *stringVar;
	int16_t stringVarCache;
	uint8_t majorunlock;
};

typedef enum
{
	SECRET_NONE = 0,			// Does nil, useful as a default only

	// One step above bragging rights
	SECRET_EXTRAMEDAL,			// Extra medal for your counter

	// Level restrictions
	SECRET_CUP,					// Permit access to entire cup (overrides SECRET_MAP)
	SECRET_MAP,					// Permit access to single map
	SECRET_ALTMUSIC,			// Permit access to single map music track

	// Player restrictions
	SECRET_SKIN,				// Permit this character
	SECRET_FOLLOWER,			// Permit this follower
	SECRET_COLOR,				// Permit this color

	// Everything below this line is supposed to be only one per Challenges list
	SECRET_ONEPERBOARD,

	// Difficulty restrictions
	SECRET_HARDSPEED = SECRET_ONEPERBOARD, // Permit Hard gamespeed
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
#define MAXCONDITIONSETS 1024
#define MAXEMBLEMS       (MAXCONDITIONSETS*4)
#define MAXUNLOCKABLES   MAXCONDITIONSETS

#define CHALLENGEGRIDHEIGHT 5
#ifdef DEVELOP
#define CHALLENGEGRIDLOOPWIDTH 3
#else
#define CHALLENGEGRIDLOOPWIDTH (BASEVIDWIDTH/16)
#endif
#define challengegridloops (gamedata->challengegridwidth >= CHALLENGEGRIDLOOPWIDTH)

#define CH_FURYBIKE 55

// See also M_PlayMenuJam
typedef enum {
	GDMUSIC_NONE = 0,
	GDMUSIC_KEYG,
	GDMUSIC_KEEPONMENU, // Minimum to keep after leaving the Challenge Grid
	GDMUSIC_LOSERCLUB = GDMUSIC_KEEPONMENU,
	GDMUSIC_TRACK10,
	GDMUSIC_MAX
} gdmusic_t;

// This is the largest number of 9s that will fit in uint32_t and uint16_t respectively.
#define GDMAX_RINGS 999999999
#define GDMAX_CHAOKEYS 9999
#define GDMAX_SEALEDSWAPS 7

#define GDCONVERT_ROUNDSTOKEY 5

#define GDINIT_CHAOKEYS 0 // Start with ZERO Chao Keys. You get NONE. fizzy lifting dink
#define GDINIT_PRISONSTOPRIZE 15 // 15 Prison Eggs to your [Wild Prize] !!

typedef enum {
	GDGONER_INIT = 0,
	GDGONER_INTRO,
	GDGONER_VIDEO,
	GDGONER_SOUND,
	GDGONER_PROFILE,
	GDGONER_TUTORIAL,
	GDGONER_OUTRO,
	GDGONER_DONE,
} gdgoner_t;

struct candata_t
{
	uint16_t col;
	uint16_t map;
};

// GAMEDATA STRUCTURE
// Everything that would get saved in gamedata.dat
struct gamedata_t
{
	// WHENEVER OR NOT WE'RE READY TO SAVE
	dboolean loaded;

	// DEFERRED EVENTS RELATING TO CHALLENGE PROCESSING
	dboolean deferredsave;
	dboolean deferredconditioncheck;

	// CONDITION SETS ACHIEVED
	dboolean achieved[MAXCONDITIONSETS];

	// EMBLEMS COLLECTED
	dboolean collected[MAXEMBLEMS];

	// UNLOCKABLES UNLOCKED
	dboolean unlocked[MAXUNLOCKABLES];
	dboolean unlockpending[MAXUNLOCKABLES];

	// SPRAYCANS COLLECTED
	uint16_t numspraycans;
	uint16_t gotspraycans;
	candata_t* spraycans;

	// PRISON EGG PICKUPS
	uint16_t numprisoneggpickups;
	uint16_t thisprisoneggpickup;
	condition_t *thisprisoneggpickup_cached;
	dboolean thisprisoneggpickupgrabbed;
	uint16_t prisoneggstothispickup;
	uint16_t* prisoneggpickups;

	// CHALLENGE GRID
	uint16_t challengegridwidth;
	uint16_t *challengegrid;

	// # OF TIMES THE GAME HAS BEEN BEATEN
	uint32_t timesBeaten;

	// PLAY TIME
	uint32_t totalplaytime;
	uint32_t totalnetgametime;
	uint32_t timeattackingtotaltime;
	uint32_t spbattackingtotaltime;
	uint32_t modeplaytime[GDGT_MAX];
	uint32_t totalmenutime;
	uint32_t totaltimestaringatstatistics;
	uint32_t roundsplayed[GDGT_MAX];
	uint32_t totalrings;
	uint32_t totaltumbletime;

	// CHAO KEYS AND THEIR GENERATION
	uint32_t pendingkeyrounds;
	uint8_t pendingkeyroundoffset;
	uint16_t keyspending;
	uint16_t chaokeys;

	// EMERALD REMAPPING
	cupheader_t *sealedswaps[GDMAX_SEALEDSWAPS];

	// SPECIFIC SPECIAL EVENTS
	dboolean everloadedaddon;
	dboolean everfinishedcredits;
	dboolean eversavedreplay;
	dboolean everseenspecial;
	dboolean evercrashed;
	dboolean chaokeytutorial;
	dboolean majorkeyskipattempted;
	dboolean enteredtutorialchallenge;
	dboolean finishedtutorialchallenge;
	dboolean sealedswapalerted;
	dboolean tutorialdone;
	dboolean playgroundroute;
	gdmusic_t musicstate;

	uint8_t gonerlevel;

	// BACKWARDS COMPAT ASSIST
	dboolean importprofilewins;
};

extern gamedata_t *gamedata;

// Netsynced functional alternative to gamedata->unlocked
extern dboolean netUnlocked[MAXUNLOCKABLES];

extern conditionset_t conditionSets[MAXCONDITIONSETS];
extern emblem_t emblemlocations[MAXEMBLEMS];
extern unlockable_t unlockables[MAXUNLOCKABLES];

extern int32_t numemblems;

void M_NewGameDataStruct(void);

// Challenges menu stuff
void M_PopulateChallengeGrid(void);
void M_SanitiseChallengeGrid(void);

struct challengegridextradata_t
{
	uint8_t flags;
	uint8_t flip;
};

void M_UpdateChallengeGridExtraData(challengegridextradata_t *extradata);

#define CHE_NONE          0
#define CHE_HINT          1
#define CHE_CONNECTEDLEFT (1<<1)
#define CHE_CONNECTEDUP   (1<<2)
#define CHE_DONTDRAW (CHE_CONNECTEDLEFT|CHE_CONNECTEDUP)
#define CHE_ALLCLEAR      (1<<3)

char *M_BuildConditionSetString(uint16_t unlockid);
#define DESCRIPTIONWIDTH 170

// Condition set setup
void M_AddRawCondition(uint16_t set, uint8_t id, conditiontype_t c, int32_t r, int16_t x1, int16_t x2, char *stringvar);
void M_UpdateConditionSetsPending(void);

// Gamedata clear/init
void M_ClearConditionSet(uint16_t set);
void M_ClearSecrets(void);
void M_ClearStats(void);
void M_FinaliseGameData(void);
void M_SetNetUnlocked(void);

dboolean M_NotFreePlay(void);
uint16_t M_CheckCupEmeralds(uint8_t difficulty);

// Updating conditions and unlockables
dboolean M_CheckCondition(condition_t *cn, player_t *player);
dboolean M_UpdateUnlockablesAndExtraEmblems(dboolean loud, dboolean doall);

#define PENDING_CHAOKEYS (UINT16_MAX-1)
uint16_t M_GetNextAchievedUnlock(dboolean canskipchaokeys);

void M_UpdateNextPrisonEggPickup(void);

uint16_t M_CheckLevelEmblems(void);
uint16_t M_CompletionEmblems(void);

extern uint16_t gamestartchallenge;

// Checking unlockable status
dboolean M_CheckNetUnlockByID(uint16_t unlockid);
dboolean M_SecretUnlocked(int32_t type, dboolean local);
dboolean M_GameTrulyStarted(void);
dboolean M_GameAboutToStart(void);
dboolean M_CupLocked(cupheader_t *cup);
dboolean M_CupSecondRowLocked(void);
dboolean M_MapLocked(uint16_t mapnum);
int32_t M_CountMedals(dboolean all, dboolean extraonly);

// Emblem shit
emblem_t *M_GetLevelEmblems(int32_t mapnum);
skincolornum_t M_GetEmblemColor(emblem_t *em);
const char *M_GetEmblemPatch(emblem_t *em, dboolean big);

// If you're looking to compare stats for unlocks or what not, use these
// They stop checking upon reaching the target number so they
// should be (theoretically?) slightly faster.
dboolean M_GotEnoughMedals(int32_t number);
dboolean M_GotLowEnoughTime(int32_t tictime);

int32_t M_UnlockableSkinNum(unlockable_t *unlock);
int32_t M_UnlockableFollowerNum(unlockable_t *unlock);
int32_t M_UnlockableColorNum(unlockable_t *unlock);
cupheader_t *M_UnlockableCup(unlockable_t *unlock);
uint16_t M_UnlockableMapNum(unlockable_t *unlock);

int32_t M_EmblemSkinNum(emblem_t *emblem);
uint16_t M_EmblemMapNum(emblem_t *emblem);

#define M_Achieved(a) ((a) >= MAXCONDITIONSETS || gamedata->achieved[a])

dboolean M_UseAlternateTitleScreen(void);
int32_t M_GameDataGameType(int32_t gametype, dboolean battleprisons);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // M_COND_H
