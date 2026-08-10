// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.h
/// \brief host/client network commands
///        commands are executed through the command buffer
///        like console commands

#ifndef D_NETCMD_H
#define D_NETCMD_H

#include "command.h"
#include "d_player.h"

#ifdef __cplusplus
extern "C" {
#endif

// console vars
extern consvar_t cv_playername[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_playercolor[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_skin[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_follower[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_followercolor[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_lastprofile[MAXSPLITSCREENPLAYERS];

// current profile loaded.
// Used to know how to make the options menu behave among other things.
extern consvar_t cv_currprofile;

// This is used to save the last profile you used on the title screen.
// that way you can mash n all...
extern consvar_t cv_ttlprofilen;

// CVar that allows starting as many splitscreens as you want with one device
// Intended for use with testing
extern consvar_t cv_splitdevice;

// preferred number of players
extern consvar_t cv_splitplayers;

extern consvar_t cv_seenames;
extern consvar_t cv_usemouse;
extern consvar_t cv_joyscale[MAXSPLITSCREENPLAYERS];

extern consvar_t cv_pointlimit;
extern consvar_t cv_timelimit;
extern consvar_t cv_dueltimelimit, cv_duelscorelimit;
extern consvar_t cv_numlaps;
extern uint32_t timelimitintics, extratimeintics, secretextratime;
extern uint32_t g_pointlimit;
extern consvar_t cv_allowexitlevel;

extern consvar_t cv_netstat;

extern consvar_t cv_countdowntime;
extern consvar_t cv_mute;
extern consvar_t cv_voice_allowservervoice;
extern consvar_t cv_pause;

extern consvar_t cv_restrictskinchange, cv_allowteamchange, cv_maxplayers, cv_shuffleloser;
extern consvar_t cv_spectatorreentry, cv_duelspectatorreentry, cv_antigrief;

// SRB2kart items
extern consvar_t cv_items[NUMKARTRESULTS-1];

extern consvar_t cv_kartspeed;
extern consvar_t cv_kartbumpers;
extern consvar_t cv_kartfrantic;
extern consvar_t cv_kartencore;
extern consvar_t cv_kartspeedometer;
extern consvar_t cv_kartvoices;
extern consvar_t cv_karthorns;
extern consvar_t cv_kartbot;
extern consvar_t cv_karteliminatelast;
extern consvar_t cv_thunderdome;
extern consvar_t cv_teamplay;
extern consvar_t cv_duel;
extern consvar_t cv_kartusepwrlv;
#ifdef DEVELOP
	extern consvar_t cv_kartencoremap;
#endif

extern consvar_t cv_votetime;
extern consvar_t cv_botscanvote;

extern consvar_t cv_kartdebugitem, cv_kartdebugamount, cv_kartdebugdistribution, cv_kartdebughuddrop;
extern consvar_t cv_kartdebugnodes, cv_kartdebugcolorize, cv_kartdebugdirector;
extern consvar_t cv_spbtest, cv_reducevfx, cv_screenshake;
extern consvar_t cv_kartdebugwaypoints, cv_kartdebugbots;
extern consvar_t cv_kartdebugbotwhip;
extern consvar_t cv_kartdebugstart;
extern consvar_t cv_debugrank;
extern consvar_t cv_battletest;

extern consvar_t cv_bighead;
extern consvar_t cv_levelskull;
extern consvar_t cv_shittysigns;
extern consvar_t cv_tastelesstaunts;
extern consvar_t cv_4thgear;

typedef enum {
	CV_CAPSULETEST_OFF,
	CV_CAPSULETEST_MULTIPLAYER,
	CV_CAPSULETEST_TIMEATTACK,
} capsuletest_val_t;
extern consvar_t cv_capsuletest;

extern consvar_t cv_itemfinder;

extern consvar_t cv_inttime, cv_advancemap;
extern consvar_t cv_overtime;

// for F_finale.c
extern consvar_t cv_rollingdemos;

extern consvar_t cv_soundtest;

extern consvar_t cv_maxping;
extern consvar_t cv_lagless;
extern consvar_t cv_pingmeasurement;
extern consvar_t cv_showviewpointtext;

extern consvar_t cv_skipmapcheck;

extern consvar_t cv_sleep;

extern consvar_t cv_perfstats;

extern consvar_t cv_schedule;

extern consvar_t cv_livestudioaudience;

extern char timedemo_name[256];
extern dboolean timedemo_csv;
extern char timedemo_csv_id[256];
extern dboolean timedemo_quit;

typedef enum
{
	XD_NAMEANDCOLOR = 1,
	XD_WEAPONPREF,  // 2
	XD_KICK,        // 3
	XD_NETVAR,      // 4
	XD_SAY,         // 5
	XD_MAP,         // 6
	XD_EXITLEVEL,   // 7
	XD_ADDFILE,     // 8
	XD_PAUSE,       // 9
	XD_ADDPLAYER,   // 10
	XD_SPECTATE,    // 11
	XD_SETSCORE,    // 12
	XD_VERIFIED,    // 13
	XD_RANDOMSEED,  // 14
	XD_RUNSOC,      // 15
	XD_REQADDFILE,  // 16
	XD_SETMOTD,     // 17
	XD_DEMOTED,     // 18
	XD_LUACMD,      // 19
	XD_LUAVAR,      // 20
	XD_LUAFILE,     // 21

	// Ring Racers
	XD_SETUPVOTE,   // 22
	XD_MODIFYVOTE,  // 23
	XD_PICKVOTE,    // 24
	XD_REMOVEPLAYER,// 25
	XD_PARTYINVITE, // 26
	XD_ACCEPTPARTYINVITE, // 27
	XD_LEAVEPARTY,  // 28
	XD_CANCELPARTYINVITE, // 29
	XD_CHEAT,       // 30
	XD_ADDBOT,      // 31
	XD_DISCORD,     // 32
	XD_PLAYSOUND,   // 33
	XD_SCHEDULETASK, // 34
	XD_SCHEDULECLEAR, // 35
	XD_AUTOMATE,    // 36
	XD_MAPQUEUE = XD_AUTOMATE+2, // 38
	XD_CALLZVOTE,   // 39
	XD_SETZVOTE,    // 40
	XD_TEAMCHANGE,  // 41
	XD_SERVERMUTEPLAYER, // 42
	XD_SERVERDEAFENPLAYER, // 43
	XD_SERVERTEMPMUTEPLAYER, // 44

	MAXNETXCMD
} netxcmd_t;

extern const char *netxcmdnames[MAXNETXCMD - 1];

// add game commands, needs cleanup
void D_RegisterServerCommands(void);
void D_RegisterClientCommands(void);
void CleanupPlayerName(int32_t playernum, const char *newname);
dboolean IsPlayerNameUnique(const char *name, int32_t playernum);
dboolean IsPlayerNameGood(char *name);
dboolean EnsurePlayerNameIsGood(char *name, int32_t playernum);
void D_FillPlayerSkinAndColor(const uint8_t n, const player_t *player, player_config_t *config);
void D_PlayerChangeSkinAndColor(player_t *player, uint16_t skin, uint16_t color, int16_t follower, uint16_t followercolor);
void D_FillPlayerWeaponPref(const uint8_t n, player_config_t *config);
void WeaponPref_Send(uint8_t ssplayer);
void WeaponPref_Set(int32_t playernum, uint8_t prefs);
void WeaponPref_Save(uint8_t **cp, int32_t playernum);
size_t WeaponPref_Parse(const uint8_t *p, int32_t playernum);
void D_SendPlayerConfig(uint8_t n);
void Command_ExitGame_f(void);
void Command_Retry_f(void);
void Handle_MapQueueSend(uint16_t newmapnum, uint16_t newgametype, dboolean newencoremode);
dboolean G_GamestateUsesExitLevel(void);
void D_GameTypeChanged(int32_t lastgametype); // not a real _OnChange function anymore
void D_MapChange(uint16_t pmapnum, int32_t pgametype, dboolean pencoremode, dboolean presetplayers, int32_t pdelay, dboolean pskipprecutscene, dboolean pforcespecialstage);
void D_SetupVote(int16_t newgametype);
void D_ModifyClientVote(uint8_t player, int8_t voted);
void D_PickVote(int8_t angry_map);
void ObjectPlace_OnChange(void);
void P_SetPlayerSpectator(int32_t playernum);
dboolean IsPlayerAdmin(int32_t playernum);
void SetAdminPlayer(int32_t playernum);
void ClearAdminPlayers(void);
void RemoveAdminPlayer(int32_t playernum);
void ItemFinder_OnChange(void);
void D_SetPassword(const char *pw);

struct scheduleTask_t
{
	uint16_t basetime;
	uint16_t timer;
	char *command;
};

extern scheduleTask_t **schedule;
extern size_t schedule_size;
extern size_t schedule_len;

void Schedule_Run(void);
void Schedule_Insert(scheduleTask_t *addTask);
void Schedule_Add(int16_t basetime, int16_t timeleft, const char *command);
void Schedule_Clear(void);

typedef enum
{
	AEV_ROUNDSTART,
	AEV_INTERMISSIONSTART,
	AEV_VOTESTART,
	AEV_QUEUESTART,
	AEV_QUEUEEND,
	AEV__MAX
} automateEvents_t;

void Automate_Run(automateEvents_t type);
void Automate_Set(automateEvents_t type, const char *command);
void Automate_Clear(void);

extern uint32_t livestudioaudience_timer;
void LiveStudioAudience(void);

typedef enum
{
	SYNC_NONE,
	SYNC_RNG,
	SYNC_HEALTH,
	SYNC_POSITION,
	SYNC_ITEM,
	SYNC_CHARACTER,
	SYNC__MAX
} staffsync_reason_t;

void D_Cheat(int32_t playernum, int32_t cheat, ...);

// used for the player setup menu
dboolean CanChangeSkin(int32_t playernum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
