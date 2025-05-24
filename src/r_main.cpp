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
/// \file  r_main.c
/// \brief Rendering main loop and setup functions,
///        utility functions (BSP, geometry, trigonometry).
///        See tables.c, too.

#include <algorithm>

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h" // faB(21jan): testing
#include "r_sky.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "k_menu.h"
#include "am_map.h"
#include "d_main.h"
#include "v_video.h"
//#include "p_spec.h"
#include "p_setup.h"
#include "z_zone.h"
#include "m_random.h" // quake camera shake
#include "r_portal.h"
#include "r_main.h"
#include "i_system.h" // I_GetPreciseTime
#include "doomstat.h" // MAXSPLITSCREENPLAYERS
#include "r_fps.h" // Frame interpolation/uncapped
#include "core/thread_pool.h"
#include "m_misc.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
INT64 mycount;
INT64 mytotal = 0;
//unsigned long  nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------

extern "C" consvar_t cv_debugrender_visplanes;

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// increment every time a check is made
size_t validcount = 1;

INT32 centerx, centery;

fixed_t centerxfrac, centeryfrac;
fixed_t projection[MAXSPLITSCREENPLAYERS];
fixed_t projectiony[MAXSPLITSCREENPLAYERS]; // aspect ratio
fixed_t fovtan[MAXSPLITSCREENPLAYERS]; // field of view
fixed_t g_fovcache[MAXSPLITSCREENPLAYERS];

// just for profiling purposes
size_t framecount;

size_t loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle, viewroll;
UINT8 viewssnum;
fixed_t viewcos, viewsin;
sector_t *viewsector;
player_t *viewplayer;
mobj_t *r_viewmobj;

int r_splitscreen;

fixed_t rendertimefrac;
fixed_t rendertimefrac_unpaused;
fixed_t renderdeltatics;
boolean renderisnewtic;

//
// precalculated math tables
//
angle_t clipangle[MAXSPLITSCREENPLAYERS];
angle_t doubleclipangle[MAXSPLITSCREENPLAYERS];

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
INT32 viewangletox[MAXSPLITSCREENPLAYERS][FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[MAXSPLITSCREENPLAYERS][MAXVIDWIDTH+1];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// Hack to support extra boom colormaps.
extracolormap_t *extra_colormaps = NULL;

// Render stats
precise_t ps_prevframetime = 0;
precise_t ps_rendercalltime = 0;
precise_t ps_uitime = 0;
precise_t ps_swaptime = 0;

precise_t ps_bsptime = 0;

precise_t ps_sw_spritecliptime = 0;
precise_t ps_sw_portaltime = 0;
precise_t ps_sw_planetime = 0;
precise_t ps_sw_maskedtime = 0;

int ps_numbspcalls = 0;
int ps_numsprites = 0;
int ps_numdrawnodes = 0;
int ps_numpolyobjects = 0;

struct RenderStats g_renderstats;

void SplitScreen_OnChange(void)
{
	UINT8 i;

	/*
	local splitscreen is updated before you're in a game,
	so this is the first value for renderer splitscreen
	*/
	r_splitscreen = splitscreen;

	// recompute screen size
	R_ExecuteSetViewSize();

	if (!demo.playback)
	{
		for (i = 1; i < MAXSPLITSCREENPLAYERS; i++)
		{
			if (i > splitscreen)
				CL_RemoveSplitscreenPlayer(displayplayers[i]);
			else
				CL_AddSplitscreenPlayer();
		}

		if (server && !netgame)
			multiplayer = splitscreen;
	}
	else
	{
		for (i = 1; i < MAXSPLITSCREENPLAYERS; i++)
			displayplayers[i] = consoleplayer;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && i != consoleplayer)
			{
				UINT8 j;
				for (j = 1; j < MAXSPLITSCREENPLAYERS; j++)
				{
					if (displayplayers[j] == consoleplayer)
					{
						displayplayers[j] = i;
						break;
					}
				}

				if (j == MAXSPLITSCREENPLAYERS)
					break;
			}
		}
	}
}
extern "C" void Fov_OnChange(void);
void Fov_OnChange(void)
{
	R_CheckFOV();
}

extern "C" void ChaseCam_OnChange(void);
void ChaseCam_OnChange(void)
{
	;
}

extern "C" void ChaseCam2_OnChange(void);
void ChaseCam2_OnChange(void)
{
	;
}

extern "C" void ChaseCam3_OnChange(void);
void ChaseCam3_OnChange(void)
{
	;
}

extern "C" void ChaseCam4_OnChange(void);
void ChaseCam4_OnChange(void)
{
	;
}

//
// R_PointOnSide
// Traverse BSP (sub) tree,
// check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//
INT32 R_PointOnSide(fixed_t x, fixed_t y, node_t *node)
{
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;

	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;

	x -= node->x;
	y -= node->y;

	// Try to quickly decide by looking at sign bits.
	// also use a mask to avoid branch prediction
	INT32 mask = (node->dy ^ node->dx ^ x ^ y) >> 31;
	return (mask & ((node->dy ^ x) < 0)) |  // (left is negative)
		(~mask & (FixedMul(y, node->dx>>FRACBITS) >= FixedMul(node->dy>>FRACBITS, x)));
}

