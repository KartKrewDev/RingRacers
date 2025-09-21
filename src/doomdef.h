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
/// \file  doomdef.h
/// \brief Internally used data structures for virtually everything,
///        key definitions, lots of other stuff.

#ifndef __DOOMDEF__
#define __DOOMDEF__

#ifdef _WIN32
#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4127 4152 4213 4514)
#ifdef _WIN64
#pragma warning(disable : 4306)
#endif
#endif
// warning level 4
// warning C4127: conditional expression is constant
// warning C4152: nonstandard extension, function/data pointer conversion in expression
// warning C4213: nonstandard extension used : cast on l-value


#include "version.h"
#include "doomtype.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES // fixes M_PI errors in r_plane.c for Visual Studio
#include <math.h>

#ifdef GETTEXT
#include <libintl.h>
#endif
#include <locale.h> // locale should not be dependent on GETTEXT -- 11/01/20 Monster Iestyn

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef _WIN32
#include <io.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//#define NOMD5

// If you don't disable ALL debug first, you get ALL debug enabled
#if !defined (NDEBUG)
#ifndef PACKETDROP
#define PACKETDROP
#endif
#ifndef PARANOIA
#define PARANOIA
#endif
#ifndef ZDEBUG
#define ZDEBUG
#endif
#endif

// Uncheck this to compile debugging code
//#ifndef PARANOIA
//#define PARANOIA // do some tests that never fail but maybe
// turn this on by make etc.. DEBUGMODE = 1 or use the Debug profile in the VC++ projects
//#endif
#if defined (_WIN32) || (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (macintosh)
#define LOGMESSAGES // write message in log.txt
#endif

#ifdef LOGMESSAGES
extern FILE *logstream;
extern char logfilename[1024];
#endif

/* A mod name to further distinguish versions. */
#define SRB2APPLICATION "RingRacers"

//#define DEVELOP // Disable this for release builds to remove excessive cheat commands and enable MD5 checking and stuff, all in one go. :3
#ifdef DEVELOP
#ifndef PARANOIA
#define PARANOIA // On by default for DEVELOP builds
#endif
#define VERSIONSTRING "Development EXE"
#define VERSIONSTRING_RC "Development EXE" "\0"
// most interface strings are ignored in development mode.
// we use comprevision and compbranch instead.
// VERSIONSTRING_RC is for the resource-definition script used by windows builds
#else
#ifdef BETAVERSION
#define VERSIONSTRING "v" SRB2VERSION " " BETAVERSION
#define VERSIONSTRING_RC SRB2VERSION " " BETAVERSION "\0"
#else
#define VERSIONSTRING "v" SRB2VERSION
#define VERSIONSTRING_RC SRB2VERSION "\0"
#endif
// Hey! If you change this, add 1 to the MODVERSION below!
// Otherwise we can't force updates!
#endif

#define VERSIONSTRINGW WSTRING (VERSIONSTRING)

/* A custom URL protocol for server links. */
#define SERVER_URL_PROTOCOL "ringracers://"

// Does this version require an added patch file?
// Comment or uncomment this as necessary.
// #define USE_PATCH_FILE

// Use .kart extension addons
#define USE_KART

// Modification options
// If you want to take advantage of the Master Server's ability to force clients to update
// to the latest version, fill these out.  Otherwise, just comment out UPDATE_ALERT and leave
// the other options the same.

// Comment out this line to completely disable update alerts (recommended for testing, but not for release)
#if !defined(DEVELOP)
#define UPDATE_ALERT
#endif

// The string used in the alert that pops up in the event of an update being available.
// Please change to apply to your modification (we don't want everyone asking where your mod is on SRB2.org!).
#define UPDATE_ALERT_STRING \
"A new update is available for Ring Racers.\n"\
"Please visit kartkrew.org to download it.\n"\
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"

