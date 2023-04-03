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
#include "i_time.h"
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
#include "k_menu.h"
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
#include "m_cond.h" // condition sets
#include "r_fps.h" // frame interpolation/uncapped
#include "lua_hud.h"

// SRB2kart
#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "k_color.h"
#include "k_respawn.h"
#include "k_grandprix.h"
#include "k_boss.h"
#include "k_specialstage.h"
#include "k_bot.h"
#include "doomstat.h"
#include "k_director.h"
#include "k_podium.h"
#include "k_rank.h"
#include "acs/interface.h"
#include "g_party.h"

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

gameaction_t gameaction;
gamestate_t gamestate = GS_NULL;
UINT8 ultimatemode = false;

JoyType_t Joystick[MAXSPLITSCREENPLAYERS];

// SRB2kart
char gamedatafilename[64] =
#if defined (TESTERS) || defined (HOSTTESTERS)
	"test"
#elif defined(DEVELOP)
	"develop"
#endif
	"ringdata.dat";
char timeattackfolder[64] = "ringracers";
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
UINT8 mapmusrng; // Random selection result

INT16 gamemap = 1;
UINT32 maptol;

preciptype_t globalweather = PRECIP_NONE;
preciptype_t curWeather = PRECIP_NONE;

precipprops_t precipprops[MAXPRECIP] =
{
	{"NONE",				MT_NULL, 			0}, // PRECIP_NONE
	{"RAIN",				MT_RAIN, 			0}, // PRECIP_RAIN
	{"SNOW",				MT_SNOWFLAKE,		0}, // PRECIP_SNOW
	{"BLIZZARD",			MT_BLIZZARDSNOW,	0}, // PRECIP_BLIZZARD
	{"STORM",				MT_RAIN,			PRECIPFX_THUNDER|PRECIPFX_LIGHTNING}, // PRECIP_STORM
	{"STORM_NORAIN",		MT_NULL,			PRECIPFX_THUNDER|PRECIPFX_LIGHTNING}, // PRECIP_STORM_NORAIN
	{"STORM_NOSTRIKES",		MT_RAIN,			PRECIPFX_THUNDER} // PRECIP_STORM_NOSTRIKES
};

preciptype_t precip_freeslot = PRECIP_FIRSTFREESLOT;

INT32 cursaveslot = 0; // Auto-save 1p savegame slot
//INT16 lastmapsaved = 0; // Last map we auto-saved at
INT16 lastmaploaded = 0; // Last map the game loaded
UINT8 gamecomplete = 0;

marathonmode_t marathonmode = 0;
tic_t marathontime = 0;

UINT8 numgameovers = 0; // for startinglives balance
SINT8 startinglivesbalance[maxgameovers+1] = {3, 5, 7, 9, 12, 15, 20, 25, 30, 40, 50, 75, 99, 0x7F};

UINT16 mainwads = 0;
UINT16 musicwads = 0;
boolean modifiedgame = false; // Set if homebrew PWAD stuff has been added.
boolean majormods = false; // Set if Lua/Gameplay SOC/replacement map has been added.
boolean savemoddata = false;
boolean usedCheats = false; // Set when a "cheats on" is ever used.
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
INT16 lastmap; // last level you were at (returning from special stages)
tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)

char * titlemap = NULL;
boolean hidetitlepics = false;
char * bootmap = NULL; //bootmap for loading a map on startup

char * tutorialmap = NULL; // map to load for tutorial
boolean tutorialmode = false; // are we in a tutorial right now?

char * podiummap = NULL; // map to load for podium

boolean looptitle = true;

UINT16 skincolor_redteam = SKINCOLOR_RED;
UINT16 skincolor_blueteam = SKINCOLOR_BLUE;
UINT16 skincolor_redring = SKINCOLOR_RASPBERRY;
UINT16 skincolor_bluering = SKINCOLOR_PERIWINKLE;

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
mapheader_t** mapheaderinfo = {NULL};
INT32 nummapheaders = 0;
INT32 mapallocsize = 0;

// Kart cup definitions
cupheader_t *kartcupheaders = NULL;
UINT16 numkartcupheaders = 0;

static boolean exitgame = false;
static boolean retrying = false;
static boolean retryingmodeattack = false;

UINT8 stagefailed; // Used for GEMS BONUS? Also to see if you beat the stage.

UINT16 emeralds;
INT32 luabanks[NUM_LUABANKS];

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
tic_t introtime = 3;
tic_t starttime = 3;

const tic_t bulbtime = TICRATE/2;
UINT8 numbulbs = 1;

tic_t raceexittime = 5*TICRATE + (2*TICRATE/3);
tic_t battleexittime = 8*TICRATE;

INT32 hyudorotime = 7*TICRATE;
INT32 stealtime = TICRATE/2;
INT32 sneakertime = TICRATE + (TICRATE/3);
INT32 itemtime = 8*TICRATE;
INT32 bubbletime = TICRATE/2;
INT32 comebacktime = 3*TICRATE;
INT32 bumptime = 6;
INT32 greasetics = 3*TICRATE;
INT32 wipeoutslowtime = 20;
INT32 wantedreduce = 5*TICRATE;
INT32 wantedfrequency = 10*TICRATE;
INT32 flameseg = TICRATE/4;

UINT8 use1upSound = 0;
UINT8 maxXtraLife = 2; // Max extra lives from rings

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

struct maplighting maplighting;

INT16 autobalance; //for CTF team balance
INT16 teamscramble; //for CTF team scramble
INT16 scrambleplayers[MAXPLAYERS]; //for CTF team scramble
INT16 scrambleteams[MAXPLAYERS]; //for CTF team scramble
INT16 scrambletotal; //for CTF team scramble
INT16 scramblecount; //for CTF team scramble

// SRB2Kart
// Cvars that we don't want changed mid-game
UINT8 numlaps; // Removed from Cvar hell
UINT8 gamespeed; // Game's current speed (or difficulty, or cc, or etc); 0 for easy, 1 for normal, 2 for hard
boolean encoremode = false; // Encore Mode currently enabled?
boolean prevencoremode;
boolean franticitems; // Frantic items currently enabled?

// Voting system
INT16 votelevels[4][2]; // Levels that were rolled by the host
SINT8 votes[MAXPLAYERS]; // Each player's vote
SINT8 pickedvote; // What vote the host rolls

// Server-sided, synched variables
tic_t wantedcalcdelay; // Time before it recalculates WANTED
tic_t itemCooldowns[NUMKARTITEMS - 1]; // Cooldowns to prevent item spawning
tic_t mapreset; // Map reset delay when enough players have joined an empty game
boolean thwompsactive; // Thwomps activate on lap 2
UINT8 lastLowestLap; // Last lowest lap, for activating race lap executors
SINT8 spbplace; // SPB exists, give the person behind better items
boolean rainbowstartavailable; // Boolean, keeps track of if the rainbow start was gotten
boolean inDuel; // Boolean, keeps track of if it is a 1v1

// Client-sided, unsynched variables (NEVER use in anything that needs to be synced with other players)
tic_t bombflashtimer = 0;	// Cooldown before another FlashPal can be intialized by a bomb exploding near a displayplayer. Avoids seizures.
boolean legitimateexit; // Did this client actually finish the match?
boolean comebackshowninfo; // Have you already seen the "ATTACK OR PROTECT" message?
tic_t curlap; // Current lap time
tic_t bestlap; // Best lap time

typedef struct
{
	INT16 *mapbuffer;			// Pointer to zone memory
	INT32 lastnummapheaders;	// Reset if nummapheaders != this
} randmaps_t;
static randmaps_t randmaps = {NULL, 0};

static void G_ResetRandMapBuffer(void)
{
	INT32 i;
	Z_Free(randmaps.mapbuffer);
	randmaps.lastnummapheaders = nummapheaders;
	randmaps.mapbuffer = Z_Malloc(randmaps.lastnummapheaders * sizeof(INT16), PU_STATIC, NULL);
	for (i = 0; i < randmaps.lastnummapheaders; i++)
		randmaps.mapbuffer[i] = -1;
}

typedef struct joystickvector2_s
{
	INT32 xaxis;
	INT32 yaxis;
} joystickvector2_t;

boolean precache = true; // if true, load all graphics at start

INT16 prevmap, nextmap;

static void weaponPrefChange(void);
static void weaponPrefChange2(void);
static void weaponPrefChange3(void);
static void weaponPrefChange4(void);

static void rumble_off_handle(void);
static void rumble_off_handle2(void);
static void rumble_off_handle3(void);
static void rumble_off_handle4(void);

// don't mind me putting these here, I was lazy to figure out where else I could put those without blowing up the compiler.

// chat timer thingy
static CV_PossibleValue_t chattime_cons_t[] = {{5, "MIN"}, {999, "MAX"}, {0, NULL}};
consvar_t cv_chattime = CVAR_INIT ("chattime", "8", CV_SAVE, chattime_cons_t, NULL);

// chatwidth
static CV_PossibleValue_t chatwidth_cons_t[] = {{64, "MIN"}, {150, "MAX"}, {0, NULL}};
consvar_t cv_chatwidth = CVAR_INIT ("chatwidth", "150", CV_SAVE, chatwidth_cons_t, NULL);

// chatheight
static CV_PossibleValue_t chatheight_cons_t[] = {{6, "MIN"}, {22, "MAX"}, {0, NULL}};
consvar_t cv_chatheight = CVAR_INIT ("chatheight", "8", CV_SAVE, chatheight_cons_t, NULL);

// chat notifications (do you want to hear beeps? I'd understand if you didn't.)
consvar_t cv_chatnotifications = CVAR_INIT ("chatnotifications", "On", CV_SAVE, CV_OnOff, NULL);

// chat spam protection (why would you want to disable that???)
consvar_t cv_chatspamprotection = CVAR_INIT ("chatspamprotection", "On", CV_SAVE, CV_OnOff, NULL);

// minichat text background
consvar_t cv_chatbacktint = CVAR_INIT ("chatbacktint", "On", CV_SAVE, CV_OnOff, NULL);

// old shit console chat. (mostly exists for stuff like terminal, not because I cared if anyone liked the old chat.)
static CV_PossibleValue_t consolechat_cons_t[] = {{0, "Window"}, {1, "Console"}, {2, "Window (Hidden)"}, {0, NULL}};
consvar_t cv_consolechat = CVAR_INIT ("chatmode", "Window", CV_SAVE, consolechat_cons_t, NULL);

// Shout settings
// The relevant ones are CV_NETVAR because too lazy to send them any other way
consvar_t cv_shoutname = CVAR_INIT ("shout_name", "SERVER", CV_NETVAR, NULL, NULL);

static CV_PossibleValue_t shoutcolor_cons_t[] =
{
	{-1, "Player color"},
	{0, "White"},
	{1, "Yellow"},
	{2, "Purple"},
	{3, "Green"},
	{4, "Blue"},
	{5, "Red"},
	{6, "Gray"},
	{7, "Orange"},
	{8, "Sky-blue"},
	{9, "Gold"},
	{10, "Lavender"},
	{11, "Aqua-green"},
	{12, "Magenta"},
	{13, "Pink"},
	{14, "Brown"},
	{15, "Tan"},
	{0, NULL}
};
consvar_t cv_shoutcolor = CVAR_INIT ("shout_color", "Red", CV_NETVAR, shoutcolor_cons_t, NULL);

// If on and you're an admin, your messages will automatically become shouts.
consvar_t cv_autoshout = CVAR_INIT ("autoshout", "Off", CV_NETVAR, CV_OnOff, NULL);

// Pause game upon window losing focus
consvar_t cv_pauseifunfocused = CVAR_INIT ("pauseifunfocused", "Yes", CV_SAVE, CV_YesNo, NULL);

// Display song credits
consvar_t cv_songcredits = CVAR_INIT ("songcredits", "On", CV_SAVE, CV_OnOff, NULL);