// killough 5/2/98: reformatted
INT32 R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
	fixed_t lx = line->v1->x;
	fixed_t ly = line->v1->y;
	fixed_t ldx = line->v2->x - lx;
	fixed_t ldy = line->v2->y - ly;

	if (!ldx)
		return x <= lx ? ldy > 0 : ldy < 0;

	if (!ldy)
		return y <= ly ? ldx < 0 : ldx > 0;

	x -= lx;
	y -= ly;

	// Try to quickly decide by looking at sign bits.
	if ((ldy ^ ldx ^ x ^ y) < 0)
		return (ldy ^ x) < 0;          // (left is negative)
	return FixedMul(y, ldx>>FRACBITS) >= FixedMul(ldy>>FRACBITS, x);
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return R_PointToAngle2(viewx, viewy, x, y);
}

// Similar to R_PointToAngle, but requires an additional player_t argument.
// If this player is a local displayplayer, this will base off the calculations off of their camera instead, otherwise use viewx/viewy as usual.
// Yes this is kinda ghetto.
angle_t R_PointToAnglePlayer(player_t *player, fixed_t x, fixed_t y)
{
	fixed_t refx = viewx, refy = viewy;
	camera_t *cam = NULL;
	UINT8 i;

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (player == &players[displayplayers[i]])
		{
			cam = &camera[i];
			break;
		}
	}

	// use whatever cam we found's coordinates.
	if (cam != NULL)
	{
		refx = cam->x;
		refy = cam->y;

		// Bandaid for two very specific bugs that arise with chasecam off.
		// 1: Camera tilt from slopes wouldn't apply correctly in first person.
		// 2: Trick pies would appear strangely in first person.
		if (player->mo)
		{
			if ((!cam->chase) && player->mo->x == x && player->mo->y == y)
			{
				return player->mo->angle;
			}
		}
	}
	return R_PointToAngle2(refx, refy, x, y);
}

// This version uses 64-bit variables to avoid overflows with large values.
// Currently used only by OpenGL rendering.
angle_t R_PointToAngle64(INT64 x, INT64 y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDivEx(y,x)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x,y)] :                               // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDivEx(y,x)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x,y)] :                              // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDivEx(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x,y)] :                             // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDivEx(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x,y)] :                              // octant 5
		0;
}

angle_t R_PointToAngle2(fixed_t pviewx, fixed_t pviewy, fixed_t x, fixed_t y)
{
	return (y -= pviewy, (x -= pviewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1)
{
	return FixedHypot(px1 - px2, py1 - py2);
}

// Little extra utility. Works in the same way as R_PointToAngle2
fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	return R_PointToDist2(viewx, viewy, x, y);
}

angle_t R_PointToAngleEx(INT32 x2, INT32 y2, INT32 x1, INT32 y1)
{
	INT64 dx = x1-x2;
	INT64 dy = y1-y2;
	if (dx < INT32_MIN || dx > INT32_MAX || dy < INT32_MIN || dy > INT32_MAX)
	{
		x1 = (int)(dx / 2 + x2);
		y1 = (int)(dy / 2 + y2);
	}
	return (y1 -= y2, (x1 -= x2) || y1) ?
	x1 >= 0 ?
	y1 >= 0 ?
		(x1 > y1) ? tantoangle[SlopeDivEx(y1,x1)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x1,y1)] :                               // octant 1
		x1 > (y1 = -y1) ? 0-tantoangle[SlopeDivEx(y1,x1)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x1,y1)] :                              // octant 7
		y1 >= 0 ? (x1 = -x1) > y1 ? ANGLE_180-tantoangle[SlopeDivEx(y1,x1)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x1,y1)] :                             // octant 2
		(x1 = -x1) > (y1 = -y1) ? ANGLE_180+tantoangle[SlopeDivEx(y1,x1)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x1,y1)] :                              // octant 5
		0;
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
// killough 5/2/98: reformatted, cleaned up
//
// note: THIS IS USED ONLY FOR WALLS!
fixed_t R_ScaleFromGlobalAngle(angle_t visangle)
{
	angle_t anglea = ANGLE_90 + (visangle-viewangle);
	angle_t angleb = ANGLE_90 + (visangle-rw_normalangle);
	fixed_t den = FixedMul(rw_distance, FINESINE(anglea>>ANGLETOFINESHIFT));
	// proff 11/06/98: Changed for high-res
	fixed_t num = FixedMul(projectiony[viewssnum],
			FINESINE(angleb>>ANGLETOFINESHIFT));

	if (den > num>>16)
	{
		num = FixedDiv(num, den);
		if (num > 64*FRACUNIT)
			return 64*FRACUNIT;
		if (num < 256)
			return 256;
		return num;
	}
	return 64*FRACUNIT;
}

//
// R_DoCulling
// Checks viewz and top/bottom heights of an item against culling planes
// Returns true if the item is to be culled, i.e it shouldn't be drawn!
// if args[1] is set, the camera view is required to be in the same area for culling to occur
boolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph)
{
	fixed_t cullplane;

	if (!cullheight)
		return false;

	cullplane = cullheight->frontsector->floorheight;
	if (cullheight->args[1]) // Group culling
	{
		if (!viewcullheight)
			return false;

		// Make sure this is part of the same group
		if (viewcullheight->frontsector == cullheight->frontsector)
		{
			// OK, we can cull
			if (vz > cullplane && toph < cullplane) // Cull if below plane
				return true;

			if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
				return true;
		}
	}
	else // Quick culling
	{
		if (vz > cullplane && toph < cullplane) // Cull if below plane
			return true;

		if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
			return true;
	}

	return false;
}

