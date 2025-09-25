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
#include "../doomstat.h"
#include "../info.h"
#include "../g_game.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../r_state.h"
#include "../k_kart.h"
#include "../k_battle.h"
#include "../m_random.h"
#include "../r_main.h"
#include "../s_sound.h"

#define FINE90 (FINEANGLES/4)
#define FINE180 (FINEANGLES/2)
#define TRUETAN(n) FINETANGENT(FINE90 + (n)) // bruh

#define HEALTHFACTOR (FRACUNIT/4) // Always takes at most, 4 hits.

#define MONITOR_PART_DEFINE(dispoffset, nsides, ...) \
{dispoffset, nsides, (statenum_t[]){__VA_ARGS__, 0}}

static const struct monitor_part_config {
	INT32 dispoffset;
	UINT8 nsides;
	statenum_t * states;
} monitor_parts[] = {
	MONITOR_PART_DEFINE (0, 3,
			S_MONITOR_SCREEN1A,
			S_ITEMICON,
			S_MONITOR_CRACKB,
			S_MONITOR_CRACKA),

	MONITOR_PART_DEFINE (-5, 5, S_MONITOR_STAND),
};

#define monitor_spot(o) ((o)->target)
#define monitor_rngseed(o) ((o)->movedir)
#define monitor_itemcount(o) ((o)->movecount)
#define monitor_spawntic(o) ((o)->reactiontime)
#define monitor_emerald(o) ((o)->extravalue1)
#define monitor_damage(o) ((o)->extravalue2)
#define monitor_rammingspeed(o) ((o)->movefactor)
#define monitor_combohit(o) ((o)->cusval)

static inline UINT8
get_monitor_itemcount (const mobj_t *monitor)
{
	// protects against divide by zero
	return max(monitor_itemcount(monitor), 1);
}

#define part_monitor(o) ((o)->target)
#define part_type(o) ((o)->extravalue1)
#define part_index(o) ((o)->extravalue2)
#define part_theta(o) ((o)->movedir)

#define shard_can_roll(o) ((o)->extravalue1)

static const sprcache_t * get_state_sprcache (statenum_t);

static const sprcache_t *
get_sprcache
(		spritenum_t sprite,
		UINT8 frame)
{
	const spritedef_t *sprdef = &sprites[sprite];

	if (frame < sprdef->numframes)
	{
		size_t lump = sprdef->spriteframes[frame].lumpid[0];

		return &spritecachedinfo[lump];
	}
	else
	{
		return get_state_sprcache(S_UNKNOWN);
	}
}

static const sprcache_t *
get_state_sprcache (statenum_t statenum)
{
	return get_sprcache(states[statenum].sprite,
			states[statenum].frame & FF_FRAMEMASK);
}

static inline fixed_t
get_inradius
(		fixed_t length,
		INT32 nsides)
{
	return FixedDiv(length, 2 * TRUETAN(FINE180 / nsides));
}

static inline void
center_item_sprite
(		mobj_t * part,
		fixed_t scale)
{
	part->spriteyoffset = 25*FRACUNIT;
	part->spritexscale = scale;
	part->spriteyscale = scale;
}

static mobj_t *
spawn_part
(		mobj_t * monitor,
		statenum_t state)
{
	mobj_t *part = P_SpawnMobjFromMobj(
			monitor, 0, 0, 0, MT_MONITOR_PART);

	P_SetMobjState(part, state);
	P_SetTarget(&part_monitor(part), monitor);

	part_type(part) = state;

	switch (state)
	{
		case S_ITEMICON:
			// The first frame of the monitor is TV static so
			// this should be invisible on the first frame.
			part->renderflags |= RF_DONTDRAW;
			break;

		default:
			break;
	}

	return part;
}

static void
spawn_part_side
(		mobj_t * monitor,
		fixed_t rad,
		fixed_t ang,
		const struct monitor_part_config * p,
		size_t side)
{
	INT32 i = 0;

	while (p->states[i])
	{
		mobj_t *part = spawn_part(monitor, p->states[i]);

		part->radius = rad;
		part_theta(part) = ang;

		// add one point for each layer (back to front order)
		part->dispoffset = p->dispoffset + i;

		part_index(part) = side;

		i++;
	}
}

