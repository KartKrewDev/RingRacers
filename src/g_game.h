// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_game.h
/// \brief Game loop, events handling.

#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"
#include "g_demo.h"
#include "m_cheat.h" // objectplacing

#ifdef __cplusplus
extern "C" {
#endif

extern char gamedatafilename[64];
extern char timeattackfolder[64];
extern char customversionstring[32];

extern char  player_names[MAXPLAYERS][MAXPLAYERNAME+1];
extern INT32 player_name_changes[MAXPLAYERS];

extern player_t players[MAXPLAYERS];
extern boolean playeringame[MAXPLAYERS];

// gametic at level start
extern tic_t levelstarttic;

// for modding?
extern UINT16 prevmap, nextmap;

// see also G_MapNumber
typedef enum
{
	NEXTMAP_RESERVED = INT16_MAX, // so nextmap+1 doesn't roll over -- remove when gamemap is made 0-indexed
	NEXTMAP_TITLE = INT16_MAX-1,
	NEXTMAP_EVALUATION = INT16_MAX-2,
	NEXTMAP_CREDITS = INT16_MAX-3,
	NEXTMAP_CEREMONY = INT16_MAX-4,
	NEXTMAP_VOTING = INT16_MAX-5,
	NEXTMAP_TUTORIALCHALLENGE = INT16_MAX-6,
	NEXTMAP_INVALID = INT16_MAX-7, // Always last
	NEXTMAP_SPECIAL = NEXTMAP_INVALID
} nextmapspecial_t;

struct roundentry_t
{
	UINT16 mapnum;				// Map number at this position
	UINT16 gametype;			// Gametype we want to play this in
	boolean encore;				// Whether this will be flipped
	boolean rankrestricted;		// For grand prix progression
	boolean overridden;			// For nextmapoverride
};

extern struct roundqueue
{
	UINT8 roundnum;							// Visible number on HUD
	UINT8 position;							// Head position in the round queue
	UINT8 size;								// Number of entries in the round queue
	boolean netcommunicate;					// As server, should we net-communicate this in XD_MAP?
	boolean writetextmap;					// This queue is for automated map conversion
	boolean snapshotmaps;					// This queue is for automated map thumbnails
	roundentry_t entries[ROUNDQUEUE_MAX];	// Entries in the round queue
} roundqueue;

extern struct menuqueue
{
	// Degenerate version of roundqueue exclusively for menu use.
	UINT8 size;
	UINT8 sending;
	UINT8 anchor;
	boolean clearing;
	boolean cupqueue;
	roundentry_t entries[ROUNDQUEUE_MAX];
} menuqueue;

void G_MapSlipIntoRoundQueue(UINT8 position, UINT16 map, UINT8 setgametype, boolean setencore, boolean rankrestricted);
void G_MapIntoRoundQueue(UINT16 map, UINT8 setgametype, boolean setencore, boolean rankrestricted);
void G_GPCupIntoRoundQueue(cupheader_t *cup, UINT8 setgametype, boolean setencore);

extern INT32 gameovertics;
extern UINT8 ammoremovaltics;
extern tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)
extern INT32 pausedelay;
extern boolean pausebreakkey;

extern boolean usedTourney;

extern boolean promptactive;

extern consvar_t cv_tutorialprompt;

extern consvar_t cv_chatwidth, cv_chatnotifications, cv_chatheight, cv_chattime, cv_consolechat, cv_chatbacktint, cv_chatspamprotection;
extern consvar_t cv_shoutname, cv_shoutcolor, cv_autoshout;
extern consvar_t cv_songcredits;

extern consvar_t cv_pauseifunfocused;

