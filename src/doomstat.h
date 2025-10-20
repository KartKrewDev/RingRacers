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

#ifndef __DOOMSTAT__
#define __DOOMSTAT__

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
extern INT16 gamemap;
extern boolean g_reloadingMap;
extern char mapmusname[7];
extern UINT32 mapmusposition;
extern UINT32 mapmusresume;
extern UINT8 mapmusrng;
#define MUSIC_TRACKMASK   0x0FFF // ----************
#define MUSIC_RELOADRESET 0x8000 // *---------------
#define MUSIC_FORCERESET  0x4000 // -*--------------
// Use other bits if necessary.

extern UINT32 maptol;

extern INT32 cursaveslot;
extern UINT8 gamecomplete;

#define CUPMENU_COLUMNS 7
#define CUPMENU_ROWS 2

// Extra abilities/settings for skins (combinable stuff)
typedef enum
{
	MA_RUNNING     = 1,    // In action
	MA_INIT        = 1<<1, // Initialisation
	MA_NOCUTSCENES = 1<<2, // No cutscenes
	MA_INGAME      = 1<<3  // Timer ignores loads
} marathonmode_t;

extern marathonmode_t marathonmode;
extern tic_t marathontime;

#define maxgameovers 13
extern UINT8 numgameovers;
extern SINT8 startinglivesbalance[maxgameovers+1];

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

typedef enum
{
	PRECIPFX_THUNDER = 1,
	PRECIPFX_LIGHTNING = 1<<1,
	PRECIPFX_WATERPARTICLES = 1<<2
} precipeffect_t;

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
	UINT32 wins;
	UINT32 rounds;
	UINT32 timeplayed;
	UINT32 modetimeplayed[GDGT_MAX];
	UINT32 tumbletime;
};

struct unloaded_skin_t
{
	char name[SKINNAMESIZE+1];
	UINT32 namehash;

	skinrecord_t records;

	unloaded_skin_t *next;
};

extern unloaded_skin_t *unloadedskins;

struct skinreference_t
{
	unloaded_skin_t *unloaded;
	UINT16 id;
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
	UINT8 mapvisited; ///< Generalised flags
	recordtimes_t timeattack; ///< Best times for Time Attack
	recordtimes_t spbattack; ///< Best times for SPB Attack
	UINT16 spraycan; ///< Associated spraycan id
	UINT32 timeplayed;
	UINT32 netgametimeplayed;
	UINT32 modetimeplayed[GDGT_MAX];
	UINT32 timeattacktimeplayed;
	UINT32 spbattacktimeplayed;
	UINT32 rounds;
	UINT32 wins;
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
	UINT8 best_placement;
	gp_rank_e best_grade;
	boolean got_emerald;
	skinreference_t best_skin;
};

// Set if homebrew PWAD stuff has been added.
extern boolean modifiedgame;
extern boolean majormods;
extern UINT16 mainwads;
extern UINT16 musicwads;
extern boolean savemoddata; // This mod saves time/emblem data.
extern boolean usedCheats;
extern boolean imcontinuing; // Temporary flag while continuing

#define ATTACKING_NONE	0
#define ATTACKING_TIME	1
#define ATTACKING_LAP	(1<<1)
#define ATTACKING_SPB	(1<<2)
extern UINT8 modeattacking;
const char *M_GetRecordMode(void);

// menu demo things
extern UINT8  numDemos;
extern UINT32 demoDelayTime;
extern UINT32 demoIdleTime;

// Netgame? only true in a netgame
extern boolean netgame;
extern boolean addedtogame; // true after the server has added you
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern boolean multiplayer;

extern UINT8 splitscreen;
extern int r_splitscreen;

extern boolean forceresetplayers, deferencoremode, forcespecialstage;
extern boolean staffsync;
extern UINT32 staffsync_map, staffsync_ghost, staffsync_done, staffsync_total, staffsync_failed;