// For future use, the codebase is the version of SRB2 that the modification is based on,
// and should not be changed unless you have merged changes between versions of SRB2
// (such as 2.0.4 to 2.0.5, etc) into your working copy.
// Will always resemble the versionstring, 205 = 2.0.5, 210 = 2.1, etc.
#define CODEBASE 220

// To version config.cfg, MAJOREXECVERSION is set equal to MODVERSION automatically.
// Increment MINOREXECVERSION whenever a config change is needed that does not correspond
// to an increment in MODVERSION. This might never happen in practice.
// If MODVERSION increases, set MINOREXECVERSION to 0.
#define MAJOREXECVERSION MODVERSION
#define MINOREXECVERSION 0
// (It would have been nice to use VERSION and SUBVERSION but those are zero'd out for DEVELOP builds)

// Macros
#define GETMAJOREXECVERSION(v) (v & 0xFFFF)
#define GETMINOREXECVERSION(v) (v >> 16)
#define GETEXECVERSION(major,minor) (major + (minor << 16))
#define EXECVERSION GETEXECVERSION(MAJOREXECVERSION, MINOREXECVERSION)

// =========================================================================

// The maximum number of players, multiplayer/networking.
// NOTE: it needs more than this to increase the number of players...

#define MAXPLAYERS 16
#define PLAYERSMASK (MAXPLAYERS-1)
#define MAXPLAYERNAME 21
#define MAXSPLITSCREENPLAYERS 4 // Max number of players on a single computer
#define MAXGAMEPADS (MAXSPLITSCREENPLAYERS * 2) // Number of gamepads we'll be allowing

#define MAXSKINS 1024
#define SKINNAMESIZE 16
#define MAXSKINUNAVAILABLE 128
#define MAXAVAILABILITY (MAXSKINUNAVAILABLE / 8)

#define COLORRAMPSIZE 16
#define MAXCOLORNAME 32
#define NUMCOLORFREESLOTS 1024

// Master Server compatibility ONLY
#define MSCOMPAT_MAXPLAYERS (32)

struct skincolor_t
{
	char name[MAXCOLORNAME+1];  // Skincolor name
	UINT8 ramp[COLORRAMPSIZE];  // Colormap ramp
	UINT16 invcolor;            // Signpost color
	UINT8 invshade;             // Signpost color shade
	UINT16 chatcolor;           // Chat color
	boolean accessible;         // Accessible by the color command + setup menu
	UINT16 cache_spraycan;		// Cache for associated spraycan id
};

#define FOLLOWERCOLOR_MATCH UINT16_MAX
#define FOLLOWERCOLOR_OPPOSITE (UINT16_MAX-1)
UINT16 K_GetEffectiveFollowerColor(UINT16 followercolor, follower_t *follower, UINT16 playercolor, skin_t *playerskin);

