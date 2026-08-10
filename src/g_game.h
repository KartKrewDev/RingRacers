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

#ifndef G_GAME_H
#define G_GAME_H

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
extern int32_t player_name_changes[MAXPLAYERS];

extern player_t players[MAXPLAYERS];
extern dboolean playeringame[MAXPLAYERS];

// gametic at level start
extern tic_t levelstarttic;

// for modding?
extern uint16_t prevmap, nextmap;

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
	uint16_t mapnum;				// Map number at this position
	uint16_t gametype;			// Gametype we want to play this in
	dboolean encore;				// Whether this will be flipped
	dboolean rankrestricted;		// For grand prix progression
	dboolean overridden;			// For nextmapoverride
};

extern struct roundqueue
{
	uint8_t roundnum;							// Visible number on HUD
	uint8_t position;							// Head position in the round queue
	uint8_t size;								// Number of entries in the round queue
	dboolean netcommunicate;					// As server, should we net-communicate this in XD_MAP?
	dboolean writetextmap;					// This queue is for automated map conversion
	dboolean snapshotmaps;					// This queue is for automated map thumbnails
	roundentry_t entries[ROUNDQUEUE_MAX];	// Entries in the round queue
} roundqueue;

extern struct menuqueue
{
	// Degenerate version of roundqueue exclusively for menu use.
	uint8_t size;
	uint8_t sending;
	uint8_t anchor;
	dboolean clearing;
	dboolean cupqueue;
	roundentry_t entries[ROUNDQUEUE_MAX];
} menuqueue;

void G_MapSlipIntoRoundQueue(uint8_t position, uint16_t map, uint8_t setgametype, dboolean setencore, dboolean rankrestricted);
void G_MapIntoRoundQueue(uint16_t map, uint8_t setgametype, dboolean setencore, dboolean rankrestricted);
void G_GPCupIntoRoundQueue(cupheader_t *cup, uint8_t setgametype, dboolean setencore);

extern int32_t gameovertics;
extern uint8_t ammoremovaltics;
extern tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)
extern int32_t pausedelay;
extern dboolean pausebreakkey;

extern dboolean usedTourney;

extern dboolean promptactive;

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

const char *G_BuildMapName(int32_t map);
int32_t G_MapNumber(const char *mapname);

void G_BuildTiccmd(ticcmd_t *cmd, int32_t realtics, uint8_t ssplayer);

// copy ticcmd_t to and fro the normal way
ticcmd_t *G_CopyTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n);
// copy ticcmd_t to and fro network packets
ticcmd_t *G_MoveTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n);

// clip the console player aiming to the view
int32_t G_ClipAimingPitch(int32_t *aiming);
int16_t G_SoftwareClipAimingPitch(int32_t *aiming);
void G_FinalClipAimingPitch(int32_t *aiming, player_t *player, dboolean skybox);

extern angle_t localangle[MAXSPLITSCREENPLAYERS];
extern int32_t localaiming[MAXSPLITSCREENPLAYERS]; // should be an angle_t but signed
extern int32_t localsteering[MAXSPLITSCREENPLAYERS];

int32_t G_PlayerInputAnalog(uint8_t p, int32_t gc, uint8_t menuPlayers);
dboolean G_PlayerInputDown(uint8_t p, int32_t gc, uint8_t menuPlayers);

//
// GAME
//
void G_ChangePlayerReferences(mobj_t *oldmo, mobj_t *newmo);
void G_DoReborn(int32_t playernum);
void G_PlayerReborn(int32_t player, dboolean betweenmaps);
void G_InitNew(uint8_t pencoremode, int32_t map, dboolean resetplayer,
	dboolean skipprecutscene);
char *G_BuildMapTitle(int32_t mapnum);

struct searchdim
{
	uint8_t pos;
	uint8_t siz;
};

struct mapsearchfreq_t
{
	int16_t  mapnum;
	uint8_t  matchc;
	struct searchdim *matchd;/* offset that a pattern was matched */
	uint8_t  keywhc;
	struct searchdim *keywhd;/* ...in KEYWORD */
	uint8_t  total;/* total hits */
};

int32_t G_FindMap(const char *query, char **foundmapnamep,
		mapsearchfreq_t **freqp, int32_t *freqc);
void G_FreeMapSearch(mapsearchfreq_t *freq, int32_t freqc);

/* Match map name by search + 2 digit map code or map number. */
int32_t G_FindMapByNameOrCode(const char *query, char **foundmapnamep);

// XMOD spawning
mapthing_t *G_FindTeamStart(int32_t playernum);
mapthing_t *G_FindBattleStart(int32_t playernum);
mapthing_t *G_FindRaceStart(int32_t playernum);
mapthing_t *G_FindPodiumStart(int32_t playernum);
mapthing_t *G_FindMapStart(int32_t playernum);
void G_MovePlayerToSpawnOrCheatcheck(int32_t playernum);
void G_SpawnPlayer(int32_t playernum);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1, but a warp test can start elsewhere
void G_DeferedInitNew(dboolean pencoremode, int32_t map, int32_t pickedchar,
	uint8_t ssplayers, dboolean FLS);