struct staffsync_t
{
	UINT32 map;
	char name[MAXPLAYERNAME+1];
	UINT32 reason;
	UINT32 extra;
	fixed_t totalerror;
	UINT32 numerror;
	UINT32 rngerror_presync[32];
	UINT32 rngerror_postsync[32];
};
extern staffsync_t staffsync_results[1024];

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean sound_disabled;
extern boolean digital_disabled;
extern boolean g_voice_disabled;

// =========================
// Status flags for refresh.
// =========================
//

extern boolean menuactive; // Menu overlaid?
extern UINT8 paused; // Game paused?
extern UINT8 window_notinfocus; // are we in focus? (backend independant -- handles auto pausing and display of "focus lost" message)
extern INT32 window_x;
extern INT32 window_y;

extern boolean nodrawers;
extern boolean noblit;
extern boolean lastdraw;
extern postimg_t postimgtype[MAXSPLITSCREENPLAYERS];
extern INT32 postimgparam[MAXSPLITSCREENPLAYERS];

extern INT32 viewwindowx, viewwindowy;
extern INT32 viewwidth, scaledviewwidth;

// Player taking events, and displaying.
extern INT32 consoleplayer;
extern INT32 displayplayers[MAXSPLITSCREENPLAYERS];
/* g_localplayers[0] = consoleplayer */
extern INT32 g_localplayers[MAXSPLITSCREENPLAYERS];

extern char * titlemap;
extern boolean hidetitlepics;
extern boolean looptitle;

extern char * bootmap; //bootmap for loading a map on startup
extern char * podiummap; // map to load for podium

extern char * tutorialplaygroundmap; // map to load for playground
extern char * tutorialchallengemap; // map to load for tutorial skip
extern UINT8 tutorialchallenge;
#define TUTORIALSKIP_NONE 0
#define TUTORIALSKIP_FAILED 1
#define TUTORIALSKIP_INPROGRESS 2

extern boolean exitfadestarted;

struct scene_t
{
	UINT8 numpics;
	char picname[8][8];
	UINT8 pichires[8];
	char *text;
	UINT16 xcoord[8];
	UINT16 ycoord[8];
	UINT16 picduration[8];
	UINT8 musicloop;
	UINT16 textxpos;
	UINT16 textypos;

	char   musswitch[7];
	UINT16 musswitchflags;
	UINT32 musswitchposition;

	UINT8 fadecolor; // Color number for fade, 0 means don't do the first fade
	UINT8 fadeinid;  // ID of the first fade, to a color -- ignored if fadecolor is 0
	UINT8 fadeoutid; // ID of the second fade, to the new screen
}; // TODO: It would probably behoove us to implement subsong/track selection here, too, but I'm lazy -SH

struct cutscene_t
{
	scene_t scene[128]; // 128 scenes per cutscene.
	INT32 numscenes; // Number of scenes in this cutscene
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
	UINT8 numpics;
	UINT8 picmode; // sequence mode after displaying last pic, 0 = persist, 1 = loop, 2 = destroy
	UINT8 pictoloop; // if picmode == loop, which pic to loop to?
	UINT8 pictostart; // initial pic number to show
	char picname[MAX_PROMPT_PICS][8];
	UINT8 pichires[MAX_PROMPT_PICS];
	UINT16 xcoord[MAX_PROMPT_PICS]; // gfx
	UINT16 ycoord[MAX_PROMPT_PICS]; // gfx
	UINT16 picduration[MAX_PROMPT_PICS];

	char   musswitch[7];
	UINT16 musswitchflags;
	UINT8 musicloop;

