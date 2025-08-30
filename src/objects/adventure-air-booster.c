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
/// \file  adventure-air-booster.c
/// \brief Adventure Air Booster object code.

#include "../p_local.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../s_sound.h"
#include "../m_random.h"
#include "../r_main.h"

// Hardcoder note: I renamed anything using "Adventure Dash Ring"/"ADR" to "Adventure Air Booster"/"AAB" for consistency with the HVR config
// Hardcoder note: AAB_DIST used to be 64, but it was halved everywhere it was used, so I just halved the base value instead of during calculations
// Hardcoder note: AAB_STRENGTH_ADD used to be 75 to be multiplied with AAB_STRENGTH/100 at runtime, I just baked that calculation into it instead

#define AAB_DIST (32*FRACUNIT)                                 // distance between the back and front of the booster
#define AAB_FRONTBACKSPACE (8*FRACUNIT)                        // distance between the 2 sides of the front & back part of the booster
#define AAB_RADIUS (225*FRACUNIT)                              // radius (width) of the booster (in pixels). Used to determine where to spawn the 2 arrows on the sides
#define AAB_STRENGTH (55*FRACUNIT)                             // default speed for the booster
static const fixed_t AAB_STRENGTH_ADD = (75*AAB_STRENGTH/100); // each time the booster is used, add AAB_STRENGTH_ADD speed to the next player
#define AAB_DASHRINGPUSHTICS (3*TICRATE/2)
#define AAB_SPRINGSTARSTICS (TICRATE/2)

static const skincolornum_t AAB_COLORS[] = {
	SKINCOLOR_YELLOW,
	SKINCOLOR_GREEN,
	SKINCOLOR_RED,
	SKINCOLOR_BLUE,
};
static const UINT8 AAB_NUM_COLORS = sizeof(AAB_COLORS) / sizeof(skincolornum_t);

static void AdventureAirBoosterUpdateColor(mobj_t *mobj)
{
	mobj_t *part = mobj->hnext;

	mobj->color = AAB_COLORS[mobj->extravalue1];

	while (!P_MobjWasRemoved(part))
	{
		part->color = mobj->color;
		part = part->hnext;
	}
}

void Obj_AdventureAirBoosterSetup(mobj_t *mobj, mapthing_t *mthing)
{
	mobj_t *prev = mobj;
	mobj_t *part;
	angle_t positionAngle = mobj->angle;
	fixed_t oldHeight = mobj->height;
	fixed_t baseScale = mobj->scale;
	fixed_t xPos, yPos;
	SINT8 i;

	// arg1: double scale (pre-dates UDMF)
	if (mthing->thing_args[0])
	{
		baseScale *= 2;
	}

	// set scale and flip
	P_SetScale(mobj, mobj->movefactor = mobj->old_scale = mobj->destscale = baseScale);
	mobj->scalespeed = mobj->scale >> 2;
	if (mthing->options & MTF_OBJECTFLIP)
	{
		mobj->eflags |= MFE_VERTICALFLIP;
		mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->old_z = mobj->z -= (mobj->height - oldHeight);
	}

#define SpawnPart()\
{\
	part = P_SpawnMobjFromMobj(mobj, xPos, yPos, 0, MT_ADVENTUREAIRBOOSTER_PART);\
	P_SetTarget(&part->target, mobj);\
	P_SetTarget(&prev->hnext, part);\
	P_SetTarget(&part->hprev, prev);\
	prev = part;\
}

	// Spawn the back rings
	for (i = 0; i < 2; i++)
	{
		xPos = -P_ReturnThrustX(NULL, positionAngle, (AAB_FRONTBACKSPACE * i) + AAB_DIST);
		yPos = -P_ReturnThrustY(NULL, positionAngle, (AAB_FRONTBACKSPACE * i) + AAB_DIST);
		// with this order of operations, the first part to spawn is the front, then the back.
		SpawnPart();
		part->frame |= (1 - i);
		part->old_angle = part->angle = mobj->angle + ANGLE_90; // put it sideways
	}

	// Now the front ring
	for (i = 0; i < 2; i++)
	{
		xPos = P_ReturnThrustX(NULL, positionAngle, -(AAB_FRONTBACKSPACE * i) + AAB_DIST);
		yPos = P_ReturnThrustY(NULL, positionAngle, -(AAB_FRONTBACKSPACE * i) + AAB_DIST);
		// with this order of operations, the first part to spawn is the front, then the back.
		SpawnPart();
		part->frame |= (3 - i);
		part->old_angle = part->angle = mobj->angle + ANGLE_90; // put it sideways
	}

	// and now the 2 arrows
	positionAngle = mobj->angle + ANGLE_90; // put the angle to the side!
	for (i = -1; i < 2; i += 2)
	{
		xPos = P_ReturnThrustX(NULL, positionAngle, AAB_RADIUS * i) / 2;
		yPos = P_ReturnThrustY(NULL, positionAngle, AAB_RADIUS * i) / 2;
		// with this order of operations, the first arrow we spawn is the right one, then the left one. Angle them accordingly.
		SpawnPart();
		P_SetMobjState(part, S_ADVENTUREAIRBOOSTER_ARROW);
		part->old_angle = part->angle = mobj->angle - (ANGLE_45 * i);
	}
#undef SpawnPart

	part = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_ADVENTUREAIRBOOSTER_HITBOX);
	part->old_angle = part->angle = mobj->angle;
	part->old_z = part->z -= P_MobjFlip(part) * (part->height >> 1);
	P_SetTarget(&part->target, mobj);
	P_SetTarget(&mobj->tracer, part); // Hardcoder note: Since the hitbox doesn't need to be colored, I decided not to tack it on the end of the hnext chain; tracer is more easily accessible to scripters

	mobj->extravalue1 = 1; // default to green
	AdventureAirBoosterUpdateColor(mobj);
}

