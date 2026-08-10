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
/// \file  doomstat.h
/// \brief All the global variables that store the internal state.
///
///        Theoretically speaking, the internal state of the engine
///        should be found by looking at the variables collected
///        here, and every relevant module will have to include
///        this header file. In practice... things are a bit messy.

#ifndef DOOMSTAT_H
#define DOOMSTAT_H

// We need globally shared data structures, for defining the global state variables.
#include "doomdata.h"

// We need the player data structure as well.
#include "d_player.h"

// For lumpnum_t.
#include "w_wad.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================
// Selected map etc.
// =============================

#define ROUNDQUEUE_MAX 10 // sane max? maybe make dynamically allocated later
// These two live in gametype field of packets
#define ROUNDQUEUE_CMD_CLEAR UINT16_MAX
#define ROUNDQUEUE_CMD_SHOW UINT16_MAX-1
// The roundqueue itself is resident in g_game.h

// Selected by user.
extern int16_t gamemap;
extern dboolean g_reloadingMap;
extern char mapmusname[7];
extern uint32_t mapmusposition;
extern uint32_t mapmusresume;
extern uint8_t mapmusrng;
#define MUSIC_TRACKMASK   0x0FFF // ----************
#define MUSIC_RELOADRESET 0x8000 // *---------------
#define MUSIC_FORCERESET  0x4000 // -*--------------
// Use other bits if necessary.

extern uint32_t maptol;

extern int32_t cursaveslot;
extern uint8_t gamecomplete;

#define CUPMENU_COLUMNS 7
#define CUPMENU_ROWS 2

// Extra abilities/settings for skins (combinable stuff)
typedef int32_t marathonmode_t;
#define MA_RUNNING     (1)    // In action
#define MA_INIT        (1<<1) // Initialisation
#define MA_NOCUTSCENES (1<<2) // No cutscenes
#define MA_INGAME      (1<<3)  // Timer ignores loads

extern marathonmode_t marathonmode;
extern tic_t marathontime;

#define maxgameovers 13
extern uint8_t numgameovers;
extern int8_t startinglivesbalance[maxgameovers+1];

#define NUMPRECIPFREESLOTS 64

typedef enum
{
	PRECIP_NONE = 0,

	PRECIP_RAIN,
	PRECIP_SNOW,
	PRECIP_BLIZZARD,
	PRECIP_STORM,
	PRECIP_STORM_NORAIN,
	PRECIP_STORM_NOSTRIKES,

	PRECIP_FIRSTFREESLOT,
	PRECIP_LASTFREESLOT = PRECIP_FIRSTFREESLOT + NUMPRECIPFREESLOTS - 1,

	MAXPRECIP
} preciptype_t;

typedef int32_t precipeffect_t;
#define PRECIPFX_THUNDER (1)
#define PRECIPFX_LIGHTNING (1<<1)
#define PRECIPFX_WATERPARTICLES (1<<2)

struct precipprops_t
{
	const char *name;
	mobjtype_t type;
	precipeffect_t effects;
};

extern precipprops_t precipprops[MAXPRECIP];
extern preciptype_t precip_freeslot;

extern preciptype_t globalweather;
extern preciptype_t curWeather;

/** Time attack information, currently a very small structure.
  */

struct skinrecord_t
{
	uint32_t wins;
	uint32_t rounds;
	uint32_t timeplayed;
	uint32_t modetimeplayed[GDGT_MAX];
	uint32_t tumbletime;
};

struct unloaded_skin_t
{
	char name[SKINNAMESIZE+1];
	uint32_t namehash;

	skinrecord_t records;

	unloaded_skin_t *next;
};

extern unloaded_skin_t *unloadedskins;

struct skinreference_t
{
	unloaded_skin_t *unloaded;
	uint16_t id;
};

// mapvisited is now a set of flags that says what we've done in the map.
#define MV_VISITED      	(1)
#define MV_BEATEN       	(1<<1)
#define MV_ENCORE       	(1<<2)
#define MV_SPBATTACK    	(1<<3)
#define MV_MYSTICMELODY		(1<<4)
#define MV_MAX          	(MV_VISITED|MV_BEATEN|MV_ENCORE|MV_SPBATTACK|MV_MYSTICMELODY)

#define MCAN_INVALID		(UINT16_MAX)
#define MCAN_BONUS			(UINT16_MAX-1)

struct recordtimes_t
{
	tic_t time; ///< Time in which the level was finished.
	tic_t lap;  ///< Best lap time for this level.
};