	char tag[33]; // page tag
	char name[34]; // narrator name, extra char for color
	char iconname[8]; // narrator icon lump
	boolean rightside; // narrator side, false = left, true = right
	boolean iconflip; // narrator flip icon horizontally
	UINT8 hidehud; // hide hud, 0 = show all, 1 = hide depending on prompt position (top/bottom), 2 = hide all
	UINT8 lines; // # of lines to show. If name is specified, name takes one of the lines. If 0, defaults to 4.
	INT32 backcolor; // see CON_SetupBackColormap: 0-11, INT32_MAX for user-defined (CONS_BACKCOLOR)
	UINT8 align; // text alignment, 0 = left, 1 = right, 2 = center
	UINT8 verticalalign; // vertical text alignment, 0 = top, 1 = bottom, 2 = middle
	UINT8 textspeed; // text speed, delay in tics between characters.
	sfxenum_t textsfx; // sfx_ id for printing text
	UINT8 nextprompt; // next prompt to jump to, one-based. 0 = current prompt
	UINT8 nextpage; // next page to jump to, one-based. 0 = next page within prompt->numpages
	char nexttag[33]; // next tag to jump to. If set, this overrides nextprompt and nextpage.
	INT32 timetonext; // time in tics to jump to next page automatically. 0 = don't jump automatically
	char *text;
};

struct textprompt_t
{
	textpage_t page[MAX_PAGES];
	INT32 numpages; // Number of pages in this prompt
};

extern textprompt_t *textprompts[MAX_PROMPTS];

// For the Custom Exit linedef.
extern UINT16 nextmapoverride;
extern UINT8 skipstats;

// Fun extra stuff
extern INT16 lastmap; // Last level you were at (returning from special stages).

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
	UINT16 id;								///< Cup ID
	UINT8 monitor;							///< Monitor graphic 1-9 or A-Z

	char name[MAXCUPNAME];					///< Cup title
	UINT32 namehash;						///< Cup title hash

	char realname[MAXCUPNAME];				///< Cup nomme de gurre

	char icon[9];							///< Name of the icon patch
	char *levellist[CUPCACHE_MAX];			///< List of levels that belong to this cup
	INT16 cachedlevels[CUPCACHE_MAX];		///< IDs in levellist, bonusgame, and specialstage
	UINT8 numlevels;						///< Number of levels defined in levellist
	UINT8 numbonus;							///< Number of bonus stages defined
	UINT8 emeraldnum;						///< ID of Emerald to use for special stage (1-7 for Chaos Emeralds, 8-14 for Super Emeralds, 0 for no emerald)

	// Modifiable in mainwads only
	boolean playcredits;					///< Play the credits?
	UINT16 hintcondition;					///< Hint condition for 2.4 Super Cup

	// Truly internal data
	UINT16 cache_cuplock;					///< Cached Unlockable ID
	cupwindata_t windata[4];				///< Data for cup visitation
	cupheader_t *next;						///< Next cup in linked list
};

extern cupheader_t *kartcupheaders; // Start of cup linked list
extern UINT16 numkartcupheaders, basenumkartcupheaders;

struct unloaded_cupheader_t
{
	char name[MAXCUPNAME];
	UINT32 namehash;

	cupwindata_t windata[4];

	unloaded_cupheader_t *next;
};

extern unloaded_cupheader_t *unloadedcupheaders;

#define MAXMAPLUMPNAME 64 // includes \0, for cleaner savedata

struct staffbrief_t
{
	UINT16 wad;
	UINT16 lump;
	char name[MAXPLAYERNAME+1];
	tic_t time;
	tic_t lap;
};

#define MAXMUSNAMES 3 // maximum definable music tracks per level
#define MAXDESTRUCTIBLES 3
#define MAXHEADERFOLLOWERS 32

struct mapheader_lighting_t
{
	UINT8 light_contrast;				///< Range of wall lighting. 0 is no lighting.
	SINT8 sprite_backlight;				///< Subtract from wall lighting for sprites only.
	boolean use_light_angle;			///< When false, wall lighting is evenly distributed. When true, wall lighting is directional.
	angle_t light_angle;				///< Angle of directional wall lighting.
};

/** Map header information.
  */
struct mapheader_t
{
	// Core game information, not user-modifiable directly
	char *lumpname;						///< Lump name can be really long
	UINT32 lumpnamehash;				///< quickncasehash(->lumpname, MAXMAPLUMPNAME)
	lumpnum_t lumpnum;       			///< Lump number for the map, used by vres_GetMap

