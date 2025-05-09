// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_sight.c
/// \brief Line of sight/visibility checks, uses REJECT lookup table

#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"
#include "p_slopes.h"
#include "r_main.h"
#include "r_state.h"

#include "k_bot.h" // K_BotHatesThisSector
#include "k_kart.h" // K_TripwirePass

//
// P_CheckSight
//
// killough 4/19/98:
// Convert LOS info to struct for reentrancy and efficiency of data locality

typedef struct
{
	fixed_t sightzstart, t2x, t2y;		// eye z of looker
	divline_t strace;					// from t1 to t2
	fixed_t topslope, bottomslope;		// slopes to top and bottom of target
	fixed_t bbox[4];

	mobj_t *t1, *t2;
	boolean alreadyHates;				// For bot traversal, for if the bot is already in a sector it doesn't want to be
	UINT8 traversed;
} los_t;

typedef boolean (*los_init_t)(mobj_t *, mobj_t *, register los_t *);
typedef boolean (*los_valid_t)(seg_t *, divline_t *, register los_t *);
typedef boolean (*los_valid_poly_t)(polyobj_t *, divline_t *, register los_t *);

typedef struct
{
	los_init_t init;					// Initialization function. If true, we'll continue with checking across linedefs. If false, end early with failure.
	los_valid_t validate;				// Validation function. If true, continue iterating for possible success. If false, end early with failure.
	los_valid_poly_t validatePolyobj;	// If not NULL, then we will also check polyobject lines using this func.
} los_funcs_t;

static INT32 sightcounts[2];

#ifdef DEVELOP
extern consvar_t cv_debugtraversemax;
#undef TRAVERSE_MAX
#define TRAVERSE_MAX (cv_debugtraversemax.value)
#endif

//
// P_DivlineSide
//
// Returns side 0 (front), 1 (back), or 2 (on).
//
// killough 4/19/98: made static, cleaned up

static INT32 P_DivlineSide(fixed_t x, fixed_t y, const divline_t *node)
{
	fixed_t left, right;
	return
		!node->dx ? x == node->x ? 2 : x <= node->x ? node->dy > 0 : node->dy < 0 :
		!node->dy ? y == node->y ? 2 : y <= node->y ? node->dx < 0 : node->dx > 0 :
		(right = ((y - node->y) >> FRACBITS) * (node->dx >> FRACBITS)) <
		(left  = ((x - node->x) >> FRACBITS) * (node->dy >> FRACBITS)) ? 0 :
		right == left ? 2 : 1;
}

static INT32 P_DivlineCrossed(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, const divline_t *node)
{
	return (P_DivlineSide(x1, y1, node) == P_DivlineSide(x2, y2, node));
}

static boolean P_IsVisiblePolyObj(polyobj_t *po, divline_t *divl, register los_t *los)
{
	sector_t *polysec = po->lines[0]->backsector;
	fixed_t frac;
	fixed_t topslope, bottomslope;

	if (!(po->flags & POF_RENDERALL))
	{
		return true; // the polyobject isn't visible, so we can ignore it
	}

	// stop because it is not two sided
	/*
	if (!(po->flags & POF_TESTHEIGHT))
	{
		return false;
	}
	*/

	frac = P_InterceptVector(&los->strace, divl);

	// get slopes of top and bottom of this polyobject line
	topslope = FixedDiv(polysec->ceilingheight - los->sightzstart , frac);
	bottomslope = FixedDiv(polysec->floorheight - los->sightzstart , frac);

	if (topslope >= los->topslope && bottomslope <= los->bottomslope)
	{
		// view completely blocked
		return false;
	}

	// TODO: figure out if it's worth considering partially blocked cases or not?
	// maybe to adjust los's top/bottom slopes if needed
	/*
	if (los->topslope <= los->bottomslope)
	{
		return false;
	}
	*/

	return true;
}

