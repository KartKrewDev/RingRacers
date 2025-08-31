// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../k_battle.h"
#include "../k_objects.h"
#include "../k_specialstage.h"
#include "../info.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../tables.h"
#include "../g_game.h"

#define emerald_type(o) ((o)->extravalue1)
#define emerald_anim_start(o) ((o)->movedir)
#define emerald_revolution_time(o) ((o)->threshold)
#define emerald_start_radius(o) ((o)->movecount)
#define emerald_target_radius(o) ((o)->extravalue2)
#define emerald_z_shift(o) ((o)->reactiontime)
#define emerald_scale_rate(o) ((o)->movefactor)
#define emerald_orbit(o) ((o)->target)
#define emerald_award(o) ((o)->tracer)

// Think of this like EMERALD_SPEED_UP / EMERALD_SPEED_UP_RATE
#define EMERALD_SPEED_UP (1) // speed up by this much...
#define EMERALD_SPEED_UP_RATE (1) // ...every N tics

void Obj_SpawnEmeraldSparks(mobj_t *mobj)
{
	if (leveltime % 3 != 0)
	{
		return;
	}

	fixed_t rand_x;
	fixed_t rand_y;
	fixed_t rand_z;

	// note: determinate random argument eval order
	rand_z = P_RandomRange(PR_SPARKLE, 0, 64);
	rand_y = P_RandomRange(PR_SPARKLE, -48, 48);
	rand_x = P_RandomRange(PR_SPARKLE, -48, 48);

	mobj_t *sparkle = P_SpawnMobjFromMobj(
		mobj,
		rand_x * FRACUNIT,
		rand_y * FRACUNIT,
		rand_z * FRACUNIT,
		MT_EMERALDSPARK
	);

	sparkle->color = mobj->color;
	sparkle->momz += 8 * mobj->scale * P_MobjFlip(mobj);
	sparkle->sprzoff = mobj->sprzoff;
}

static INT32 get_elapsed(mobj_t *emerald)
{
	return leveltime - min((tic_t)emerald_anim_start(emerald), leveltime);
}

static INT32 get_revolve_time(mobj_t *emerald)
{
	return max(1, emerald_revolution_time(emerald));
}

static fixed_t get_suck_factor(mobj_t *emerald)
{
	const INT32 suck_time = get_revolve_time(emerald) * 2;

	return (min(get_elapsed(emerald), suck_time) * FRACUNIT) / suck_time;
}

static fixed_t get_current_radius(mobj_t *emerald)
{
	fixed_t s = emerald_start_radius(emerald);
	fixed_t t = emerald_target_radius(emerald);

	return s + FixedMul(t - s, get_suck_factor(emerald));
}

static fixed_t get_bob(mobj_t *emerald)
{
	// With a fuse, the emerald experiences "speed up" and the
	// scale also shrinks. All of these these effects caused
	// the bob phase shift to look disproportioned.
	angle_t phase = emerald->fuse ? 0 : get_elapsed(emerald) * ((ANGLE_MAX / get_revolve_time(emerald)) / 2);

	return FixedMul(30 * mapobjectscale, FSIN(emerald->angle + phase));
}

static fixed_t center_of(mobj_t *mobj)
{
	return mobj->z + (mobj->height / 2);
}

static fixed_t get_target_z(mobj_t *emerald)
{
	fixed_t shift = FixedMul(emerald_z_shift(emerald), FRACUNIT - get_suck_factor(emerald));

	return center_of(emerald_orbit(emerald)) + get_bob(emerald) + shift;
}

static void speed_up(mobj_t *emerald)
{
	// Revolution time shouldn't decrease below zero.
	if (emerald_revolution_time(emerald) <= EMERALD_SPEED_UP)
	{
		return;
	}

	if (get_elapsed(emerald) % EMERALD_SPEED_UP_RATE)
	{
		return;
	}

	// Decrease the fuse proportionally to the revolution time.
	const fixed_t ratio = (emerald->fuse * FRACUNIT) / emerald_revolution_time(emerald);

	emerald_revolution_time(emerald) -= EMERALD_SPEED_UP;

	emerald->fuse = max(1, (emerald_revolution_time(emerald) * ratio) / FRACUNIT);
}