struct recorddata_t
{
	uint8_t mapvisited; ///< Generalised flags
	recordtimes_t timeattack; ///< Best times for Time Attack
	recordtimes_t spbattack; ///< Best times for SPB Attack
	uint16_t spraycan; ///< Associated spraycan id
	uint32_t timeplayed;
	uint32_t netgametimeplayed;
	uint32_t modetimeplayed[GDGT_MAX];
	uint32_t timeattacktimeplayed;
	uint32_t spbattacktimeplayed;
	uint32_t rounds;
	uint32_t wins;
};

#define KARTSPEED_AUTO -1
#define KARTSPEED_EASY 0
#define KARTSPEED_NORMAL 1
#define KARTSPEED_HARD 2
#define KARTGP_MASTER 3 // Not a speed setting, gives the hardest speed with maxed out bots
#define KARTGP_MAX 4

typedef enum
{
	GRADE_INVALID = -1,
	GRADE_E,
	GRADE_D,
	GRADE_C,
	GRADE_B,
	GRADE_A,
	GRADE_S
} gp_rank_e;

struct cupwindata_t
{
	uint8_t best_placement;
	gp_rank_e best_grade;
	dboolean got_emerald;
	skinreference_t best_skin;
};

// Set if homebrew PWAD stuff has been added.
extern dboolean modifiedgame;
extern dboolean majormods;
extern uint16_t mainwads;
extern uint16_t musicwads;
extern dboolean savemoddata; // This mod saves time/emblem data.
extern dboolean usedCheats;
extern dboolean imcontinuing; // Temporary flag while continuing

#define ATTACKING_NONE	0
#define ATTACKING_TIME	1
#define ATTACKING_LAP	(1<<1)
#define ATTACKING_SPB	(1<<2)
extern uint8_t modeattacking;
const char *M_GetRecordMode(void);

// menu demo things
extern uint8_t  numDemos;
extern uint32_t demoDelayTime;
extern uint32_t demoIdleTime;

// Netgame? only true in a netgame
extern dboolean netgame;
extern dboolean addedtogame; // true after the server has added you
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern dboolean multiplayer;

extern uint8_t splitscreen;
extern int r_splitscreen;

extern dboolean forceresetplayers, deferencoremode, forcespecialstage;
extern dboolean staffsync;
extern uint32_t staffsync_map, staffsync_ghost, staffsync_done, staffsync_total, staffsync_failed;

struct staffsync_t
{
	uint32_t map;
	char name[MAXPLAYERNAME+1];
	uint32_t reason;
	uint32_t extra;
	fixed_t totalerror;
	uint32_t numerror;
	uint32_t rngerror_presync[32];
	uint32_t rngerror_postsync[32];
};
extern staffsync_t staffsync_results[1024];

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern dboolean sound_disabled;
extern dboolean digital_disabled;
extern dboolean g_voice_disabled;

// =========================
// Status flags for refresh.
// =========================
//

extern dboolean menuactive; // Menu overlaid?
extern uint8_t paused; // Game paused?
extern uint8_t window_notinfocus; // are we in focus? (backend independant -- handles auto pausing and display of "focus lost" message)
extern int32_t window_x;
extern int32_t window_y;

extern dboolean nodrawers;
extern dboolean noblit;
extern dboolean lastdraw;
extern postimg_t postimgtype[MAXSPLITSCREENPLAYERS];
extern int32_t postimgparam[MAXSPLITSCREENPLAYERS];

extern int32_t viewwindowx, viewwindowy;
extern int32_t viewwidth, scaledviewwidth;

// Player taking events, and displaying.
extern int32_t consoleplayer;
extern int32_t displayplayers[MAXSPLITSCREENPLAYERS];
/* g_localplayers[0] = consoleplayer */
extern int32_t g_localplayers[MAXSPLITSCREENPLAYERS];

extern char * titlemap;
extern dboolean hidetitlepics;
extern dboolean looptitle;

extern char * bootmap; //bootmap for loading a map on startup
extern char * podiummap; // map to load for podium

extern char * tutorialplaygroundmap; // map to load for playground
extern char * tutorialchallengemap; // map to load for tutorial skip
extern uint8_t tutorialchallenge;
#define TUTORIALSKIP_NONE 0
#define TUTORIALSKIP_FAILED 1
#define TUTORIALSKIP_INPROGRESS 2

extern dboolean exitfadestarted;

struct scene_t
{
	uint8_t numpics;
	char picname[8][8];
	uint8_t pichires[8];
	char *text;
	uint16_t xcoord[8];
	uint16_t ycoord[8];
	uint16_t picduration[8];
	uint8_t musicloop;
	uint16_t textxpos;
	uint16_t textypos;

	char   musswitch[7];
	uint16_t musswitchflags;
	uint32_t musswitchposition;