typedef enum
{
	SKINCOLOR_NONE = 0,

	SKINCOLOR_WHITE,
	SKINCOLOR_SILVER,
	SKINCOLOR_GREY,
	SKINCOLOR_NICKEL,
	SKINCOLOR_BLACK,
	SKINCOLOR_SKUNK,
	SKINCOLOR_FAIRY,
	SKINCOLOR_POPCORN,
	SKINCOLOR_ARTICHOKE,
	SKINCOLOR_PIGEON,
	SKINCOLOR_SEPIA,
	SKINCOLOR_BEIGE,
	SKINCOLOR_CARAMEL,
	SKINCOLOR_PEACH,
	SKINCOLOR_BROWN,
	SKINCOLOR_LEATHER,

	FIRSTRAINBOWCOLOR,

	SKINCOLOR_PINK = FIRSTRAINBOWCOLOR,
	SKINCOLOR_ROSE,
	SKINCOLOR_CINNAMON,
	SKINCOLOR_RUBY,
	SKINCOLOR_RASPBERRY,
	SKINCOLOR_RED,
	SKINCOLOR_CRIMSON,
	SKINCOLOR_MAROON,
	SKINCOLOR_LEMONADE,
	SKINCOLOR_SCARLET,
	SKINCOLOR_KETCHUP,
	SKINCOLOR_DAWN,
	SKINCOLOR_SUNSLAM,
	SKINCOLOR_CREAMSICLE,
	SKINCOLOR_ORANGE,
	SKINCOLOR_ROSEWOOD,
	SKINCOLOR_TANGERINE,
	SKINCOLOR_TAN,
	SKINCOLOR_CREAM,
	SKINCOLOR_GOLD,
	SKINCOLOR_ROYAL,
	SKINCOLOR_BRONZE,
	SKINCOLOR_COPPER,
	SKINCOLOR_YELLOW,
	SKINCOLOR_MUSTARD,
	SKINCOLOR_BANANA,
	SKINCOLOR_OLIVE,
	SKINCOLOR_CROCODILE,
	SKINCOLOR_PERIDOT,
	SKINCOLOR_VOMIT,
	SKINCOLOR_GARDEN,
	SKINCOLOR_LIME,
	SKINCOLOR_HANDHELD,
	SKINCOLOR_TEA,
	SKINCOLOR_PISTACHIO,
	SKINCOLOR_MOSS,
	SKINCOLOR_CAMOUFLAGE,
	SKINCOLOR_MINT,
	SKINCOLOR_GREEN,
	SKINCOLOR_PINETREE,
	SKINCOLOR_TURTLE,
	SKINCOLOR_SWAMP,
	SKINCOLOR_DREAM,
	SKINCOLOR_PLAGUE,
	SKINCOLOR_EMERALD,
	SKINCOLOR_ALGAE,
	SKINCOLOR_AQUAMARINE,
	SKINCOLOR_TURQUOISE,
	SKINCOLOR_TEAL,
	SKINCOLOR_ROBIN,
	SKINCOLOR_CYAN,
	SKINCOLOR_JAWZ, // Oni's torment
	SKINCOLOR_CERULEAN,
	SKINCOLOR_NAVY,
	SKINCOLOR_PLATINUM,
	SKINCOLOR_SLATE,
	SKINCOLOR_STEEL,
	SKINCOLOR_THUNDER,
	SKINCOLOR_NOVA,
	SKINCOLOR_RUST,
	SKINCOLOR_WRISTWATCH,
	SKINCOLOR_JET,
	SKINCOLOR_SAPPHIRE, // sweet mother, i cannot weave - slender aphrodite has overcome me with longing for a girl
	SKINCOLOR_ULTRAMARINE,
	SKINCOLOR_PERIWINKLE,
	SKINCOLOR_BLUE,
	SKINCOLOR_MIDNIGHT,
	SKINCOLOR_BLUEBERRY,
	SKINCOLOR_THISTLE,
	SKINCOLOR_PURPLE,
	SKINCOLOR_PASTEL,
	SKINCOLOR_MOONSET,
	SKINCOLOR_DUSK,
	SKINCOLOR_VIOLET,
	SKINCOLOR_MAGENTA,
	SKINCOLOR_FUCHSIA,
	SKINCOLOR_TOXIC,
	SKINCOLOR_MAUVE,
	SKINCOLOR_LAVENDER,
	SKINCOLOR_BYZANTIUM,
	SKINCOLOR_POMEGRANATE,
	SKINCOLOR_LILAC,
	SKINCOLOR_BLOSSOM,
	SKINCOLOR_TAFFY,

	FIRSTSUPERCOLOR,

	// Super special awesome Super flashing colors!
	SKINCOLOR_SUPERSILVER1 = FIRSTSUPERCOLOR,
	SKINCOLOR_SUPERSILVER2,
	SKINCOLOR_SUPERSILVER3,
	SKINCOLOR_SUPERSILVER4,
	SKINCOLOR_SUPERSILVER5,

	SKINCOLOR_SUPERRED1,
	SKINCOLOR_SUPERRED2,
	SKINCOLOR_SUPERRED3,
	SKINCOLOR_SUPERRED4,
	SKINCOLOR_SUPERRED5,

	SKINCOLOR_SUPERORANGE1,
	SKINCOLOR_SUPERORANGE2,
	SKINCOLOR_SUPERORANGE3,
	SKINCOLOR_SUPERORANGE4,
	SKINCOLOR_SUPERORANGE5,

	SKINCOLOR_SUPERGOLD1,
	SKINCOLOR_SUPERGOLD2,
	SKINCOLOR_SUPERGOLD3,
	SKINCOLOR_SUPERGOLD4,
	SKINCOLOR_SUPERGOLD5,

	SKINCOLOR_SUPERPERIDOT1,
	SKINCOLOR_SUPERPERIDOT2,
	SKINCOLOR_SUPERPERIDOT3,
	SKINCOLOR_SUPERPERIDOT4,
	SKINCOLOR_SUPERPERIDOT5,

	SKINCOLOR_SUPERSKY1,
	SKINCOLOR_SUPERSKY2,
	SKINCOLOR_SUPERSKY3,
	SKINCOLOR_SUPERSKY4,
	SKINCOLOR_SUPERSKY5,

	SKINCOLOR_SUPERPURPLE1,
	SKINCOLOR_SUPERPURPLE2,
	SKINCOLOR_SUPERPURPLE3,
	SKINCOLOR_SUPERPURPLE4,
	SKINCOLOR_SUPERPURPLE5,

	SKINCOLOR_SUPERRUST1,
	SKINCOLOR_SUPERRUST2,
	SKINCOLOR_SUPERRUST3,
	SKINCOLOR_SUPERRUST4,
	SKINCOLOR_SUPERRUST5,

	SKINCOLOR_SUPERTAN1,
	SKINCOLOR_SUPERTAN2,
	SKINCOLOR_SUPERTAN3,
	SKINCOLOR_SUPERTAN4,
	SKINCOLOR_SUPERTAN5,

	SKINCOLOR_CHAOSEMERALD1,
	SKINCOLOR_CHAOSEMERALD2,
	SKINCOLOR_CHAOSEMERALD3,
	SKINCOLOR_CHAOSEMERALD4,
	SKINCOLOR_CHAOSEMERALD5,
	SKINCOLOR_CHAOSEMERALD6,
	SKINCOLOR_CHAOSEMERALD7,

	SKINCOLOR_INVINCFLASH,
	SKINCOLOR_POSNUM,
	SKINCOLOR_POSNUM_WIN1,
	SKINCOLOR_POSNUM_WIN2,
	SKINCOLOR_POSNUM_WIN3,
	SKINCOLOR_POSNUM_LOSE1,
	SKINCOLOR_POSNUM_LOSE2,
	SKINCOLOR_POSNUM_LOSE3,
	SKINCOLOR_POSNUM_BEST1,
	SKINCOLOR_POSNUM_BEST2,
	SKINCOLOR_POSNUM_BEST3,
	SKINCOLOR_POSNUM_BEST4,
	SKINCOLOR_POSNUM_BEST5,
	SKINCOLOR_POSNUM_BEST6,

	SKINCOLOR_INTERMISSION1,
	SKINCOLOR_INTERMISSION2,
	SKINCOLOR_INTERMISSION3,

	SKINCOLOR_FIRSTFREESLOT,
	SKINCOLOR_LASTFREESLOT = SKINCOLOR_FIRSTFREESLOT + NUMCOLORFREESLOTS - 1,

	MAXSKINCOLORS,

	NUMSUPERCOLORS = ((SKINCOLOR_FIRSTFREESLOT - FIRSTSUPERCOLOR)/5)
} skincolornum_t;