// Returns search dimensions within a blockmap, in the direction of viewangle and out to a certain distance.
void R_GetRenderBlockMapDimensions(fixed_t drawdist, INT32 *xl, INT32 *xh, INT32 *yl, INT32 *yh)
{
	const angle_t left = viewangle - clipangle[viewssnum];
	const angle_t right = viewangle + clipangle[viewssnum];

	const fixed_t vxleft = viewx + FixedMul(drawdist, FCOS(left));
	const fixed_t vyleft = viewy + FixedMul(drawdist, FSIN(left));

	const fixed_t vxright = viewx + FixedMul(drawdist, FCOS(right));
	const fixed_t vyright = viewy + FixedMul(drawdist, FSIN(right));

	// Try to narrow the search to within only the field of view
	*xl = (unsigned)(std::min(viewx, std::min(vxleft, vxright)) - bmaporgx)>>MAPBLOCKSHIFT;
	*xh = (unsigned)(std::max(viewx, std::max(vxleft, vxright)) - bmaporgx)>>MAPBLOCKSHIFT;
	*yl = (unsigned)(std::min(viewy, std::min(vyleft, vyright)) - bmaporgy)>>MAPBLOCKSHIFT;
	*yh = (unsigned)(std::max(viewy, std::max(vyleft, vyright)) - bmaporgy)>>MAPBLOCKSHIFT;

	if (*xh >= bmapwidth)
		*xh = bmapwidth - 1;

	if (*yh >= bmapheight)
		*yh = bmapheight - 1;

	BMBOUNDFIX(*xl, *xh, *yl, *yh);
}

//
// R_InitTextureMapping
//
static void R_InitTextureMapping(int s)
{
	INT32 i;
	INT32 x;
	INT32 t;
	fixed_t focallength;

	// Use tangent table to generate viewangletox:
	//  viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv(projection[s],
		FINETANGENT(FINEANGLES/4+FIELDOFVIEW/2));

	focallengthf[s] = FIXED_TO_FLOAT(focallength);

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > fovtan[s]*2)
			t = -1;
		else if (FINETANGENT(i) < -fovtan[s]*2)
			t = viewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > viewwidth+1)
				t = viewwidth+1;
		}
		viewangletox[s][i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//  xtoviewangle will give the smallest view angle
	//  that maps to x.
	for (x = 0; x <= viewwidth;x++)
	{
		i = 0;
		while (viewangletox[s][i] > x)
			i++;
		xtoviewangle[s][x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (viewangletox[s][i] == -1)
			viewangletox[s][i] = 0;
		else if (viewangletox[s][i] == viewwidth+1)
			viewangletox[s][i]  = viewwidth;
	}

	clipangle[s] = xtoviewangle[s][0];
	doubleclipangle[s] = clipangle[s]*2;
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP 2

static inline void R_InitLightTables(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	INT32 scale;

	// Calculate the light levels to use
	//  for each level / distance combination.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTZ; j++)
		{
			//added : 02-02-98 : use BASEVIDWIDTH, vid.width is not set already,
			// and it seems it needs to be calculated only once.
			scale = FixedDiv((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = startmapl - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			zlight[i][j] = colormaps + level*256;
		}
	}
}

//#define WOUGHMP_WOUGHMP // I got a fish-eye lens - I'll make a rap video with a couple of friends
// it's kinda laggy sometimes

#ifdef WOUGHMP_WOUGHMP
#define AHHHH_IM_SO_MAAAAD { 0U, 0, FRACUNIT, NULL, 0, 0, {0}, {0}, false }
#else
#define AHHHH_IM_SO_MAAAAD { 0U,    FRACUNIT, NULL, 0, 0, {0}, {0}, false }
#endif

static struct viewmorph {
	angle_t rollangle; // pre-shifted by fineshift
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye;
#endif

	fixed_t zoomneeded;
	INT32 *scrmap;
	INT32 scrmapsize;

	INT32 x1; // clip rendering horizontally for efficiency
	INT16 ceilingclip[MAXVIDWIDTH], floorclip[MAXVIDWIDTH];

	boolean use;
} viewmorph[MAXSPLITSCREENPLAYERS] = {
	AHHHH_IM_SO_MAAAAD,
	AHHHH_IM_SO_MAAAAD,
	AHHHH_IM_SO_MAAAAD,
	AHHHH_IM_SO_MAAAAD,
};

#undef AHHHH_IM_SO_MAAAAD

void R_CheckViewMorph(int s)
{
	struct viewmorph * v = &viewmorph[s];

	float zoomfactor, rollcos, rollsin;
	float x1, y1, x2, y2;
	fixed_t temp;
	INT32 end, vx, vy, pos, usedpos;
	INT32 realend;
	INT32 usedx, usedy;

	INT32 width = vid.width;
	INT32 height = vid.height;

	INT32 halfwidth;
	INT32 halfheight;

#ifdef WOUGHMP_WOUGHMP
	float fisheyemap[MAXVIDWIDTH/2 + 1];
#endif

	angle_t rollangle = viewroll;
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye = cv_cam2_turnmultiplier.value; // temporary test value
#endif

	rollangle >>= ANGLETOFINESHIFT;
	rollangle = ((rollangle+2) & ~3) & FINEMASK; // Limit the distinct number of angles to reduce recalcs from angles changing a lot.

#ifdef WOUGHMP_WOUGHMP
	fisheye &= ~0x7FF; // Same
#endif

	if (r_splitscreen > 0)
	{
		height /= 2;

		if (r_splitscreen > 1)
		{
			width /= 2;
		}
	}

	halfwidth = width / 2;
	halfheight = height / 2;

	if (rollangle == v->rollangle &&
#ifdef WOUGHMP_WOUGHMP
		fisheye == v->fisheye &&
#endif
		v->scrmapsize == width * height)
		return; // No change

	v->rollangle = rollangle;
#ifdef WOUGHMP_WOUGHMP
	v->fisheye = fisheye;
#endif

	if (v->rollangle == 0
#ifdef WOUGHMP_WOUGHMP
		 && v->fisheye == 0
#endif
	 )
	{
		v->use = false;
		v->x1 = 0;
		if (v->zoomneeded != FRACUNIT)
			R_SetViewSize();
		v->zoomneeded = FRACUNIT;

		return;
	}

	if (v->scrmapsize != width * height)
	{
		if (v->scrmap)
			free(v->scrmap);
		v->scrmap = static_cast<INT32*>(malloc(width * height * sizeof(INT32)));
		v->scrmapsize = width * height;
	}

	temp = FINECOSINE(rollangle);
	rollcos = FIXED_TO_FLOAT(temp);
	temp = FINESINE(rollangle);
	rollsin = FIXED_TO_FLOAT(temp);

	// Calculate maximum zoom needed
	x1 = (width  * fabsf(rollcos) + height * fabsf(rollsin)) / width;
	y1 = (height * fabsf(rollcos) + width  * fabsf(rollsin)) / height;

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		float f = FIXED_TO_FLOAT(fisheye);
		for (vx = 0; vx <= halfwidth; vx++)
			fisheyemap[vx] = 1.0f / cos(atan(vx * f / halfwidth));

		f = cos(atan(f));
		if (f < 1.0f)
		{
			x1 /= f;
			y1 /= f;
		}
	}
#endif

	temp = std::max(x1, y1)*FRACUNIT;
	if (temp < FRACUNIT)
		temp = FRACUNIT;
	else
		temp |= 0x3FFF; // Limit how many times the viewport needs to be recalculated

	//CONS_Printf("Setting zoom to %f\n", FIXED_TO_FLOAT(temp));

	if (temp != v->zoomneeded)
	{
		v->zoomneeded = temp;
		R_SetViewSize();
	}

	zoomfactor = FIXED_TO_FLOAT(v->zoomneeded);

	realend = end = width * height - 1;

	if (r_splitscreen > 1)
	{
		realend = ( realend << 1 ) - width;
	}

	pos = 0;

	// Pre-multiply rollcos and rollsin to use for positional stuff
	rollcos /= zoomfactor;
	rollsin /= zoomfactor;

	x1 = -(halfwidth * rollcos - halfheight * rollsin);
	y1 = -(halfheight * rollcos + halfwidth * rollsin);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
		v->x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)) * fisheyemap[halfwidth]);
	else