consvar_t cv_invincmusicfade = CVAR_INIT ("invincmusicfade", "300", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_growmusicfade = CVAR_INIT ("growmusicfade", "500", CV_SAVE, CV_Unsigned, NULL);

consvar_t cv_resetspecialmusic = CVAR_INIT ("resetspecialmusic", "Yes", CV_SAVE, CV_YesNo, NULL);

consvar_t cv_resume = CVAR_INIT ("resume", "Yes", CV_SAVE, CV_YesNo, NULL);

consvar_t cv_kickstartaccel[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("kickstartaccel", "Off", CV_SAVE|CV_CALL, CV_OnOff, weaponPrefChange),
	CVAR_INIT ("kickstartaccel2", "Off", CV_SAVE|CV_CALL, CV_OnOff, weaponPrefChange2),
	CVAR_INIT ("kickstartaccel3", "Off", CV_SAVE|CV_CALL, CV_OnOff, weaponPrefChange3),
	CVAR_INIT ("kickstartaccel4", "Off", CV_SAVE|CV_CALL, CV_OnOff, weaponPrefChange4)
};

consvar_t cv_shrinkme[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("shrinkme", "Off", CV_CALL, CV_OnOff, weaponPrefChange),
	CVAR_INIT ("shrinkme2", "Off", CV_CALL, CV_OnOff, weaponPrefChange2),
	CVAR_INIT ("shrinkme3", "Off", CV_CALL, CV_OnOff, weaponPrefChange3),
	CVAR_INIT ("shrinkme4", "Off", CV_CALL, CV_OnOff, weaponPrefChange4)
};

static CV_PossibleValue_t zerotoone_cons_t[] = {{0, "MIN"}, {FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_deadzone[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("deadzone", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL),
	CVAR_INIT ("deadzone2", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL),
	CVAR_INIT ("deadzone3", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL),
	CVAR_INIT ("deadzone4", "0.25", CV_FLOAT|CV_SAVE, zerotoone_cons_t, NULL)
};

consvar_t cv_rumble[MAXSPLITSCREENPLAYERS] = {
	CVAR_INIT ("rumble", "On", CV_SAVE|CV_CALL, CV_OnOff, rumble_off_handle),
	CVAR_INIT ("rumble2", "On", CV_SAVE|CV_CALL, CV_OnOff, rumble_off_handle2),
	CVAR_INIT ("rumble3", "On", CV_SAVE|CV_CALL, CV_OnOff, rumble_off_handle3),
	CVAR_INIT ("rumble4", "On", CV_SAVE|CV_CALL, CV_OnOff, rumble_off_handle4)
};

// now automatically allocated in D_RegisterClientCommands
// so that it doesn't have to be updated depending on the value of MAXPLAYERS
char player_names[MAXPLAYERS][MAXPLAYERNAME+1];
INT32 player_name_changes[MAXPLAYERS];

// Allocation for time and nights data
void G_AllocMainRecordData(INT16 i)
{
	if (i > nummapheaders || !mapheaderinfo[i])
		I_Error("G_AllocMainRecordData: Internal map ID %d not found (nummapheaders = %d)\n", i, nummapheaders);
	if (!mapheaderinfo[i]->mainrecord)
		mapheaderinfo[i]->mainrecord = Z_Malloc(sizeof(recorddata_t), PU_STATIC, NULL);
	memset(mapheaderinfo[i]->mainrecord, 0, sizeof(recorddata_t));
}

// MAKE SURE YOU SAVE DATA BEFORE CALLING THIS
void G_ClearRecords(void)
{
	INT16 i;
	cupheader_t *cup;

	for (i = 0; i < nummapheaders; ++i)
	{
		if (mapheaderinfo[i]->mainrecord)
		{
			Z_Free(mapheaderinfo[i]->mainrecord);
			mapheaderinfo[i]->mainrecord = NULL;
		}
	}

	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		memset(&cup->windata, 0, sizeof(cup->windata));
	}
}

// For easy retrieval of records
tic_t G_GetBestTime(INT16 map)
{
	if (!mapheaderinfo[map] || !mapheaderinfo[map]->mainrecord || mapheaderinfo[map]->mainrecord->time <= 0)
		return (tic_t)UINT32_MAX;

	return mapheaderinfo[map]->mainrecord->time;
}

// BE RIGHT BACK

// Not needed
/*
tic_t G_GetBestLap(INT16 map)
{
	if (!mapheaderinfo[map] || !mapheaderinfo[map]->mainrecord || mapheaderinfo[map]->mainrecord->lap <= 0)
		return (tic_t)UINT32_MAX;

	return mapheaderinfo[map]->mainrecord->lap;
}
*/

struct stickermedalinfo stickermedalinfo;

void G_UpdateTimeStickerMedals(UINT16 map, boolean showownrecord)
{
	emblem_t *emblem = M_GetLevelEmblems(map+1);
	boolean gonnadrawtime = false;

	memset(&stickermedalinfo, 0, sizeof(stickermedalinfo));
	stickermedalinfo.timetoreach = UINT32_MAX;

	while (emblem != NULL)
	{
		UINT8 i = 0;

		switch (emblem->type)
		{
			case ET_TIME:
			{
				break;
			}
			case ET_MAP:
			{
				if (emblem->flags & ME_SPBATTACK && cv_dummyspbattack.value)
					break;
				goto bademblem;
			}
			default:
				goto bademblem;
		}

		if (cv_dummyspbattack.value && !(emblem->flags & ME_SPBATTACK))
			return;

		if (!gamedata->collected[(emblem-emblemlocations)] && gonnadrawtime)
			break;

		// Simpler than having two checks
		if (stickermedalinfo.visiblecount == MAXMEDALVISIBLECOUNT)
			stickermedalinfo.visiblecount--;

		// Shuffle along, so [0] is the "main focus"
		for (i = stickermedalinfo.visiblecount; i > 0; i--)
		{
			stickermedalinfo.emblems[i] = stickermedalinfo.emblems[i-1];
		}
		stickermedalinfo.emblems[0] = emblem;
		stickermedalinfo.visiblecount++;

		if (!gamedata->collected[(emblem-emblemlocations)] || Playing())
			gonnadrawtime = true;

bademblem:
		emblem = M_GetLevelEmblems(-1);
	}

	if (stickermedalinfo.visiblecount > 0)
	{
		if (emblem != NULL && emblem != stickermedalinfo.emblems[0])
		{
			// Regenerate the entire array if this is unlocked
			stickermedalinfo.regenemblem = emblem;
		}
		emblem = stickermedalinfo.emblems[0];

		if (gonnadrawtime)
		{
			if (emblem->tag > 0)
			{
				if (emblem->tag > mapheaderinfo[map]->ghostCount
				|| mapheaderinfo[map]->ghostBrief[emblem->tag-1] == NULL)
					snprintf(stickermedalinfo.targettext, 9, "Invalid");
				else if (mapheaderinfo[map]->ghostBrief[emblem->tag-1]->time == UINT32_MAX)
					snprintf(stickermedalinfo.targettext, 9, "DNF");
				else
					stickermedalinfo.timetoreach = mapheaderinfo[map]->ghostBrief[emblem->tag-1]->time;
			}
			else
				stickermedalinfo.timetoreach = emblem->var;
		}
	}

	if (!gonnadrawtime && showownrecord)
	{
		stickermedalinfo.timetoreach = G_GetBestTime(map);
	}

	if (stickermedalinfo.timetoreach != UINT32_MAX)
	{
		snprintf(stickermedalinfo.targettext, 9, "%i'%02i\"%02i",
			G_TicsToMinutes(stickermedalinfo.timetoreach, false),
			G_TicsToSeconds(stickermedalinfo.timetoreach),
			G_TicsToCentiseconds(stickermedalinfo.timetoreach));
	}
}

void G_TickTimeStickerMedals(void)
{
	if (stickermedalinfo.jitter)
		stickermedalinfo.jitter--;

	if (players[consoleplayer].realtime > stickermedalinfo.timetoreach)
	{
		if (stickermedalinfo.norecord == false)
		{
			S_StartSound(NULL, sfx_s3k72); //sfx_s26d); -- you STOLE fizzy lifting drinks
			stickermedalinfo.norecord = true;
			stickermedalinfo.jitter = 4;
		}
	}
	else
	{
		stickermedalinfo.norecord = false;
	}
}

//
// G_UpdateRecords
//
// Update time attack records
//
void G_UpdateRecords(void)
{
	UINT8 earnedEmblems;

	// Record new best time
	if (!mapheaderinfo[gamemap-1]->mainrecord)
		G_AllocMainRecordData(gamemap-1);

	if (modeattacking & ATTACKING_TIME)
	{
		tic_t time = players[consoleplayer].realtime;
		if (players[consoleplayer].pflags & PF_NOCONTEST)
			time = UINT32_MAX;
		if (((mapheaderinfo[gamemap-1]->mainrecord->time == 0) || (time < mapheaderinfo[gamemap-1]->mainrecord->time))
		&& (time < UINT32_MAX)) // DNF
			mapheaderinfo[gamemap-1]->mainrecord->time = time;
	}
	else
	{
		mapheaderinfo[gamemap-1]->mainrecord->time = 0;
	}

	if (modeattacking & ATTACKING_LAP)
	{
		if ((mapheaderinfo[gamemap-1]->mainrecord->lap == 0) || (bestlap < mapheaderinfo[gamemap-1]->mainrecord->lap))
			mapheaderinfo[gamemap-1]->mainrecord->lap = bestlap;
	}
	else
	{
		mapheaderinfo[gamemap-1]->mainrecord->lap = 0;
	}

	// Check emblems when level data is updated
	if ((earnedEmblems = M_CheckLevelEmblems()))
	{
		if (stickermedalinfo.regenemblem != NULL
			&& gamedata->collected[(stickermedalinfo.regenemblem-emblemlocations)])
		{
			G_UpdateTimeStickerMedals(gamemap-1, false);
		}

		stickermedalinfo.jitter = 4*earnedEmblems;
		S_StartSound(NULL, sfx_ncitem);
	}

	M_UpdateUnlockablesAndExtraEmblems(true, true);
	gamedata->deferredsave = true;
}

//
// G_UpdateRecordReplays
//
// Update replay files/data, etc. for Record Attack
//
static void G_UpdateRecordReplays(void)
{
	char *gpath;
	char lastdemo[256], bestdemo[256];

	if (players[consoleplayer].pflags & PF_NOCONTEST)
	{
		players[consoleplayer].realtime = UINT32_MAX;
	}

	// Save demo!
	bestdemo[255] = '\0';
	lastdemo[255] = '\0';
	G_SetDemoTime(players[consoleplayer].realtime, bestlap);
	G_CheckDemoStatus();

	gpath = va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s",
			srb2home, timeattackfolder);
	M_MkdirEach(gpath, M_PathParts(gpath) - 3, 0755);

	strcat(gpath, PATHSEP);
	strcat(gpath, G_BuildMapName(gamemap));

	snprintf(lastdemo, 255, "%s-%s-last.lmp", gpath, cv_skin[0].string);

	if (modeattacking != ATTACKING_NONE && FIL_FileExists(lastdemo))
	{
		UINT8 *buf;
		size_t len;

		gpath = Z_StrDup(gpath);

		len = FIL_ReadFile(lastdemo, &buf);

		if (modeattacking & ATTACKING_TIME)
		{
			snprintf(bestdemo, 255, "%s-%s-time-best.lmp", gpath, cv_skin[0].string);
			if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & 1)
			{ // Better time, save this demo.
				if (FIL_FileExists(bestdemo))
					remove(bestdemo);
				FIL_WriteFile(bestdemo, buf, len);
				CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD TIME!"), M_GetText("Saved replay as"), bestdemo);
			}
		}

		if (modeattacking & ATTACKING_LAP)
		{
			snprintf(bestdemo, 255, "%s-%s-lap-best.lmp", gpath, cv_skin[0].string);
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

		Z_Free(gpath);
	}
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

// for consistency among messages: this sets cheats as used.
void G_SetUsedCheats(void)
{
	if (usedCheats)
		return;

	usedCheats = true;
	CONS_Alert(CONS_NOTICE, M_GetText("Cheats activated -- game must be restarted to save progress.\n"));

	// If in record attack recording, cancel it.
	if (modeattacking)
		M_EndModeAttackRun();
	else if (marathonmode)
		Command_ExitGame_f();
}
/** Returns the map lump name for a map number.
  *
  * \param map Map number.
  * \return Map name.
  * \sa G_MapNumber
  */
const char *G_BuildMapName(INT32 map)
{
	if (map > 0 && map <= nummapheaders && mapheaderinfo[map - 1] != NULL)
	{
		return mapheaderinfo[map - 1]->lumpname;
	}
	else
	{
		return NULL;
	}
}

/** Returns the map number for map lump name.
  *
  * \param name Map name;
  * \return Map number.
  * \sa G_BuildMapName, nextmapspecial_t
  */
INT32 G_MapNumber(const char * name)
{
#ifdef NEXTMAPINSOC
	if (strncasecmp("NEXTMAP_", name, 8) != 0)
#endif
	{
		INT32 map;

		for (map = 0; map < nummapheaders; ++map)
		{
			if (strcasecmp(mapheaderinfo[map]->lumpname, name) != 0)
				continue;

			return map;
		}

		return NEXTMAP_INVALID;
	}

#ifdef NEXTMAPINSOC
	name += 8;

	if (strcasecmp("EVALUATION", name) == 0)
		return NEXTMAP_EVALUATION;
	if (strcasecmp("CREDITS", name) == 0)
		return NEXTMAP_CREDITS;
	if (strcasecmp("CEREMONY", name) == 0)
		return NEXTMAP_CEREMONY;
	//if (strcasecmp("TITLE", name) == 0)
		return NEXTMAP_TITLE;
#endif
}

/** Clips the console player's mouse aiming to the current view.
  * Used whenever the player view is changed manually.
  *
  * \param aiming Pointer to the vertical angle to clip.
  * \return The clipped angle.
  */
INT32 G_ClipAimingPitch(INT32 *aiming)
{
	INT32 limitangle;

	limitangle = ANGLE_90 - 1;

	if (*aiming > limitangle)
		*aiming = limitangle;
	else if (*aiming < -limitangle)
		*aiming = -limitangle;

	return (*aiming);
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

static INT32 G_GetValueFromControlTable(INT32 deviceID, INT32 deadzone, INT32 *controltable)
{
	INT32 i, failret = NO_BINDS_REACHABLE;

	if (deviceID <= UNASSIGNED_DEVICE)
	{
		// An invalid device can't have any binds!
		return failret;
	}

	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		INT32 key = controltable[i];
		INT32 value = 0;

		// Invalid key number.
		if (G_KeyIsAvailable(key, deviceID) == false)
		{
			continue;
		}

		value = G_GetDeviceGameKeyDownArray(deviceID)[key];

		if (value >= deadzone)
		{
			return value;
		}

		failret = 0;
	}

	// Not pressed.
	return failret;
}

INT32 G_PlayerInputAnalog(UINT8 p, INT32 gc, UINT8 menuPlayers)
{
	const INT32 deadzone = (JOYAXISRANGE * cv_deadzone[p].value) / FRACUNIT;
	const INT32 keyboard_player = G_GetPlayerForDevice(KEYBOARD_MOUSE_DEVICE);
	const boolean in_menu = (menuPlayers > 0);
	const boolean main_player = (p == 0);
	INT32 deviceID = UNASSIGNED_DEVICE;
	INT32 value = -1;
	INT32 avail_gamepad_id = 0;
	INT32 i;
	boolean bind_was_reachable = false;

	if (p >= MAXSPLITSCREENPLAYERS)
	{
#ifdef PARANOIA
		CONS_Debug(DBG_GAMELOGIC, "G_PlayerInputAnalog: Invalid player ID %d\n", p);
#endif
		return 0;
	}

	deviceID = G_GetDeviceForPlayer(p);

	if ((in_menu == true && G_KeyBindIsNecessary(gc) == true) // In menu: check for all unoverrideable menu default controls.
		|| (in_menu == false && gc == gc_start)) // In gameplay: check for the unoverrideable start button to be able to bring up the menu.
	{
		value = G_GetValueFromControlTable(KEYBOARD_MOUSE_DEVICE, JOYAXISRANGE/4, &(menucontrolreserved[gc][0]));
		if (value > 0) // Check for press instead of bound.
		{
			// This is only intended for P1.
			if (main_player == true)
			{
				return value;
			}
			else
			{
				return 0;
			}
		}
	}

	// Player 1 is always allowed to use the keyboard in 1P, even if they got disconnected.
	if (main_player == true && keyboard_player == -1 && deviceID == UNASSIGNED_DEVICE)
	{
		deviceID = KEYBOARD_MOUSE_DEVICE;
	}

	// First, try our actual binds.
	value = G_GetValueFromControlTable(deviceID, deadzone, &(gamecontrol[p][gc][0]));
	if (value > 0)
	{
		return value;
	}
	if (value != NO_BINDS_REACHABLE)
	{
		bind_was_reachable = true;
	}

	// If you're on gamepad in 1P, and you didn't have a gamepad bind for this, then try your keyboard binds.
	if (main_player == true && keyboard_player == -1 && deviceID > KEYBOARD_MOUSE_DEVICE)
	{
		value = G_GetValueFromControlTable(KEYBOARD_MOUSE_DEVICE, deadzone, &(gamecontrol[p][gc][0]));
		if (value > 0)
		{
			return value;
		}
		if (value != NO_BINDS_REACHABLE)
		{
			bind_was_reachable = true;
		}
	}

	if (in_menu == true)
	{
		if (main_player == true)
		{
			// We are P1 controlling menus. We should be able to
			// control the menu with any unused gamepads, so
			// that gamepads are able to navigate to the player
			// setup menu in the first place.
			for (avail_gamepad_id = 0; avail_gamepad_id < G_GetNumAvailableGamepads(); avail_gamepad_id++)
			{
				INT32 tryDevice = G_GetAvailableGamepadDevice(avail_gamepad_id);
				if (tryDevice <= KEYBOARD_MOUSE_DEVICE)
				{
					continue;
				}

				for (i = 0; i < menuPlayers; i++)
				{
					if (tryDevice == G_GetDeviceForPlayer(i))
					{
						// Don't do this for already taken devices.
						break;
					}
				}

				if (i == menuPlayers)
				{
					// This gamepad isn't being used, so we can
					// use it for P1 menu navigation.
					value = G_GetValueFromControlTable(tryDevice, deadzone, &(gamecontrol[p][gc][0]));
					if (value > 0)
					{
						return value;
					}
					if (value != NO_BINDS_REACHABLE)
					{
						bind_was_reachable = true;
					}
				}
			}
		}

		if (bind_was_reachable == false)
		{
			// Still nothing bound after everything. Try default gamepad controls.
			value = G_GetValueFromControlTable(deviceID, deadzone, &(gamecontroldefault[gc][0]));
			if (value > 0)
			{
				return value;
			}
		}
	}

	// Literally not bound at all, so it can't be pressed at all.
	return 0;
}

#undef KEYBOARDDEFAULTSSPLIT

boolean G_PlayerInputDown(UINT8 p, INT32 gc, UINT8 menuPlayers)
{
	return (G_PlayerInputAnalog(p, gc, menuPlayers) != 0);
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

INT32 localsteering[MAXSPLITSCREENPLAYERS];

// Turning was removed from G_BuildTiccmd to prevent easy client hacking.
// This brings back the camera prediction that was lost.
static void G_DoAnglePrediction(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer, UINT8 viewnum, player_t *player)
{
	angle_t angleChange = 0;

	// Chasecam stops in these situations, so local cam should stop too.
	// Otherwise it'll jerk when it resumes.
	if (player->playerstate == PST_DEAD)
		return;
	if (player->mo != NULL && !P_MobjWasRemoved(player->mo) && player->mo->hitlag > 0)
		return;

	while (realtics > 0)
	{
		localsteering[ssplayer - 1] = K_UpdateSteeringValue(localsteering[ssplayer - 1], cmd->turning);
		angleChange = K_GetKartTurnValue(player, localsteering[ssplayer - 1]) << TICCMD_REDUCE;

		realtics--;
	}

#if 0
	// Left here in case it needs unsealing later. This tried to replicate an old localcam function, but this behavior was unpopular in tests.
	//if (player->pflags & PF_DRIFTEND)
	{
		localangle[ssplayer - 1] = player->mo->angle;
	}
	else
#endif
	{
		localangle[viewnum] += angleChange;
	}
}

void G_BuildTiccmd(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer)
{
	const UINT8 forplayer = ssplayer-1;
	const UINT8 viewnum = G_PartyPosition(g_localplayers[forplayer]);

	INT32 forward;

	joystickvector2_t joystickvector;

	player_t *player = &players[g_localplayers[forplayer]];
	//camera_t *thiscam = &camera[forplayer];
	//boolean *kbl = &keyboard_look[forplayer];
	//boolean *rd = &resetdown[forplayer];
	//const boolean mouseaiming = player->spectator;

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

	cmd->angle = localangle[viewnum] >> TICCMD_REDUCE;

	// why build a ticcmd if we're paused?
	// Or, for that matter, if we're being reborn.
	if (paused || P_AutoPause() || (gamestate == GS_LEVEL && player->playerstate == PST_REBORN))
	{
		return;
	}

	cmd->flags = 0;

	if (menuactive || chat_on || CON_Ready())
	{
		cmd->flags |= TICCMD_TYPING;

		if (hu_keystrokes)
		{
			cmd->flags |= TICCMD_KEYSTROKE;
		}

		goto aftercmdinput;
	}

	if (G_IsPartyLocal(displayplayers[forplayer]) == false)
	{
		if (M_MenuButtonPressed(forplayer, MBT_A))
		{
			G_AdjustView(ssplayer, 1, true);
			K_ToggleDirector(false);
		}

		if (M_MenuButtonPressed(forplayer, MBT_X))
		{
			G_AdjustView(ssplayer, -1, true);
			K_ToggleDirector(false);
		}

		if (player->spectator == true)
		{
			// duplication of fire
			if (G_PlayerInputDown(forplayer, gc_item, 0))
			{
				cmd->buttons |= BT_ATTACK;
			}

			if (M_MenuButtonPressed(forplayer, MBT_R))
			{
				K_ToggleDirector(true);
			}
		}

		goto aftercmdinput;
	}

	if (K_PlayerUsesBotMovement(player))
	{
		// Bot ticcmd is generated by K_BuildBotTiccmd
		return;
	}

	joystickvector.xaxis = G_PlayerInputAnalog(forplayer, gc_right, 0) - G_PlayerInputAnalog(forplayer, gc_left, 0);
	joystickvector.yaxis = 0;
	G_HandleAxisDeadZone(forplayer, &joystickvector);

	// For kart, I've turned the aim axis into a digital axis because we only
	// use it for aiming to throw items forward/backward and the vote screen
	// This mean that the turn axis will still be gradient but up/down will be 0
	// until the stick is pushed far enough
	joystickvector.yaxis = G_PlayerInputAnalog(forplayer, gc_down, 0) - G_PlayerInputAnalog(forplayer, gc_up, 0);

	if (encoremode)
	{
		joystickvector.xaxis = -joystickvector.xaxis;
	}

	forward = 0;
	cmd->turning = 0;
	cmd->aiming = 0;

	if (joystickvector.xaxis != 0)
	{
		cmd->turning -= (joystickvector.xaxis * KART_FULLTURN) / JOYAXISRANGE;
	}

	if (player->spectator || objectplacing) // SRB2Kart: spectators need special controls
	{
		if (G_PlayerInputDown(forplayer, gc_accel, 0))
		{
			cmd->buttons |= BT_ACCELERATE;
		}

		if (G_PlayerInputDown(forplayer, gc_brake, 0))
		{
			cmd->buttons |= BT_BRAKE;
		}

		if (G_PlayerInputDown(forplayer, gc_lookback, 0))
		{
			cmd->aiming -= joystickvector.yaxis;
		}
		else
		{
			if (joystickvector.yaxis < 0)
			{
				forward += MAXPLMOVE;
			}

			if (joystickvector.yaxis > 0)
			{
				forward -= MAXPLMOVE;
			}
		}
	}
	else
	{
		// forward with key or button // SRB2kart - we use an accel/brake instead of forward/backward.
		INT32 value = G_PlayerInputAnalog(forplayer, gc_accel, 0);
		if (value != 0)
		{
			cmd->buttons |= BT_ACCELERATE;
			forward += ((value * MAXPLMOVE) / JOYAXISRANGE);
		}

		value = G_PlayerInputAnalog(forplayer, gc_brake, 0);
		if (value != 0)
		{
			cmd->buttons |= BT_BRAKE;
			forward -= ((value * MAXPLMOVE) / JOYAXISRANGE);
		}

		// But forward/backward IS used for aiming.
		if (joystickvector.yaxis != 0)
		{
			cmd->throwdir -= (joystickvector.yaxis * KART_FULLTURN) / JOYAXISRANGE;
		}
	}

	// drift
	if (G_PlayerInputDown(forplayer, gc_drift, 0))
	{
		cmd->buttons |= BT_DRIFT;
	}

	// C
	if (G_PlayerInputDown(forplayer, gc_spindash, 0))
	{
		forward = 0;
		cmd->buttons |= BT_SPINDASHMASK;
	}

	// fire
	if (G_PlayerInputDown(forplayer, gc_item, 0))
	{
		cmd->buttons |= BT_ATTACK;
	}

	// rear view
	if (G_PlayerInputDown(forplayer, gc_lookback, 0))
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	// lua buttons a thru c
	if (G_PlayerInputDown(forplayer, gc_luaa, 0)) { cmd->buttons |= BT_LUAA; }
	if (G_PlayerInputDown(forplayer, gc_luab, 0)) { cmd->buttons |= BT_LUAB; }
	if (G_PlayerInputDown(forplayer, gc_luac, 0)) { cmd->buttons |= BT_LUAC; }

	// spectator aiming shit, ahhhh...
	/*
	{
		INT32 player_invert = invertmouse ? -1 : 1;
		INT32 screen_invert =
			(player->mo && (player->mo->eflags & MFE_VERTICALFLIP)
			 && (!thiscam->chase)) //because chasecam's not inverted
			 ? -1 : 1; // set to -1 or 1 to multiply

		axis = PlayerJoyAxis(ssplayer, AXISLOOK);
		if (analogjoystickmove && axis != 0 && lookaxis && player->spectator)
			cmd->aiming += (axis<<16) * screen_invert;

		// spring back if not using keyboard neither mouselookin'
		if (*kbl == false && !lookaxis && !mouseaiming)
			cmd->aiming = 0;

		if (player->spectator)
		{
			if (PlayerInputDown(ssplayer, gc_lookup) || (gamepadjoystickmove && axis < 0))
			{
				cmd->aiming += KB_LOOKSPEED * screen_invert;
				*kbl = true;
			}
			else if (PlayerInputDown(ssplayer, gc_lookdown) || (gamepadjoystickmove && axis > 0))
			{
				cmd->aiming -= KB_LOOKSPEED * screen_invert;
				*kbl = true;
			}
		}

		if (PlayerInputDown(ssplayer, gc_centerview)) // No need to put a spectator limit on this one though :V
			cmd->aiming = 0;
	}
	*/

	cmd->forwardmove += (SINT8)forward;

aftercmdinput:

	/* 	Lua: Allow this hook to overwrite ticcmd.
		We check if we're actually in a level because for some reason this Hook would run in menus and on the titlescreen otherwise.
		Be aware that within this hook, nothing but this player's cmd can be edited (otherwise we'd run in some pretty bad synching problems since this is clientsided, or something)

		Possible usages for this are:
			-Forcing the player to perform an action, which could otherwise require terrible, terrible hacking to replicate.
			-Preventing the player to perform an action, which would ALSO require some weirdo hacks.
			-Making some galaxy brain autopilot Lua if you're a masochist
			-Making a Mario Kart 8 Deluxe tier baby mode that steers you away from walls and whatnot. You know what, do what you want!
	*/
	if (addedtogame && gamestate == GS_LEVEL)
	{
		LUA_HookTiccmd(player, cmd, HOOK(PlayerCmd));

		// Send leveltime when this tic was generated to the server for control lag calculations.
		// Only do this when in a level. Also do this after the hook, so that it can't overwrite this.
		cmd->latency = (leveltime & TICCMD_LATENCYMASK);
	}

	if (cmd->forwardmove > MAXPLMOVE)
		cmd->forwardmove = MAXPLMOVE;
	else if (cmd->forwardmove < -MAXPLMOVE)
		cmd->forwardmove = -MAXPLMOVE;

	if (cmd->turning > KART_FULLTURN)
		cmd->turning = KART_FULLTURN;
	else if (cmd->turning < -KART_FULLTURN)
		cmd->turning = -KART_FULLTURN;

	if (cmd->throwdir > KART_FULLTURN)
		cmd->throwdir = KART_FULLTURN;
	else if (cmd->throwdir < -KART_FULLTURN)
		cmd->throwdir = -KART_FULLTURN;

	G_DoAnglePrediction(cmd, realtics, ssplayer, viewnum, player);
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
		dest[i].angle = (INT16)SHORT(src[i].angle);
		dest[i].throwdir = (INT16)SHORT(src[i].throwdir);
		dest[i].aiming = (INT16)SHORT(src[i].aiming);
		dest[i].buttons = (UINT16)SHORT(src[i].buttons);
		dest[i].latency = src[i].latency;
		dest[i].flags = src[i].flags;
	}
	return dest;
}

static void weaponPrefChange(void)
{
	if (Playing())
		WeaponPref_Send(0);
}

static void weaponPrefChange2(void)
{
	if (Playing())
		WeaponPref_Send(1);
}

static void weaponPrefChange3(void)
{
	if (Playing())
		WeaponPref_Send(2);
}

static void weaponPrefChange4(void)
{
	if (Playing())
		WeaponPref_Send(3);
}

static void rumble_off_handle(void)
{
	if (cv_rumble[0].value == 0)
		G_ResetPlayerDeviceRumble(0);
}

static void rumble_off_handle2(void)
{
	if (cv_rumble[1].value == 0)
		G_ResetPlayerDeviceRumble(1);
}

static void rumble_off_handle3(void)
{
	if (cv_rumble[2].value == 0)
		G_ResetPlayerDeviceRumble(2);
}

static void rumble_off_handle4(void)
{
	if (cv_rumble[3].value == 0)
		G_ResetPlayerDeviceRumble(3);
}

//
// G_DoLoadLevelEx
//
void G_DoLoadLevelEx(boolean resetplayer, gamestate_t newstate)
{
	boolean doAutomate = false;
	INT32 i;

	// Make sure objectplace is OFF when you first start the level!
	OP_ResetObjectplace();

	levelstarttic = gametic; // for time calculation

	if (wipegamestate == newstate)
		wipegamestate = -1; // force a wipe

	if (cv_currprofile.value == -1 && !demo.playback)
	{
		PR_ApplyProfilePretend(cv_ttlprofilen.value, 0);
		for (i = 1; i < cv_splitplayers.value; i++)
		{
			PR_ApplyProfile(cv_lastprofile[i].value, i);
		}
	}
	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	if (gamestate == GS_VOTING)
		Y_EndVote();

	// cleanup
	// Is this actually necessary? Doesn't F_StartTitleScreen already do a significantly more comprehensive check?
	if (newstate == GS_TITLESCREEN)
	{
		if (gamemap < 1 || gamemap > nummapheaders)
		{
			G_SetGamestate(GS_TITLESCREEN);
			titlemapinaction = false;

			Z_Free(titlemap);
			titlemap = NULL; // let's not infinite recursion ok

			Command_ExitGame_f();
			return;
		}
	}

	// Doing this matches HOSTMOD behavior.
	// Is that desired? IDK
	doAutomate = (gamestate != GS_LEVEL && newstate == GS_LEVEL);

	G_SetGamestate(newstate);
	if (wipegamestate == GS_MENU)
		M_ClearMenus(true);
	I_UpdateMouseGrab();

	K_ResetCeremony();

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (resetplayer || (playeringame[i] && players[i].playerstate == PST_DEAD))
			players[i].playerstate = PST_REBORN;
	}

	// Setup the level.
	if (!P_LoadLevel(false, false)) // this never returns false?
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
			P_ResetCamera(&players[displayplayers[i]], &camera[i]);
	}

	// clear cmd building stuff
	memset(gamekeydown, 0, sizeof (gamekeydown));
	G_ResetAllDeviceResponding();

	// clear hud messages remains (usually from game startup)
	CON_ClearHUD();

	server_lagless = cv_lagless.value;

	if (doAutomate == true)
	{
		Automate_Run(AEV_ROUNDSTART);
	}

	G_ResetAllDeviceRumbles();
}