static boolean P_CrossSubsecPolyObj(polyobj_t *po, register los_t *los, register los_funcs_t *funcs)
{
	size_t i;

	for (i = 0; i < po->numLines; ++i)
	{
		line_t *line = po->lines[i];
		divline_t divl;
		const vertex_t *v1,*v2;

		// already checked other side?
		if (line->validcount == validcount)
			continue;

		line->validcount = validcount;

		// OPTIMIZE: killough 4/20/98: Added quick bounding-box rejection test
		if (line->bbox[BOXLEFT  ] > los->bbox[BOXRIGHT ] ||
			line->bbox[BOXRIGHT ] < los->bbox[BOXLEFT  ] ||
			line->bbox[BOXBOTTOM] > los->bbox[BOXTOP   ] ||
			line->bbox[BOXTOP]    < los->bbox[BOXBOTTOM])
			continue;

		v1 = line->v1;
		v2 = line->v2;

		// line isn't crossed?
		if (P_DivlineCrossed(v1->x, v1->y, v2->x, v2->y, &los->strace))
			continue;

		divl.dx = v2->x - (divl.x = v1->x);
		divl.dy = v2->y - (divl.y = v1->y);

		// line isn't crossed?
		if (P_DivlineCrossed(los->strace.x, los->strace.y, los->t2x, los->t2y, &divl))
			continue;

		if (funcs->validatePolyobj(po, &divl, los) == false)
		{
			return false;
		}
	}

	return true;
}

static boolean P_IsVisible(seg_t *seg, divline_t *divl, register los_t *los)
{
	line_t *line = seg->linedef;
	fixed_t popentop, popenbottom;
	const sector_t *front, *back;
	fixed_t frac;
	fixed_t fracx, fracy;
	fixed_t frontf, backf, frontc, backc;

	// stop because it is not two sided anyway
	if (!(line->flags & ML_TWOSIDED))
	{
		return false;
	}

	// calculate fractional intercept (how far along we are divided by how far we are from t2)
	frac = P_InterceptVector(&los->strace, divl);

	front = seg->frontsector;
	back  = seg->backsector;
	// calculate position at intercept
	fracx = los->strace.x + FixedMul(los->strace.dx, frac);
	fracy = los->strace.y + FixedMul(los->strace.dy, frac);
	// calculate sector heights
	frontf = P_GetSectorFloorZAt  (front, fracx, fracy);
	frontc = P_GetSectorCeilingZAt(front, fracx, fracy);
	backf  = P_GetSectorFloorZAt  (back , fracx, fracy);
	backc  = P_GetSectorCeilingZAt(back , fracx, fracy);
	// crosses a two sided line
	// no wall to block sight with?
	if (frontf == backf && frontc == backc
	&& !front->ffloors & !back->ffloors) // (and no FOFs)
	{
		return true;
	}

	// possible occluder
	// because of ceiling height differences
	popentop = min(frontc, backc);

	// because of floor height differences
	popenbottom = max(frontf, backf);

	// quick test for totally closed doors
	if (popenbottom >= popentop)
	{
		return false;
	}

	if (frontf != backf)
	{
		fixed_t slope = FixedDiv(popenbottom - los->sightzstart , frac);
		if (slope > los->bottomslope)
			los->bottomslope = slope;
	}

	if (frontc != backc)
	{
		fixed_t slope = FixedDiv(popentop - los->sightzstart , frac);
		if (slope < los->topslope)
			los->topslope = slope;
	}

	if (los->topslope <= los->bottomslope)
	{
		return false;
	}

	// Monster Iestyn: check FOFs!
	if (front->ffloors || back->ffloors)
	{
		ffloor_t *rover;
		fixed_t topslope, bottomslope;
		fixed_t topz, bottomz;
		// check front sector's FOFs first
		for (rover = front->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS)
				|| !(rover->fofflags & FOF_RENDERSIDES) || (rover->fofflags & (FOF_TRANSLUCENT|FOF_FOG)))
			{
				continue;
			}

			topz    = P_GetFFloorTopZAt   (rover, fracx, fracy);
			bottomz = P_GetFFloorBottomZAt(rover, fracx, fracy);
			topslope    = FixedDiv(   topz - los->sightzstart, frac);
			bottomslope = FixedDiv(bottomz - los->sightzstart, frac);

			if (topslope >= los->topslope && bottomslope <= los->bottomslope)
			{
				return false; // view completely blocked
			}
		}
		// check back sector's FOFs as well
		for (rover = back->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS)
				|| !(rover->fofflags & FOF_RENDERSIDES) || (rover->fofflags & (FOF_TRANSLUCENT|FOF_FOG)))
			{
				continue;
			}

			topz    = P_GetFFloorTopZAt   (rover, fracx, fracy);
			bottomz = P_GetFFloorBottomZAt(rover, fracx, fracy);
			topslope    = FixedDiv(   topz - los->sightzstart, frac);
			bottomslope = FixedDiv(bottomz - los->sightzstart, frac);

			if (topslope >= los->topslope && bottomslope <= los->bottomslope)
			{
				return false; // view completely blocked
			}
		}
		// TODO: figure out if it's worth considering partially blocked cases or not?
		// maybe to adjust los's top/bottom slopes if needed
	}

	return true;
}