#endif
	v->x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)));
	//CONS_Printf("saving %d cols\n", v->x1);

	// Set ceilingclip and floorclip
	for (vx = 0; vx < width; vx++)
	{
		v->ceilingclip[vx] = height;
		v->floorclip[vx] = -1;
	}
	x2 = x1;
	y2 = y1;
	for (vx = 0; vx < width; vx++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight-1;
		xb = width-1-xa;
		yb = height-1-ya;

		v->ceilingclip[xa] = std::min(v->ceilingclip[xa], ya);
		v->floorclip[xa] = std::max(v->floorclip[xa], ya);
		v->ceilingclip[xb] = std::min(v->ceilingclip[xb], yb);
		v->floorclip[xb] = std::max(v->floorclip[xb], yb);
		x2 += rollcos;
		y2 += rollsin;
	}
	x2 = x1;
	y2 = y1;
	for (vy = 0; vy < height; vy++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight;
		xb = width-1-xa;
		yb = height-1-ya;

		v->ceilingclip[xa] = std::min(v->ceilingclip[xa], ya);
		v->floorclip[xa] = std::max(v->floorclip[xa], ya);
		v->ceilingclip[xb] = std::min(v->ceilingclip[xb], yb);
		v->floorclip[xb] = std::max(v->floorclip[xb], yb);
		x2 -= rollsin;
		y2 += rollcos;
	}

	//CONS_Printf("Top left corner is %f %f\n", x1, y1);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		for (vy = 0; vy < halfheight; vy++)
		{
			x2 = x1;
			y2 = y1;
			x1 -= rollsin;
			y1 += rollcos;

			for (vx = 0; vx < width; vx++)
			{
				usedx = halfwidth + x2*fisheyemap[(int) floorf(fabsf(y2*zoomfactor))];
				usedy = halfheight + y2*fisheyemap[(int) floorf(fabsf(x2*zoomfactor))];

				usedpos = usedx + usedy*width;

				v->scrmap[pos] = usedpos;
				v->scrmap[end-pos] = end-usedpos;

				x2 += rollcos;
				y2 += rollsin;
				pos++;
			}
		}
	}
	else
	{
#endif
	x1 += halfwidth;
	y1 += halfheight;

	for (vy = 0; vy < halfheight; vy++)
	{
		x2 = x1;
		y2 = y1;
		x1 -= rollsin;
		y1 += rollcos;

		for (vx = 0; vx < width; vx++)
		{
			usedx = x2;
			usedy = y2;

			usedpos = usedx + usedy * vid.width;

			v->scrmap[pos] = usedpos;
			v->scrmap[end-pos] = realend-usedpos;

			x2 += rollcos;
			y2 += rollsin;
			pos++;
		}
	}
#ifdef WOUGHMP_WOUGHMP
	}
#endif

	v->use = true;
}

void R_ApplyViewMorph(int s)
{
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	INT32 width = vid.width;
	INT32 height = vid.height;
	INT32 p;
	INT32 end;

	if (!viewmorph[s].use)
		return;

	if (r_splitscreen == 1)
	{
		height /= 2;

		if (s == 1)
		{
			srcscr += vid.width * height;
		}
	}
	else if (r_splitscreen > 1)
	{
		width /= 2;
		height /= 2;

		if (s % 2)
		{
			srcscr += width;
		}

		if (s > 1)
		{
			srcscr += vid.width * height;
		}
	}

	end = width * height;

	for (p = 0; p < end; p++)
	{
		tmpscr[p] = srcscr[viewmorph[s].scrmap[p]];
	}

	VID_BlitLinearScreen(tmpscr, srcscr,
			width*vid.bpp, height, width*vid.bpp, vid.width);
}