	void *thumbnailPic;					///< Lump data for the level select thumbnail.
	void *minimapPic;					///< Lump data for the minimap graphic.
	void *encoreLump;					///< Lump data for the Encore Mode remap.
	void *tweakLump;					///< Lump data for the palette tweak remap.

	// Staff Ghost information
	UINT8 ghostCount;					///< Count of valid staff ghosts
	UINT32 ghostBriefSize;              ///< Size of ghostBrief vector allocation
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
	UINT8 actnum;						///< Act number or 0 for none.

	// Selection metadata
	char keywords[33];					///< Keywords separated by space to search for. 32 characters.

	UINT8 levelselect;					///< Is this map available in the level select? If so, which map list is it available in?
	UINT16 menuflags;					///< LF2_flags: options that affect record attack menus
	UINT8 playerLimit;					///< This map does not appear in multiplayer vote if there are too many players

	// Operational metadata
	UINT16 levelflags;					///< LF_flags:  merged booleans into one UINT16 for space, see below
	UINT32 typeoflevel;					///< Combination of typeoflevel flags.
	UINT8 numlaps;						///< Number of laps in circuit mode, unless overridden.
	UINT8 lapspersection;				///< Number of laps per section in hybrid section-circuit maps.
	fixed_t gravity;					///< Map-wide gravity.
	char relevantskin[SKINNAMESIZE+1];	///< Skin to use for tutorial (if not provided, uses Eggman.)

	// Music information
	char musname[MAXMUSNAMES][7];			///< Music tracks to play. First dimension is the track number, second is the music string. "" for no music.
	char encoremusname[MAXMUSNAMES][7];	///< Music tracks to play in Encore. First dimension is the track number, second is the music string. "" for no music.
	UINT16 cache_muslock[MAXMUSNAMES-1];	///< Cached Alt Music IDs
	char associatedmus[MAXMUSNAMES][7];		///< Associated music tracks for sound test unlock.
	char positionmus[7];					///< Custom Position track. Doesn't play in Encore or other fun game-controlled contexts
	UINT8 musname_size;						///< Number of music tracks defined
	UINT8 encoremusname_size;				///< Number of Encore music tracks defined
	UINT8 associatedmus_size;				///< Number of associated music tracks defined
	UINT16 mustrack;						///< Subsong to play. Only really relevant for music modules and specific formats supported by GME. 0 to ignore.
	UINT32 muspos;							///< Music position to jump to.

	// Sky information
	UINT8 weather;						///< See preciptype_t
	char skytexture[9];					///< Sky texture to use.
	INT16 skybox_scalex;				///< Skybox X axis scale. (0 = no movement, 1 = 1:1 movement, 16 = 16:1 slow movement, -4 = 1:4 fast movement, etc.)
	INT16 skybox_scaley;				///< Skybox Y axis scale.
	INT16 skybox_scalez;				///< Skybox Z axis scale.

	fixed_t darkness;					///< Pohbee darkness multiplier

	// Distance information
	fixed_t mobj_scale;					///< Defines the size all object calculations are relative to
	fixed_t default_waypoint_radius;	///< 0 is a special value for DEFAULT_WAYPOINT_RADIUS, but scaled with mobjscale

	// Visual information
	UINT16 palette;						///< PAL lump to use on this map
	UINT16 encorepal;					///< PAL for encore mode

	mapheader_lighting_t lighting;			///< Wall and sprite lighting
	mapheader_lighting_t lighting_encore;	///< Alternative lighting for Encore mode
	boolean use_encore_lighting;			///< Whether to use separate Encore lighting

	fixed_t cameraHeight;					///< Player camera height to use on this map

	// Audience information
	UINT8 numFollowers;					///< Internal. For audience support.
	INT16 *followers;					///< List of audience followers in this level. Allocated dynamically for space reasons. Be careful.

