// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_game.c
/// \brief game loop functions, events handling

#include "doomdef.h"
#include "console.h"
#include "d_main.h"
#include "d_clisrv.h"
#include "d_player.h"
#include "d_clisrv.h"
#include "f_finale.h"
#include "filesrch.h" // for refreshdirmenu
#include "p_setup.h"
#include "p_saveg.h"
#include "i_system.h"
#include "am_map.h"
#include "m_random.h"
#include "p_local.h"
#include "r_draw.h"
#include "r_main.h"
#include "s_sound.h"
#include "g_game.h"
#include "g_demo.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_argv.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "z_zone.h"
#include "i_video.h"
#include "byteptr.h"
#include "i_joy.h"
#include "r_local.h"
#include "r_skins.h"
#include "y_inter.h"
#include "v_video.h"
#include "lua_hook.h"
#include "k_bot.h"
#include "m_cond.h" // condition sets
#include "lua_hud.h"

// SRB2kart
#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "k_color.h"
#include "k_respawn.h"
#include "k_grandprix.h"
#include "doomstat.h"

gameaction_t gameaction;
gamestate_t gamestate = GS_NULL;
UINT8 ultimatemode = false;

JoyType_t Joystick[MAXSPLITSCREENPLAYERS];

// 1024 bytes is plenty for a savegame
#define SAVEGAMESIZE (1024)

// SRB2kart
char gamedatafilename[64] = "kartdata.dat";
char timeattackfolder[64] = "kart";
char customversionstring[32] = "\0";

static void G_DoCompleted(void);
static void G_DoStartContinue(void);
static void G_DoContinued(void);
static void G_DoWorldDone(void);
static void G_DoStartVote(void);

char   mapmusname[7]; // Music name
UINT16 mapmusflags; // Track and reset bit
UINT32 mapmusposition; // Position to jump to
UINT32 mapmusresume;

INT16 gamemap = 1;
UINT32 maptol;

UINT8 globalweather = PRECIP_NONE;
UINT8 curWeather = PRECIP_NONE;

precipprops_t precipprops[MAXPRECIP] =
{
	{MT_NULL, 0}, // PRECIP_NONE
	{MT_RAIN, 0}, // PRECIP_RAIN
	{MT_SNOWFLAKE, 0}, // PRECIP_SNOW
	{MT_BLIZZARDSNOW, 0}, // PRECIP_BLIZZARD
	{MT_RAIN, PRECIPFX_THUNDER|PRECIPFX_LIGHTNING}, // PRECIP_STORM
	{MT_NULL, PRECIPFX_THUNDER|PRECIPFX_LIGHTNING}, // PRECIP_STORM_NORAIN
	{MT_RAIN, PRECIPFX_THUNDER} // PRECIP_STORM_NOSTRIKES
};

INT32 cursaveslot = 0; // Auto-save 1p savegame slot
//INT16 lastmapsaved = 0; // Last map we auto-saved at
INT16 lastmaploaded = 0; // Last map the game loaded
UINT8 gamecomplete = 0;

marathonmode_t marathonmode = 0;
tic_t marathontime = 0;

UINT8 numgameovers = 0; // for startinglives balance
SINT8 startinglivesbalance[maxgameovers+1] = {3, 5, 7, 9, 12, 15, 20, 25, 30, 40, 50, 75, 99, 0x7F};

UINT16 mainwads = 0;
boolean modifiedgame = false; // Set if homebrew PWAD stuff has been added.
boolean majormods = false; // Set if Lua/Gameplay SOC/replacement map has been added.
boolean savemoddata = false;
UINT8 paused;
UINT8 modeattacking = ATTACKING_NONE;
boolean imcontinuing = false;
boolean runemeraldmanager = false;
UINT16 emeraldspawndelay = 60*TICRATE;

// menu demo things
UINT8  numDemos      = 0;
UINT32 demoDelayTime = 15*TICRATE;
UINT32 demoIdleTime  = 3*TICRATE;

boolean netgame; // only true if packets are broadcast
boolean multiplayer;
boolean playeringame[MAXPLAYERS];
boolean addedtogame;
player_t players[MAXPLAYERS];

INT32 consoleplayer; // player taking events and displaying
INT32 displayplayers[MAXSPLITSCREENPLAYERS]; // view being displayed
INT32 g_localplayers[MAXSPLITSCREENPLAYERS];

tic_t gametic;
tic_t levelstarttic; // gametic at level start
UINT32 ssspheres; // old special stage
INT16 lastmap; // last level you were at (returning from special stages)
tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)

INT16 spstage_start, spmarathon_start;
INT16 sstage_start, sstage_end, smpstage_start, smpstage_end;

INT16 titlemap = 0;
boolean hidetitlepics = false;
INT16 bootmap; //bootmap for loading a map on startup

INT16 tutorialmap = 0; // map to load for tutorial
boolean tutorialmode = false; // are we in a tutorial right now?
INT32 tutorialgcs = gcs_custom; // which control scheme is loaded?

boolean looptitle = true;

UINT16 skincolor_redteam = SKINCOLOR_RED;
UINT16 skincolor_blueteam = SKINCOLOR_BLUE;
UINT16 skincolor_redring = SKINCOLOR_RASPBERRY;
UINT16 skincolor_bluering = SKINCOLOR_PERIWINKLE;

tic_t countdowntimer = 0;
boolean countdowntimeup = false;
boolean exitfadestarted = false;

cutscene_t *cutscenes[128];
textprompt_t *textprompts[MAX_PROMPTS];

INT16 nextmapoverride;
UINT8 skipstats;

// Pointers to each CTF flag
mobj_t *redflag;
mobj_t *blueflag;
// Pointers to CTF spawn location
mapthing_t *rflagpoint;
mapthing_t *bflagpoint;

struct quake quake;

// Map Header Information
mapheader_t* mapheaderinfo[NUMMAPS] = {NULL};

// Kart cup definitions
cupheader_t *kartcupheaders = NULL;
UINT16 numkartcupheaders = 0;

static boolean exitgame = false;
static boolean retrying = false;
static boolean retryingmodeattack = false;

UINT8 stagefailed; // Used for GEMS BONUS? Also to see if you beat the stage.

UINT16 emeralds;
INT32 luabanks[NUM_LUABANKS];
UINT32 token; // Number of tokens collected in a level
UINT32 tokenlist; // List of tokens collected
boolean gottoken; // Did you get a token? Used for end of act
INT32 tokenbits; // Used for setting token bits

// Old Special Stage
INT32 sstimer; // Time allotted in the special stage

tic_t totalplaytime;
UINT32 matchesplayed; // SRB2Kart
boolean gamedataloaded = false;

// Time attack data for levels
// These are dynamically allocated for space reasons now
recorddata_t *mainrecords[NUMMAPS]   = {NULL};
//nightsdata_t *nightsrecords[NUMMAPS] = {NULL};
UINT8 mapvisited[NUMMAPS];

// Temporary holding place for nights data for the current map
//nightsdata_t ntemprecords;

UINT32 bluescore, redscore; // CTF and Team Match team scores

// ring count... for PERFECT!
INT32 nummaprings = 0;

// Elminates unnecessary searching.
boolean CheckForBustableBlocks;
boolean CheckForBouncySector;
boolean CheckForQuicksand;
boolean CheckForMarioBlocks;
boolean CheckForFloatBob;
boolean CheckForReverseGravity;

// Powerup durations
UINT16 invulntics = 20*TICRATE;
UINT16 sneakertics = 20*TICRATE;
UINT16 flashingtics = 3*TICRATE/2; // SRB2kart
UINT16 tailsflytics = 8*TICRATE;
UINT16 underwatertics = 30*TICRATE;
UINT16 spacetimetics = 11*TICRATE + (TICRATE/2);
UINT16 extralifetics = 4*TICRATE;
UINT16 nightslinktics = 2*TICRATE;

INT32 gameovertics = 15*TICRATE;
UINT8 ammoremovaltics = 2*TICRATE;

// SRB2kart
tic_t introtime = 0;
tic_t starttime = 0;

const tic_t bulbtime = TICRATE/2;
UINT8 numbulbs = 0;

tic_t raceexittime = 5*TICRATE + (2*TICRATE/3);
tic_t battleexittime = 8*TICRATE;

INT32 hyudorotime = 7*TICRATE;
INT32 stealtime = TICRATE/2;
INT32 sneakertime = TICRATE + (TICRATE/3);
INT32 itemtime = 8*TICRATE;
INT32 bubbletime = TICRATE/2;
INT32 comebacktime = 10*TICRATE;
INT32 bumptime = 6;
INT32 greasetics = 3*TICRATE;
INT32 wipeoutslowtime = 20;
INT32 wantedreduce = 5*TICRATE;
INT32 wantedfrequency = 10*TICRATE;
INT32 flameseg = TICRATE/4;

UINT8 use1upSound = 0;
UINT8 maxXtraLife = 2; // Max extra lives from rings
UINT8 useContinues = 0; // Set to 1 to enable continues outside of no-save scenarioes

UINT8 introtoplay;
UINT8 creditscutscene;
UINT8 useBlackRock = 1;

// Emerald locations
mobj_t *hunt1;
mobj_t *hunt2;
mobj_t *hunt3;

tic_t racecountdown, exitcountdown; // for racing

fixed_t gravity;
fixed_t mapobjectscale;

INT16 autobalance; //for CTF team balance
INT16 teamscramble; //for CTF team scramble
INT16 scrambleplayers[MAXPLAYERS]; //for CTF team scramble
INT16 scrambleteams[MAXPLAYERS]; //for CTF team scramble
INT16 scrambletotal; //for CTF team scramble
INT16 scramblecount; //for CTF team scramble

INT32 cheats; //for multiplayer cheat commands

// SRB2Kart
// Cvars that we don't want changed mid-game
UINT8 gamespeed; // Game's current speed (or difficulty, or cc, or etc); 0 for easy, 1 for normal, 2 for hard
boolean encoremode = false; // Encore Mode currently enabled?
boolean prevencoremode;
boolean franticitems; // Frantic items currently enabled?
boolean comeback; // Battle Mode's karma comeback is on/off

// Voting system
INT16 votelevels[5][2]; // Levels that were rolled by the host
SINT8 votes[MAXPLAYERS]; // Each player's vote
SINT8 pickedvote; // What vote the host rolls

// Server-sided, synched variables
SINT8 battlewanted[4]; // WANTED players in battle, worth x2 points
tic_t wantedcalcdelay; // Time before it recalculates WANTED
tic_t indirectitemcooldown; // Cooldown before any more Shrink, SPB, or any other item that works indirectly is awarded
tic_t hyubgone; // Cooldown before hyudoro is allowed to be rerolled
tic_t mapreset; // Map reset delay when enough players have joined an empty game
boolean thwompsactive; // Thwomps activate on lap 2
SINT8 spbplace; // SPB exists, give the person behind better items
boolean rainbowstartavailable; // Boolean, keeps track of if the rainbow start was gotten

// Client-sided, unsynched variables (NEVER use in anything that needs to be synced with other players)
tic_t bombflashtimer = 0;	// Cooldown before another FlashPal can be intialized by a bomb exploding near a displayplayer. Avoids seizures.
boolean legitimateexit; // Did this client actually finish the match?
boolean comebackshowninfo; // Have you already seen the "ATTACK OR PROTECT" message?
tic_t curlap; // Current lap time
tic_t bestlap; // Best lap time
static INT16 randmapbuffer[NUMMAPS+1]; // Buffer for maps RandMap is allowed to roll

// Grading
UINT32 timesBeaten;

typedef struct joystickvector2_s
{
	INT32 xaxis;
	INT32 yaxis;
} joystickvector2_t;

boolean precache = true; // if true, load all graphics at start

INT16 prevmap, nextmap;

static UINT8 *savebuffer;

void SendWeaponPref(void);
void SendWeaponPref2(void);
void SendWeaponPref3(void);
void SendWeaponPref4(void);

static CV_PossibleValue_t joyaxis_cons_t[] = {{0, "None"},
{1, "X-Axis"}, {2, "Y-Axis"}, {-1, "X-Axis-"}, {-2, "Y-Axis-"},
#if JOYAXISSET > 1
{3, "Z-Axis"}, {4, "X-Rudder"}, {-3, "Z-Axis-"}, {-4, "X-Rudder-"},
#endif
#if JOYAXISSET > 2
{5, "Y-Rudder"}, {6, "Z-Rudder"}, {-5, "Y-Rudder-"}, {-6, "Z-Rudder-"},
#endif
#if JOYAXISSET > 3
{7, "U-Axis"}, {8, "V-Axis"}, {-7, "U-Axis-"}, {-8, "V-Axis-"},
#endif
 {0, NULL}};
#if JOYAXISSET > 4
"More Axis Sets"
#endif

// don't mind me putting these here, I was lazy to figure out where else I could put those without blowing up the compiler.

