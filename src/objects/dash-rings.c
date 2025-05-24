// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Lachlan "Lach" Wright
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dash-rings.c
/// \brief Dash Ring and Rainbow Dash Ring object code.

#include "../p_local.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../s_sound.h"

// Dash Rings are scaled by this much relative to the map scale
#define DASHRING_SCALE (3*FRACUNIT/2)

// Dash Ring angles are defined by their mapthing's args[0] (previously used mapthing->options flags, hence the selections)
#define DASHRING_TYPE_HORIZONTAL 0
#define DASHRING_TYPE_30DEGREES 1
#define DASHRING_TYPE_60DEGREES 4
#define DASHRING_TYPE_VERTICAL 8

// Dash Rings must be this far apart for players to interact with them in succession
#define DASHRING_MIN_SPACING_HORIZONTAL (512*FRACUNIT)
#define DASHRING_MIN_SPACING_VERTICAL (384*FRACUNIT)

// timer values
#define DASHRING_PULL_TICS 5
#define DASHRING_PUSH_TICS (TICRATE/2)
#define DASHRING_ANTIGRAVITY_TICS 5

// base launch speed
#define DASHRING_BASE_LAUNCH_SPEED (48*FRACUNIT)

// factor of distance traveled per tic while being pulled towards a Dash Ring
#define DASHRING_PULL_FACTOR (FRACUNIT/3)

static const skincolornum_t ring_colors[] = {
	SKINCOLOR_GREY,        // 1x
	SKINCOLOR_TAN,         // 1.25x
	SKINCOLOR_YELLOW,      // 1.5x
	SKINCOLOR_TANGERINE,   // 1.75x
	SKINCOLOR_KETCHUP,     // 2x
	SKINCOLOR_MOONSET,     // 2.25x
	SKINCOLOR_ULTRAMARINE,  // 2.5x +
};

static const skincolornum_t rainbow_colors[] = {
	SKINCOLOR_PINK,
	SKINCOLOR_CREAMSICLE,
	SKINCOLOR_TAN,
	SKINCOLOR_TURTLE,
	SKINCOLOR_TURQUOISE,
	SKINCOLOR_THISTLE,
};

void Obj_RegularDashRingSpawn(mobj_t *mobj)
{
	P_SetScale(mobj, mobj->destscale = FixedMul(mobj->scale, DASHRING_SCALE));
	mobj->renderflags |= RF_SEMIBRIGHT;

	P_SetTarget(&mobj->tracer, P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_OVERLAY));
	P_SetTarget(&mobj->tracer->target, mobj);
	P_SetMobjState(mobj->tracer, S_DASHRING_HORIZONTAL_FLASH1);
	mobj->tracer->color = SKINCOLOR_WHITE;
	mobj->tracer->renderflags |= RF_SEMIBRIGHT;
}

void Obj_RainbowDashRingSpawn(mobj_t *mobj)
{
	P_SetScale(mobj, mobj->destscale = FixedMul(mobj->scale, DASHRING_SCALE));
	mobj->renderflags |= RF_FULLBRIGHT;
}

void Obj_DashRingSetup(mobj_t *mobj, mapthing_t *mthing)
{
	static const UINT8 numColors = sizeof(ring_colors) / sizeof(skincolornum_t);
	const UINT8 additionalThrust = mthing->thing_args[1];
	statenum_t ringState, overlayState;

	mobj->extravalue1 = mthing->thing_args[0];
	mobj->cusval = 4 + additionalThrust;

	switch (mobj->extravalue1)
	{
		case DASHRING_TYPE_30DEGREES:
			ringState = S_DASHRING_30DEGREES;
			overlayState = S_DASHRING_30DEGREES_FLASH1;
			break;
		case DASHRING_TYPE_60DEGREES:
			ringState = S_DASHRING_60DEGREES;
			overlayState = S_DASHRING_60DEGREES_FLASH1;
			break;
		case DASHRING_TYPE_VERTICAL:
			ringState = S_DASHRING_VERTICAL;
			overlayState = S_DASHRING_VERTICAL_FLASH1;
			break;
		case DASHRING_TYPE_HORIZONTAL:
		default:
			ringState = S_DASHRING_HORIZONTAL;
			overlayState = S_DASHRING_HORIZONTAL_FLASH1;
			break;
	}

	P_SetMobjState(mobj, ringState);
	if (!P_MobjWasRemoved(mobj->tracer))
		P_SetMobjState(mobj->tracer, overlayState);

	mobj->spriteyoffset = mobj->info->height >> 1; // I think this is to center the sprite within its hitbox regardless of height
	mobj->color = ring_colors[min(additionalThrust, numColors - 1)];
}

void Obj_RainbowDashRingThink(mobj_t *mobj)
{
	static const UINT8 numColors = sizeof(rainbow_colors) / sizeof(skincolornum_t);
	mobj->color = rainbow_colors[(leveltime / 2) % numColors];
}

static boolean DashRingsAreTooClose(mobj_t *ring1, mobj_t *ring2)
{
	if (ring1 == ring2)
		return true;

	if ((FixedHypot(ring2->x - ring1->x, ring2->y - ring1->y) < FixedMul(DASHRING_MIN_SPACING_HORIZONTAL, mapobjectscale))
		&& (abs(ring1->z - ring2->z) < FixedMul(DASHRING_MIN_SPACING_VERTICAL, mapobjectscale)))
			return true;

	return false;
}

