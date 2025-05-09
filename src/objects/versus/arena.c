// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  arena.c
/// \brief Boss battle arena logic

#include "../../p_local.h"
#include "../../p_setup.h"
#include "../../m_random.h"
#include "../../k_boss.h"
#include "../../r_main.h" // R_PointToAngle2, R_PointToDist2

boolean VS_ArenaCenterInit(mobj_t *mobj, mapthing_t *mthing)
{
	INT32 dist1 = mthing->thing_args[1]*FRACUNIT;
	INT32 dist2 = mthing->thing_args[2]*FRACUNIT;

	if (dist1 || dist2)
	{
		if (dist1 > dist2)
		{
			INT32 swap = dist1;
			dist1 = dist2;
			dist2 = swap;
		}

		mobj->extravalue1 = dist1;
		mobj->extravalue2 = dist2;
		return true;
	}

	CONS_Alert(CONS_ERROR, "Versus arena of index %d has no max radius.", mthing->thing_args[0]);
	return false;
}

mobj_t *VS_GetArena(INT32 bossindex)
{
	size_t i;

	for (i = 0; i < nummapthings; ++i)
	{
		if (mapthings[i].type != mobjinfo[MT_BOSSARENACENTER].doomednum)
			continue;

		if (mapthings[i].thing_args[0] != bossindex)
			continue;

		if (mapthings[i].mobj == NULL)
			continue;

		return mapthings[i].mobj;
	}

	CONS_Alert(CONS_ERROR, "No Versus arena of index %d found.", bossindex);
	return NULL;
}

fixed_t *VS_PredictAroundArena(mobj_t *arena, mobj_t *movingobject, fixed_t magnitude, angle_t mompoint, fixed_t radiussubtract, boolean forcegoaround, fixed_t radiusdeltafactor)
{
	static fixed_t dest[2] = {0, 0};

	if (P_MobjWasRemoved(arena))
	{
		CONS_Alert(CONS_ERROR, "VS_PredictAroundArena: No Versus arena provided.");
		return dest;
	}

	if (P_MobjWasRemoved(movingobject))
	{
		CONS_Alert(CONS_ERROR, "VS_PredictAroundArena: No moving object provided.");
		return dest;
	}

	fixed_t radius = FixedHypot(movingobject->x - arena->x, movingobject->y - arena->y);
	angle_t basedir = R_PointToAngle2(arena->x, arena->y, movingobject->x, movingobject->y);
	fixed_t radiusdelta = 0;

	const fixed_t minarena = arena->extravalue1 + radiussubtract;
	const fixed_t maxarena = arena->extravalue2 - radiussubtract;

	mompoint -= basedir;

	if (radiusdeltafactor > 0) // for kneecapping the prediction
	{
		boolean clipped = false;

		// Add radius so we can compare against the arena size.
		radiusdelta = radius + P_ReturnThrustX(arena, mompoint, magnitude);

		if (radiusdelta > maxarena)
		{
			radiusdelta = maxarena;
			clipped = true;
		}
		else if (radiusdelta < minarena)
		{
			radiusdelta = minarena;
			clipped = true;
		}

		if (clipped == true && forcegoaround == true)
		{
			mompoint = (mompoint < ANGLE_180) ? ANGLE_90 : ANGLE_270;
		}

		// Subtract the radius so it's usable as a delta!
		radiusdelta -= radius;
	
		if (radiusdelta && radiusdeltafactor != FRACUNIT)
			radiusdelta = FixedDiv(radiusdelta, radiusdeltafactor);
	}

	radius += (radiusdelta/2); // average...
	if (radius > 0)
	{
		fixed_t sidecomponent = P_ReturnThrustY(arena, mompoint, magnitude);
		// percent*(TAU/100)% of circumference = radians*r, so divide by r to reverse that
		fixed_t radians = FixedDiv(sidecomponent, radius);
		basedir += FixedAngle(FixedDiv(360*radians, M_TAU_FIXED)); // converting to degrees along the way.
	}
	radius += (radiusdelta/2); // ...and then finalise!

	dest[0] = arena->x + P_ReturnThrustX(arena, basedir, radius);
	dest[1] = arena->y + P_ReturnThrustY(arena, basedir, radius);
	return dest;
}

fixed_t *VS_RandomPointOnArena(mobj_t *arena, fixed_t radiussubtract)
{
	static fixed_t dest[2] = {0, 0};

	if (P_MobjWasRemoved(arena))
		return dest;

	const fixed_t minarena = arena->extravalue1 + radiussubtract;
	const fixed_t maxarena = arena->extravalue2 - radiussubtract;

	angle_t rand1 = FixedAngle(P_RandomKey(PR_MOVINGTARGET, 360)*FRACUNIT);
	fixed_t rand2 = P_RandomRange(PR_MOVINGTARGET, minarena/FRACUNIT, maxarena/FRACUNIT)*FRACUNIT;
	dest[0] = arena->x + P_ReturnThrustX(arena, rand1, rand2);
	dest[1] = arena->y + P_ReturnThrustY(arena, rand1, rand2);

	return dest;
}