void G_DoLoadLevel(boolean resetplayer)
{
	G_DoLoadLevelEx(resetplayer, GS_LEVEL);
}

//
// Start the title card.
//
void G_StartTitleCard(void)
{
	// The title card has been disabled for this map.
	// Oh well.
	if (demo.rewinding || !G_IsTitleCardAvailable())
	{
		WipeStageTitle = false;
		return;
	}

	// clear the hud
	CON_ClearHUD();

	// prepare status bar
	ST_startTitleCard();

	// play the sound
	{
		sfxenum_t kstart = sfx_kstart;
		if (K_CheckBossIntro() == true)
			kstart = sfx_ssa021;
		else if (encoremode == true)
			kstart = sfx_ruby2;
		S_StartSound(NULL, kstart);
	}

	// start the title card
	WipeStageTitle = (gamestate == GS_LEVEL);
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
		ST_runTitleCard();
		ST_preLevelTitleCardDrawer();
		I_FinishUpdate(); 	// page flip or blit buffer
		NetKeepAlive();		// Prevent timeouts

#ifdef HWRENDER
		if (moviemode && rendermode == render_opengl)
			M_LegacySaveFrame();
#endif

		while (!((nowtime = I_GetTime()) - lasttime))
		{
			I_Sleep(cv_sleep.value);
			I_UpdateTime(cv_timescale.value);
		}
		lasttime = nowtime;
	}
