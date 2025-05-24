// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"

#define MORPH_TIME_FACTOR (3)

#define morph_target(o) ((o)->target)
#define morph_time(o) ((o)->extravalue1)

void
Obj_BeginDropTargetMorph
(		mobj_t * target,
		skincolornum_t color)
{
	mobj_t *x = P_SpawnMobjFromMobj(target, 0, 0, 0,
			MT_DROPTARGET_MORPH);

	x->color = color;
	x->colorized = true;
	x->renderflags |= RF_FULLBRIGHT;

	P_SetTarget(&morph_target(x), target);

	morph_time(x) = tr_trans90 * MORPH_TIME_FACTOR;

	x->health = target->health;
}

boolean
Obj_DropTargetMorphThink (mobj_t *x)
{
	mobj_t *target = morph_target(x);

	if (P_MobjWasRemoved(target))
	{
		P_RemoveMobj(x);
		return false;
	}

	if (target->health != x->health)
	{
		P_RemoveMobj(x);
		return false;
	}

	morph_time(x)--;

	if (morph_time(x) <= 0)
	{
		target->health--;
		target->color = x->color;

		P_RemoveMobj(x);
		return false;
	}

	x->renderflags = (x->renderflags & ~(RF_TRANSMASK))
		| ((morph_time(x) / MORPH_TIME_FACTOR) << RF_TRANSSHIFT);

	P_MoveOrigin(x, target->x, target->y, target->z);

	return true;
}