// chat timer thingy
static CV_PossibleValue_t chattime_cons_t[] = {{5, "MIN"}, {999, "MAX"}, {0, NULL}};
consvar_t cv_chattime = {"chattime", "8", CV_SAVE, chattime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// chatwidth
static CV_PossibleValue_t chatwidth_cons_t[] = {{64, "MIN"}, {150, "MAX"}, {0, NULL}};
consvar_t cv_chatwidth = {"chatwidth", "150", CV_SAVE, chatwidth_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// chatheight
static CV_PossibleValue_t chatheight_cons_t[] = {{6, "MIN"}, {22, "MAX"}, {0, NULL}};
consvar_t cv_chatheight = {"chatheight", "8", CV_SAVE, chatheight_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// chat notifications (do you want to hear beeps? I'd understand if you didn't.)
consvar_t cv_chatnotifications = {"chatnotifications", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// chat spam protection (why would you want to disable that???)
consvar_t cv_chatspamprotection = {"chatspamprotection", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// minichat text background
consvar_t cv_chatbacktint = {"chatbacktint", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// old shit console chat. (mostly exists for stuff like terminal, not because I cared if anyone liked the old chat.)
static CV_PossibleValue_t consolechat_cons_t[] = {{0, "Window"}, {1, "Console"}, {2, "Window (Hidden)"}, {0, NULL}};
consvar_t cv_consolechat = {"chatmode", "Window", CV_SAVE, consolechat_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// Pause game upon window losing focus
consvar_t cv_pauseifunfocused = {"pauseifunfocused", "Yes", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

// Display song credits
consvar_t cv_songcredits = {"songcredits", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_invertmouse = {"invertmouse", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_invincmusicfade = {"invincmusicfade", "300", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_growmusicfade = {"growmusicfade", "500", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_resetspecialmusic = {"resetspecialmusic", "Yes", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_resume = {"resume", "Yes", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_turnaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_turn", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis2_turn", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis3_turn", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis4_turn", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL}
};

consvar_t cv_moveaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_move", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis_move2", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis_move3", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis_move4", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

consvar_t cv_brakeaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_brake", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis2_brake", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis3_brake", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis4_brake", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

consvar_t cv_aimaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_aim", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis2_aim", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis3_aim", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis4_aim", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

consvar_t cv_lookaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_look", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis2_look", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis3_look", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis4_look", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

consvar_t cv_fireaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_fire", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis_fire2", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis_fire3", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis_fire4", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

consvar_t cv_driftaxis[MAXSPLITSCREENPLAYERS] = {
	{"joyaxis_drift", "Z-Rudder", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis2_drift", "Z-Rudder", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis3_drift", "Z-Rudder", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joyaxis4_drift", "Z-Rudder", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

static CV_PossibleValue_t zerotoone_cons_t[] = {{0, "MIN"}, {FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_deadzone[MAXSPLITSCREENPLAYERS] = {
	{"joy_deadzone", "0.125", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joy2_deadzone", "0.125", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joy3_deadzone", "0.125", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joy4_deadzone", "0.125", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

consvar_t cv_digitaldeadzone[MAXSPLITSCREENPLAYERS] = {
	{"joy_digdeadzone", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joy2_digdeadzone", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joy3_digdeadzone", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"joy4_digdeadzone", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
};

#ifdef SEENAMES
player_t *seenplayer; // player we're aiming at right now
#endif

// now automatically allocated in D_RegisterClientCommands
// so that it doesn't have to be updated depending on the value of MAXPLAYERS
char player_names[MAXPLAYERS][MAXPLAYERNAME+1];

// Allocation for time and nights data
void G_AllocMainRecordData(INT16 i)
{
	if (!mainrecords[i])
		mainrecords[i] = Z_Malloc(sizeof(recorddata_t), PU_STATIC, NULL);
	memset(mainrecords[i], 0, sizeof(recorddata_t));
}

// MAKE SURE YOU SAVE DATA BEFORE CALLING THIS
void G_ClearRecords(void)
{
	INT16 i;
	for (i = 0; i < NUMMAPS; ++i)
	{
		if (mainrecords[i])
		{
			Z_Free(mainrecords[i]);
			mainrecords[i] = NULL;
		}
		/*if (nightsrecords[i])
		{
			Z_Free(nightsrecords[i]);
			nightsrecords[i] = NULL;
		}*/
	}
}

// For easy retrieval of records
tic_t G_GetBestTime(INT16 map)
{
	if (!mainrecords[map-1] || mainrecords[map-1]->time <= 0)
		return (tic_t)UINT32_MAX;

	return mainrecords[map-1]->time;
}

// Not needed
/*
tic_t G_GetBestLap(INT16 map)
{
	if (!mainrecords[map-1] || mainrecords[map-1]->lap <= 0)
		return (tic_t)UINT32_MAX;

	return mainrecords[map-1]->lap;
}
*/

//
// G_UpdateRecordReplays
//
// Update replay files/data, etc. for Record Attack
//
static void G_UpdateRecordReplays(void)
{
	const size_t glen = strlen(srb2home)+1+strlen("media")+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char *gpath;
	char lastdemo[256], bestdemo[256];
	UINT8 earnedEmblems;

	// Record new best time
	if (!mainrecords[gamemap-1])
		G_AllocMainRecordData(gamemap-1);

	if (players[consoleplayer].pflags & PF_GAMETYPEOVER)
	{
		players[consoleplayer].realtime = UINT32_MAX;
	}

	if (((mainrecords[gamemap-1]->time == 0) || (players[consoleplayer].realtime < mainrecords[gamemap-1]->time))
		&& (players[consoleplayer].realtime < UINT32_MAX)) // DNF
	{
		mainrecords[gamemap-1]->time = players[consoleplayer].realtime;
	}

	if (modeattacking == ATTACKING_TIME)
	{
		if ((mainrecords[gamemap-1]->lap == 0) || (bestlap < mainrecords[gamemap-1]->lap))
			mainrecords[gamemap-1]->lap = bestlap;
	}
	else
	{
		mainrecords[gamemap-1]->lap = 0;
	}

	// Save demo!
	bestdemo[255] = '\0';
	lastdemo[255] = '\0';
	G_SetDemoTime(players[consoleplayer].realtime, bestlap);
	G_CheckDemoStatus();

	gpath = va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s",
			srb2home, timeattackfolder);
	M_MkdirEach(gpath, M_PathParts(gpath) - 3, 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));
	snprintf(lastdemo, 255, "%s-%s-last.lmp", gpath, cv_chooseskin.string);

	if (FIL_FileExists(lastdemo))
	{
		UINT8 *buf;
		size_t len = FIL_ReadFile(lastdemo, &buf);

		snprintf(bestdemo, 255, "%s-%s-time-best.lmp", gpath, cv_chooseskin.string);
		if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & 1)
		{ // Better time, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD TIME!"), M_GetText("Saved replay as"), bestdemo);
		}

		if (modeattacking == ATTACKING_TIME)
		{
			snprintf(bestdemo, 255, "%s-%s-lap-best.lmp", gpath, cv_chooseskin.string);
			if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & (1<<1))
			{ // Better lap time, save this demo.
				if (FIL_FileExists(bestdemo))
					remove(bestdemo);
				FIL_WriteFile(bestdemo, buf, len);
				CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD LAP!"), M_GetText("Saved replay as"), bestdemo);
			}
		}

		//CONS_Printf("%s '%s'\n", M_GetText("Saved replay as"), lastdemo);

		Z_Free(buf);
	}
	free(gpath);

	// Check emblems when level data is updated
	if ((earnedEmblems = M_CheckLevelEmblems()))
		CONS_Printf(M_GetText("\x82" "Earned %hu medal%s for Record Attack records.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");

	if (M_UpdateUnlockablesAndExtraEmblems())
		S_StartSound(NULL, sfx_ncitem);

	// SRB2Kart - save here so you NEVER lose your earned times/medals.
	G_SaveGameData();
}

// for consistency among messages: this modifies the game and removes savemoddata.
void G_SetGameModified(boolean silent, boolean major)
{
	if ((majormods && modifiedgame) || !mainwads || (refreshdirmenu & REFRESHDIR_GAMEDATA)) // new gamedata amnesty?
		return;

	modifiedgame = true;

	if (!major)
		return;

	//savemoddata = false; -- there is literally no reason to do this anymore.
	majormods = true;

	if (!silent)
		CONS_Alert(CONS_NOTICE, M_GetText("Game must be restarted to play Record Attack.\n"));

	// If in record attack recording, cancel it.
	if (modeattacking)
		M_EndModeAttackRun();
	else if (marathonmode)
		Command_ExitGame_f();
}

/** Builds an original game map name from a map number.
  * The complexity is due to MAPA0-MAPZZ.
  *
  * \param map Map number.
  * \return Pointer to a static buffer containing the desired map name.
  * \sa M_MapNumber
  */
const char *G_BuildMapName(INT32 map)
{
	static char mapname[10] = "MAPXX"; // internal map name (wad resource name)

	I_Assert(map >= 0);
	I_Assert(map <= NUMMAPS);

	if (map == 0) // hack???
	{
		if (gamestate == GS_TITLESCREEN)
			map = -1;
		else if (gamestate == GS_LEVEL)
			map = gamemap-1;
		else
			map = prevmap;
		map = G_RandMap(G_TOLFlag(cv_newgametype.value), map, false, 0, false, NULL)+1;
	}

	if (map < 100)
		sprintf(&mapname[3], "%.2d", map);
	else
	{
		mapname[3] = (char)('A' + (char)((map - 100) / 36));
		if ((map - 100) % 36 < 10)
			mapname[4] = (char)('0' + (char)((map - 100) % 36));
		else
			mapname[4] = (char)('A' + (char)((map - 100) % 36) - 10);
		mapname[5] = '\0';
	}

	return mapname;
}

/** Clips the console player's mouse aiming to the current view.
  * Used whenever the player view is changed manually.
  *
  * \param aiming Pointer to the vertical angle to clip.
  * \return Short version of the clipped angle for building a ticcmd.
  */
INT16 G_ClipAimingPitch(INT32 *aiming)
{
	INT32 limitangle;

	limitangle = ANGLE_90 - 1;

	if (*aiming > limitangle)
		*aiming = limitangle;
	else if (*aiming < -limitangle)
		*aiming = -limitangle;

	return (INT16)((*aiming)>>16);
}

INT16 G_SoftwareClipAimingPitch(INT32 *aiming)
{
	INT32 limitangle;

	// note: the current software mode implementation doesn't have true perspective
	limitangle = ANGLE_90 - ANG10; // Some viewing fun, but not too far down...

	if (*aiming > limitangle)
		*aiming = limitangle;
	else if (*aiming < -limitangle)
		*aiming = -limitangle;

	return (INT16)((*aiming)>>16);
}

INT32 PlayerJoyAxis(UINT8 player, axis_input_e axissel)
{
	INT32 retaxis;
	INT32 axisval;
	boolean flp = false;

	//find what axis to get
	switch (axissel)
	{
		case AXISTURN:
			axisval = cv_turnaxis[player-1].value;
			break;
		case AXISMOVE:
			axisval = cv_moveaxis[player-1].value;
			break;
		case AXISBRAKE:
			axisval = cv_brakeaxis[player-1].value;
			break;
		case AXISAIM:
			axisval = cv_aimaxis[player-1].value;
			break;
		case AXISLOOK:
			axisval = cv_lookaxis[player-1].value;
			break;
		case AXISFIRE:
			axisval = cv_fireaxis[player-1].value;
			break;
		case AXISDRIFT:
			axisval = cv_driftaxis[player-1].value;
			break;
		default:
			return 0;
	}

	if (axisval < 0) //odd -axises
	{
		axisval = -axisval;
		flp = true;
	}
	if (axisval > JOYAXISSET*2 || axisval == 0) //not there in array or None
		return 0;

	if (axisval%2)
	{
		axisval /= 2;
		retaxis = joyxmove[player-1][axisval];
	}
	else
	{
		axisval--;
		axisval /= 2;
		retaxis = joyymove[player-1][axisval];
	}

	if (retaxis < (-JOYAXISRANGE))
		retaxis = -JOYAXISRANGE;
	if (retaxis > (+JOYAXISRANGE))
		retaxis = +JOYAXISRANGE;

	if (!Joystick[player-1].bGamepadStyle && axissel >= AXISDIGITAL)
	{
		const INT32 jdeadzone = ((JOYAXISRANGE-1) * cv_digitaldeadzone[player-1].value) >> FRACBITS;
		if (-jdeadzone < retaxis && retaxis < jdeadzone)
			return 0;
	}

	if (flp) retaxis = -retaxis; //flip it around
	return retaxis;
}

// Take a magnitude of two axes, and adjust it to take out the deadzone
// Will return a value between 0 and JOYAXISRANGE
static INT32 G_BasicDeadZoneCalculation(INT32 magnitude, fixed_t deadZone)
{
	const INT32 jdeadzone = (JOYAXISRANGE * deadZone) / FRACUNIT;
	INT32 deadzoneAppliedValue = 0;
	INT32 adjustedMagnitude = abs(magnitude);

	if (jdeadzone >= JOYAXISRANGE && adjustedMagnitude >= JOYAXISRANGE) // If the deadzone and magnitude are both 100%...
		return JOYAXISRANGE; // ...return 100% input directly, to avoid dividing by 0
	else if (adjustedMagnitude > jdeadzone) // Otherwise, calculate how much the magnitude exceeds the deadzone
	{
		adjustedMagnitude = min(adjustedMagnitude, JOYAXISRANGE);

		adjustedMagnitude -= jdeadzone;

		deadzoneAppliedValue = (adjustedMagnitude * JOYAXISRANGE) / (JOYAXISRANGE - jdeadzone);
	}

	return deadzoneAppliedValue;
}

// Get the actual sensible radial value for a joystick axis when accounting for a deadzone
static void G_HandleAxisDeadZone(UINT8 splitnum, joystickvector2_t *joystickvector)
{
	INT32 gamepadStyle = Joystick[splitnum].bGamepadStyle;
	fixed_t deadZone = cv_deadzone[splitnum].value;

	// When gamepadstyle is "true" the values are just -1, 0, or 1. This is done in the interface code.
	if (!gamepadStyle)
	{
		// Get the total magnitude of the 2 axes
		INT32 magnitude = (joystickvector->xaxis * joystickvector->xaxis) + (joystickvector->yaxis * joystickvector->yaxis);
		INT32 normalisedXAxis;
		INT32 normalisedYAxis;
		INT32 normalisedMagnitude;
		double dMagnitude = sqrt((double)magnitude);
		magnitude = (INT32)dMagnitude;

		// Get the normalised xy values from the magnitude
		normalisedXAxis = (joystickvector->xaxis * magnitude) / JOYAXISRANGE;
		normalisedYAxis = (joystickvector->yaxis * magnitude) / JOYAXISRANGE;

		// Apply the deadzone to the magnitude to give a correct value between 0 and JOYAXISRANGE
		normalisedMagnitude = G_BasicDeadZoneCalculation(magnitude, deadZone);

		// Apply the deadzone to the xy axes
		joystickvector->xaxis = (normalisedXAxis * normalisedMagnitude) / JOYAXISRANGE;
		joystickvector->yaxis = (normalisedYAxis * normalisedMagnitude) / JOYAXISRANGE;

		// Cap the values so they don't go above the correct maximum
		joystickvector->xaxis = min(joystickvector->xaxis, JOYAXISRANGE);
		joystickvector->xaxis = max(joystickvector->xaxis, -JOYAXISRANGE);
		joystickvector->yaxis = min(joystickvector->yaxis, JOYAXISRANGE);
		joystickvector->yaxis = max(joystickvector->yaxis, -JOYAXISRANGE);
	}
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
// set secondaryplayer true to build player 2's ticcmd in splitscreen mode
//
INT32 localaiming[MAXSPLITSCREENPLAYERS];
angle_t localangle[MAXSPLITSCREENPLAYERS];

static INT32 angleturn[2] = {KART_FULLTURN, KART_FULLTURN / 4}; // + slow turn

void G_BuildTiccmd(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer)
{
	const UINT8 forplayer = ssplayer-1;

	const INT32 lookaxis = cv_lookaxis[forplayer].value;
	const boolean invertmouse = cv_invertmouse.value;
	const boolean analogjoystickmove = cv_usejoystick[forplayer].value && !Joystick[forplayer].bGamepadStyle;
	const boolean gamepadjoystickmove = cv_usejoystick[forplayer].value && Joystick[forplayer].bGamepadStyle;
	const boolean usejoystick = (analogjoystickmove || gamepadjoystickmove);

	static INT32 turnheld[MAXSPLITSCREENPLAYERS]; // for accelerative turning
	static boolean keyboard_look[MAXSPLITSCREENPLAYERS]; // true if lookup/down using keyboard
	static boolean resetdown[MAXSPLITSCREENPLAYERS]; // don't cam reset every frame

	INT32 tspeed, forward, axis;

	joystickvector2_t joystickvector;

	boolean turnleft, turnright;

	player_t *player = &players[g_localplayers[forplayer]];
	camera_t *thiscam = &camera[forplayer];
	INT32 *laim = &localaiming[forplayer];
	INT32 *th = &turnheld[forplayer];
	boolean *kbl = &keyboard_look[forplayer];
	boolean *rd = &resetdown[forplayer];
	const boolean mouseaiming = player->spectator;

	if (demo.playback) return;

	// Is there any reason this can't just be I_BaseTiccmd?
	switch (ssplayer)
	{
		case 2:
			G_CopyTiccmd(cmd, I_BaseTiccmd2(), 1);
			break;
		case 3:
			G_CopyTiccmd(cmd, I_BaseTiccmd3(), 1);
			break;
		case 4:
			G_CopyTiccmd(cmd, I_BaseTiccmd4(), 1);
			break;
		case 1:
		default:
			G_CopyTiccmd(cmd, I_BaseTiccmd(), 1); // empty, or external driver
			break;
	}

	// why build a ticcmd if we're paused?
	// Or, for that matter, if we're being reborn.
	// Kart, don't build a ticcmd if someone is resynching or the server is stopped too so we don't fly off course in bad conditions
	if (paused || P_AutoPause() || (gamestate == GS_LEVEL && player->playerstate == PST_REBORN) || hu_resynching)
	{
		cmd->aiming = G_ClipAimingPitch(laim);
		return;
	}

	if (K_PlayerUsesBotMovement(player))
	{
		// Bot ticcmd is generated by K_BuildBotTiccmd
		return;
	}

	turnright = PlayerInputDown(ssplayer, gc_turnright);
	turnleft = PlayerInputDown(ssplayer, gc_turnleft);

	joystickvector.xaxis = PlayerJoyAxis(ssplayer, AXISTURN);
	joystickvector.yaxis = PlayerJoyAxis(ssplayer, AXISAIM);
	G_HandleAxisDeadZone(forplayer, &joystickvector);

	if (encoremode)
	{
		turnright ^= turnleft; // swap these using three XORs
		turnleft ^= turnright;
		turnright ^= turnleft;
		joystickvector.xaxis = -joystickvector.xaxis;
	}

	if (gamepadjoystickmove && joystickvector.xaxis != 0)
	{
		turnright = turnright || (joystickvector.xaxis > 0);
		turnleft = turnleft || (joystickvector.xaxis < 0);
	}
	forward = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		*th += realtics;
	else
		*th = 0;

	if (*th < SLOWTURNTICS)
		tspeed = 1; // slow turn
	else
		tspeed = 0;

	cmd->turning = 0;

	// let movement keys cancel each other out
	if (turnright && !(turnleft))
	{
		cmd->turning = (INT16)(cmd->turning - (angleturn[tspeed]));
	}
	else if (turnleft && !(turnright))
	{
		cmd->turning = (INT16)(cmd->turning + (angleturn[tspeed]));
	}

	if (analogjoystickmove && joystickvector.xaxis != 0)
	{
		cmd->turning = (INT16)(cmd->turning - (((joystickvector.xaxis * angleturn[0]) >> 10)));
	}

	// Specator mouse turning
	if (player->spectator)
	{
		cmd->turning = (INT16)(cmd->turning - ((mousex*(encoremode ? -1 : 1)*8)));
	}

	if (player->spectator || objectplacing) // SRB2Kart: spectators need special controls
	{
		axis = PlayerJoyAxis(ssplayer, AXISMOVE);
		if (PlayerInputDown(ssplayer, gc_accelerate) || (usejoystick && axis > 0))
			cmd->buttons |= BT_ACCELERATE;
		axis = PlayerJoyAxis(ssplayer, AXISBRAKE);
		if (PlayerInputDown(ssplayer, gc_brake) || (usejoystick && axis > 0))
			cmd->buttons |= BT_BRAKE;
		axis = PlayerJoyAxis(ssplayer, AXISAIM);
		if (PlayerInputDown(ssplayer, gc_aimforward) || (usejoystick && axis < 0))
			forward += MAXPLMOVE;
		if (PlayerInputDown(ssplayer, gc_aimbackward) || (usejoystick && axis > 0))
			forward -= MAXPLMOVE;
	}
	else
	{
		// forward with key or button // SRB2kart - we use an accel/brake instead of forward/backward.
		axis = PlayerJoyAxis(ssplayer, AXISMOVE);
		if (PlayerInputDown(ssplayer, gc_accelerate) || (gamepadjoystickmove && axis > 0) || player->kartstuff[k_sneakertimer])
		{
			cmd->buttons |= BT_ACCELERATE;
			forward = MAXPLMOVE; // 50
		}
		else if (analogjoystickmove && axis > 0)
		{
			cmd->buttons |= BT_ACCELERATE;
			// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			forward += ((axis * MAXPLMOVE) >> 10);
		}

		axis = PlayerJoyAxis(ssplayer, AXISBRAKE);
		if (PlayerInputDown(ssplayer, gc_brake) || (gamepadjoystickmove && axis > 0))
		{
			cmd->buttons |= BT_BRAKE;
			if (cmd->buttons & BT_ACCELERATE || cmd->forwardmove <= 0)
				forward -= MAXPLMOVE;
		}
		else if (analogjoystickmove && axis > 0)
		{
			cmd->buttons |= BT_BRAKE;
			// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			if (cmd->buttons & BT_ACCELERATE || cmd->forwardmove <= 0)
				forward -= ((axis * MAXPLMOVE) >> 10);
		}

		// But forward/backward IS used for aiming.
		if (PlayerInputDown(ssplayer, gc_aimforward) || (joystickvector.yaxis < 0))
			cmd->buttons |= BT_FORWARD;
		if (PlayerInputDown(ssplayer, gc_aimbackward) || (joystickvector.yaxis > 0))
			cmd->buttons |= BT_BACKWARD;
	}

	// fire with any button/key
	axis = PlayerJoyAxis(ssplayer, AXISFIRE);
	if (PlayerInputDown(ssplayer, gc_fire) || (usejoystick && axis > 0))
		cmd->buttons |= BT_ATTACK;

	// drift with any button/key
	axis = PlayerJoyAxis(ssplayer, AXISDRIFT);
	if (PlayerInputDown(ssplayer, gc_drift) || (usejoystick && axis > 0))
		cmd->buttons |= BT_DRIFT;

	// rear view with any button/key
	axis = PlayerJoyAxis(ssplayer, AXISLOOKBACK);
	if (PlayerInputDown(ssplayer, gc_lookback) || (usejoystick && axis > 0))
		cmd->buttons |= BT_LOOKBACK;

	// Lua scriptable buttons
	if (PlayerInputDown(ssplayer, gc_custom1))
		cmd->buttons |= BT_CUSTOM1;
	if (PlayerInputDown(ssplayer, gc_custom2))
		cmd->buttons |= BT_CUSTOM2;
	if (PlayerInputDown(ssplayer, gc_custom3))
		cmd->buttons |= BT_CUSTOM3;

	// Reset camera
	if (PlayerInputDown(ssplayer, gc_camreset))
	{
		if (thiscam->chase && *rd == false)
			P_ResetCamera(player, thiscam);
		*rd = true;
	}
	else
		*rd = false;

	// spectator aiming shit, ahhhh...
	{
		INT32 player_invert = invertmouse ? -1 : 1;
		INT32 screen_invert =
			(player->mo && (player->mo->eflags & MFE_VERTICALFLIP)
			 && (!thiscam->chase)) //because chasecam's not inverted
			 ? -1 : 1; // set to -1 or 1 to multiply

		// mouse look stuff (mouse look is not the same as mouse aim)
		if (mouseaiming && player->spectator)
		{
			*kbl = false;

			// looking up/down
			*laim += (mlooky<<19)*player_invert*screen_invert;
		}

		axis = PlayerJoyAxis(ssplayer, AXISLOOK);
		if (analogjoystickmove && axis != 0 && lookaxis && player->spectator)
			*laim += (axis<<16) * screen_invert;

		// spring back if not using keyboard neither mouselookin'
		if (*kbl == false && !lookaxis && !mouseaiming)
			*laim = 0;

		if (player->spectator)
		{
			if (PlayerInputDown(ssplayer, gc_lookup) || (gamepadjoystickmove && axis < 0))
			{
				*laim += KB_LOOKSPEED * screen_invert;
				*kbl = true;
			}
			else if (PlayerInputDown(ssplayer, gc_lookdown) || (gamepadjoystickmove && axis > 0))
			{
				*laim -= KB_LOOKSPEED * screen_invert;
				*kbl = true;
			}
		}

		if (PlayerInputDown(ssplayer, gc_centerview)) // No need to put a spectator limit on this one though :V
			*laim = 0;

		// accept no mlook for network games
		if (!cv_allowmlook.value)
			*laim = 0;

		cmd->aiming = G_ClipAimingPitch(laim);
	}

	mousex = mousey = mlooky = 0;

	cmd->forwardmove += (SINT8)forward;

	cmd->latency = modeattacking ? 0 : (leveltime & 0xFF); // Send leveltime when this tic was generated to the server for control lag calculations
	cmd->flags = 0;

	/* 	Lua: Allow this hook to overwrite ticcmd.
		We check if we're actually in a level because for some reason this Hook would run in menus and on the titlescreen otherwise.
		Be aware that within this hook, nothing but this player's cmd can be edited (otherwise we'd run in some pretty bad synching problems since this is clientsided, or something)

		Possible usages for this are:
			-Forcing the player to perform an action, which could otherwise require terrible, terrible hacking to replicate.
			-Preventing the player to perform an action, which would ALSO require some weirdo hacks.
			-Making some galaxy brain autopilot Lua if you're a masochist
			-Making a Mario Kart 8 Deluxe tier baby mode that steers you away from walls and whatnot. You know what, do what you want!
	*/
	if (gamestate == GS_LEVEL)
		LUAh_PlayerCmd(player, cmd);

	if (cmd->forwardmove > MAXPLMOVE)
		cmd->forwardmove = MAXPLMOVE;
	else if (cmd->forwardmove < -MAXPLMOVE)
		cmd->forwardmove = -MAXPLMOVE;

	if (cmd->turning > (angleturn[0]))
		cmd->turning = (angleturn[0]);
	else if (cmd->turning < (-angleturn[0]))
		cmd->turning = (-angleturn[0]);

	// Reset away view if a command is given.
	if ((cmd->forwardmove || cmd->buttons)
		&& !r_splitscreen && displayplayers[0] != consoleplayer && ssplayer == 1)
	{
		// Call ViewpointSwitch hooks here.
		// The viewpoint was forcibly changed.
		LUAh_ViewpointSwitch(player, &players[consoleplayer], true);
		displayplayers[0] = consoleplayer;
	}
}

ticcmd_t *G_CopyTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n)
{
	return M_Memcpy(dest, src, n*sizeof(*src));
}

ticcmd_t *G_MoveTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
	{
		dest[i].forwardmove = src[i].forwardmove;
		dest[i].turning = (INT16)SHORT(src[i].turning);
		dest[i].aiming = (INT16)SHORT(src[i].aiming);
		dest[i].buttons = (UINT16)SHORT(src[i].buttons);
		dest[i].latency = src[i].latency;
		dest[i].flags = src[i].flags;
	}
	return dest;
}

//
// G_DoLoadLevel
//
void G_DoLoadLevel(boolean resetplayer)
{
	INT32 i, j;

	// Make sure objectplace is OFF when you first start the level!
	OP_ResetObjectplace();

	levelstarttic = gametic; // for time calculation

	if (wipegamestate == GS_LEVEL)
		wipegamestate = -1; // force a wipe

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	if (gamestate == GS_VOTING)
		Y_EndVote();

	// cleanup
	if (titlemapinaction == TITLEMAP_LOADING)
	{
		if (W_CheckNumForName(G_BuildMapName(gamemap)) == LUMPERROR)
		{
			titlemap = 0; // let's not infinite recursion ok
			Command_ExitGame_f();
			return;
		}

		titlemapinaction = TITLEMAP_RUNNING;
	}
	else
		titlemapinaction = TITLEMAP_OFF;

	G_SetGamestate(GS_LEVEL);
	I_UpdateMouseGrab();

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (resetplayer || (playeringame[i] && players[i].playerstate == PST_DEAD))
			players[i].playerstate = PST_REBORN;
	}

	// Setup the level.
	if (!P_LoadLevel(false)) // this never returns false?
	{
		// fail so reset game stuff
		Command_ExitGame_f();
		return;
	}

	gameaction = ga_nothing;
#ifdef PARANOIA
	Z_CheckHeap(-2);
#endif

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (camera[i].chase)
			P_ResetCamera(&players[g_localplayers[i]], &camera[i]);
	}

	// clear cmd building stuff
	memset(gamekeydown, 0, sizeof (gamekeydown));
	for (i = 0;i < JOYAXISSET; i++)
	{
		for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
			joyxmove[j][i] = joyymove[j][i] = 0;
	}
	mousex = mousey = 0;

	// clear hud messages remains (usually from game startup)
	CON_ClearHUD();

	server_lagless = cv_lagless.value;
}

//
// Start the title card.
//
void G_StartTitleCard(void)
{
	// The title card has been disabled for this map.
	// Oh well.
	if (!G_IsTitleCardAvailable())
	{
		WipeStageTitle = false;
		return;
	}

	// clear the hud
	CON_ClearHUD();

	// prepare status bar
	ST_startTitleCard();

	// start the title card
	WipeStageTitle = false; //(!titlemapinaction); -- temporary until titlecards are reworked
}

//
// Run the title card before fading in to the level.
//
void G_PreLevelTitleCard(void)
{
#ifndef NOWIPE
	tic_t strtime = I_GetTime();
	tic_t endtime = strtime + (PRELEVELTIME*NEWTICRATERATIO);
	tic_t nowtime = strtime;
	tic_t lasttime = strtime;
	while (nowtime < endtime)
	{
		// draw loop
		while (!((nowtime = I_GetTime()) - lasttime))
			I_Sleep();
		lasttime = nowtime;

		ST_runTitleCard();
		ST_preLevelTitleCardDrawer();
		I_FinishUpdate(); // page flip or blit buffer

		if (moviemode)
			M_SaveFrame();
		if (takescreenshot) // Only take screenshots after drawing.
			M_DoScreenShot();
	}
#endif
}

//
// Returns true if the current level has a title card.
//
boolean G_IsTitleCardAvailable(void)
{
	// The current level has no name.
	if (!mapheaderinfo[gamemap-1]->lvlttl[0])
		return false;

	// The title card is available.
	return true;
}

INT32 pausedelay = 0;
boolean pausebreakkey = false;
static INT32 camtoggledelay[MAXSPLITSCREENPLAYERS];

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
boolean G_Responder(event_t *ev)
{
	UINT8 i;

	// any other key pops up menu if in demos
	if (gameaction == ga_nothing && !demo.quitafterplaying &&
		((demo.playback && !modeattacking && !demo.title && !multiplayer) || gamestate == GS_TITLESCREEN))
	{
		if (ev->type == ev_keydown && ev->data1 != 301 && !(gamestate == GS_TITLESCREEN && finalecount < TICRATE))
		{
			M_StartControlPanel();
			return true;
		}
		return false;
	}
	else if (demo.playback && demo.title)
	{
		// Title demo uses intro responder
		if (F_IntroResponder(ev))
		{
			// stop the title demo
			G_CheckDemoStatus();
			return true;
		}
		return false;
	}

	if (gamestate == GS_LEVEL)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event
		if (AM_Responder(ev))
			return true; // automap ate it
		// map the event (key/mouse/joy) to a gamecontrol
	}
	// Intro
	else if (gamestate == GS_INTRO)
	{
		if (F_IntroResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}
	else if (gamestate == GS_CUTSCENE)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event

		if (F_CutsceneResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}
	else if (gamestate == GS_CREDITS || gamestate == GS_ENDING) // todo: keep ending here?
	{
		if (HU_Responder(ev))
			return true; // chat ate the event

		if (F_CreditResponder(ev))
		{
			// Skip credits for everyone
			if (! serverrunning)/* hahahahahaha */
				F_StartGameEvaluation();
			else if (server || IsPlayerAdmin(consoleplayer))
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			return true;
		}
	}
	else if (gamestate == GS_CONTINUING)
	{
		return true;
	}
	// Demo End
	else if (gamestate == GS_GAMEEND)
	{
		return true;
	}
	else if (gamestate == GS_INTERMISSION || gamestate == GS_VOTING || gamestate == GS_EVALUATION)
		if (HU_Responder(ev))
			return true; // chat ate the event

	// allow spy mode changes even during the demo
	if (gamestate == GS_LEVEL && ev->type == ev_keydown
		&& (ev->data1 == KEY_F12 || ev->data1 == gamecontrol[0][gc_viewpoint][0] || ev->data1 == gamecontrol[0][gc_viewpoint][1]))
	{
		if (!demo.playback && (r_splitscreen || !netgame))
			g_localplayers[0] = consoleplayer;
		else
		{
			G_AdjustView(1, 1, true);

			// change statusbar also if playing back demo
			if (demo.quitafterplaying)
				ST_changeDemoView();

			return true;
		}
	}

	if (gamestate == GS_LEVEL && ev->type == ev_keydown && multiplayer && demo.playback && !demo.freecam)
	{
		if (ev->data1 == gamecontrol[1][gc_viewpoint][0] || ev->data1 == gamecontrol[1][gc_viewpoint][1])
		{
			G_AdjustView(2, 1, true);
			return true;
		}
		else if (ev->data1 == gamecontrol[2][gc_viewpoint][0] || ev->data1 == gamecontrol[2][gc_viewpoint][1])
		{
			G_AdjustView(3, 1, true);
			return true;
		}
		else if (ev->data1 == gamecontrol[3][gc_viewpoint][0] || ev->data1 == gamecontrol[3][gc_viewpoint][1])
		{
			G_AdjustView(4, 1, true);
			return true;
		}

		// Allow pausing
		if (
			ev->data1 == gamecontrol[0][gc_pause][0]
			|| ev->data1 == gamecontrol[0][gc_pause][1]
			|| ev->data1 == KEY_PAUSE
		)
		{
			paused = !paused;

			if (demo.rewinding)
			{
				G_ConfirmRewind(leveltime);
				paused = true;
				S_PauseAudio();
			}
			else if (paused)
				S_PauseAudio();
			else
				S_ResumeAudio();

			return true;
		}

		// open menu but only w/ esc
		if (ev->data1 == 32)
		{
			M_StartControlPanel();

			return true;
		}
	}

	// update keys current state
	G_MapEventsToControls(ev);

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 == gamecontrol[0][gc_pause][0]
				|| ev->data1 == gamecontrol[0][gc_pause][1]
				|| ev->data1 == KEY_PAUSE)
			{
				if (modeattacking && !demo.playback && (gamestate == GS_LEVEL))
				{
					pausebreakkey = (ev->data1 == KEY_PAUSE);
					if (menuactive || pausedelay < 0 || leveltime < 2)
						return true;

					if (pausedelay < 1+(NEWTICRATE/2))
						pausedelay = 1+(NEWTICRATE/2);
					else if (++pausedelay > 1+(NEWTICRATE/2)+(NEWTICRATE/3))
					{
						G_SetModeAttackRetryFlag();
						return true;
					}
					pausedelay++; // counteract subsequent subtraction this frame
				}
				else
				{
					INT32 oldpausedelay = pausedelay;
					pausedelay = (NEWTICRATE/7);
					if (!oldpausedelay)
					{
						// command will handle all the checks for us
						COM_ImmedExecute("pause");
						return true;
					}
				}
			}

			for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
			{
				if (ev->data1 == gamecontrol[i][gc_camtoggle][0]
					|| ev->data1 == gamecontrol[i][gc_camtoggle][1])
				{
					if (!camtoggledelay[i])
					{
						camtoggledelay[i] = NEWTICRATE / 7;
						CV_SetValue(&cv_chasecam[i], cv_chasecam[i].value ? 0 : 1);
					}
				}
			}

			return true;

		case ev_keyup:
			return false; // always let key up events filter down

		case ev_mouse:
			return true; // eat events

		case ev_joystick:
			return true; // eat events

		case ev_joystick2:
			return true; // eat events

		case ev_joystick3:
			return true; // eat events

		case ev_joystick4:
			return true; // eat events

		default:
			break;
	}

	return false;
}

//
// G_CouldView
// Return whether a player could be viewed by any means.
//
boolean G_CouldView(INT32 playernum)
{
	player_t *player;

	if (playernum < 0 || playernum > MAXPLAYERS-1)
		return false;

	if (!playeringame[playernum])
		return false;

	player = &players[playernum];

	if (player->spectator)
		return false;

	// SRB2Kart: Only go through players who are actually playing
	if (player->exiting)
		return false;
	if (( player->pflags & PF_GAMETYPEOVER ))
		return false;

	// I don't know if we want this actually, but I'll humor the suggestion anyway
	if ((gametyperules & GTR_BUMPERS) && !demo.playback)
	{
		if (player->kartstuff[k_bumper] <= 0)
			return false;
	}

	// SRB2Kart: we have no team-based modes, YET...
	if (G_GametypeHasTeams())
	{
		if (players[consoleplayer].ctfteam && player->ctfteam != players[consoleplayer].ctfteam)
			return false;
	}

	return true;
}

//
// G_CanView
// Return whether a player can be viewed on a particular view (splitscreen).
//
boolean G_CanView(INT32 playernum, UINT8 viewnum, boolean onlyactive)
{
	UINT8 splits;
	UINT8 viewd;
	INT32 *displayplayerp;

	if (!(onlyactive ? G_CouldView(playernum) : (playeringame[playernum] && !players[playernum].spectator)))
		return false;

	splits = r_splitscreen+1;
	if (viewnum > splits)
		viewnum = splits;

	for (viewd = 1; viewd < viewnum; ++viewd)
	{
		displayplayerp = (&displayplayers[viewd-1]);
		if ((*displayplayerp) == playernum)
			return false;
	}
	for (viewd = viewnum + 1; viewd <= splits; ++viewd)
	{
		displayplayerp = (&displayplayers[viewd-1]);
		if ((*displayplayerp) == playernum)
			return false;
	}

	return true;
}

//
// G_FindView
// Return the next player that can be viewed on a view, wraps forward.
// An out of range startview is corrected.
//
INT32 G_FindView(INT32 startview, UINT8 viewnum, boolean onlyactive, boolean reverse)
{
	INT32 i, dir = reverse ? -1 : 1;
	startview = min(max(startview, 0), MAXPLAYERS);
	for (i = startview; i < MAXPLAYERS && i >= 0; i += dir)
	{
		if (G_CanView(i, viewnum, onlyactive))
			return i;
	}
	for (i = (reverse ? MAXPLAYERS-1 : 0); i != startview; i += dir)
	{
		if (G_CanView(i, viewnum, onlyactive))
			return i;
	}
	return -1;
}

INT32 G_CountPlayersPotentiallyViewable(boolean active)
{
	INT32 total = 0;
	INT32 i;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (active ? G_CouldView(i) : (playeringame[i] && !players[i].spectator))
			total++;
	}
	return total;
}

//
// G_ResetView
// Correct a viewpoint to playernum or the next available, wraps forward.
// Also promotes splitscreen up to available viewable players.
// An out of range playernum is corrected.
//
void G_ResetView(UINT8 viewnum, INT32 playernum, boolean onlyactive)
{
	UINT8 splits;
	UINT8 viewd;

	INT32    *displayplayerp;
	camera_t *camerap;

	INT32 olddisplayplayer;
	INT32 playersviewable;

	splits = r_splitscreen+1;

	/* Promote splits */
	if (viewnum > splits)
	{
		playersviewable = G_CountPlayersPotentiallyViewable(onlyactive);
		if (playersviewable < splits)/* do not demote */
			return;

		if (viewnum > playersviewable)
			viewnum = playersviewable;
		r_splitscreen = viewnum-1;

		/* Prepare extra views for G_FindView to pass. */
		for (viewd = splits+1; viewd < viewnum; ++viewd)
		{
			displayplayerp = (&displayplayers[viewd-1]);
			(*displayplayerp) = INT32_MAX;
		}

		R_ExecuteSetViewSize();
	}

	displayplayerp = (&displayplayers[viewnum-1]);
	olddisplayplayer = (*displayplayerp);

	/* Check if anyone is available to view. */
	if (( playernum = G_FindView(playernum, viewnum, onlyactive, playernum < olddisplayplayer) ) == -1)
		return;

	/* Focus our target view first so that we don't take its player. */
	(*displayplayerp) = playernum;
	if ((*displayplayerp) != olddisplayplayer)
	{
		camerap = &camera[viewnum-1];
		P_ResetCamera(&players[(*displayplayerp)], camerap);
	}

	if (viewnum > splits)
	{
		for (viewd = splits+1; viewd < viewnum; ++viewd)
		{
			displayplayerp = (&displayplayers[viewd-1]);
			camerap = &camera[viewd];

			(*displayplayerp) = G_FindView(0, viewd, onlyactive, false);

			P_ResetCamera(&players[(*displayplayerp)], camerap);
		}
	}

	if (viewnum == 1 && demo.playback)
		consoleplayer = displayplayers[0];
}

//
// G_AdjustView
// Increment a viewpoint by offset from the current player. A negative value
// decrements.
//
void G_AdjustView(UINT8 viewnum, INT32 offset, boolean onlyactive)
{
	INT32 *displayplayerp, oldview;
	displayplayerp = &displayplayers[viewnum-1];
	oldview = (*displayplayerp);
	G_ResetView(viewnum, ( (*displayplayerp) + offset ), onlyactive);

	// If no other view could be found, go back to what we had.
	if ((*displayplayerp) == -1)
		(*displayplayerp) = oldview;
}

//
// G_ResetViews
// Ensures all viewpoints are valid
// Also demotes splitscreen down to one player.
//
void G_ResetViews(void)
{
	UINT8 splits;
	UINT8 viewd;

	INT32 playersviewable;

	splits = r_splitscreen+1;

	playersviewable = G_CountPlayersPotentiallyViewable(false);
	/* Demote splits */
	if (playersviewable < splits)
	{
		splits = playersviewable;
		r_splitscreen = max(splits-1, 0);
		R_ExecuteSetViewSize();
	}

	/*
	Consider installing a method to focus the last
	view elsewhere if all players spectate?
	*/
	for (viewd = 1; viewd <= splits; ++viewd)
	{
		G_AdjustView(viewd, 0, false);
	}
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(boolean run)
{
	UINT32 i;
	INT32 buf;
	ticcmd_t *cmd;

	// see also SCR_DisplayMarathonInfo
	if ((marathonmode & (MA_INIT|MA_INGAME)) == MA_INGAME && gamestate == GS_LEVEL)
		marathontime++;

	P_MapStart();
	// do player reborns if needed
	if (gamestate == GS_LEVEL)
	{
		// Or, alternatively, retry.
		if (G_GetRetryFlag())
		{
			G_ClearRetryFlag();

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
				{
					K_PlayerLoseLife(&players[i]);
				}
			}

			D_MapChange(gamemap, gametype, (cv_kartencore.value == 1), false, 1, false, false);
		}

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].playerstate == PST_REBORN)
				G_DoReborn(i);
	}
	P_MapEnd();

	// do things to change the game state
	while (gameaction != ga_nothing)
		switch (gameaction)
		{
			case ga_completed: G_DoCompleted(); break;
			case ga_startcont: G_DoStartContinue(); break;
			case ga_continued: G_DoContinued(); break;
			case ga_worlddone: G_DoWorldDone(); break;
			case ga_startvote: G_DoStartVote(); break;
			case ga_nothing: break;
			default: I_Error("gameaction = %d\n", gameaction);
		}

	buf = gametic % TICQUEUE;

	if (!demo.playback)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			cmd = &players[i].cmd;

			if (playeringame[i])
			{
				G_CopyTiccmd(cmd, &netcmds[buf][i], 1);

				// Use the leveltime sent in the player's ticcmd to determine control lag
				if (K_PlayerUsesBotMovement(&players[i]))
				{
					// Never has lag
					cmd->latency = 0;
				}
				else
				{
					//@TODO add a cvar to allow setting this max
					cmd->latency = min(((leveltime & 0xFF) - cmd->latency) & 0xFF, MAXPREDICTTICS-1);
				}
			}
		}
	}

	// do main actions
	switch (gamestate)
	{
		case GS_LEVEL:
			if (demo.title)
				F_TitleDemoTicker();
			P_Ticker(run); // tic the game
			ST_Ticker(run);
			F_TextPromptTicker();
			AM_Ticker();
			HU_Ticker();
			break;

		case GS_INTERMISSION:
			if (run)
				Y_Ticker();
			HU_Ticker();
			break;

		case GS_VOTING:
			if (run)
				Y_VoteTicker();
			HU_Ticker();
			break;

		case GS_TIMEATTACK:
			F_MenuPresTicker(run);
			break;

		case GS_INTRO:
			if (run)
				F_IntroTicker();
			break;

		case GS_ENDING:
			if (run)
				F_EndingTicker();
			HU_Ticker();
			break;

		case GS_CUTSCENE:
			if (run)
				F_CutsceneTicker();
			HU_Ticker();
			break;

		case GS_GAMEEND:
			if (run)
				F_GameEndTicker();
			break;

		case GS_EVALUATION:
			if (run)
				F_GameEvaluationTicker();
			HU_Ticker();
			break;

		case GS_CONTINUING:
			break;

		case GS_CREDITS:
			if (run)
				F_CreditTicker();
			HU_Ticker();
			break;

		case GS_TITLESCREEN:
			if (titlemapinaction) P_Ticker(run);
			F_TitleScreenTicker(run);
			break;

		case GS_WAITINGPLAYERS:
			if (netgame)
				F_WaitingPlayersTicker();
			HU_Ticker();
			break;

		case GS_DEDICATEDSERVER:
		case GS_NULL:
			break; // do nothing
	}

	if (run)
	{
		if (G_GametypeHasSpectators()
			&& (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING // definitely good
			|| gamestate == GS_WAITINGPLAYERS)) // definitely a problem if we don't do it at all in this gamestate, but might need more protection?
		{
			K_CheckSpectateStatus();
		}

		if (pausedelay && pausedelay != INT32_MIN)
		{
			if (pausedelay > 0)
				pausedelay--;
			else
				pausedelay++;
		}

		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			if (camtoggledelay[i])
				camtoggledelay[i]--;
		}
	}
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_PlayerFinishLevel
// Called when a player completes a level.
//
static inline void G_PlayerFinishLevel(INT32 player)
{
	player_t *p;

	p = &players[player];

	memset(p->powers, 0, sizeof (p->powers));
	memset(p->kartstuff, 0, sizeof (p->kartstuff)); // SRB2kart

	p->mo->drawflags &= ~(MFD_TRANSMASK|MFD_BRIGHTMASK); // cancel invisibility
	P_FlashPal(p, 0, 0); // Resets

	p->starpostnum = 0;
	memset(&p->respawn, 0, sizeof (p->respawn));

	// SRB2kart: Increment the "matches played" counter.
	if (player == consoleplayer)
	{
		if (legitimateexit && !demo.playback && !mapreset) // (yes you're allowed to unlock stuff this way when the game is modified)
		{
			matchesplayed++;
			if (M_UpdateUnlockablesAndExtraEmblems())
				S_StartSound(NULL, sfx_ncitem);
			G_SaveGameData();
		}

		legitimateexit = false;
	}
}

//
// G_PlayerReborn
// Called after a player dies. Almost everything is cleared and initialized.
//
void G_PlayerReborn(INT32 player, boolean betweenmaps)
{
	player_t *p;
	INT32 score, marescore;
	INT32 lives;
	boolean lostlife;
	INT32 continues;

	UINT8 kartspeed;
	UINT8 kartweight;

	boolean followerready;
	INT32 followerskin;
	UINT16 followercolor;
	mobj_t *follower; // old follower, will probably be removed by the time we're dead but you never know.

	INT32 charflags;
	UINT32 followitem;

	INT32 pflags;

	INT32 ctfteam;

	INT32 starpostnum;
	INT32 exiting;
	tic_t dashmode;
	INT16 numboxes;
	INT16 totalring;
	UINT8 laps;
	UINT8 mare;
	UINT16 skincolor;
	INT32 skin;
	UINT32 availabilities;

	tic_t jointime;
	tic_t quittime;

	UINT8 splitscreenindex;
	boolean spectator;
	boolean bot;
	UINT8 botdifficulty;

	INT16 rings;
	angle_t playerangleturn;

	UINT8 botdiffincrease;
	boolean botrival;

	SINT8 xtralife;

	// SRB2kart
	respawnvars_t respawn;
	INT32 itemtype;
	INT32 itemamount;
	INT32 itemroulette;
	INT32 roulettetype;
	INT32 growshrinktimer;
	INT32 bumper;
	INT32 comebackpoints;
	INT32 wanted;
	boolean songcredit = false;

	score = players[player].score;
	marescore = players[player].marescore;
	lives = players[player].lives;
	lostlife = players[player].lostlife;
	continues = players[player].continues;
	ctfteam = players[player].ctfteam;
	exiting = players[player].exiting;

	jointime = players[player].jointime;
	quittime = players[player].quittime;

	splitscreenindex = players[player].splitscreenindex;
	spectator = players[player].spectator;

	pflags = (players[player].pflags & (PF_WANTSTOJOIN|PF_GAMETYPEOVER));

	playerangleturn = players[player].angleturn;

	// As long as we're not in multiplayer, carry over cheatcodes from map to map
	if (!(netgame || multiplayer))
		pflags |= (players[player].pflags & (PF_GODMODE|PF_NOCLIP|PF_INVIS));

	dashmode = players[player].dashmode;

	numboxes = players[player].numboxes;
	laps = players[player].laps;
	totalring = players[player].totalring;

	skincolor = players[player].skincolor;
	skin = players[player].skin;

	// SRB2kart
	kartspeed = players[player].kartspeed;
	kartweight = players[player].kartweight;

	follower = players[player].follower;
	followerready = players[player].followerready;
	followercolor = players[player].followercolor;
	followerskin = players[player].followerskin;

	availabilities = players[player].availabilities;

	charflags = players[player].charflags;

	starpostnum = players[player].starpostnum;
	followitem = players[player].followitem;

	mare = players[player].mare;
	bot = players[player].bot;
	botdifficulty = players[player].botvars.difficulty;

	botdiffincrease = players[player].botvars.diffincrease;
	botrival = players[player].botvars.rival;

	xtralife = players[player].xtralife;

	// SRB2kart
	if (betweenmaps || leveltime < starttime)
	{
		itemroulette = 0;
		roulettetype = 0;
		itemtype = 0;
		itemamount = 0;
		growshrinktimer = 0;
		bumper = ((gametyperules & GTR_BUMPERS) ? K_StartingBumperCount() : 0);
		rings = ((gametyperules & GTR_RINGS) ? 5 : 0);
		comebackpoints = 0;
		wanted = 0;
	}
	else
	{
		itemroulette = (players[player].kartstuff[k_itemroulette] > 0 ? 1 : 0);
		roulettetype = players[player].kartstuff[k_roulettetype];

		if (players[player].kartstuff[k_itemheld])
		{
			itemtype = 0;
			itemamount = 0;
		}
		else
		{
			itemtype = players[player].kartstuff[k_itemtype];
			itemamount = players[player].kartstuff[k_itemamount];
		}

		// Keep Shrink status, remove Grow status
		if (players[player].kartstuff[k_growshrinktimer] < 0)
			growshrinktimer = players[player].kartstuff[k_growshrinktimer];
		else
			growshrinktimer = 0;

		bumper = players[player].kartstuff[k_bumper];
		rings = players[player].rings;
		comebackpoints = players[player].kartstuff[k_comebackpoints];
		wanted = players[player].kartstuff[k_wanted];
	}

	// Obliterate follower from existence
	P_SetTarget(&players[player].follower, NULL);

	memcpy(&respawn, &players[player].respawn, sizeof (respawn));

	p = &players[player];
	memset(p, 0, sizeof (*p));

	p->score = score;
	p->marescore = marescore;
	p->lives = lives;
	p->lostlife = lostlife;
	p->continues = continues;
	p->pflags = pflags;
	p->ctfteam = ctfteam;
	p->jointime = jointime;
	p->quittime = quittime;
	p->splitscreenindex = splitscreenindex;
	p->spectator = spectator;
	p->angleturn = playerangleturn;

	// save player config truth reborn
	p->skincolor = skincolor;
	p->skin = skin;
	p->kartspeed = kartspeed;
	p->kartweight = kartweight;
	//
	p->charflags = charflags;
	p->availabilities = availabilities;
	p->followitem = followitem;

	p->starpostnum = starpostnum;
	p->exiting = exiting;

	p->dashmode = dashmode;

	p->numboxes = numboxes;
	p->laps = laps;
	p->totalring = totalring;

	p->mare = mare;
	p->bot = bot;
	p->botvars.difficulty = botdifficulty;
	p->rings = rings;
	p->botvars.diffincrease = botdiffincrease;
	p->botvars.rival = botrival;
	p->xtralife = xtralife;

	// SRB2kart
	p->kartstuff[k_itemroulette] = itemroulette;
	p->kartstuff[k_roulettetype] = roulettetype;
	p->kartstuff[k_itemtype] = itemtype;
	p->kartstuff[k_itemamount] = itemamount;
	p->kartstuff[k_growshrinktimer] = growshrinktimer;
	p->kartstuff[k_bumper] = bumper;
	p->kartstuff[k_comebackpoints] = comebackpoints;
	p->kartstuff[k_comebacktimer] = comebacktime;
	p->kartstuff[k_wanted] = wanted;
	p->kartstuff[k_eggmanblame] = -1;
	p->kartstuff[k_lastdraft] = -1;

	memcpy(&p->respawn, &respawn, sizeof (p->respawn));

	if (follower)
		P_RemoveMobj(follower);

	p->followerready = followerready;
	p->followerskin = followerskin;
	p->followercolor = followercolor;
	//p->follower = NULL;	// respawn a new one with you, it looks better.
	// ^ Not necessary anyway since it will be respawned regardless considering it doesn't exist anymore.


	// Don't do anything immediately
	p->pflags |= PF_USEDOWN;
	p->pflags |= PF_ATTACKDOWN;
	p->pflags |= PF_JUMPDOWN;

	p->playerstate = PST_LIVE;
	p->panim = PA_IDLE; // standing animation

	// Check to make sure their color didn't change somehow...
	if (G_GametypeHasTeams())
	{
		UINT8 i;

		if (p->ctfteam == 1 && p->skincolor != skincolor_redteam)
		{
			for (i = 0; i <= splitscreen; i++)
			{
				if (p == &players[g_localplayers[i]])
				{
					CV_SetValue(&cv_playercolor[i], skincolor_redteam);
					break;
				}
			}
		}
		else if (p->ctfteam == 2 && p->skincolor != skincolor_blueteam)
		{
			for (i = 0; i <= splitscreen; i++)
			{
				if (p == &players[g_localplayers[i]])
				{
					CV_SetValue(&cv_playercolor[i], skincolor_blueteam);
					break;
				}
			}
		}
	}

	if (betweenmaps)
		return;

	if (p-players == consoleplayer)
	{
		if (mapmusflags & MUSIC_RELOADRESET)
		{
			strncpy(mapmusname, mapheaderinfo[gamemap-1]->musname, 7);
			mapmusname[6] = 0;
			mapmusflags = (mapheaderinfo[gamemap-1]->mustrack & MUSIC_TRACKMASK);
			mapmusposition = mapheaderinfo[gamemap-1]->muspos;
			mapmusresume = 0;
			songcredit = true;
		}

		// This is in S_Start, but this was not here previously.
		// if (RESETMUSIC)
		// 	S_StopMusic();
		S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);
	}

	P_RestoreMusic(p);

	if (songcredit)
		S_ShowMusicCredit();

	if (leveltime > (starttime + (TICRATE/2)) && !p->spectator)
	{
		K_DoIngameRespawn(p);
	}
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
static boolean G_CheckSpot(INT32 playernum, mapthing_t *mthing)
{
	fixed_t x;
	fixed_t y;
	INT32 i;

	// maybe there is no player start
	if (!mthing)
		return false;

	if (!players[playernum].mo)
	{
		// first spawn of level
		for (i = 0; i < playernum; i++)
			if (playeringame[i] && players[i].mo
				&& players[i].mo->x == mthing->x << FRACBITS
				&& players[i].mo->y == mthing->y << FRACBITS)
			{
				return false;
			}
		return true;
	}

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	if (!K_CheckPlayersRespawnColliding(playernum, x, y))
		return false;

	if (!P_CheckPosition(players[playernum].mo, x, y))
		return false;

	return true;
}

//
// G_SpawnPlayer
// Spawn a player in a spot appropriate for the gametype --
// or a not-so-appropriate spot, if it initially fails
// due to a lack of starts open or something.
//
void G_SpawnPlayer(INT32 playernum)
{
	if (!playeringame[playernum])
		return;

	P_SpawnPlayer(playernum);
	G_MovePlayerToSpawnOrStarpost(playernum);
	LUAh_PlayerSpawn(&players[playernum]); // Lua hook for player spawning :)
}

void G_MovePlayerToSpawnOrStarpost(INT32 playernum)
{
	if (players[playernum].starpostnum)
		P_MovePlayerToStarpost(playernum);
	else
		P_MovePlayerToSpawn(playernum, G_FindMapStart(playernum));
}

mapthing_t *G_FindTeamStart(INT32 playernum)
{
	const boolean doprints = P_IsLocalPlayer(&players[playernum]);
	INT32 i,j;

	if (!numredctfstarts && !numbluectfstarts) //why even bother, eh?
	{
		if ((gametyperules & GTR_TEAMSTARTS) && doprints)
			CONS_Alert(CONS_WARNING, M_GetText("No CTF starts in this map!\n"));
		return NULL;
	}

	if ((!players[playernum].ctfteam && numredctfstarts && (!numbluectfstarts || P_RandomChance(FRACUNIT/2))) || players[playernum].ctfteam == 1) //red
	{
		if (!numredctfstarts)
		{
			if (doprints)
				CONS_Alert(CONS_WARNING, M_GetText("No Red Team starts in this map!\n"));
			return NULL;
		}

		for (j = 0; j < 32; j++)
		{
			i = P_RandomKey(numredctfstarts);
			if (G_CheckSpot(playernum, redctfstarts[i]))
				return redctfstarts[i];
		}

		if (doprints)
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Red Team starts!\n"));
		return NULL;
	}
	else if (!players[playernum].ctfteam || players[playernum].ctfteam == 2) //blue
	{
		if (!numbluectfstarts)
		{
			if (doprints)
				CONS_Alert(CONS_WARNING, M_GetText("No Blue Team starts in this map!\n"));
			return NULL;
		}

		for (j = 0; j < 32; j++)
		{
			i = P_RandomKey(numbluectfstarts);
			if (G_CheckSpot(playernum, bluectfstarts[i]))
				return bluectfstarts[i];
		}
		if (doprints)
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Blue Team starts!\n"));
		return NULL;
	}
	//should never be reached but it gets stuff to shut up
	return NULL;
}

mapthing_t *G_FindBattleStart(INT32 playernum)
{
	const boolean doprints = P_IsLocalPlayer(&players[playernum]);
	INT32 i, j;

	if (numdmstarts)
	{
		for (j = 0; j < 64; j++)
		{
			i = P_RandomKey(numdmstarts);
			if (G_CheckSpot(playernum, deathmatchstarts[i]))
				return deathmatchstarts[i];
		}
		if (doprints)
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Deathmatch starts!\n"));
		return NULL;
	}

	if ((gametyperules & GTR_BATTLESTARTS) && doprints)
		CONS_Alert(CONS_WARNING, M_GetText("No Deathmatch starts in this map!\n"));
	return NULL;
}

mapthing_t *G_FindRaceStart(INT32 playernum)
{
	const boolean doprints = P_IsLocalPlayer(&players[playernum]);

	if (numcoopstarts)
	{
		UINT8 i;
		UINT8 pos = 0;

		// SRB2Kart: figure out player spawn pos from points
		if (!playeringame[playernum] || players[playernum].spectator)
			return playerstarts[0]; // go to first spot if you're a spectator

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;
			if (i == playernum)
				continue;

			if (players[i].score < players[playernum].score)
			{
				UINT8 j;
				UINT8 num = 0;

				for (j = 0; j < MAXPLAYERS; j++) // I hate similar loops inside loops... :<
				{
					if (!playeringame[j] || players[j].spectator)
						continue;
					if (j == playernum)
						continue;
					if (j == i)
						continue;

					if (netgame && cv_kartusepwrlv.value)
					{
						if (clientpowerlevels[j][PWRLV_RACE] == clientpowerlevels[i][PWRLV_RACE])
							num++;
					}
					else
					{
						if (players[j].score == players[i].score)
							num++;
					}
				}

				if (num > 1) // found dupes
					pos++;
			}
			else
			{
				if (i < playernum)
					pos++;
				else
				{
					if (netgame && cv_kartusepwrlv.value)
					{
						if (clientpowerlevels[i][PWRLV_RACE] > clientpowerlevels[playernum][PWRLV_RACE])
							pos++;
					}
					else
					{
						if (players[i].score > players[playernum].score)
							pos++;
					}
				}
			}
		}

		if (G_CheckSpot(playernum, playerstarts[pos % numcoopstarts]))
			return playerstarts[pos % numcoopstarts];

		// Your spot isn't available? Find whatever you can get first.
		for (i = 0; i < numcoopstarts; i++)
		{
			if (G_CheckSpot(playernum, playerstarts[i]))
				return playerstarts[i];
		}

		// SRB2Kart: We have solid players, so this behavior is less ideal.
		// Don't bother checking to see if the player 1 start is open.
		// Just spawn there.
		//return playerstarts[0];

		if (doprints)
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Race starts!\n"));
		return NULL;
	}

	if (doprints)
		CONS_Alert(CONS_WARNING, M_GetText("No Race starts in this map!\n"));
	return NULL;
}

// Find a Co-op start, or fallback into other types of starts.
static inline mapthing_t *G_FindRaceStartOrFallback(INT32 playernum)
{
	mapthing_t *spawnpoint = NULL;
	if (!(spawnpoint = G_FindRaceStart(playernum)) // find a Race start
	&& !(spawnpoint = G_FindBattleStart(playernum))) // find a DM start
		spawnpoint = G_FindTeamStart(playernum); // fallback
	return spawnpoint;
}

// Find a Match start, or fallback into other types of starts.
static inline mapthing_t *G_FindBattleStartOrFallback(INT32 playernum)
{
	mapthing_t *spawnpoint = NULL;
	if (!(spawnpoint = G_FindBattleStart(playernum)) // find a DM start
	&& !(spawnpoint = G_FindTeamStart(playernum))) // find a CTF start
		spawnpoint = G_FindRaceStart(playernum); // fallback
	return spawnpoint;
}

static inline mapthing_t *G_FindTeamStartOrFallback(INT32 playernum)
{
	mapthing_t *spawnpoint = NULL;
	if (!(spawnpoint = G_FindTeamStart(playernum)) // find a CTF start
	&& !(spawnpoint = G_FindBattleStart(playernum))) // find a DM start
		spawnpoint = G_FindRaceStart(playernum); // fallback
	return spawnpoint;
}

mapthing_t *G_FindMapStart(INT32 playernum)
{
	mapthing_t *spawnpoint;

	if (!playeringame[playernum])
		return NULL;

	// -- Spectators --
	// Order in platform gametypes: Race->DM->CTF
	// And, with deathmatch starts: DM->CTF->Race
	if (players[playernum].spectator)
	{
		// In platform gametypes, spawn in Co-op starts first
		// Overriden by GTR_BATTLESTARTS.
		if (gametyperules & GTR_BATTLESTARTS)
			spawnpoint = G_FindBattleStartOrFallback(playernum);
		else
			spawnpoint = G_FindRaceStartOrFallback(playernum);
	}

	// -- CTF --
	// Order: CTF->DM->Race
	else if ((gametyperules & GTR_TEAMSTARTS) && players[playernum].ctfteam)
		spawnpoint = G_FindTeamStartOrFallback(playernum);

	// -- DM/Tag/CTF-spectator/etc --
	// Order: DM->CTF->Race
	else if (gametyperules & GTR_BATTLESTARTS)
		spawnpoint = G_FindBattleStartOrFallback(playernum);

	// -- Other game modes --
	// Order: Race->DM->CTF
	else
		spawnpoint = G_FindRaceStartOrFallback(playernum);

	//No spawns found. ANYWHERE.
	if (!spawnpoint)
	{
		if (nummapthings)
		{
			if (P_IsLocalPlayer(&players[playernum]))
				CONS_Alert(CONS_ERROR, M_GetText("No player spawns found, spawning at the first mapthing!\n"));
			spawnpoint = &mapthings[0];
		}
		else
		{
			if (P_IsLocalPlayer(&players[playernum]))
				CONS_Alert(CONS_ERROR, M_GetText("No player spawns found, spawning at the origin!\n"));
		}
	}

	return spawnpoint;
}

// Go back through all the projectiles and remove all references to the old
// player mobj, replacing them with the new one.
void G_ChangePlayerReferences(mobj_t *oldmo, mobj_t *newmo)
{
	thinker_t *th;
	mobj_t *mo2;

	I_Assert((oldmo != NULL) && (newmo != NULL));

	// scan all thinkers
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo2 = (mobj_t *)th;

		if (!(mo2->flags & MF_MISSILE))
			continue;

		if (mo2->target == oldmo)
		{
			P_SetTarget(&mo2->target, newmo);
			mo2->flags2 |= MF2_BEYONDTHEGRAVE; // this mobj belongs to a player who has reborn
		}
	}
}

//
// G_DoReborn
//
void G_DoReborn(INT32 playernum)
{
	player_t *player = &players[playernum];

	// Make sure objectplace is OFF when you first start the level!
	OP_ResetObjectplace();

	{
		// respawn at the start
		mobj_t *oldmo = NULL;

		// first dissasociate the corpse
		if (player->mo)
		{
			oldmo = player->mo;
			// Don't leave your carcass stuck 10-billion feet in the ground!
			P_RemoveMobj(player->mo);
		}

		G_SpawnPlayer(playernum);
		if (oldmo)
			G_ChangePlayerReferences(oldmo, players[playernum].mo);
	}
}

void G_AddPlayer(INT32 playernum)
{
	player_t *p = &players[playernum];
	p->playerstate = PST_REBORN;
	demo_extradata[playernum] |= DXD_PLAYSTATE|DXD_COLOR|DXD_NAME|DXD_SKIN|DXD_FOLLOWER; // Set everything
}

void G_ExitLevel(void)
{
	if (gamestate == GS_LEVEL)
	{
		if (grandprixinfo.gp == true && grandprixinfo.wonround != true)
		{
			UINT8 i;

			// You didn't win...

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator && !players[i].bot)
				{
					if (players[i].lives > 0)
					{
						break;
					}
				}
			}

			if (i == MAXPLAYERS)
			{
				// GAME OVER, try again from the start!

				if (netgame)
				{
					; // restart cup here if we do online GP
				}
				else
				{
					D_QuitNetGame();
					CL_Reset();
					D_StartTitle();
				}
			}
			else
			{
				// Go redo this course.
				G_SetRetryFlag();
			}

			return;
		}

		gameaction = ga_completed;
		lastdraw = true;

		// If you want your teams scrambled on map change, start the process now.
		// The teams will scramble at the start of the next round.
		if (cv_scrambleonchange.value && G_GametypeHasTeams())
		{
			if (server)
				CV_SetValue(&cv_teamscramble, cv_scrambleonchange.value);
		}

		CON_LogMessage(M_GetText("The round has ended.\n"));

		// Remove CEcho text on round end.
		HU_ClearCEcho();

		// Don't save demos immediately here! Let standings write first
	}
	else if (gamestate == GS_ENDING)
	{
		F_StartCredits();
	}
	else if (gamestate == GS_CREDITS)
	{
		F_StartGameEvaluation();
	}
}

// See also the enum GameType in doomstat.h
const char *Gametype_Names[NUMGAMETYPES] =
{
	"Race", // GT_RACE
	"Battle" // GT_BATTLE
};

// For dehacked
const char *Gametype_ConstantNames[NUMGAMETYPES] =
{
	"GT_RACE", // GT_RACE
	"GT_BATTLE" // GT_BATTLE
};

// Gametype rules
UINT32 gametypedefaultrules[NUMGAMETYPES] =
{
	// Race
	GTR_CIRCUIT|GTR_RINGS|GTR_BOTS,
	// Battle
	GTR_BUMPERS|GTR_KARMA|GTR_ITEMARROWS|GTR_BATTLESTARTS|GTR_POINTLIMIT|GTR_TIMELIMIT|GTR_OVERTIME
};

//
// G_SetGametype
//
// Set a new gametype, also setting gametype rules accordingly. Yay!
//
void G_SetGametype(INT16 gtype)
{
	gametype = gtype;
	gametyperules = gametypedefaultrules[gametype];
}

//
// G_AddGametype
//
// Add a gametype. Returns the new gametype number.
//
INT16 G_AddGametype(UINT32 rules)
{
	INT16 newgtype = gametypecount;
	gametypecount++;

	// Set gametype rules.
	gametypedefaultrules[newgtype] = rules;
	Gametype_Names[newgtype] = "???";

	// Update gametype_cons_t accordingly.
	G_UpdateGametypeSelections();

	return newgtype;
}

//
// G_AddGametypeConstant
//
// Self-explanatory. Filters out "bad" characters.
//
void G_AddGametypeConstant(INT16 gtype, const char *newgtconst)
{
	size_t r = 0; // read
	size_t w = 0; // write
	char *gtconst = Z_Calloc(strlen(newgtconst) + 4, PU_STATIC, NULL);
	char *tmpconst = Z_Calloc(strlen(newgtconst) + 1, PU_STATIC, NULL);

	// Copy the gametype name.
	strcpy(tmpconst, newgtconst);

	// Make uppercase.
	strupr(tmpconst);

	// Prepare to write the new constant string now.
	strcpy(gtconst, "GT_");

	// Remove characters that will not be allowed in the constant string.
	for (; r < strlen(tmpconst); r++)
	{
		boolean writechar = true;
		char rc = tmpconst[r];
		switch (rc)
		{
			// Space, at sign and question mark
			case ' ':
			case '@':
			case '?':
			// Used for operations
			case '+':
			case '-':
			case '*':
			case '/':
			case '%':
			case '^':
			case '&':
			case '!':
			// Part of Lua's syntax
			case '#':
			case '=':
			case '~':
			case '<':
			case '>':
			case '(':
			case ')':
			case '{':
			case '}':
			case '[':
			case ']':
			case ':':
			case ';':
			case ',':
			case '.':
				writechar = false;
				break;
		}
		if (writechar)
		{
			gtconst[3 + w] = rc;
			w++;
		}
	}

	// Free the temporary string.
	Z_Free(tmpconst);

	// Finally, set the constant string.
	Gametype_ConstantNames[gtype] = gtconst;
}

//
// G_UpdateGametypeSelections
//
// Updates gametype_cons_t.
//
void G_UpdateGametypeSelections(void)
{
	INT32 i;
	for (i = 0; i < gametypecount; i++)
	{
		gametype_cons_t[i].value = i;
		gametype_cons_t[i].strvalue = Gametype_Names[i];
	}
	gametype_cons_t[NUMGAMETYPES].value = 0;
	gametype_cons_t[NUMGAMETYPES].strvalue = NULL;
}

// Gametype rankings
INT16 gametyperankings[NUMGAMETYPES] =
{
	GT_RACE,
	GT_BATTLE,
};

// Gametype to TOL (Type Of Level)
UINT32 gametypetol[NUMGAMETYPES] =
{
	TOL_RACE, // Race
	TOL_BATTLE, // Battle
	TOL_TV, // Midnight Channel effect
};

tolinfo_t TYPEOFLEVEL[NUMTOLNAMES] = {
	{"RACE",TOL_RACE},
	{"BATTLE",TOL_BATTLE},
	{"TV",TOL_TV},
	{NULL, 0}
};

UINT32 lastcustomtol = (TOL_TV<<1);

//
// G_AddTOL
//
// Adds a type of level.
//
void G_AddTOL(UINT32 newtol, const char *tolname)
{
	INT32 i;
	for (i = 0; TYPEOFLEVEL[i].name; i++)
		;

	TYPEOFLEVEL[i].name = Z_StrDup(tolname);
	TYPEOFLEVEL[i].flag = newtol;
}

//
// G_AddGametypeTOL
//
// Assigns a type of level to a gametype.
//
void G_AddGametypeTOL(INT16 gtype, UINT32 newtol)
{
	gametypetol[gtype] = newtol;
}

//
// G_GetGametypeByName
//
// Returns the number for the given gametype name string, or -1 if not valid.
//
INT32 G_GetGametypeByName(const char *gametypestr)
{
	INT32 i;

	for (i = 0; i < gametypecount; i++)
		if (!stricmp(gametypestr, Gametype_Names[i]))
			return i;

	return -1; // unknown gametype
}

//
// G_IsSpecialStage
//
// Returns TRUE if
// the given map is a special stage.
//
boolean G_IsSpecialStage(INT32 mapnum)
{
#if 1
	(void)mapnum;
#else
	if (modeattacking == ATTACKING_TIME)
		return false;
	if (mapnum >= sstage_start && mapnum <= sstage_end)
		return true;
	if (mapnum >= smpstage_start && mapnum <= smpstage_end)
		return true;
#endif

	return false;
}

//
// G_GametypeUsesLives
//
// Returns true if the current gametype uses
// the lives system.  False otherwise.
//
boolean G_GametypeUsesLives(void)
{
	if ((grandprixinfo.gp == true) // In Grand Prix
		&& (gametype == GT_RACE) // NOT in bonus round
		&& !G_IsSpecialStage(gamemap) // NOT in special stage
		&& !(modeattacking || metalrecording)) // NOT in Record Attack
	{
		return true;
	}

	return false;
}

//
// G_GametypeHasTeams
//
// Returns true if the current gametype uses
// Red/Blue teams.  False otherwise.
//
boolean G_GametypeHasTeams(void)
{
	if (gametyperules & GTR_TEAMS)
	{
		// Teams forced on by this gametype
		return true;
	}
	else if (gametyperules & GTR_NOTEAMS)
	{
		// Teams forced off by this gametype
		return false;
	}

	// Teams are determined by the "teamplay" modifier!
	return false; // teamplay
}

//
// G_GametypeHasSpectators
//
// Returns true if the current gametype supports
// spectators.  False otherwise.
//
boolean G_GametypeHasSpectators(void)
{
	return (netgame || (multiplayer && demo.playback));
}

//
// G_SometimesGetDifferentGametype
//
// Oh, yeah, and we sometimes flip encore mode on here too.
//
INT16 G_SometimesGetDifferentGametype(void)
{
	boolean encorepossible = ((M_SecretUnlocked(SECRET_ENCORE) || encorescramble == 1) && (gametyperules & GTR_CIRCUIT));

	if (!cv_kartvoterulechanges.value // never
		&& encorescramble != 1) // destroying the code for this one instance
		return gametype;

	if (randmapbuffer[NUMMAPS] > 0 && (encorepossible || cv_kartvoterulechanges.value != 3))
	{
		randmapbuffer[NUMMAPS]--;
		if (encorepossible)
		{
			if (encorescramble != -1)
				encorepossible = (boolean)encorescramble; // FORCE to what was scrambled on intermission
			else
			{
				switch (cv_kartvoterulechanges.value)
				{
					case 3: // always
						randmapbuffer[NUMMAPS] = 0; // gotta prep this in case it isn't already set
						break;
					case 2: // frequent
						encorepossible = M_RandomChance(FRACUNIT>>1);
						break;
					case 1: // sometimes
					default:
						encorepossible = M_RandomChance(FRACUNIT>>2);
						break;
				}
			}
			if (encorepossible != (cv_kartencore.value == 1))
				return (gametype|0x80);
		}
		return gametype;
	}

	if (!cv_kartvoterulechanges.value) // never (again)
		return gametype;

	switch (cv_kartvoterulechanges.value) // okay, we're having a gametype change! when's the next one, luv?
	{
		case 3: // always
			randmapbuffer[NUMMAPS] = 1; // every other vote (or always if !encorepossible)
			break;
		case 1: // sometimes
			randmapbuffer[NUMMAPS] = 5; // per "cup"
			break;
		default:
			// fallthrough - happens when clearing buffer, but needs a reasonable countdown if cvar is modified
		case 2: // frequent
			randmapbuffer[NUMMAPS] = 2; // ...every 1/2th-ish cup?
			break;
	}

	if (gametype == GT_BATTLE)
		return GT_RACE;
	return GT_BATTLE;
}

//
// G_GetGametypeColor
//
// Pretty and consistent ^u^
// See also M_GetGametypeColor.
//
UINT8 G_GetGametypeColor(INT16 gt)
{
	if (modeattacking // == ATTACKING_TIME
	|| gamestate == GS_TIMEATTACK)
		return orangemap[0];

	if (gt == GT_BATTLE)
		return redmap[0];

	if (gt == GT_RACE)
		return skymap[0];

	return 255; // FALLBACK
}

/** Get the typeoflevel flag needed to indicate support of a gametype.
  * \param gametype The gametype for which support is desired.
  * \return The typeoflevel flag to check for that gametype.
  * \author Graue <graue@oceanbase.org>
  */
UINT32 G_TOLFlag(INT32 pgametype)
{
	return gametypetol[pgametype];
}

static UINT32 TOLMaps(UINT32 tolflags)
{
	UINT32 num = 0;
	UINT32 i;

	// Find all the maps that are ok and and put them in an array.
	for (i = 0; i < NUMMAPS; i++)
	{
		if (!mapheaderinfo[i])
			continue;
		if (mapheaderinfo[i]->menuflags & LF2_HIDEINMENU) // Don't include Map Hell
			continue;
		if ((mapheaderinfo[i]->typeoflevel & tolflags) == tolflags)
			num++;
	}

	return num;
}

/** Select a random map with the given typeoflevel flags.
  * If no map has those flags, this arbitrarily gives you map 1.
  * \param tolflags The typeoflevel flags to insist on. Other bits may
  *                 be on too, but all of these must be on.
  * \return A random map with those flags, 1-based, or 1 if no map
  *         has those flags.
  * \author Graue <graue@oceanbase.org>
  */
static INT16 *okmaps = NULL;
INT16 G_RandMap(UINT32 tolflags, INT16 pprevmap, boolean ignorebuffer, UINT8 maphell, boolean callagainsoon, INT16 *extbuffer)
{
	INT32 numokmaps = 0;
	INT16 ix, bufx;
	UINT16 extbufsize = 0;
	boolean usehellmaps; // Only consider Hell maps in this pick

	if (!okmaps)
		okmaps = Z_Malloc(NUMMAPS * sizeof(INT16), PU_STATIC, NULL);

	if (extbuffer != NULL)
	{
		bufx = 0;
		while (extbuffer[bufx]) {
			extbufsize++; bufx++;
		}
	}

tryagain:

	usehellmaps = (maphell == 0 ? false : (maphell == 2 || M_RandomChance(FRACUNIT/100))); // 1% chance of Hell

	// Find all the maps that are ok and and put them in an array.
	for (ix = 0; ix < NUMMAPS; ix++)
	{
		boolean isokmap = true;

		if (!mapheaderinfo[ix])
			continue;

		if ((mapheaderinfo[ix]->typeoflevel & tolflags) != tolflags
			|| ix == pprevmap
			|| (!dedicated && M_MapLocked(ix+1))
			|| (usehellmaps != (mapheaderinfo[ix]->menuflags & LF2_HIDEINMENU))) // this is bad
			continue; //isokmap = false;

		if (!ignorebuffer)
		{
			if (extbufsize > 0)
			{
				for (bufx = 0; bufx < extbufsize; bufx++)
				{
					if (extbuffer[bufx] == -1) // Rest of buffer SHOULD be empty
						break;
					if (ix == extbuffer[bufx])
					{
						isokmap = false;
						break;
					}
				}

				if (!isokmap)
					continue;
			}

			for (bufx = 0; bufx < (maphell ? 3 : NUMMAPS); bufx++)
			{
				if (randmapbuffer[bufx] == -1) // Rest of buffer SHOULD be empty
					break;
				if (ix == randmapbuffer[bufx])
				{
					isokmap = false;
					break;
				}
			}

			if (!isokmap)
				continue;
		}

		if (pprevmap == -2) // title demo hack
		{
			lumpnum_t l;
			if ((l = W_CheckNumForName(va("%sS01",G_BuildMapName(ix+1)))) == LUMPERROR)
				continue;
		}

		okmaps[numokmaps++] = ix;
	}

	if (numokmaps == 0)  // If there's no matches... (Goodbye, incredibly silly function chains :V)
	{
		if (!ignorebuffer)
		{
			if (randmapbuffer[3] == -1) // Is the buffer basically empty?
			{
				ignorebuffer = true; // This will probably only help in situations where there's very few maps, but it's folly not to at least try it
				goto tryagain;
			}

			for (bufx = 3; bufx < NUMMAPS; bufx++) // Let's clear all but the three most recent maps...
				randmapbuffer[bufx] = -1;
			goto tryagain;
		}

		if (maphell) // Any wiggle room to loosen our restrictions here?
		{
			maphell--;
			goto tryagain;
		}

		ix = 0; // Sorry, none match. You get MAP01.
		for (bufx = 0; bufx < NUMMAPS+1; bufx++)
			randmapbuffer[bufx] = -1; // if we're having trouble finding a map we should probably clear it
	}
	else
		ix = okmaps[M_RandomKey(numokmaps)];

	if (!callagainsoon)
	{
		Z_Free(okmaps);
		okmaps = NULL;
	}

	return ix;
}

void G_AddMapToBuffer(INT16 map)
{
	INT16 bufx, refreshnum = max(0, (INT32)TOLMaps(G_TOLFlag(gametype))-3);

	// Add the map to the buffer.
	for (bufx = NUMMAPS-1; bufx > 0; bufx--)
		randmapbuffer[bufx] = randmapbuffer[bufx-1];
	randmapbuffer[0] = map;

	// We're getting pretty full, so lets flush this for future usage.
	if (randmapbuffer[refreshnum] != -1)
	{
		// Clear all but the five most recent maps.
		for (bufx = 5; bufx < NUMMAPS; bufx++) // bufx < refreshnum? Might not handle everything for gametype switches, though.
			randmapbuffer[bufx] = -1;
		//CONS_Printf("Random map buffer has been flushed.\n");
	}
}

//
// G_UpdateVisited
//
static void G_UpdateVisited(void)
{
	boolean spec = G_IsSpecialStage(gamemap);
	// Update visitation flags?
	if ((!modifiedgame || savemoddata) // Not modified
		&& !multiplayer && !demo.playback // SP/RA/NiGHTS mode
		&& !(spec && stagefailed)) // Not failed the special stage
	{
		UINT8 earnedEmblems;

		// Update visitation flags
		mapvisited[gamemap-1] |= MV_BEATEN;

		if (encoremode == true)
		{
			mapvisited[gamemap-1] |= MV_ENCORE;
		}

		if (modeattacking)
			G_UpdateRecordReplays();

		if ((earnedEmblems = M_CompletionEmblems()))
			CONS_Printf(M_GetText("\x82" "Earned %hu emblem%s for level completion.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");
	}
}

static boolean CanSaveLevel(INT32 mapnum)
{
	// SRB2Kart: No save files yet
	(void)mapnum;
	return false;
}

static void G_HandleSaveLevel(void)
{
	// do this before running the intermission or custom cutscene, mostly for the sake of marathon mode but it also massively reduces redundant file save events in f_finale.c
	if (nextmap >= 1100-1)
	{
		if (!gamecomplete)
			gamecomplete = 2; // special temporary mode to prevent using SP level select in pause menu until the intermission is over without restricting it in every intermission
		if (cursaveslot > 0)
		{
			if (marathonmode)
			{
				// don't keep a backup around when the run is done!
				if (FIL_FileExists(liveeventbackup))
					remove(liveeventbackup);
				cursaveslot = 0;
			}
			else if ((!modifiedgame || savemoddata) && !(netgame || multiplayer || ultimatemode || demo.recording || metalrecording || modeattacking))
				G_SaveGame((UINT32)cursaveslot, spstage_start);
		}
	}
	// and doing THIS here means you don't lose your progress if you close the game mid-intermission
	else if (!(ultimatemode || netgame || multiplayer || demo.playback || demo.recording || metalrecording || modeattacking)
		&& (!modifiedgame || savemoddata) && cursaveslot > 0 && CanSaveLevel(lastmap+1))
		G_SaveGame((UINT32)cursaveslot, lastmap+1); // not nextmap+1 to route around special stages
}

//
// G_DoCompleted
//
static void G_DoCompleted(void)
{
	INT32 i, j = 0;
	boolean spec = G_IsSpecialStage(gamemap);
	SINT8 powertype = K_UsingPowerLevels();

	if (modeattacking && pausedelay)
		pausedelay = 0;

	gameaction = ga_nothing;

	if (metalplayback)
		G_StopMetalDemo();
	if (metalrecording)
		G_StopMetalRecording(false);

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
		{
			// SRB2Kart: exitlevel shouldn't get you the points
			if (!players[i].exiting && !(players[i].pflags & PF_GAMETYPEOVER))
			{
				if (players[i].bot)
				{
					K_FakeBotResults(&players[i]);
				}
				else
				{
					players[i].pflags |= PF_GAMETYPEOVER;

					if (P_IsLocalPlayer(&players[i]))
					{
						j++;
					}
				}
			}

			G_PlayerFinishLevel(i); // take away cards and stuff
		}

	// play some generic music if there's no win/cool/lose music going on (for exitlevel commands)
	if ((gametyperules & GTR_CIRCUIT) && ((multiplayer && demo.playback) || j == r_splitscreen+1) && (cv_inttime.value > 0))
		S_ChangeMusicInternal("racent", true);

	if (automapactive)
		AM_Stop();

	S_StopSounds();

	prevmap = (INT16)(gamemap-1);

	if (demo.playback) goto demointermission;

	// go to next level
	// nextmap is 0-based, unlike gamemap
	if (nextmapoverride != 0)
	{
		nextmap = (INT16)(nextmapoverride-1);
	}
	else if (marathonmode && mapheaderinfo[gamemap-1]->marathonnext)
	{
		nextmap = (INT16)(mapheaderinfo[gamemap-1]->marathonnext-1);
	}
	else if (grandprixinfo.gp == true)
	{
		if (grandprixinfo.roundnum == 0 || grandprixinfo.cup == NULL) // Single session
		{
			nextmap = prevmap; // Same map
		}
		else
		{
			if (grandprixinfo.roundnum >= grandprixinfo.cup->numlevels) // On final map
			{
				nextmap = 1101; // ceremonymap
			}
			else
			{
				// Proceed to next map
				nextmap = grandprixinfo.cup->levellist[grandprixinfo.roundnum];
				grandprixinfo.roundnum++;
			}
		}
	}
	else
	{
		nextmap = (INT16)(mapheaderinfo[gamemap-1]->nextlevel-1);
		if (marathonmode && nextmap == spmarathon_start-1)
			nextmap = 1100-1; // No infinite loop for you
	}

	// Remember last map for when you come out of the special stage.
	if (!spec)
		lastmap = nextmap;

	// If nextmap is actually going to get used, make sure it points to
	// a map of the proper gametype -- skip levels that don't support
	// the current gametype. (Helps avoid playing boss levels in Race,
	// for instance).
	if (!modeattacking && grandprixinfo.gp == false && (nextmap >= 0 && nextmap < NUMMAPS))
	{
		if (nextmap >= 0 && nextmap < NUMMAPS)
		{
			register INT16 cm = nextmap;
			UINT32 tolflag = G_TOLFlag(gametype);
			UINT8 visitedmap[(NUMMAPS+7)/8];

			memset(visitedmap, 0, sizeof (visitedmap));

			while (!mapheaderinfo[cm] || !(mapheaderinfo[cm]->typeoflevel & tolflag))
			{
				visitedmap[cm/8] |= (1<<(cm&7));
				if (!mapheaderinfo[cm])
					cm = -1; // guarantee error execution
				else if (marathonmode && mapheaderinfo[cm]->marathonnext)
					cm = (INT16)(mapheaderinfo[cm]->marathonnext-1);
				else
					cm = (INT16)(mapheaderinfo[cm]->nextlevel-1);

				if (cm >= NUMMAPS || cm < 0) // out of range (either 1100ish or error)
				{
					cm = nextmap; //Start the loop again so that the error checking below is executed.

					//Make sure the map actually exists before you try to go to it!
					if ((W_CheckNumForName(G_BuildMapName(cm + 1)) == LUMPERROR))
					{
						CONS_Alert(CONS_ERROR, M_GetText("Next map given (MAP %d) doesn't exist! Reverting to MAP01.\n"), cm+1);
						cm = 0;
						break;
					}
				}

				if (visitedmap[cm/8] & (1<<(cm&7))) // smells familiar
				{
					// We got stuck in a loop, came back to the map we started on
					// without finding one supporting the current gametype.
					// Thus, print a warning, and just use this map anyways.
					CONS_Alert(CONS_WARNING, M_GetText("Can't find a compatible map after map %d; using map %d anyway\n"), prevmap+1, cm+1);
					break;
				}
			}
			nextmap = cm;
		}

		// wrap around in race
		if (nextmap >= 1100-1 && nextmap <= 1102-1 && !(gametyperules & GTR_CAMPAIGN))
			nextmap = (INT16)(spstage_start-1);

		if (nextmap < 0 || (nextmap >= NUMMAPS && nextmap < 1100-1) || nextmap > 1103-1)
			I_Error("Followed map %d to invalid map %d\n", prevmap + 1, nextmap + 1);

		lastmap = nextmap; // Remember last map for when you come out of the special stage.
	}

	automapactive = false;

	if (!(gametyperules & GTR_CAMPAIGN))
	{
		if (cv_advancemap.value == 0) // Stay on same map.
		{
			nextmap = prevmap;
		}
		else if (cv_advancemap.value == 2) // Go to random map.
		{
			nextmap = G_RandMap(G_TOLFlag(gametype), prevmap, false, 0, false, NULL);
		}
	}

	// We are committed to this map now.
	// We may as well allocate its header if it doesn't exist
	// (That is, if it's a real map)
	if (nextmap < NUMMAPS && !mapheaderinfo[nextmap])
		P_AllocMapHeader(nextmap);

	// Set up power level gametype scrambles
	K_SetPowerLevelScrambles(powertype);

demointermission:

	// If the current gametype has no intermission screen set, then don't start it.
	Y_DetermineIntermissionType();

	if ((skipstats && !modeattacking) || (spec && modeattacking && stagefailed) || (intertype == int_none))
	{
		G_UpdateVisited();
		G_HandleSaveLevel();
		G_AfterIntermission();
	}
	else
	{
		G_SetGamestate(GS_INTERMISSION);
		Y_StartIntermission();
		G_UpdateVisited();
		G_HandleSaveLevel();
	}
}

// See also F_EndCutscene, the only other place which handles intra-map/ending transitions
void G_AfterIntermission(void)
{
	Y_CleanupScreenBuffer();

	if (modeattacking)
	{
		M_EndModeAttackRun();
		return;
	}

	if (gamecomplete == 2) // special temporary mode to prevent using SP level select in pause menu until the intermission is over without restricting it in every intermission
		gamecomplete = 1;

	HU_ClearCEcho();
	//G_NextLevel();

	if (demo.playback)
	{
		G_StopDemo();

		if (demo.inreplayhut)
			M_ReplayHut(0);
		else
			D_StartTitle();

		return;
	}
	else if (demo.recording && (modeattacking || demo.savemode != DSM_NOTSAVING))
		G_SaveDemo();

	if (modeattacking) // End the run.
	{
		M_EndModeAttackRun();
		return;
	}

	if ((gametyperules & GTR_CAMPAIGN) && mapheaderinfo[gamemap-1]->cutscenenum && !modeattacking && skipstats <= 1 && (gamecomplete || !(marathonmode & MA_NOCUTSCENES))) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1]->cutscenenum-1, false, false);
	else
	{
		if (nextmap < 1100-1)
			G_NextLevel();
		else
			G_EndGame();
	}
}

//
// G_NextLevel (WorldDone)
//
// init next level or go to the final scene
// called by end of intermission screen (y_inter)
//
void G_NextLevel(void)
{
	if (gamestate != GS_VOTING)
	{
		if ((cv_advancemap.value == 3) && grandprixinfo.gp == false && !modeattacking && !skipstats && (multiplayer || netgame))
		{
			UINT8 i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator)
				{
					gameaction = ga_startvote;
					return;
				}
			}
		}

		forceresetplayers = false;
		deferencoremode = (cv_kartencore.value == 1);
	}

	gameaction = ga_worlddone;
}

