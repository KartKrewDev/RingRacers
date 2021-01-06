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
/// \file  d_player.h
/// \brief player data structures

#ifndef __D_PLAYER__
#define __D_PLAYER__

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "p_pspr.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

// the player struct stores a waypoint for racing
#include "k_waypoint.h"

// Extra abilities/settings for skins (combinable stuff)
typedef enum
{
	SF_HIRES            = 1, // Draw the sprite at different size?
	SF_MACHINE          = 1<<1, // Beep boop. Are you a robot?
	// free up to and including 1<<31
} skinflags_t;

//
// Player states.
//
typedef enum
{
	// Playing or camping.
	PST_LIVE,
	// Dead on the ground, view follows killer.
	PST_DEAD,
	// Ready to restart/respawn???
	PST_REBORN
} playerstate_t;

//
// Player internal flags
//
typedef enum
{
	PF_FAULT = 1,

	// Cheats
	PF_GODMODE = 1<<4,
	PF_NOCLIP  = 1<<5,
	PF_INVIS   = 1<<6,

	// True if button down last tic.
	PF_ATTACKDOWN = 1<<7,
	PF_SPINDOWN   = 1<<8,
	PF_JUMPDOWN   = 1<<9,
	PF_WPNDOWN    = 1<<10,

	// Unmoving states
	PF_STASIS     = 1<<11, // Player is not allowed to move
	PF_JUMPSTASIS = 1<<12, // and that includes jumping.
	PF_FULLSTASIS = PF_STASIS|PF_JUMPSTASIS,

	// SRB2Kart: Spectator that wants to join
	PF_WANTSTOJOIN = 1<<13,

	// Character action status
	PF_STARTJUMP     = 1<<14,
	PF_JUMPED        = 1<<15,
	PF_NOJUMPDAMAGE  = 1<<16,

	PF_SPINNING      = 1<<17,
	PF_STARTDASH     = 1<<18,

	PF_THOKKED       = 1<<19,
	PF_SHIELDABILITY = 1<<20,
	PF_GLIDING       = 1<<21,
	PF_BOUNCING      = 1<<22,

	// Sliding (usually in water) like Labyrinth/Oil Ocean
	PF_SLIDING       = 1<<23,

	// NiGHTS stuff
	PF_TRANSFERTOCLOSEST = 1<<24,
	PF_DRILLING          = 1<<25,

	// Gametype-specific stuff
	PF_GAMETYPEOVER = 1<<26, // Race time over, or H&S out-of-game
	PF_TAGIT        = 1<<27, // The player is it! For Tag Mode

	/*** misc ***/
	PF_FORCESTRAFE = 1<<28, // Turning inputs are translated into strafing inputs
	PF_CANCARRY    = 1<<29, // Can carry another player?
	PF_HITFINISHLINE     = 1<<30, // Already hit the finish line this tic

	// up to 1<<31 is free
} pflags_t;

typedef enum
{
	// Are animation frames playing?
	PA_ETC=0,
	PA_IDLE,
	PA_EDGE,
	PA_WALK,
	PA_RUN,
	PA_DASH,
	PA_PAIN,
	PA_ROLL,
	PA_JUMP,
	PA_SPRING,
	PA_FALL,
	PA_ABILITY,
	PA_ABILITY2,
	PA_RIDE
} panim_t;

//
// All of the base srb2 shields are either a single constant,
// or use damagetype-protecting flags applied to a constant,
// or are the force shield (which does everything weirdly).
//
// Base flags by themselves aren't used so modders can make
// abstract, ability-less shields should they so choose.
//
typedef enum
{
	SH_NONE = 0,

	// Shield flags
	SH_PROTECTFIRE = 0x400,
	SH_PROTECTWATER = 0x800,
	SH_PROTECTELECTRIC = 0x1000,
	SH_PROTECTSPIKE = 0x2000, // cactus shield one day? thanks, subarashii
	//SH_PROTECTNUKE = 0x4000, // intentionally no hardcoded defense against nukes

	// Indivisible shields
	SH_PITY = 1, // the world's most basic shield ever, given to players who suck at Match
	SH_WHIRLWIND,
	SH_ARMAGEDDON,
	SH_PINK, // PITY IN PINK!

	// Normal shields that use flags
	SH_ATTRACT = SH_PITY|SH_PROTECTELECTRIC,
	SH_ELEMENTAL = SH_PITY|SH_PROTECTFIRE|SH_PROTECTWATER,

	// Sonic 3 shields
	SH_FLAMEAURA = SH_PITY|SH_PROTECTFIRE,
	SH_BUBBLEWRAP = SH_PITY|SH_PROTECTWATER,
	SH_THUNDERCOIN = SH_WHIRLWIND|SH_PROTECTELECTRIC,

	// The force shield uses the lower 8 bits to count how many extra hits are left.
	SH_FORCE = 0x100,
	SH_FORCEHP = 0xFF, // to be used as a bitmask only

	// Mostly for use with Mario mode.
	SH_FIREFLOWER = 0x200,

	SH_STACK = SH_FIREFLOWER, // second-layer shields
	SH_NOSTACK = ~SH_STACK
} shieldtype_t; // pw_shield

