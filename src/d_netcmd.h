// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.h
/// \brief host/client network commands
///        commands are executed through the command buffer
///        like console commands

#ifndef __D_NETCMD__
#define __D_NETCMD__

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
extern consvar_t cv_numlaps;
extern UINT32 timelimitintics, extratimeintics, secretextratime;
extern UINT32 g_pointlimit;
extern consvar_t cv_allowexitlevel;

extern consvar_t cv_autobalance;
extern consvar_t cv_teamscramble;
extern consvar_t cv_scrambleonchange;

extern consvar_t cv_netstat;

extern consvar_t cv_countdowntime;
extern consvar_t cv_mute;
extern consvar_t cv_pause;

extern consvar_t cv_restrictskinchange, cv_allowteamchange, cv_maxplayers;
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
extern consvar_t cv_kartdebugstart;
extern consvar_t cv_debugrank;
extern consvar_t cv_battletest;

extern consvar_t cv_mentalsonic, cv_4thgear;

typedef enum {
	CV_CAPSULETEST_OFF,
	CV_CAPSULETEST_MULTIPLAYER,
	CV_CAPSULETEST_TIMEATTACK,
} capsuletest_val_t;
extern consvar_t cv_capsuletest;

extern consvar_t cv_alttitle, cv_itemfinder;

extern consvar_t cv_inttime, cv_advancemap;
extern consvar_t cv_overtime;

// for F_finale.c
extern consvar_t cv_rollingdemos;

extern consvar_t cv_soundtest;

extern consvar_t cv_maxping;
extern consvar_t cv_lagless;
extern consvar_t cv_pingtimeout;
extern consvar_t cv_showping;
extern consvar_t cv_pingmeasurement;
extern consvar_t cv_showviewpointtext;

extern consvar_t cv_skipmapcheck;

extern consvar_t cv_sleep;

extern consvar_t cv_perfstats;

extern consvar_t cv_schedule;

extern consvar_t cv_livestudioaudience;

extern char timedemo_name[256];
extern boolean timedemo_csv;
extern char timedemo_csv_id[256];
extern boolean timedemo_quit;

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
	XD_TEAMCHANGE,  // 11
	XD_CLEARSCORES, // 12
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
	XD_REQMAPQUEUE, // 37
	XD_MAPQUEUE,	// 38
	XD_CALLZVOTE,   // 39
	XD_SETZVOTE,    // 40

	MAXNETXCMD
} netxcmd_t;

extern const char *netxcmdnames[MAXNETXCMD - 1];

#if defined(_MSC_VER)
#pragma pack(1)
#endif

#ifdef _MSC_VER
#pragma warning(disable :  4214)
#endif

//Packet composition for Command_TeamChange_f() ServerTeamChange, etc.
//bitwise structs make packing bits a little easier, but byte alignment harder?
//todo: decide whether to make the other netcommands conform, or just get rid of this experiment.
struct changeteam_packet_t {
	UINT32 playernum    : 5;  // value 0 to 31
	UINT32 newteam      : 5;  // value 0 to 31
	UINT32 verification : 1;  // value 0 to 1
	UINT32 autobalance  : 1;  // value 0 to 1
	UINT32 scrambled    : 1;  // value 0 to 1
} ATTRPACK;

#ifdef _MSC_VER
#pragma warning(default : 4214)
#endif

struct changeteam_value_t {
	UINT16 l; // liitle endian
	UINT16 b; // big enian
} ATTRPACK;

//Since we do not want other files/modules to know about this data buffer we union it here with a Short Int.
//Other files/modules will hand the INT16 back to us and we will decode it here.
//We don't have to use a union, but we would then send four bytes instead of two.
typedef union {
	changeteam_packet_t packet;
	changeteam_value_t value;
} ATTRPACK changeteam_union;

#if defined(_MSC_VER)
#pragma pack()
#endif

// add game commands, needs cleanup
void D_RegisterServerCommands(void);
void D_RegisterClientCommands(void);
void CleanupPlayerName(INT32 playernum, const char *newname);
boolean EnsurePlayerNameIsGood(char *name, INT32 playernum);
void WeaponPref_Send(UINT8 ssplayer);
void WeaponPref_Save(UINT8 **cp, INT32 playernum);
size_t WeaponPref_Parse(const UINT8 *p, INT32 playernum);
void D_SendPlayerConfig(UINT8 n);
void Command_ExitGame_f(void);
void Command_Retry_f(void);
boolean G_GamestateUsesExitLevel(void);
void D_GameTypeChanged(INT32 lastgametype); // not a real _OnChange function anymore
void D_MapChange(UINT16 pmapnum, INT32 pgametype, boolean pencoremode, boolean presetplayers, INT32 pdelay, boolean pskipprecutscene, boolean pforcespecialstage);
void D_SetupVote(INT16 newgametype);
void D_ModifyClientVote(UINT8 player, SINT8 voted);
void D_PickVote(void);
void ObjectPlace_OnChange(void);
void P_SetPlayerSpectator(INT32 playernum);
boolean IsPlayerAdmin(INT32 playernum);
void SetAdminPlayer(INT32 playernum);
void ClearAdminPlayers(void);
void RemoveAdminPlayer(INT32 playernum);
void AltTitle_OnChange(void);
void ItemFinder_OnChange(void);
void D_SetPassword(const char *pw);

struct scheduleTask_t
{
	UINT16 basetime;
	UINT16 timer;
	char *command;
};

extern scheduleTask_t **schedule;
extern size_t schedule_size;
extern size_t schedule_len;

void Schedule_Run(void);
void Schedule_Insert(scheduleTask_t *addTask);
void Schedule_Add(INT16 basetime, INT16 timeleft, const char *command);
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

extern UINT32 livestudioaudience_timer;
void LiveStudioAudience(void);

void D_Cheat(INT32 playernum, INT32 cheat, ...);

// used for the player setup menu
boolean CanChangeSkin(INT32 playernum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