static void G_DoWorldDone(void)
{
	if (server)
	{
		// SRB2Kart
		D_MapChange(nextmap+1,
			gametype,
			deferencoremode,
			forceresetplayers,
			0,
			false,
			false);
	}

	gameaction = ga_nothing;
}

//
// G_DoStartVote
//
static void G_DoStartVote(void)
{
	if (server)
		D_SetupVote();
	gameaction = ga_nothing;
}

//
// G_UseContinue
//
void G_UseContinue(void)
{
	if (gamestate == GS_LEVEL && !netgame && !multiplayer)
	{
		gameaction = ga_startcont;
		lastdraw = true;
	}
}

static void G_DoStartContinue(void)
{
	I_Assert(!netgame && !multiplayer);

	legitimateexit = false;
	G_PlayerFinishLevel(consoleplayer); // take away cards and stuff

	//F_StartContinue();
	gameaction = ga_nothing;
}

//
// G_Continue
//
// re-init level, used by continue and possibly countdowntimeup
//
void G_Continue(void)
{
	if (!netgame && !multiplayer)
		gameaction = ga_continued;
}

static void G_DoContinued(void)
{
	player_t *pl = &players[consoleplayer];
	I_Assert(!netgame && !multiplayer);
	I_Assert(pl->continues > 0);

	if (pl->continues)
		pl->continues--;

	// Reset score
	pl->score = 0;

	// Allow tokens to come back
	tokenlist = 0;
	token = 0;

	if (!(netgame || multiplayer || demo.playback || demo.recording || metalrecording || modeattacking) && (!modifiedgame || savemoddata) && cursaveslot > 0)
		G_SaveGameOver((UINT32)cursaveslot, true);

	// Reset # of lives
	pl->lives = 3;

	D_MapChange(gamemap, gametype, false, false, 0, false, false);

	gameaction = ga_nothing;
}