typedef enum
{
	CR_NONE = 0,
	// Specific level gimmicks.
	CR_ZOOMTUBE,
} carrytype_t; // pw_carry

// Player powers. (don't edit this comment)
typedef enum
{
	pw_invulnerability,
	pw_sneakers,
	pw_flashing,
	pw_shield,
	pw_carry,
	pw_tailsfly, // tails flying
	pw_underwater, // underwater timer
	pw_spacetime, // In space, no one can hear you spin!
	pw_extralife, // Extra Life timer
	pw_pushing,
	pw_justsprung,
	pw_noautobrake,

	pw_super, // Are you super?
	pw_gravityboots, // gravity boots

	// Weapon ammunition
	pw_infinityring,
	pw_automaticring,
	pw_bouncering,
	pw_scatterring,
	pw_grenadering,
	pw_explosionring,
	pw_railring,

	// Power Stones
	pw_emeralds, // stored like global 'emeralds' variable

	// NiGHTS powerups
	pw_nights_superloop,
	pw_nights_helper,
	pw_nights_linkfreeze,

	pw_nocontrol, //for linedef exec 427

	pw_dye, // for dyes

	pw_justlaunched, // Launched off a slope this tic (0=none, 1=standard launch, 2=half-pipe launch)

	pw_ignorelatch, // Don't grab onto CR_GENERIC, add 32768 (powers[pw_ignorelatch] & 1<<15) to avoid ALL not-NiGHTS CR_ types

	NUMPOWERS
} powertype_t;

/*
To use: #define FOREACH( name, number )
Do with it whatever you want.
Run this macro, then #undef FOREACH afterward
*/
#define KART_ITEM_ITERATOR \
	FOREACH (SAD,           -1),\
	FOREACH (NONE,           0),\
	FOREACH (SNEAKER,        1),\
	FOREACH (ROCKETSNEAKER,  2),\
	FOREACH (INVINCIBILITY,  3),\
	FOREACH (BANANA,         4),\
	FOREACH (EGGMAN,         5),\
	FOREACH (ORBINAUT,       6),\
	FOREACH (JAWZ,           7),\
	FOREACH (MINE,           8),\
	FOREACH (BALLHOG,        9),\
	FOREACH (SPB,           10),\
	FOREACH (GROW,          11),\
	FOREACH (SHRINK,        12),\
	FOREACH (THUNDERSHIELD, 13),\
	FOREACH (BUBBLESHIELD,  14),\
	FOREACH (FLAMESHIELD,   15),\
	FOREACH (HYUDORO,       16),\
	FOREACH (POGOSPRING,    17),\
	FOREACH (SUPERRING,     18),\
	FOREACH (KITCHENSINK,   19)

typedef enum
{
#define FOREACH( name, n ) KITEM_ ## name = n
	KART_ITEM_ITERATOR,
#undef  FOREACH

	NUMKARTITEMS,

	// Additional roulette numbers, only used for K_KartGetItemResult
	KRITEM_DUALSNEAKER = NUMKARTITEMS,
	KRITEM_TRIPLESNEAKER,
	KRITEM_TRIPLEBANANA,
	KRITEM_TENFOLDBANANA,
	KRITEM_TRIPLEORBINAUT,
	KRITEM_QUADORBINAUT,
	KRITEM_DUALJAWZ,

	NUMKARTRESULTS
} kartitems_t;

typedef enum
{
	KSHIELD_NONE = 0,
	KSHIELD_THUNDER = 1,
	KSHIELD_BUBBLE = 2,
	KSHIELD_FLAME = 3,
	NUMKARTSHIELDS
} kartshields_t;

