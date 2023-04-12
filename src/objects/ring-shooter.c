// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by "Lach"
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ring-shooter.c
/// \brief DEZ "Ring Shooter" respawner object

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../r_skins.h"
#include "../k_respawn.h"

#define RS_FUSE_TIME (4*TICRATE)

static void ActivateRingShooter(mobj_t *mo)
{
	mobj_t *part = mo->tracer;

	while (!P_MobjWasRemoved(part->tracer))
	{
		part = part->tracer;
		part->renderflags &= ~RF_DONTDRAW;
		part->frame += 4;
	}
}

#define rs_base_scalespeed(o) ((o)->scalespeed)
#define rs_base_scalestate(o) ((o)->threshold)
#define rs_base_xscale(o) ((o)->extravalue1)
#define rs_base_yscale(o) ((o)->extravalue2)

#define rs_part_xoffset(o) ((o)->extravalue1)
#define rs_part_yoffset(o) ((o)->extravalue2)

static void ScalePart(mobj_t *part, mobj_t *base)
{
	part->spritexscale = rs_base_xscale(base);
	part->spriteyscale = rs_base_yscale(base);
}

static void MovePart(mobj_t *part, mobj_t *base, mobj_t *refNipple)
{
	P_MoveOrigin(
		part,
		refNipple->x + FixedMul(rs_part_xoffset(part), rs_base_xscale(base)),
		refNipple->y + FixedMul(rs_part_yoffset(part), rs_base_xscale(base)),
		part->z
	);
}

// I've tried to reduce redundancy as much as I can,
// but check K_SpawnRingShooter if you edit this
static void UpdateRingShooterParts(mobj_t *mo)
{
	mobj_t *part, *refNipple;

	part = mo;
	while (!P_MobjWasRemoved(part->hprev))
	{
		part = part->hprev;
		ScalePart(part, mo);
	}
	refNipple = part;

	part = mo;
	while (!P_MobjWasRemoved(part->hnext))
	{
		part = part->hnext;
		MovePart(part, mo, refNipple);
		ScalePart(part, mo);
	}

	part = mo->tracer;
	part->z = mo->z + FixedMul(refNipple->height, rs_base_yscale(mo));
	MovePart(part, mo, refNipple);
	ScalePart(part, mo);
}

static boolean RingShooterInit(mobj_t *mo)
{
	if (rs_base_scalestate(mo) == -1)
	{
		return false;
	}

	switch (rs_base_scalestate(mo))
	{
		case 0:
		{
			rs_base_yscale(mo) += rs_base_scalespeed(mo);
			if (rs_base_yscale(mo) >= FRACUNIT)
			{
				//rs_base_xscale(mo) -= rs_base_scalespeed(mo);
				rs_base_scalestate(mo)++;
			}
			break;
		}
		case 1:
		{
			rs_base_scalespeed(mo) -= FRACUNIT/5;
			rs_base_yscale(mo) += rs_base_scalespeed(mo);
			rs_base_xscale(mo) -= rs_base_scalespeed(mo);
			if (rs_base_yscale(mo) < 3*FRACUNIT/4)
			{
				rs_base_scalestate(mo)++;
				rs_base_scalespeed(mo) = FRACUNIT >> 2;
			}
			break;
		}
		case 2:
		{
			rs_base_yscale(mo) += rs_base_scalespeed(mo);
			rs_base_xscale(mo) -= rs_base_scalespeed(mo);
			if (rs_base_yscale(mo) >= FRACUNIT)
			{
				rs_base_scalestate(mo) = -1;
				rs_base_xscale(mo) = rs_base_yscale(mo) = FRACUNIT;
				ActivateRingShooter(mo);
			}
		}
	}

	UpdateRingShooterParts(mo);
	return (rs_base_scalestate(mo) != -1);
}

static void RingShooterCountdown(mobj_t *mo)
{
	mobj_t *part = mo->tracer;

	if (mo->reactiontime == -1)
	{
		return;
	}

	if (--mo->reactiontime > 0)
	{
		return;
	}

	while (!P_MobjWasRemoved(part->tracer))
	{
		part = part->tracer;
		part->frame--;
	}

	switch ((part->frame & FF_FRAMEMASK) - (part->state->frame & FF_FRAMEMASK))
	{
		case -1:
		{
			mo->reactiontime = -1;
			part->skin = mo->skin;
			P_SetMobjState(part, S_RINGSHOOTER_FACE);
			break;
		}
		case 0:
		{
			mo->reactiontime = TICRATE;
			S_StartSound(mo, mo->info->deathsound);
			break;
		}
		default:
		{
			mo->reactiontime = TICRATE;
			S_StartSound(mo, mo->info->painsound);
			break;
		}
	}
}

static void RingShooterFlicker(mobj_t *mo)
{
	UINT32 trans;
	mobj_t *part = mo->tracer;

	while (!P_MobjWasRemoved(part->tracer))
	{
		part = part->tracer;
	}

	part->renderflags ^= RF_DONTDRAW;
	if (part->renderflags & RF_DONTDRAW)
	{
		trans = FF_TRANS50;
	}
	else
	{
		trans = 0;
	}
	part->target->frame = (part->target->frame & ~FF_TRANSMASK) | trans;
}

void Obj_RingShooterThinker(mobj_t *mo)
{
	if (P_MobjWasRemoved(mo->tracer) || RingShooterInit(mo))
		return;

	RingShooterCountdown(mo);
	RingShooterFlicker(mo);
}