#endif
}

//
// Returns true if the current level has a title card.
//
boolean G_IsTitleCardAvailable(void)
{
	// Overwrites all other title card exceptions.
	if (K_CheckBossIntro() == true)
		return true;

	// The current level has no name.
	if (!mapheaderinfo[gamemap-1]->lvlttl[0])
		return false;

	// Instant white fade.
	if (gametyperules & GTR_SPECIALSTART)
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
	//INT32 i;

	// any other key pops up menu if in demos
	if (gameaction == ga_nothing && !demo.quitafterplaying &&
		((demo.playback && !modeattacking && !demo.title && !multiplayer) || gamestate == GS_TITLESCREEN))
	{
		if (ev->type == ev_keydown
		|| (ev->type == ev_gamepad_axis && ev->data1 >= JOYANALOGS
			&& ((abs(ev->data2) > JOYAXISRANGE/2
			|| abs(ev->data3) > JOYAXISRANGE/2))
		))
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
		{
			hu_keystrokes = true;
			return true; // chat ate the event
		}
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
		{
			hu_keystrokes = true;
			return true; // chat ate the event
		}

		if (F_CutsceneResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}
	else if (gamestate == GS_CREDITS || gamestate == GS_ENDING) // todo: keep ending here?
	{
		if (HU_Responder(ev))
		{
			hu_keystrokes = true;
			return true; // chat ate the event
		}

		if (F_CreditResponder(ev))
		{
			// Skip credits for everyone
			if (! netgame)
				F_StartGameEvaluation();
			else if (server || IsPlayerAdmin(consoleplayer))
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			return true;
		}
	}
	else if (gamestate == GS_CEREMONY)
	{
		if (HU_Responder(ev))
		{
			hu_keystrokes = true;
			return true; // chat ate the event
		}

		if (K_CeremonyResponder(ev))
		{
			nextmap = NEXTMAP_TITLE;
			G_EndGame();
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
	{
		if (HU_Responder(ev))
		{
			hu_keystrokes = true;
			return true; // chat ate the event
		}
	}

	if (gamestate == GS_LEVEL && ev->type == ev_keydown && multiplayer && demo.playback && !demo.freecam)
	{
		// Allow pausing
		if (
			//ev->data1 == gamecontrol[0][gc_pause][0]
			//|| ev->data1 == gamecontrol[0][gc_pause][1]
			ev->data1 == KEY_PAUSE
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

	switch (ev->type)
	{
		case ev_keydown:
			if (//ev->data1 == gamecontrol[0][gc_pause][0]
				//|| ev->data1 == gamecontrol[0][gc_pause][1]
				ev->data1 == KEY_PAUSE)
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

			/*
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
			*/

			return true;

		case ev_keyup:
			return false; // always let key up events filter down

		case ev_mouse:
			return true; // eat events

		case ev_gamepad_axis:
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
	if (( player->pflags & PF_NOCONTEST ))
		return false;

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
	startview = min(max(startview, -1), MAXPLAYERS);
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
	{
		if (G_PartySize(consoleplayer) < viewnum)
		{
			return;
		}

		/* Fall back on true self */
		playernum = G_PartyMember(consoleplayer, viewnum - 1);
	}

	// Call ViewpointSwitch hooks here.
	// The viewpoint was forcibly changed.
	LUA_HookViewpointSwitch(&players[g_localplayers[viewnum - 1]], &players[playernum], true);

	/* Focus our target view first so that we don't take its player. */
	(*displayplayerp) = playernum;
	if ((*displayplayerp) != olddisplayplayer)
	{
		camerap = &camera[viewnum-1];
		P_ResetCamera(&players[(*displayplayerp)], camerap);

		// Why does it need to be done twice?
		R_ResetViewInterpolation(viewnum);
		R_ResetViewInterpolation(viewnum);
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

	// change statusbar also if playing back demo
	if (demo.quitafterplaying)
		ST_changeDemoView();
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
		splits = max(playersviewable, G_PartySize(consoleplayer)); // don't delete local players
		r_splitscreen = splits - 1;
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
					if (players[i].bot == true && grandprixinfo.gp == true && grandprixinfo.masterbots == false)
					{
						players[i].botvars.difficulty--;

						if (players[i].botvars.difficulty < 1)
						{
							players[i].botvars.difficulty = 1;
						}
					}
					else
					{
						K_PlayerLoseLife(&players[i]);
					}
				}
			}

			D_MapChange(gamemap, gametype, (cv_kartencore.value == 1), false, 1, false, false);
		}
	}

	// do player reborns if needed
	if (G_GamestateUsesLevel() == true)
	{
		boolean changed = false;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].playerstate == PST_REBORN)
			{
				G_DoReborn(i);
				changed = true;
			}
		}

		if (changed == true)
		{
			K_UpdateAllPlayerPositions();
		}
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

	buf = gametic % BACKUPTICS;

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
					cmd->latency = min(((leveltime & TICCMD_LATENCYMASK) - cmd->latency) & TICCMD_LATENCYMASK, MAXPREDICTTICS-1);
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

		case GS_MENU:
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
			if (titlemapinaction)
				P_Ticker(run);

			F_TitleScreenTicker(run);
			break;

		case GS_CEREMONY:
			P_Ticker(run);
			K_CeremonyTicker(run);
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

		if (gametic % NAMECHANGERATE == 0)
		{
			memset(player_name_changes, 0, sizeof player_name_changes);
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

	p->mo->renderflags &= ~(RF_TRANSMASK|RF_BRIGHTMASK); // cancel invisibility
	P_FlashPal(p, 0, 0); // Resets

	p->starpostnum = 0;
	memset(&p->respawn, 0, sizeof (p->respawn));
}

//
// G_PlayerReborn
// Called after a player dies. Almost everything is cleared and initialized.
//
void G_PlayerReborn(INT32 player, boolean betweenmaps)
{
	player_t *p;
	INT32 score, roundscore;
	INT32 lives;

	UINT8 kartspeed;
	UINT8 kartweight;

	boolean followerready;
	INT32 followerskin;
	UINT16 followercolor;

	mobj_t *follower; // old follower, will probably be removed by the time we're dead but you never know.
	mobj_t *hoverhyudoro;
	mobj_t *skyboxviewpoint;
	mobj_t *skyboxcenterpoint;

	INT32 charflags;
	UINT32 followitem;

	INT32 pflags;

	UINT8 ctfteam;

	INT32 starpostnum;
	INT32 exiting;
	INT32 khudfinish;
	INT32 khudcardanimation;
	INT16 totalring;
	UINT8 laps;
	UINT8 latestlap;
	UINT32 lapPoints;
	UINT16 skincolor;
	INT32 skin;
	UINT8 availabilities[MAXAVAILABILITY];
	UINT8 fakeskin;
	UINT8 lastfakeskin;

	tic_t jointime;

	UINT8 splitscreenindex;
	boolean spectator;
	boolean bot;
	UINT8 botdifficulty;

	INT16 rings;
	INT16 spheres;
	INT16 steering;
	angle_t playerangleturn;

	UINT8 botdiffincrease;
	boolean botrival;

	SINT8 xtralife;

	uint8_t public_key[PUBKEYLENGTH];

	// SRB2kart
	itemroulette_t itemRoulette;
	respawnvars_t respawn;
	INT32 itemtype;
	INT32 itemamount;
	INT32 growshrinktimer;
	boolean songcredit = false;
	UINT16 nocontrol;
	INT32 khudfault;
	INT32 kickstartaccel;
	boolean enteredGame;

	roundconditions_t roundconditions;
	boolean saveroundconditions;

	score = players[player].score;
	lives = players[player].lives;
	ctfteam = players[player].ctfteam;

	jointime = players[player].jointime;

	splitscreenindex = players[player].splitscreenindex;
	spectator = players[player].spectator;

	steering = players[player].steering;
	playerangleturn = players[player].angleturn;

	skincolor = players[player].skincolor;
	skin = players[player].skin;

	if (betweenmaps)
	{
		fakeskin = MAXSKINS;
		kartspeed = skins[players[player].skin].kartspeed;
		kartweight = skins[players[player].skin].kartweight;
		charflags = skins[players[player].skin].flags;
	}
	else
	{
		UINT32 skinflags = (demo.playback)
			? demo.skinlist[demo.currentskinid[player]].flags
			: skins[players[player].skin].flags;

		fakeskin = players[player].fakeskin;
		kartspeed = players[player].kartspeed;
		kartweight = players[player].kartweight;

		charflags = (skinflags & SF_IRONMAN) ? skinflags : players[player].charflags;
	}
	lastfakeskin = players[player].lastfakeskin;

	followerready = players[player].followerready;
	followercolor = players[player].followercolor;
	followerskin = players[player].followerskin;

	memcpy(availabilities, players[player].availabilities, sizeof(availabilities));

	followitem = players[player].followitem;

	bot = players[player].bot;
	botdifficulty = players[player].botvars.difficulty;

	botdiffincrease = players[player].botvars.diffincrease;
	botrival = players[player].botvars.rival;

	totalring = players[player].totalring;
	xtralife = players[player].xtralife;

	pflags = (players[player].pflags & (PF_WANTSTOJOIN|PF_KICKSTARTACCEL|PF_SHRINKME|PF_SHRINKACTIVE));

	// SRB2kart
	memcpy(&itemRoulette, &players[player].itemRoulette, sizeof (itemRoulette));
	memcpy(&respawn, &players[player].respawn, sizeof (respawn));
	memcpy(&public_key, &players[player].public_key, sizeof(public_key));

	if (betweenmaps || leveltime < introtime)
	{
		itemRoulette.active = false;

		itemtype = 0;
		itemamount = 0;
		growshrinktimer = 0;
		if (gametyperules & GTR_SPHERES)
		{
			rings = 0;
		}
		else if (modeattacking & ATTACKING_SPB)
		{
			rings = 20;
		}
		else
		{
			rings = 5;
		}
		spheres = 0;
		kickstartaccel = 0;
		khudfault = 0;
		nocontrol = 0;
		laps = 0;
		latestlap = 0;
		lapPoints = 0;
		roundscore = 0;
		exiting = 0;
		khudfinish = 0;
		khudcardanimation = 0;
		starpostnum = 0;

		saveroundconditions = false;
	}
	else
	{
		if (players[player].pflags & PF_ITEMOUT)
		{
			itemtype = 0;
			itemamount = 0;
		}
		else
		{
			itemtype = players[player].itemtype;
			itemamount = players[player].itemamount;
		}

		// Keep Shrink status, remove Grow status
		if (players[player].growshrinktimer < 0)
			growshrinktimer = players[player].growshrinktimer;
		else
			growshrinktimer = 0;

		rings = players[player].rings;
		spheres = players[player].spheres;
		kickstartaccel = players[player].kickstartaccel;

		khudfault = players[player].karthud[khud_fault];
		nocontrol = players[player].nocontrol;

		laps = players[player].laps;
		latestlap = players[player].latestlap;
		lapPoints = players[player].lapPoints;

		roundscore = players[player].roundscore;

		exiting = players[player].exiting;
		if (exiting > 0)
		{
			khudfinish = players[player].karthud[khud_finish];
			khudcardanimation = players[player].karthud[khud_cardanimation];
		}
		else
		{
			khudfinish = 0;
			khudcardanimation = 0;
		}

		starpostnum = players[player].starpostnum;

		pflags |= (players[player].pflags & (PF_STASIS|PF_ELIMINATED|PF_NOCONTEST|PF_FAULT|PF_LOSTLIFE));

		memcpy(&roundconditions, &players[player].roundconditions, sizeof (roundconditions));
		saveroundconditions = true;
	}

	if (!betweenmaps)
	{
		follower = players[player].follower;
		P_SetTarget(&players[player].follower, NULL);
		P_SetTarget(&players[player].awayview.mobj, NULL);
		P_SetTarget(&players[player].stumbleIndicator, NULL);
		P_SetTarget(&players[player].followmobj, NULL);

		hoverhyudoro = players[player].hoverhyudoro;
		skyboxviewpoint = players[player].skybox.viewpoint;
		skyboxcenterpoint = players[player].skybox.centerpoint;
	}
	else
	{
		follower = hoverhyudoro = NULL;
		skyboxviewpoint = skyboxcenterpoint = NULL;
	}

	enteredGame = players[player].enteredGame;

	p = &players[player];
	memset(p, 0, sizeof (*p));

	p->score = score;
	p->roundscore = roundscore;
	p->lives = lives;
	p->pflags = pflags;
	p->ctfteam = ctfteam;
	p->jointime = jointime;
	p->splitscreenindex = splitscreenindex;
	p->spectator = spectator;
	p->steering = steering;
	p->angleturn = playerangleturn;

	// save player config truth reborn
	p->skincolor = skincolor;
	p->skin = skin;

	p->fakeskin = fakeskin;
	p->kartspeed = kartspeed;
	p->kartweight = kartweight;
	p->charflags = charflags;
	p->lastfakeskin = lastfakeskin;

	memcpy(players[player].availabilities, availabilities, sizeof(availabilities));
	p->followitem = followitem;

	p->starpostnum = starpostnum;
	p->exiting = exiting;
	p->karthud[khud_finish] = khudfinish;
	p->karthud[khud_cardanimation] = khudcardanimation;

	p->laps = laps;
	p->latestlap = latestlap;
	p->lapPoints = lapPoints;
	p->totalring = totalring;

	p->bot = bot;
	p->botvars.difficulty = botdifficulty;
	p->rings = rings;
	p->spheres = spheres;
	p->botvars.diffincrease = botdiffincrease;
	p->botvars.rival = botrival;
	p->xtralife = xtralife;

	// SRB2kart
	p->itemtype = itemtype;
	p->itemamount = itemamount;
	p->growshrinktimer = growshrinktimer;
	p->karmadelay = 0;
	p->eggmanblame = -1;
	p->lastdraft = -1;
	p->karthud[khud_fault] = khudfault;
	p->nocontrol = nocontrol;
	p->kickstartaccel = kickstartaccel;

	p->botvars.rubberband = FRACUNIT;
	p->botvars.controller = UINT16_MAX;

	memcpy(&p->itemRoulette, &itemRoulette, sizeof (p->itemRoulette));
	memcpy(&p->respawn, &respawn, sizeof (p->respawn));

	memcpy(&p->public_key, &public_key, sizeof(p->public_key));

	if (saveroundconditions)
		memcpy(&p->roundconditions, &roundconditions, sizeof (p->roundconditions));

	if (follower)
		P_RemoveMobj(follower);

	p->hoverhyudoro = hoverhyudoro;
	p->skybox.viewpoint = skyboxviewpoint;
	p->skybox.centerpoint = skyboxcenterpoint;

	p->followerready = followerready;
	p->followerskin = followerskin;
	p->followercolor = followercolor;
	//p->follower = NULL;	// respawn a new one with you, it looks better.
	// ^ Not necessary anyway since it will be respawned regardless considering it doesn't exist anymore.

	p->playerstate = PST_LIVE;
	p->panim = PA_STILL; // standing animation

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

	if (p->spectator == false)
	{
		if (betweenmaps || enteredGame == true)
		{
			ACS_RunPlayerEnterScript(p);
		}
		else
		{
			ACS_RunPlayerRespawnScript(p);
		}
	}

	if (betweenmaps)
		return;

	if (leveltime < starttime)
		return;

	if (exiting)
		return;

	P_RestoreMusic(p);

	if (songcredit)
		S_ShowMusicCredit();
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

	if (!P_CheckPosition(players[playernum].mo, x, y, NULL))
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
	LUA_HookPlayer(&players[playernum], HOOK(PlayerSpawn)); // Lua hook for player spawning :)
}

void G_MovePlayerToSpawnOrStarpost(INT32 playernum)
{
#if 0
	if (leveltime <= introtime && !players[playernum].spectator)
		P_MovePlayerToSpawn(playernum, G_FindMapStart(playernum));
	else
		P_MovePlayerToStarpost(playernum);
#else
	// Player's first spawn should be at the "map start".
	// I.e. level load or join mid game.
	if (leveltime > starttime && players[playernum].jointime > 1 && K_PodiumSequence() == false)
		P_MovePlayerToStarpost(playernum);
	else
		P_MovePlayerToSpawn(playernum, G_FindMapStart(playernum));
#endif
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

	if ((!players[playernum].ctfteam && numredctfstarts && (!numbluectfstarts || P_RandomChance(PR_PLAYERSTARTS, FRACUNIT/2))) || players[playernum].ctfteam == 1) //red
	{
		if (!numredctfstarts)
		{
			if (doprints)
				CONS_Alert(CONS_WARNING, M_GetText("No Red Team starts in this map!\n"));
			return NULL;
		}

		for (j = 0; j < 32; j++)
		{
			i = P_RandomKey(PR_PLAYERSTARTS, numredctfstarts);
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
			i = P_RandomKey(PR_PLAYERSTARTS, numbluectfstarts);
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
			i = P_RandomKey(PR_PLAYERSTARTS, numdmstarts);
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
		{
			return playerstarts[0]; // go to first spot if you're a spectator
		}

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

					if ((netgame || (demo.playback && demo.netgame)) && cv_kartusepwrlv.value)
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
					if ((netgame || (demo.playback && demo.netgame)) && cv_kartusepwrlv.value)
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

mapthing_t *G_FindPodiumStart(INT32 playernum)
{
	const boolean doprints = P_IsLocalPlayer(&players[playernum]);

	if (numcoopstarts)
	{
		UINT8 pos = K_GetPodiumPosition(&players[playernum]) - 1;
		UINT8 i;

		if (G_CheckSpot(playernum, playerstarts[pos % numcoopstarts]))
		{
			return playerstarts[pos % numcoopstarts];
		}

		// Your spot isn't available? Find whatever you can get first.
		for (i = 0; i < numcoopstarts; i++)
		{
			if (G_CheckSpot(playernum, playerstarts[(pos + i) % numcoopstarts]))
			{
				return playerstarts[(pos + i) % numcoopstarts];
			}
		}

		if (doprints)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Podium starts!\n"));
		}

		return NULL;
	}

	if (doprints)
		CONS_Alert(CONS_WARNING, M_GetText("No Podium starts in this map!\n"));
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

	// -- Podium -- 
	// Single special behavior
	if (K_PodiumSequence() == true)
		spawnpoint = G_FindPodiumStart(playernum);

	// -- Time Attack --
	// Order: Race->DM->CTF
	else if (K_TimeAttackRules() == true)
		spawnpoint = G_FindRaceStartOrFallback(playernum);

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
			P_SetTarget(&player->mo, NULL);
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
	demo_extradata[playernum] |= DXD_JOINDATA|DXD_PLAYSTATE|DXD_COLOR|DXD_NAME|DXD_SKIN|DXD_FOLLOWER; // Set everything
}

void G_ExitLevel(void)
{
	G_ResetAllDeviceRumbles();

	if (gamestate == GS_LEVEL)
	{
		UINT8 i;
		boolean doretry = false;

		if (!G_GametypeUsesLives())
			; // never force a retry
		else if (specialstageinfo.valid == true || (gametyperules & GTR_BOSS))
		{
			doretry = true;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator && !players[i].bot)
				{
					if (!K_IsPlayerLosing(&players[i]))
					{
						doretry = false;
						break;
					}
				}
			}
		}
		else if (grandprixinfo.gp == true && grandprixinfo.eventmode == GPEVENT_NONE)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator)
				{
					K_PlayerFinishGrandPrix(&players[i]);
				}
			}

			doretry = (grandprixinfo.wonround != true);
		}

		if (doretry)
		{
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
				if (grandprixinfo.gp == true
					&& grandprixinfo.eventmode == GPEVENT_SPECIAL)
				{
					// We were in a Special Stage.
					// We can still progress to the podium when we game over here.
					doretry = false;
				}
				else if (netgame)
				{
					; // Restart cup here whenever we do Online GP
				}
				else
				{
					// Back to the menu with you.
					D_QuitNetGame();
					CL_Reset();
					D_ClearState();
					M_StartControlPanel();
				}
			}
			else
			{
				// We have lives, just redo this one course.
				G_SetRetryFlag();
			}

			if (doretry == true)
			{
				return;
			}
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
		HU_ClearTitlecardCEcho();

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

static gametype_t defaultgametypes[] =
{
	// GT_RACE
	{
		"Race",
		"GT_RACE",
		GTR_CIRCUIT|GTR_BOTS|GTR_ENCORE,
		TOL_RACE,
		int_time,
		0,
		0,
	},

	// GT_BATTLE
	{
		"Battle",
		"GT_BATTLE",
		GTR_SPHERES|GTR_BUMPERS|GTR_PAPERITEMS|GTR_POWERSTONES|GTR_KARMA|GTR_ITEMARROWS|GTR_PRISONS|GTR_BATTLESTARTS|GTR_POINTLIMIT|GTR_TIMELIMIT|GTR_OVERTIME|GTR_CLOSERPLAYERS,
		TOL_BATTLE,
		int_scoreortimeattack,
		0,
		2,
	},

	// GT_SPECIAL
	{
		"Special",
		"GT_SPECIAL",
		GTR_CATCHER|GTR_SPECIALSTART|GTR_ROLLINGSTART|GTR_CIRCUIT,
		TOL_SPECIAL,
		int_time,
		0,
		0,
	},

	// GT_VERSUS
	{
		"Versus",
		"GT_VERSUS",
		GTR_BOSS|GTR_SPHERES|GTR_BUMPERS|GTR_POINTLIMIT|GTR_CLOSERPLAYERS|GTR_NOCUPSELECT|GTR_ENCORE,
		TOL_BOSS,
		int_scoreortimeattack,
		0,
		0,
	},
};

gametype_t *gametypes[MAXGAMETYPES+1] =
{
	&defaultgametypes[GT_RACE],
	&defaultgametypes[GT_BATTLE],
	&defaultgametypes[GT_SPECIAL],
	&defaultgametypes[GT_VERSUS],
};

//
// G_GetGametypeByName
//
// Returns the number for the given gametype name string, or -1 if not valid.
//
INT32 G_GetGametypeByName(const char *gametypestr)
{
	INT32 i = 0;

	while (gametypes[i] != NULL)
	{
		if (!stricmp(gametypestr, gametypes[i]->name))
			return i;
		i++;
	}

	return -1; // unknown gametype
}

//
// G_GuessGametypeByTOL
//
// Returns the first valid number for the given typeoflevel, or -1 if not valid.
//
INT32 G_GuessGametypeByTOL(UINT32 tol)
{
	INT32 i = 0;

	while (gametypes[i] != NULL)
	{
		if (tol & gametypes[i]->tol)
			return i;
		i++;
	}

	return -1; // unknown gametype
}

//
// G_SetGametype
//
// Set a new gametype, also setting gametype rules accordingly. Yay!
//
void G_SetGametype(INT16 gtype)
{
	if (gtype < 0 || gtype > numgametypes)
	{
		I_Error("G_SetGametype: Bad gametype change %d (was %d/\"%s\")", gtype, gametype, gametypes[gametype]->name);
	}

	gametype = gtype;
}

//
// G_PrepareGametypeConstant
//
// Self-explanatory. Filters out "bad" characters.
//
char *G_PrepareGametypeConstant(const char *newgtconst)
{
	size_t r = 0; // read
	size_t w = 0; // write
	size_t len = strlen(newgtconst);
	char *gtconst = Z_Calloc(len + 4, PU_STATIC, NULL);
	char *tmpconst = Z_Calloc(len + 1, PU_STATIC, NULL);

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

	// Finally, return the constant string.
	return gtconst;
}

tolinfo_t TYPEOFLEVEL[NUMTOLNAMES] = {
	{"RACE",TOL_RACE},
	{"BATTLE",TOL_BATTLE},
	{"BOSS",TOL_BOSS},
	{"SPECIAL",TOL_SPECIAL},
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
// G_GametypeUsesLives
//
// Returns true if the current gametype uses
// the lives system.  False otherwise.
//
boolean G_GametypeUsesLives(void)
{
	if (modeattacking || metalrecording) // NOT in Record Attack
		return false;

	if ((grandprixinfo.gp == true) // In Grand Prix
		&& grandprixinfo.eventmode != GPEVENT_BONUS) // NOT in bonus round
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
	return (netgame || (multiplayer && demo.netgame));
}

//
// G_SometimesGetDifferentGametype
//
// Because gametypes are no longer on the vote screen, all this does is sometimes flip encore mode.
// However, it remains a seperate function for long-term possibility.
//
INT16 G_SometimesGetDifferentGametype(void)
{
	boolean encorepossible = ((M_SecretUnlocked(SECRET_ENCORE, false) || encorescramble == 1)
		&& (gametyperules & GTR_ENCORE));
	UINT8 encoremodifier = 0;

	// -- the below is only necessary if you want to use randmaps.mapbuffer here
	//if (randmaps.lastnummapheaders != nummapheaders)
		//G_ResetRandMapBuffer();

	// FORCE to what was scrambled on intermission?
	if (encorepossible && encorescramble != -1)
	{
		// FORCE to what was scrambled on intermission
		if ((encorescramble != 0) != (cv_kartencore.value == 1))
		{
			encoremodifier = VOTEMODIFIER_ENCORE;
		}
	}

	return (gametype|encoremodifier);
}

/** Get the typeoflevel flag needed to indicate support of a gametype.
  * \param gametype The gametype for which support is desired.
  * \return The typeoflevel flag to check for that gametype.
  * \author Graue <graue@oceanbase.org>
  */
UINT32 G_TOLFlag(INT32 pgametype)
{
	if (pgametype >= 0 && pgametype < numgametypes)
		return gametypes[pgametype]->tol;
	return 0;
}

INT16 G_GetFirstMapOfGametype(UINT8 pgametype)
{
	UINT8 i = 0;
	INT16 mapnum = NEXTMAP_INVALID;
	levelsearch_t templevelsearch;

	templevelsearch.cup = NULL;
	templevelsearch.typeoflevel = G_TOLFlag(pgametype);
	templevelsearch.cupmode = (!(gametypes[pgametype]->rules & GTR_NOCUPSELECT));
	templevelsearch.timeattack = false;
	templevelsearch.checklocked = true;

	if (templevelsearch.cupmode)
	{
		templevelsearch.cup = kartcupheaders;
		while (templevelsearch.cup && mapnum >= nummapheaders)
		{
			mapnum = M_GetFirstLevelInList(&i, &templevelsearch);
			i = 0;
			templevelsearch.cup = templevelsearch.cup->next;
		}
	}
	else
	{
		mapnum = M_GetFirstLevelInList(&i, &templevelsearch);
	}

	return mapnum;
}

static INT32 TOLMaps(UINT8 pgametype)
{
	INT32 num = 0;
	INT32 i;

	UINT32 tolflag = G_TOLFlag(pgametype);

	// Find all the maps that are ok
	for (i = 0; i < nummapheaders; i++)
	{
		if (!mapheaderinfo[i])
			continue;
		if (mapheaderinfo[i]->lumpnum == LUMPERROR)
			continue;
		if (!(mapheaderinfo[i]->typeoflevel & tolflag))
			continue;
		if (mapheaderinfo[i]->menuflags & LF2_HIDEINMENU) // Don't include Map Hell
			continue;
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
INT16 G_RandMap(UINT32 tolflags, INT16 pprevmap, UINT8 ignorebuffer, UINT8 maphell, boolean callagainsoon, INT16 *extbuffer)
{
	UINT32 numokmaps = 0;
	INT16 ix, bufx;
	UINT16 extbufsize = 0;
	boolean usehellmaps; // Only consider Hell maps in this pick

	if (randmaps.lastnummapheaders != nummapheaders)
		G_ResetRandMapBuffer();

	if (!okmaps)
	{
		//CONS_Printf("(making okmaps)\n");
		okmaps = Z_Malloc(nummapheaders * sizeof(INT16), PU_STATIC, NULL);
	}

	if (extbuffer != NULL)
	{
		bufx = 0;
		while (extbuffer[bufx])
		{
			extbufsize++; bufx++;
		}
	}

tryagain:

	usehellmaps = (maphell == 0 ? false : (maphell == 2 || M_RandomChance(FRACUNIT/100))); // 1% chance of Hell

	// Find all the maps that are ok and and put them in an array.
	for (ix = 0; ix < nummapheaders; ix++)
	{
		boolean isokmap = true;

		if (!mapheaderinfo[ix] || mapheaderinfo[ix]->lumpnum == LUMPERROR)
			continue;

		if (!(mapheaderinfo[ix]->typeoflevel & tolflags)
			|| ix == pprevmap
			|| M_MapLocked(ix+1)
			|| (usehellmaps != (mapheaderinfo[ix]->menuflags & LF2_HIDEINMENU))) // this is bad
			continue; //isokmap = false;

		if (pprevmap == -2 // title demo hack
			&& mapheaderinfo[ix]->ghostCount == 0)
			continue;

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

			for (bufx = 0; bufx < (maphell ? 3 : randmaps.lastnummapheaders); bufx++)
			{
				if (randmaps.mapbuffer[bufx] == -1) // Rest of buffer SHOULD be empty
					break;
				if (ix == randmaps.mapbuffer[bufx])
				{
					isokmap = false;
					break;
				}
			}

			if (!isokmap)
				continue;
		}

		okmaps[numokmaps++] = ix;
	}

	if (numokmaps == 0)  // If there's no matches... (Goodbye, incredibly silly function chains :V)
	{
		if (!ignorebuffer)
		{
			if (randmaps.mapbuffer[3] == -1) // Is the buffer basically empty?
			{
				ignorebuffer = 1; // This will probably only help in situations where there's very few maps, but it's folly not to at least try it
				//CONS_Printf("RANDMAP - ignoring buffer\n");
				goto tryagain;
			}

			for (bufx = 3; bufx < randmaps.lastnummapheaders; bufx++) // Let's clear all but the three most recent maps...
				randmaps.mapbuffer[bufx] = -1;
			//CONS_Printf("RANDMAP - emptying randmapbuffer\n");
			goto tryagain;
		}

		if (maphell) // Any wiggle room to loosen our restrictions here?
		{
			//CONS_Printf("RANDMAP -maphell decrement\n");
			maphell--;
			goto tryagain;
		}

		//CONS_Printf("RANDMAP - defaulting to map01\n");
		ix = 0; // Sorry, none match. You get MAP01.
		if (ignorebuffer == 1)
		{
			//CONS_Printf("(emptying randmapbuffer entirely)\n");
			for (bufx = 0; bufx < randmaps.lastnummapheaders; bufx++)
				randmaps.mapbuffer[bufx] = -1; // if we're having trouble finding a map we should probably clear it
		}
	}
	else
	{
		//CONS_Printf("RANDMAP - %d maps available to grab\n", numokmaps);
		ix = okmaps[M_RandomKey(numokmaps)];
	}

	if (!callagainsoon)
	{
		//CONS_Printf("(freeing okmaps)\n");
		Z_Free(okmaps);
		okmaps = NULL;
	}

	return ix;
}

void G_AddMapToBuffer(INT16 map)
{
	INT16 bufx;
	INT16 refreshnum = (TOLMaps(gametype))-3;

	if (refreshnum < 0)
		refreshnum = 3;

	if (nummapheaders != randmaps.lastnummapheaders)
	{
		G_ResetRandMapBuffer();
	}
	else
	{
		for (bufx = randmaps.lastnummapheaders-1; bufx > 0; bufx--)
			randmaps.mapbuffer[bufx] = randmaps.mapbuffer[bufx-1];
	}

	randmaps.mapbuffer[0] = map;

	// We're getting pretty full, so lets flush this for future usage.
	if (randmaps.mapbuffer[refreshnum] != -1)
	{
		// Clear all but the five most recent maps.
		for (bufx = 5; bufx < randmaps.lastnummapheaders; bufx++)
			randmaps.mapbuffer[bufx] = -1;
		//CONS_Printf("Random map buffer has been flushed.\n");
	}
}

//
// G_UpdateVisited
//
static void G_UpdateVisited(void)
{
	UINT8 i;
	UINT8 earnedEmblems;

	// No demos.
	if (demo.playback)
		return;

	// Check if every local player wiped out.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i]) // Not here.
			continue;

		if (!P_IsLocalPlayer(&players[i])) // Not local.
			continue;

		if (players[i].spectator) // Not playing.
			continue;

		if (players[i].pflags & PF_NOCONTEST) // Sonic after not surviving.
			continue;
		break;
	}

	if (i == MAXPLAYERS) // Not a single living local soul?
		return;

	// Update visitation flags
	mapheaderinfo[prevmap]->mapvisited |= MV_BEATEN;

	if (encoremode == true)
	{
		mapheaderinfo[prevmap]->mapvisited |= MV_ENCORE;
	}

	if (modeattacking & ATTACKING_SPB)
	{
		mapheaderinfo[prevmap]->mapvisited |= MV_SPBATTACK;
	}

	if (modeattacking)
		G_UpdateRecordReplays();

	if ((earnedEmblems = M_CompletionEmblems()))
		CONS_Printf(M_GetText("\x82" "Earned %hu emblem%s for level completion.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");

	M_UpdateUnlockablesAndExtraEmblems(true, true);
	G_SaveGameData();
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
	if (nextmap >= NEXTMAP_SPECIAL)
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
			else if (!usedCheats && !(netgame || multiplayer || ultimatemode || demo.recording || metalrecording || modeattacking))
				G_SaveGame((UINT32)cursaveslot, 0); // TODO when we readd a campaign one day
		}
	}
	// and doing THIS here means you don't lose your progress if you close the game mid-intermission
	else if (!(ultimatemode || demo.playback || demo.recording || metalrecording || modeattacking)
		&& cursaveslot > 0 && CanSaveLevel(lastmap+1))
	{
		G_SaveGame((UINT32)cursaveslot, lastmap+1); // not nextmap+1 to route around special stages
	}
}

static void G_GetNextMap(void)
{
	INT32 i;

	// go to next level
	// nextmap is 0-based, unlike gamemap
	if (nextmapoverride != 0)
	{
		nextmap = (INT16)(nextmapoverride-1);
	}
	else if (grandprixinfo.gp == true)
	{
		if (grandprixinfo.roundnum == 0 || grandprixinfo.cup == NULL) // Single session
		{
			nextmap = prevmap; // Same map
		}
		else
		{
			INT32 lastgametype = gametype, newgametype = GT_RACE;
			// 5 levels, 2 bonus stages: after rounds 2 and 4 (but flexible enough to accomodate other solutions)
			UINT8 bonusmodulo = (grandprixinfo.cup->numlevels+1)/(grandprixinfo.cup->numbonus+1);
			UINT8 bonusindex = (grandprixinfo.roundnum / bonusmodulo) - 1;

			// If we're in a GP event, don't immediately follow it up with another.
			// I also suspect this will not work with online GP so I'm gonna prevent it right now.
			// The server might have to communicate eventmode (alongside other GP data) in XD_MAP later.
			if (netgame)
				;
			else if	(grandprixinfo.eventmode != GPEVENT_NONE)
			{
				grandprixinfo.eventmode = GPEVENT_NONE;

				G_SetGametype(GT_RACE);
				if (gametype != lastgametype)
					D_GameTypeChanged(lastgametype);
			}
			// Special stage
			else if (grandprixinfo.roundnum >= grandprixinfo.cup->numlevels)
			{
				gp_rank_e grade = K_CalculateGPGrade(&grandprixinfo.rank);

				if (grade >= GRADE_A && grandprixinfo.gamespeed >= KARTSPEED_NORMAL) // On A rank pace? Then you get a chance for S rank!
				{
					const INT32 cupLevelNum = grandprixinfo.cup->cachedlevels[CUPCACHE_SPECIAL];
					if (cupLevelNum < nummapheaders && mapheaderinfo[cupLevelNum])
					{
						grandprixinfo.eventmode = GPEVENT_SPECIAL;
						nextmap = cupLevelNum;
						newgametype = G_GuessGametypeByTOL(mapheaderinfo[cupLevelNum]->typeoflevel);

						if (gamedata->everseenspecial == false)
						{
							gamedata->everseenspecial = true;
							M_UpdateUnlockablesAndExtraEmblems(true, true);
							G_SaveGameData();
						}
					}
				}
			}
			else if ((grandprixinfo.cup->numbonus > 0)
				&& (grandprixinfo.roundnum % bonusmodulo) == 0
				&& bonusindex < MAXBONUSLIST)
			{
				// todo any other condition?
				{
					const INT32 cupLevelNum = grandprixinfo.cup->cachedlevels[CUPCACHE_BONUS + bonusindex];
					if (cupLevelNum < nummapheaders && mapheaderinfo[cupLevelNum])
					{
						grandprixinfo.eventmode = GPEVENT_BONUS;
						nextmap = cupLevelNum;
						newgametype = G_GuessGametypeByTOL(mapheaderinfo[cupLevelNum]->typeoflevel);
					}
				}
			}

			if (newgametype == -1)
			{
				// Don't permit invalid changes.
				grandprixinfo.eventmode = GPEVENT_NONE;
				newgametype = gametype;
			}

			if (grandprixinfo.eventmode != GPEVENT_NONE)
			{
				G_SetGametype(newgametype);
				if (gametype != lastgametype)
					D_GameTypeChanged(lastgametype);
			}
			else if (grandprixinfo.roundnum >= grandprixinfo.cup->numlevels) // On final map
			{
				nextmap = NEXTMAP_CEREMONY; // ceremonymap
			}
			else
			{
				// Proceed to next map
				const INT32 cupLevelNum = grandprixinfo.cup->cachedlevels[grandprixinfo.roundnum];

				if (cupLevelNum < nummapheaders && mapheaderinfo[cupLevelNum])
				{
					nextmap = cupLevelNum;
				}
				else
				{
					nextmap = 0; // Prevent uninitialised use -- go to TEST RUN, it's very obvious
				}

				grandprixinfo.roundnum++;
			}
		}
	}
	else
	{
		UINT32 tolflag = G_TOLFlag(gametype);
		register INT16 cm;

		if (!(gametyperules & GTR_NOCUPSELECT))
		{
			cupheader_t *cup = mapheaderinfo[gamemap-1]->cup;
			UINT8 gettingresult = 0;

			while (cup)
			{
				// Not unlocked? Grab the next result afterwards
				if (!marathonmode && M_CupLocked(cup))
				{
					cup = cup->next;
					gettingresult = 1;
					continue;
				}

				for (i = 0; i < cup->numlevels; i++)
				{
					cm = cup->cachedlevels[i];

					// Not valid?
					if (cm >= nummapheaders
						|| !mapheaderinfo[cm]
						|| mapheaderinfo[cm]->lumpnum == LUMPERROR
						|| !(mapheaderinfo[cm]->typeoflevel & tolflag)
						|| (!marathonmode && M_MapLocked(cm+1)))
						continue;

					// If the map is in multiple cups, only consider the first one valid.
					if (mapheaderinfo[cm]->cup != cup)
					{
						continue;
					}

					// Grab the first valid after the map you're on
					if (gettingresult)
					{
						nextmap = cm;
						gettingresult = 2;
						break;
					}

					// Not the map you're on?
					if (cm != prevmap)
					{
						continue;
					}

					// Ok, this is the current map, time to get the next
					gettingresult = 1;
				}

				// We have a good nextmap?
				if (gettingresult == 2)
				{
					break;
				}

				// Ok, iterate to the next
				cup = cup->next;
			}

			// Didn't get a nextmap before reaching the end?
			if (gettingresult != 2)
			{
				nextmap = NEXTMAP_CEREMONY; // ceremonymap
			}
		}
		else
		{
			cm = prevmap;
			if (++cm >= nummapheaders)
				cm = 0;

			while (cm != prevmap)
			{
				if (!mapheaderinfo[cm]
					|| mapheaderinfo[cm]->lumpnum == LUMPERROR
					|| !(mapheaderinfo[cm]->typeoflevel & tolflag)
					|| (mapheaderinfo[cm]->menuflags & LF2_HIDEINMENU)
					|| M_MapLocked(cm+1))
				{
					if (++cm >= nummapheaders)
						cm = 0;
					continue;
				}

				break;
			}

			nextmap = cm;
		}

		if (K_CanChangeRules(true))
		{
			if (!netgame) // Match Race.
				nextmap = NEXTMAP_TITLE;
			else switch (cv_advancemap.value)
			{
				case 0: // Stay on same map.
					nextmap = prevmap;
					break;
				case 3: // Voting screen.
					{
						for (i = 0; i < MAXPLAYERS; i++)
						{
							if (!playeringame[i])
								continue;
							if (players[i].spectator)
								continue;
							break;
						}
						if (i != MAXPLAYERS)
						{
							nextmap = NEXTMAP_VOTING;
							break;
						}
					}
					/* FALLTHRU */
				case 2: // Go to random map.
					nextmap = G_RandMap(G_TOLFlag(gametype), prevmap, 0, 0, false, NULL);
					break;
				default:
					if (nextmap >= NEXTMAP_SPECIAL) // Loop back around
					{
						nextmap = G_GetFirstMapOfGametype(gametype);
					}
					break;
			}
		}
	}

	// We are committed to this map now.
	if (nextmap == NEXTMAP_INVALID || (nextmap < NEXTMAP_SPECIAL && (nextmap >= nummapheaders || !mapheaderinfo[nextmap] || mapheaderinfo[nextmap]->lumpnum == LUMPERROR)))
		I_Error("G_GetNextMap: Internal map ID %d not found (nummapheaders = %d)\n", nextmap, nummapheaders);

#if 0 // This is a surprise tool that will help us later.
	if (!spec)
#endif //#if 0
		lastmap = nextmap;

	deferencoremode = (cv_kartencore.value == 1);
}

//
// G_DoCompleted
//
static void G_DoCompleted(void)
{
	INT32 i, j = 0;

	if (modeattacking && pausedelay)
		pausedelay = 0;

	// We do this here so Challenges-related sounds aren't insta-killed
	S_StopSounds();

	if (legitimateexit && !demo.playback && !mapreset) // (yes you're allowed to unlock stuff this way when the game is modified)
	{
		UINT8 roundtype = GDGT_CUSTOM;

		if (gametype == GT_RACE)
			roundtype = GDGT_RACE;
		else if (gametype == GT_BATTLE)
			roundtype = (battleprisons ? GDGT_PRISONS : GDGT_BATTLE);
		else if (gametype == GT_SPECIAL || gametype == GT_VERSUS)
			roundtype = GDGT_SPECIAL;

		gamedata->roundsplayed[roundtype]++;
		gamedata->pendingkeyrounds++;

		// Done before forced addition of PF_NOCONTEST to make UCRP_NOCONTEST harder to achieve
		M_UpdateUnlockablesAndExtraEmblems(true, true);
		gamedata->deferredsave = true;
	}

	if (gamedata->deferredsave)
		G_SaveGameData();

	legitimateexit = false;

	gameaction = ga_nothing;

	if (metalplayback)
		G_StopMetalDemo();
	if (metalrecording)
		G_StopMetalRecording(false);

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;

	grandprixinfo.rank.prisons += numtargets;
	grandprixinfo.rank.position = MAXPLAYERS;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			// SRB2Kart: exitlevel shouldn't get you the points
			if (!players[i].exiting && !(players[i].pflags & PF_NOCONTEST))
			{
				clientPowerAdd[i] = 0;

				if (players[i].bot)
				{
					K_FakeBotResults(&players[i]);
				}
				else
				{
					players[i].pflags |= PF_NOCONTEST;

					if (P_IsLocalPlayer(&players[i]))
					{
						j++;
					}
				}
			}

			G_PlayerFinishLevel(i); // take away cards and stuff

			if (players[i].bot == false)
			{
				grandprixinfo.rank.position = min(grandprixinfo.rank.position, K_GetPodiumPosition(&players[i]));
			}
		}
	}

	// See Y_StartIntermission timer handling
	if ((gametyperules & GTR_CIRCUIT) && ((multiplayer && demo.playback) || j == r_splitscreen+1) && (!K_CanChangeRules(false) || cv_inttime.value > 0))
	// play some generic music if there's no win/cool/lose music going on (for exitlevel commands)
		S_ChangeMusicInternal("racent", true);

	if (automapactive)
		AM_Stop();

	prevmap = (INT16)(gamemap-1);

	if (!demo.playback)
	{
		// Set up power level gametype scrambles
		K_SetPowerLevelScrambles(K_UsingPowerLevels());
	}

	// If the current gametype has no intermission screen set, then don't start it.
	Y_DetermineIntermissionType();

	if ((skipstats && !modeattacking)
		|| (modeattacking && (players[consoleplayer].pflags & PF_NOCONTEST))
		|| (intertype == int_none))
	{
		G_UpdateVisited();
		G_AfterIntermission();
	}
	else
	{
		G_SetGamestate(GS_INTERMISSION);
		Y_StartIntermission();
		G_UpdateVisited();
	}
}