angle_t R_ViewRollAngle(const player_t *player, UINT8 viewnum)
{
	angle_t roll = 0;

	if (gamestate != GS_LEVEL)
	{
		// FIXME: The way this is implemented is totally
		// incompatible with cameras that aren't directly
		// tied to the player. (podium, titlemap,
		// MT_ALTVIEWMAN in general)

		// All of these player variables should affect their
		// camera_t in P_MoveChaseCamera, and then this
		// just returns that variable instead.
		return 0;
	}

	roll = player->viewrollangle;

	if (cv_tilting.value)
	{
		if (!player->spectator && !camera[viewnum].freecam)
		{
			roll += player->tilt;
		}
	}

	return roll;
}


//
// R_SetViewSize
// Do not really change anything here,
// because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
boolean setsizeneeded;

void R_SetViewSize(void)
{
	setsizeneeded = true;
}

void R_CheckFOV(void)
{
	for (UINT8 s = 0; s <= r_splitscreen; ++s)
	{
		if (g_fovcache[s] != R_FOV(s))
		{
			R_SetViewSize();
			break;
		}
	}
}

boolean R_ShowHUD(void)
{
	if (g_takemapthumbnail != TMT_NO)
	{
		return false;
	}

	return (boolean)cv_showhud.value;
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
	fixed_t dy;
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	angle_t fov;
	int s;

	setsizeneeded = false;

	if (rendermode == render_none)
		return;

	// status bar overlay
	st_overlay = R_ShowHUD();

	scaledviewwidth = vid.width;
	viewheight = vid.height;

	if (r_splitscreen)
		viewheight >>= 1;

	viewwidth = scaledviewwidth;

	if (r_splitscreen > 1)
	{
		viewwidth >>= 1;
		scaledviewwidth >>= 1;
	}

	centerx = viewwidth/2;
	centery = viewheight/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	for (s = 0; s <= r_splitscreen; ++s)
	{
		g_fovcache[s] = R_FOV(s);
		fov = FixedAngle(g_fovcache[s]/2) + ANGLE_90;
		fovtan[s] = FixedMul(FINETANGENT(fov >> ANGLETOFINESHIFT), viewmorph[s].zoomneeded);
		if (r_splitscreen == 1) // Splitscreen FOV should be adjusted to maintain expected vertical view
			fovtan[s] = 17*fovtan[s]/10;

		projection[s] = projectiony[s] = FixedDiv(centerxfrac, fovtan[s]);

		R_InitTextureMapping(s);

		// planes
		if (rendermode == render_soft)
		{
			// this is only used for planes rendering in software mode
			j = viewheight*16;
			for (i = 0; i < j; i++)
			{
				dy = ((i - viewheight*8)<<FRACBITS);
				dy = FixedMul(abs(dy), fovtan[s]);
				yslopetab[s][i] = FixedDiv(centerx*FRACUNIT, dy);
			}
		}
	}

	R_InitViewBuffer(scaledviewwidth, viewheight);

	// thing clipping
	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (INT16)viewheight;

	// setup sky scaling
	R_SetSkyScale();

	// planes
	if (rendermode == render_soft)
	{
		if (ds_su)
			Z_Free(ds_su);
		if (ds_sv)
			Z_Free(ds_sv);
		if (ds_sz)
			Z_Free(ds_sz);

		ds_su = ds_sv = ds_sz = NULL;
	}

	memset(scalelight, 0xFF, sizeof(scalelight));

	// Calculate the light levels to use for each level/scale combination.
	for (i = 0; i< LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS - 1 - i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmapl - j*vid.width/(viewwidth)/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS - 1;

			scalelight[i][j] = colormaps + level*256;
		}
	}

	// continue to do the software setviewsize as long as we use the reference software view
#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_SetViewSize();
#endif

	am_recalc = true;
}

fixed_t R_FOV(int split)
{
	if (gamestate == GS_TITLESCREEN || gamestate == GS_CEREMONY)
	{
		return 90*FRACUNIT; // standard setting
	}

	return cv_fov[split].value;
}

//
// R_Init
//

void R_Init(void)
{
	// screensize independent
	//I_OutputMsg("\nR_InitData");
	//R_InitData(); -- split to d_main for its own startup steps since it takes AGES
	CONS_Printf("R_InitColormaps()...\n");
	R_InitColormaps();

	//I_OutputMsg("\nR_InitViewBorder");
	R_InitViewBorder();
	R_SetViewSize(); // setsizeneeded is set true

	//I_OutputMsg("\nR_InitPlanes");
	R_InitPlanes();

	// this is now done by SCR_Recalc() at the first mode set
	//I_OutputMsg("\nR_InitLightTables");
	R_InitLightTables();

	//I_OutputMsg("\nR_InitTranslucencyTables\n");
	//R_InitTranslucencyTables();

	R_InitDrawNodes();

	framecount = 0;
}

//
// R_PointInSubsector
//
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
	size_t nodenum = numnodes-1;

	while (!(nodenum & NF_SUBSECTOR))
		nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_PointInSubsectorOrNull, same as above but returns 0 if not in subsector