	uint8_t fadecolor; // Color number for fade, 0 means don't do the first fade
	uint8_t fadeinid;  // ID of the first fade, to a color -- ignored if fadecolor is 0
	uint8_t fadeoutid; // ID of the second fade, to the new screen
}; // TODO: It would probably behoove us to implement subsong/track selection here, too, but I'm lazy -SH

struct cutscene_t
{
	scene_t scene[128]; // 128 scenes per cutscene.
	int32_t numscenes; // Number of scenes in this cutscene
};

extern cutscene_t *cutscenes[128];

// Reserve prompt space for tutorials
#define TUTORIAL_PROMPT 201 // one-based
#define TUTORIAL_AREAS 6
#define TUTORIAL_AREA_PROMPTS 5
#define MAX_PROMPTS (TUTORIAL_PROMPT+TUTORIAL_AREAS*TUTORIAL_AREA_PROMPTS*3) // 3 control modes
#define MAX_PAGES 128

#define PROMPT_PIC_PERSIST 0
#define PROMPT_PIC_LOOP 1
#define PROMPT_PIC_DESTROY 2
#define MAX_PROMPT_PICS 8
struct textpage_t
{
	uint8_t numpics;
	uint8_t picmode; // sequence mode after displaying last pic, 0 = persist, 1 = loop, 2 = destroy
	uint8_t pictoloop; // if picmode == loop, which pic to loop to?
	uint8_t pictostart; // initial pic number to show
	char picname[MAX_PROMPT_PICS][8];
	uint8_t pichires[MAX_PROMPT_PICS];
	uint16_t xcoord[MAX_PROMPT_PICS]; // gfx
	uint16_t ycoord[MAX_PROMPT_PICS]; // gfx
	uint16_t picduration[MAX_PROMPT_PICS];

	char   musswitch[7];
	uint16_t musswitchflags;
	uint8_t musicloop;

	char tag[33]; // page tag
	char name[34]; // narrator name, extra char for color
	char iconname[8]; // narrator icon lump
	dboolean rightside; // narrator side, false = left, true = right
	dboolean iconflip; // narrator flip icon horizontally
	uint8_t hidehud; // hide hud, 0 = show all, 1 = hide depending on prompt position (top/bottom), 2 = hide all
	uint8_t lines; // # of lines to show. If name is specified, name takes one of the lines. If 0, defaults to 4.
	int32_t backcolor; // see CON_SetupBackColormap: 0-11, INT32_MAX for user-defined (CONS_BACKCOLOR)
	uint8_t align; // text alignment, 0 = left, 1 = right, 2 = center
	uint8_t verticalalign; // vertical text alignment, 0 = top, 1 = bottom, 2 = middle
	uint8_t textspeed; // text speed, delay in tics between characters.
	sfxenum_t textsfx; // sfx_ id for printing text
	uint8_t nextprompt; // next prompt to jump to, one-based. 0 = current prompt
	uint8_t nextpage; // next page to jump to, one-based. 0 = next page within prompt->numpages
	char nexttag[33]; // next tag to jump to. If set, this overrides nextprompt and nextpage.
	int32_t timetonext; // time in tics to jump to next page automatically. 0 = don't jump automatically
	char *text;
};

struct textprompt_t
{
	textpage_t page[MAX_PAGES];
	int32_t numpages; // Number of pages in this prompt
};

extern textprompt_t *textprompts[MAX_PROMPTS];

// For the Custom Exit linedef.
extern uint16_t nextmapoverride;
extern uint8_t skipstats;

// Fun extra stuff
extern int16_t lastmap; // Last level you were at (returning from special stages).

// A single point in space.
struct mappoint_t
{
	fixed_t x, y, z;
};

struct quake_t
{
	tic_t time, startTime;
	fixed_t intensity;

	// optional intensity modulation based on position
	fixed_t radius;
	mappoint_t *epicenter;
	mobj_t *mobj;

	// linked list
	quake_t *next;
	quake_t *prev;
};

extern quake_t *g_quakes;

// Custom Lua values
struct customoption_t
{
	char option[32]; // 31 usable characters
	char value[256]; // 255 usable characters. If this seriously isn't enough then wtf.
};

// This could support more, but is that a good idea?
// Keep in mind that it may encourage people making overly long cups just because they "can", and would be a waste of memory.
#define MAXLEVELLIST 5
#define CUPCACHE_BONUS MAXLEVELLIST
#define MAXBONUSLIST 2
#define CUPCACHE_SPECIAL (CUPCACHE_BONUS+MAXBONUSLIST)
#define CUPCACHE_PODIUM (CUPCACHE_SPECIAL+1)
#define CUPCACHE_MAX (CUPCACHE_PODIUM+1)

#define MAXCUPNAME 16 // includes \0, for cleaner savedata