static boolean P_CanTraceBlockingLine(seg_t *seg, divline_t *divl, register los_t *los)
{
	line_t *line = seg->linedef;

	(void)divl;

	if (!(line->flags & ML_TWOSIDED))
	{
		// stop because it is not two sided anyway
		return false;
	}

	if (P_IsLineBlocking(line, los->t1) == true)
	{
		// This line will always block us
		return false;
	}

	if (los->t1->player != NULL)
	{
		if (P_IsLineTripWire(line) == true && K_TripwirePass(los->t1->player) == false)
		{
			// Can't go through trip wire.
			return false;
		}
	}

	return true;
}

static boolean P_CanBotTraverse(seg_t *seg, divline_t *divl, register los_t *los)
{
	const boolean flip = ((los->t1->eflags & MFE_VERTICALFLIP) == MFE_VERTICALFLIP);
	line_t *line = seg->linedef;
	fixed_t frac = 0;
	boolean canStepUp, canDropOff;
	fixed_t maxstep = 0;
	opening_t open = {0};

	if (P_CanTraceBlockingLine(seg, divl, los) == false)
	{
		// Blocked, so obviously can't traverse either.
		return false;
	}

	// calculate fractional intercept (how far along we are divided by how far we are from t2)
	frac = P_InterceptVector(&los->strace, divl);

	// calculate position at intercept
	g_tm.x = los->strace.x + FixedMul(los->strace.dx, frac);
	g_tm.y = los->strace.y + FixedMul(los->strace.dy, frac);

	// set openrange, opentop, openbottom
	open.fofType = (flip ? LO_FOF_CEILINGS : LO_FOF_FLOORS);
	P_LineOpening(line, los->t1, &open);
	maxstep = P_GetThingStepUp(los->t1, g_tm.x, g_tm.y);

	if (open.range < los->t1->height)
	{
		// Can't fit
		return false;
	}

	// If we can step up...
	canStepUp = ((flip ? (open.highceiling - open.ceiling) : (open.floor - open.lowfloor)) <= maxstep);

	// Or if we're on the higher side...
	canDropOff = (flip ? (los->t1->z + los->t1->height <= open.ceiling) : (los->t1->z >= open.floor));

	if (canStepUp || canDropOff)
	{
		if (los->t1->player != NULL && los->alreadyHates == false)
		{
			// Treat damage / offroad sectors like walls.
			UINT8 side = P_DivlineSide(los->t2x, los->t2y, divl) & 1;
			sector_t *sector = (side == 1) ? seg->backsector : seg->frontsector;

			if (K_BotHatesThisSector(los->t1->player, sector, g_tm.x, g_tm.y))
			{
				// This line does not block us, but we don't want to cross it regardless.
				return false;
			}
		}

		return true;
	}

	los->traversed++;
	return (los->traversed < TRAVERSE_MAX);
}