//
subsector_t *R_PointInSubsectorOrNull(fixed_t x, fixed_t y)
{
	node_t *node;
	INT32 side, i;
	size_t nodenum;
	subsector_t *ret;
	seg_t *seg;

	// single subsector is a special case
	if (numnodes == 0)
		return subsectors;

	nodenum = numnodes - 1;

	while (!(nodenum & NF_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = R_PointOnSide(x, y, node);
		nodenum = node->children[side];
	}

	ret = &subsectors[nodenum & ~NF_SUBSECTOR];
	for (i = 0, seg = &segs[ret->firstline]; i < ret->numlines; i++, seg++)
	{
		if (seg->glseg)
			continue;

		//if (R_PointOnSegSide(x, y, seg)) -- breaks in ogl because polyvertex_t cast over vertex pointers
		if (P_PointOnLineSide(x, y, seg->linedef) != seg->side)
			return 0;
	}

	return ret;
}

//
// R_SetupFrame
//

static void
R_SetupCommonFrame
(		player_t * player,
		subsector_t * subsector)
{
	const UINT8 viewnum = R_GetViewNumber();
	mappoint_t viewPos = { newview->x, newview->y, newview->z };
	mappoint_t offset = { 0, 0, 0 };

	newview->player = player;

	P_DoQuakeOffset(viewnum, &viewPos, &offset);
	newview->x += offset.x;
	newview->y += offset.y;
	newview->z += offset.z;

	newview->roll = R_ViewRollAngle(player, viewnum);

	if (subsector)
		newview->sector = subsector->sector;
	else
		newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;

	R_InterpolateView(rendertimefrac_unpaused);
}

static void R_SetupAimingFrame(int s)
{
	player_t *player = &players[displayplayers[s]];
	camera_t *thiscam = &camera[s];

	if (player->awayview.tics)
	{
		newview->aim = player->awayview.mobj->pitch;
		newview->angle = player->awayview.mobj->angle;
	}
	else if (thiscam && thiscam->chase)
	{
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;
	}
	else if (!demo.playback && player->playerstate != PST_DEAD)
	{
		newview->aim = localaiming[s];
		newview->angle = localangle[s];
	}
	else
	{
		newview->aim = player->aiming;
		newview->angle = player->mo->angle;
	}
}

void R_SetupFrame(int s)
{
	player_t *player = &players[displayplayers[s]];
	camera_t *thiscam = &camera[s];
	boolean chasecam = (cv_chasecam[s].value != 0);

	R_SetViewContext(static_cast<viewcontext_e>(VIEWCONTEXT_PLAYER1 + s));

	if (player->spectator)
	{
		// Free flying spectator uses demo freecam. This
		// requires chasecam to be enabled.
		chasecam = true;
	}

	if (chasecam && (thiscam && !thiscam->chase))
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = true;
	}
	else if (!chasecam)
		thiscam->chase = false;

	newview->sky = false;

	R_SetupAimingFrame(s);

	if (player->awayview.tics)
	{
		// cut-away view stuff
		r_viewmobj = player->awayview.mobj; // should be a MT_ALTVIEWMAN
		I_Assert(r_viewmobj != NULL);

		newview->x = r_viewmobj->x;
		newview->y = r_viewmobj->y;
		newview->z = r_viewmobj->z;

		R_SetupCommonFrame(player, r_viewmobj->subsector);
	}
	else if (chasecam)
	// use outside cam view
	{
		r_viewmobj = NULL;

		newview->x = thiscam->x;
		newview->y = thiscam->y;
		newview->z = thiscam->z + (thiscam->height>>1);

		R_SetupCommonFrame(player, thiscam->subsector);
	}
	else
	// use the player's eyes view
	{
		r_viewmobj = player->mo;
		I_Assert(r_viewmobj != NULL);

		newview->x = r_viewmobj->x;
		newview->y = r_viewmobj->y;
		newview->z = player->viewz;

		R_SetupCommonFrame(player, r_viewmobj->subsector);
	}
}

void R_SkyboxFrame(int s)
{
	player_t *player = &players[displayplayers[s]];
	camera_t *thiscam = &camera[s];

	R_SetViewContext(static_cast<viewcontext_e>(VIEWCONTEXT_SKY1 + s));

	// cut-away view stuff
	newview->sky = true;
	r_viewmobj = player->skybox.viewpoint;
#ifdef PARANOIA
	if (!r_viewmobj)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("R_SkyboxFrame: r_viewmobj null (player %s)", sizeu1(playeri));
	}