struct cupheader_t
{
	uint16_t id;								///< Cup ID
	uint8_t monitor;							///< Monitor graphic 1-9 or A-Z

	char name[MAXCUPNAME];					///< Cup title
	uint32_t namehash;						///< Cup title hash

	char realname[MAXCUPNAME];				///< Cup nomme de gurre

	char icon[9];							///< Name of the icon patch
	char *levellist[CUPCACHE_MAX];			///< List of levels that belong to this cup
	int16_t cachedlevels[CUPCACHE_MAX];		///< IDs in levellist, bonusgame, and specialstage
	uint8_t numlevels;						///< Number of levels defined in levellist
	uint8_t numbonus;							///< Number of bonus stages defined
	uint8_t emeraldnum;						///< ID of Emerald to use for special stage (1-7 for Chaos Emeralds, 8-14 for Super Emeralds, 0 for no emerald)

	// Modifiable in mainwads only
	dboolean playcredits;					///< Play the credits?
	uint16_t hintcondition;					///< Hint condition for 2.4 Super Cup

	// Truly internal data
	uint16_t cache_cuplock;					///< Cached Unlockable ID
	cupwindata_t windata[4];				///< Data for cup visitation
	cupheader_t *next;						///< Next cup in linked list
};

extern cupheader_t *kartcupheaders; // Start of cup linked list
extern uint16_t numkartcupheaders, basenumkartcupheaders;

struct unloaded_cupheader_t
{
	char name[MAXCUPNAME];
	uint32_t namehash;

	cupwindata_t windata[4];

	unloaded_cupheader_t *next;
};

extern unloaded_cupheader_t *unloadedcupheaders;

#define MAXMAPLUMPNAME 64 // includes \0, for cleaner savedata

struct staffbrief_t
{
	uint16_t wad;
	uint16_t lump;
	char name[MAXPLAYERNAME+1];
	tic_t time;
	tic_t lap;
};

#define MAXMUSNAMES 3 // maximum definable music tracks per level
#define MAXDESTRUCTIBLES 3
#define MAXHEADERFOLLOWERS 32

struct mapheader_lighting_t
{
	uint8_t light_contrast;				///< Range of wall lighting. 0 is no lighting.
	int8_t sprite_backlight;				///< Subtract from wall lighting for sprites only.
	dboolean use_light_angle;			///< When false, wall lighting is evenly distributed. When true, wall lighting is directional.
	angle_t light_angle;				///< Angle of directional wall lighting.
};

/** Map header information.
  */
struct mapheader_t
{
	// Core game information, not user-modifiable directly
	char *lumpname;						///< Lump name can be really long
	uint32_t lumpnamehash;				///< quickncasehash(->lumpname, MAXMAPLUMPNAME)
	lumpnum_t lumpnum;       			///< Lump number for the map, used by vres_GetMap

	void *thumbnailPic;					///< Lump data for the level select thumbnail.
	void *minimapPic;					///< Lump data for the minimap graphic.
	void *encoreLump;					///< Lump data for the Encore Mode remap.
	void *tweakLump;					///< Lump data for the palette tweak remap.

	// Staff Ghost information
	uint8_t ghostCount;					///< Count of valid staff ghosts
	uint32_t ghostBriefSize;              ///< Size of ghostBrief vector allocation
	staffbrief_t **ghostBrief;			///< Valid staff ghosts, pointers are owned
	tic_t automedaltime[4];             ///< Auto Medal times derived from ghost times, best to worst

	recorddata_t records;				///< Stores completion/record attack data

	cupheader_t *cup;					///< Cached cup

	size_t justPlayed;					///< Prevent this map from showing up in votes if it was recently picked.
	size_t anger;						///< No one picked this map... it's mad now.

	// Titlecard information
	char lvlttl[22];					///< Level name without "Zone". (21 character limit instead of 32, 21 characters can display on screen max anyway)
	char menuttl[22];					///< Menu title for level
	char zonttl[22];					///< "ZONE" replacement name
	uint8_t actnum;						///< Act number or 0 for none.

	// Selection metadata
	char keywords[33];					///< Keywords separated by space to search for. 32 characters.

	uint8_t levelselect;					///< Is this map available in the level select? If so, which map list is it available in?
	uint16_t menuflags;					///< LF2_flags: options that affect record attack menus
	uint8_t playerLimit;					///< This map does not appear in multiplayer vote if there are too many players

	// Operational metadata
	uint16_t levelflags;					///< LF_flags:  merged booleans into one uint16_t for space, see below
	uint32_t typeoflevel;					///< Combination of typeoflevel flags.
	uint8_t numlaps;						///< Number of laps in circuit mode, unless overridden.
	uint8_t lapspersection;				///< Number of laps per section in hybrid section-circuit maps.
	fixed_t gravity;					///< Map-wide gravity.
	char relevantskin[SKINNAMESIZE+1];	///< Skin to use for tutorial (if not provided, uses Eggman.)

