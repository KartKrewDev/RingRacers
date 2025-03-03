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
/// \file  r_defs.h
/// \brief Refresh/rendering module, shared data struct definitions

#ifndef __R_DEFS__
#define __R_DEFS__

// Some more or less basic data types we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"

#include "screen.h" // MAXVIDWIDTH, MAXVIDHEIGHT

#ifdef HWRENDER
#include "m_aatree.h"
#endif

#include "taglist.h"

#include "k_mapuser.h"

#include "k_bot.h" // botcontroller_t

#ifdef __cplusplus
extern "C" {
#endif

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
struct cliprange_t
{
	INT32 first;
	INT32 last;
};

// Silhouette, needed for clipping segs (mainly) and sprites representing things.
#define SIL_NONE   0
#define SIL_BOTTOM 1
#define SIL_TOP    2
#define SIL_BOTH   3

// This could be wider for >8 bit display.
// Indeed, true color support is possible precalculating 24bpp lightmap/colormap LUT
// from darkening PLAYPAL to all black.
// Could even use more than 32 levels.
typedef UINT8 lighttable_t;

#define CMF_FADEFULLBRIGHTSPRITES  1
#define CMF_FOG 4

// ExtraColormap type. Use for extra_colormaps from now on.
struct extracolormap_t
{
	UINT8 fadestart, fadeend;
	UINT8 flags;

	// store rgba values in combined bitwise
	// also used in OpenGL instead lighttables
	INT32 rgba; // similar to maskcolor in sw mode
	INT32 fadergba; // The colour the colourmaps fade to

	lighttable_t *colormap;

#ifdef EXTRACOLORMAPLUMPS
	lumpnum_t lump; // for colormap lump matching, init to LUMPERROR
	char lumpname[9]; // for netsyncing
#endif

	extracolormap_t *next;
	extracolormap_t *prev;
};

//
// INTERNAL MAP TYPES used by play and refresh
//

/** Your plain vanilla vertex.
  */
struct vertex_t
{
	fixed_t x, y;
	boolean floorzset, ceilingzset;
	fixed_t floorz, ceilingz;
};

/** Degenerate version of ::mobj_t, storing only a location.
  * Used for sound origins in sectors, hoop centers, and the like. Does not
  * handle sound from moving objects (doppler), because position is probably
  * just buffered, not updated.
  */
struct degenmobj_t
{
	thinker_t thinker; ///< Not used for anything.
	fixed_t x;         ///< X coordinate.
	fixed_t y;         ///< Y coordinate.
	fixed_t z;         ///< Z coordinate.
};

#include "p_polyobj.h"

// Store fake planes in a resizable array insted of just by
// heightsec. Allows for multiple fake planes.
/** Flags describing 3Dfloor behavior and appearance.
  */
typedef enum
{
	FOF_EXISTS            = 0x1,        ///< Always set, to check for validity.
	FOF_BLOCKPLAYER       = 0x2,        ///< Solid to player, but nothing else
	FOF_BLOCKOTHERS       = 0x4,        ///< Solid to everything but player
	FOF_SOLID             = 0x6,        ///< Clips things.
	FOF_RENDERSIDES       = 0x8,        ///< Renders the sides.
	FOF_RENDERPLANES      = 0x10,       ///< Renders the floor/ceiling.
	FOF_RENDERALL         = 0x18,       ///< Renders everything.
	FOF_SWIMMABLE         = 0x20,       ///< Is a water block.
	FOF_NOSHADE           = 0x40,       ///< Messes with the lighting?
	FOF_CUTSOLIDS         = 0x80,       ///< Cuts out hidden solid pixels.
	FOF_CUTEXTRA          = 0x100,      ///< Cuts out hidden translucent pixels.
	FOF_CUTLEVEL          = 0x180,      ///< Cuts out all hidden pixels.
	FOF_CUTSPRITES        = 0x200,      ///< Final step in making 3D water.
	FOF_BOTHPLANES        = 0x400,      ///< Render inside and outside planes.
	FOF_EXTRA             = 0x800,      ///< Gets cut by ::FOF_CUTEXTRA.
	FOF_TRANSLUCENT       = 0x1000,     ///< See through!
	FOF_FOG               = 0x2000,     ///< Fog "brush."
	FOF_INVERTPLANES      = 0x4000,     ///< Only render inside planes.
	FOF_ALLSIDES          = 0x8000,     ///< Render inside and outside sides.
	FOF_INVERTSIDES       = 0x10000,    ///< Only render inside sides.
	FOF_DOUBLESHADOW      = 0x20000,    ///< Make two lightlist entries to reset light?
	FOF_FLOATBOB          = 0x40000,    ///< Floats on water and bobs if you step on it.
	FOF_NORETURN          = 0x80000,    ///< Used with ::FOF_CRUMBLE. Will not return to its original position after falling.
	FOF_CRUMBLE           = 0x100000,   ///< Falls 2 seconds after being stepped on, and randomly brings all touching crumbling 3dfloors down with it, providing their master sectors share the same tag (allows crumble platforms above or below, to also exist).
	FOF_GOOWATER          = 0x200000,   ///< Used with ::FOF_SWIMMABLE. Makes thick bouncey goop.
	FOF_MARIO             = 0x400000,   ///< Acts like a question block when hit from underneath. Goodie spawned at top is determined by master sector.
	FOF_BUSTUP            = 0x800000,   ///< You can spin through/punch this block and it will crumble!
	FOF_QUICKSAND         = 0x1000000,  ///< Quicksand!
	FOF_PLATFORM          = 0x2000000,  ///< You can jump up through this to the top.
	FOF_REVERSEPLATFORM   = 0x4000000,  ///< A fall-through floor in normal gravity, a platform in reverse gravity.
	FOF_INTANGIBLEFLATS   = 0x6000000,  ///< Both flats are intangible, but the sides are still solid.
	FOF_RIPPLE            = 0x8000000,  ///< Ripple the flats
	FOF_COLORMAPONLY      = 0x10000000, ///< Only copy the colormap, not the lightlevel
	FOF_BOUNCY            = 0x20000000, ///< Bounces players
	FOF_SPLAT             = 0x40000000, ///< Use splat flat renderer (treat cyan pixels as invisible)
} ffloortype_e;

typedef enum
{
	FF_OLD_EXISTS            = 0x1,
	FF_OLD_BLOCKPLAYER       = 0x2,
	FF_OLD_BLOCKOTHERS       = 0x4,
	FF_OLD_SOLID             = 0x6,
	FF_OLD_RENDERSIDES       = 0x8,
	FF_OLD_RENDERPLANES      = 0x10,
	FF_OLD_RENDERALL         = 0x18,
	FF_OLD_SWIMMABLE         = 0x20,
	FF_OLD_NOSHADE           = 0x40,
	FF_OLD_CUTSOLIDS         = 0x80,
	FF_OLD_CUTEXTRA          = 0x100,
	FF_OLD_CUTLEVEL          = 0x180,
	FF_OLD_CUTSPRITES        = 0x200,
	FF_OLD_BOTHPLANES        = 0x400,
	FF_OLD_EXTRA             = 0x800,
	FF_OLD_TRANSLUCENT       = 0x1000,
	FF_OLD_FOG               = 0x2000,
	FF_OLD_INVERTPLANES      = 0x4000,
	FF_OLD_ALLSIDES          = 0x8000,
	FF_OLD_INVERTSIDES       = 0x10000,
	FF_OLD_DOUBLESHADOW      = 0x20000,
	FF_OLD_FLOATBOB          = 0x40000,
	FF_OLD_NORETURN          = 0x80000,
	FF_OLD_CRUMBLE           = 0x100000,
	FF_OLD_SHATTERBOTTOM     = 0x200000,
	FF_OLD_GOOWATER          = 0x200000,
	FF_OLD_MARIO             = 0x400000,
	FF_OLD_BUSTUP            = 0x800000,
	FF_OLD_QUICKSAND         = 0x1000000,
	FF_OLD_PLATFORM          = 0x2000000,
	FF_OLD_REVERSEPLATFORM   = 0x4000000,
	FF_OLD_INTANGIBLEFLATS   = 0x6000000,
	FF_OLD_SHATTER           = 0x8000000,
	FF_OLD_SPINBUST          = 0x10000000,
	FF_OLD_STRONGBUST        = 0x20000000,
	FF_OLD_RIPPLE            = 0x40000000,
	FF_OLD_COLORMAPONLY      = (INT32)0x80000000,
} oldffloortype_e;

typedef enum
{
	FB_PUSHABLES   = 0x1, // Bustable by pushables
	FB_EXECUTOR    = 0x2, // Trigger linedef executor
	FB_ONLYBOTTOM  = 0x4, // Only bustable from below
} ffloorbustflags_e;

typedef enum
{
	BT_TOUCH,
	BT_SPINBUST,
	BT_REGULAR,
	BT_STRONG,
} busttype_e;

struct ffloor_t
{
	fixed_t *topheight;
	INT32 *toppic;
	INT16 *toplightlevel;
	fixed_t *topxoffs;
	fixed_t *topyoffs;
	angle_t *topangle;

	fixed_t *bottomheight;
	INT32 *bottompic;
	fixed_t *bottomxoffs;
	fixed_t *bottomyoffs;
	angle_t *bottomangle;

	// Pointers to pointers. Yup.
	pslope_t **t_slope;
	pslope_t **b_slope;

	size_t secnum;
	ffloortype_e fofflags;
	line_t *master;

	sector_t *target;

	ffloor_t *next;
	ffloor_t *prev;

	INT32 lastlight;
	INT32 alpha;
	UINT8 blend;
	tic_t norender; // for culling

	// Only relevant for FOF_BUSTUP
	ffloorbustflags_e bustflags;
	UINT8 busttype;
	INT16 busttag;

	// Only relevant for FOF_QUICKSAND
	fixed_t sinkspeed;
	fixed_t friction;

	// Only relevant for FOF_BOUNCY
	fixed_t bouncestrength;

	// these are saved for netgames, so do not let Lua touch these!
	ffloortype_e spawnflags; // flags the 3D floor spawned with
	INT32 spawnalpha; // alpha the 3D floor spawned with

	void *fadingdata; // fading FOF thinker
};


// This struct holds information for shadows casted by 3D floors.
// This information is contained inside the sector_t and is used as the base
// information for casted shadows.
struct lightlist_t
{
	fixed_t height;
	INT16 *lightlevel;
	extracolormap_t **extra_colormap; // pointer-to-a-pointer, so we can react to colormap changes
	INT32 flags;
	ffloor_t *caster;
	pslope_t *slope; // FOF_DOUBLESHADOW makes me have to store this pointer here. Bluh bluh.
};


// This struct is used for rendering walls with shadows casted on them...
struct r_lightlist_t
{
	fixed_t height;
	fixed_t heightstep;
	fixed_t botheight;
	fixed_t botheightstep;
	fixed_t startheight; // for repeating midtextures
	INT16 lightlevel;
	extracolormap_t *extra_colormap;
	lighttable_t *rcolormap;
	ffloortype_e flags;
	INT32 lightnum;
};

// Slopes
typedef enum {
	SL_NOPHYSICS = 1, /// This plane will have no physics applied besides the positioning.
	SL_DYNAMIC = 1<<1, /// This plane slope will be assigned a thinker to make it dynamic.
} slopeflags_t;

struct pslope_t
{
	UINT16 id; // The number of the slope, mostly used for netgame syncing purposes
	pslope_t *next; // Make a linked list of dynamic slopes, for easy reference later

	// The plane's definition.
	vector3_t o;		/// Plane origin.
	vector3_t normal;	/// Plane normal.

	vector2_t d;		/// Precomputed normalized projection of the normal over XY.
	fixed_t zdelta;		/// Precomputed Z unit increase per XY unit.

	// This values only check and must be updated if the slope itself is modified
	angle_t zangle;		/// Precomputed angle of the plane going up from the ground (not measured in degrees).
	angle_t xydirection;/// Precomputed angle of the normal's projection on the XY plane.

	UINT8 flags; // Slope options

	// SRB2Kart: For P_VeryTopOfFOF & P_VeryBottomOfFOF
	fixed_t lowz;
	fixed_t highz;

	// The ABCD constants used to define this slope
	fixed_t constants[4];

	// Light offsets (see seg_t)
	SINT8 lightOffset;
#ifdef HWRENDER
	INT16 hwLightOffset;
#endif
};

// Per-sector bot controller override
struct botcontroller_t
{
	UINT8 trick;
	UINT32 flags;
	angle_t forceAngle;
};

typedef enum
{
	// flipspecial - planes with effect
	MSF_FLIPSPECIAL_FLOOR       =  1,
	MSF_FLIPSPECIAL_CEILING     =  1<<1,
	MSF_FLIPSPECIAL_BOTH        =  (MSF_FLIPSPECIAL_FLOOR|MSF_FLIPSPECIAL_CEILING),
	// triggerspecial - conditions under which plane touch causes effect
	MSF_TRIGGERSPECIAL_TOUCH    =  1<<2,
	MSF_TRIGGERSPECIAL_HEADBUMP =  1<<3,
	// triggerline - conditions for linedef executor triggering
	MSF_TRIGGERLINE_PLANE       =  1<<4, // require plane touch
	MSF_TRIGGERLINE_MOBJ        =  1<<5, // allow non-pushable mobjs to trigger
	// invertprecip - inverts presence of precipitation
	MSF_INVERTPRECIP            =  1<<6,
	MSF_GRAVITYFLIP             =  1<<7,
	MSF_HEATWAVE                =  1<<8,
	MSF_NOCLIPCAMERA            =  1<<9,
	// water ripple
	MSF_RIPPLE_FLOOR            =  1<<10,
	MSF_RIPPLE_CEILING          =  1<<11,
	// invert encore color remap status
	MSF_INVERTENCORE            =  1<<12,
	// turn off directional lighting
	MSF_FLATLIGHTING            =  1<<13,
	// force it on (even if it was disabled)
	MSF_DIRECTIONLIGHTING       =  1<<14,
} sectorflags_t;

typedef enum
{
	SSF_NOSTEPUP = 1,
	SSF_DOUBLESTEPUP = 1<<1,
	SSF_NOSTEPDOWN = 1<<2,
	SSF_WINDCURRENT = 1<<3,
	SSF_CONVEYOR = 1<<4,
	// free: 1<<5,
	SSF_CHEATCHECKACTIVATOR = 1<<6,
	SSF_EXIT = 1<<7,
	SSF_DELETEITEMS = 1<<8,
	// free: 1<<9,
	// free: 1<<10,
	// free: 1<<11,
	SSF_FAN = 1<<12,
	// free: 1<<13,
	// free: 1<<14,
	SSF_ZOOMTUBESTART = 1<<15,
	SSF_ZOOMTUBEEND = 1<<16,
} sectorspecialflags_t;

typedef enum
{
	// Mask to get trigger type.
	SECSPAC_TRIGGERMASK			= 0x0000000F,

	// Special action is activated once.
	SECSPAC_ONCESPECIAL			= 0x00000000,

	// Special action is repeatable.
	SECSPAC_REPEATSPECIAL		= 0x00000001,

	// Special action is activated continously.
	SECSPAC_CONTINUOUSSPECIAL	= 0x00000002,

	// When a player enters this sector.
	SECSPAC_ENTER				= 0x00000010,

	// When a player touches the floor of this sector.
	SECSPAC_FLOOR				= 0x00000020,

	// When a player touches the ceiling of this sector.
	SECSPAC_CEILING				= 0x00000040,

	// When an enemy enters this sector.
	SECSPAC_ENTERMONSTER		= 0x00000080,

	// When an enemy touches the floor of this sector.
	SECSPAC_FLOORMONSTER		= 0x00000100,

	// When an enemy touches the ceiling of this sector.
	SECSPAC_CEILINGMONSTER		= 0x00000200,

	// When a projectile enters this sector.
	SECSPAC_ENTERMISSILE		= 0x00000400,

	// When a projectile touches the floor of this sector.
	SECSPAC_FLOORMISSILE		= 0x00000800,

	// When a projectile touches the ceiling of this sector.
	SECSPAC_CEILINGMISSILE		= 0x00001000,
} sectoractionflags_t;

typedef enum
{
	SD_NONE = 0,
	SD_GENERIC = 1,
	SD_LAVA = 2,
	SD_DEATHPIT = 3,
	SD_INSTAKILL = 4,
	SD_STUMBLE = 5,
} sectordamage_t;

typedef enum
{
	TO_PLAYER = 0,
	TO_ALLPLAYERS = 1,
	TO_MOBJ = 2,
} triggerobject_t;

typedef enum
{
	CRUMBLE_NONE, // No crumble thinker
	CRUMBLE_WAIT, // Don't float on water because this is supposed to wait for a crumble
	CRUMBLE_ACTIVATED, // Crumble thinker activated, but hasn't fallen yet
	CRUMBLE_FALL, // Crumble thinker is falling
	CRUMBLE_RESTORE, // Crumble thinker is about to restore to original position
} crumblestate_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
struct sector_t
{
	fixed_t floorheight;
	fixed_t ceilingheight;
	INT32 floorpic;
	INT32 ceilingpic;
	INT16 lightlevel;
	INT16 special;
	taglist_t tags;

	// origin for any sounds played by the sector
	// also considered the center for e.g. Mario blocks
	degenmobj_t soundorg;

	// if == validcount, already checked
	size_t validcount;

	// list of mobjs in sector
	mobj_t *thinglist;

	// thinker_ts for reversable actions
	void *floordata; // floor move thinker
	void *ceilingdata; // ceiling move thinker
	void *lightingdata; // lighting change thinker
	void *fadecolormapdata; // fade colormap thinker

	// floor and ceiling texture offsets
	fixed_t floor_xoffs, floor_yoffs;
	fixed_t ceiling_xoffs, ceiling_yoffs;

	// flat angle
	angle_t floorpic_angle;
	angle_t ceilingpic_angle;

	INT32 heightsec; // other sector, or -1 if no other sector
	INT32 camsec; // used for camera clipping

	// floor and ceiling lighting
	INT16 floorlightlevel, ceilinglightlevel;
	boolean floorlightabsolute, ceilinglightabsolute; // absolute or relative to sector's light level?
	INT32 floorlightsec, ceilinglightsec; // take floor/ceiling light level from another sector

	INT32 crumblestate; // used for crumbling and bobbing

	// list of mobjs that are at least partially in the sector
	// thinglist is a subset of touching_thinglist
	msecnode_t *touching_thinglist;

	size_t linecount;
	line_t **lines; // [linecount] size

	// Improved fake floor hack
	ffloor_t *ffloors;
	size_t *attached;
	boolean *attachedsolid;
	size_t numattached;
	size_t maxattached;
	lightlist_t *lightlist;
	INT32 numlights;
	boolean moved;

	// per-sector colormaps!
	extracolormap_t *extra_colormap;
	boolean colormap_protected;

	fixed_t gravity; // per-sector gravity factor
	fixed_t *gravityptr; // For binary format: Read gravity from floor height of master sector

	sectorflags_t flags;
	sectorspecialflags_t specialflags;
	UINT8 damagetype;

	fixed_t offroad; // Ring Racers

	// Linedef executor triggering
	mtag_t triggertag; // tag to call upon triggering
	UINT8 triggerer; // who can trigger?

	fixed_t friction;

	// Sprite culling feature
	line_t *cullheight;

	// Current speed of ceiling/floor. For Knuckles to hold onto stuff.
	fixed_t floorspeed, ceilspeed;

	// list of precipitation mobjs in sector
	mprecipsecnode_t *touching_preciplist;

	// Eternity engine slope
	pslope_t *f_slope; // floor slope
	pslope_t *c_slope; // ceiling slope
	boolean hasslope; // The sector, or one of its visible FOFs, contains a slope

	// for fade thinker
	INT16 spawn_lightlevel;

	// colormap structure
	extracolormap_t *spawn_extra_colormap;

	// Ring Racers bots
	botcontroller_t botController;

	// Action specials
	INT16 action;
	INT32 args[NUM_SCRIPT_ARGS];
	char *stringargs[NUM_SCRIPT_STRINGARGS];
	sectoractionflags_t activation;

	// UDMF user-defined custom properties.
	mapUserProperties_t user;
};

//
// Move clipping aid for linedefs.
//
typedef enum
{
	ST_HORIZONTAL,
	ST_VERTICAL,
	ST_POSITIVE,
	ST_NEGATIVE
} slopetype_t;

#define HORIZONSPECIAL (41)

struct line_t
{
	// Vertices, from v1 to v2.
	vertex_t *v1;
	vertex_t *v2;

	fixed_t dx, dy; // Precalculated v2 - v1 for side checking.
	angle_t angle; // Precalculated angle between dx and dy

	// Animation related.
	UINT32 flags;
	UINT32 activation;
	INT16 special;
	taglist_t tags;
	INT32 args[NUM_SCRIPT_ARGS];
	char *stringargs[NUM_SCRIPT_STRINGARGS];

	// Visual appearance: sidedefs.
	UINT16 sidenum[2]; // sidenum[1] will be 0xffff if one-sided
	fixed_t alpha; // translucency
	UINT8 blendmode; // blendmode
	INT32 executordelay;

	fixed_t bbox[4]; // bounding box for the extent of the linedef

	// To aid move clipping.
	slopetype_t slopetype;

	// Front and back sector.
	// Note: redundant? Can be retrieved from SideDefs.
	sector_t *frontsector;
	sector_t *backsector;

	size_t validcount; // if == validcount, already checked
	polyobj_t *polyobj; // Belongs to a polyobject?

	boolean tripwire;

	INT16 callcount; // no. of calls left before triggering, for the "X calls" linedef specials, defaults to 0

	// UDMF user-defined custom properties.
	mapUserProperties_t user;
};

struct side_t
{
	// add this to the calculated texture column
	fixed_t textureoffset;

	// add this to the calculated texture top
	fixed_t rowoffset;

	// Texture indices.
	// We do not maintain names here.
	INT32 toptexture, bottomtexture, midtexture;

	// Interpolator installed? (R_CreateInterpolator_SideScroll)
	boolean acs_interpolated;

	// Linedef the sidedef belongs to
	line_t *line;

	// Sector the sidedef is facing.
	sector_t *sector;

	INT16 special; // the special of the linedef this side belongs to
	INT16 repeatcnt; // # of times to repeat midtexture

	extracolormap_t *colormap_data; // storage for colormaps; not applied to sectors.

	// UDMF user-defined custom properties.
	mapUserProperties_t user;
};

//
// A subsector.
// References a sector.
// Basically, this is a list of linesegs, indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
struct subsector_t
{
	sector_t *sector;
	INT16 numlines;
	UINT16 firstline;
	polyobj_t *polyList; // haleyjd 02/19/06: list of polyobjects
	size_t validcount;
};

// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_thinglist_next
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_sectorlist_next links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

struct msecnode_t
{
	sector_t *m_sector; // a sector containing this object
	mobj_t *m_thing;  // this object
	msecnode_t *m_sectorlist_prev;  // prev msecnode_t for this thing
	msecnode_t *m_sectorlist_next;  // next msecnode_t for this thing
	msecnode_t *m_thinglist_prev;  // prev msecnode_t for this sector
	msecnode_t *m_thinglist_next;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
};

struct mprecipsecnode_t
{
	sector_t *m_sector; // a sector containing this object
	precipmobj_t *m_thing;  // this object
	mprecipsecnode_t *m_sectorlist_prev;  // prev msecnode_t for this thing
	mprecipsecnode_t *m_sectorlist_next;  // next msecnode_t for this thing
	mprecipsecnode_t *m_thinglist_prev;  // prev msecnode_t for this sector
	mprecipsecnode_t *m_thinglist_next;  // next msecnode_t for this sector
	boolean visited; // used in search algorithms
};

// for now, only used in hardware mode
// maybe later for software as well?
// that's why it's moved here
struct light_t
{
	UINT16 type;          // light,... (cfr #define in hwr_light.c)

	float light_xoffset;
	float light_yoffset;  // y offset to adjust corona's height

	UINT32 corona_color;   // color of the light for static lighting
	float corona_radius;  // radius of the coronas

	UINT32 dynamic_color;  // color of the light for dynamic lighting
	float dynamic_radius; // radius of the light ball
	float dynamic_sqrradius; // radius^2 of the light ball
};

struct lightmap_t
{
	float s[2], t[2];
	light_t *light;
	lightmap_t *next;
};

//
// The lineseg.
//
struct seg_t
{
	vertex_t *v1;
	vertex_t *v2;

	INT32 side;

	fixed_t offset;

	angle_t angle;

	side_t *sidedef;
	line_t *linedef;

	// Sector references.
	// Could be retrieved from linedef, too. backsector is NULL for one sided lines
	sector_t *frontsector;
	sector_t *backsector;

	fixed_t length;	// precalculated seg length
#ifdef HWRENDER
	// new pointers so that AdjustSegs doesn't mess with v1/v2
	void *pv1; // polyvertex_t
	void *pv2; // polyvertex_t
	float flength; // length of the seg, used by hardware renderer

	lightmap_t *lightmaps; // for static lightmap
#endif

	// Why slow things down by calculating lightlists for every thick side?
	size_t numlights;
	r_lightlist_t *rlights;
	polyobj_t *polyseg;
	boolean dontrenderme;
	boolean glseg;

	// Fake contrast calculated on level load
	SINT8 lightOffset;
#ifdef HWRENDER
	INT16 hwLightOffset;
#endif
};

//
// BSP node.
//
struct node_t
{
	// Partition line.
	fixed_t x, y;
	fixed_t dx, dy;

	// Bounding box for each child.
	fixed_t bbox[2][4];

	// If NF_SUBSECTOR its a subsector.
	UINT16 children[2];
};

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// posts are runs of non masked source pixels
struct post_t
{
	UINT8 topdelta; // -1 is the last post in a column
	UINT8 length;   // length data bytes follows
} ATTRPACK;

#if defined(_MSC_VER)
#pragma pack()
#endif

// column_t is a list of 0 or more post_t, (UINT8)-1 terminated
typedef post_t column_t;

//
// OTHER TYPES
//

#ifndef MAXFFLOORS
#define MAXFFLOORS 40
#endif

//
// ?
//
struct drawseg_t
{
	seg_t *curline;
	INT32 x1;
	INT32 x2;

	fixed_t scale1;
	fixed_t scale2;
	fixed_t scalestep;

	INT32 silhouette; // 0 = none, 1 = bottom, 2 = top, 3 = both

	fixed_t bsilheight; // do not clip sprites above this
	fixed_t tsilheight; // do not clip sprites below this

	// Pointers to lists for sprite clipping, all three adjusted so [x1] is first value.
	INT16 *sprtopclip;
	INT16 *sprbottomclip;
	INT16 *maskedtexturecol;

	visplane_t *ffloorplanes[MAXFFLOORS];
	INT32 numffloorplanes;
	ffloor_t *thicksides[MAXFFLOORS];
	INT16 *thicksidecol;
	INT32 numthicksides;
	fixed_t frontscale[MAXVIDWIDTH];

	UINT8 portalpass; // if > 0 and <= portalrender, do not affect sprite clipping

	fixed_t maskedtextureheight[MAXVIDWIDTH]; // For handling sloped midtextures

	vertex_t leftpos, rightpos; // Used for rendering FOF walls with slopes
};

typedef enum
{
	PALETTE         = 0,  // 1 byte is the index in the doom palette (as usual)
	INTENSITY       = 1,  // 1 byte intensity
	INTENSITY_ALPHA = 2,  // 2 byte: alpha then intensity
	RGB24           = 3,  // 24 bit rgb
	RGBA32          = 4,  // 32 bit rgba
} pic_mode_t;

#ifdef ROTSPRITE
struct rotsprite_t
{
	INT32 angles;
	void **patches;
};
#endif

// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures, and we compose
// textures from the TEXTURES list of patches.
//
struct patch_t
{
	INT16 width, height;
	INT16 leftoffset, topoffset;

	INT32 *columnofs; // Column offsets. This is relative to patch->columns
	UINT8 *columns; // Software column data

	void *hardware; // OpenGL patch, allocated whenever necessary
	void *flats[4]; // The patch as flats

#ifdef ROTSPRITE
	rotsprite_t *rotated; // Rotated patches
#endif
};

extern patch_t *missingpat;
extern patch_t *blanklvl, *nolvl;
extern patch_t *unvisitedlvl[4];

#if defined(_MSC_VER)
#pragma pack(1)
#endif

struct softwarepatch_t
{
	INT16 width;          // bounding box size
	INT16 height;
	INT16 leftoffset;     // pixels to the left of origin
	INT16 topoffset;      // pixels below the origin
	INT32 columnofs[8];     // only [width] used
	// the [0] is &columnofs[width]
} ATTRPACK;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// a pic is an unmasked block of pixels, stored in horizontal way
struct pic_t
{
	INT16 width;
	UINT8 zero;       // set to 0 allow autodetection of pic_t
	                 // mode instead of patch or raw
	UINT8 mode;       // see pic_mode_t above
	INT16 height;
	INT16 reserved1; // set to 0
	UINT8 data[];
} ATTRPACK;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

#if defined(_MSC_VER)
#pragma pack()
#endif

// Possible alpha types for a patch.
typedef enum {AST_COPY, AST_TRANSLUCENT, AST_ADD, AST_SUBTRACT, AST_REVERSESUBTRACT, AST_MODULATE, AST_OVERLAY, AST_FOG} patchalphastyle_t;

typedef enum
{
	RF_HORIZONTALFLIP   = 0x00000001,   // Flip sprite horizontally
	RF_VERTICALFLIP     = 0x00000002,   // Flip sprite vertically
	RF_ABSOLUTEOFFSETS  = 0x00000004,   // Sprite uses the object's offsets absolutely, instead of relatively
	RF_FLIPOFFSETS      = 0x00000008,   // Relative object offsets are flipped with the sprite

	RF_SPLATMASK        = 0x000000F0,   // --Floor sprite flags
	RF_SLOPESPLAT       = 0x00000010,   // Rotate floor sprites by a slope
	RF_OBJECTSLOPESPLAT = 0x00000020,   // Rotate floor sprites by the object's standing slope
	RF_NOSPLATBILLBOARD = 0x00000040,   // Don't billboard floor sprites (faces forward from the view angle)
	RF_NOSPLATROLLANGLE = 0x00000080,   // Don't rotate floor sprites by the object's rollangle (uses rotated patches instead)

	RF_BRIGHTMASK       = 0x00000300,   // --Bright modes
	RF_FULLBRIGHT       = 0x00000100,   // Sprite is drawn at full brightness
	RF_FULLDARK         = 0x00000200,   // Sprite is drawn completely dark
	RF_SEMIBRIGHT       = (RF_FULLBRIGHT | RF_FULLDARK), // between sector bright and full bright

	RF_NOCOLORMAPS      = 0x00000400,   // Sprite is not drawn with colormaps

	RF_ALWAYSONTOP      = 0x00000800,   // Sprite is drawn on top of level geometry

	RF_SPRITETYPEMASK   = 0x00003000,   // --Different sprite types
	RF_PAPERSPRITE      = 0x00001000,   // Paper sprite
	RF_FLOORSPRITE      = 0x00002000,   // Floor sprite

	RF_SHADOWDRAW       = 0x00004000,  // Stretches and skews the sprite like a shadow.
	RF_SHADOWEFFECTS    = 0x00008000,  // Scales and becomes transparent like a shadow.
	RF_DROPSHADOW       = (RF_SHADOWDRAW | RF_SHADOWEFFECTS | RF_FULLDARK),

	RF_ABSOLUTELIGHTLEVEL = 0x00010000, //  mobj_t.lightlevel is absolute instead of relative
	RF_REDUCEVFX          = 0x00020000, //  only mobj_t.owner can see this object
	RF_HIDEINSKYBOX       = 0x00040000, //  do not render in skybox

	RF_DONTDRAW         = 0x00F00000,   // --Don't generate a vissprite
	RF_DONTDRAWP1       = 0x00100000,   // No P1
	RF_DONTDRAWP2       = 0x00200000,   // No P2
	RF_DONTDRAWP3       = 0x00400000,   // No P3
	RF_DONTDRAWP4       = 0x00800000,   // No P4

	RF_BLENDMASK       	= 0x07000000,   // --Blending override - see patchalphastyle_t
	RF_BLENDSHIFT		= (6*4),
	// minus 1 as effects don't distinguish between AST_COPY and AST_TRANSLUCENT
	RF_ADD				= ((AST_ADD-1)<<RF_BLENDSHIFT),
	RF_SUBTRACT			= ((AST_SUBTRACT-1)<<RF_BLENDSHIFT),
	RF_REVERSESUBTRACT	= ((AST_REVERSESUBTRACT-1)<<RF_BLENDSHIFT),
	RF_MODULATE			= ((AST_MODULATE-1)<<RF_BLENDSHIFT),
	RF_OVERLAY			= ((AST_OVERLAY-1)<<RF_BLENDSHIFT),

	RF_TRANSMASK       	= (INT32)0xF0000000,   // --Transparency override
	RF_TRANSSHIFT		= (7*4),
	RF_TRANS10       	= (1<<RF_TRANSSHIFT),   // 10%
	RF_TRANS20       	= (2<<RF_TRANSSHIFT),   // 20%
	RF_TRANS30       	= (3<<RF_TRANSSHIFT),   // 30%
	RF_TRANS40       	= (4<<RF_TRANSSHIFT),   // 40%
	RF_TRANS50       	= (5<<RF_TRANSSHIFT),   // 50%
	RF_TRANS60       	= (6<<RF_TRANSSHIFT),   // 60%
	RF_TRANS70       	= (7<<RF_TRANSSHIFT),   // 70%
	RF_TRANS80       	= (INT32)(8U<<RF_TRANSSHIFT),   // 80%
	RF_TRANS90       	= (INT32)(9U<<RF_TRANSSHIFT),   // 90%
	RF_GHOSTLY			= (RF_TRANS80 | RF_FULLBRIGHT),
	RF_GHOSTLYMASK		= (RF_TRANSMASK | RF_FULLBRIGHT),
} renderflags_t;

typedef enum
{
	SRF_SINGLE      = 0,   // 0-angle for all rotations
	SRF_3D          = 1,   // Angles 1-8
	SRF_3DGE        = 2,   // 3DGE, ZDoom and Doom Legacy all have 16-angle support. Why not us?
	SRF_3DMASK      = SRF_3D|SRF_3DGE, // 3
	SRF_LEFT        = 4,   // Left side uses single patch
	SRF_RIGHT       = 8,   // Right side uses single patch
	SRF_2D          = SRF_LEFT|SRF_RIGHT, // 12
	SRF_NONE        = 0xff // Initial value
} spriterotateflags_t;     // SRF's up!

//
// Sprites are patches with a special naming convention so they can be
//  recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with x indicating the rotation,
//  x = 0, 1-8, 9+A-G, L/R
// The sprite and frame specified by a thing_t is range checked at run time.
// A sprite is a patch_t that is assumed to represent a three dimensional
//  object and may have multiple rotations predrawn.
// Horizontal flipping is used to save space, thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used for all views: NNNNF0
// Some sprites will take the entirety of the left side: NNNNFL
// Or the right side: NNNNFR
// Or both, mirrored: NNNNFLFR
//
struct spriteframe_t
{
	// If false use 0 for any position.
	// Note: as eight entries are available, we might as well insert the same
	//  name eight times.
	UINT8 rotate; // see spriterotateflags_t above

	// Lump to use for view angles 0-7/15.
	lumpnum_t lumppat[16]; // lump number 16 : 16 wad : lump
	size_t lumpid[16]; // id in the spriteoffset, spritewidth, etc. tables

	// Flip bits (1 = flip) to use for view angles 0-7/15.
	UINT16 flip;

#ifdef ROTSPRITE
	rotsprite_t *rotated[2][16]; // Rotated patches
#endif
};

//
// A sprite definition:  a number of animation frames.
//
struct spritedef_t
{
	size_t numframes;
	spriteframe_t *spriteframes;
};

// Column and span drawing data bundles

typedef struct
{
	lighttable_t* colormap;
	lighttable_t* fullbright;
	INT32 x;
	INT32 yl;
	INT32 yh;
	fixed_t iscale;
	fixed_t texturemid;
	UINT8 hires;
	UINT8 shadowcolor;

	UINT8* source; // first pixel in a column
	UINT8* brightmap; // brightmap texture column, can be NULL
	UINT8* lightmap; // lighting only

	// translucency stuff here
	UINT8* transmap;

	// translation stuff here
	UINT8* translation;

	struct r_lightlist_t* lightlist;

	INT32 numlights;
	INT32 maxlights;

	//Fix TUTIFRUTI
	INT32 texheight;
	INT32 sourcelength;

	UINT8 r8_flatcolor;
} drawcolumndata_t;

extern drawcolumndata_t g_dc;

typedef struct {
	float x, y, z;
} floatv3_t;

typedef struct
{
	INT32 y;
	INT32 x1;
	INT32 x2;
	lighttable_t* colormap;
	lighttable_t* fullbright;
	lighttable_t* translation;
	lighttable_t* flatlighting;

	fixed_t xfrac;
	fixed_t yfrac;
	fixed_t xstep;
	fixed_t ystep;
	INT32 waterofs;
	INT32 bgofs;

	fixed_t xoffs;
	fixed_t yoffs;

	UINT16 flatwidth;
	UINT16 flatheight;
	boolean powersoftwo;

	visplane_t *currentplane;
	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *transmap;

	UINT8 flatcolor;

	float zeroheight;

	// Vectors for Software's tilted slope drawers
	floatv3_t sup;
	floatv3_t svp;
	floatv3_t szp;
	floatv3_t slope_origin;
	floatv3_t slope_u;
	floatv3_t slope_v;

	// Variable flat sizes
	UINT32 nflatxshift;
	UINT32 nflatyshift;
	UINT32 nflatshiftup;
	UINT32 nflatmask;

	fixed_t planeheight;
	lighttable_t **planezlight;

	//
	// Water ripple effect
	// Needs the height of the plane, and the vertical position of the span.
	// Sets planeripple.xfrac and planeripple.yfrac, added to ds_xfrac and ds_yfrac, if the span is not tilted.
	//
	struct
	{
		INT32 offset;
		fixed_t xfrac, yfrac;
		boolean active;
	} planeripple;

	UINT8 r8_flatcolor;
} drawspandata_t;

extern drawspandata_t g_ds;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
