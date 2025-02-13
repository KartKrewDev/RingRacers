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
/// \file  r_plane.h
/// \brief Refresh, visplane stuff (floor, ceilings)

#ifndef __R_PLANE__
#define __R_PLANE__

#ifdef __cplusplus
extern "C" {
#endif

#include "screen.h" // needs MAXVIDWIDTH/MAXVIDHEIGHT
#include "r_data.h"
#include "r_defs.h"
#include "r_textures.h"
#include "p_polyobj.h"

#define VISPLANEHASHBITS 9
#define VISPLANEHASHMASK ((1<<VISPLANEHASHBITS)-1)
// the last visplane list is outside of the hash table and is used for fof planes
#define MAXVISPLANES ((1<<VISPLANEHASHBITS)+1)

//
// Now what is a visplane, anyway?
// Simple: kinda floor/ceiling polygon optimised for SRB2 rendering.
//
struct visplane_t
{
	visplane_t *next;

	fixed_t height;
	fixed_t viewx, viewy, viewz;
	angle_t viewangle;
	angle_t plangle;
	INT32 picnum;
	INT32 lightlevel;
	INT32 minx, maxx;

	// colormaps per sector
	extracolormap_t *extra_colormap;

	// leave pads for [minx-1]/[maxx+1]
	UINT16 padtopstart, top[MAXVIDWIDTH], padtopend;
	UINT16 padbottomstart, bottom[MAXVIDWIDTH], padbottomend;
	INT32 high, low; // R_PlaneBounds should set these.

	fixed_t xoffs, yoffs; // Scrolling flats.

	ffloor_t *ffloor;
	polyobj_t *polyobj;
	pslope_t *slope;

	boolean noencore;
	boolean ripple;
	sectordamage_t damage;
};

extern visplane_t *visplanes[MAXVISPLANES];
extern visplane_t *floorplane;
extern visplane_t *ceilingplane;

// Visplane related.
extern INT16 *lastopening, *openings;
extern size_t maxopenings;

extern INT16 floorclip[MAXVIDWIDTH], ceilingclip[MAXVIDWIDTH];
extern fixed_t frontscale[MAXVIDWIDTH];
extern fixed_t yslopetab[MAXSPLITSCREENPLAYERS][MAXVIDHEIGHT*16];

extern fixed_t *yslope;

void R_InitPlanes(void);
void R_ClearPlanes(void);
void R_ClearFFloorClips (void);

void R_DrawPlanes(void);
visplane_t *R_FindPlane(fixed_t height, INT32 picnum, INT32 lightlevel, fixed_t xoff, fixed_t yoff, angle_t plangle,
	extracolormap_t *planecolormap, ffloor_t *ffloor, polyobj_t *polyobj, pslope_t *slope, boolean noencore,
	boolean ripple, boolean reverseLight, const sector_t *lighting_sector, sectordamage_t damage);
visplane_t *R_CheckPlane(visplane_t *pl, INT32 start, INT32 stop);
void R_ExpandPlane(visplane_t *pl, INT32 start, INT32 stop);
void R_PlaneBounds(visplane_t *plane);

size_t R_FlatDimensionsFromLumpSize(size_t size);
void R_CheckFlatLength(drawspandata_t* ds, size_t size);
boolean R_CheckPowersOfTwo(drawspandata_t* ds);

// Draws a single visplane.
void R_DrawSinglePlane(drawspandata_t* ds, visplane_t *pl, boolean allow_parallel);

// Calculates the slope vectors needed for tilted span drawing.
void R_SetSlopePlane(drawspandata_t* ds, pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle);
void R_SetScaledSlopePlane(drawspandata_t* ds, pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xs, fixed_t ys, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle);
void R_CalculateSlopeVectors(drawspandata_t* ds);

// Sets the slope vector pointers for the current tilted span.
void R_SetTiltedSpan(drawspandata_t* ds, INT32 span);

// Returns a palette index or -1 if not highlighted
INT16 R_PlaneIsHighlighted(const visplane_t *pl);

struct visffloor_t
{
	visplane_t *plane;
	fixed_t height;
	fixed_t f_pos; // F for Front sector
	fixed_t b_pos; // B for Back sector
	fixed_t f_frac, f_step;
	fixed_t b_frac, b_step;
	INT16 f_clip[MAXVIDWIDTH];
	INT16 c_clip[MAXVIDWIDTH];

	// For slope rendering; the height at the other end
	fixed_t f_pos_slope;
	fixed_t b_pos_slope;

	pslope_t *slope;

	ffloor_t *ffloor;
	polyobj_t *polyobj;
};

extern visffloor_t ffloor[MAXFFLOORS];
extern INT32 numffloors;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