	// Music information
	char musname[MAXMUSNAMES][7];			///< Music tracks to play. First dimension is the track number, second is the music string. "" for no music.
	char encoremusname[MAXMUSNAMES][7];	///< Music tracks to play in Encore. First dimension is the track number, second is the music string. "" for no music.
	uint16_t cache_muslock[MAXMUSNAMES-1];	///< Cached Alt Music IDs
	char associatedmus[MAXMUSNAMES][7];		///< Associated music tracks for sound test unlock.
	char positionmus[7];					///< Custom Position track. Doesn't play in Encore or other fun game-controlled contexts
	uint8_t musname_size;						///< Number of music tracks defined
	uint8_t encoremusname_size;				///< Number of Encore music tracks defined
	uint8_t associatedmus_size;				///< Number of associated music tracks defined
	uint16_t mustrack;						///< Subsong to play. Only really relevant for music modules and specific formats supported by GME. 0 to ignore.
	uint32_t muspos;							///< Music position to jump to.

	// Sky information
	uint8_t weather;						///< See preciptype_t
	char skytexture[9];					///< Sky texture to use.
	int16_t skybox_scalex;				///< Skybox X axis scale. (0 = no movement, 1 = 1:1 movement, 16 = 16:1 slow movement, -4 = 1:4 fast movement, etc.)
	int16_t skybox_scaley;				///< Skybox Y axis scale.
	int16_t skybox_scalez;				///< Skybox Z axis scale.

	fixed_t darkness;					///< Pohbee darkness multiplier

	// Distance information
	fixed_t mobj_scale;					///< Defines the size all object calculations are relative to
	fixed_t default_waypoint_radius;	///< 0 is a special value for DEFAULT_WAYPOINT_RADIUS, but scaled with mobjscale

	// Visual information
	uint16_t palette;						///< PAL lump to use on this map
	uint16_t encorepal;					///< PAL for encore mode

	mapheader_lighting_t lighting;			///< Wall and sprite lighting
	mapheader_lighting_t lighting_encore;	///< Alternative lighting for Encore mode
	dboolean use_encore_lighting;			///< Whether to use separate Encore lighting

	fixed_t cameraHeight;					///< Player camera height to use on this map

	// Audience information
	uint8_t numFollowers;					///< Internal. For audience support.
	int16_t *followers;					///< List of audience followers in this level. Allocated dynamically for space reasons. Be careful.

	// Script information
	char runsoc[33];					///< SOC to execute at start of level (32 character limit instead of 63)
	char scriptname[33];				///< Script to use when the map is switched to. (32 character limit instead of 191)

	// Cutscene information
	uint8_t precutscenenum;				///< Cutscene number to play BEFORE a level starts.
	uint8_t cutscenenum;					///< Cutscene number to use, 0 for none.

	mobjtype_t destroyforchallenge[MAXDESTRUCTIBLES];	///< Assistive for UCRP_MAPDESTROYOBJECTS
	uint8_t destroyforchallenge_size;						///< Number for above

	uint16_t cache_maplock;				///< Cached Unlockable ID

	// Lua information
	uint8_t numCustomOptions;				///< Internal. For Lua custom value support.
	customoption_t *customopts;			///< Custom options. Allocated dynamically for space reasons. Be careful.
};

// level flags
//#define LF_(this slot is free) (1<<0)
#define LF_NOZONE             (1<<1) ///< Don't include "ZONE" on level title
#define LF_SECTIONRACE        (1<<2) ///< Section race level
#define LF_SUBTRACTNUM        (1<<3) ///< Use subtractive position number (for bright levels)
#define LF_NOCOMMS			  (1<<4) ///< Disable dialogue "communications in progress" graphic

#define LF2_HIDEINMENU		(1<<0) ///< Hide in the multiplayer menu
#define LF2_NOTIMEATTACK	(1<<1) ///< Hide this map in Time Attack modes
#define LF2_NOVISITNEEDED	(1<<2) ///< Map does not require visitation to be selectable
#define LF2_FINISHNEEDED	(1<<3) ///< Not available in Time Attack modes until you beat the level

extern mapheader_t** mapheaderinfo;
extern int32_t nummapheaders, basenummapheaders, mapallocsize;

struct unloaded_mapheader_t
{
	char *lumpname;
	uint32_t lumpnamehash;

	recorddata_t records;

	unloaded_mapheader_t *next;
};

extern unloaded_mapheader_t *unloadedmapheaders;