	// Script information
	char runsoc[33];					///< SOC to execute at start of level (32 character limit instead of 63)
	char scriptname[33];				///< Script to use when the map is switched to. (32 character limit instead of 191)

	// Cutscene information
	UINT8 precutscenenum;				///< Cutscene number to play BEFORE a level starts.
	UINT8 cutscenenum;					///< Cutscene number to use, 0 for none.

	mobjtype_t destroyforchallenge[MAXDESTRUCTIBLES];	///< Assistive for UCRP_MAPDESTROYOBJECTS
	UINT8 destroyforchallenge_size;						///< Number for above

	UINT16 cache_maplock;				///< Cached Unlockable ID

	// Lua information
	UINT8 numCustomOptions;				///< Internal. For Lua custom value support.
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
extern INT32 nummapheaders, basenummapheaders, mapallocsize;

struct unloaded_mapheader_t
{
	char *lumpname;
	UINT32 lumpnamehash;

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
	UINT32 rules;
	UINT32 tol;
	UINT8 intermission;
	SINT8 speed;
	INT32 pointlimit;
	INT32 timelimit;
	char gppic[9];
	char gppicmini[9];
};

extern gametype_t *gametypes[MAXGAMETYPES+1];
extern INT16 numgametypes;

extern INT16 gametype, g_lastgametype;

// Gametype rules
enum GameTypeRules
{
	// Race rules
	GTR_CIRCUIT				= 1,		// Enables the finish line, laps, and the waypoint system.
	GTR_BOTS				= 1<<1,		// Allows bots in this gametype. Combine with BotTiccmd hooks to make bots support your gametype.

	// Battle gametype rules
	GTR_BUMPERS				= 1<<2,		// Enables the bumper health system
	GTR_SPHERES				= 1<<3,		// Replaces rings with blue spheres
	GTR_CLOSERPLAYERS		= 1<<4,		// Buffs spindash and draft power to bring everyone together, nerfs invincibility and grow to prevent excessive combos

	GTR_BATTLESTARTS		= 1<<5,		// Use Battle Mode start positions.
	GTR_PAPERITEMS			= 1<<6,		// Replaces item boxes with paper item spawners
	GTR_POWERSTONES			= 1<<7,		// Battle Emerald collectables.
	GTR_KARMA				= 1<<8,		// Enables the Karma system if you're out of bumpers
	// 1<<9 - UNUSED

	// Bonus gametype rules
	GTR_CHECKPOINTS			= 1<<10,	// Player respawns at specific checkpoints
	GTR_PRISONS				= 1<<11,	// Can enter Prison Break mode
	GTR_CATCHER				= 1<<12,	// UFO Catcher (only works with GTR_CIRCUIT)
	GTR_ROLLINGSTART		= 1<<13,	// Rolling start (only works with GTR_CIRCUIT)
	GTR_SPECIALSTART		= 1<<14,	// White fade instant start
	GTR_BOSS				= 1<<15,	// Boss intro and spawning

	// General purpose rules
	GTR_POINTLIMIT			= 1<<16,	// Reaching point limit ends the round
	GTR_TIMELIMIT			= 1<<17,	// Reaching time limit ends the round
	GTR_OVERTIME			= 1<<18,	// Allow overtime behavior
	GTR_ENCORE				= 1<<19,	// Alternate Encore mirroring, scripting, and texture remapping

	GTR_TEAMS				= 1<<20,	// Teams are forced on
	GTR_NOTEAMS				= 1<<21,	// Teams are forced off
	GTR_TEAMSTARTS			= 1<<22,	// Use team-based start positions

	GTR_NOMP				= 1<<23,	// No multiplayer
	GTR_NOCUPSELECT			= 1<<24,	// Your maps are not selected via cup.
	GTR_NOPOSITION			= 1<<25,	// No POSITION

