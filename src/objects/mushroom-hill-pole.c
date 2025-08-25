// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Lachlan "Lach" Wright
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mushroom-hill-pole.c
/// \brief Mushroom Hill Pole object code.

#include "../p_local.h"
#include "../k_objects.h"
#include "../s_sound.h"
#include "../k_kart.h"
#include "../m_easing.h"
#include "../r_main.h"

// where 0 = fully facing angle, FRACUNIT = fully momentum angle
#define MOMENTUM_ANGLE_PROPORTION (FRACUNIT/2)
#define MOMENTUM_MULTIPLIER (FRACUNIT * 5 / 2)
#define MINIMUM_SPIN_DEGREES (360 + 180)

static const fixed_t PI = 355 * FRACUNIT / 113;

static void MushroomHillPolePlayerThink(player_t *player)
{
	mobj_t *mo = player->mo;
	mobj_t *pole = mo->tracer;
	fixed_t radius, circumference;
	angle_t deltaAngle;

	radius = mo->radius + pole->radius; // distance from the pole to spin from
	circumference = 2 * FixedMul(PI, radius);
	deltaAngle = FixedAngle(FixedMul(FixedDiv(pole->movefactor, circumference), 360 * FRACUNIT)); // change in angle for this tic

	if ((pole->movecount -= AngleFixed(deltaAngle)) > 0) // continue spinning as long as we have degrees remaining
	{
		deltaAngle *= pole->extravalue1;
		pole->movedir += deltaAngle;

		P_InstaThrust(mo, pole->movedir + pole->extravalue1 * ANGLE_90, pole->movefactor); // force the player's momentum so effects like drift sparks look cool
		P_MoveOrigin(mo,
			pole->x + P_ReturnThrustX(NULL, pole->movedir, radius) - mo->momx,
			pole->y + P_ReturnThrustY(NULL, pole->movedir, radius) - mo->momy,
			mo->z
		);
		P_SetPlayerAngle(player, player->angleturn + deltaAngle);
	}
	else // <Steve> RELEASE!!!!!!!!!!
	{
		P_SetTarget(&pole->target, NULL);
		P_SetTarget(&pole->tracer, mo); // track this player so that they can't interact with this pole again...
		pole->fuse = pole->reactiontime; /// ...for this many tics

		P_SetPlayerAngle(player, (angle_t)pole->extravalue2); // restore camera angle
		P_InstaThrust(mo, pole->angle, FixedMul(pole->movefactor, MOMENTUM_MULTIPLIER)); // launch at launch angle
		S_StartSound(mo, pole->info->activesound);
		P_SetTarget(&mo->tracer, NULL);
		player->carry = CR_NONE;
	}
}

void Obj_MushroomHillPolePlayerThink(player_t *player)
{
	// not carried by a pole?
	if (player->carry != CR_MUSHROOMHILLPOLE)
	{
		return;
	}

	// pole mysteriously vanquished by eldritch addon setups?
	if (P_MobjWasRemoved(player->mo->tracer))
	{
		P_SetTarget(&player->mo->tracer, NULL);
		player->carry = CR_NONE;
		return;
	}

	MushroomHillPolePlayerThink(player);
}

void Obj_MushroomHillPoleTouch(mobj_t *pole, mobj_t *toucher)
{
	player_t *player = toucher->player;
	angle_t momentumAngle;

	if (
		player->carry != CR_NONE // player is already being carried by something else
		|| pole->tracer == toucher // pole just launched this player
		|| (
			!P_MobjWasRemoved(pole->target)
			&& pole->target->player
			&& pole->target->player->carry == CR_MUSHROOMHILLPOLE
		) // pole is already occupied by a player
	)
	{
		return;
	}

	momentumAngle = K_MomentumAngle(toucher);

	P_SetTarget(&pole->target, toucher);
	pole->movefactor = max(FixedHypot(toucher->momx, toucher->momy), FixedMul(pole->info->speed, pole->scale)); // speed at which to spin around the pole
	pole->movedir = R_PointToAngle2(pole->x, pole->y, toucher->x, toucher->y); // angle at which to project the player from the pole
	pole->angle = toucher->angle + Easing_Linear(MOMENTUM_ANGLE_PROPORTION, 0, (INT32)(momentumAngle - toucher->angle)); // final launch angle
	pole->extravalue1 = (pole->movedir - momentumAngle < ANGLE_180) ? -1 : 1; // direction to spin around the pole
	pole->extravalue2 = (INT32)player->angleturn; // player's old angle, to restore upon launch
	pole->movecount = AngleFixed(pole->extravalue1 * (pole->angle - pole->movedir) - ANGLE_90); // fixed-scale number of degrees to spin around the pole

	while (pole->movecount < MINIMUM_SPIN_DEGREES * FRACUNIT)
	{
		pole->movecount += 360 * FRACUNIT;
	}

	P_SetTarget(&toucher->tracer, pole);
	player->carry = CR_MUSHROOMHILLPOLE;
	S_StartSound(toucher, pole->info->seesound);

	MushroomHillPolePlayerThink(player); // start spinning immediately
}

void Obj_MushroomHillPoleFuse(mobj_t *pole)
{
	P_SetTarget(&pole->tracer, NULL);
}