static void
spawn_monitor_parts
(		mobj_t * monitor,
		const struct monitor_part_config *p)
{
	const sprcache_t *info = get_state_sprcache(p->states[0]);
	const fixed_t width = FixedMul(monitor->scale, info->width);
	const fixed_t rad = get_inradius(width, p->nsides);
	const fixed_t angle_factor = ANGLE_MAX / p->nsides;

	INT32 i;
	angle_t ang = 0;

	for (i = 0; i < p->nsides; ++i)
	{
		spawn_part_side(monitor, rad, ang, p, i);
		ang += angle_factor;
	}
}

static inline boolean
can_shard_state_roll (statenum_t state)
{
	switch (state)
	{
		case S_MONITOR_BIG_SHARD:
		case S_MONITOR_SMALL_SHARD:
			return true;

		default:
			return false;
	}
}

static void
spawn_shard
(		mobj_t * part,
		statenum_t state)
{
	mobj_t *monitor = part_monitor(part);

	// These divisions and multiplications are done on the
	// offsets to give bigger increments of randomness.

	const fixed_t half = FixedDiv(
			monitor->height, monitor->scale) / 2;

	const UINT16 rad = (monitor->radius / monitor->scale) / 4;
	const UINT16 tall = (half / FRACUNIT) / 4;


	// note: determinate random argument eval order
	fixed_t rand_z = P_RandomKey(PR_ITEM_DEBRIS, tall + 1);
	fixed_t rand_y = P_RandomRange(PR_ITEM_DEBRIS, -(rad), rad);
	fixed_t rand_x = P_RandomRange(PR_ITEM_DEBRIS, -(rad), rad);
	mobj_t *p = P_SpawnMobjFromMobj(monitor,
			rand_x * 8 * FRACUNIT,
			rand_y * 8 * FRACUNIT,
			(half / 4) + rand_z * 4 * FRACUNIT,
			MT_MONITOR_SHARD);

	angle_t th = (part->angle + ANGLE_90);

	th -= P_RandomKey(PR_ITEM_DEBRIS, ANGLE_45) - ANGLE_22h;

	p->hitlag = 0;

	P_Thrust(p, th, 6 * p->scale + monitor_rammingspeed(monitor));
	p->momz = P_RandomRange(PR_ITEM_DEBRIS, 3, 10) * p->scale;

	P_SetMobjState(p, state);

	shard_can_roll(p) = can_shard_state_roll(state);

	if (shard_can_roll(p))
	{
		p->rollangle = P_Random(PR_ITEM_DEBRIS);
	}

	if (P_RandomChance(PR_ITEM_DEBRIS, FRACUNIT/2))
	{
		p->renderflags |= RF_DONTDRAW;
	}
}

static void
spawn_debris (mobj_t *part)
{
	const mobj_t *monitor = part_monitor(part);

	fixed_t i;

	for (i = monitor->health;
			i <= FRACUNIT; i += HEALTHFACTOR/2)
	{
		spawn_shard(part, S_MONITOR_BIG_SHARD);
		spawn_shard(part, S_MONITOR_SMALL_SHARD);
		spawn_shard(part, S_MONITOR_TWINKLE);
	}
}

static void
spawn_monitor_explosion (mobj_t *monitor)
{
	mobj_t *smoldering = P_SpawnMobjFromMobj(monitor, 0, 0, 0, MT_SMOLDERING);

	UINT8 i;

	// Note that a Broly Ki is purposefully not spawned. This
	// is to reduce visual clutter since these monitors would
	// probably get popped a lot.

	K_MineFlashScreen(monitor);

	P_SetScale(smoldering, (smoldering->destscale /= 3));
	smoldering->tics = TICRATE*3;

	for (i = 0; i < 8; ++i)
	{
		mobj_t *x = P_SpawnMobjFromMobj(monitor, 0, 0, 0, MT_BOOMEXPLODE);
		x->hitlag = 0;
		x->color = SKINCOLOR_WHITE;
		x->momx = P_RandomRange(PR_EXPLOSION, -5, 5) * monitor->scale;
		x->momy = P_RandomRange(PR_EXPLOSION, -5, 5) * monitor->scale;
		x->momz = P_RandomRange(PR_EXPLOSION, 0, 6) * monitor->scale * P_MobjFlip(monitor);
		P_SetScale(x, (x->destscale *= 3));
	}
}