static boolean AllowRingShooter(player_t *player)
{
	const fixed_t minSpeed = 6 * player->mo->scale;

	if (player->respawn.state != RESPAWNST_NONE
		&& player->respawn.init == true)
	{
		return false;
	}

	if (player->drift == 0
		&& player->justbumped == 0
		&& player->spindashboost == 0
		&& player->nocontrol == 0
		&& player->fastfall == 0
		&& player->speed < minSpeed
		&& P_PlayerInPain(player) == false
		&& P_IsObjectOnGround(player->mo) == true)
	{
		return true;
	}

	return false;
}

// I've tried to reduce redundancy as much as I can,
// but check P_UpdateRingShooterParts if you edit this
static void SpawnRingShooter(player_t *player)
{
	const fixed_t scale = 2*FRACUNIT;
	mobjinfo_t *info = &mobjinfo[MT_RINGSHOOTER_PART];
	mobj_t *mo = player->mo;
	mobj_t *base = P_SpawnMobj(mo->x, mo->y, mo->z, MT_RINGSHOOTER);
	mobj_t *part, *refNipple;
	UINT32 frameNum;
	angle_t angle;
	vector2_t offset;
	SINT8 i;

	K_FlipFromObject(base, mo);
	P_SetTarget(&base->target, mo);
	P_SetScale(base, base->destscale = FixedMul(base->destscale, scale));
	base->angle = mo->angle;
	base->scalespeed = FRACUNIT/2;
	base->extravalue1 = FRACUNIT; // horizontal scale
	base->extravalue2 = 0; // vertical scale
	base->fuse = RS_FUSE_TIME;

	// the ring shooter object itself is invisible and acts as the thinker
	// each ring shooter uses three linked lists to keep track of its parts
	// the hprev chain stores the two NIPPLE BARS
	// the hnext chain stores the four sides of the box
	// the tracer chain stores the screen and the screen layers

	// spawn the RING NIPPLES
	part = base;
	frameNum = 0;
	FV2_Load(&offset, -96*FRACUNIT, 160*FRACUNIT);
	FV2_Divide(&offset, scale);
	for (i = -1; i < 2; i += 2)
	{
		P_SetTarget(&part->hprev, P_SpawnMobjFromMobj(base,
			P_ReturnThrustX(NULL, base->angle - ANGLE_90, i*offset.x) + P_ReturnThrustX(NULL, base->angle, offset.y),
			P_ReturnThrustY(NULL, base->angle - ANGLE_90, i*offset.x) + P_ReturnThrustY(NULL, base->angle, offset.y),
			0, MT_RINGSHOOTER_PART));
		P_SetTarget(&part->hprev->hnext, part);
		part = part->hprev;
		P_SetTarget(&part->target, base);

		part->angle = base->angle - i * ANGLE_45;
		P_SetMobjState(part, S_RINGSHOOTER_NIPPLES);
		part->frame += frameNum;
		part->flags |= MF_NOTHINK;
		part->old_spriteyscale = part->spriteyscale = 0;
		frameNum++;
	}
	refNipple = part; // keep the second ring nipple; its position will be referenced by the box

	// spawn the box
	part = base;
	frameNum = 0;
	angle = base->angle + ANGLE_90;
	FV2_Load(&offset, offset.x - info->radius, offset.y - info->radius); // set the new origin to the centerpoint of the box
	FV2_Load(&offset,
		P_ReturnThrustX(NULL, base->angle - ANGLE_90, offset.x) + P_ReturnThrustX(NULL, base->angle, offset.y),
		P_ReturnThrustY(NULL, base->angle - ANGLE_90, offset.x) + P_ReturnThrustY(NULL, base->angle, offset.y)); // transform it relative to the base
	for (i = 0; i < 4; i++)
	{
		P_SetTarget(&part->hnext, P_SpawnMobjFromMobj(base,
			offset.x + P_ReturnThrustX(NULL, angle, info->radius),
			offset.y + P_ReturnThrustY(NULL, angle, info->radius),
			0, MT_RINGSHOOTER_PART));
		P_SetTarget(&part->hnext->hprev, part);
		part = part->hnext;
		P_SetTarget(&part->target, base);

		if (i == 2)
			frameNum++;
		frameNum ^= FF_HORIZONTALFLIP;
		angle -= ANGLE_90;
		part->angle = angle;
		part->frame += frameNum;
		part->extravalue1 = part->x - refNipple->x;
		part->extravalue2 = part->y - refNipple->y;
		part->flags |= MF_NOTHINK;
		part->old_spriteyscale = part->spriteyscale = 0;
	}

	// spawn the screen
	part = P_SpawnMobjFromMobj(base, offset.x, offset.y, 0, MT_RINGSHOOTER_SCREEN);
	P_SetTarget(&base->tracer, part);
	P_SetTarget(&part->target, base);
	part->angle = base->angle - ANGLE_45;
	part->extravalue1 = part->x - refNipple->x;
	part->extravalue2 = part->y - refNipple->y;
	part->flags |= MF_NOTHINK;
	part->old_spriteyscale = part->spriteyscale = 0;

	// spawn the screen numbers
	for (i = 0; i < 2; i++)
	{
		P_SetTarget(&part->tracer, P_SpawnMobjFromMobj(part, 0, 0, 0, MT_OVERLAY));
		P_SetTarget(&part->tracer->target, part);
		part = part->tracer;
		part->angle = part->target->angle;
		P_SetMobjState(part, S_RINGSHOOTER_NUMBERBACK + i);
		part->renderflags |= RF_DONTDRAW;
	}
}

void Obj_RingShooterInput(player_t *player)
{
	if (AllowRingShooter(player) == true
		&& (player->cmd.buttons & BT_RESPAWN) == BT_RESPAWN
		&& (player->oldcmd.buttons & BT_RESPAWN) == 0)
	{
		SpawnRingShooter(player);
	}
}
