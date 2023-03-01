// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James R.
// Copyright (C) 2023 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  loop-endpoint.c
/// \brief Sonic loops, start and end points

#include "../doomdef.h"
#include "../k_kart.h"
#include "../taglist.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../p_spec.h"
#include "../r_main.h"
#include "../k_objects.h"

#define end_anchor(o) ((o)->target)

#define center_max_revolution(o) ((o)->threshold)
#define center_alpha(o) ((o)->target)
#define center_beta(o) ((o)->tracer)

static inline boolean
center_has_flip (const mobj_t *center)
{
	return (center->flags2 & MF2_AMBUSH) == MF2_AMBUSH;
}

static inline void
center_set_flip
(		mobj_t * center,
		boolean mode)
{
	center->flags2 = (center->flags2 & ~(MF2_AMBUSH)) |
		((mode != false) * MF2_AMBUSH);
}

#define anchor_center(o) ((o)->target)
#define anchor_other(o) ((o)->tracer)
#define anchor_type(o) ((o)->reactiontime)

static void
set_shiftxy
(		player_t * player,
		const mobj_t * a)
{
	const mobj_t *b = anchor_other(a);

	const fixed_t dx = (b->x - a->x);
	const fixed_t dy = (b->y - a->y);

	const angle_t th =
		(R_PointToAngle2(0, 0, dx, dy) - a->angle);

	const fixed_t adj = FixedMul(
			abs(FCOS(AbsAngle(th - ANGLE_90))),
			FixedHypot(dx, dy)) / 2;

	vector2_t *xy = &player->loop.shift;

	xy->x = FixedMul(FSIN(a->angle), adj);
	xy->y = FixedMul(FCOS(a->angle), adj);
}

static void
measure_clock
(		const mobj_t * center,
		const mobj_t * anchor,
		angle_t * pitch,
		fixed_t * radius)
{
	const fixed_t dx = (anchor->x - center->x);
	const fixed_t dy = (anchor->y - center->y);
	const fixed_t dz = (anchor->z - center->z);

	/* Translate the anchor point to be along a center line.
	   This makes the horizontal position one dimensional
	   relative to the center point. */
	const fixed_t xy = (
			FixedMul(dx, FCOS(anchor->angle)) +
			FixedMul(dy, FSIN(anchor->angle)));

	/* The 3d position of the anchor point is then reduced to
	   two axes and can be measured as an angle. */
	*pitch = R_PointToAngle2(0, 0, xy, dz) + ANGLE_90;
	*radius = FixedHypot(xy, dz);
}

static void
crisscross
(		mobj_t * anchor,
		mobj_t ** target_p,
		mobj_t ** other_p)
{
	P_SetTarget(target_p, anchor);

	if (!P_MobjWasRemoved(*other_p))
	{
		P_SetTarget(&anchor_other(anchor), *other_p);
		P_SetTarget(&anchor_other(*other_p), anchor);
	}
}

static boolean
moving_toward_gate
(		const player_t * player,
		const mobj_t * anchor,
		angle_t pitch)
{
	const fixed_t
		x = player->mo->momx,
		y = player->mo->momy,
		z = player->mo->momz,

		zx = FixedMul(FCOS(anchor->angle), z),
		zy = FixedMul(FSIN(anchor->angle), z),

		co = abs(FCOS(pitch)),
		si = abs(FSIN(pitch)),

		dx = FixedMul(co, x) + FixedMul(si, zx),
		dy = FixedMul(co, y) + FixedMul(si, zy);

	return AngleDelta(anchor->angle,
			R_PointToAngle2(0, 0, dx, dy)) < ANG60;
}

static SINT8
get_binary_direction
(		angle_t pitch,
		mobj_t * toucher)
{
	const fixed_t si = FSIN(pitch);

	if (abs(si) < abs(FCOS(pitch)))
	{
		// pitch = 0 points downward so offset 90 degrees
		// clockwise so 180 occurs at horizon
		return ((pitch + ANGLE_90) < ANGLE_180) ? 1 : -(1);
	}
	else
	{
		return intsign(si) * P_MobjFlip(toucher);
	}
}

mobj_t *
Obj_FindLoopCenter (const mtag_t tag)
{
	INT32 i;

	TAG_ITER_THINGS(tag, i)
	{
		mapthing_t *mt = &mapthings[i];

		if (mt->type == mobjinfo[MT_LOOPCENTERPOINT].doomednum)
		{
			return mt->mobj;
		}
	}

	return NULL;
}

void
Obj_InitLoopEndpoint
(		mobj_t * end,
		mobj_t * anchor)
{
	P_SetTarget(&end_anchor(end), anchor);
}

void
Obj_InitLoopCenter (mobj_t *center)
{
	const mapthing_t *mt = center->spawnpoint;

	center_max_revolution(center) = mt->args[1] * FRACUNIT / 360;
	center_set_flip(center, mt->args[0]);
}

void
Obj_LinkLoopAnchor
(		mobj_t * anchor,
		mobj_t * center,
		UINT8 type)
{
	P_SetTarget(&anchor_center(anchor), center);

	anchor_type(anchor) = type;

	if (!P_MobjWasRemoved(center))
	{
		switch (type)
		{
			case TMLOOP_ALPHA:
				crisscross(anchor,
						&center_alpha(center),
						&center_beta(center));
				break;

			case TMLOOP_BETA:
				crisscross(anchor,
						&center_beta(center),
						&center_alpha(center));
				break;
		}
	}
}

void
Obj_LoopEndpointCollide
(		mobj_t * end,
		mobj_t * toucher)
{
	player_t *player = toucher->player;
	sonicloopvars_t *s = &player->loop;

	mobj_t *anchor = end_anchor(end);
	mobj_t *center = anchor ? anchor_center(anchor) : NULL;

	angle_t pitch;
	fixed_t radius;

	/* predict movement for a smooth transition */
	const fixed_t px = toucher->x + toucher->momx;
	const fixed_t py = toucher->y + toucher->momy;

	SINT8 flip;

	if (P_MobjWasRemoved(center))
	{
		return;
	}

	if (player->loop.radius != 0)
	{
		return;
	}

	measure_clock(center, anchor, &pitch, &radius);

	if (!moving_toward_gate(player, anchor, pitch))
	{
		return;
	}

	if (!P_MobjWasRemoved(anchor_other(anchor)))
	{
		set_shiftxy(player, anchor);
	}

	flip = get_binary_direction(pitch, toucher);

	s->yaw = anchor->angle;

	s->origin.x = center->x - (anchor->x - px);
	s->origin.y = center->y - (anchor->y - py);
	s->origin.z = center->z;

	s->radius = radius * flip;
	s->revolution = AngleFixed(pitch) / 360;

	s->min_revolution = s->revolution;
	s->max_revolution = s->revolution +
		center_max_revolution(center) * flip;

	s->flip = center_has_flip(center);

	player->speed =
		3 * (player->speed + toucher->momz) / 2;

	/* cancel the effects of K_Squish */
	toucher->spritexscale = FRACUNIT;
	toucher->spriteyscale = FRACUNIT;
}