static void
kill_monitor_part (mobj_t *part)
{
	const statenum_t statenum = part_type(part);

	switch (statenum)
	{
		case S_ITEMICON:
			P_RemoveMobj(part);
			return;

		case S_MONITOR_STAND:
			part->momx = 0;
			part->momy = 0;
			break;

		case S_MONITOR_SCREEN1A:
			spawn_debris(part);
			P_SetMobjState(part, S_MONITOR_SCREEN1B);
			/*FALLTHRU*/

		default:
			/* To be clear, momx/y do not need to set because
			   those fields are set every tic to offset each
			   part. */
			part->momz = (part->height / 8) * P_MobjFlip(part);
	}

	part->fuse = TICRATE;
	part->flags &= ~(MF_NOGRAVITY);
}

static inline UINT32
restore_item_rng (UINT32 seed)
{
	const UINT32 oldseed = P_GetRandSeed(PR_ITEM_SPAWNER);

	P_SetRandSeedNet(PR_ITEM_SPAWNER,
			P_GetInitSeed(PR_ITEM_SPAWNER), seed);

	return oldseed;
}

static inline SINT8
get_item_result (void)
{
	return K_GetTotallyRandomResult(0);
}

static SINT8
get_cycle_result
(		const mobj_t * monitor,
		size_t cycle)
{
	const size_t rem = cycle %
		get_monitor_itemcount(monitor);

	SINT8 result;
	size_t i;

	const UINT32 oldseed = restore_item_rng(
			monitor_rngseed(monitor));

	for (i = 0; i <= rem; ++i)
	{
		result = get_item_result();
	}

	restore_item_rng(oldseed);

	return result;
}

static inline tic_t
get_age (const mobj_t *monitor)
{
	return (leveltime - monitor_spawntic(monitor));
}

static inline boolean
is_flickering (const mobj_t *part)
{
	const mobj_t *monitor = part_monitor(part);

	return monitor->fuse > 0 && monitor->fuse <= TICRATE;
}

static void
flicker
(		mobj_t * part,
		UINT8 interval)
{
	const tic_t age = get_age(part_monitor(part));

	if (age % interval)
	{
		part->renderflags |= RF_DONTDRAW;
	}
	else
	{
		part->renderflags &= ~(RF_DONTDRAW);
	}
}

static void
project_icon (mobj_t *part)
{
	const mobj_t *monitor = part_monitor(part);
	const tic_t age = get_age(monitor);

	// Item displayed on monitor cycles every N tics
	if (age % 64 == 0)
	{
		const SINT8 result = get_cycle_result(monitor,
				part_index(part) + (age / 64));

		K_UpdateMobjItemOverlay(part,
				K_ItemResultToType(result),
				K_ItemResultToAmount(result, NULL));

		center_item_sprite(part, 5*FRACUNIT/4);
	}

	flicker(part, is_flickering(part) ? 4 : 2);
}

static void
translate (mobj_t *part)
{
	const angle_t ang = part_theta(part) +
		part_monitor(part)->angle;

	part->angle = (ang - ANGLE_90);

	// Because of MF_NOCLIPTHING, no friction is applied.
	// This object is teleported back to the monitor every
	// tic so its position is in total only ever translated
	// by this much.
	part->momx = P_ReturnThrustX(NULL, ang, part->radius);
	part->momy = P_ReturnThrustY(NULL, ang, part->radius);
}

static inline fixed_t
get_damage_multiplier (const mobj_t *monitor)
{
	return FixedDiv(monitor_damage(monitor), HEALTHFACTOR);
}

static inline boolean
has_state
(		const mobj_t * mobj,
		statenum_t state)
{
	return mobj->hitlag == 0 &&
		(size_t)(mobj->state - states) == (size_t)state;
}