void Obj_AdventureAirBoosterHitboxTouch(mobj_t *hitbox, player_t *player)
{
	mobj_t *mo = player->mo;
	mobj_t *booster = hitbox->target;
	mobj_t *part;
	angle_t finalAngle = hitbox->angle;
	angle_t playerAngle;
	fixed_t finalSpeed, playerSpeed, xPos, yPos;
	SINT8 i;

	if (P_MobjWasRemoved(booster) || !Obj_DashRingIsUsableByPlayer(booster, player))
	{
		return;
	}

	// reflect angles like springs
	finalSpeed = FixedMul(AAB_STRENGTH + (booster->extravalue1 * AAB_STRENGTH_ADD), mapobjectscale);
	playerSpeed = FixedHypot(mo->momx, mo->momy);
	if (playerSpeed > 0)
	{
		playerAngle = R_PointToAngle2(0, 0, mo->momx, mo->momy);
	}
	else
	{
		playerAngle = mo->angle;
	}

	finalAngle = K_ReflectAngle(playerAngle, finalAngle, playerSpeed, finalSpeed);

	mo->momz = 0;
	P_InstaThrust(mo, finalAngle, finalSpeed);

	P_SetTarget(&mo->tracer, booster);
	player->carry = CR_DASHRING;
	player->dashRingPullTics = 0;
	player->dashRingPushTics = AAB_DASHRINGPUSHTICS;
	player->springstars = AAB_SPRINGSTARSTICS;
	player->springcolor = booster->color;
	player->turbine = 0;
	player->flashing = 0;
	player->fastfall = 0;
	K_TumbleInterrupt(player);

	S_StartSound(mo, booster->info->seesound);

	// before we change the colour, spawn a buncha sparkles
	for (i = 0; i < 12; i++)
	{
		fixed_t rand_x;
		fixed_t rand_y;
		fixed_t rand_z;

		// note: determinate random argument eval order
		rand_z = P_RandomFixed(PR_DECORATION);
		rand_y = P_RandomFixed(PR_DECORATION);
		rand_x = P_RandomFixed(PR_DECORATION);
		part = P_SpawnMobjFromMobj(
			hitbox,
			FixedMul(AAB_RADIUS << 1, rand_x) - AAB_RADIUS,
			FixedMul(AAB_RADIUS << 1, rand_y) - AAB_RADIUS,
			FixedMul(AAB_RADIUS << 1, rand_z) - AAB_RADIUS,
			MT_DVDPARTICLE
		);
		part->color = booster->color;
		P_SetMobjState(part, S_DVDSHINE1);
		P_SetScale(part, part->old_scale = hitbox->scale * P_RandomRange(PR_DECORATION, 2, 4));
		part->tics = P_RandomRange(PR_DECORATION, 1, 8);
		part->fuse = TICRATE >> 1;
		part->destscale = 1;
		part->scalespeed = part->scale / part->fuse;
		P_InstaThrust(part, finalAngle, FixedMul(finalSpeed, P_RandomRange(PR_DECORATION, FRACUNIT*80/100, FRACUNIT*120/100)));
	}

	// visuals
	booster->fuse = TICRATE >> 1;
	booster->destscale = 1;

	// spawn the 3 layers
	for (i = -1; i < 2; i++)
	{
		xPos = P_ReturnThrustX(NULL, booster->angle + ANGLE_90, (AAB_RADIUS >> 1) * i) + P_ReturnThrustX(NULL, booster->angle, AAB_RADIUS);
		yPos = P_ReturnThrustY(NULL, booster->angle + ANGLE_90, (AAB_RADIUS >> 1) * i) + P_ReturnThrustY(NULL, booster->angle, AAB_RADIUS);

		part = P_SpawnMobjFromMobj(hitbox, xPos, yPos, hitbox->info->height >> 1, MT_PARTICLE);
		part->old_angle = part->angle = booster->angle;
		part->color = booster->color;
		P_SetMobjState(part, i == 0 ? S_ADVENTUREAIRBOOSTER_EXHAUST2 : S_ADVENTUREAIRBOOSTER_EXHAUST1);
	}

	if (++booster->extravalue1 >= AAB_NUM_COLORS)
	{
		booster->extravalue1 = 0;
		// we just went through a blue booster, play sparkle sfx
		S_StartSound(mo, booster->info->activesound);
	}
	AdventureAirBoosterUpdateColor(booster);
}

void Obj_AdventureAirBoosterFuse(mobj_t *mobj)
{
	mobj_t *ghost = P_SpawnGhostMobj(mobj);
	mobj->destscale = mobj->movefactor;
	ghost->destscale = mobj->movefactor * 8;
	ghost->scalespeed = mobj->movefactor;
	ghost->height = mobj->height;
}
