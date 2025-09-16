// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Lachlan "Lach" Wright
// Copyright (C) 2025 by Kart Krew
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
#include "../lua_hook.h"

#define RS_FUSE_TIME (4*TICRATE)
#define RS_FUSE_BLINK (TICRATE >> 1)

#define RS_GRABBER_START (16 << FRACBITS)
#define RS_GRABBER_SLIDE (RS_GRABBER_START >> 4)
#define RS_GRABBER_EXTRA (18 << FRACBITS)

#define RS_KARTED_INC (3)

#define rs_base_scalespeed(o) ((o)->scalespeed)
#define rs_base_initstate(o) ((o)->threshold)
#define rs_base_xscale(o) ((o)->extravalue1)
#define rs_base_yscale(o) ((o)->extravalue2)

#define rs_base_playerid(o) ((o)->lastlook)
#define rs_base_playerface(o) ((o)->cusval)
#define rs_base_playerlast(o) ((o)->watertop)

#define rs_base_karted(o) ((o)->movecount)
#define rs_base_grabberdist(o) ((o)->movefactor)
#define rs_base_canceled(o) ((o)->cvmem)

#define rs_part_xoffset(o) ((o)->extravalue1)
#define rs_part_yoffset(o) ((o)->extravalue2)

static void RemoveRingShooterPointer(mobj_t *base)
{
	player_t *player = NULL;

	if (rs_base_playerid(base) < 0 || rs_base_playerid(base) >= MAXPLAYERS)
	{
		// No pointer set
		return;
	}

	// NULL the player's pointer.
	if (playeringame[ rs_base_playerid(base) ])
	{
		player = &players[ rs_base_playerid(base) ];
		if (player->ringShooter == base)
			P_SetTarget(&player->ringShooter, NULL);
	}

	// Remove our player ID
	rs_base_playerid(base) = -1;
}


static void ChangeRingShooterPointer(mobj_t *base, player_t *player)
{
	// Remove existing pointer first.
	RemoveRingShooterPointer(base);

	if (player == NULL)
	{
		// Just remove it.
		return;
	}

	// Set new player pointer.
	P_SetTarget(&player->ringShooter, base);

	// Set new player ID.
	rs_base_playerid(base) = (player - players);
}