	// free: to and including 1<<31
};
// Remember to update GAMETYPERULE_LIST in deh_soc.c

#define GTR_FORBIDMP (GTR_NOMP|GTR_CATCHER|GTR_BOSS)

// TODO: replace every instance
#define gametyperules (gametypes[gametype]->rules)

// TypeOfLevel things
enum TypeOfLevel
{
	// Gametypes
	TOL_RACE	 = 0x0001, ///< Race
	TOL_BATTLE	 = 0x0002, ///< Battle
	TOL_SPECIAL	 = 0x0004, ///< Special Stage (variant of race, but forbidden)
	TOL_VERSUS	 = 0x0008, ///< Versus (variant of battle, but forbidden)
	TOL_TUTORIAL = 0x0010, ///< Tutorial (variant of race, but forbidden)

	// Modifiers
	TOL_TV		= 0x0100 ///< Midnight Channel specific: draw TV like overlay on HUD
};
// Make sure to update TYPEOFLEVEL too

#define MAXTOL             (1<<31)
#define NUMBASETOLNAMES    (5)
#define NUMTOLNAMES        (NUMBASETOLNAMES + NUMGAMETYPEFREESLOTS)

struct tolinfo_t
{
	const char *name;
	UINT32 flag;
};
extern tolinfo_t TYPEOFLEVEL[NUMTOLNAMES];
extern UINT32 lastcustomtol;

extern UINT8 stagefailed;

// Emeralds stored as bits to throw savegame hackers off.
typedef enum
{
	EMERALD_CHAOS1 = 1,
	EMERALD_CHAOS2 = 1<<1,
	EMERALD_CHAOS3 = 1<<2,
	EMERALD_CHAOS4 = 1<<3,
	EMERALD_CHAOS5 = 1<<4,
	EMERALD_CHAOS6 = 1<<5,
	EMERALD_CHAOS7 = 1<<6,
	EMERALD_ALLCHAOS = EMERALD_CHAOS1|EMERALD_CHAOS2|EMERALD_CHAOS3|EMERALD_CHAOS4|EMERALD_CHAOS5|EMERALD_CHAOS6|EMERALD_CHAOS7,

	EMERALD_SUPER1 = 1<<7,
	EMERALD_SUPER2 = 1<<8,
	EMERALD_SUPER3 = 1<<9,
	EMERALD_SUPER4 = 1<<10,
	EMERALD_SUPER5 = 1<<11,
	EMERALD_SUPER6 = 1<<12,
	EMERALD_SUPER7 = 1<<13,
	EMERALD_ALLSUPER = EMERALD_SUPER1|EMERALD_SUPER2|EMERALD_SUPER3|EMERALD_SUPER4|EMERALD_SUPER5|EMERALD_SUPER6|EMERALD_SUPER7,

	EMERALD_ALL = EMERALD_ALLCHAOS|EMERALD_ALLSUPER
} emeraldflags_t;

#define ALLCHAOSEMERALDS(v) ((v & EMERALD_ALLCHAOS) == EMERALD_ALLCHAOS)
#define ALLSUPEREMERALDS(v) ((v & EMERALD_ALLSUPER) == EMERALD_ALLSUPER)
#define ALLEMERALDS(v) ((v & EMERALD_ALL) == EMERALD_ALL)

#define NUM_LUABANKS 16 // please only make this number go up between versions, never down. you'll break saves otherwise. also, must fit in UINT8
extern INT32 luabanks[NUM_LUABANKS];

extern INT32 nummaprings; //keep track of spawned rings/coins

extern UINT8 nummapspraycans;
extern UINT16 numchallengedestructibles;

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
	UINT32 chat_color;
};

extern teaminfo_t g_teaminfo[TEAM__MAX];
extern UINT32 g_teamscores[TEAM__MAX];

// Eliminates unnecessary searching.
extern boolean CheckForBustableBlocks;
extern boolean CheckForBouncySector;
extern boolean CheckForQuicksand;
extern boolean CheckForMarioBlocks;
extern boolean CheckForFloatBob;
extern boolean CheckForReverseGravity;