boolean Obj_DashRingIsUsableByPlayer(mobj_t *ring, player_t *player)
{
	if (player->carry != CR_NONE)
	{
		if (player->carry != CR_DASHRING) // being carried by something else
			return false;

		if (player->dashRingPullTics > 0) // being pulled into a dash ring already
			return false;

		if (player->dashRingPushTics > 0 && !P_MobjWasRemoved(player->mo->tracer) && DashRingsAreTooClose(player->mo->tracer, ring)) // dash ring is too close to recently used dash ring
			return false;
	}
	return true;
}

void Obj_DashRingTouch(mobj_t *ring, player_t *player)
{
	if (!Obj_DashRingIsUsableByPlayer(ring, player))
	{
		return;
	}

	P_SetTarget(&player->mo->tracer, ring);
	player->carry = CR_DASHRING;
	player->dashRingPullTics = DASHRING_PULL_TICS;
	player->dashRingPushTics = 0;
	player->turbine = 0;
}

static fixed_t GetPlayerDashRingZ(player_t *player, mobj_t *ring)
{
	return (ring->z + (ring->height >> 1) - (player->mo->height >> 1));
}

static void DashRingLaunch(player_t *player, mobj_t *ring)
{
	mobj_t *ghost = P_SpawnGhostMobj(ring);
	const fixed_t launchSpeed = FixedMul(DASHRING_BASE_LAUNCH_SPEED * ring->cusval / 4, mapobjectscale);
	angle_t pitch;

	ghost->destscale = ring->scale * 8;
	ghost->scalespeed = ring->scale / 12;
	ghost->old_z = ghost->z += P_MobjFlip(ring) * FixedMul(ghost->spriteyoffset, ghost->scale); // apply sprite offset to physical position instead, so ghost is centered
	ghost->spriteyoffset = 0;

	P_MoveOrigin(player->mo, ring->x, ring->y, GetPlayerDashRingZ(player, ring));
	player->dashRingPullTics = 0;
	player->dashRingPushTics = DASHRING_PUSH_TICS;

	player->mo->rollangle = 0;
	P_ResetPitchRoll(player->mo);
	player->flashing = 0;
	player->fastfall = 0;
	K_TumbleInterrupt(player);
	player->transfer = 0;

	switch (ring->extravalue1)
	{
		case DASHRING_TYPE_30DEGREES:
			pitch = 30 * ANG1;
			break;
		case DASHRING_TYPE_60DEGREES:
			pitch = 60 * ANG1;
			break;
		case DASHRING_TYPE_VERTICAL:
			pitch = 90 * ANG1;
			break;
		case DASHRING_TYPE_HORIZONTAL:
		default:
			pitch = 0;
			break;
	}

	P_InstaThrust(player->mo, ring->angle, P_ReturnThrustX(NULL, pitch, launchSpeed));
	player->mo->momz = P_MobjFlip(ring) * P_ReturnThrustY(NULL, pitch, launchSpeed);

	S_StartSound(player->mo, ring->info->seesound);
}

static void RegularDashRingLaunch(player_t *player, mobj_t *ring)
{
	player->springstars = TICRATE/2;
	player->springcolor = ring->color;
	DashRingLaunch(player, ring);
}

static void RainbowDashRingLaunch(player_t *player, mobj_t *ring)
{
	player->mo->eflags &= ~MFE_SPRUNG;
	K_DoPogoSpring(player->mo, 0, 0);
	DashRingLaunch(player, ring);
}

void Obj_DashRingPlayerThink(player_t *player)
{
	if (player->carry != CR_DASHRING)
		return;

	if (player->dashRingPullTics > 0)
	{
		mobj_t *ring = player->mo->tracer;

		if (P_MobjWasRemoved(player->mo->tracer))
		{
			player->carry = CR_NONE;
			player->dashRingPullTics = 0;
		}
		else
		{
			player->mo->momx = FixedMul(DASHRING_PULL_FACTOR, ring->x - player->mo->x);
			player->mo->momy = FixedMul(DASHRING_PULL_FACTOR, ring->y - player->mo->y);
			player->mo->momz = FixedMul(DASHRING_PULL_FACTOR, GetPlayerDashRingZ(player, ring) - player->mo->z);
			player->mo->rollangle = (angle_t)FixedMul(DASHRING_PULL_FACTOR, (fixed_t)player->mo->rollangle);

			if (--player->dashRingPullTics == 0)
			{
				if (ring->type == MT_DASHRING)
				{
					RegularDashRingLaunch(player, ring);
				}
				else
				{
					RainbowDashRingLaunch(player, ring);
				}
			}
		}
	}

	if (player->dashRingPushTics > 0)
	{
		if (leveltime & 1)
		{
			mobj_t *ghost = P_SpawnGhostMobj(player->mo);
			ghost->colorized = true;
			ghost->fuse = 3;
		}

		if (--player->dashRingPushTics == 0)
		{
			player->carry = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
		}
	}
}

boolean Obj_DashRingPlayerHasNoGravity(player_t *player)
{
	if (player->dashRingPullTics > 0)
		return true;

	if (player->dashRingPushTics >= DASHRING_PUSH_TICS - DASHRING_ANTIGRAVITY_TICS)
		return true;

	return false;
}