//
// G_EndGame (formerly Y_EndGame)
// Frankly this function fits better in g_game.c than it does in y_inter.c
//
// ...Gee, (why) end the game?
// Because G_AfterIntermission and F_EndCutscene would
// both do this exact same thing *in different ways* otherwise,
// which made it so that you could only unlock Ultimate mode
// if you had a cutscene after the final level and crap like that.
// This function simplifies it so only one place has to be updated
// when something new is added.
void G_EndGame(void)
{
	if (demo.recording && (modeattacking || demo.savemode != DSM_NOTSAVING))
		G_SaveDemo();

	// Only do evaluation and credits in coop games.
	if (gametyperules & GTR_CAMPAIGN)
	{
		if (nextmap == 1103-1) // end game with ending
		{
			F_StartEnding();
			return;
		}
		if (nextmap == 1102-1) // end game with credits
		{
			F_StartCredits();
			return;
		}
		if (nextmap == 1101-1) // end game with evaluation
		{
			F_StartGameEvaluation();
			return;
		}
	}

	// 1100 or competitive multiplayer, so go back to title screen.
	D_StartTitle();
}

//
// G_LoadGameSettings
//
// Sets a tad of default info we need.
void G_LoadGameSettings(void)
{
	// defaults
	spstage_start = spmarathon_start = 1;
	sstage_start = 50;
	sstage_end = 56; // 7 special stages in vanilla SRB2
	sstage_end++; // plus one weirdo
	smpstage_start = 60;
	smpstage_end = 66; // 7 multiplayer special stages too

	// initialize free sfx slots for skin sounds
	S_InitRuntimeSounds();
}