// See also F_EndCutscene, the only other place which handles intra-map/ending transitions
void G_AfterIntermission(void)
{
	if (gamecomplete == 2) // special temporary mode to prevent using SP level select in pause menu until the intermission is over without restricting it in every intermission
		gamecomplete = 1;

	HU_ClearCEcho();
	HU_ClearTitlecardCEcho();

	if (demo.playback)
	{
		M_PlaybackQuit(0);
		return;
	}
	else if (demo.recording && (modeattacking || demo.savemode != DSM_NOTSAVING))
		G_SaveDemo();

	if (modeattacking) // End the run.
	{
		M_EndModeAttackRun();
		return;
	}

	if (gamestate != GS_VOTING)
	{
		G_GetNextMap();
		G_HandleSaveLevel();
	}

	if ((grandprixinfo.gp == true) && mapheaderinfo[prevmap]->cutscenenum && !modeattacking && skipstats <= 1 && (gamecomplete || !(marathonmode & MA_NOCUTSCENES))) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[prevmap]->cutscenenum-1, false, false);
	else
	{
		G_NextLevel();
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
	if (nextmap >= NEXTMAP_SPECIAL)
	{
		G_EndGame();
		return;
	}

	forceresetplayers = false;

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
	{
		if (gamestate == GS_VOTING)
			I_Error("G_DoStartVote: NEXTMAP_VOTING causes recursive vote!");
		D_SetupVote();
	}
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
	//I_Assert(pl->continues > 0);

	/*if (pl->continues)
		pl->continues--;*/

	// Reset score
	pl->score = 0;

	if (!(netgame || multiplayer || demo.playback || demo.recording || metalrecording || modeattacking) && !usedCheats && cursaveslot > 0)
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
	// Handle voting
	if (nextmap == NEXTMAP_VOTING)
	{
		gameaction = ga_startvote;
		return;
	}

	// Only do evaluation and credits in singleplayer contexts
	if (!netgame && grandprixinfo.gp == true)
	{
		if (nextmap == NEXTMAP_CEREMONY) // end game with ceremony
		{
			if (K_StartCeremony() == true)
			{
				return;
			}
		}
		if (nextmap == NEXTMAP_CREDITS) // end game with credits
		{
			F_StartCredits();
			return;
		}
		if (nextmap == NEXTMAP_EVALUATION) // end game with evaluation
		{
			F_StartGameEvaluation();
			return;
		}
	}

	// In a netgame, don't unwittingly boot everyone.
	if (netgame)
	{
		S_StopMusic();
		G_SetGamestate(GS_WAITINGPLAYERS); // hack to prevent a command repeat

		if (server)
		{
			UINT16 map = G_GetFirstMapOfGametype(gametype)+1;

			if (map > nummapheaders)
				I_Error("G_EndGame: No valid map ID found!?");

			COM_BufAddText(va("map %s\n", G_BuildMapName(map)));
		}

		return;
	}

	// Time to return to the menu.
	D_ClearState();
	M_StartControlPanel();
}

