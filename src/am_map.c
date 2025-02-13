// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  am_map.c
/// \brief Code for the 'automap', former Doom feature used for DEVMODE testing

#include "am_map.h"
#include "g_game.h"
#include "g_input.h"
#include "p_local.h"
#include "p_slopes.h"
#include "v_video.h"
#include "i_video.h"
#include "r_state.h"
#include "r_draw.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// For use if I do walls with outsides/insides
static const UINT8 REDS        = (2*16);
static const UINT8 REDRANGE    = 16;
static const UINT8 GRAYS       = (1*16);
static const UINT8 GRAYSRANGE  = 16;
static const UINT8 BROWNS      = (15*16);
static const UINT8 YELLOWS     = (5*16)+8;
static const UINT8 GREENS      = (6*16);
static const UINT8 CYANS       = (8*16);
static const UINT8 BLUES       = (9*16);
static const UINT8 DBLACK      = 31;
static const UINT8 DWHITE      = 0;


// Automap colors
#define BACKGROUND            DBLACK
#define WALLCOLORS            (REDS + REDRANGE/2)
#define WALLRANGE             (REDRANGE/2)
#define THOKWALLCOLORS        REDS
#define THOKWALLRANGE         REDRANGE
#define TSWALLCOLORS          DWHITE
#define TSFINISHLINE          (GREENS + 2)
#define TSTRIPWIRE            (CYANS + 4)
#define TSFOFINFO             (BLUES + 4)
#define FDWALLCOLORS          BROWNS
#define CDWALLCOLORS          YELLOWS
#define THINGCOLORS           GREENS
#define GRIDCOLORS            (GRAYS + GRAYSRANGE/2)
#define XHAIRCOLORS           GRAYS

// Automap passes
#define PASS_SOLID 			1
#define PASS_INTANGIBLE		2
#define PASS_FOF			4

// controls
#define AM_PANUPKEY     KEY_UPARROW
#define AM_PANDOWNKEY   KEY_DOWNARROW
#define AM_PANLEFTKEY   KEY_LEFTARROW
#define AM_PANRIGHTKEY  KEY_RIGHTARROW

#define AM_ZOOMINKEY    '='
#define AM_ZOOMOUTKEY   '-'
#define AM_GOBIGKEY     '0'

#define AM_FOLLOWKEY    'f'
#define AM_GRIDKEY      'g'

#define AM_TOGGLEKEY    KEY_TAB

// scale on entry
#define INITSCALEMTOF (FRACUNIT/5)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC 4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN ((51*FRACUNIT)/50)
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT ((50*FRACUNIT)/51)

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<FRACBITS),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>FRACBITS)
// translates between frame-buffer and map coordinates
#define CXMTOF(x) (f_x + MTOF((x)-m_x))
#define CYMTOF(y) (f_y + (f_h - MTOF((y)-m_y)))

#define MAPBITS (FRACBITS-4)
#define FRACTOMAPBITS (FRACBITS-MAPBITS)

typedef struct
{
	fixed_t x, y;
} mpoint_t;

typedef struct
{
	mpoint_t a, b;
} mline_t;

//
// The vector graphics for the automap.
// A line drawing of the player pointing right,
//   starting from the middle.
//

#define PLAYERRADIUS (16*(1<<MAPBITS))
#define R ((8*PLAYERRADIUS)/7)