// G_LoadGameData
// Loads the main data file, which stores information such as emblems found, etc.
void G_LoadGameData(void)
{
	size_t length;
	INT32 i, j;
	UINT8 modded = false;
	UINT8 rtemp;

	//For records
	tic_t rectime;
	tic_t reclap;

	// Clear things so previously read gamedata doesn't transfer
	// to new gamedata
	G_ClearRecords(); // main and nights records
	M_ClearSecrets(); // emblems, unlocks, maps visited, etc

	totalplaytime = 0; // total play time (separate from all)
	matchesplayed = 0; // SRB2Kart: matches played & finished

	for (i = 0; i < PWRLV_NUMTYPES; i++) // SRB2Kart: online rank system
		vspowerlevel[i] = PWRLVRECORD_START;

	if (M_CheckParm("-nodata"))
		return; // Don't load.

	// Allow saving of gamedata beyond this point
	gamedataloaded = true;

	if (M_CheckParm("-gamedata") && M_IsNextParm())
	{
		strlcpy(gamedatafilename, M_GetNextParm(), sizeof gamedatafilename);
	}

	if (M_CheckParm("-resetdata"))
		return; // Don't load (essentially, reset).

	length = FIL_ReadFile(va(pandf, srb2home, gamedatafilename), &savebuffer);
	if (!length) // Aw, no game data. Their loss!
		return;

	save_p = savebuffer;

	// Version check
	if (READUINT32(save_p) != 0xFCAFE211)
	{
		const char *gdfolder = "the SRB2Kart folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		Z_Free(savebuffer);
		save_p = NULL;
		I_Error("Game data is from another version of SRB2.\nDelete %s(maybe in %s) and try again.", gamedatafilename, gdfolder);
	}

	totalplaytime = READUINT32(save_p);
	matchesplayed = READUINT32(save_p);

	for (i = 0; i < PWRLV_NUMTYPES; i++)
	{
		vspowerlevel[i] = READUINT16(save_p);
		if (vspowerlevel[i] < PWRLVRECORD_MIN || vspowerlevel[i] > PWRLVRECORD_MAX)
			goto datacorrupt;
	}

	modded = READUINT8(save_p);

	// Aha! Someone's been screwing with the save file!
	if ((modded && !savemoddata))
		goto datacorrupt;
	else if (modded != true && modded != false)
		goto datacorrupt;

	// TODO put another cipher on these things? meh, I don't care...
	for (i = 0; i < NUMMAPS; i++)
		if ((mapvisited[i] = READUINT8(save_p)) > MV_MAX)
			goto datacorrupt;

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			emblemlocations[j+i].collected = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXEXTRAEMBLEMS;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXEXTRAEMBLEMS; ++j)
			extraemblems[j+i].collected = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			unlockables[j+i].unlocked = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			conditionSets[j+i].achieved = ((rtemp >> j) & 1);
		i += j;
	}

	timesBeaten = READUINT32(save_p);

	// Main records
	for (i = 0; i < NUMMAPS; ++i)
	{
		rectime = (tic_t)READUINT32(save_p);
		reclap  = (tic_t)READUINT32(save_p);

		if (rectime || reclap)
		{
			G_AllocMainRecordData((INT16)i);
			mainrecords[i]->time = rectime;
			mainrecords[i]->lap = reclap;
		}
	}

	// done
	Z_Free(savebuffer);
	save_p = NULL;

	// Silent update unlockables in case they're out of sync with conditions
	M_SilentUpdateUnlockablesAndEmblems();

	return;

	// Landing point for corrupt gamedata
	datacorrupt:
	{
		const char *gdfolder = "the SRB2Kart folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		Z_Free(savebuffer);
		save_p = NULL;

		I_Error("Corrupt game data file.\nDelete %s(maybe in %s) and try again.", gamedatafilename, gdfolder);
	}
}