#endif

	R_SetupAimingFrame(s);

	newview->angle += r_viewmobj->angle;

	newview->x = r_viewmobj->x;
	newview->y = r_viewmobj->y;
	newview->z = r_viewmobj->z; // 26/04/17: use actual Z position instead of spawnpoint angle!

	if (mapheaderinfo[gamemap-1])
	{
		mapheader_t *mh = mapheaderinfo[gamemap-1];
		mappoint_t campos = { 0, 0, 0 }; // Position of player's actual view point
		mobj_t *center = player->skybox.centerpoint;

		if (player->awayview.tics) {
			campos.x = player->awayview.mobj->x;
			campos.y = player->awayview.mobj->y;
			campos.z = player->awayview.mobj->z;
		} else if (thiscam->chase) {
			campos.x = thiscam->x;
			campos.y = thiscam->y;
			campos.z = thiscam->z + (thiscam->height>>1);
		} else {
			campos.x = player->mo->x;
			campos.y = player->mo->y;
			campos.z = player->viewz;
		}

		// Earthquake effects should be scaled in the skybox
		// (if an axis isn't used, the skybox won't shake in that direction)
		P_DoQuakeOffset(s, &campos, &campos);

		if (center) // Is there a viewpoint?
		{
			fixed_t x = 0, y = 0;
			if (mh->skybox_scalex > 0)
				x = (campos.x - center->x) / mh->skybox_scalex;
			else if (mh->skybox_scalex < 0)
				x = (campos.x - center->x) * -mh->skybox_scalex;

			if (mh->skybox_scaley > 0)
				y = (campos.y - center->y) / mh->skybox_scaley;
			else if (mh->skybox_scaley < 0)
				y = (campos.y - center->y) * -mh->skybox_scaley;

			if (r_viewmobj->angle == 0)
			{
				newview->x += x;
				newview->y += y;
			}
			else if (r_viewmobj->angle == ANGLE_90)
			{
				newview->x -= y;
				newview->y += x;
			}
			else if (r_viewmobj->angle == ANGLE_180)
			{
				newview->x -= x;
				newview->y -= y;
			}
			else if (r_viewmobj->angle == ANGLE_270)
			{
				newview->x += y;
				newview->y -= x;
			}
			else
			{
				angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
				newview->x += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
				newview->y += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
			}
		}
		if (mh->skybox_scalez > 0)
			newview->z += campos.z / mh->skybox_scalez;
		else if (mh->skybox_scalez < 0)
			newview->z += campos.z * -mh->skybox_scalez;
	}

	R_SetupCommonFrame(player, r_viewmobj->subsector);
}

boolean R_ViewpointHasChasecam(player_t *player)
{
	boolean chasecam = false;
	UINT8 i;

	for (i = 0; i <= splitscreen; i++)
	{
		if (player == &players[g_localplayers[i]])
		{
			chasecam = (cv_chasecam[i].value != 0);
			break;
		}
	}

	if (player->playerstate == PST_DEAD || gamestate == GS_TITLESCREEN || player->spectator)
		chasecam = true; // force chasecam on

	return chasecam;
}

boolean R_IsViewpointThirdPerson(player_t *player, boolean skybox)
{
	boolean chasecam = false;

	// Prevent game crash if player is ever invalid.
	if (!player)
		return false;
		
	chasecam = R_ViewpointHasChasecam(player);

	// cut-away view stuff
	if (player->awayview.tics || skybox)
		return chasecam;
	// use outside cam view
	else if (chasecam)
		return true;

	// use the player's eyes view
	return false;
}

static void R_PortalFrame(portal_t *portal)
{
	viewx = portal->viewx;
	viewy = portal->viewy;
	viewz = portal->viewz;

	viewangle = portal->viewangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	portalclipstart = portal->start;
	portalclipend = portal->end;

	if (portal->clipline != -1)
	{
		portalclipline = &lines[portal->clipline];
		portalcullsector = portalclipline->frontsector;
		viewsector = portalclipline->frontsector;
	}
	else
	{
		portalclipline = NULL;
		portalcullsector = NULL;
		viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}
}

static void Mask_Pre (maskcount_t* m)
{
	m->drawsegs[0] = ds_p - drawsegs;
	m->vissprites[0] = visspritecount;
	m->viewx = viewx;
	m->viewy = viewy;
	m->viewz = viewz;
	m->viewsector = viewsector;
}

static void Mask_Post (maskcount_t* m)
{
	m->drawsegs[1] = ds_p - drawsegs;
	m->vissprites[1] = visspritecount;
}

// ================
// R_RenderView
// ================

// viewx, viewy, viewangle, all that good stuff must be set
static void R_RenderViewpoint(maskcount_t* mask, INT32 cachenum)
{
	Mask_Pre(mask);

	curdrawsegs = ds_p;

	R_RenderFirstBSPNode(cachenum);
	R_AddPrecipitationSprites();

	Mask_Post(mask);
}

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