static void Obj_EmeraldOrbitPlayer(mobj_t *emerald)
{
	fixed_t r = get_current_radius(emerald);
	fixed_t x = FixedMul(r, FCOS(emerald->angle));
	fixed_t y = FixedMul(r, FSIN(emerald->angle));

	// Multiplayer Sealed Stars can become unwinnable if someone deathpits with the emerald. Find a player to retarget!
	if ((gametyperules & GTR_CATCHER) && emerald_orbit(emerald)->player && (emerald_orbit(emerald)->player->pflags & PF_NOCONTEST))
	{
		player_t *bestplayer = emerald_orbit(emerald)->player;

		for (int i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;
			if (players[i].spectator)
				continue;
			if (players[i].pflags & PF_NOCONTEST)
				continue;
			if (!(players[i].mo && !P_MobjWasRemoved(players[i].mo)))
				continue;
			if ((bestplayer->pflags & PF_NOCONTEST) || (players[i].distancetofinish < bestplayer->distancetofinish))
					bestplayer = &players[i];
		}

		if (!(bestplayer->pflags & PF_NOCONTEST))
		{
			P_MoveOrigin(emerald, bestplayer->mo->x, bestplayer->mo->y, bestplayer->mo->z);
			P_SetTarget(&emerald->tracer, NULL); // Ensures that tracer is correctly reset, allowing ACS to detect empty-handdeness.
			// "Why not just check target, which has to be set for any part of the behavior to work at all?"
			// Because Hyudoro is an emerald. I will not explain further. Program for a different game.
			Obj_BeginEmeraldOrbit(emerald, bestplayer->mo, 100 * mapobjectscale, 64, 0);
			return;
		}
	}

	P_MoveOrigin(
			emerald,
			emerald_orbit(emerald)->x + x,
			emerald_orbit(emerald)->y + y,
			get_target_z(emerald)
	);

	emerald->angle += ANGLE_MAX / get_revolve_time(emerald);

	if (emerald->fuse > 0)
	{
		speed_up(emerald);

		P_InstaScale(emerald, emerald->fuse * emerald_scale_rate(emerald));
	}
}

void Obj_EmeraldThink(mobj_t *emerald)
{
	if (!P_MobjWasRemoved(emerald_orbit(emerald)))
	{
		switch (emerald_orbit(emerald)->type)
		{
			case MT_SPECIAL_UFO:
				Obj_UFOEmeraldThink(emerald);
				break;

			case MT_PLAYER:
				Obj_EmeraldOrbitPlayer(emerald);
				break;

			default:
				break;
		}

		return;
	}

	if (emerald->threshold > 0)
	{
		emerald->threshold--;
	}

	A_AttractChase(emerald);

	Obj_SpawnEmeraldSparks(emerald);

	K_BattleOvertimeKiller(emerald);
}

static mobj_t *spawn_glow(mobj_t *flare)
{
	mobj_t *targ = flare->target;
	mobj_t *x = P_SpawnGhostMobj(targ);

	x->old_x = targ->old_x;
	x->old_y = targ->old_y;
	x->old_z = targ->old_z;

	x->fuse = 2; // this actually does last one tic
	x->extravalue1 = 1;
	x->extravalue2 = 0;

	x->renderflags = RF_ADD | RF_ALWAYSONTOP;

	// FIXME: linkdraw doesn't work consistently, so I drew it on top of everyting (and through walls)
#if 0
	P_SetTarget(&x->tracer, targ);
	x->flags2 |= MF2_LINKDRAW;
	x->dispoffset = 1000;
#endif

	return x;
}

static mobj_t *spawn_glow_colorize(mobj_t *flare)
{
	mobj_t *x = spawn_glow(flare);

	x->color = flare->color;
	x->colorized = true;

	return x;
}

void Obj_EmeraldFlareThink(mobj_t *flare)
{
	const INT32 kExtraTics = 3;
	const INT32 flare_tics = states[S_EMERALDFLARE1].tics + kExtraTics;

	if (P_MobjWasRemoved(flare->target))
	{
		P_RemoveMobj(flare);
		return;
	}

	// Target is assumed to be the emerald in orbit. When
	// emerald fuse runs out, it shall update player's emerald
	// flags. Time the flare animation so it ends with the
	// emerald fuse.
	if (!flare->fuse && flare->target->fuse > flare_tics)
	{
		return;
	}

	if (flare->state == &states[S_INVISIBLE])
	{
		// In special stages, just follow the emerald.
		if (specialstageinfo.valid == false)
		{
			// Update target to player. We don't need to track
			// the emerald anymore.
			P_SetTarget(&flare->target, flare->target->target);

			if (P_MobjWasRemoved(flare->target))
			{
				P_RemoveMobj(flare);
				return;
			}
		}

		P_SetMobjState(flare, S_EMERALDFLARE1);
		flare->fuse = flare_tics;
	}

	// Focus on center of player.
	P_SetOrigin(flare, flare->target->x, flare->target->y, center_of(flare->target));

	if (leveltime & 1)
	{
		// Stacked for more exposure
		spawn_glow_colorize(flare);
		spawn_glow(flare);
		spawn_glow(flare);
	}
}