// G_SaveGameData
// Saves the main data file, which stores information such as emblems found, etc.
void G_SaveGameData(void)
{
	size_t length;
	INT32 i, j;
	UINT8 btemp;

	if (!gamedataloaded)
		return; // If never loaded (-nodata), don't save

	save_p = savebuffer = (UINT8 *)malloc(GAMEDATASIZE);
	if (!save_p)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for saving game data\n"));
		return;
	}

#if 0
	// SRB2Kart: Let players unlock stuff with addons.
	if (modifiedgame && !savemoddata)
	{
		free(savebuffer);
		save_p = savebuffer = NULL;
		return;
	}
#endif

	// Version test
	WRITEUINT32(save_p, 0xFCAFE211);

	WRITEUINT32(save_p, totalplaytime);
	WRITEUINT32(save_p, matchesplayed);

	for (i = 0; i < PWRLV_NUMTYPES; i++)
		WRITEUINT16(save_p, vspowerlevel[i]);

	WRITEUINT8(save_p, (UINT8)savemoddata);

	// TODO put another cipher on these things? meh, I don't care...
	for (i = 0; i < NUMMAPS; i++)
		WRITEUINT8(save_p, (mapvisited[i] & MV_MAX));

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			btemp |= (emblemlocations[j+i].collected << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXEXTRAEMBLEMS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEXTRAEMBLEMS; ++j)
			btemp |= (extraemblems[j+i].collected << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			btemp |= (unlockables[j+i].unlocked << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			btemp |= (conditionSets[j+i].achieved << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}

	WRITEUINT32(save_p, timesBeaten);

	// Main records
	for (i = 0; i < NUMMAPS; i++)
	{
		if (mainrecords[i])
		{
			WRITEUINT32(save_p, mainrecords[i]->time);
			WRITEUINT32(save_p, mainrecords[i]->lap);
		}
		else
		{
			WRITEUINT32(save_p, 0);
			WRITEUINT32(save_p, 0);
		}
		WRITEUINT8(save_p, 0); // compat
	}

	length = save_p - savebuffer;

	FIL_WriteFile(va(pandf, srb2home, gamedatafilename), savebuffer, length);
	free(savebuffer);
	save_p = savebuffer = NULL;
}

#define VERSIONSIZE 16

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
void G_LoadGame(UINT32 slot, INT16 mapoverride)
{
	size_t length;
	char vcheck[VERSIONSIZE];
	char savename[255];

	// memset savedata to all 0, fixes calling perfectly valid saves corrupt because of bots
	memset(&savedata, 0, sizeof(savedata));

#ifdef SAVEGAME_OTHERVERSIONS
	//Oh christ.  The force load response needs access to mapoverride too...
	startonmapnum = mapoverride;
#endif

	if (marathonmode)
		strcpy(savename, liveeventbackup);
	else
		sprintf(savename, savegamename, slot);

	length = FIL_ReadFile(savename, &savebuffer);
	if (!length)
	{
		CONS_Printf(M_GetText("Couldn't read file %s\n"), savename);
		return;
	}

	save_p = savebuffer;

	memset(vcheck, 0, sizeof (vcheck));
	sprintf(vcheck, (marathonmode ? "back-up %d" : "version %d"), VERSION);
	if (strcmp((const char *)save_p, (const char *)vcheck))
	{
#ifdef SAVEGAME_OTHERVERSIONS
		M_StartMessage(M_GetText("Save game from different version.\nYou can load this savegame, but\nsaving afterwards will be disabled.\n\nDo you want to continue anyway?\n\n(Press 'Y' to confirm)\n"),
		               M_ForceLoadGameResponse, MM_YESNO);
		//Freeing done by the callback function of the above message
#else
		M_ClearMenus(true); // so ESC backs out to title
		M_StartMessage(M_GetText("Save game from different version\n\nPress ESC\n"), NULL, MM_NOTHING);
		Command_ExitGame_f();
		Z_Free(savebuffer);
		save_p = savebuffer = NULL;

		// no cheating!
		memset(&savedata, 0, sizeof(savedata));
#endif
		return; // bad version
	}
	save_p += VERSIONSIZE;

	if (demo.playback) // reset game engine
		G_StopDemo();

//	paused = false;
//	automapactive = false;

	// dearchive all the modifications
	if (!P_LoadGame(mapoverride))
	{
		M_ClearMenus(true); // so ESC backs out to title
		M_StartMessage(M_GetText("Savegame file corrupted\n\nPress ESC\n"), NULL, MM_NOTHING);
		Command_ExitGame_f();
		Z_Free(savebuffer);
		save_p = savebuffer = NULL;

		// no cheating!
		memset(&savedata, 0, sizeof(savedata));
		return;
	}
	if (marathonmode)
	{
		marathontime = READUINT32(save_p);
		marathonmode |= READUINT8(save_p);
	}

	// done
	Z_Free(savebuffer);
	save_p = savebuffer = NULL;

//	gameaction = ga_nothing;
//	G_SetGamestate(GS_LEVEL);
	displayplayers[0] = consoleplayer;
	multiplayer = false;
	splitscreen = 0;
	SplitScreen_OnChange(); // not needed?

//	G_DeferedInitNew(sk_medium, G_BuildMapName(1), 0, 0, 1);
	if (setsizeneeded)
		R_ExecuteSetViewSize();

	M_ClearMenus(true);
	CON_ToggleOff();
}

//
// G_SaveGame
// Saves your game.
//
void G_SaveGame(UINT32 slot, INT16 mapnum)
{
	boolean saved;
	char savename[256] = "";
	const char *backup;

	if (marathonmode)
		strcpy(savename, liveeventbackup);
	else
		sprintf(savename, savegamename, slot);
	backup = va("%s",savename);

	gameaction = ga_nothing;
	{
		char name[VERSIONSIZE];
		size_t length;

		save_p = savebuffer = (UINT8 *)malloc(SAVEGAMESIZE);
		if (!save_p)
		{
			CONS_Alert(CONS_ERROR, M_GetText("No more free memory for saving game data\n"));
			return;
		}

		memset(name, 0, sizeof (name));
		sprintf(name, (marathonmode ? "back-up %d" : "version %d"), VERSION);
		WRITEMEM(save_p, name, VERSIONSIZE);

		P_SaveGame(mapnum);
		if (marathonmode)
		{
			UINT32 writetime = marathontime;
			if (!(marathonmode & MA_INGAME))
				writetime += TICRATE*5; // live event backup penalty because we don't know how long it takes to get to the next map
			WRITEUINT32(save_p, writetime);
			WRITEUINT8(save_p, (marathonmode & ~MA_INIT));
		}

		length = save_p - savebuffer;
		saved = FIL_WriteFile(backup, savebuffer, length);
		free(savebuffer);
		save_p = savebuffer = NULL;
	}

	gameaction = ga_nothing;

	if (cv_debug && saved)
		CONS_Printf(M_GetText("Game saved.\n"));
	else if (!saved)
		CONS_Alert(CONS_ERROR, M_GetText("Error while writing to %s for save slot %u, base: %s\n"), backup, slot, (marathonmode ? liveeventbackup : savegamename));
}

#define BADSAVE goto cleanup;
#define CHECKPOS if (save_p >= end_p) BADSAVE
void G_SaveGameOver(UINT32 slot, boolean modifylives)
{
	boolean saved = false;
	size_t length;
	char vcheck[VERSIONSIZE];
	char savename[255];
	const char *backup;

	if (marathonmode)
		strcpy(savename, liveeventbackup);
	else
		sprintf(savename, savegamename, slot);
	backup = va("%s",savename);

	length = FIL_ReadFile(savename, &savebuffer);
	if (!length)
	{
		CONS_Printf(M_GetText("Couldn't read file %s\n"), savename);
		return;
	}

	{
		char temp[sizeof(timeattackfolder)];
		UINT8 *end_p = savebuffer + length;
		UINT8 *lives_p;
		SINT8 pllives;

		save_p = savebuffer;
		// Version check
		memset(vcheck, 0, sizeof (vcheck));
		sprintf(vcheck, (marathonmode ? "back-up %d" : "version %d"), VERSION);
		if (strcmp((const char *)save_p, (const char *)vcheck)) BADSAVE
		save_p += VERSIONSIZE;

		// P_UnArchiveMisc()
		(void)READINT16(save_p);
		CHECKPOS
		(void)READUINT16(save_p); // emeralds
		CHECKPOS
		READSTRINGN(save_p, temp, sizeof(temp)); // mod it belongs to
		if (strcmp(temp, timeattackfolder)) BADSAVE

		// P_UnArchivePlayer()
		CHECKPOS
		(void)READUINT16(save_p);
		CHECKPOS

		WRITEUINT8(save_p, numgameovers);
		CHECKPOS

		lives_p = save_p;
		pllives = READSINT8(save_p); // lives
		CHECKPOS
		if (modifylives && pllives < startinglivesbalance[numgameovers])
		{
			pllives = startinglivesbalance[numgameovers];
			WRITESINT8(lives_p, pllives);
		}

		(void)READINT32(save_p); // Score
		CHECKPOS
		(void)READINT32(save_p); // continues

		// File end marker check
		CHECKPOS
		switch (READUINT8(save_p))
		{
			case 0xb7:
				{
					UINT8 i, banksinuse;
					CHECKPOS
					banksinuse = READUINT8(save_p);
					CHECKPOS
					if (banksinuse > NUM_LUABANKS)
						BADSAVE
					for (i = 0; i < banksinuse; i++)
					{
						(void)READINT32(save_p);
						CHECKPOS
					}
					if (READUINT8(save_p) != 0x1d)
						BADSAVE
				}
			case 0x1d:
				break;
			default:
				BADSAVE
		}

		// done
		saved = FIL_WriteFile(backup, savebuffer, length);
	}

cleanup:
	if (cv_debug && saved)
		CONS_Printf(M_GetText("Game saved.\n"));
	else if (!saved)
		CONS_Alert(CONS_ERROR, M_GetText("Error while writing to %s for save slot %u, base: %s\n"), backup, slot, (marathonmode ? liveeventbackup : savegamename));
	Z_Free(savebuffer);
	save_p = savebuffer = NULL;

}
#undef CHECKPOS
#undef BADSAVE

//
// G_DeferedInitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayers[], playeringame[] should be set.
//
void G_DeferedInitNew(boolean pencoremode, const char *mapname, INT32 pickedchar, UINT8 ssplayers, boolean FLS)
{
	INT32 i;
	UINT16 color = SKINCOLOR_NONE;

	paused = false;

	if (demo.playback)
		COM_BufAddText("stopdemo\n");

	G_FreeGhosts(); // TODO: do we actually need to do this?

	for (i = 0; i < NUMMAPS+1; i++)
		randmapbuffer[i] = -1;

	// this leave the actual game if needed
	SV_StartSinglePlayerServer();

	if (splitscreen != ssplayers)
	{
		splitscreen = ssplayers;
		SplitScreen_OnChange();
	}

	SetPlayerSkinByNum(consoleplayer, pickedchar);
	CV_StealthSet(&cv_skin[0], skins[pickedchar].name);

	if (color != SKINCOLOR_NONE)
	{
		CV_StealthSetValue(&cv_playercolor[0], color);
	}

	if (mapname)
	{
		D_MapChange(M_MapNumber(mapname[3], mapname[4]), gametype, pencoremode, true, 1, false, FLS);
	}
}

//
// This is the map command interpretation something like Command_Map_f
//
// called at: map cmd execution, doloadgame, doplaydemo
void G_InitNew(UINT8 pencoremode, const char *mapname, boolean resetplayer, boolean skipprecutscene, boolean FLS)
{
	INT32 i;

	(void)FLS;

	Y_CleanupScreenBuffer();

	if (paused)
	{
		paused = false;
		S_ResumeAudio();
	}

	prevencoremode = ((gamestate == GS_TITLESCREEN) ? false : encoremode);
	encoremode = pencoremode;

	legitimateexit = false; // SRB2Kart
	comebackshowninfo = false;

	if (!demo.playback && !netgame) // Netgame sets random seed elsewhere, demo playback sets seed just before us!
		P_SetRandSeed(M_RandomizedSeed()); // Use a more "Random" random seed

	// Clear a bunch of variables
	redscore = bluescore = lastmap = 0;
	racecountdown = exitcountdown = mapreset = exitfadestarted = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		players[i].playerstate = PST_REBORN;
		players[i].starpostnum = 0;
		memset(&players[i].respawn, 0, sizeof (players[i].respawn));

		// The latter two should clear by themselves, but just in case
		players[i].pflags &= ~(PF_GAMETYPEOVER|PF_FULLSTASIS);

		// Clear cheatcodes too, just in case.
		players[i].pflags &= ~(PF_GODMODE|PF_NOCLIP|PF_INVIS);

		players[i].marescore = 0;

		if (resetplayer && !(multiplayer && demo.playback)) // SRB2Kart
		{
			players[i].lives = 3;
			players[i].xtralife = 0;
			players[i].totalring = 0;
			players[i].score = 0;
		}
	}

	// Reset unlockable triggers
	unlocktriggers = 0;

	// clear itemfinder, just in case
	if (!dedicated)	// except in dedicated servers, where it is not registered and can actually I_Error debug builds
		CV_StealthSetValue(&cv_itemfinder, 0);

	// internal game map
	// well this check is useless because it is done before (d_netcmd.c::command_map_f)
	// but in case of for demos....
	if (W_CheckNumForName(mapname) == LUMPERROR)
	{
		I_Error("Internal game map '%s' not found\n", mapname);
		Command_ExitGame_f();
		return;
	}

	gamemap = (INT16)M_MapNumber(mapname[3], mapname[4]); // get xx out of MAPxx

	// gamemap changed; we assume that its map header is always valid,
	// so make it so
	if(!mapheaderinfo[gamemap-1])
		P_AllocMapHeader(gamemap-1);

	maptol = mapheaderinfo[gamemap-1]->typeoflevel;
	globalweather = mapheaderinfo[gamemap-1]->weather;

	// Don't carry over custom music change to another map.
	mapmusflags |= MUSIC_RELOADRESET;

	automapactive = false;
	imcontinuing = false;

	if ((gametyperules & GTR_CAMPAIGN) && !skipprecutscene && mapheaderinfo[gamemap-1]->precutscenenum && !modeattacking && !(marathonmode & MA_NOCUTSCENES)) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1]->precutscenenum-1, true, resetplayer);
	else
	{
		LUAh_MapChange(gamemap);
		G_DoLoadLevel(resetplayer);
	}

	if (netgame)
	{
		char *title = G_BuildMapTitle(gamemap);

		CON_LogMessage(va(M_GetText("Map is now \"%s"), G_BuildMapName(gamemap)));
		if (title)
		{
			CON_LogMessage(va(": %s", title));
			Z_Free(title);
		}
		CON_LogMessage("\"\n");
	}
}