// Gametypes
#define NUMGAMETYPEFREESLOTS (128)
#define MAXGAMETYPELENGTH (32)

enum GameType
{
	GT_RACE = 0,
	GT_BATTLE,
	GT_SPECIAL,
	GT_VERSUS,
	GT_TUTORIAL,

	GT_FIRSTFREESLOT,
	GT_LASTFREESLOT = GT_FIRSTFREESLOT + NUMGAMETYPEFREESLOTS - 1,
	MAXGAMETYPES
};
// If you alter this list, update defaultgametypes and *gametypes in g_game.c

#define MAXTOL             (1<<31)
#define NUMBASETOLNAMES    (5)
#define NUMTOLNAMES        (NUMBASETOLNAMES + NUMGAMETYPEFREESLOTS)

struct gametype_t
{
	const char *name;
	const char *constant;
	uint32_t rules;
	uint32_t tol;
	uint8_t intermission;
	int8_t speed;
	int32_t pointlimit;
	int32_t timelimit;
	char gppic[9];
	char gppicmini[9];
};

extern gametype_t *gametypes[MAXGAMETYPES+1];
extern int16_t numgametypes;

extern int16_t gametype, g_lastgametype;

// Gametype rules
typedef int32_t GameTypeRules;
#define GTR_CIRCUIT				(1)		// Enables the finish line, laps, and the waypoint system.
#define GTR_BOTS				(1<<1)		// Allows bots in this gametype. Combine with BotTiccmd hooks to make bots support your gametype.

// Battle gametype rules
#define GTR_BUMPERS				(1<<2)		// Enables the bumper health system
#define GTR_SPHERES				(1<<3)		// Replaces rings with blue spheres
#define GTR_CLOSERPLAYERS		(1<<4)		// Buffs spindash and draft power to bring everyone together, nerfs invincibility and grow to prevent excessive combos

#define GTR_BATTLESTARTS		(1<<5)		// Use Battle Mode start positions.
#define GTR_PAPERITEMS			(1<<6)		// Replaces item boxes with paper item spawners
#define GTR_POWERSTONES			(1<<7)		// Battle Emerald collectables.
#define GTR_KARMA				(1<<8)		// Enables the Karma system if you're out of bumpers
// 1<<9 - UNUSED

// Bonus gametype rules
#define GTR_CHECKPOINTS			(1<<10)	// Player respawns at specific checkpoints
#define GTR_PRISONS				(1<<11)	// Can enter Prison Break mode
#define GTR_CATCHER				(1<<12)	// UFO Catcher (only works with GTR_CIRCUIT)
#define GTR_ROLLINGSTART		(1<<13)	// Rolling start (only works with GTR_CIRCUIT)
#define GTR_SPECIALSTART		(1<<14)	// White fade instant start
#define GTR_BOSS				(1<<15)	// Boss intro and spawning

// General purpose rules
#define GTR_POINTLIMIT			(1<<16)	// Reaching point limit ends the round
#define GTR_TIMELIMIT			(1<<17)	// Reaching time limit ends the round
#define GTR_OVERTIME			(1<<18)	// Allow overtime behavior
#define GTR_ENCORE				(1<<19)	// Alternate Encore mirroring, scripting, and texture remapping

#define GTR_TEAMS				(1<<20)	// Teams are forced on
#define GTR_NOTEAMS				(1<<21)	// Teams are forced off
#define GTR_TEAMSTARTS			(1<<22)	// Use team-based start positions

#define GTR_NOMP				(1<<23)	// No multiplayer
#define GTR_NOCUPSELECT			(1<<24)	// Your maps are not selected via cup.
#define GTR_NOPOSITION			(1<<25)	// No POSITION

// free: to and including 1<<31
// Remember to update GAMETYPERULE_LIST in deh_soc.c

#define GTR_FORBIDMP (GTR_NOMP|GTR_CATCHER|GTR_BOSS)

// TODO: replace every instance
#define gametyperules (gametypes[gametype]->rules)

// TypeOfLevel things
typedef int32_t TypeOfLevel;
#define TOL_RACE	 (0x0001) ///< Race
#define TOL_BATTLE	 (0x0002) ///< Battle
#define TOL_SPECIAL	 (0x0004) ///< Special Stage (variant of race, but forbidden)
#define TOL_VERSUS	 (0x0008) ///< Versus (variant of battle, but forbidden)
#define TOL_TUTORIAL (0x0010) ///< Tutorial (variant of race, but forbidden)

// Modifiers
#define TOL_TV		(0x0100) ///< Midnight Channel specific: draw TV like overlay on HUD
// Make sure to update TYPEOFLEVEL too