//
// G_LoadGameSettings
//
// Sets a tad of default info we need.
void G_LoadGameSettings(void)
{
	INT32 i;

	// initialize free sfx slots for skin sounds
	S_InitRuntimeSounds();

	// Prepare skincolor material.
	for (i = 0; i < MAXSKINCOLORS; i++)
	{
		Color_cons_t[i].value = Followercolor_cons_t[i+2].value = i;
		Color_cons_t[i].strvalue = Followercolor_cons_t[i+2].strvalue = skincolors[i].name;
	}

	Followercolor_cons_t[1].value = FOLLOWERCOLOR_MATCH;
	Followercolor_cons_t[1].strvalue = "Match"; // Add "Match" option, which will make the follower color match the player's

	Followercolor_cons_t[0].value = FOLLOWERCOLOR_OPPOSITE;
	Followercolor_cons_t[0].strvalue = "Opposite"; // Add "Opposite" option, ...which is like "Match", but for coloropposite.

	Color_cons_t[MAXSKINCOLORS].value = Followercolor_cons_t[MAXSKINCOLORS+2].value = 0;
	Color_cons_t[MAXSKINCOLORS].strvalue = Followercolor_cons_t[MAXSKINCOLORS+2].strvalue = NULL;
}

#define GD_VERSIONCHECK 0xBA5ED123 // Change every major version, as usual
#define GD_VERSIONMINOR 2 // Change every format update