// Powerup durations
extern UINT16 invulntics;
extern UINT16 sneakertics;
extern UINT16 flashingtics;
extern UINT16 tailsflytics;
extern UINT16 underwatertics;
extern UINT16 spacetimetics;
extern UINT16 extralifetics;
extern UINT16 nightslinktics;

// SRB2kart
extern tic_t introtime;
extern tic_t starttime;

extern const tic_t bulbtime;
extern UINT8 numbulbs;

extern INT32 hyudorotime;
extern INT32 stealtime;
extern INT32 sneakertime;
extern INT32 itemtime;
extern INT32 bubbletime;
extern INT32 comebacktime;
extern INT32 bumptime;
extern INT32 ebraketime;
extern INT32 greasetics;
extern INT32 wipeoutslowtime;
extern INT32 wantedreduce;
extern INT32 wantedfrequency;

extern UINT8 introtoplay;
extern UINT8 g_credits_cutscene;
extern UINT8 useSeal;

extern UINT8 use1upSound;
extern UINT8 maxXtraLife; // Max extra lives from rings

struct exitcondition_t
{
	boolean losing;
	boolean retry;
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
	boolean ticked;
} g_musicfade;

#define DEFAULT_GRAVITY (4*FRACUNIT/5)
extern fixed_t gravity;
extern fixed_t mapobjectscale;

extern struct maplighting
{
	UINT8 contrast;
	SINT8 backlight;
	boolean directional;
	angle_t angle;
} maplighting;

// SRB2kart
extern UINT8 numlaps;
extern UINT8 gamespeed;
extern boolean franticitems;
extern boolean encoremode, prevencoremode;
extern boolean g_teamplay;
extern boolean g_duelpermitted;

extern tic_t wantedcalcdelay;
extern tic_t itemCooldowns[NUMKARTITEMS - 1];
extern tic_t mapreset;
extern boolean thwompsactive;
extern UINT8 lastLowestLap;
extern SINT8 spbplace;
extern boolean rainbowstartavailable;
extern tic_t attacktimingstarted;
extern boolean inDuel;
extern UINT8 overtimecheckpoints;

extern tic_t bombflashtimer;	// Used to avoid causing seizures if multiple mines explode close to you :)
extern boolean legitimateexit;
extern boolean comebackshowninfo;

#define VOTE_NUM_LEVELS (4)
#define VOTE_NOT_PICKED (-1)
#define VOTE_SPECIAL (MAXPLAYERS)
#define VOTE_TOTAL (MAXPLAYERS+1)

#define VOTE_TIMEOUT_LOSER (MAXPLAYERS+1) // not a real vote ID
#define VOTE_TIMEOUT_WINNER (MAXPLAYERS+2) // ditto

extern UINT16 g_voteLevels[VOTE_NUM_LEVELS][2];
extern SINT8 g_votes[VOTE_TOTAL];
extern SINT8 g_pickedVote;
extern boolean g_votes_striked[VOTE_NUM_LEVELS];

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
extern UINT16 numtubewaypoints[NUMTUBEWAYPOINTSEQUENCES];

void P_AddTubeWaypoint(UINT8 sequence, UINT8 id, mobj_t *waypoint);
mobj_t *P_GetFirstTubeWaypoint(UINT8 sequence);
mobj_t *P_GetLastTubeWaypoint(UINT8 sequence);
mobj_t *P_GetPreviousTubeWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetNextTubeWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetClosestTubeWaypoint(UINT8 sequence, mobj_t *mo);
boolean P_IsDegeneratedTubeWaypointSequence(UINT8 sequence);

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
extern INT32 debugload;
#endif

// if true, load all graphics at level load
extern boolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;
extern INT16 wipetypepre;
extern INT16 wipetypepost;

// debug flag to cancel adaptiveness
extern boolean g_singletics;
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
extern INT32 serverplayer;
extern INT32 adminplayers[MAXPLAYERS];

/// \note put these in d_clisrv outright?

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__DOOMSTAT__