char *G_BuildMapTitle(INT32 mapnum)
{
	char *title = NULL;

	if (mapnum == 0)
		return Z_StrDup("Random");

	if (!mapheaderinfo[mapnum-1])
		P_AllocMapHeader(mapnum-1);

	if (strcmp(mapheaderinfo[mapnum-1]->lvlttl, ""))
	{
		size_t len = 1;
		const char *zonetext = NULL;
		const UINT8 actnum = mapheaderinfo[mapnum-1]->actnum;

		len += strlen(mapheaderinfo[mapnum-1]->lvlttl);
		if (strlen(mapheaderinfo[mapnum-1]->zonttl) > 0)
		{
			zonetext = M_GetText(mapheaderinfo[mapnum-1]->zonttl);
			len += strlen(zonetext) + 1;	// ' ' + zonetext
		}
		else if (!(mapheaderinfo[mapnum-1]->levelflags & LF_NOZONE))
		{
			zonetext = M_GetText("Zone");
			len += strlen(zonetext) + 1;	// ' ' + zonetext
		}

		if (actnum > 0)
			len += 1 + 11;					// ' ' + INT32

		title = Z_Malloc(len, PU_STATIC, NULL);

		sprintf(title, "%s", mapheaderinfo[mapnum-1]->lvlttl);
		if (zonetext) sprintf(title + strlen(title), " %s", zonetext);
		if (actnum > 0) sprintf(title + strlen(title), " %d", actnum);
	}

	return title;
}