static const mline_t player_arrow[] = {
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/4 } }, // ----->
	{ { R, 0 }, { R-R/2, -R/4 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
	{ { -R+R/8, 0 }, { -R-R/8, -R/4 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof (player_arrow)/sizeof (mline_t))

#define R (FRACUNIT)
static const mline_t cross_mark[] =
{
	{ { -R, 0 }, { R, 0} },
	{ { 0, -R }, { 0, R } },
};
#undef R
#define NUMCROSSMARKLINES (sizeof(cross_mark)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t thintriangle_guy[] = {
	{ { (-1*R)/2, (-7*R)/10 }, {        R, 0         } },
	{ {        R, 0         }, { (-1*R)/2, (7*R)/10 } },
	{ { (-1*R)/2, (7*R)/10 }, { (-1*R)/2, (-7*R)/10 } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof (thintriangle_guy)/sizeof (mline_t))

static boolean bigstate;	// user view and large view (full map view)
static boolean draw_grid = false;

boolean automapactive = false;
boolean am_recalc = false; //added : 05-02-98 : true when screen size changes
static boolean am_stopped = true;
static boolean am_minigen = false;

static UINT8 *am_buf = NULL;

static INT32 f_x, f_y;	// location of window on screen (always zero for both)
static INT32 f_w, f_h;	// size of window on screen (always the screen width and height respectively)

static boolean m_keydown[4]; // which window panning keys are being pressed down?
static mpoint_t m_paninc; // how far the window pans each tic (map coords)
static fixed_t mtof_zoommul; // how far the window zooms in each tic (map coords)
static fixed_t ftom_zoommul; // how far the window zooms in each tic (fb coords)

static fixed_t m_x, m_y;   // LL x,y where the window is on the map (map coords)
static fixed_t m_x2, m_y2; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t m_w;
static fixed_t m_h;

// based on level size
static fixed_t min_x;
static fixed_t min_y;
static fixed_t max_x;
static fixed_t max_y;

static fixed_t max_w; // max_x-min_x,
static fixed_t max_h; // max_y-min_y

static fixed_t min_scale_mtof; // used to tell when to stop zooming out
static fixed_t max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = (fixed_t)INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static player_t *plr; // the player represented by an arrow

static boolean followplayer = true; // specifies whether to follow the player around

// function for drawing lines, depends on rendermode
typedef void (*AMDRAWFLINEFUNC) (const fline_t *fl, INT32 color);
static AMDRAWFLINEFUNC AM_drawFline;

static void AM_drawPixel(INT32 xx, INT32 yy, INT32 cc);
static void AM_drawFline_soft(const fline_t *fl, INT32 color);

static void AM_activateNewScale(void)
{
	m_x += m_w/2;
	m_y += m_h/2;
	m_w = FTOM(f_w);
	m_h = FTOM(f_h);
	m_x -= m_w/2;
	m_y -= m_h/2;
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

static inline void AM_saveScaleAndLoc(void)
{
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

static inline void AM_restoreScaleAndLoc(void)
{
	m_w = old_m_w;
	m_h = old_m_h;
	if (!followplayer)
	{
		m_x = old_m_x;
		m_y = old_m_y;
	}
	else
	{
		m_x = (plr->mo->x >> FRACTOMAPBITS) - m_w/2;
		m_y = (plr->mo->y >> FRACTOMAPBITS) - m_h/2;
	}
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;

	// Change the scaling multipliers
	scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

/** Determines the bounding box around all vertices.
  * This is used to set global variables controlling the zoom range.
  */
static void AM_findMinMaxBoundaries(void)
{
	fixed_t a, b;

	min_x = minimapinfo.min_x << MAPBITS;
	max_x = minimapinfo.max_x << MAPBITS;
	min_y = minimapinfo.min_y << MAPBITS;
	max_y = minimapinfo.max_y << MAPBITS;

	max_w = minimapinfo.map_w << MAPBITS;
	max_h = minimapinfo.map_h << MAPBITS;

	a = FixedDiv(f_w<<FRACBITS, max_w);
	b = FixedDiv(f_h<<FRACBITS, max_h);

	min_scale_mtof = a < b ? a : b;
	max_scale_mtof = FixedDiv(f_h<<FRACBITS, 2*PLAYERRADIUS);
}

static void AM_changeWindowLoc(void)
{
	if (m_paninc.x || m_paninc.y)
	{
		followplayer = false;
		f_oldloc.x = INT32_MAX;
	}

	m_x += m_paninc.x;
	m_y += m_paninc.y;

	if (m_x + m_w/2 > max_x)
		m_x = max_x - m_w/2;
	else if (m_x + m_w/2 < min_x)
		m_x = min_x - m_w/2;

	if (m_y + m_h/2 > max_y)
		m_y = max_y - m_h/2;
	else if (m_y + m_h/2 < min_y)
		m_y = min_y - m_h/2;

	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

static void AM_initVariables(void)
{
	INT32 pnum;

	automapactive = true;
	f_oldloc.x = INT32_MAX;

	m_paninc.x = m_paninc.y = 0;
	ftom_zoommul = FRACUNIT;
	mtof_zoommul = FRACUNIT;

	m_w = FTOM(f_w);
	m_h = FTOM(f_h);

	// find player to center on initially
	if (!playeringame[pnum = consoleplayer])
		for (pnum = 0; pnum < MAXPLAYERS; pnum++)
			if (playeringame[pnum])
				break;

	plr = &players[pnum];
	if (plr != NULL && plr->mo != NULL)
	{
		m_x = (plr->mo->x >> FRACTOMAPBITS) - m_w/2;
		m_y = (plr->mo->y >> FRACTOMAPBITS) - m_h/2;
	}
	AM_changeWindowLoc();

	// for saving & restoring
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

//
// Called when the screen size changes.
//
static void AM_FrameBufferInit(void)
{
	f_x = f_y = 0;
	f_w = vid.width;
	f_h = vid.height;
	am_buf = screens[0];
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
static void AM_LevelInit(void)
{
	AM_findMinMaxBoundaries();
	scale_mtof = FixedDiv(min_scale_mtof*10, 7*FRACUNIT);
	if (scale_mtof > max_scale_mtof)
		scale_mtof = min_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

/** Disables automap.
  *
  * \sa AM_Start
  */
void AM_Stop(void)
{
	automapactive = false;
	am_stopped = true;
}

/** Enables automap.
  *
  * \sa AM_Stop
  */
void AM_Start(void)
{
	static INT32 lastlevel = -1;

	if (!am_stopped)
		AM_Stop();
	am_stopped = false;
	if (lastlevel != gamemap || am_recalc) // screen size changed
	{
		AM_FrameBufferInit();
		if (lastlevel != gamemap)
		{
			AM_LevelInit();
			lastlevel = gamemap;
		}
		am_recalc = false;
	}
	AM_initVariables();
}

//
// set the window scale to the maximum size
//
static void AM_minOutWindowScale(void)
{
	scale_mtof = min_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
	AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
static void AM_maxOutWindowScale(void)
{
	scale_mtof = max_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
	AM_activateNewScale();
}

//
// set window panning
//
static void AM_setWindowPanning(void)
{
	// up and down
	if (m_keydown[2]) // pan up
		m_paninc.y = FTOM(F_PANINC);
	else if (m_keydown[3]) // pan down
		m_paninc.y = -FTOM(F_PANINC);
	else
		m_paninc.y = 0;

	// left and right
	if (m_keydown[0]) // pan right
		m_paninc.x = FTOM(F_PANINC);
	else if (m_keydown[1]) // pan left
		m_paninc.x = -FTOM(F_PANINC);
	else
		m_paninc.x = 0;
}

/** Responds to user inputs in automap mode.
  *
  * \param ev Event to possibly respond to.
  * \return True if the automap responder ate the event.
  */
boolean AM_Responder(event_t *ev)
{
	INT32 rc = false;

	if (devparm || cht_debug) // only automap in Debug Tails 01-19-2001
	{
		if (!automapactive)
		{
			if (ev->type == ev_keydown && ev->data1 == AM_TOGGLEKEY)
			{
				//faB: prevent alt-tab in win32 version to activate automap just before
				//     minimizing the app; doesn't do any harm to the DOS version
				if (!G_GetDeviceGameKeyDownArray(0)[KEY_LALT] && !G_GetDeviceGameKeyDownArray(0)[KEY_RALT])
				{
					bigstate = 0; //added : 24-01-98 : toggle off large view
					AM_Start();
					rc = true;
				}
			}
		}
		else if (ev->type == ev_keydown)
		{
			rc = true;
			switch (ev->data1)
			{
				case AM_PANRIGHTKEY: // pan right
					if (!followplayer)
					{
						m_keydown[0] = true;
						AM_setWindowPanning();
					}
					else
						rc = false;
					break;
				case AM_PANLEFTKEY: // pan left
					if (!followplayer)
					{
						m_keydown[1] = true;
						AM_setWindowPanning();
					}
					else
						rc = false;
					break;
				case AM_PANUPKEY: // pan up
					if (!followplayer)
					{
						m_keydown[2] = true;
						AM_setWindowPanning();
					}
					else
						rc = false;
					break;
				case AM_PANDOWNKEY: // pan down
					if (!followplayer)
					{
						m_keydown[3] = true;
						AM_setWindowPanning();
					}
					else
						rc = false;
					break;
				case AM_ZOOMOUTKEY: // zoom out
					mtof_zoommul = M_ZOOMOUT;
					ftom_zoommul = M_ZOOMIN;
					AM_setWindowPanning();
					break;
				case AM_ZOOMINKEY: // zoom in
					mtof_zoommul = M_ZOOMIN;
					ftom_zoommul = M_ZOOMOUT;
					AM_setWindowPanning();
					break;
				case AM_TOGGLEKEY:
					AM_Stop();
					break;
				case AM_GOBIGKEY:
					bigstate = !bigstate;
					if (bigstate)
					{
						AM_saveScaleAndLoc();
						AM_minOutWindowScale();
					}
					else
						AM_restoreScaleAndLoc();
					AM_setWindowPanning();
					break;
				case AM_FOLLOWKEY:
					followplayer = !followplayer;
					f_oldloc.x = INT32_MAX;
					break;
				case AM_GRIDKEY:
					draw_grid = !draw_grid;
					break;
				default:
					rc = false;
			}
		}

		else if (ev->type == ev_keyup)
		{
			rc = false;
			switch (ev->data1)
			{
				case AM_PANRIGHTKEY:
					if (!followplayer)
					{
						m_keydown[0] = false;
						AM_setWindowPanning();
					}
					break;
				case AM_PANLEFTKEY:
					if (!followplayer)
					{
						m_keydown[1] = false;
						AM_setWindowPanning();
					}
					break;
				case AM_PANUPKEY:
					if (!followplayer)
					{
						m_keydown[2] = false;
						AM_setWindowPanning();
					}
					break;
				case AM_PANDOWNKEY:
					if (!followplayer)
					{
						m_keydown[3] = false;
						AM_setWindowPanning();
					}
					break;
				case AM_ZOOMOUTKEY:
				case AM_ZOOMINKEY:
					mtof_zoommul = FRACUNIT;
					ftom_zoommul = FRACUNIT;
					break;
			}
		}
	}

	return rc;
}

/** Makes a zooming change take effect.
  */
static inline void AM_changeWindowScale(void)
{
	// Change the scaling multipliers
	scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

	if (scale_mtof < min_scale_mtof)
		AM_minOutWindowScale();
	else if (scale_mtof > max_scale_mtof)
		AM_maxOutWindowScale();
	else
		AM_activateNewScale();
}

static inline void AM_doFollowPlayer(void)
{
	if (f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
	{
		m_x = FTOM(MTOF(plr->mo->x >> FRACTOMAPBITS)) - m_w/2;
		m_y = FTOM(MTOF(plr->mo->y >> FRACTOMAPBITS)) - m_h/2;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
		f_oldloc.x = plr->mo->x;
		f_oldloc.y = plr->mo->y;
	}
}

/** Updates automap on a game tic, while the automap is enabled.
  */
void AM_Ticker(void)
{
	if (!cht_debug)
		AM_Stop();

	if (dedicated || !automapactive)
		return;

	if (followplayer)
		AM_doFollowPlayer();

	// Change the zoom if necessary
	if (ftom_zoommul != FRACUNIT)
		AM_changeWindowScale();

	// Change x,y location
	if (m_paninc.x || m_paninc.y)
		AM_changeWindowLoc();
}

/** Clears the automap framebuffer.
  *
  * \param color Color to erase to.
  */
static void AM_clearFB(INT32 color)
{
	V_DrawFill(f_x, f_y, f_w, f_h, color|V_NOSCALESTART);
}

/** Performs automap clipping of lines.
  * Based on Cohen-Sutherland clipping algorithm but with a slightly
  * faster reject and precalculated slopes. If the speed is needed,
  * use a hash algorithm to handle the common cases.
  *
  * \param ml Line to clip.
  * \param fl Resulting framebuffer coordinates?
  * \return True if the line is inside the boundaries.
  */
static boolean AM_clipMline(const mline_t *ml, fline_t *fl)
{
	enum
	{
		LEFT   = 1,
		RIGHT  = 2,
		BOTTOM = 4,
		TOP    = 8
	};

	register INT32 outcode1 = 0, outcode2 = 0, outside;
	fpoint_t tmp ={0,0};
	INT32 dx, dy;

#define DOOUTCODE(oc, mx, my) \
	(oc) = 0; \
	if ((my) < 0) (oc) |= TOP; \
	else if ((my) >= f_h) (oc) |= BOTTOM; \
	if ((mx) < 0) (oc) |= LEFT; \
	else if ((mx) >= f_w) (oc) |= RIGHT;

	// do trivial rejects and outcodes
	if (ml->a.y > m_y2)
		outcode1 = TOP;
	else if (ml->a.y < m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > m_y2)
		outcode2 = TOP;
	else if (ml->b.y < m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > m_x2)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return false; // trivially outside

	// transform to frame-buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return false;

	while (outcode1 | outcode2)
	{
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
			tmp.y = 0;
		}
		else if (outside & BOTTOM)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
			tmp.y = f_h-1;
		}
		else if (outside & RIGHT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
			tmp.x = f_w-1;
		}
		else if (outside & LEFT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
			tmp.x = 0;
		}

		if (outside == outcode1)
		{
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		}
		else
		{
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}

		if (outcode1 & outcode2)
			return false; // trivially outside
	}

	return true;
}
#undef DOOUTCODE

//
// Draws a pixel.
//
static void AM_drawPixel(INT32 xx, INT32 yy, INT32 cc)
{
	if (xx < 0 || yy < 0 || xx >= f_w || yy >= f_h)
		return; // off the screen
	am_buf[(yy*f_w) + xx] = cc;
}

//
// Classic Bresenham w/ whatever optimizations needed for speed
//
static void AM_drawFline_soft(const fline_t *fl, INT32 color)
{
	INT32 x, y, dx, dy, sx, sy, ax, ay, d;

#ifdef _DEBUG
	static INT32 num = 0;

	// For debugging only
	if (fl->a.x < 0 || fl->a.x >= f_w
	|| fl->a.y < 0 || fl->a.y >= f_h
	|| fl->b.x < 0 || fl->b.x >= f_w
	|| fl->b.y < 0 || fl->b.y >= f_h)
	{
		CONS_Debug(DBG_RENDER, "line clipping problem %d\n", num++);
		return;
	}
#endif

	dx = fl->b.x - fl->a.x;
	ax = 2 * (dx < 0 ? -dx : dx);
	sx = dx < 0 ? -1 : 1;

	dy = fl->b.y - fl->a.y;
	ay = 2 * (dy < 0 ? -dy : dy);
	sy = dy < 0 ? -1 : 1;

	x = fl->a.x;
	y = fl->a.y;

	if (ax > ay)
	{
		d = ay - ax/2;
		for (;;)
		{
			AM_drawPixel(x, y, color);
			if (x == fl->b.x)
				return;
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay/2;
		for (;;)
		{
			AM_drawPixel(x, y, color);
			if (y == fl->b.y)
				return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

//
// Clip lines, draw visible parts of lines.
//
static void AM_drawMline(const mline_t *ml, INT32 color)
{
	static fline_t fl;

	if (AM_clipMline(ml, &fl))
		AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
}

//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
static void AM_drawGrid(INT32 color)
{
	fixed_t x, y;
	fixed_t start, end;
	mline_t ml;
	fixed_t gridsize = (MAPBLOCKUNITS<<MAPBITS);

	// Figure out start of vertical gridlines
	start = m_x;
	if ((start - (bmaporgx>>FRACTOMAPBITS)) % gridsize)
		start += gridsize - ((start - (bmaporgx>>FRACTOMAPBITS)) % gridsize);
	end = m_x + m_w;

	// draw vertical gridlines
	ml.a.y = m_y;
	ml.b.y = m_y + m_h;
	for (x = start; x < end; x += gridsize)
	{
		ml.a.x = x;
		ml.b.x = x;
		AM_drawMline(&ml, color);
	}

	// Figure out start of horizontal gridlines
	start = m_y;
	if ((start - (bmaporgy>>FRACTOMAPBITS)) % gridsize)
		start += gridsize - ((start - (bmaporgy>>FRACTOMAPBITS)) % gridsize);
	end = m_y + m_h;

	// draw horizontal gridlines
	ml.a.x = m_x;
	ml.b.x = m_x + m_w;
	for (y = start; y < end; y += gridsize)
	{
		ml.a.y = y;
		ml.b.y = y;
		AM_drawMline(&ml, color);
	}
}

#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		end1 = (P_GetZAt(slope, lines[i].v1->x, lines[i].v1->y, normalheight) + FRACUNIT/2) >> FRACBITS; \
		end2 = (P_GetZAt(slope, lines[i].v2->x, lines[i].v2->y, normalheight) + FRACUNIT/2) >> FRACBITS;

static ffloor_t *AM_CompareFOFs(size_t i, ffloor_t *rover, ffloor_t *secondarystore)
{
	ffloor_t *secondaryrover = NULL;

	for (; rover; rover = rover->next)
	{
		fixed_t rovt1, rovt2;
		fixed_t rovb1, rovb2;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;
		if (!(rover->fofflags & FOF_BLOCKPLAYER))
			continue;

		SLOPEPARAMS(*rover->t_slope, rovt1, rovt2, *rover->topheight)
		SLOPEPARAMS(*rover->b_slope, rovb1, rovb2, *rover->bottomheight)

		for (secondaryrover = secondarystore; secondaryrover; secondaryrover = secondaryrover->next)
		{
			fixed_t sect1, sect2;
			fixed_t secb1, secb2;

			terrain_t *terrain1 = NULL;
			terrain_t *terrain2 = NULL;

			if (!(secondaryrover->fofflags & FOF_EXISTS))
				continue;
			if (!(secondaryrover->fofflags & FOF_BLOCKPLAYER))
				continue;
			if (secondaryrover->secnum == rover->secnum)
				break;

			SLOPEPARAMS(*secondaryrover->t_slope, sect1, sect2, *secondaryrover->topheight)
			SLOPEPARAMS(*secondaryrover->b_slope, secb1, secb2, *secondaryrover->bottomheight)

			if (rovt1 != sect1)
				continue;
			if (rovt2 != sect2)
				continue;
			if (rovb1 != secb1)
				continue;
			if (rovb2 != secb2)
				continue;

			if (sectors[rover->secnum].damagetype != sectors[secondaryrover->secnum].damagetype
				|| sectors[rover->secnum].friction != sectors[secondaryrover->secnum].friction
				|| sectors[rover->secnum].offroad != sectors[secondaryrover->secnum].offroad)
				continue;

			if (*rover->toppic != *secondaryrover->toppic)
			{
				terrain1 = K_GetTerrainForFlatNum(*rover->toppic);
				terrain2 = K_GetTerrainForFlatNum(*secondaryrover->toppic);
			}
			else if (*rover->bottompic != *secondaryrover->bottompic)
			{
				terrain1 = K_GetTerrainForFlatNum(*rover->bottompic);
				terrain2 = K_GetTerrainForFlatNum(*secondaryrover->bottompic);
			}

			if ((terrain1 && K_TerrainHasAffect(terrain1, false))
				|| (terrain2 && K_TerrainHasAffect(terrain2, false)))
				continue;

			break;
		}

		if (secondaryrover == NULL)
			break;
	}

	return rover;
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
static void AM_drawWalls(UINT8 pass)
{
	size_t i;
	static mline_t l;
	const fixed_t maxstep = P_BaseStepUp()>>FRACBITS;
	fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
	fixed_t backf1 = 0, backf2 = 0, backc1 = 0, backc2 = 0; // back floor ceiling ends

	for (i = 0; i < numlines; i++)
	{
		l.a.x = lines[i].v1->x >> FRACTOMAPBITS;
		l.a.y = lines[i].v1->y >> FRACTOMAPBITS;
		l.b.x = lines[i].v2->x >> FRACTOMAPBITS;
		l.b.y = lines[i].v2->y >> FRACTOMAPBITS;

		SLOPEPARAMS(lines[i].frontsector->f_slope, frontf1, frontf2, lines[i].frontsector->floorheight)
		SLOPEPARAMS(lines[i].frontsector->c_slope, frontc1, frontc2, lines[i].frontsector->ceilingheight)

		if (!lines[i].backsector) // 1-sided
		{
			if (!(pass & PASS_SOLID))
				;
			else if (frontf1 != frontc1 || frontf2 != frontc2 || lines[i].frontsector->f_slope)
			{
				AM_drawMline(&l, TSWALLCOLORS);
			}
			else if (!am_minigen)
			{
				AM_drawMline(&l, WALLCOLORS);
			}
			continue;
		}

		SLOPEPARAMS(lines[i].backsector->f_slope, backf1,  backf2,  lines[i].backsector->floorheight)
		SLOPEPARAMS(lines[i].backsector->c_slope, backc1,  backc2,  lines[i].backsector->ceilingheight)

		if ((backf1 == backc1 && backf2 == backc2) // Back is thok barrier
				 || (frontf1 == frontc1 && frontf2 == frontc2)) // Front is thok barrier
		{
			if (backf1 == backc1 && backf2 == backc2
				&& frontf1 == frontc1 && frontf2 == frontc2) // BOTH are thok barriers
			{
				if (!am_minigen && (pass & PASS_INTANGIBLE))
				{
					AM_drawMline(&l, GRIDCOLORS);
				}
			}
			else if (pass & PASS_SOLID)
			{
				AM_drawMline(&l, TSWALLCOLORS);
			}
		}
		else
		{
			if (lines[i].flags & (ML_IMPASSABLE|ML_BLOCKPLAYERS))
			{
				if (pass & PASS_SOLID)
					AM_drawMline(&l, TSWALLCOLORS); // Completely solid course boundary
			}
			else if ((lines[i].flags & ML_MIDSOLID)
				&& sides[lines->sidenum[0]].midtexture)
			{
				if (pass & PASS_SOLID)
					AM_drawMline(&l, TSWALLCOLORS); // solid midtexture, likely a course boundary
			}
			if ((backf1 != frontf1 && abs(backf1 - frontf1) > maxstep)
				|| (backf2 != frontf2 && abs(backf2 - frontf2) > maxstep))
			{
				if (pass & PASS_SOLID)
					AM_drawMline(&l, TSWALLCOLORS); // floor-wall, likely a course boundary
			}
			else if (lines[i].special == 2001)
			{
				if (pass & PASS_SOLID)
					AM_drawMline(&l, TSFINISHLINE); // finish line
			}
			else if (P_IsLineTripWire(&lines[i]))
			{
				if (pass & PASS_SOLID)
					AM_drawMline(&l, TSTRIPWIRE); // tripwire shortcut
			}
			else if (backf1 != frontf1 || backf2 != frontf2)
			{
				if (pass & PASS_INTANGIBLE)
					AM_drawMline(&l, FDWALLCOLORS); // floorlevel change
			}
			else if (backc1 != frontc1 || backc2 != frontc2)
			{
				if (!(pass & PASS_INTANGIBLE))
					;
				else if (abs(backc1 - frontc1) < maxstep
					|| abs(backc2 - frontc2) < maxstep)
				{
					AM_drawMline(&l, CDWALLCOLORS); // ceilinglevel change
				}
				else
				{
					AM_drawMline(&l, GRIDCOLORS); // ceiling-wall, not likely to be a course boundary but flagged up just in case
				}
			}
			else if (lines[i].frontsector->damagetype != lines[i].backsector->damagetype
				|| lines[i].frontsector->friction != lines[i].backsector->friction
				|| lines[i].frontsector->offroad != lines[i].backsector->offroad)
			{
				if (pass & PASS_INTANGIBLE)
					AM_drawMline(&l, FDWALLCOLORS); // Functionality boundary
			}
			else
			{
				terrain_t *terrain1 = NULL;
				terrain_t *terrain2 = NULL;
				UINT8 defercol = GRIDCOLORS;

				if (lines[i].frontsector->floorpic != lines[i].backsector->floorpic)
				{
					terrain1 = K_GetTerrainForFlatNum(lines[i].frontsector->floorpic);
					terrain2 = K_GetTerrainForFlatNum(lines[i].backsector->floorpic);
					defercol = FDWALLCOLORS; // possible floor offroad boundary
				}
				else if (lines[i].frontsector->ceilingpic != lines[i].backsector->ceilingpic)
				{
					terrain1 = K_GetTerrainForFlatNum(lines[i].frontsector->ceilingpic);
					terrain2 = K_GetTerrainForFlatNum(lines[i].backsector->ceilingpic);
					defercol = CDWALLCOLORS; // possible ceiling offroad boundary
				}

				if ((terrain1 && K_TerrainHasAffect(terrain1, false))
					|| (terrain2 && K_TerrainHasAffect(terrain2, false)))
				{
					if (pass & PASS_INTANGIBLE)
						AM_drawMline(&l, defercol); // Yep, definitely a functionality boundary
				}
				else
				{
					ffloor_t *rover = NULL;

					if (lines[i].frontsector->ffloors || lines[i].backsector->ffloors)
					{
						if (lines[i].backsector->ffloors == NULL)
						{
							// Check frontside for one solid
							for (rover = lines[i].frontsector->ffloors; rover; rover = rover->next)
							{
								if (!(rover->fofflags & FOF_EXISTS))
									continue;
								if (!(rover->fofflags & FOF_BLOCKPLAYER))
									continue;
								break;
							}
						}
						else if (lines[i].frontsector->ffloors == NULL)
						{
							// Check backside for one solid
							for (rover = lines[i].backsector->ffloors; rover; rover = rover->next)
							{
								if (!(rover->fofflags & FOF_EXISTS))
									continue;
								if (!(rover->fofflags & FOF_BLOCKPLAYER))
									continue;
								break;
							}
						}
						else
						{
							// Check to see if any secnums exist in one but not the other.
							rover = AM_CompareFOFs(i, lines[i].frontsector->ffloors, lines[i].backsector->ffloors);
							if (rover == NULL)
								rover = AM_CompareFOFs(i, lines[i].backsector->ffloors, lines[i].frontsector->ffloors);
						}
					}

					if (rover != NULL)
					{
						if (pass & PASS_FOF)
							AM_drawMline(&l, TSFOFINFO); // a FOF is here but we don't know how to distinguish them yet
					}
					else if (!am_minigen)
					{
						if (pass & PASS_INTANGIBLE)
							AM_drawMline(&l, GRIDCOLORS); // likely low-relevance line
					}
				}
			}
		}
	}
}

#undef SLOPEPARAMS

//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
static void AM_rotate(fixed_t *x, fixed_t *y, angle_t a)
{
	fixed_t tmpx;

	tmpx = FixedMul(*x, FINECOSINE(a>>ANGLETOFINESHIFT))
		- FixedMul(*y, FINESINE(a>>ANGLETOFINESHIFT));

	*y = FixedMul(*x, FINESINE(a>>ANGLETOFINESHIFT))
		+ FixedMul(*y, FINECOSINE(a>>ANGLETOFINESHIFT));

	*x = tmpx;
}

static void AM_drawLineCharacter(const mline_t *lineguy, size_t lineguylines, fixed_t scale, angle_t angle,
	INT32 color, fixed_t x, fixed_t y)
{
	size_t i;
	mline_t l;

	for (i = 0; i < lineguylines; i++)
	{
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale)
		{
			l.a.x = FixedMul(scale, l.a.x);
			l.a.y = FixedMul(scale, l.a.y);
		}

		if (angle)
			AM_rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale)
		{
			l.b.x = FixedMul(scale, l.b.x);
			l.b.y = FixedMul(scale, l.b.y);
		}

		if (angle)
			AM_rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		l.a.x >>= FRACTOMAPBITS;
		l.a.y >>= FRACTOMAPBITS;
		l.b.x >>= FRACTOMAPBITS;
		l.b.y >>= FRACTOMAPBITS;

		AM_drawMline(&l, color);
	}
}

static inline void AM_drawPlayers(void)
{
	INT32 i;
	player_t *p;
	INT32 color = GREENS;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		p = &players[i];
		if (p->skincolor > 0)
			color = R_GetTranslationColormap(TC_DEFAULT, p->skincolor, GTC_CACHE)[GREENS + 8];

		AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 16<<FRACBITS, p->mo->angle, color, p->mo->x, p->mo->y);
	}
}

static inline void AM_drawThings(UINT8 colors)
{
	size_t i;
	mobj_t *t;

	for (i = 0; i < numsectors; i++)
	{
		t = sectors[i].thinglist;
		while (t)
		{
			if (!t->player)
				AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES, 16<<FRACBITS, t->angle, colors, t->x, t->y);
			t = t->snext;
		}
	}
}

static inline void AM_drawSpecialThingsOnly(UINT8 colors)
{
	size_t i;

	for (i = 0; i < numsectors; i++)
	{
		terrain_t *terrain = NULL;
		mobj_t *t = NULL;

		if (sectors[i].damagetype != 0
		|| sectors[i].friction < ORIG_FRICTION
		|| sectors[i].offroad != 0)
			continue;

		terrain = K_GetTerrainForFlatNum(sectors[i].floorpic);
		if (!terrain)
			terrain = K_GetTerrainForFlatNum(sectors[i].ceilingpic);

		if (terrain && K_TerrainHasAffect(terrain, true))
			continue;

		t = sectors[i].thinglist;
		while (t)
		{
			if (t->type == MT_RANDOMITEM
			|| t->type == MT_PAPERITEMSPOT
			|| t->type == MT_OVERTIME_CENTER
			|| t->type == MT_RING
			|| t->type == MT_BLUESPHERE
			|| t->type == MT_WAYPOINT
			|| t->type == MT_ITEMCAPSULE
			|| t->flags & MF_SPRING)
				AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES, 16<<FRACBITS, t->angle, colors, t->x, t->y);
			t = t->snext;
		}
	}
}

/** Draws the crosshair.
  *
  * \param color Color for the crosshair.
  */
static inline void AM_drawCrosshair(UINT8 color)
{
	const fixed_t scale = 4<<FRACBITS;
	size_t i;
	fline_t fl;

	for (i = 0; i < NUMCROSSMARKLINES; i++)
	{
		fl.a.x = FixedMul(cross_mark[i].a.x, scale) >> FRACBITS;
		fl.a.y = FixedMul(cross_mark[i].a.y, scale) >> FRACBITS;
		fl.b.x = FixedMul(cross_mark[i].b.x, scale) >> FRACBITS;
		fl.b.y = FixedMul(cross_mark[i].b.y, scale) >> FRACBITS;

		fl.a.x += f_x + (f_w / 2);
		fl.a.y += f_y + (f_h / 2);
		fl.b.x += f_x + (f_w / 2);
		fl.b.y += f_y + (f_h / 2);

		AM_drawFline(&fl, color);
	}
}

/** Draws the automap.
  */
void AM_Drawer(void)
{
	if (!automapactive)
		return;

	AM_drawFline = AM_drawFline_soft;
#ifdef HWRENDER
	if (rendermode == render_opengl)
		AM_drawFline = HWR_drawAMline;
#endif

	AM_clearFB(BACKGROUND);
	if (draw_grid) AM_drawGrid(GRIDCOLORS);
	AM_drawWalls(PASS_FOF|PASS_INTANGIBLE|PASS_SOLID);
	AM_drawThings(THINGCOLORS);
	AM_drawPlayers();

	if (!followplayer) AM_drawCrosshair(XHAIRCOLORS);
}

minigen_t *AM_MinimapGenerate(INT32 mul)
{
	static minigen_t ret = {0};

	if (automapactive)
		return NULL;

	am_minigen = true;

	AM_drawFline = AM_drawFline_soft; // yes, even in GL

	//AM_FrameBufferInit();
	f_x = f_y = 0;
	ret.w = f_w = minimapinfo.minimap_w * mul;
	ret.h = f_h = minimapinfo.minimap_h * mul;
	am_buf = ret.buf = NULL;

	//AM_LevelInit();
	AM_findMinMaxBoundaries();
	scale_mtof = min_scale_mtof - (min_scale_mtof/20);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

	//AM_initVariables();
	f_oldloc.x = INT32_MAX;

	m_paninc.x = m_paninc.y = 0;
	ftom_zoommul = FRACUNIT;
	mtof_zoommul = FRACUNIT;

	m_w = FTOM(f_w);
	m_h = FTOM(f_h);

	//AM_changeWindowLoc();
	m_x = min_x - FixedMul(m_w, FRACUNIT/40);
	m_y = min_y - FixedMul(m_h, FRACUNIT/40);
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;

	// for saving & restoring
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;

	am_buf = ret.buf = malloc((f_w*f_h));

	if (ret.buf == NULL)
		return NULL;

	//AM_clearFB(BACKGROUND);
	memset(am_buf, 0xff, (f_w*f_h));
	AM_drawSpecialThingsOnly(BACKGROUND);
	AM_drawWalls(PASS_FOF);
	AM_drawWalls(PASS_INTANGIBLE);
	AM_drawWalls(PASS_SOLID);

	am_buf = NULL;
	am_recalc = true;

	am_minigen = false;

	return &ret;
}