static boolean P_CanWaypointTraverse(seg_t *seg, divline_t *divl, register los_t *los)
{
	const boolean flip = ((los->t1->eflags & MFE_VERTICALFLIP) == MFE_VERTICALFLIP);
	line_t *line = seg->linedef;
	fixed_t frac = 0;
	boolean canStepUp, canDropOff;
	fixed_t maxstep = 0;
	opening_t open = {0};

	if (P_CanTraceBlockingLine(seg, divl, los) == false)
	{
		// Blocked, so obviously can't traverse either.
		return false;
	}

	if (line->special == 2001)
	{
		// Don't allow through the finish linedef.
		// Causes some janky behavior.
		return false;
	}

	// calculate fractional intercept (how far along we are divided by how far we are from t2)
	frac = P_InterceptVector(&los->strace, divl);

	// calculate position at intercept
	g_tm.x = los->strace.x + FixedMul(los->strace.dx, frac);
	g_tm.y = los->strace.y + FixedMul(los->strace.dy, frac);

	// set openrange, opentop, openbottom
	open.fofType = (flip ? LO_FOF_CEILINGS : LO_FOF_FLOORS);
	P_LineOpening(line, los->t1, &open);
	maxstep = P_GetThingStepUp(los->t1, g_tm.x, g_tm.y);

#if 0
	if (los->t2->type == MT_WAYPOINT)
	{
		waypoint_t *wp = K_SearchWaypointHeapForMobj(los->t2);

		if (wp != NULL)
		{
			CONS_Printf(
				"========\nID: %d\nrange: %.2f >= %.2f\n",
				K_GetWaypointID(wp),
				FIXED_TO_FLOAT(open.range),
				FIXED_TO_FLOAT(los->t1->height)
			);

			if (open.range >= los->t1->height)
			{
				CONS_Printf(
					"floor: %.2f\nlowfloor: %.2f\nstep: %.2f <= %.2f\n",
					FIXED_TO_FLOAT(open.floor),
					FIXED_TO_FLOAT(open.lowfloor),
					FIXED_TO_FLOAT(open.floor - open.lowfloor),
					FIXED_TO_FLOAT(maxstep)
				);
			}
		}
	}
#endif

	if (open.range < los->t1->height)
	{
		// Can't fit
		return false;
	}

	// If we can step up...
	canStepUp = ((flip ? (open.highceiling - open.ceiling) : (open.floor - open.lowfloor)) <= maxstep);

	// Or if we're on the higher side...
	canDropOff = (flip ? (los->t1->z + los->t1->height <= open.ceiling) : (los->t1->z >= open.floor));

	if (canStepUp || canDropOff)
	{
		return true;
	}

	los->traversed++;
	return (los->traversed < TRAVERSE_MAX);
}

//
// P_CrossSubsector
//
// Returns true if strace crosses the given subsector successfully.
//
static boolean P_CrossSubsector(size_t num, register los_t *los, register los_funcs_t *funcs)
{
	seg_t *seg;
	INT32 count;

	if (num >= numsubsectors)
	{
		CONS_Debug(DBG_RENDER, "P_CrossSubsector: ss %s with numss = %s\n", sizeu1(num), sizeu2(numsubsectors));
		return true;
	}

	// haleyjd 02/23/06: this assignment should be after the above check
	seg = segs + subsectors[num].firstline;

	// haleyjd 02/23/06: check polyobject lines
	if (funcs->validatePolyobj != NULL)
	{
		polyobj_t *po;

		if ((po = subsectors[num].polyList))
		{
			while (po)
			{
				if (po->validcount != validcount)
				{
					po->validcount = validcount;
					if (!P_CrossSubsecPolyObj(po, los, funcs))
						return false;
				}
				po = (polyobj_t *)(po->link.next);
			}
		}
	}

	for (count = subsectors[num].numlines; --count >= 0; seg++)  // check lines
	{
		line_t *line = seg->linedef;
		const vertex_t *v1, *v2;
		divline_t divl;

		if (seg->glseg)
			continue;

		// already checked other side?
		if (line->validcount == validcount)
			continue;

		line->validcount = validcount;

		// OPTIMIZE: killough 4/20/98: Added quick bounding-box rejection test
		if (line->bbox[BOXLEFT  ] > los->bbox[BOXRIGHT ] ||
			line->bbox[BOXRIGHT ] < los->bbox[BOXLEFT  ] ||
			line->bbox[BOXBOTTOM] > los->bbox[BOXTOP   ] ||
			line->bbox[BOXTOP]    < los->bbox[BOXBOTTOM])
			continue;

		v1 = line->v1;
		v2 = line->v2;

		// line isn't crossed?
		if (P_DivlineCrossed(v1->x, v1->y, v2->x, v2->y, &los->strace))
			continue;

		divl.dx = v2->x - (divl.x = v1->x);
		divl.dy = v2->y - (divl.y = v1->y);

		// line isn't crossed?
		if (P_DivlineCrossed(los->strace.x, los->strace.y, los->t2x, los->t2y, &divl))
			continue;

		if (funcs->validate(seg, &divl, los) == false)
		{
			return false;
		}
	}

	// passed the subsector ok
	return true;
}

