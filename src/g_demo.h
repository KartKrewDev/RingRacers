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
/// \file  g_demo.h
/// \brief Demo recording and playback

#ifndef __G_DEMO__
#define __G_DEMO__

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"

#ifdef __cplusplus

#include "core/json.hpp"
#include "core/string.h"
#include "core/vector.hpp"

// Modern json formats
namespace srb2
{
struct StandingJson
{
	uint8_t ranking;
	String name;
	uint16_t demoskin;
	String skincolor;
	uint32_t timeorscore;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
		StandingJson,
		ranking,
		name,
		demoskin,
		skincolor,
		timeorscore
	)
};
struct StandingsJson
{
	Vector<StandingJson> standings;

	SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(StandingsJson, standings)
};

void write_current_demo_standings(const StandingsJson& standings);
void write_current_demo_end_marker();

} // namespace srb2

extern "C" {
#endif

// ======================================
// DEMO playback/recording related stuff.
// ======================================

extern consvar_t cv_recordmultiplayerdemos, cv_netdemosyncquality;

extern tic_t demostarttime;

struct democharlist_t {
	char name[SKINNAMESIZE+1];
	UINT32 namehash;
	UINT16 mapping; // No, this isn't about levels. It maps to loaded character ID.
	UINT8 kartspeed;
	UINT8 kartweight;
	UINT32 flags;
	boolean unlockrequired;
};

// Publicly-accessible demo vars
struct demovars_s {
	char titlename[65];
	boolean recording, playback, timing;
	UINT16 version; // Current file format of the demo being played
	UINT8 attract; // Attract demo can be cancelled by any key
	UINT8 simplerewind;

	boolean loadfiles, ignorefiles; // Demo file loading options
	boolean quitafterplaying; // quit after playing a demo from cmdline
	boolean deferstart; // don't start playing demo right away
	boolean netgame; // multiplayer netgame
	boolean waitingfortally; // demo ended but we're keeping the level open for the tally to finish

	tic_t savebutton; // Used to determine when the local player can choose to save the replay while the race is still going
	boolean willsave;

	boolean freecam;

	UINT16 numskins;
	democharlist_t *skinlist;
	UINT16 currentskinid[MAXPLAYERS];

	const savebuffer_t *buffer; // debug, valid only if recording or playback
};

extern struct demovars_s demo;

typedef enum {
	MD_NOTLOADED,
	MD_LOADED,
	MD_SUBDIR,
	MD_OUTDATED,
	MD_INVALID
} menudemotype_e;

struct menudemo_t {
	char filepath[1023 + 256]; // see M_PrepReplayList and sizeof menupath
	menudemotype_e type;

	char title[65]; // Null-terminated for string prints
	UINT16 map;
	UINT8 addonstatus; // What do we need to do addon-wise to play this demo?
	INT16 gametype;
	SINT8 kartspeed; // Add OR DF_ENCORE for encore mode, idk
	UINT8 numlaps;
	UINT8 gp;

	struct {
		UINT8 ranking;
		char name[MAXPLAYERNAME+1];
		UINT16 skin, color;
		UINT32 timeorscore;
	} standings[MAXPLAYERS];
};


// Only called by startup code.
void G_RecordDemo(const char *name);
void G_BeginRecording(void);

// Only called by shutdown code.
void G_SetDemoTime(UINT32 ptime, UINT32 plap);
UINT8 G_CmpDemoTime(char *oldname, char *newname);

typedef enum
{
	GHC_NORMAL = 0,
	GHC_INVINCIBLE,
	GHC_SUPER
} ghostcolor_t;

extern UINT8 demo_extradata[MAXPLAYERS];
extern UINT8 demo_writerng;

#define DXD_JOINDATA   0x01 // join-specific data
#define DXD_PLAYSTATE  0x02 // state changed between playing, spectating, or not in-game
#define DXD_SKIN       0x04 // skin changed
#define DXD_NAME       0x08 // name changed
#define DXD_COLOR      0x10 // color changed
#define DXD_FOLLOWER   0x20 // follower was changed

#define DXD_ADDPLAYER (DXD_JOINDATA|DXD_PLAYSTATE|DXD_COLOR|DXD_NAME|DXD_SKIN|DXD_FOLLOWER)

#define DXD_WEAPONPREF 0x80 // netsynced playsim settings were changed

#define DXD_PST_PLAYING    0x01
#define DXD_PST_SPECTATING 0x02
#define DXD_PST_LEFT       0x03

#define MAXSPLITS (32)

boolean G_CompatLevel(UINT16 level);

// Record/playback tics
boolean G_ConsiderEndingDemoRead(void);
boolean G_ConsiderEndingDemoWrite(void);
void G_ReadDemoExtraData(void);
void G_WriteDemoExtraData(void);
void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum);
void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum);
void G_GhostAddColor(INT32 playernum, ghostcolor_t color);
void G_GhostAddFlip(INT32 playernum);
void G_GhostAddScale(INT32 playernum, fixed_t scale);
void G_GhostAddHit(INT32 playernum, mobj_t *victim);
void G_WriteAllGhostTics(void);
void G_WriteGhostTic(mobj_t *ghost, INT32 playernum);
void G_ConsAllGhostTics(void);
void G_ConsGhostTic(INT32 playernum);
void G_GhostTicker(void);