#define MAXTOL             (1<<31)
#define NUMBASETOLNAMES    (5)
#define NUMTOLNAMES        (NUMBASETOLNAMES + NUMGAMETYPEFREESLOTS)

struct tolinfo_t
{
	const char *name;
	uint32_t flag;
};
extern tolinfo_t TYPEOFLEVEL[NUMTOLNAMES];
extern uint32_t lastcustomtol;

extern uint8_t stagefailed;

// Emeralds stored as bits to throw savegame hackers off.
typedef int32_t emeraldflags_t;
#define EMERALD_CHAOS1 (1)
#define EMERALD_CHAOS2 (1<<1)
#define EMERALD_CHAOS3 (1<<2)
#define EMERALD_CHAOS4 (1<<3)
#define EMERALD_CHAOS5 (1<<4)
#define EMERALD_CHAOS6 (1<<5)
#define EMERALD_CHAOS7 (1<<6)
#define EMERALD_ALLCHAOS ((EMERALD_CHAOS1)|(EMERALD_CHAOS2)|(EMERALD_CHAOS3)|(EMERALD_CHAOS4)|(EMERALD_CHAOS5)|(EMERALD_CHAOS6)|(EMERALD_CHAOS7))

#define EMERALD_SUPER1 (1<<7)
#define EMERALD_SUPER2 (1<<8)
#define EMERALD_SUPER3 (1<<9)
#define EMERALD_SUPER4 (1<<10)
#define EMERALD_SUPER5 (1<<11)
#define EMERALD_SUPER6 (1<<12)
#define EMERALD_SUPER7 (1<<13)
#define EMERALD_ALLSUPER ((EMERALD_SUPER1)|(EMERALD_SUPER2)|(EMERALD_SUPER3)|(EMERALD_SUPER4)|(EMERALD_SUPER5)|(EMERALD_SUPER6)|(EMERALD_SUPER7))

#define EMERALD_ALL ((EMERALD_ALLCHAOS)|(EMERALD_ALLSUPER))

#define ALLCHAOSEMERALDS(v) ((v & EMERALD_ALLCHAOS) == EMERALD_ALLCHAOS)
#define ALLSUPEREMERALDS(v) ((v & EMERALD_ALLSUPER) == EMERALD_ALLSUPER)
#define ALLEMERALDS(v) ((v & EMERALD_ALL) == EMERALD_ALL)

#define NUM_LUABANKS 16 // please only make this number go up between versions, never down. you'll break saves otherwise. also, must fit in uint8_t
extern int32_t luabanks[NUM_LUABANKS];

extern int32_t nummaprings; //keep track of spawned rings/coins

extern uint8_t nummapspraycans;
extern uint16_t numchallengedestructibles;

// Teamplay
typedef enum
{
	TEAM_UNASSIGNED = 0,
	TEAM_ORANGE,
	TEAM_BLUE,
	TEAM__MAX
} team_e;

struct teaminfo_t
{
	const char *name;
	skincolornum_t color;
	uint32_t chat_color;
};

extern teaminfo_t g_teaminfo[TEAM__MAX];
extern uint32_t g_teamscores[TEAM__MAX];

// Eliminates unnecessary searching.
extern dboolean CheckForBustableBlocks;
extern dboolean CheckForBouncySector;
extern dboolean CheckForQuicksand;
extern dboolean CheckForMarioBlocks;
extern dboolean CheckForFloatBob;
extern dboolean CheckForReverseGravity;

// Powerup durations
extern uint16_t invulntics;
extern uint16_t sneakertics;
extern uint16_t flashingtics;
extern uint16_t tailsflytics;
extern uint16_t underwatertics;
extern uint16_t spacetimetics;
extern uint16_t extralifetics;
extern uint16_t nightslinktics;

// SRB2kart
extern tic_t introtime;
extern tic_t starttime;

extern const tic_t bulbtime;
extern uint8_t numbulbs;

extern int32_t hyudorotime;
extern int32_t stealtime;
extern int32_t sneakertime;
extern int32_t itemtime;
extern int32_t bubbletime;
extern int32_t comebacktime;
extern int32_t bumptime;
extern int32_t ebraketime;
extern int32_t greasetics;
extern int32_t wipeoutslowtime;
extern int32_t wantedreduce;
extern int32_t wantedfrequency;

extern uint8_t introtoplay;
extern uint8_t g_credits_cutscene;
extern uint8_t useSeal;

extern uint8_t use1upSound;
extern uint8_t maxXtraLife; // Max extra lives from rings

struct exitcondition_t
{
	dboolean losing;
	dboolean retry;
};

// For racing
extern tic_t racecountdown, exitcountdown, musiccountdown;
extern exitcondition_t g_exit;