void R_RenderPlayerView(void)
{
	player_t * player = &players[displayplayers[viewssnum]];
	INT32			nummasks	= 1;
	maskcount_t*	masks		= static_cast<maskcount_t*>(malloc(sizeof(maskcount_t)));

	R_SetupFrame(viewssnum);
	framecount++;
	validcount++;

	memset(&g_renderstats, 0, sizeof g_renderstats);

	// Clear buffers.
	R_ClearPlanes();
	if (viewmorph[viewssnum].use)
	{
		portalclipstart = viewmorph[viewssnum].x1;
		portalclipend = viewwidth-viewmorph[viewssnum].x1-1;
		R_PortalClearClipSegs(portalclipstart, portalclipend);
		memcpy(ceilingclip, viewmorph[viewssnum].ceilingclip, sizeof(INT16)*vid.width);
		memcpy(floorclip, viewmorph[viewssnum].floorclip, sizeof(INT16)*vid.width);
	}
	else
	{
		portalclipstart = 0;
		portalclipend = viewwidth;
		R_ClearClipSegs();
	}
	R_ClearDrawSegs();
	R_ClearSprites();
	Portal_InitList();

	// The head node is the last node output.

//profile stuff ---------------------------------------------------------
#ifdef TIMING
	mytotal = 0;
	ProfZeroTimer();
#endif
	ps_numbspcalls = ps_numpolyobjects = ps_numdrawnodes = 0;
	ps_bsptime = I_GetPreciseTime();

	srb2::ThreadPool::Sema tp_sema;
	srb2::g_main_threadpool->begin_sema();
	R_RenderViewpoint(&masks[nummasks - 1], nummasks - 1);

	ps_bsptime = I_GetPreciseTime() - ps_bsptime;
#ifdef TIMING
	RDMSR(0x10, &mycount);
	mytotal += mycount; // 64bit add

	CONS_Debug(DBG_RENDER, "RenderBSPNode: 0x%d %d\n", *((INT32 *)&mytotal + 1), (INT32)mytotal);
#endif
//profile stuff ---------------------------------------------------------

	ps_sw_spritecliptime = I_GetPreciseTime();
	R_ClipSprites(drawsegs, NULL);
	ps_sw_spritecliptime = I_GetPreciseTime() - ps_sw_spritecliptime;


	// Add skybox portals caused by sky visplanes.
	if (cv_skybox.value && player->skybox.viewpoint)
		Portal_AddSkyboxPortals(player);

	// Portal rendering. Hijacks the BSP traversal.
	ps_sw_portaltime = I_GetPreciseTime();
	if (portal_base && !cv_debugrender_portal.value)
	{
		// tp_sema = srb2::g_main_threadpool->end_sema();
		// srb2::g_main_threadpool->notify_sema(tp_sema);
		// srb2::g_main_threadpool->wait_sema(tp_sema);
		// srb2::g_main_threadpool->begin_sema();

		portal_t *portal;

		for(portal = portal_base; portal; portal = portal_base)
		{
			portalrender = portal->pass; // Recursiveness depth.

			R_ClearFFloorClips();

			// Apply the viewpoint stored for the portal.
			R_PortalFrame(portal);

			// Hack in the clipsegs to delimit the starting
			// clipping for sprites and possibly other similar
			// future items.
			R_PortalClearClipSegs(portal->start, portal->end);

			// Hack in the top/bottom clip values for the window
			// that were previously stored.
			Portal_ClipApply(portal);

			validcount++;

			masks = static_cast<maskcount_t*>(realloc(masks, (++nummasks)*sizeof(maskcount_t)));

			portalskipprecipmobjs = portal->isskybox;

			// Render the BSP from the new viewpoint, and clip
			// any sprites with the new clipsegs and window.

			R_RenderViewpoint(&masks[nummasks - 1], nummasks - 1);

			portalskipprecipmobjs = false;

			R_ClipSprites(ds_p - (masks[nummasks - 1].drawsegs[1] - masks[nummasks - 1].drawsegs[0]), portal);

			Portal_Remove(portal);
		}

		// tp_sema = srb2::g_main_threadpool->end_sema();
		// srb2::g_main_threadpool->notify_sema(tp_sema);
		// srb2::g_main_threadpool->wait_sema(tp_sema);
		// srb2::g_main_threadpool->begin_sema();
	}
	ps_sw_portaltime = I_GetPreciseTime() - ps_sw_portaltime;

	ps_sw_planetime = I_GetPreciseTime();
	R_DrawPlanes();
	tp_sema = srb2::g_main_threadpool->end_sema();
	srb2::g_main_threadpool->notify_sema(tp_sema);
	srb2::g_main_threadpool->wait_sema(tp_sema);
	ps_sw_planetime = I_GetPreciseTime() - ps_sw_planetime;

	// draw mid texture and sprite
	// And now 3D floors/sides!
	ps_sw_maskedtime = I_GetPreciseTime();
	R_DrawMasked(masks, nummasks);
	ps_sw_maskedtime = I_GetPreciseTime() - ps_sw_maskedtime;

	if (cv_debugrender_visplanes.value)
	{
		for (INT32 i = 0; i < MAXVISPLANES; i++)
		{
			for (visplane_t* pl = visplanes[i]; pl; pl = pl->next)
			{
				if (pl->minx > pl->maxx)
					continue;
				auto col = [](int x, int top, int bot)
				{
					if (top > bot)
						std::swap(top, bot);
					if (top < 0)
						top = 0;
					if (bot > viewheight-1)
						bot = viewheight-1;
					UINT8* p = &topleft[x + top * vid.width];
					while (top <= bot)
					{
						*p = 35;
						p += vid.width;
						top++;
					}
				};
				auto span = [col](int x, int top, int bot)
				{
					if (top <= bot)
						col(x, top, bot);
				};
				INT32 top = pl->top[pl->minx];
				INT32 bottom = pl->bottom[pl->minx];
				span(pl->minx, top, bottom);
				span(pl->maxx, pl->top[pl->maxx], pl->bottom[pl->maxx]);
				for (INT32 x = pl->minx + 1; x < std::min(pl->maxx, viewwidth); ++x)
				{
					INT32 new_top = pl->top[x];
					INT32 new_bottom = pl->bottom[x];
					if (new_top > new_bottom)
						continue;
					if (top > bottom)
					{
						col(x, new_top, new_top);
						col(x, new_bottom, new_bottom);
					}
					else
					{
						col(x, top, new_top);
						col(x, bottom, new_bottom);
					}
					top = new_top;
					bottom = new_bottom;
				}
			}
		}
	}

	// debugrender_portal: fill portals with red, draw over everything
	if (portal_base && cv_debugrender_portal.value)
	{
		const UINT8 pal = 0x23; // red
		portal_t *portal;

		for(portal = portal_base; portal; portal = portal_base)
		{
			INT32 width = (portal->end - portal->start);
			INT32 i;

			for (i = 0; i < std::min(width, viewwidth); ++i)
			{
				INT32 yl = std::max(portal->ceilingclip[i] + 1, 0);
				INT32 yh = std::min(static_cast<INT32>(portal->floorclip[i]), viewheight);

				for (; yl < yh; ++yl)
				{
					topleft[portal->start + i + (yl * vid.width)] = pal;
				}
			}

			Portal_Remove(portal);
		}
	}

	free(masks);
}

// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

void R_RegisterEngineStuff(void)
{
	// Enough for dedicated server
	if (dedicated)
		return;

	// debugging

	COM_AddDebugCommand("debugrender_highlight", Command_Debugrender_highlight);
}