//
// P_CrossBSPNode
// Returns true
//  if strace crosses the given node successfully.
//
// killough 4/20/98: rewritten to remove tail recursion, clean up, and optimize
// cph - Made to use R_PointOnSide instead of P_DivlineSide, since the latter
//  could return 2 which was ambigous, and the former is
//  better optimised; also removes two casts :-)

static boolean P_CrossBSPNode(INT32 bspnum, register los_t *los, register los_funcs_t *funcs)
{
	while (!(bspnum & NF_SUBSECTOR))
	{
		register node_t *bsp = nodes + bspnum;
		INT32 side = R_PointOnSide(los->strace.x, los->strace.y, bsp);
		INT32 side2 = R_PointOnSide(los->t2x, los->t2y, bsp);

		if (side == side2)
		{
			// doesn't touch the other side
			bspnum = bsp->children[side];
		}
		else
		{
			// the partition plane is crossed here
			if (!P_CrossBSPNode(bsp->children[side], los, funcs))
			{
				return false;  // cross the starting side
			}
			else
			{
				bspnum = bsp->children[side ^ 1];  // cross the ending side
			}
		}
	}

	return P_CrossSubsector((bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR), los, funcs);
}

static boolean P_InitCheckSight(mobj_t *t1, mobj_t *t2, register los_t *los)
{
	const sector_t *s1, *s2;

	// An unobstructed LOS is possible.
	// Now look from eyes of t1 to any part of t2.
	sightcounts[1]++;

	// Prevent SOME cases of looking through 3dfloors
	//
	// This WILL NOT work for things like 3d stairs with monsters behind
	// them - they will still see you! TODO: Fix.
	//
	s1 = t1->subsector->sector;
	s2 = t2->subsector->sector;

	if (s1 == s2) // Both sectors are the same.
	{
		ffloor_t *rover;
		fixed_t topz1, bottomz1; // top, bottom heights at t1's position
		fixed_t topz2, bottomz2; // likewise but for t2

		for (rover = s1->ffloors; rover; rover = rover->next)
		{
			// Allow sight through water, fog, etc.
			/// \todo Improve by checking fog density/translucency
			/// and setting a sight limit.
			if (!(rover->fofflags & FOF_EXISTS)
				|| !(rover->fofflags & FOF_RENDERPLANES) /*|| (rover->fofflags & (FOF_TRANSLUCENT|FOF_FOG))*/)
			{
				continue;
			}

			topz1    = P_GetFFloorTopZAt   (rover, t1->x, t1->y);
			topz2    = P_GetFFloorTopZAt   (rover, t2->x, t2->y);
			bottomz1 = P_GetFFloorBottomZAt(rover, t1->x, t1->y);
			bottomz2 = P_GetFFloorBottomZAt(rover, t2->x, t2->y);

			// Check for blocking floors here.
			if ((los->sightzstart < bottomz1 && t2->z >= topz2)
				|| (los->sightzstart >= topz1 && t2->z + t2->height < bottomz2))
			{
				// no way to see through that
				return false;
			}

			if (rover->fofflags & FOF_SOLID)
				continue; // shortcut since neither mobj can be inside the 3dfloor

			if (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES))
			{
				if (los->sightzstart >= topz1 && t2->z + t2->height < topz2)
					return false; // blocked by upper outside plane

				if (los->sightzstart < bottomz1 && t2->z >= bottomz2)
					return false; // blocked by lower outside plane
			}

			if (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES)
			{
				if (los->sightzstart < topz1 && t2->z >= topz2)
					return false; // blocked by upper inside plane

				if (los->sightzstart >= bottomz1 && t2->z + t2->height < bottomz2)
					return false; // blocked by lower inside plane
			}
		}
	}

	return true;
}