static const char *G_GameDataFolder(void)
{
	if (strcmp(srb2home,"."))
		return srb2home;
	else
		return "the Ring Racers folder";
}

// G_LoadGameData
// Loads the main data file, which stores information such as emblems found, etc.
void G_LoadGameData(void)
{
	UINT32 i, j;
	UINT32 versionID;
	UINT8 versionMinor;
	UINT8 rtemp;
	boolean gridunusable = false;
	savebuffer_t save = {0};

	//For records
	UINT32 numgamedatamapheaders;
	UINT32 numgamedatacups;

	// Stop saving, until we successfully load it again.
	gamedata->loaded = false;

	// Clear things so previously read gamedata doesn't transfer
	// to new gamedata
	// see also M_EraseDataResponse
	G_ClearRecords(); // records
	M_ClearStats(); // statistics
	M_ClearSecrets(); // emblems, unlocks, maps visited, etc

	if (M_CheckParm("-nodata"))
	{
		// Don't load at all.
		// The following used to be in M_ClearSecrets, but that was silly.
		M_UpdateUnlockablesAndExtraEmblems(false, true);
		return;
	}

	if (M_CheckParm("-resetdata"))
	{
		// Don't load, but do save. (essentially, reset)
		goto finalisegamedata;
	}

	if (P_SaveBufferFromFile(&save, va(pandf, srb2home, gamedatafilename)) == false)
	{
		// No gamedata. We can save a new one.
		goto finalisegamedata;
	}

	// Version check
	versionID = READUINT32(save.p);
	if (versionID != GD_VERSIONCHECK)
	{
		const char *gdfolder = G_GameDataFolder();

		P_SaveBufferFree(&save);
		I_Error("Game data is not for Ring Racers v2.0.\nDelete %s (maybe in %s) and try again.", gamedatafilename, gdfolder);
	}

	versionMinor = READUINT8(save.p);
	if (versionMinor > GD_VERSIONMINOR)
	{
		const char *gdfolder = G_GameDataFolder();

		P_SaveBufferFree(&save);
		I_Error("Game data is from the future! (expected %d, got %d)\nRename or delete %s (maybe in %s) and try again.", GD_VERSIONMINOR, versionMinor, gamedatafilename, gdfolder);
	}
	if ((versionMinor == 0 || versionMinor == 1)
#ifdef DEVELOP
		|| M_CheckParm("-resetchallengegrid")
#endif
		)
	{
		gridunusable = true;
	}

	if (versionMinor > 1)
	{
		gamedata->evercrashed = (boolean)READUINT8(save.p);
	}

	gamedata->totalplaytime = READUINT32(save.p);

	if (versionMinor > 1)
	{
		gamedata->totalrings = READUINT32(save.p);

		for (i = 0; i < GDGT_MAX; i++)
		{
			gamedata->roundsplayed[i] = READUINT32(save.p);
		}

		gamedata->pendingkeyrounds = READUINT32(save.p);
		gamedata->pendingkeyroundoffset = READUINT8(save.p);
		gamedata->keyspending = READUINT8(save.p);
		gamedata->chaokeys = READUINT16(save.p);

		gamedata->everloadedaddon = (boolean)READUINT8(save.p);
		gamedata->eversavedreplay = (boolean)READUINT8(save.p);
		gamedata->everseenspecial = (boolean)READUINT8(save.p);
	}
	else
	{
		save.p += 4; // no direct equivalent to matchesplayed
	}

	{
		// Quick & dirty hash for what mod this save file is for.
		UINT32 modID = READUINT32(save.p);
		UINT32 expectedID = quickncasehash(timeattackfolder, 64);

		if (modID != expectedID)
		{
			// Aha! Someone's been screwing with the save file!
			goto datacorrupt;
		}
	}

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		rtemp = READUINT8(save.p);
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			gamedata->collected[j+i] = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		rtemp = READUINT8(save.p);
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			gamedata->unlocked[j+i] = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		rtemp = READUINT8(save.p);
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			gamedata->unlockpending[j+i] = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		rtemp = READUINT8(save.p);
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			gamedata->achieved[j+i] = ((rtemp >> j) & 1);
		i += j;
	}

	if (gridunusable)
	{
		UINT16 burn = READUINT16(save.p); // Previous challengegridwidth
		UINT8 height = (versionMinor > 0) ? CHALLENGEGRIDHEIGHT : 5;
		save.p += (burn * height * sizeof(UINT8)); // Step over previous grid data

		gamedata->challengegridwidth = 0;
		Z_Free(gamedata->challengegrid);
		gamedata->challengegrid = NULL;
	}
	else
	{
		gamedata->challengegridwidth = READUINT16(save.p);
		Z_Free(gamedata->challengegrid);
		if (gamedata->challengegridwidth)
		{
			gamedata->challengegrid = Z_Malloc(
				(gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT * sizeof(UINT8)),
				PU_STATIC, NULL);
			for (i = 0; i < (gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT); i++)
			{
				gamedata->challengegrid[i] = READUINT8(save.p);
			}
		}
		else
		{
			gamedata->challengegrid = NULL;
		}
	}

	gamedata->timesBeaten = READUINT32(save.p);

	// Main records
	numgamedatamapheaders = READUINT32(save.p);
	if (numgamedatamapheaders >= NEXTMAP_SPECIAL)
		goto datacorrupt;

	for (i = 0; i < numgamedatamapheaders; i++)
	{
		char mapname[MAXMAPLUMPNAME];
		INT16 mapnum;
		tic_t rectime;
		tic_t reclap;

		READSTRINGN(save.p, mapname, sizeof(mapname));
		mapnum = G_MapNumber(mapname);

		rtemp = READUINT8(save.p);
		rectime = (tic_t)READUINT32(save.p);
		reclap  = (tic_t)READUINT32(save.p);

		if (mapnum < nummapheaders && mapheaderinfo[mapnum])
		{
			// Valid mapheader, time to populate with record data.

			if ((mapheaderinfo[mapnum]->mapvisited = rtemp) & ~MV_MAX)
				goto datacorrupt;

			if (rectime || reclap)
			{
				G_AllocMainRecordData((INT16)i);
				mapheaderinfo[i]->mainrecord->time = rectime;
				mapheaderinfo[i]->mainrecord->lap = reclap;
				//CONS_Printf("ID %d, Time = %d, Lap = %d\n", i, rectime/35, reclap/35);
			}
		}
		else
		{
			// Since it's not worth declaring the entire gamedata
			// corrupt over extra maps, we report and move on.
			CONS_Alert(CONS_WARNING, "Map with lumpname %s does not exist, time record data will be discarded\n", mapname);
		}
	}

	if (versionMinor > 1)
	{
		numgamedatacups = READUINT32(save.p);

		for (i = 0; i < numgamedatacups; i++)
		{
			char cupname[16];
			cupheader_t *cup;

			// Find the relevant cup.
			READSTRINGN(save.p, cupname, sizeof(cupname));
			for (cup = kartcupheaders; cup; cup = cup->next)
			{
				if (strcmp(cup->name, cupname))
					continue;
				break;
			}

			// Digest its data...
			for (j = 0; j < KARTGP_MAX; j++)
			{
				rtemp = READUINT8(save.p);

				// ...but only record it if we actually found the associated cup.
				if (cup)
				{
					cup->windata[j].best_placement = (rtemp & 0x0F);
					cup->windata[j].best_grade = (rtemp & 0x70)>>4;
					if (rtemp & 0x80)
					{
						if (j == 0)
							goto datacorrupt;

						cup->windata[j].got_emerald = true;
					}
				}
			}
		}
	}

	// done
	P_SaveBufferFree(&save);

	finalisegamedata:
	{
		// Don't consider loaded until it's a success!
		// It used to do this much earlier, but this would cause the gamedata to
		// save over itself when it I_Errors from the corruption landing point below,
		// which can accidentally delete players' legitimate data if the code ever has any tiny mistakes!
		gamedata->loaded = true;

		// Silent update unlockables in case they're out of sync with conditions
		M_UpdateUnlockablesAndExtraEmblems(false, true);

		return;
	}

	// Landing point for corrupt gamedata
	datacorrupt:
	{
		const char *gdfolder = "the Ring Racers folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		P_SaveBufferFree(&save);

		I_Error("Corrupt game data file.\nDelete %s(maybe in %s) and try again.", gamedatafilename, gdfolder);
	}
}

// G_DirtyGameData
// Modifies the gamedata as little as possible to maintain safety in a crash event, while still recording it.
void G_DirtyGameData(void)
{
	FILE *handle = NULL;
	const UINT8 writebytesource = true;

	if (gamedata)
		gamedata->evercrashed = true;

	//if (FIL_WriteFileOK(name))
		handle = fopen(va(pandf, srb2home, gamedatafilename), "r+");

	if (!handle)
		return;

	// Write a dirty byte immediately after the gamedata check + minor version.
	if (fseek(handle, 5, SEEK_SET) != -1)
		fwrite(&writebytesource, 1, 1, handle);

	fclose(handle);

	return;
}

