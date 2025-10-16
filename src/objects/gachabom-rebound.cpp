// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>

#include "../d_player.h"
#include "../k_battle.h"
#include "../k_objects.h"
#include "../m_fixed.h"
#include "../info.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../tables.h"
#include "../s_sound.h"
#include "../k_kart.h"

/* An object may not be visible on the same tic:
   1) that it spawned
   2) that it cycles to the next state */
#define BUFFER_TICS (2)

#define rebound_target(o) ((o)->target)
#define rebound_mode(o) ((o)->threshold)
#define rebound_timer(o) ((o)->reactiontime)
#define played_rebound_sound(o) ((o)->movefactor)

namespace
{

constexpr int kReboundSpeed = 128;
constexpr int kReboundAcceptPause = 17;
constexpr int kReboundAcceptDuration = 8;
constexpr int kOrbitRadius = 0;

enum class Mode : int
{
	kAlpha = 0,
	kBeta,
	kOrbit,
};

fixed_t z_center(const mobj_t* mobj)
{
	return mobj->z + (mobj->height / 2);
}

bool rebound_is_returning(const mobj_t* mobj)
{
	return mobj->state == &states[S_GACHABOM_RETURNING];
}

fixed_t distance_to_target(const mobj_t* mobj)
{
	const mobj_t* target = rebound_target(mobj);
	const fixed_t zDelta = z_center(target) - mobj->z;

	return FixedHypot(FixedHypot(target->x - mobj->x, target->y - mobj->y), zDelta);
}

bool award_target(mobj_t* mobj)
{
	mobj_t* target = rebound_target(mobj);
	player_t* player = target->player;

	if (mobj->fuse)
	{
		return false;
	}

	if (player == nullptr)
	{
		return true;
	}

	if ((player->itemtype == KITEM_GACHABOM || player->itemtype == KITEM_NONE) && !player->itemRoulette.active && !player->instaWhipCharge)
	{
		rebound_timer(mobj)--;

		if (rebound_timer(mobj) < 1)
		{
			player->itemtype = KITEM_GACHABOM;
			K_AdjustPlayerItemAmount(player, 1);
			if (player->roundconditions.gachabom_miser == 1)
				player->roundconditions.gachabom_miser = 0;

			//S_StartSoundAtVolume(target, sfx_grbnd3, 255/3);
			S_StartSound(target, sfx_mbs54);

			return true;
		}
	}

	return false;
}

void chase_rebound_target(mobj_t* mobj)
{
	const mobj_t* target = rebound_target(mobj);
	const fixed_t zDelta = z_center(target) - mobj->z;
	const fixed_t distance = distance_to_target(mobj);
	const fixed_t travelDistance = kReboundSpeed * mapobjectscale;

	if (distance <= travelDistance)
	{
		rebound_mode(mobj) = static_cast<int>(Mode::kOrbit);
		S_StartSoundAtVolume(mobj, sfx_grbnd2, 255/2);

		// Freeze
		mobj->momx = 0;
		mobj->momy = 0;
		mobj->momz = 0;
	}
	else
	{
		const angle_t facing = R_PointToAngle2(mobj->x, mobj->y, target->x, target->y);

		P_InstaThrust(mobj, facing, travelDistance);
		mobj->angle = facing;

		// This has a nice effect of "jumping up" rather quickly
		mobj->momz = zDelta / 4;

		const tic_t t = distance_to_target(mobj) / travelDistance;
		const fixed_t newSpeed = std::abs(mobj->scale - mobj->destscale) / std::max<tic_t>(t, 1u);

		if (newSpeed > mobj->scalespeed)
		{
			mobj->scalespeed = newSpeed;
		}

		if (!played_rebound_sound(mobj))
			S_StartSoundAtVolume(mobj, sfx_grbnd1, 255/2);
		played_rebound_sound(mobj) = true;
	}
}

void orbit_target(mobj_t* mobj)
{
	if (award_target(mobj))
	{
		mobj->fuse = kReboundAcceptDuration + BUFFER_TICS;
		mobj->destscale = 0;
		mobj->scalespeed = mobj->scale / kReboundAcceptDuration;
	}

	const mobj_t* target = rebound_target(mobj);
	const fixed_t rad = (2 * (mobj->radius + target->radius)) + (kOrbitRadius * mapobjectscale);

	P_MoveOrigin(mobj,
		target->x - FixedMul(FCOS(mobj->angle), rad),
		target->y - FixedMul(FSIN(mobj->angle), rad),
		target->z + (target->height / 2));

	constexpr angle_t kOrbitSpeed = ANGLE_MAX / (kReboundAcceptPause + kReboundAcceptDuration);

	mobj->angle -= 2 * kOrbitSpeed;
}

// Copied from MT_BANANA_SPARK
void squish(mobj_t* mobj)
{
	if (leveltime & 1)
	{
		mobj->spritexscale = mobj->spriteyscale = FRACUNIT;
	}
	else
	{
		if ((leveltime / 2) & 1)
		{
			mobj->spriteyscale = 3*FRACUNIT;
		}
		else
		{
			mobj->spritexscale = 3*FRACUNIT;
		}
	}
}

void spawn_afterimages(mobj_t* mobj)
{
	mobj_t* ghost = P_SpawnGhostMobj(mobj);

	// Flickers every frame
	ghost->extravalue1 = 1;
	ghost->extravalue2 = 2;

	// No transparency
	ghost->renderflags = 0;

	ghost->tics = 8;
}

}; // namespace

void Obj_GachaBomReboundThink(mobj_t* mobj)
{
	if (P_MobjWasRemoved(rebound_target(mobj)))
	{
		P_RemoveMobj(mobj);
		return;
	}

	if (static_cast<Mode>(rebound_mode(mobj)) == Mode::kOrbit)
	{
		// Ready to be delivered to the player
		orbit_target(mobj);
		squish(mobj);
	}
	else if (rebound_is_returning(mobj))
	{
		// Travelling back
		chase_rebound_target(mobj);
		squish(mobj);
		spawn_afterimages(mobj);
	}

	// Now the gummy animation is over
	if (mobj->sprite == SPR_GBOM)
	{
		if (static_cast<Mode>(rebound_mode(mobj)) == Mode::kBeta)
		{
			// Only the alpha object remains
			P_RemoveMobj(mobj);
			return;
		}
	}
}

void Obj_SpawnGachaBomRebound(mobj_t* source, mobj_t* target)
{
	auto spawn = [&](angle_t angle, Mode mode)
	{
		mobj_t *x = P_SpawnMobjFromMobjUnscaled(source, 0, 0, target->height / 2, MT_GACHABOM_REBOUND);

		x->color = target->player ? target->player->skincolor : target->color;
		x->angle = angle;

		if (!(gametyperules & GTR_BUMPERS) || battleprisons)
		{
			P_InstaScale(x, 2 * x->scale);
		}

		rebound_mode(x) = static_cast<int>(mode);
		rebound_timer(x) = kReboundAcceptPause;
		played_rebound_sound(x) = false;

		P_SetTarget(&rebound_target(x), target);
	};

	spawn(0, Mode::kAlpha);
	spawn(ANGLE_45, Mode::kBeta);
	spawn(ANGLE_90, Mode::kBeta);
	spawn(ANGLE_135, Mode::kBeta);
}