extern consvar_t cv_kickstartaccel[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_autoroulette[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_litesteer[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_strictfastfall[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_autoring[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_shrinkme[MAXSPLITSCREENPLAYERS];

extern consvar_t cv_deadzone[MAXSPLITSCREENPLAYERS];

extern consvar_t cv_descriptiveinput[MAXSPLITSCREENPLAYERS];

extern consvar_t cv_ghost_besttime, cv_ghost_bestlap, cv_ghost_last, cv_ghost_guest, cv_ghost_staff;

// mouseaiming (looking up/down with the mouse or keyboard)
#define KB_LOOKSPEED (1<<25)
#define MAXPLMOVE (50)
#define SLOWTURNTICS (6)

const char *G_BuildMapName(INT32 map);
INT32 G_MapNumber(const char *mapname);

void G_BuildTiccmd(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer);

// copy ticcmd_t to and fro the normal way
ticcmd_t *G_CopyTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n);
// copy ticcmd_t to and fro network packets
ticcmd_t *G_MoveTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n);

// clip the console player aiming to the view
INT32 G_ClipAimingPitch(INT32 *aiming);
INT16 G_SoftwareClipAimingPitch(INT32 *aiming);
void G_FinalClipAimingPitch(INT32 *aiming, player_t *player, boolean skybox);

extern angle_t localangle[MAXSPLITSCREENPLAYERS];
extern INT32 localaiming[MAXSPLITSCREENPLAYERS]; // should be an angle_t but signed
extern INT32 localsteering[MAXSPLITSCREENPLAYERS];

INT32 G_PlayerInputAnalog(UINT8 p, INT32 gc, UINT8 menuPlayers);
boolean G_PlayerInputDown(UINT8 p, INT32 gc, UINT8 menuPlayers);

//
// GAME
//
void G_ChangePlayerReferences(mobj_t *oldmo, mobj_t *newmo);
void G_DoReborn(INT32 playernum);
void G_PlayerReborn(INT32 player, boolean betweenmaps);
void G_InitNew(UINT8 pencoremode, INT32 map, boolean resetplayer,
	boolean skipprecutscene);
char *G_BuildMapTitle(INT32 mapnum);

struct searchdim
{
	UINT8 pos;
	UINT8 siz;
};

struct mapsearchfreq_t
{
	INT16  mapnum;
	UINT8  matchc;
	struct searchdim *matchd;/* offset that a pattern was matched */
	UINT8  keywhc;
	struct searchdim *keywhd;/* ...in KEYWORD */
	UINT8  total;/* total hits */
};

INT32 G_FindMap(const char *query, char **foundmapnamep,
		mapsearchfreq_t **freqp, INT32 *freqc);
void G_FreeMapSearch(mapsearchfreq_t *freq, INT32 freqc);

/* Match map name by search + 2 digit map code or map number. */
INT32 G_FindMapByNameOrCode(const char *query, char **foundmapnamep);

// XMOD spawning
mapthing_t *G_FindTeamStart(INT32 playernum);
mapthing_t *G_FindBattleStart(INT32 playernum);
mapthing_t *G_FindRaceStart(INT32 playernum);
mapthing_t *G_FindPodiumStart(INT32 playernum);
mapthing_t *G_FindMapStart(INT32 playernum);
void G_MovePlayerToSpawnOrCheatcheck(INT32 playernum);
void G_SpawnPlayer(INT32 playernum);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1, but a warp test can start elsewhere
void G_DeferedInitNew(boolean pencoremode, INT32 map, INT32 pickedchar,
	UINT8 ssplayers, boolean FLS);
void G_DoLoadLevelEx(boolean resetplayer, gamestate_t newstate);
void G_DoLoadLevel(boolean resetplayer);

void G_StartTitleCard(void);
void G_PreLevelTitleCard(void);
boolean G_IsTitleCardAvailable(void);

void G_HandleSaveLevel(boolean removecondition);
void G_SaveGame(void);
void G_LoadGame(void);
void G_GetBackupCupData(boolean actuallygetdata);

void G_SaveGameData(void);
void G_DirtyGameData(void);

void G_SetGametype(INT16 gametype);
char *G_PrepareGametypeConstant(const char *newgtconst);
void G_AddTOL(UINT32 newtol, const char *tolname);
INT32 G_GetGametypeByName(const char *gametypestr);
INT32 G_GuessGametypeByTOL(UINT32 tol);

boolean G_GametypeUsesLives(void);
boolean G_GametypeAllowsRetrying(void);
boolean G_GametypeHasTeams(void);
boolean G_GametypeHasSpectators(void);
INT16 G_SometimesGetDifferentEncore(void);
void G_BeginLevelExit(void);
void G_FinishExitLevel(void);
void G_NextLevel(void);
void G_GetNextMap(void);
void G_Continue(void);
void G_UseContinue(void);
void G_AfterIntermission(void);
void G_EndGame(void); // moved from y_inter.c/h and renamed

#define MAXMEDALVISIBLECOUNT 4
extern struct stickermedalinfo
{
	UINT8 visiblecount;
	UINT8 platinumcount;
	UINT8 jitter;
	boolean norecord;
	tic_t timetoreach;
	emblem_t *emblems[MAXMEDALVISIBLECOUNT];
	emblem_t *regenemblem;
	char targettext[9];
} stickermedalinfo;

void G_UpdateTimeStickerMedals(UINT16 map, boolean showownrecord);
void G_TickTimeStickerMedals(void);
void G_UpdateRecords(void);

void G_UpdatePlayerPreferences(player_t *const player);
void G_UpdateAllPlayerPreferences(void);

void G_Ticker(boolean run);
boolean G_Responder(event_t *ev);

boolean G_CouldView(INT32 playernum);
boolean G_CanView(INT32 playernum, UINT8 viewnum, boolean onlyactive);

INT32 G_FindView(INT32 startview, UINT8 viewnum, boolean onlyactive, boolean reverse);
INT32 G_CountPlayersPotentiallyViewable(boolean active);

void G_ResetViews(void);
void G_ResetView(UINT8 viewnum, INT32 playernum, boolean onlyactive);
void G_AdjustView(UINT8 viewnum, INT32 offset, boolean onlyactive);
void G_FixCamera(UINT8 viewnum);

void G_AddPlayer(INT32 playernum, INT32 console);
void G_SpectatePlayerOnJoin(INT32 playernum);

void G_SetExitGameFlag(void);
void G_ClearExitGameFlag(void);
boolean G_GetExitGameFlag(void);

void G_SetRetryFlag(void);
void G_ClearRetryFlag(void);
boolean G_GetRetryFlag(void);

boolean G_IsModeAttackRetrying(void);

void G_LoadGameData(void);
void G_LoadGameSettings(void);

void G_SetGameModified(boolean silent, boolean major);
void G_SetUsedCheats(void);

boolean G_TimeAttackStart(void);

// Gamedata record shit
void G_ClearRecords(void);

tic_t G_GetBestTime(INT16 map);

FUNCMATH INT32 G_TicsToHours(tic_t tics);
FUNCMATH INT32 G_TicsToMinutes(tic_t tics, boolean full);
FUNCMATH INT32 G_TicsToSeconds(tic_t tics);
FUNCMATH INT32 G_TicsToCentiseconds(tic_t tics);
FUNCMATH INT32 G_TicsToMilliseconds(tic_t tics);

// Don't split up TOL handling
UINT32 G_TOLFlag(INT32 pgametype);
UINT16 G_GetFirstMapOfGametype(UINT16 pgametype);

UINT16 G_RandMapPerPlayerCount(UINT32 tolflags, UINT16 pprevmap, boolean ignoreBuffers, boolean callAgainSoon, UINT16 *extBuffer, UINT8 numPlayers);
UINT16 G_RandMap(UINT32 tolflags, UINT16 pprevmap, boolean ignoreBuffers, boolean callAgainSoon, UINT16 *extBuffer);
void G_AddMapToBuffer(UINT16 map);

void G_UpdateVisited(void);

boolean G_SameTeam(const player_t *a, const player_t *b);
UINT8 G_CountTeam(UINT8 team);
void G_AssignTeam(player_t *const p, UINT8 new_team);
void G_AutoAssignTeam(player_t *const p);
void G_AddTeamScore(UINT8 team, INT32 amount, player_t *source);
UINT32 G_TeamOrIndividualScore(const player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