extern UINT16 numskincolors;

extern skincolor_t skincolors[MAXSKINCOLORS];

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define TICRATE 35
#define NEWTICRATERATIO 1 // try 4 for 140 fps :)
#define NEWTICRATE (TICRATE*NEWTICRATERATIO)

#define MUSICRATE 1000 // sound timing is calculated by milliseconds

#define RING_DIST 1280*FRACUNIT // how close you need to be to a ring to attract it

#define PUSHACCEL (2*FRACUNIT) // Acceleration for MF2_SLIDEPUSH items.

// Special linedef executor tag numbers! Binary map format only (UDMF has other ways of doing these things).
enum {
	LE_PINCHPHASE      =    -2, // A boss entered pinch phase (and, in most cases, is preparing their pinch phase attack!)
	LE_ALLBOSSESDEAD   =    -3, // All bosses in the map are dead (Egg capsule raise)
	LE_BOSSDEAD        =    -4, // A boss in the map died (Chaos mode boss tally)
	LE_BOSS4DROP       =    -5, // CEZ boss dropped its cage (also subtract the number of hitpoints it's lost)
	LE_BRAKVILEATACK   =    -6, // Brak's doing his LOS attack, oh noes
	LE_TURRET          = 32000, // THZ turret
	LE_BRAKPLATFORM    =  4200, // v2.0 Black Eggman destroys platform
	LE_CAPSULE2        =   682, // Egg Capsule
	LE_CAPSULE1        =   681, // Egg Capsule
	LE_CAPSULE0        =   680, // Egg Capsule
	LE_KOOPA           =   650, // Distant cousin to Gay Bowser
	LE_AXE             =   649, // MKB Axe object
	LE_PARAMWIDTH      =  -100  // If an object that calls LinedefExecute has a nonzero parameter value, this times the parameter will be subtracted. (Mostly for the purpose of coexisting bosses...)
};