static void spawn_lens_flare(mobj_t *emerald)
{
	mobj_t *flare = P_SpawnMobjFromMobj(emerald, 0, 0, 0, MT_EMERALDFLARE);

	P_SetTarget(&flare->target, emerald);
	P_InstaScale(flare, emerald_orbit(emerald)->scale);

	flare->color = emerald->color;
	flare->colorized = true;

	flare->renderflags |= RF_ALWAYSONTOP;

	// FIXME: linkdraw doesn't work consistently, so I drew it on top of everyting (and through walls)
#if 0
	P_SetTarget(&flare->tracer, emerald_orbit(emerald));
	flare->flags2 |= MF2_LINKDRAW;
	flare->dispoffset = 1000;
#endif
}

void Obj_BeginEmeraldOrbit(mobj_t *emerald, mobj_t *target, fixed_t radius, INT32 revolution_time, tic_t fuse)
{
	P_SetTarget(&emerald_orbit(emerald), target);
	P_SetTarget(&emerald->punt_ref, target);

	if (!emerald_award(emerald) || P_MobjWasRemoved(emerald_award(emerald)))
	{
		P_SetTarget(&emerald_award(emerald), target);
	}

	emerald_anim_start(emerald) = leveltime;
	emerald_revolution_time(emerald) = revolution_time;

	emerald_start_radius(emerald) = R_PointToDist2(target->x, target->y, emerald->x, emerald->y);
	emerald_target_radius(emerald) = radius;

	emerald->fuse = fuse;

	if (fuse)
	{
		emerald_scale_rate(emerald) = emerald->scale / fuse;
	}

	emerald->angle = R_PointToAngle2(target->x, target->y, emerald->x, emerald->y);
	emerald_z_shift(emerald) = emerald->z - get_target_z(emerald);

	emerald->flags |= MF_NOGRAVITY | MF_NOCLIP | MF_NOCLIPTHING | MF_NOCLIPHEIGHT;
	emerald->shadowscale = 0;

	spawn_lens_flare(emerald);
}

static void give_player(mobj_t *emerald)
{
	player_t *player = emerald_award(emerald)->player;

	if (!player)
	{
		return;
	}

	player->emeralds |= emerald_type(emerald);
	K_CheckEmeralds(player);

	S_StartSound(emerald_award(emerald), emerald->info->deathsound);
}

void Obj_GiveEmerald(mobj_t *emerald)
{
	if (P_MobjWasRemoved(emerald_orbit(emerald)) || P_MobjWasRemoved(emerald_award(emerald)))
	{
		return;
	}

	// FIXME: emerald orbiting behavior should become its own object. For now,
	// though, enjoy these special conditions!
	switch (emerald_award(emerald)->type)
	{
		case MT_PLAYER:
			give_player(emerald);
			break;

		case MT_ITEMCAPSULE: // objects/hyudoro.c
			// DMG_INSTAKILL to kill it without respawning later
			P_KillMobj(emerald_award(emerald), emerald_orbit(emerald), emerald_orbit(emerald), DMG_INSTAKILL);
			S_StartSound(emerald_orbit(emerald), sfx_kc3e);

			if (emerald_orbit(emerald)->player)
			{
				// Unlock item for stacked Hyudoros
				emerald_orbit(emerald)->player->itemRoulette.reserved = 0;
			}
			break;

		default:
			break;
	}
}

void Obj_SetEmeraldAwardee(mobj_t *emerald, mobj_t *awardee)
{
	P_SetTarget(&emerald_award(emerald), awardee);
}

boolean Obj_EmeraldCanHUDTrack(const mobj_t *emerald)
{
	if (!P_MobjWasRemoved(emerald_award(emerald)) && emerald_award(emerald)->type == MT_ITEMCAPSULE)
	{
		return false;
	}

	return true;
}
