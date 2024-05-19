// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by AJ "Tyron" Martinez.
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  instawhip.c
/// \brief Instawhip object code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"

#define recharge_target(o) ((o)->target)
#define recharge_offset(o) ((o)->movedir)

void Obj_InstaWhipThink (mobj_t *whip)
{
	if (P_MobjWasRemoved(whip->target))
	{
		P_RemoveMobj(whip);
	}
	else
	{
		mobj_t *mo = whip->target;
		player_t *player = mo->player;

		// Follow player
		whip->flags &= ~(MF_NOCLIPTHING);
		P_SetScale(whip, whip->target->scale);
		P_MoveOrigin(whip, mo->x, mo->y, mo->z + mo->height/2);
		whip->flags |= MF_NOCLIPTHING;

		// Twirl
		whip->angle = whip->target->angle + (ANG30 * 2 * whip->fuse);
		whip->target->player->drawangle = whip->angle;
		if (player->follower)
			player->follower->angle = whip->angle;
		player->pflags |= PF_GAINAX;
		player->glanceDir = -2;

		// Visuals
		whip->renderflags |= RF_NOSPLATBILLBOARD|RF_FULLBRIGHT;

		// This is opposite of player flashing tics
		if (leveltime & 1)
			whip->renderflags &= ~RF_DONTDRAW;
		else
			whip->renderflags |= RF_DONTDRAW;

		if (whip->extravalue2) // Whip has no hitbox but removing it is a pain in the ass
			whip->renderflags |= RF_DONTDRAW;
	}
}

void Obj_SpawnInstaWhipRecharge(player_t *player, angle_t angleOffset)
{
	mobj_t *x = P_SpawnMobjFromMobj(player->mo, 0, 0, player->mo->height / 2, MT_INSTAWHIP_RECHARGE);

	// This was previously used to delay the visual, back when this was VFX for a cooldown
	// instead of VFX for a charge. We want to instantly bail out of that state now.
	x->tics = 1;
	x->eflags &= ~MFE_VERTICALFLIP; // Fix the visual being misaligned.
	x->renderflags |= RF_SLOPESPLAT | RF_NOSPLATBILLBOARD;

	P_SetTarget(&recharge_target(x), player->mo);
	recharge_offset(x) = angleOffset;
}

void Obj_InstaWhipRechargeThink(mobj_t *x)
{
	mobj_t *target = recharge_target(x);

	if (P_MobjWasRemoved(target) || !target->player->instaWhipCharge)
	{
		P_RemoveMobj(x);
		return;
	}

	P_MoveOrigin(x, target->x, target->y, target->z + (target->height / 2));
	if (x->scale != target->scale * 2)
		P_InstaScale(x, target->scale * 2);
	x->angle = target->angle + recharge_offset(x);

	// Flickers every other frame
	x->renderflags ^= RF_DONTDRAW;
}

void Obj_SpawnInstaWhipReject(player_t *player)
{
	mobj_t *x = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_INSTAWHIP_REJECT);
	x->eflags &= ~MFE_VERTICALFLIP;
	// Fixes an issue with gravflip misplacing the object for the first tic.
	if (player->mo->eflags & MFE_VERTICALFLIP)
		P_SetOrigin(x, player->mo->x, player->mo->y, player->mo->z);

	P_SetTarget(&recharge_target(x), player->mo);
}

void Obj_InstaWhipRejectThink(mobj_t *x)
{
	mobj_t *target = x->target;

	if (P_MobjWasRemoved(target) || !target->player->instaWhipCharge)
	{
		P_RemoveMobj(x);
		return;
	}

	x->angle = x->target->angle;
	P_MoveOrigin(x, target->x, target->y, target->z);
	P_InstaScale(x, target->scale);
}