typedef enum
{
	KSPIN_THRUST    = (1<<0),
	KSPIN_IFRAMES   = (1<<1),
	KSPIN_AIRTIMER  = (1<<2),

	KSPIN_TYPEBIT   = (1<<3),
	KSPIN_TYPEMASK  = ~( KSPIN_TYPEBIT - 1 ),

#define KSPIN_TYPE( type ) ( KSPIN_TYPEBIT << type )

	KSPIN_SPINOUT   = KSPIN_TYPE(0)|KSPIN_IFRAMES|KSPIN_THRUST,
	KSPIN_WIPEOUT   = KSPIN_TYPE(1)|KSPIN_IFRAMES,
	KSPIN_STUNG     = KSPIN_TYPE(2),
	KSPIN_EXPLOSION = KSPIN_TYPE(3)|KSPIN_IFRAMES|KSPIN_AIRTIMER,

#undef KSPIN_TYPE
} kartspinoutflags_t;

//{ SRB2kart - kartstuff
typedef enum
{
	// TODO: Kill this giant array. Add them as actual player_t variables, or condense related timers into their own, smaller arrays.
	// Basic gameplay things
	k_position,			// Used for Kart positions, mostly for deterministic stuff
	k_oldposition,		// Used for taunting when you pass someone
	k_positiondelay,	// Used for position number, so it can grow when passing/being passed

	k_throwdir, 		// Held dir of controls; 1 = forward, 0 = none, -1 = backward (was "player->heldDir")
	k_instashield,		// Instashield no-damage animation timer

	k_floorboost,		// Prevents Sneaker sounds for a breif duration when triggered by a floor panel
	k_spinouttype,		// Determines the mode of spinout/wipeout, see kartspinoutflags_t

	k_drift,			// Drifting Left or Right, plus a bigger counter = sharper turn
	k_driftend,			// Drift has ended, used to adjust character angle after drift
	k_driftcharge,		// Charge your drift so you can release a burst of speed
	k_driftboost,		// Boost you get from drifting
	k_boostcharge,		// Charge-up for boosting at the start of the race
	k_startboost,		// Boost you get from start of race or respawn drop dash
	k_rings,			// Number of held rings
	k_pickuprings,		// Number of rings being picked up before added to the counter (prevents rings from being deleted forever over 20)
	k_userings,			// Have to be not holding the item button to change from using rings to using items (or vice versa), to prevent some weirdness with the button
	k_ringdelay,		// 3 tic delay between every ring usage
	k_ringboost,		// Ring boost timer
	k_ringlock,			// Prevent picking up rings while SPB is locked on
	k_sparkleanim,		// Angle offset for ring sparkle animation
	k_jmp,				// In Mario Kart, letting go of the jump button stops the drift
	k_offroad,			// In Super Mario Kart, going offroad has lee-way of about 1 second before you start losing speed
	k_brakestop,		// Wait until you've made a complete stop for a few tics before letting brake go in reverse.
	k_spindash,			// Spindash charge timer
	k_spindashspeed,	// Spindash release speed
	k_spindashboost,	// Spindash release boost timer
	k_waterskip,		// Water skipping counter
	k_dashpadcooldown,	// Separate the vanilla SA-style dash pads from using pw_flashing
	k_numboosts,		// Count of how many boosts are being stacked, for after image spawning
	k_boostpower,		// Base boost value, for offroad
	k_speedboost,		// Boost value smoothing for max speed
	k_accelboost,		// Boost value smoothing for acceleration
	k_handleboost,		// Boost value smoothing for handling
	k_draftpower,		// Drafting power (from 0 to FRACUNIT), doubles your top speed & acceleration at max
	k_draftleeway,		// Leniency timer before removing draft power
	k_lastdraft,		// Last player being drafted
	k_boostangle,		// angle set when not spun out OR boosted to determine what direction you should keep going at if you're spun out and boosted.
	k_aizdriftstrat,	// Let go of your drift while boosting? Helper for the SICK STRATZ you have just unlocked
	k_brakedrift,		// Helper for brake-drift spark spawning

	k_itemroulette,		// Used for the roulette when deciding what item to give you (was "pw_kartitem")
	k_roulettetype,		// Used for the roulette, for deciding type (currently only used for Battle, to give you better items from Karma items)

	// Item held stuff
	k_itemtype,		// KITEM_ constant for item number
	k_itemamount,	// Amount of said item
	k_itemheld,		// Are you holding an item?
	k_holdready,	// Hold button-style item is ready to activate

	// Some items use timers for their duration or effects
	k_curshield,			// 0 = no shield, 1 = thunder shield
	k_hyudorotimer,			// Duration of the Hyudoro offroad effect itself
	k_stealingtimer,		// You are stealing an item, this is your timer
	k_stolentimer,			// You are being stolen from, this is your timer
	k_superring,			// Spawn rings on top of you every tic!
	k_sneakertimer,			// Duration of a Sneaker Boost (from Sneakers or level boosters)
	k_numsneakers,			// Number of stacked sneaker effects
	k_growshrinktimer,		// > 0 = Big, < 0 = small
	k_squishedtimer,		// Squished frame timer
	k_rocketsneakertimer,	// Rocket Sneaker duration timer
	k_invincibilitytimer,	// Invincibility timer
	k_bubblecool,			// Bubble Shield use cooldown
	k_bubbleblowup,			// Bubble Shield usage blowup
	k_flamedash,			// Flame Shield dash power
	k_flamemeter,			// Flame Shield dash meter left
	k_flamelength,			// Flame Shield dash meter, number of segments
	k_eggmanheld,			// Eggman monitor held, separate from k_itemheld so it doesn't stop you from getting items
	k_eggmanexplode,		// Fake item recieved, explode in a few seconds
	k_eggmanblame,			// Fake item recieved, who set this fake
	k_lastjawztarget,		// Last person you target with jawz, for playing the target switch sfx
	k_bananadrag,			// After a second of holding a banana behind you, you start to slow down
	k_spinouttimer,			// Spin-out from a banana peel or oil slick (was "pw_bananacam")
	k_wipeoutslow,			// Timer before you slowdown when getting wiped out
	k_justbumped,			// Prevent players from endlessly bumping into each other
	k_comebacktimer,		// Battle mode, how long before you become a bomb after death
	k_sadtimer,				// How long you've been sad

	// Battle Mode vars
	k_bumper,			// Number of bumpers left
	k_comebackpoints,	// Number of times you've bombed or gave an item to someone; once it's 3 it gets set back to 0 and you're given a bumper
	k_comebackmode, 	// 0 = bomb, 1 = item
	k_wanted, 			// Timer for determining WANTED status, lowers when hitting people, prevents the game turning into Camp Lazlo

	// v1.0.2+ vars
	k_getsparks,		// Disable drift sparks at low speed, JUST enough to give acceleration the actual headstart above speed
	k_jawztargetdelay,	// Delay for Jawz target switching, to make it less twitchy
	k_spectatewait,		// How long have you been waiting as a spectator
	k_tiregrease,		// Reduced friction timer after hitting a horizontal spring
	k_springstars,		// Spawn stars around a player when they hit a spring
	k_springcolor,		// Color of spring stars
	k_killfield, 		// How long have you been in the kill field, stay in too long and lose a bumper
	k_wrongway, 		// Display WRONG WAY on screen

	NUMKARTSTUFF
} kartstufftype_t;