// G_SaveGameData
// Saves the main data file, which stores information such as emblems found, etc.
void G_SaveGameData(void)
{
	size_t length;
	INT32 i, j, numcups;
	cupheader_t *cup;
	UINT8 btemp;
	savebuffer_t save = {0};

	if (gamedata == NULL || !gamedata->loaded)
		return; // If never loaded (-nodata), don't save

	gamedata->deferredsave = false;

	if (usedCheats)
	{
#ifdef DEVELOP
		CONS_Alert(CONS_WARNING, M_GetText("Cheats used - Gamedata will not be saved.\n"));
#endif
		return;
	}

	length = (4+1+1+
		4+4+
		(4*GDGT_MAX)+
		4+1+1+2+
		1+1+1+
		4+
		(MAXEMBLEMS+(MAXUNLOCKABLES*2)+MAXCONDITIONSETS)+
		4+2);

	if (gamedata->challengegrid)
	{
		length += gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT;
	}
	length += 4 + (nummapheaders * (MAXMAPLUMPNAME+1+4+4));

	numcups = 0;
	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		numcups++;
	}
	length += 4 + (numcups * (4+16));

	if (P_SaveBufferAlloc(&save, length) == false)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for saving game data\n"));
		return;
	}

	// Version test

	WRITEUINT32(save.p, GD_VERSIONCHECK); // 4
	WRITEUINT8(save.p, GD_VERSIONMINOR); // 1

	// Crash dirtiness
	// cannot move, see G_DirtyGameData
	WRITEUINT8(save.p, gamedata->evercrashed); // 1

	// Statistics

	WRITEUINT32(save.p, gamedata->totalplaytime); // 4
	WRITEUINT32(save.p, gamedata->totalrings); // 4

	for (i = 0; i < GDGT_MAX; i++) // 4 * GDGT_MAX
	{
		WRITEUINT32(save.p, gamedata->roundsplayed[i]);
	}

	WRITEUINT32(save.p, gamedata->pendingkeyrounds); // 4
	WRITEUINT8(save.p, gamedata->pendingkeyroundoffset); // 1
	WRITEUINT8(save.p, gamedata->keyspending); // 1
	WRITEUINT16(save.p, gamedata->chaokeys); // 2

	WRITEUINT8(save.p, gamedata->everloadedaddon); // 1
	WRITEUINT8(save.p, gamedata->eversavedreplay); // 1
	WRITEUINT8(save.p, gamedata->everseenspecial); // 1

	WRITEUINT32(save.p, quickncasehash(timeattackfolder, 64));

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;) // MAXEMBLEMS * 1;
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			btemp |= (gamedata->collected[j+i] << j);
		WRITEUINT8(save.p, btemp);
		i += j;
	}

	// MAXUNLOCKABLES * 2;
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			btemp |= (gamedata->unlocked[j+i] << j);
		WRITEUINT8(save.p, btemp);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			btemp |= (gamedata->unlockpending[j+i] << j);
		WRITEUINT8(save.p, btemp);
		i += j;
	}

	for (i = 0; i < MAXCONDITIONSETS;) // MAXCONDITIONSETS * 1;
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			btemp |= (gamedata->achieved[j+i] << j);
		WRITEUINT8(save.p, btemp);
		i += j;
	}

	if (gamedata->challengegrid) // 2 + gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT
	{
		WRITEUINT16(save.p, gamedata->challengegridwidth);
		for (i = 0; i < (gamedata->challengegridwidth * CHALLENGEGRIDHEIGHT); i++)
		{
			WRITEUINT8(save.p, gamedata->challengegrid[i]);
		}
	}
	else // 2
	{
		WRITEUINT16(save.p, 0);
	}

	WRITEUINT32(save.p, gamedata->timesBeaten); // 4

	// Main records
	WRITEUINT32(save.p, nummapheaders); // 4

	for (i = 0; i < nummapheaders; i++) // nummapheaders * (255+1+4+4)
	{
		// For figuring out which header to assign it to on load
		WRITESTRINGN(save.p, mapheaderinfo[i]->lumpname, MAXMAPLUMPNAME);

		WRITEUINT8(save.p, (mapheaderinfo[i]->mapvisited & MV_MAX));

		if (mapheaderinfo[i]->mainrecord)
		{
			WRITEUINT32(save.p, mapheaderinfo[i]->mainrecord->time);
			WRITEUINT32(save.p, mapheaderinfo[i]->mainrecord->lap);
		}
		else
		{
			WRITEUINT32(save.p, 0);
			WRITEUINT32(save.p, 0);
		}
	}

	WRITEUINT32(save.p, numcups); // 4

	for (cup = kartcupheaders; cup; cup = cup->next)
	{
		// For figuring out which header to assign it to on load
		WRITESTRINGN(save.p, cup->name, 16);

		for (i = 0; i < KARTGP_MAX; i++)
		{
			btemp = min(cup->windata[i].best_placement, 0x0F);
			btemp |= (cup->windata[i].best_grade<<4);
			if (i != 0 && cup->windata[i].got_emerald == true)
				btemp |= 0x80;

			WRITEUINT8(save.p, btemp); // 4 * numcups
		}
	}

	length = save.p - save.buffer;

	FIL_WriteFile(va(pandf, srb2home, gamedatafilename), save.buffer, length);
	P_SaveBufferFree(&save);

	// Also save profiles here.
	PR_SaveProfiles();
}

#define VERSIONSIZE 16

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
void G_LoadGame(UINT32 slot, INT16 mapoverride)
{
	char vcheck[VERSIONSIZE];
	char savename[255];
	savebuffer_t save = {0};

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

	if (P_SaveBufferFromFile(&save, savename) == false)
	{
		CONS_Printf(M_GetText("Couldn't read file %s\n"), savename);
		return;
	}

	memset(vcheck, 0, sizeof (vcheck));
	sprintf(vcheck, (marathonmode ? "back-up %d" : "version %d"), VERSION);
	if (strcmp((const char *)save.p, (const char *)vcheck))
	{
#ifdef SAVEGAME_OTHERVERSIONS
		M_StartMessage(M_GetText("Save game from different version.\nYou can load this savegame, but\nsaving afterwards will be disabled.\n\nDo you want to continue anyway?\n\n(Press 'Y' to confirm)\n"),
		               M_ForceLoadGameResponse, MM_YESNO);
		//Freeing done by the callback function of the above message
#else
		M_ClearMenus(true); // so ESC backs out to title
		M_StartMessage(M_GetText("Save game from different version\n\nPress ESC\n"), NULL, MM_NOTHING);
		Command_ExitGame_f();
		P_SaveBufferFree(&save);

		// no cheating!
		memset(&savedata, 0, sizeof(savedata));
#endif
		return; // bad version
	}
	save.p += VERSIONSIZE;

	if (demo.playback) // reset game engine
		G_StopDemo();

//	paused = false;
//	automapactive = false;

	// dearchive all the modifications
	if (!P_LoadGame(&save, mapoverride))
	{
		M_ClearMenus(true); // so ESC backs out to title
		M_StartMessage(M_GetText("Savegame file corrupted\n\nPress ESC\n"), NULL, MM_NOTHING);
		Command_ExitGame_f();
		Z_Free(save.buffer);
		save.p = save.buffer = NULL;

		// no cheating!
		memset(&savedata, 0, sizeof(savedata));
		return;
	}
	if (marathonmode)
	{
		marathontime = READUINT32(save.p);
		marathonmode |= READUINT8(save.p);
	}

	// done
	P_SaveBufferFree(&save);

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
	savebuffer_t save = {0};

	if (marathonmode)
		strcpy(savename, liveeventbackup);
	else
		sprintf(savename, savegamename, slot);
	backup = va("%s",savename);

	gameaction = ga_nothing;
	{
		char name[VERSIONSIZE];
		size_t length;

		if (P_SaveBufferAlloc(&save, SAVEGAMESIZE) == false)
		{
			CONS_Alert(CONS_ERROR, M_GetText("No more free memory for saving game data\n"));
			return;
		}

		memset(name, 0, sizeof (name));
		sprintf(name, (marathonmode ? "back-up %d" : "version %d"), VERSION);
		WRITEMEM(save.p, name, VERSIONSIZE);

		P_SaveGame(&save, mapnum);
		if (marathonmode)
		{
			UINT32 writetime = marathontime;
			if (!(marathonmode & MA_INGAME))
				writetime += TICRATE*5; // live event backup penalty because we don't know how long it takes to get to the next map
			WRITEUINT32(save.p, writetime);
			WRITEUINT8(save.p, (marathonmode & ~MA_INIT));
		}

		length = save.p - save.buffer;
		saved = FIL_WriteFile(backup, save.buffer, length);
		P_SaveBufferFree(&save);
	}

	gameaction = ga_nothing;

	if (cht_debug && saved)
		CONS_Printf(M_GetText("Game saved.\n"));
	else if (!saved)
		CONS_Alert(CONS_ERROR, M_GetText("Error while writing to %s for save slot %u, base: %s\n"), backup, slot, (marathonmode ? liveeventbackup : savegamename));
}

#define BADSAVE goto cleanup;
#define CHECKPOS if (save.p >= save.end) BADSAVE
void G_SaveGameOver(UINT32 slot, boolean modifylives)
{
	boolean saved = false;
	size_t length;
	char vcheck[VERSIONSIZE];
	char savename[255];
	const char *backup;
	savebuffer_t save = {0};

	if (marathonmode)
		strcpy(savename, liveeventbackup);
	else
		sprintf(savename, savegamename, slot);
	backup = va("%s",savename);

	if (P_SaveBufferFromFile(&save, savename) == false)
	{
		CONS_Printf(M_GetText("Couldn't read file %s\n"), savename);
		return;
	}

	length = save.size;

	{
		char temp[sizeof(timeattackfolder)];
		UINT8 *lives_p;
		SINT8 pllives;

		// Version check
		memset(vcheck, 0, sizeof (vcheck));
		sprintf(vcheck, (marathonmode ? "back-up %d" : "version %d"), VERSION);
		if (strcmp((const char *)save.p, (const char *)vcheck)) BADSAVE
		save.p += VERSIONSIZE;

		// P_UnArchiveMisc()
		(void)READINT16(save.p);
		CHECKPOS
		(void)READUINT16(save.p); // emeralds
		CHECKPOS
		READSTRINGN(save.p, temp, sizeof(temp)); // mod it belongs to
		if (strcmp(temp, timeattackfolder)) BADSAVE

		// P_UnArchivePlayer()
		CHECKPOS
		(void)READUINT16(save.p);
		CHECKPOS

		WRITEUINT8(save.p, numgameovers);
		CHECKPOS

		lives_p = save.p;
		pllives = READSINT8(save.p); // lives
		CHECKPOS
		if (modifylives && pllives < startinglivesbalance[numgameovers])
		{
			pllives = startinglivesbalance[numgameovers];
			WRITESINT8(lives_p, pllives);
		}

		(void)READINT32(save.p); // Score
		CHECKPOS
		(void)READINT32(save.p); // continues

		// File end marker check
		CHECKPOS
		switch (READUINT8(save.p))
		{
			case 0xb7:
				{
					UINT8 i, banksinuse;
					CHECKPOS
					banksinuse = READUINT8(save.p);
					CHECKPOS
					if (banksinuse > NUM_LUABANKS)
						BADSAVE
					for (i = 0; i < banksinuse; i++)
					{
						(void)READINT32(save.p);
						CHECKPOS
					}
					if (READUINT8(save.p) != 0x1d)
						BADSAVE
				}
			case 0x1d:
				break;
			default:
				BADSAVE
		}

		// done
		saved = FIL_WriteFile(backup, save.buffer, length);
	}

cleanup:
	if (cht_debug && saved)
		CONS_Printf(M_GetText("Game saved.\n"));
	else if (!saved)
		CONS_Alert(CONS_ERROR, M_GetText("Error while writing to %s for save slot %u, base: %s\n"), backup, slot, (marathonmode ? liveeventbackup : savegamename));

	P_SaveBufferFree(&save);
}
#undef CHECKPOS
#undef BADSAVE

//
// G_DeferedInitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayers[], playeringame[] should be set.
//
void G_DeferedInitNew(boolean pencoremode, INT32 map, INT32 pickedchar, UINT8 ssplayers, boolean FLS)
{
	UINT16 color = SKINCOLOR_NONE;

	paused = false;

	if (demo.playback)
		COM_BufAddText("stopdemo\n");

	G_FreeGhosts(); // TODO: do we actually need to do this?

	G_ResetRandMapBuffer();

	// this leave the actual game if needed
	SV_StartSinglePlayerServer(gametype, false);

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

	D_MapChange(map, gametype, pencoremode, true, 1, false, FLS);
}

//
// This is the map command interpretation something like Command_Map_f
//
// called at: map cmd execution, doloadgame, doplaydemo
void G_InitNew(UINT8 pencoremode, INT32 map, boolean resetplayer, boolean skipprecutscene, boolean FLS)
{
	const char * mapname = G_BuildMapName(map);

	INT32 i;

	(void)FLS;

	if (paused)
	{
		paused = false;
		S_ResumeAudio();
	}

	prevencoremode = ((!Playing()) ? false : encoremode);
	encoremode = pencoremode;

	legitimateexit = false; // SRB2Kart
	comebackshowninfo = false;

	if (!demo.playback && !netgame) // Netgame sets random seed elsewhere, demo playback sets seed just before us!
		P_ClearRandom(M_RandomizedSeed()); // Use a more "Random" random seed

	// Clear a bunch of variables
	redscore = bluescore = lastmap = 0;
	racecountdown = exitcountdown = mapreset = exitfadestarted = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		players[i].playerstate = PST_REBORN;
		memset(&players[i].respawn, 0, sizeof (players[i].respawn));

		players[i].roundscore = 0;

		if (resetplayer && !(multiplayer && demo.playback)) // SRB2Kart
		{
			players[i].lives = 3;
			players[i].xtralife = 0;
			players[i].totalring = 0;
			players[i].score = 0;
		}
	}

	// clear itemfinder, just in case
	if (!dedicated)	// except in dedicated servers, where it is not registered and can actually I_Error debug builds
		CV_StealthSetValue(&cv_itemfinder, 0);

	// internal game map
	// well this check is useless because it is done before (d_netcmd.c::command_map_f)
	// but in case of for demos....
	if (!mapname)
	{
		I_Error("Internal game map with ID %d not found\n", map);
		Command_ExitGame_f();
		return;
	}
	if (mapheaderinfo[map-1]->lumpnum == LUMPERROR)
	{
		I_Error("Internal game map '%s' not found\n", mapname);
		Command_ExitGame_f();
		return;
	}

	if (map == G_MapNumber(podiummap)+1)
	{
		// Didn't want to do this, but it needs to be here
		// for it to work on startup.
		if (K_StartCeremony() == true)
		{
			return;
		}
	}

	gamemap = map;

	maptol = mapheaderinfo[gamemap-1]->typeoflevel;
	globalweather = mapheaderinfo[gamemap-1]->weather;

	// Don't carry over custom music change to another map.
	mapmusflags |= MUSIC_RELOADRESET;

	automapactive = false;
	imcontinuing = false;

	if ((grandprixinfo.gp == true) && !skipprecutscene && mapheaderinfo[gamemap-1]->precutscenenum && !modeattacking && !(marathonmode & MA_NOCUTSCENES)) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1]->precutscenenum-1, true, resetplayer);
	else
	{
		LUA_HookInt(gamemap, HOOK(MapChange));
		G_DoLoadLevel(resetplayer);
	}

	if (netgame)
	{
		char *title = G_BuildMapTitle(gamemap);

		CON_LogMessage(va(M_GetText("Map is now \"%s"), mapname));
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

	if (!mapnum || mapnum > nummapheaders || !mapheaderinfo[mapnum-1])
		I_Error("G_BuildMapTitle: Internal map ID %d not found (nummapheaders = %d)", mapnum-1, nummapheaders);

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

	freq = ZZ_Calloc(nummapheaders * sizeof (mapsearchfreq_t));

	wanttable = !!( freqp );

	freqc = 0;
	for (i = 0, mapnum = 1; i < nummapheaders; ++i, ++mapnum)
	{
		if (!mapheaderinfo[i] || mapheaderinfo[i]->lumpnum == LUMPERROR)
			continue;

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
	INT32 newmapnum;

	char *p;

	/* Now detect map number in base 10, which no one asked for. */
	newmapnum = strtol(mapname, &p, 10);

	if (*p == '\0')/* we got it */
	{
		if (newmapnum < 1 || newmapnum > nummapheaders)
			return 0;
		if (!mapheaderinfo[newmapnum-1] || mapheaderinfo[newmapnum-1]->lumpnum == LUMPERROR)
			return 0;
	}
	else
	{
		newmapnum = G_MapNumber(mapname)+1;

		if (newmapnum > nummapheaders)
			return G_FindMap(mapname, realmapnamep, NULL, NULL);
	}

	if (realmapnamep)
		(*realmapnamep) = G_BuildMapTitle(newmapnum);

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
#ifdef HAVE_DISCORDRPC
	DRPC_UpdatePresence();
#endif
}

boolean G_GamestateUsesLevel(void)
{
	switch (gamestate)
	{
		case GS_TITLESCREEN:
			return titlemapinaction;

		case GS_LEVEL:
		case GS_CEREMONY:
			return true;

		default:
			return false;
	}
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
	if (retrying == false)
	{
		grandprixinfo.rank.continuesUsed++;
	}

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
