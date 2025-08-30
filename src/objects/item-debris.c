// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../d_player.h"
#include "../m_random.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"

// TODO: general function
static fixed_t K_GetPlayerSpeedRatio(player_t *player)
{
	return FixedDiv(player->speed,
			K_GetKartSpeed(player, false, false));
}

#define debris_type(o) ((o)->extravalue1)
#define debris_bouncesleft(o) ((o)->threshold)

enum {
	DEBRIS_ALPHA,
	DEBRIS_BETA,

	NUM_DEBRIS_TYPES
};

struct debris_config {
	mobj_t * origin;
	angle_t angle;
	fixed_t speed;
	fixed_t scale;
	UINT8 type;
};

static fixed_t
get_speed_ratio (mobj_t *thing)
{
	return thing->player ?
		K_GetPlayerSpeedRatio(thing->player) : FRACUNIT;
}

static void
spawn_debris
(		const struct debris_config * config,
		INT32 angle)
{
	const fixed_t height_table[NUM_DEBRIS_TYPES] = {
		35*FRACUNIT,
		24*FRACUNIT,
	};

	mobj_t *debris = P_SpawnMobjFromMobj(
			config->origin, 0, 0, 0, MT_ITEM_DEBRIS);

	debris_type(debris) = config->type;
	debris_bouncesleft(debris) = 1;

	P_InstaThrust(debris,
			config->angle + angle,
			config->speed);

	P_SetObjectMomZ(debris,
			FixedMul(config->scale,
				height_table[config->type]),
			false);

	debris->destscale =
		FixedMul(config->scale, 3 * debris->scale);
	P_SetScale(debris, debris->destscale);

	// Pass down color to dust particles
	debris->color = config->origin->color;

	if (config->origin->type != MT_RANDOMITEM
		|| config->origin->extravalue1 < RINGBOX_TIME)
	{
		debris->color = SKINCOLOR_WHITE;
		debris->colorized = true;
	}
}

static void
spawn_cloud
(		mobj_t * collectible,
		mobj_t * collector,
		fixed_t base_speed)
{
	const fixed_t min_speed = 90 * collectible->scale;

	const fixed_t scale = FixedDiv(
			max(base_speed, min_speed), min_speed);

	const INT16 spacing =
		(collectible->radius / 2) / collectible->scale;

	INT32 i;

	// Most of this code is from p_inter.c, MT_ITEMCAPSULE

	// dust effects
	for (i = 0; i < 10; i++)
	{
		fixed_t rand_x;
		fixed_t rand_y;
		fixed_t rand_z;

		// note: determinate random argument eval order
		rand_z = P_RandomRange(PR_UNDEFINED, 0, 4 * spacing);
		rand_y = P_RandomRange(PR_UNDEFINED, -spacing, spacing);
		rand_x = P_RandomRange(PR_UNDEFINED, -spacing, spacing);
		mobj_t *puff = P_SpawnMobjFromMobj(
				collectible,
				rand_x * FRACUNIT,
				rand_y * FRACUNIT,
				rand_z * FRACUNIT,
				MT_SPINDASHDUST
		);

		puff->color = collector->color;
		puff->colorized = true;

		puff->destscale = FixedMul(puff->destscale, scale);
		P_SetScale(puff, puff->destscale);

		puff->momz = puff->scale * P_MobjFlip(puff);

		puff->angle = R_PointToAngle2(
					collectible->x,
					collectible->y,
					puff->x,
					puff->y);

		P_Thrust(puff, puff->angle, 3 * puff->scale);

		puff->momx += collector->momx;
		puff->momy += collector->momy;
		puff->momz += collector->momz;
	}
}

static void
rotate3d (mobj_t *debris)
{
	const UINT8 steps = 30;

	debris->rollangle =
		M_RandomKey(steps) * (ANGLE_MAX / steps);
}

void
Obj_SpawnItemDebrisEffects
(		mobj_t * collectible,
		mobj_t * collector)
{
	const fixed_t min_speed = 80 * collectible->scale;

	fixed_t base_speed = FixedMul(75 * mapobjectscale,
			get_speed_ratio(collector));

	struct debris_config config;

	// Delayed effect for puffs of smoke that stick to and
	// glide off of the player
	mobj_t *spawner = P_SpawnMobjFromMobj(collectible,
			0, 0, 0, MT_ITEM_DEBRIS_CLOUD_SPAWNER);

	P_SetTarget(&spawner->target, collector);

	config.origin = collectible;
	config.angle = K_MomentumAngle(collector);
	config.speed = max(base_speed, min_speed);
	config.scale = FixedDiv(config.speed, min_speed);

	config.type = DEBRIS_ALPHA;

	spawn_debris(&config,   ANGLE_11hh);
	spawn_debris(&config, -(ANGLE_11hh));

	config.type = DEBRIS_BETA;

	spawn_debris(&config,   3*ANGLE_22h/2);
	spawn_debris(&config,   3*ANGLE_22h/4);
	spawn_debris(&config,   0);
	spawn_debris(&config, -(3*ANGLE_22h/4));
	spawn_debris(&config, -(3*ANGLE_22h/2));

	spawn_cloud(collectible, collector, base_speed);

	S_StartSound(spawner, sfx_kc2e);
	S_StartSound(spawner, sfx_s1c9);
}

void
Obj_ItemDebrisThink (mobj_t *debris)
{
	const UINT8 frame = (debris->frame & FF_FRAMEMASK);

	if (debris->momz == 0)
	{
		P_KillMobj(debris, NULL, NULL, DMG_NORMAL);
		return;
	}

	rotate3d(debris);

	if (frame % 3 == 1)
	{
		mobj_t *ghost = P_SpawnGhostMobj(debris);

		ghost->fuse = 3;
	}

	if (debris_type(debris) == DEBRIS_ALPHA)
	{
		mobj_t *dust = P_SpawnMobjFromMobj(
				debris, 0, 0, 0,  MT_SPINDASHDUST);

		P_SetScale(dust, (dust->destscale /= 3));

		dust->color = debris->color;
		dust->colorized = true;

		dust->momx = debris->momx / 4;
		dust->momy = debris->momy / 4;
		dust->momz = debris->momz / 4;
	}
}

fixed_t
Obj_ItemDebrisBounce
(		mobj_t * debris,
		fixed_t momz)
{
	if (debris_bouncesleft(debris) <= 0)
	{
		P_KillMobj(debris, NULL, NULL, DMG_NORMAL);
		return 0;
	}

	momz = -(momz);

	if (debris_type(debris) == DEBRIS_BETA)
	{
		momz /= 2;
	}

	debris_bouncesleft(debris)--;

	S_StartSound(debris, sfx_cdfm47);

	return momz;
}