typedef enum
{
	// Unsynced, HUD or clientsided effects
	// Item box
	khud_itemblink,		// Item flashing after roulette, prevents Hyudoro stealing AND serves as a mashing indicator
	khud_itemblinkmode,	// Type of flashing: 0 = white (normal), 1 = red (mashing), 2 = rainbow (enhanced items)

	// Rings
	khud_ringframe,		// Ring spin frame
	khud_ringtics,		// Tics left until next ring frame
	khud_ringdelay,		// Next frame's tics
	khud_ringspblock,	// Which frame of the SPB ring lock animation to use

	// Lap finish
	khud_lapanimation,	// Used to show the lap start wing logo animation
	khud_laphand,		// Lap hand gfx to use; 0 = none, 1 = :ok_hand:, 2 = :thumbs_up:, 3 = :thumps_down:

	// Start
	khud_fault,			// Set when faulting during the starting countdown

	// Camera
	khud_boostcam,		// Camera push forward on boost
	khud_destboostcam,	// Ditto
	khud_timeovercam,	// Camera timer for leaving behind or not

	// Sounds
	khud_enginesnd,		// Engine sound offset this player is using.
	khud_voices,		// Used to stop the player saying more voices than it should
	khud_tauntvoices,	// Used to specifically stop taunt voice spam

	// Battle
	khud_cardanimation,	// Used to determine the position of some full-screen Battle Mode graphics
	khud_yougotem, 		// "You Got Em" gfx when hitting someone as a karma player via a method that gets you back in the game instantly

	NUMKARTHUD
} karthudtype_t;