static void ScalePart(mobj_t *part, mobj_t *base)
{
	part->spritexscale = rs_base_xscale(base);
	part->spriteyscale = rs_base_yscale(base);

	if (part->type == MT_TIREGRABBER)
	{
		part->spritexscale /= 2;
		part->spriteyscale /= 2;
	}
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

static void ShowHidePart(mobj_t *part, mobj_t *base)
{
	part->renderflags = (part->renderflags & ~RF_DONTDRAW) | (base->renderflags & RF_DONTDRAW);
}

static fixed_t GetTireDist(mobj_t *base)
{
	return -(RS_GRABBER_EXTRA + rs_base_grabberdist(base));
}

static void MoveTire(mobj_t *part, mobj_t *base)
{
	const fixed_t dis = FixedMul(GetTireDist(base), base->scale);
	const fixed_t c = FINECOSINE(part->angle >> ANGLETOFINESHIFT);
	const fixed_t s =   FINESINE(part->angle >> ANGLETOFINESHIFT);
	P_MoveOrigin(
		part,
		base->x + FixedMul(dis, c),
		base->y + FixedMul(dis, s),
		part->z
	);
}

// I've tried to reduce redundancy as much as I can,
// but check K_SpawnRingShooter if you edit this
static void UpdateRingShooterParts(mobj_t *mo)
{
	mobj_t *part, *refNipple;

	part = mo;
	while (!P_MobjWasRemoved(part->target))
	{
		part = part->target;
		ScalePart(part, mo);
		MoveTire(part, mo);
	}

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
	K_FlipFromObject(part, mo);
	ScalePart(part, mo);
}

static void UpdateRingShooterPartsVisibility(mobj_t *mo)
{
	mobj_t *part;

	part = mo;
	while (!P_MobjWasRemoved(part->target))
	{
		part = part->target;
		ShowHidePart(part, mo);
	}

	part = mo;
	while (!P_MobjWasRemoved(part->hprev))
	{
		part = part->hprev;
		ShowHidePart(part, mo);
	}

	part = mo;
	while (!P_MobjWasRemoved(part->hnext))
	{
		part = part->hnext;
		ShowHidePart(part, mo);
	}

	part = mo->tracer;
	ShowHidePart(part, mo);
}

static void RingShooterCountdown(mobj_t *mo)
{
	mobj_t *part = mo->tracer;

	if (mo->reactiontime < 0)
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

			if (rs_base_playerface(mo) >= 0 && rs_base_playerface(mo) < MAXPLAYERS)
			{
				if (playeringame[rs_base_playerface(mo)] == true)
				{
					player_t *player = &players[ rs_base_playerid(mo) ];
					part->skin = skins[player->skin];
				}
			}

			P_SetMobjState(part, S_RINGSHOOTER_FACE);
			break;
		}
		case 0:
		{
			mo->reactiontime = TICRATE;
			S_StartSound(mo, mo->info->deathsound);

			if (rs_base_playerid(mo) >= 0 && rs_base_playerid(mo) < MAXPLAYERS)
			{
				if (playeringame[rs_base_playerid(mo)] == true)
				{
					player_t *player = &players[ rs_base_playerid(mo) ];
					Obj_PlayerUsedRingShooter(mo, player);
				}
			}
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

static void ActivateRingShooter(mobj_t *mo)
{
	mobj_t *part = mo->tracer;

	while (!P_MobjWasRemoved(part->tracer))
	{
		part = part->tracer;
		part->renderflags &= ~RF_DONTDRAW;
		part->frame += 4;
	}

	RingShooterCountdown(mo);
}

static boolean RingShooterInit(mobj_t *mo)
{
	if (rs_base_initstate(mo) == -1)
	{
		return false;
	}

	switch (rs_base_initstate(mo))
	{
		case 0:
		{
			rs_base_yscale(mo) += rs_base_scalespeed(mo);
			if (rs_base_yscale(mo) >= FRACUNIT)
			{
				//rs_base_xscale(mo) -= rs_base_scalespeed(mo);
				rs_base_initstate(mo)++;
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
				rs_base_initstate(mo)++;
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
				rs_base_initstate(mo)++;
				rs_base_xscale(mo) = rs_base_yscale(mo) = FRACUNIT;
			}
			break;
		}
		case 3:
		{
			if (rs_base_canceled(mo) != 0)
			{
				rs_base_initstate(mo) = -1;
				ActivateRingShooter(mo);
			}
			else
			{
				rs_base_grabberdist(mo) -= RS_GRABBER_SLIDE;
				if (rs_base_grabberdist(mo) <= 0)
				{
					rs_base_initstate(mo) = -1;
					rs_base_grabberdist(mo) = 0;
					ActivateRingShooter(mo);
				}
			}
			break;
		}
		default:
		{
			rs_base_initstate(mo) = 0; // fix invalid states
			break;
		}
	}

	UpdateRingShooterParts(mo);
	return (rs_base_initstate(mo) != -1);
}

boolean Obj_RingShooterThinker(mobj_t *mo)
{
	if (RingShooterInit(mo) == true)
	{
		return true;
	}

	if (mo->fuse > 0)
	{
		mo->fuse--;

		if (mo->fuse == 0)
		{
			P_RemoveMobj(mo);
			return false;
		}
	}

	if (rs_base_canceled(mo) == 0)
	{
		rs_base_karted(mo) += RS_KARTED_INC;

		if (P_MobjWasRemoved(mo->tracer) == false)
		{
			RingShooterCountdown(mo);
		}
	}

	if (P_MobjWasRemoved(mo->tracer) == false)
	{
		RingShooterFlicker(mo);
	}

	if (mo->fuse < RS_FUSE_BLINK)
	{
		if (leveltime & 1)
		{
			mo->renderflags |= RF_DONTDRAW;
		}
		else
		{
			mo->renderflags &= ~RF_DONTDRAW;
		}

		UpdateRingShooterPartsVisibility(mo);
	}

	return true;
}

void Obj_PlayerUsedRingShooter(mobj_t *base, player_t *player)
{
	const UINT8 playerID = player - players;
	if (playerID == rs_base_playerlast(base))
	{
		return;
	}

	// Respawn using the respawner's karted value.
	if (rs_base_karted(base) > 0)
	{
		player->airtime += rs_base_karted(base);
	}

	player->respawn.fromRingShooter = true;
	K_DoIngameRespawn(player);

	P_SetTarget(&player->ringShooter, NULL);

	// Now other players can run into it!
	base->flags |= MF_SPECIAL;

	// Reset the fuse so everyone can conga line :B
	if (base->fuse < RS_FUSE_TIME)
	{
		if (base->fuse < RS_FUSE_BLINK)
		{
			base->renderflags &= ~RF_DONTDRAW;
			UpdateRingShooterPartsVisibility(base);
		}

		base->fuse = RS_FUSE_TIME;
	}

	// Record the last person to use the ring shooter.
	rs_base_playerlast(base) = playerID;
}

void Obj_RingShooterDelete(mobj_t *mo)
{
	mobj_t *part;

	RemoveRingShooterPointer(mo);

	part = mo->target;
	while (P_MobjWasRemoved(part) == false)
	{
		mobj_t *delete = part;
		part = part->target;
		P_RemoveMobj(delete);
	}

	part = mo->hprev;
	while (P_MobjWasRemoved(part) == false)
	{
		mobj_t *delete = part;
		part = part->hprev;
		P_RemoveMobj(delete);
	}

	part = mo->hnext;
	while (P_MobjWasRemoved(part) == false)
	{
		mobj_t *delete = part;
		part = part->hnext;
		P_RemoveMobj(delete);
	}

	part = mo->tracer;
	if (P_MobjWasRemoved(part) == false)
	{
		P_RemoveMobj(part);
	}
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

	rs_base_playerid(base) = rs_base_playerlast(base) = -1;
	rs_base_karted(base) = -(RS_KARTED_INC * TICRATE); // wait for "3"
	rs_base_grabberdist(base) = RS_GRABBER_START;

	K_FlipFromObjectNoInterp(base, mo);
	P_SetScale(base, base->destscale = FixedMul(base->destscale, scale));
	base->angle = mo->angle;
	base->scalespeed = FRACUNIT/2;
	base->extravalue1 = FRACUNIT; // horizontal scale
	base->extravalue2 = 0; // vertical scale
	base->fuse = RS_FUSE_TIME;

	// the ring shooter object itself is invisible and acts as the thinker
	// each ring shooter uses four linked lists to keep track of its parts
	// the hprev chain stores the two NIPPLE BARS
	// the hnext chain stores the four sides of the box
	// the tracer chain stores the screen and the screen layers
	// the target chain stores the tire grabbers

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

	P_SetTarget(&part->hprev, base);

	// spawn the grabbers
	part = base;
	angle = base->angle + ANGLE_45;
	for (i = 0; i < 4; i++)
	{
		const fixed_t dis = GetTireDist(base);
		P_SetTarget(
			&part->target,
			P_SpawnMobjFromMobj(
				base,
				P_ReturnThrustX(NULL, angle, dis),
				P_ReturnThrustY(NULL, angle, dis),
				0,
				MT_TIREGRABBER
			)
		);
		part = part->target;
		P_SetTarget(&part->tracer, base);

		angle -= ANGLE_90;
		part->angle = angle;
		part->extravalue1 = part->extravalue2 = 0;
		part->old_spriteyscale = part->spriteyscale = 0;
	}

	ChangeRingShooterPointer(base, player);
	rs_base_playerface(base) = (player - players);
}

static boolean AllowRingShooter(const player_t *player)
{
	const fixed_t minSpeed = 6 * player->mo->scale;

	if (/*(gametyperules & GTR_CIRCUIT) &&*/ leveltime < starttime)
	{
		return false;
	}

	if (player->respawn.state != RESPAWNST_NONE)
	{
		return false;
	}

	if (player->drift == 0
		&& player->justbumped == 0
		&& player->spindashboost == 0
		&& player->nocontrol == 0
		&& player->fastfall == 0
		&& player->speed < minSpeed
		&& player->freeRingShooterCooldown == 0
		&& P_PlayerInPain(player) == false
		&& P_IsObjectOnGround(player->mo) == true)
	{
		return true;
	}

	return false;
}

boolean Obj_PlayerRingShooterFreeze(const player_t *player)
{
	const mobj_t *base = player->ringShooter;

	if (AllowRingShooter(player) == true
		&& (player->cmd.buttons & BT_RESPAWNMASK) == BT_RESPAWNMASK
		&& P_MobjWasRemoved(base) == false)
	{
		return (rs_base_canceled(base) == 0);
	}

	return false;
}

void Obj_RingShooterInput(player_t *player)
{
	mobj_t *const base = player->ringShooter;

	if (AllowRingShooter(player) == true
		&& (player->cmd.buttons & BT_RESPAWNMASK) == BT_RESPAWNMASK)
	{
		// "Freeze" final-failsafe timer if we're eligible to ringshooter, but don't reset it.
		if (player->finalfailsafe)
			player->finalfailsafe--;

		if (P_MobjWasRemoved(base) == true)
		{
			SpawnRingShooter(player);
			return;
		}

		if (rs_base_canceled(base) == 0)
		{
			player->mo->momx = player->mo->momy = 0;
			P_SetPlayerAngle(player, base->angle);
			fixed_t setz;

			if (base->eflags & MFE_VERTICALFLIP)
			{
				setz = base->z + base->height - player->mo->height;
				setz = max(setz, player->mo->z);
			}
			else
			{
				setz = min(player->mo->z, base->z);
			}

			P_MoveOrigin(
				player->mo,
				base->x, base->y,
				setz
			);
			player->fastfall = 0;

			if (base->fuse < RS_FUSE_TIME)
			{
				if (base->fuse < RS_FUSE_BLINK)
				{
					base->renderflags &= ~RF_DONTDRAW;
					UpdateRingShooterPartsVisibility(base);
				}

				base->fuse = RS_FUSE_TIME;
			}
		}
	}
	else if (P_MobjWasRemoved(base) == false)
	{
		if (rs_base_initstate(base) != -1)
		{
			// We released during the intro animation.
			// Cancel it entirely, prevent another one being created for a bit.
			player->freeRingShooterCooldown = 2*TICRATE;
			rs_base_canceled(base) = 1;

			if (base->fuse > RS_FUSE_BLINK)
			{
				base->fuse = RS_FUSE_BLINK;
			}
		}
		else if (rs_base_canceled(base) == 0)
		{
			// We released during the countdown.
			// We activate with the current karted timer on the ring shooter.
			Obj_PlayerUsedRingShooter(base, player);
		}
	}
}

void Obj_UpdateRingShooterFace(mobj_t *part)
{
	mobj_t *const base = part->hprev;
	player_t *player = NULL;

	if (P_MobjWasRemoved(base) == true)
	{
		return;
	}

	if (rs_base_playerface(base) < 0 || rs_base_playerface(base) >= MAXPLAYERS)
	{
		return;
	}

	if (playeringame[ rs_base_playerface(base) ] == false)
	{
		return;
	}

	player = &players[ rs_base_playerface(base) ];

	// it's a good idea to set the actor's skin *before* it uses this action,
	// but just in case, if it doesn't have the player's skin, set its skin then call the state again to get the correct sprite
	if (part->skin != skins[player->skin])
	{
		part->skin = skins[player->skin];
		P_SetMobjState(part, (statenum_t)(part->state - states));
		return;
	}

	// okay, now steal the player's color nyehehehe
	part->color = player->skincolor;

	// set the frame to the WANTED pic
	part->frame = (part->frame & ~FF_FRAMEMASK) | FACE_WANTED;

	// set the threshold overlay flags
	part->threshold = (OV_DONTXYSCALE|OV_DONTSCREENOFFSET);

	// we're going to assume the character's WANTED icon is 32 x 32
	// let's squish the sprite a bit so that it matches the dimensions of the screen's sprite, which is 26 x 22
	// (TODO: maybe get the dimensions/offsets from the patches themselves?)
	part->old_spritexscale = part->spritexscale = FixedDiv(26*FRACUNIT, 32*FRACUNIT);
	part->old_spriteyscale = part->spriteyscale = FixedDiv(22*FRACUNIT, 32*FRACUNIT);

	// a normal WANTED icon should have (0, 0) offsets
	// so let's offset it such that it will match the position of the screen's sprite
	part->old_spritexoffset = part->spritexoffset = 16*FRACUNIT; // 32 / 2
	part->old_spriteyoffset = part->spriteyoffset = 28*FRACUNIT + FixedDiv(11*FRACUNIT, part->spriteyscale); // 32 - 4 (generic monster bottom) + 11 (vertical offset of screen sprite from the bottom)
}