static mobj_t *
adjust_monitor_drop
(		mobj_t * monitor,
		mobj_t * drop)
{
	if (drop->type == MT_EMERALD)
	{
		drop->momx = drop->momy = drop->momz = 0;
	}
	else
	{
		P_InstaThrust(drop, drop->angle, 8*mapobjectscale);
		drop->momz *= 8;
	}

	K_FlipFromObjectNoInterp(drop, monitor);

	return drop;
}

void
Obj_MonitorSpawnParts (mobj_t *monitor)
{
	const size_t nparts =
		sizeof monitor_parts / sizeof *monitor_parts;

	size_t i;

	P_SetScale(monitor, (monitor->destscale *= 2));

	monitor_itemcount(monitor) = 0;
	monitor_rngseed(monitor) = P_GetRandSeed(PR_ITEM_SPAWNER);
	monitor_spawntic(monitor) = leveltime;
	monitor_emerald(monitor) = 0;

	for (i = 0; i < nparts; ++i)
	{
		spawn_monitor_parts(monitor, &monitor_parts[i]);
	}
}

mobj_t *
Obj_SpawnMonitor
(		mobj_t * origin,
		UINT8 numItemTypes,
		UINT8 emerald)
{
	mobj_t *monitor = P_SpawnMobj(origin->x, origin->y,
			origin->z, MT_MONITOR);

	monitor->angle = P_Random(PR_DECORATION);

	monitor_itemcount(monitor) = numItemTypes;
	monitor_emerald(monitor) = emerald;

	monitor->color = K_GetChaosEmeraldColor(emerald);

	return monitor;
}

void
Obj_MonitorThink (mobj_t *monitor)
{
	if (Obj_MonitorGetEmerald(monitor))
	{
		Obj_SpawnEmeraldSparks(monitor);
	}

	K_BattleOvertimeKiller(monitor);
}

void
Obj_MonitorPartThink (mobj_t *part)
{
	const statenum_t statenum = part_type(part);

	mobj_t *monitor = part_monitor(part);

	if (part->fuse > 0)
	{
		return;
	}

	if (P_MobjWasRemoved(monitor))
	{
		P_RemoveMobj(part);
		return;
	}

	if (has_state(monitor, monitor->info->deathstate))
	{
		kill_monitor_part(part);
		return;
	}

	if (is_flickering(part))
	{
		flicker(part, 2);
	}

	if (monitor->hitlag)
	{
		const fixed_t shake = FixedMul(
				2 * get_damage_multiplier(monitor),
				monitor->radius / 8);

		part->sprxoff = P_AltFlip(shake, 2);
		part->spryoff = P_AltFlip(shake, 4);
	}
	else
	{
		part->sprxoff = 0;
		part->spryoff = 0;
	}

	switch (statenum)
	{
		case S_MONITOR_SCREEN1A:
			if (has_state(monitor, monitor->info->painstate))
			{
				spawn_debris(part);
			}
			break;

		case S_MONITOR_CRACKA:
		case S_MONITOR_CRACKB:
			if (monitor->health < monitor->info->spawnhealth)
			{
				part->sprite = SPR_IMON; // initially SPR_NULL
				part->frame = part->state->frame +
					(monitor->health / HEALTHFACTOR);
			}
			break;

		case S_ITEMICON:
			project_icon(part);
			break;

		default:
			break;
	}

	P_MoveOrigin(part, monitor->x, monitor->y, monitor->z);

	translate(part);
}