// QUICKLY GET RING TOTAL, INCLUDING RINGS CURRENTLY IN THE PICKUP ANIMATION
#define RINGTOTAL(p) (p->rings + p->kartstuff[k_pickuprings])

//}

// player_t struct for all respawn variables
typedef struct respawnvars_s
{
	UINT8 state; // 0: not respawning, 1: heading towards respawn point, 2: about to drop
	waypoint_t *wp; // Waypoint that we're going towards, NULL if the position isn't linked to one
	fixed_t pointx; // Respawn position coords to go towards
	fixed_t pointy;
	fixed_t pointz;
	boolean flip; // Flip upside down or not
	tic_t timer; // Time left on respawn animation once you're there
	UINT32 distanceleft; // How far along the course to respawn you
	tic_t dropdash; // Drop Dash charge timer
} respawnvars_t;

// player_t struct for all bot variables
typedef struct botvars_s
{
	UINT8 difficulty; // Bot's difficulty setting
	UINT8 diffincrease; // In GP: bot difficulty will increase this much next round
	boolean rival; // If true, they're the GP rival

	tic_t itemdelay; // Delay before using item at all
	tic_t itemconfirm; // When high enough, they will use their item

	SINT8 turnconfirm; // Confirm turn direction
} botvars_t;

// ========================================================================
//                          PLAYER STRUCTURE
// ========================================================================
typedef struct player_s
{
	mobj_t *mo;

	// Caveat: ticcmd_t is ATTRPACK! Be careful what precedes it.
	ticcmd_t cmd;

	playerstate_t playerstate;

	// Focal origin above r.z
	fixed_t viewz;
	// Base height above floor for viewz.
	fixed_t viewheight;
	// Bob/squat speed.
	fixed_t deltaviewheight;
	// bounded/scaled total momentum.
	fixed_t bob;

	angle_t viewrollangle;

	angle_t angleturn;

	// Mouse aiming, where the guy is looking at!
	// It is updated with cmd->aiming.
	angle_t aiming;

	// fun thing for player sprite
	angle_t drawangle;

	// player's ring count
	INT16 rings;
	INT16 spheres;

	// Power ups. invinc and invis are tic counters.
	UINT16 powers[NUMPOWERS];

	// SRB2kart stuff
	INT32 kartstuff[NUMKARTSTUFF];
	INT32 karthud[NUMKARTHUD];
	UINT32 distancetofinish;
	waypoint_t *nextwaypoint;
	respawnvars_t respawn; // Respawn info
	tic_t airtime; // Keep track of how long you've been in the air
	UINT8 trickpanel; // Trick panel state
	tic_t trickdelay;

	UINT8 bumpers;
	INT16 karmadelay;
	boolean eliminated;


	// Bit flags.
	// See pflags_t, above.
	pflags_t pflags;

	// playing animation.
	panim_t panim;

	// For screen flashing (bright).
	UINT16 flashcount;
	UINT16 flashpal;

	// Player skin colorshift, 0-15 for which color to draw player.
	UINT16 skincolor;

	INT32 skin;
	UINT32 availabilities;

	UINT32 score; // player score
	fixed_t dashspeed; // dashing speed

	// SRB2kart
	UINT8 kartspeed; // Kart speed stat between 1 and 9
	UINT8 kartweight; // Kart weight stat between 1 and 9

	INT32 followerskin;		// Kart: This player's follower "skin"
	boolean followerready;	// Kart: Used to know when we can have a follower or not. (This is set on the first NameAndColor follower update)
	UINT16 followercolor;	// Kart: Used to store the follower colour the player wishes to use
	mobj_t *follower;		// Kart: This is the follower object we have. (If any)

	//

	UINT32 charflags; // Extra abilities/settings for skins (combinable stuff)
	                 // See SF_ flags

	mobjtype_t followitem; // Object # to spawn for Smiles
	mobj_t *followmobj; // Smiles all around

	SINT8 lives;
	boolean lostlife;
	SINT8 continues; // continues that player has acquired

	SINT8 xtralife; // Ring Extra Life counter
	UINT8 gotcontinue; // Got continue from this stage?

	fixed_t speed; // Player's speed (distance formula of MOMX and MOMY values)
	fixed_t lastspeed;
	UINT8 secondjump; // Jump counter

	UINT8 fly1; // Tails flying
	UINT8 scoreadd; // Used for multiple enemy attack bonus
	tic_t glidetime; // Glide counter for thrust
	UINT8 climbing; // Climbing on the wall
	INT32 deadtimer; // End game if game over lasts too long
	tic_t exiting; // Exitlevel timer

	UINT8 homing; // Are you homing?
	tic_t dashmode; // counter for dashmode ability

	tic_t skidtime; // Skid timer

	////////////////////////////
	// Conveyor Belt Movement //
	////////////////////////////
	fixed_t cmomx; // Conveyor momx
	fixed_t cmomy; // Conveyor momy
	fixed_t rmomx; // "Real" momx (momx - cmomx)
	fixed_t rmomy; // "Real" momy (momy - cmomy)

	/////////////////////
	// Race Mode Stuff //
	/////////////////////
	INT16 numboxes; // Number of item boxes obtained for Race Mode
	INT16 totalring; // Total number of rings obtained for Race Mode
	tic_t realtime; // integer replacement for leveltime
	UINT8 laps; // Number of laps (optional)
	INT32 starpostnum; // The number of the last starpost you hit

	////////////////////
	// CTF Mode Stuff //
	////////////////////
	INT32 ctfteam; // 0 == Spectator, 1 == Red, 2 == Blue
	UINT16 gotflag; // 1 == Red, 2 == Blue Do you have the flag?

	INT32 weapondelay; // Delay (if any) to fire the weapon again
	INT32 tossdelay;   // Delay (if any) to toss a flag/emeralds again

	/////////////////
	// NiGHTS Stuff//
	/////////////////
	angle_t angle_pos;
	angle_t old_angle_pos;

	mobj_t *axis1;
	mobj_t *axis2;
	tic_t bumpertime; // Currently being bounced by MT_NIGHTSBUMPER
	INT32 flyangle;
	tic_t drilltimer;
	INT32 linkcount;
	tic_t linktimer;
	INT32 anotherflyangle;
	tic_t nightstime; // How long you can fly as NiGHTS.
	INT32 drillmeter;
	UINT8 drilldelay;
	boolean bonustime; // Capsule destroyed, now it's bonus time!
	mobj_t *capsule; // Go inside the capsule
	mobj_t *drone; // Move center to the drone
	fixed_t oldscale; // Pre-Nightserize scale
	UINT8 mare; // Current mare
	UINT8 marelap; // Current mare lap
	UINT8 marebonuslap; // Current mare lap starting from bonus time

	// Statistical purposes.
	tic_t marebegunat; // Leveltime when mare begun
	tic_t startedtime; // Time which you started this mare with.
	tic_t finishedtime; // Time it took you to finish the mare (used for display)
	tic_t lapbegunat; // Leveltime when lap begun
	tic_t lapstartedtime; // Time which you started this lap with.
	INT16 finishedspheres; // The spheres you had left upon finishing the mare
	INT16 finishedrings; // The rings/stars you had left upon finishing the mare
	UINT32 marescore; // score for this nights stage
	UINT32 lastmarescore; // score for the last mare
	UINT32 totalmarescore; // score for all mares
	UINT8 lastmare; // previous mare
	UINT8 lastmarelap; // previous mare lap
	UINT8 lastmarebonuslap; // previous mare bonus lap
	UINT8 totalmarelap; // total mare lap
	UINT8 totalmarebonuslap; // total mare bonus lap
	INT32 maxlink; // maximum link obtained
	UINT8 texttimer; // nights_texttime should not be local
	UINT8 textvar; // which line of NiGHTS text to show -- let's not use cheap hacks

	INT16 lastsidehit, lastlinehit;

	tic_t losstime;
	UINT8 timeshit; // That's TIMES HIT, not TIME SHIT, you doofus!

	INT32 onconveyor; // You are on a conveyor belt if nonzero

	mobj_t *awayviewmobj;
	INT32 awayviewtics;
	angle_t awayviewaiming; // Used for cut-away view

	boolean spectator;

	boolean bot;
	botvars_t botvars;

	UINT8 splitscreenindex;

	tic_t jointime; // Timer when player joins game to change skin/color
	tic_t quittime; // Time elapsed since user disconnected, zero if connected

#ifdef HWRENDER
	fixed_t fovadd; // adjust FOV for hw rendering
#endif
} player_t;

// Values for dashmode
#define DASHMODE_THRESHOLD (3*TICRATE)
#define DASHMODE_MAX (DASHMODE_THRESHOLD + 3)

// Value for infinite lives
#define INFLIVES 0x7F

#endif