#define DARKNESS_FADE_TIME (8)
extern struct darkness_t
{
	tic_t start, end;
	fixed_t value[MAXSPLITSCREENPLAYERS];
} g_darkness;

extern struct musicfade_t
{
	tic_t start, end, fade;
	dboolean ticked;
} g_musicfade;

#define DEFAULT_GRAVITY (4*FRACUNIT/5)
extern fixed_t gravity;
extern fixed_t mapobjectscale;

extern struct maplighting
{
	uint8_t contrast;
	int8_t backlight;
	dboolean directional;
	angle_t angle;
} maplighting;

// SRB2kart
extern uint8_t numlaps;
extern uint8_t gamespeed;
extern dboolean franticitems;
extern dboolean encoremode, prevencoremode;
extern dboolean g_teamplay;
extern dboolean g_duelpermitted;

extern tic_t wantedcalcdelay;
extern tic_t itemCooldowns[NUMKARTITEMS - 1];
extern tic_t mapreset;
extern dboolean thwompsactive;
extern uint8_t lastLowestLap;
extern int8_t spbplace;
extern dboolean rainbowstartavailable;
extern tic_t attacktimingstarted;
extern dboolean inDuel;
extern uint8_t overtimecheckpoints;

extern tic_t bombflashtimer;	// Used to avoid causing seizures if multiple mines explode close to you :)
extern dboolean legitimateexit;
extern dboolean comebackshowninfo;

#define VOTE_NUM_LEVELS (4)
#define VOTE_NOT_PICKED (-1)
#define VOTE_SPECIAL (MAXPLAYERS)
#define VOTE_TOTAL (MAXPLAYERS+1)

#define VOTE_TIMEOUT_LOSER (MAXPLAYERS+1) // not a real vote ID
#define VOTE_TIMEOUT_WINNER (MAXPLAYERS+2) // ditto

extern uint16_t g_voteLevels[VOTE_NUM_LEVELS][2];
extern int8_t g_votes[VOTE_TOTAL];
extern int8_t g_pickedVote;
extern dboolean g_votes_striked[VOTE_NUM_LEVELS];

// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern tic_t gametic;
#define localgametic leveltime

// Player spawn spots.
extern mapthing_t *playerstarts[MAXPLAYERS]; // Cooperative
extern mapthing_t *teamstarts[TEAM__MAX][MAXPLAYERS]; // Teamplay
extern mapthing_t *faultstart; // Kart Fault

#define TUBEWAYPOINTSEQUENCESIZE 256
#define NUMTUBEWAYPOINTSEQUENCES 256
extern mobj_t *tubewaypoints[NUMTUBEWAYPOINTSEQUENCES][TUBEWAYPOINTSEQUENCESIZE];
extern uint16_t numtubewaypoints[NUMTUBEWAYPOINTSEQUENCES];

void P_AddTubeWaypoint(uint8_t sequence, uint8_t id, mobj_t *waypoint);
mobj_t *P_GetFirstTubeWaypoint(uint8_t sequence);
mobj_t *P_GetLastTubeWaypoint(uint8_t sequence);
mobj_t *P_GetPreviousTubeWaypoint(mobj_t *current, dboolean wrap);
mobj_t *P_GetNextTubeWaypoint(mobj_t *current, dboolean wrap);
mobj_t *P_GetClosestTubeWaypoint(uint8_t sequence, mobj_t *mo);
dboolean P_IsDegeneratedTubeWaypointSequence(uint8_t sequence);

// =====================================
// Internal parameters, used for engine.
// =====================================

#if defined (macintosh)
#define DEBFILE(msg) I_OutputMsg(msg)
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if (debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
#else
#define DEBFILE(msg) {}
#endif
#endif

#ifdef DEBUGFILE
extern FILE *debugfile;
extern int32_t debugload;
#endif

// if true, load all graphics at level load
extern dboolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;
extern int16_t wipetypepre;
extern int16_t wipetypepost;

// debug flag to cancel adaptiveness
extern dboolean g_singletics;
extern tic_t g_fast_forward;
extern tic_t g_fast_forward_clock_stop;

#define singletics (g_singletics == true || g_fast_forward > 0)

// =============
// Netgame stuff
// =============

#include "d_clisrv.h"

extern consvar_t cv_forceskin; // force clients to use the server's skin
extern consvar_t cv_downloading; // allow clients to downloading WADs.
extern consvar_t cv_nettimeout; // SRB2Kart: Advanced server options menu
extern consvar_t cv_jointimeout;
extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
extern int32_t serverplayer;
extern int32_t adminplayers[MAXPLAYERS];

/// \note put these in d_clisrv outright?

#ifdef __cplusplus
} // extern "C"
#endif

#endif //DOOMSTAT_H