// Name of local directory for config files and savegames
#if (((defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON)) && !defined (__CYGWIN__)) && !defined (__APPLE__)
#define DEFAULTDIR ".ringracers"
#else
#define DEFAULTDIR "ringracers"
#endif

#include "g_state.h"

// commonly used routines - moved here for include convenience

/**	\brief	The I_Error function

	\param	error	the error message

	\return	void


*/
FUNCIERROR void ATTRNORETURN I_Error(const char *error, ...);

/**	\brief	write a message to stderr (use before I_Quit) for when you need to quit with a msg, but need
 the return code 0 of I_Quit();

	\param	error	message string

	\return	void
*/
void I_OutputMsg(const char *error, ...) FUNCPRINTF;

// console.h
typedef enum
{
	CONS_NOTICE,
	CONS_WARNING,
	CONS_ERROR
} alerttype_t;

void CONS_Printf(const char *fmt, ...) FUNCPRINTF;
void CONS_Alert(alerttype_t level, const char *fmt, ...) FUNCDEBUG;
void CONS_Debug(UINT32 debugflags, const char *fmt, ...) FUNCDEBUG;

// For help debugging functions.
#define POTENTIALLYUNUSED CONS_Alert(CONS_WARNING, "(%s:%d) Unused code appears to be used.\n", __FILE__, __LINE__)

#include "m_swap.h"

// Things that used to be in dstrings.h
#define SAVEGAMENAME "ringsav"
extern char savegamename[256];
extern char gpbackup[256];

// m_misc.h
#ifdef GETTEXT
#define M_GetText(String) gettext(String)
#else
// If no translations are to be used, make a stub
// M_GetText function that just returns the string.
#define M_GetText(x) (x)
#endif
void M_StartupLocale(void);
void *M_Memcpy(void* dest, const void* src, size_t n);
char *va(const char *format, ...) FUNCPRINTF;

char *M_GetToken(const char *inputString);
void M_UnGetToken(void);
UINT32 M_GetTokenPos(void);

void M_TokenizerOpen(const char *inputString, size_t inputLength);
void M_TokenizerClose(void);
const char *M_TokenizerRead(UINT32 i);
UINT32 M_TokenizerGetEndPos(void);
void M_TokenizerSetEndPos(UINT32 newPos);
boolean M_TokenizerJustReadString(void);