void G_DoLoadLevelEx(dboolean resetplayer, gamestate_t newstate);
void G_DoLoadLevel(dboolean resetplayer);

void G_StartTitleCard(void);
void G_PreLevelTitleCard(void);
dboolean G_IsTitleCardAvailable(void);

void G_HandleSaveLevel(dboolean removecondition);
void G_SaveGame(void);
void G_LoadGame(void);
void G_GetBackupCupData(dboolean actuallygetdata);

void G_SaveGameData(void);
void G_DirtyGameData(void);

void G_SetGametype(int16_t gametype);
char *G_PrepareGametypeConstant(const char *newgtconst);
void G_AddTOL(uint32_t newtol, const char *tolname);
int32_t G_GetGametypeByName(const char *gametypestr);
int32_t G_GuessGametypeByTOL(uint32_t tol);

dboolean G_GametypeUsesLives(void);
dboolean G_GametypeAllowsRetrying(void);
dboolean G_GametypeHasTeams(void);
dboolean G_GametypeHasSpectators(void);
int16_t G_SometimesGetDifferentEncore(void);
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
	uint8_t visiblecount;
	uint8_t platinumcount;
	uint8_t jitter;
	dboolean norecord;
	tic_t timetoreach;
	emblem_t *emblems[MAXMEDALVISIBLECOUNT];
	emblem_t *regenemblem;
	char targettext[9];
} stickermedalinfo;

void G_UpdateTimeStickerMedals(uint16_t map, dboolean showownrecord);
void G_TickTimeStickerMedals(void);
void G_UpdateRecords(void);

void G_UpdatePlayerPreferences(player_t *const player);
void G_UpdateAllPlayerPreferences(void);

void G_Ticker(dboolean run);
dboolean G_Responder(event_t *ev);

dboolean G_CouldView(int32_t playernum);
dboolean G_CanView(int32_t playernum, uint8_t viewnum, dboolean onlyactive);

int32_t G_FindView(int32_t startview, uint8_t viewnum, dboolean onlyactive, dboolean reverse);
int32_t G_CountPlayersPotentiallyViewable(dboolean active);

void G_ResetViews(void);
void G_ResetView(uint8_t viewnum, int32_t playernum, dboolean onlyactive);
void G_AdjustView(uint8_t viewnum, int32_t offset, dboolean onlyactive);
void G_FixCamera(uint8_t viewnum);

void G_AddPlayer(int32_t playernum, int32_t console);
void G_SpectatePlayerOnJoin(int32_t playernum);

void G_SetExitGameFlag(void);
void G_ClearExitGameFlag(void);
dboolean G_GetExitGameFlag(void);

void G_SetRetryFlag(void);
void G_ClearRetryFlag(void);
dboolean G_GetRetryFlag(void);

dboolean G_IsModeAttackRetrying(void);

void G_LoadGameData(void);
void G_LoadGameSettings(void);

void G_SetGameModified(dboolean silent, dboolean major);
void G_SetUsedCheats(void);

dboolean G_TimeAttackStart(void);

// Gamedata record shit
void G_ClearRecords(void);

tic_t G_GetBestTime(int16_t map);

FUNCMATH int32_t G_TicsToHours(tic_t tics);
FUNCMATH int32_t G_TicsToMinutes(tic_t tics, dboolean full);
FUNCMATH int32_t G_TicsToSeconds(tic_t tics);
FUNCMATH int32_t G_TicsToCentiseconds(tic_t tics);
FUNCMATH int32_t G_TicsToMilliseconds(tic_t tics);

// Don't split up TOL handling
uint32_t G_TOLFlag(int32_t pgametype);
uint16_t G_GetFirstMapOfGametype(uint16_t pgametype);

uint16_t G_RandMapPerPlayerCount(uint32_t tolflags, uint16_t pprevmap, dboolean ignoreBuffers, dboolean callAgainSoon, uint16_t *extBuffer, uint8_t numPlayers);
uint16_t G_RandMap(uint32_t tolflags, uint16_t pprevmap, dboolean ignoreBuffers, dboolean callAgainSoon, uint16_t *extBuffer);
void G_AddMapToBuffer(uint16_t map);

void G_UpdateVisited(void);

dboolean G_SameTeam(const player_t *a, const player_t *b);
uint8_t G_CountTeam(uint8_t team);
void G_AssignTeam(player_t *const p, uint8_t new_team);
void G_AutoAssignTeam(player_t *const p);
void G_AddTeamScore(uint8_t team, int32_t amount, player_t *source);
uint32_t G_TeamOrIndividualScore(const player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