fixed_t
Obj_MonitorGetDamage
(		mobj_t * monitor,
		mobj_t * inflictor,
		UINT8 damagetype)
{
	fixed_t damage;

	if (leveltime - monitor_combohit(monitor) < 35) // Fast combo hits destroy monitors.
		return FRACUNIT;

	monitor_combohit(monitor) = leveltime;

	switch (damagetype & DMG_TYPEMASK)
	{
		case DMG_VOLTAGE:
			if (monitor->health < HEALTHFACTOR)
			{
				return HEALTHFACTOR;
			}
			else
			{
				// always reduce to final damage state
				return (monitor->health - HEALTHFACTOR) + 1;
			}
	}

	if (inflictor == NULL)
	{
		return HEALTHFACTOR;
	}

	if (inflictor->player)
	{
		const fixed_t weight =
			K_GetMobjWeight(inflictor, monitor);

		// HEALTHFACTOR is the minimum damage that can be
		// dealt but player's weight (and speed) can buff the hit.
		damage = HEALTHFACTOR +
			(FixedMul(weight, HEALTHFACTOR) / 9);


		if (inflictor->player->tiregrease > 0)
		{
			damage *= 3; // Do 3x the damage if the player is in spring grease state
		}

		if (inflictor->scale > mapobjectscale)
		{
			damage = P_ScaleFromMap(damage, inflictor->scale);
		}
	}
	else
	{
		if (inflictor->type == MT_INSTAWHIP)
		{
			damage = FRACUNIT/3;
			if (K_IsPlayerWanted(inflictor->target->player))
				damage = FRACUNIT; // Emerald hunting time!
		}
		else
		{
			damage = FRACUNIT; // kill instantly
		}
	}

	return damage;
}

void
Obj_MonitorOnDamage
(		mobj_t * monitor,
		mobj_t * inflictor,
		INT32 damage)
{
	monitor->fuse = BATTLE_DESPAWN_TIME;
	monitor_damage(monitor) = damage;
	monitor_rammingspeed(monitor) = inflictor
		? FixedDiv(FixedHypot(inflictor->momx, inflictor->momy), 4 * inflictor->radius) : 0;
	monitor->hitlag =
		3 * get_damage_multiplier(monitor) / FRACUNIT;
	S_StartSound(monitor, sfx_kc40);
}

void
Obj_MonitorOnDeath
(		mobj_t * monitor,
		mobj_t * source)
{
	const UINT8 itemcount = get_monitor_itemcount(monitor);
	const angle_t ang = ANGLE_MAX / itemcount;
	const SINT8 flip = P_MobjFlip(monitor);

	INT32 i;

	UINT32 sharedseed = restore_item_rng(
			monitor_rngseed(monitor));

	for (i = 0; i < itemcount; ++i)
	{
		const SINT8 result = get_item_result();
		const UINT32 localseed = restore_item_rng(sharedseed);

		mobj_t *drop = adjust_monitor_drop(monitor,
				K_FlingPaperItem(
					monitor->x, monitor->y, monitor->z + (128 * mapobjectscale * flip),
					i * ang, flip,
					K_ItemResultToType(result),
					K_ItemResultToAmount(result, NULL)));

		drop->momz /= 2; // This is player-locked, so no need to throw it high

		if (!P_MobjWasRemoved(source) && source->player)
		{
			P_SetTarget(&drop->tracer, source);
			drop->extravalue1 = 5*TICRATE;
			drop->colorized = true;
			drop->color = source->player->skincolor;
		}

		// K_FlingPaperItem may advance RNG, so update our
		// copy of the seed afterward
		sharedseed = restore_item_rng(localseed);
	}

	restore_item_rng(sharedseed);

	if (monitor_emerald(monitor) != 0)
	{
		adjust_monitor_drop(monitor,
				K_SpawnChaosEmerald(monitor->x, monitor->y, monitor->z + (128 * mapobjectscale * flip),
					ang, flip, monitor_emerald(monitor)));
	}

	spawn_monitor_explosion(monitor);

	S_StartSound(monitor, sfx_gshcc);

	// There is hitlag from being damaged, so remove
	// tangibility RIGHT NOW.
	monitor->flags &= ~(MF_SOLID);

	if (!P_MobjWasRemoved(monitor_spot(monitor)))
	{
		Obj_ItemSpotUpdate(monitor_spot(monitor));
	}
}

void
Obj_MonitorShardThink (mobj_t *shard)
{
	if (shard_can_roll(shard))
	{
		shard->rollangle += ANGLE_45;
	}

	shard->renderflags ^= RF_DONTDRAW;
}

UINT32
Obj_MonitorGetEmerald (const mobj_t *monitor)
{
	return monitor_emerald(monitor);
}

void
Obj_MonitorSetItemSpot
(		mobj_t * monitor,
		mobj_t * spot)
{
	P_SetTarget(&monitor_spot(monitor), spot);
}