char *sizeu1(size_t num);
char *sizeu2(size_t num);
char *sizeu3(size_t num);
char *sizeu4(size_t num);
char *sizeu5(size_t num);

// d_main.c
extern int    VERSION;
extern int SUBVERSION;

// 4 bytes handles 8 characters of a git object SHA. At
// around 20k commits, we only need 6 characters for a unique
// abbreviation. Maybe in another 20k commits, more than 8
// characters will be required! =P
// P.S. 8 is also what comptime generates
#define GIT_SHA_ABBREV (4)
extern UINT8 comprevision_abbrev_bin[GIT_SHA_ABBREV];

extern boolean devparm; // development mode (-debug)

// m_cheat.c
extern UINT32 cht_debug;

typedef enum
{
	DBG_NONE			= 0x00000000,
	DBG_BASIC			= 0x00000001,
	DBG_DETAILED		= 0x00000002,
	DBG_PLAYER			= 0x00000004,
	DBG_RENDER			= 0x00000008,
	DBG_MUSIC			= 0x00000010,
	DBG_PWRLV			= 0x00000020,
	DBG_POLYOBJ			= 0x00000040,
	DBG_GAMELOGIC		= 0x00000080,
	DBG_NETPLAY			= 0x00000100,
	DBG_MEMORY			= 0x00000200,
	DBG_SETUP			= 0x00000400,
	DBG_LUA				= 0x00000800,
	DBG_RNG				= 0x00001000,
	DBG_DEMO			= 0x00002000,
	DBG_TEAMS			= 0x00004000,
} debugFlags_t;

struct debugFlagNames_s
{
	const char *str;
	debugFlags_t flag;
};

extern struct debugFlagNames_s const debug_flag_names[];

// =======================
// Misc stuff for later...
// =======================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define ANG2RAD(angle) ((float)((angle)*M_PI)/ANGLE_180)

// Modifier key variables, accessible anywhere
extern UINT8 shiftdown, ctrldown, altdown;
extern boolean capslock;

// WARNING: a should be unsigned but to add with 2048, it isn't!
#define AIMINGTODY(a) FixedDiv((FINETANGENT((2048+(((INT32)a)>>ANGLETOFINESHIFT)) & FINEMASK)*160), fovtan[viewssnum])

// if we ever make our alloc stuff...
#define ZZ_Alloc(x) Z_Malloc(x, PU_STATIC, NULL)
#define ZZ_Calloc(x) Z_Calloc(x, PU_STATIC, NULL)

// i_system.c, replace getchar() once the keyboard has been appropriated
INT32 I_GetKey(void);

/* http://www.cse.yorku.ca/~oz/hash.html */
static inline
UINT32 quickncasehash (const char *p, size_t n)
{
	size_t i = 0;
	UINT32 x = 5381;

	while (i < n && p[i])
	{
		x = (x * 33) ^ tolower(p[i]);
		i++;
	}

	return x;
}

// m_cond, doomstat
typedef enum {
	GDGT_RACE,
	GDGT_BATTLE,
	GDGT_PRISONS,
	GDGT_SPECIAL,
	GDGT_CUSTOM,
	GDGT_MAX
} roundsplayed_t;

#ifndef __cplusplus
#ifndef min // Double-Check with WATTCP-32's cdefs.h
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max // Double-Check with WATTCP-32's cdefs.h
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef clamp
#define clamp(x, y, z) (((x) < (y)) ? (y) : (((x) > (z)) ? (z) : (x)))
#endif
#endif

// Max gamepad/joysticks that can be detected/used.
#define MAX_JOYSTICKS 4

#ifndef M_PIl
#define M_PIl 3.1415926535897932384626433832795029L
#endif

// Floating point comparison epsilons from float.h
#ifndef FLT_EPSILON
#define FLT_EPSILON 1.1920928955078125e-7f
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16l
#endif