static void measurekeywords(mapsearchfreq_t *fr,
		struct searchdim **dimp, UINT8 *cuntp,
		const char *s, const char *q, boolean wanttable)
{
	char *qp;
	char *sp;
	if (wanttable)
		(*dimp) = Z_Realloc((*dimp), 255 * sizeof (struct searchdim),
				PU_STATIC, NULL);
	for (qp = strtok(va("%s", q), " ");
			qp && fr->total < 255;
			qp = strtok(0, " "))
	{
		if (( sp = strcasestr(s, qp) ))
		{
			if (wanttable)
			{
				(*dimp)[(*cuntp)].pos = sp - s;
				(*dimp)[(*cuntp)].siz = strlen(qp);
			}
			(*cuntp)++;
			fr->total++;
		}
	}
	if (wanttable)
		(*dimp) = Z_Realloc((*dimp), (*cuntp) * sizeof (struct searchdim),
				PU_STATIC, NULL);
}

static void writesimplefreq(mapsearchfreq_t *fr, INT32 *frc,
		INT32 mapnum, UINT8 pos, UINT8 siz)
{
	fr[(*frc)].mapnum = mapnum;
	fr[(*frc)].matchd = ZZ_Alloc(sizeof (struct searchdim));
	fr[(*frc)].matchd[0].pos = pos;
	fr[(*frc)].matchd[0].siz = siz;
	fr[(*frc)].matchc = 1;
	fr[(*frc)].total = 1;
	(*frc)++;
}

INT32 G_FindMap(const char *mapname, char **foundmapnamep,
		mapsearchfreq_t **freqp, INT32 *freqcp)
{
	INT32 newmapnum = 0;
	INT32 mapnum;
	INT32 apromapnum = 0;

	size_t      mapnamelen;
	char   *realmapname = NULL;
	char   *newmapname = NULL;
	char   *apromapname = NULL;
	char   *aprop = NULL;

	mapsearchfreq_t *freq;
	boolean wanttable;
	INT32 freqc;
	UINT8 frequ;

	INT32 i;

	mapnamelen = strlen(mapname);

	/* Count available maps; how ugly. */
	for (i = 0, freqc = 0; i < NUMMAPS; ++i)
	{
		if (mapheaderinfo[i])
			freqc++;
	}

	freq = ZZ_Calloc(freqc * sizeof (mapsearchfreq_t));

	wanttable = !!( freqp );

	freqc = 0;
	for (i = 0, mapnum = 1; i < NUMMAPS; ++i, ++mapnum)
		if (mapheaderinfo[i])
	{
		if (!( realmapname = G_BuildMapTitle(mapnum) ))
			continue;

		aprop = realmapname;

		/* Now that we found a perfect match no need to fucking guess. */
		if (strnicmp(realmapname, mapname, mapnamelen) == 0)
		{
			if (wanttable)
			{
				writesimplefreq(freq, &freqc, mapnum, 0, mapnamelen);
			}
			if (newmapnum == 0)
			{
				newmapnum = mapnum;
				newmapname = realmapname;
				realmapname = 0;
				Z_Free(apromapname);
				if (!wanttable)
					break;
			}
		}
		else
		if (apromapnum == 0 || wanttable)
		{
			/* LEVEL 1--match keywords verbatim */
			if (( aprop = strcasestr(realmapname, mapname) ))
			{
				if (wanttable)
				{
					writesimplefreq(freq, &freqc,
							mapnum, aprop - realmapname, mapnamelen);
				}
				if (apromapnum == 0)
				{
					apromapnum = mapnum;
					apromapname = realmapname;
					realmapname = 0;
				}
			}
			else/* ...match individual keywords */
			{
				freq[freqc].mapnum = mapnum;
				measurekeywords(&freq[freqc],
						&freq[freqc].matchd, &freq[freqc].matchc,
						realmapname, mapname, wanttable);
				measurekeywords(&freq[freqc],
						&freq[freqc].keywhd, &freq[freqc].keywhc,
						mapheaderinfo[i]->keywords, mapname, wanttable);
				if (freq[freqc].total)
					freqc++;
			}
		}

		Z_Free(realmapname);/* leftover old name */
	}

	if (newmapnum == 0)/* no perfect match--try a substring */
	{
		newmapnum = apromapnum;
		newmapname = apromapname;
	}

	if (newmapnum == 0)/* calculate most queries met! */
	{
		frequ = 0;
		for (i = 0; i < freqc; ++i)
		{
			if (freq[i].total > frequ)
			{
				frequ = freq[i].total;
				newmapnum = freq[i].mapnum;
			}
		}
		if (newmapnum)
		{
			newmapname = G_BuildMapTitle(newmapnum);
		}
	}

	if (freqp)
		(*freqp) = freq;
	else
		Z_Free(freq);

	if (freqcp)
		(*freqcp) = freqc;

	if (foundmapnamep)
		(*foundmapnamep) = newmapname;
	else
		Z_Free(newmapname);

	return newmapnum;
}

void G_FreeMapSearch(mapsearchfreq_t *freq, INT32 freqc)
{
	INT32 i;
	for (i = 0; i < freqc; ++i)
	{
		Z_Free(freq[i].matchd);
	}
	Z_Free(freq);
}

INT32 G_FindMapByNameOrCode(const char *mapname, char **realmapnamep)
{
	boolean usemapcode = false;

	INT32 newmapnum;

	size_t mapnamelen;

	char *p;

	mapnamelen = strlen(mapname);

	if (mapnamelen == 2)/* maybe two digit code */
	{
		if (( newmapnum = M_MapNumber(mapname[0], mapname[1]) ))
			usemapcode = true;
	}
	else if (mapnamelen == 5 && strnicmp(mapname, "MAP", 3) == 0)
	{
		if (( newmapnum = M_MapNumber(mapname[3], mapname[4]) ))
			usemapcode = true;
	}

	if (!usemapcode)
	{
		/* Now detect map number in base 10, which no one asked for. */
		newmapnum = strtol(mapname, &p, 10);
		if (*p == '\0')/* we got it */
		{
			if (newmapnum < 1 || newmapnum > NUMMAPS)
			{
				CONS_Alert(CONS_ERROR, M_GetText("Invalid map number %d.\n"), newmapnum);
				return 0;
			}
			usemapcode = true;
		}
		else
		{
			newmapnum = G_FindMap(mapname, realmapnamep, NULL, NULL);
		}
	}

	if (usemapcode)
	{
		/* we can't check mapheaderinfo for this hahahaha */
		if (W_CheckNumForName(G_BuildMapName(newmapnum)) == LUMPERROR)
			return 0;

		if (realmapnamep)
			(*realmapnamep) = G_BuildMapTitle(newmapnum);
	}

	return newmapnum;
}

//
// G_SetGamestate
//
// Use this to set the gamestate, please.
//
void G_SetGamestate(gamestate_t newstate)
{
	gamestate = newstate;
}

/* These functions handle the exitgame flag. Before, when the user
   chose to end a game, it happened immediately, which could cause
   crashes if the game was in the middle of something. Now, a flag
   is set, and the game can then be stopped when it's safe to do
   so.
*/

// Used as a callback function.
void G_SetExitGameFlag(void)
{
	exitgame = true;
}

void G_ClearExitGameFlag(void)
{
	exitgame = false;
}

boolean G_GetExitGameFlag(void)
{
	return exitgame;
}

// Same deal with retrying.
void G_SetRetryFlag(void)
{
	retrying = true;
}

void G_ClearRetryFlag(void)
{
	retrying = false;
}

boolean G_GetRetryFlag(void)
{
	return retrying;
}

void G_SetModeAttackRetryFlag(void)
{
	retryingmodeattack = true;
	G_SetRetryFlag();
}

void G_ClearModeAttackRetryFlag(void)
{
	retryingmodeattack = false;
}

boolean G_GetModeAttackRetryFlag(void)
{
	return retryingmodeattack;
}

// Time utility functions
INT32 G_TicsToHours(tic_t tics)
{
	return tics/(3600*TICRATE);
}

INT32 G_TicsToMinutes(tic_t tics, boolean full)
{
	if (full)
		return tics/(60*TICRATE);
	else
		return tics/(60*TICRATE)%60;
}

INT32 G_TicsToSeconds(tic_t tics)
{
	return (tics/TICRATE)%60;
}

INT32 G_TicsToCentiseconds(tic_t tics)
{
	return (INT32)((tics%TICRATE) * (100.00f/TICRATE));
}

INT32 G_TicsToMilliseconds(tic_t tics)
{
	return (INT32)((tics%TICRATE) * (1000.00f/TICRATE));
}

