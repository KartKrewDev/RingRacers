// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_maputl.h
/// \brief map utility functions

#ifndef __P_MAPUTL__
#define __P_MAPUTL__

#include "doomtype.h"
#include "r_defs.h"
#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// P_MAPUTL
//
struct divline_t
{
	fixed_t x, y, dx, dy;
};

struct intercept_t
{
	fixed_t frac; // along trace line
	boolean isaline;
	union
	{
		mobj_t *thing;
		line_t *line;
	} d;
};

typedef boolean (*traverser_t)(intercept_t *in);

boolean P_PathTraverse(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2,
	INT32 pflags, traverser_t ptrav);

#define P_AproxDistance(dx, dy) FixedHypot(dx, dy)
void P_ClosestPointOnLine(fixed_t x, fixed_t y, const line_t *line, vertex_t *result);
void P_ClosestPointOnLineWithinLine(fixed_t x, fixed_t y, line_t *line, vertex_t *result);
void P_ClosestPointOnLine3D(const vector3_t *p, const vector3_t *line, vector3_t *result);
INT32 P_PointOnLineSide(fixed_t x, fixed_t y, const line_t *line);
void P_MakeDivline(const line_t *li, divline_t *dl);
void P_CameraLineOpening(line_t *plinedef, opening_t *open);
fixed_t P_InterceptVector(const divline_t *v2, const divline_t *v1);
INT32 P_BoxOnLineSide(const fixed_t *tmbox, const line_t *ld);
line_t * P_FindNearestLine(const fixed_t x, const fixed_t y, const sector_t *, const INT32 special);
void P_UnsetPrecipThingPosition(precipmobj_t *thing);
void P_SetPrecipitationThingPosition(precipmobj_t *thing);
void P_CreatePrecipSecNodeList(precipmobj_t *thing, fixed_t x,fixed_t y);
void P_HitSpecialLines(mobj_t *thing, fixed_t x, fixed_t y, fixed_t momx, fixed_t momy);

boolean P_GetMidtextureTopBottom(line_t *linedef, fixed_t x, fixed_t y, fixed_t *return_top, fixed_t *return_bottom);

struct opening_t
{
	fixed_t ceiling, floor, range;
	fixed_t lowfloor, highceiling;
	pslope_t *floorslope, *ceilingslope;
	ffloor_t *floorrover, *ceilingrover;
	fixed_t ceilingstep, ceilingdrop;
	fixed_t floorstep, floordrop;
	INT32 ceilingpic, floorpic;
	UINT8 fofType; // LO_FOF_ types for forcing FOF collide
};

struct fofopening_t
{
	fixed_t ceiling;
	fixed_t floor;
	ffloor_t * ceilingrover;
	ffloor_t *   floorrover;
};

#define LO_FOF_ANY		(0)
#define LO_FOF_FLOORS	(1)
#define LO_FOF_CEILINGS	(2)

boolean P_FoFOpening(sector_t *sector, line_t *linedef, mobj_t *mobj, opening_t *open, fofopening_t *fofopen);
void P_LineOpening(line_t *plinedef, mobj_t *mobj, opening_t *open);

typedef enum
{
	BMIT_CONTINUE, // Continue blockmap search
	BMIT_STOP, // End blockmap search with success
	BMIT_ABORT // End blockmap search with failure
} BlockItReturn_t;

boolean P_BlockLinesIterator(INT32 x, INT32 y, BlockItReturn_t(*func)(line_t *));
boolean P_BlockThingsIterator(INT32 x, INT32 y, BlockItReturn_t(*func)(mobj_t *));

#define PT_ADDLINES		(1)
#define PT_ADDTHINGS	(2)

extern divline_t g_trace;

// call your user function for each line of the blockmap in the
// bbox defined by the radius
//boolean P_RadiusLinesCheck(fixed_t radius, fixed_t x, fixed_t y,
//	boolean (*func)(line_t *));

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __P_MAPUTL__