// An assert-type mechanism.
// NOTE: USE SRB2_ASSERT FOR C++ CODE INSTEAD
#ifdef PARANOIA
#define I_Assert(e) ((e) ? (void)0 : I_Error("assert failed: %s, file %s, line %d", #e, __FILE__, __LINE__))
#else
#define I_Assert(e) ((void)0)
#endif

// The character that separates pathnames. Forward slash on
// most systems, but reverse solidus (\) on Windows.
#if defined (_WIN32)
	#define PATHSEP "\\"
#else
	#define PATHSEP "/"
#endif

#define PUNCTUATION "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"

// Compile date and time and revision.
extern const char
	*compdate,
	*comptime,
	*comprevision,
	*compbranch,
	*compnote,
	*comptype;
extern int
	compuncommitted,
	compoptimized;

// Disabled code and code under testing
// None of these that are disabled in the normal build are guaranteed to work perfectly
// Compile them at your own risk!

///	Who put weights on my recycler?  ... Inuyasha did.
///	\note	XMOD port.
//#define WEIGHTEDRECYCLER

///	Allow loading of savegames between different versions of the game.
///	\note	XMOD port.
///	    	Most modifications should probably enable this.
//#define SAVEGAME_OTHERVERSIONS

///	Shuffle's incomplete OpenGL sorting code.
#define SHUFFLE // This has nothing to do with sorting, why was it disabled?

///	Allow the use of the SOC RESETINFO command.
///	\note	Builds that are tight on memory should disable this.
///	    	This stops the game from storing backups of the states, sprites, and mobjinfo tables.
///	    	Though this info is compressed under normal circumstances, it's still a lot of extra
///	    	memory that never gets touched.
#define ALLOW_RESETDATA

/// Experimental tweaks to analog mode. (Needs a lot of work before it's ready for primetime.)
//#define REDSANALOG

/// Experimental attempts at preventing MF_PAPERCOLLISION objects from getting stuck in walls.
//#define PAPER_COLLISIONCORRECTION

#ifdef DEVELOP
// Easily make it so that overtime works offline
#define TESTOVERTIMEINFREEPLAY
#endif

/// OpenGL shaders
#define GL_SHADERS


/// Handle touching sector specials in P_PlayerAfterThink instead of P_PlayerThink.
/// \note   Required for proper collision with moving sloped surfaces that have sector specials on them.
#define SECTORSPECIALSAFTERTHINK

/// Sprite rotation
#define ROTSPRITE
#define ROTANGLES 72 // Needs to be a divisor of 360 (45, 60, 90, 120...)
#define ROTANGDIFF (360 / ROTANGLES)

/// PNG support
#ifndef HAVE_PNG
#define NO_PNG_LUMPS
#endif

/// Render flats on walls
#define WALLFLATS

// Crypto/RRID primitives
#define PUBKEYLENGTH 32 // Enforced by Monocypher EdDSA
#define PRIVKEYLENGTH 64 // Enforced by Monocypher EdDSA
#define SIGNATURELENGTH 64 // Enforced by Monocypher EdDSA
#define CHALLENGELENGTH 64 // Servers verify client identity by giving them messages to sign. How long are these messages?

#ifdef HAVE_CURL
#define MASTERSERVER
#else
#undef UPDATE_ALERT
#endif

#ifdef HAVE_CURL
#define MASTERSERVER
#else
#undef UPDATE_ALERT
#endif

// p_sight.c
#define TRAVERSE_MAX 8

/// Other karma comeback modes
//#define OTHERKARMAMODES

// Amp scaling
#define MAXAMPSCALINGDIST 18000

// Exp
#define EXP_STABLERATE 3*FRACUNIT/10 // how low is your placement before losing XP? 4*FRACUNIT/10 = top 40% of race will gain
#define EXP_POWER 3*FRACUNIT/100 // adjust to change overall xp volatility
#define EXP_MIN 25 // The min value target
#define EXP_TARGET 150 // Used for grading ...
#define EXP_MAX 150 // The max value displayed by the hud and in the tally screen and GP results screen

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __DOOMDEF__
