// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Lachlan "Lach" Wright
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  flybot767.c
/// \brief Flybot767 object code.

#include "../p_local.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../s_sound.h"
#include "../m_easing.h"

#define FLYBOT_QUANTITY 2
#define FLYBOT_VERTICAL_OFFSET (16 * FRACUNIT)
#define FLYBOT_BOB_AMPLITUDE (16 * FRACUNIT)
#define FLYBOT_BOB_FREQUENCY (ANG15)
#define FLYBOT_FADE_STARTTIME (2 * TICRATE)
#define FLYBOT_SCALE (17 * FRACUNIT / 20)
static const fixed_t PI = 355 * FRACUNIT / 113;

static fixed_t SetFlybotZ(mobj_t *flybot)
{
	flybot->z = FixedMul(mapobjectscale, FLYBOT_VERTICAL_OFFSET) + FixedMul(mapobjectscale, P_ReturnThrustX(NULL, flybot->movedir, FLYBOT_BOB_AMPLITUDE));
	if (flybot->eflags & MFE_VERTICALFLIP)
	{
		flybot->z = -flybot->z - flybot->height;
	}
	else
	{
		flybot->z += flybot->target->height;
	}
	flybot->z += flybot->target->z;
	return flybot->z;
}

void Obj_SpawnFlybotsForPlayer(player_t *player)
{
	UINT8 i;
	mobj_t *mo = player->mo;
	mobj_t *hprev = mo;
	fixed_t radius = mo->radius;

	for (i = 0; i < FLYBOT_QUANTITY; i++)
	{
		angle_t angle = mo->angle + ANGLE_90 + FixedAngle(i * 360 * FRACUNIT / FLYBOT_QUANTITY);
		mobj_t *flybot = P_SpawnMobj(
			mo->x + P_ReturnThrustX(NULL, angle, radius),
			mo->y + P_ReturnThrustY(NULL, angle, radius),
			mo->z,
			MT_FLYBOT767
		);

		P_InstaScale(flybot, flybot->old_scale = FixedMul(mapobjectscale, FLYBOT_SCALE));
		P_SetTarget(&flybot->target, mo);
		flybot->eflags |= mo->eflags & MFE_VERTICALFLIP;
		flybot->movedir = flybot->old_angle = flybot->angle = angle + ANGLE_90;
		flybot->old_z = SetFlybotZ(flybot);
		flybot->renderflags |= (i * RF_DONTDRAW);

		if (hprev->player)
		{
			P_SetTarget(&player->flybot, flybot);
		}
		else
		{
			P_SetTarget(&hprev->hnext, flybot);
			P_SetTarget(&flybot->hprev, hprev);
		}
		hprev = flybot;
	}
}

void Obj_FlybotThink(mobj_t *flybot)
{
	UINT16 stunned = UINT16_MAX;
	angle_t deltaAngle, angle;
	fixed_t radius, circumference;
	fixed_t speed = FixedMul(mapobjectscale, flybot->info->speed);
	mobj_t *mo = flybot->target;

	if (P_MobjWasRemoved(mo))
	{
		P_KillMobj(flybot, NULL, NULL, 0);
		return;
	}

	if (mo->player)
	{
		if (((stunned = mo->player->stunned) == 0) || (mo->player->playerstate == PST_DEAD))
		{
			P_KillMobj(flybot, NULL, NULL, 0);
			return;
		}

		// If player is spindashing, spin faster to hint that stun is going down faster
		else if (mo->player->spindash)
		{
			speed *= 2;
			flybot->movedir += FLYBOT_BOB_FREQUENCY*2;
		}
	}

	flybot->frame = flybot->frame & ~FF_TRANSMASK;
	if (stunned < FLYBOT_FADE_STARTTIME)
	{
		flybot->frame |= Easing_InCubic(FixedDiv(stunned, FLYBOT_FADE_STARTTIME), 7, 1) << FF_TRANSSHIFT;
	}

	flybot->eflags = (flybot->eflags & ~MFE_VERTICALFLIP) | (mo->eflags & MFE_VERTICALFLIP);
	flybot->movedir += FLYBOT_BOB_FREQUENCY;
	flybot->renderflags ^= RF_DONTDRAW;

	radius = mo->radius;
	circumference = 2 * FixedMul(PI, radius);
	deltaAngle = FixedAngle(FixedMul(FixedDiv(speed, circumference), 360 * FRACUNIT));
	flybot->angle += deltaAngle;
	angle = flybot->angle - ANGLE_90;

	P_MoveOrigin(flybot,
		mo->x + P_ReturnThrustX(NULL, angle, radius),
		mo->y + P_ReturnThrustY(NULL, angle, radius),
		SetFlybotZ(flybot)
	);
}

void Obj_FlybotDeath(mobj_t *flybot)
{
	UINT8 i;
	angle_t angle = 0;
	fixed_t hThrust = 4*mapobjectscale, vThrust = 4*mapobjectscale;
	vector3_t mom = {0, 0, 0};
	mobj_t *mo = flybot->target;

	if (!P_MobjWasRemoved(mo))
	{
		if (mo->player && (flybot == mo->player->flybot))
		{
			P_SetTarget(&mo->player->flybot, NULL);
		}

		mom.x = mo->momx;
		mom.y = mo->momy;
		mom.z = mo->momz;
		//S_StartSound(mo, flybot->info->deathsound);
	}

	for (i = 0; i < 4; i++)
	{
		mo = P_SpawnMobjFromMobj(flybot, 0, 0, 0, MT_PARTICLE);
		P_SetMobjState(mo, S_SPINDASHDUST);
		mo->flags |= MF_NOSQUISH;
		mo->renderflags |= RF_FULLBRIGHT;
		mo->momx = mom.x;
		mo->momy = mom.y;
		mo->momz = mom.z + vThrust;
		P_Thrust(mo, angle, hThrust);
		vThrust *= -1;
		angle += ANGLE_90;
	}
}

void Obj_FlybotRemoved(mobj_t *flybot)
{
	mobj_t *mo = flybot->target;
	if (!P_MobjWasRemoved(mo) && mo->player && (flybot == mo->player->flybot))
	{
		P_SetTarget(&mo->player->flybot, NULL);
	}
}