struct DemoBufferSizes
{
	size_t player_name;
	size_t skin_name;
	size_t color_name;
	size_t availability;
};

// Your naming conventions are stupid and useless.
// There is no conflict here.
struct demoghost {
	UINT8 checksum[16];
	UINT8 *buffer, *p, color;
	UINT16 initialskin;
	UINT16 initialcolor;
	UINT8 fadein;
	UINT16 version;
	UINT16 numskins;
	tic_t attackstart;
	tic_t splits[MAXSPLITS];
	boolean done;
	democharlist_t *skinlist;
	mobj_t oldmo, *mo;
	struct DemoBufferSizes sizes;
	struct demoghost *next;
};
extern demoghost *ghosts;

// G_CheckDemoExtraFiles: checks if our loaded WAD list matches the demo's.
#define DFILE_ERROR_NOTLOADED            0x01 // Files are not loaded, but can be without a restart.
#define DFILE_ERROR_OUTOFORDER           0x02 // Files are loaded, but out of order.
#define DFILE_ERROR_INCOMPLETEOUTOFORDER 0x03 // Some files are loaded out of order, but others are not.
#define DFILE_ERROR_CANNOTLOAD           0x04 // Files are missing and cannot be loaded.
#define DFILE_ERROR_EXTRAFILES           0x05 // Extra files outside of the replay's file list are loaded.
#define DFILE_ERROR_CORRUPT              0x06 // Demo file is corrupted

void G_DeferedPlayDemo(const char *demo);
void G_DoPlayDemoEx(const char *defdemoname, lumpnum_t deflumpnum);
#define G_DoPlayDemo(defdemoname) G_DoPlayDemoEx(defdemoname, LUMPERROR)
void G_TimeDemo(const char *name);
void G_AddGhost(savebuffer_t *buffer, const char *defdemoname);
staffbrief_t *G_GetStaffGhostBrief(UINT8 *buffer);
void G_FreeGhosts(void);
void G_DoneLevelLoad(void);

void G_StopDemo(void);
boolean G_CheckDemoStatus(void);

void G_LoadDemoInfo(menudemo_t *pdemo, boolean allownonmultiplayer);
void G_DeferedPlayDemo(const char *demo);

void G_SaveDemo(void);
void G_ResetDemoRecording(void);

void G_SetDemoAttackTiming(tic_t time);
void G_SetDemoCheckpointTiming(player_t *player, tic_t time, UINT8 checkpoint);

boolean G_CheckDemoTitleEntry(void);

typedef enum
{
	DEMO_ATTRACT_OFF = 0,
	DEMO_ATTRACT_TITLE,
	DEMO_ATTRACT_CREDITS
} demoAttractMode_t;

typedef enum
{
	DEMO_REWIND_OFF = 0,
	DEMO_REWIND_RESUME,
	DEMO_REWIND_PAUSE
} demoRewindMode_t;

void G_SyncDemoParty(INT32 rem, INT32 newsplitscreen);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __G_DEMO__