static boolean P_InitTraceBotTraversal(mobj_t *t1, mobj_t *t2, register los_t *los)
{
	(void)t2;

	if (t1->player != NULL)
	{
		los->alreadyHates = K_BotHatesThisSector(
			t1->player, t1->subsector->sector,
			t1->x, t1->y
		);
	}
	else
	{
		los->alreadyHates = false;
	}

	return true;
}

static boolean P_CompareMobjsAcrossLines(mobj_t *t1, mobj_t *t2, register los_funcs_t *funcs)
{
	los_t los;
	const sector_t *s1, *s2;
	size_t pnum;

	// First check for trivial rejection.
	if (P_MobjWasRemoved(t1) == true || P_MobjWasRemoved(t2) == true)
	{
		return false;
	}

	if (!t1->subsector || !t2->subsector
		|| !t1->subsector->sector || !t2->subsector->sector)
	{
		return false;
	}

	s1 = t1->subsector->sector;
	s2 = t2->subsector->sector;
	pnum = (s1-sectors)*numsectors + (s2-sectors);

	if (rejectmatrix != NULL)
	{
		// Check in REJECT table.
		if (rejectmatrix[pnum>>3] & (1 << (pnum&7))) // can't possibly be connected
		{
			return false;
		}
	}

	// killough 11/98: shortcut for melee situations
	// same subsector? obviously visible
	// haleyjd 02/23/06: can't do this if there are polyobjects in the subsec
	if (!t1->subsector->polyList &&
		t1->subsector == t2->subsector)
	{
		return true;
	}

	validcount++;

	los.t1 = t1;
	los.t2 = t2;
	los.alreadyHates = false;
	los.traversed = 0;

	los.topslope =
		(los.bottomslope = t2->z - (los.sightzstart =
			t1->z + t1->height -
			(t1->height>>2))) + t2->height;
	los.strace.dx = (los.t2x = t2->x) - (los.strace.x = t1->x);
	los.strace.dy = (los.t2y = t2->y) - (los.strace.y = t1->y);

	if (t1->x > t2->x)
		los.bbox[BOXRIGHT] = t1->x, los.bbox[BOXLEFT] = t2->x;
	else
		los.bbox[BOXRIGHT] = t2->x, los.bbox[BOXLEFT] = t1->x;

	if (t1->y > t2->y)
		los.bbox[BOXTOP] = t1->y, los.bbox[BOXBOTTOM] = t2->y;
	else
		los.bbox[BOXTOP] = t2->y, los.bbox[BOXBOTTOM] = t1->y;

	if (funcs->init != NULL)
	{
		if (funcs->init(t1, t2, &los) == false)
		{
			return false;
		}
	}

	// The only required function.
	I_Assert(funcs->validate != NULL);

	// the head node is the last node output
	return P_CrossBSPNode((INT32)numnodes - 1, &los, funcs);
}

//
// P_CheckSight
//
// Returns true if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
boolean P_CheckSight(mobj_t *t1, mobj_t *t2)
{
	los_funcs_t funcs = {0};

	funcs.init = &P_InitCheckSight;
	funcs.validate = &P_IsVisible;
	funcs.validatePolyobj = &P_IsVisiblePolyObj;

	return P_CompareMobjsAcrossLines(t1, t2, &funcs);
}

boolean P_TraceBlockingLines(mobj_t *t1, mobj_t *t2)
{
	los_funcs_t funcs = {0};

	funcs.validate = &P_CanTraceBlockingLine;

	return P_CompareMobjsAcrossLines(t1, t2, &funcs);
}

boolean P_TraceBotTraversal(mobj_t *t1, mobj_t *t2)
{
	los_funcs_t funcs = {0};

	funcs.init = &P_InitTraceBotTraversal;
	funcs.validate = &P_CanBotTraverse;

	return P_CompareMobjsAcrossLines(t1, t2, &funcs);
}

boolean P_TraceWaypointTraversal(mobj_t *t1, mobj_t *t2)
{
	los_funcs_t funcs = {0};

	funcs.validate = &P_CanWaypointTraverse;

	return P_CompareMobjsAcrossLines(t1, t2, &funcs);
}
