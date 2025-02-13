// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  manta-ring.c
/// \brief SPB Juicebox rings. See spb.c for their spawning.

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
#include "../k_respawn.h"

#define MANTA_RACETIME (90)
#define MANTA_MINTIME (15)
#define MANTA_SPRINTTIME (60)

#define MANTA_ALIVEGATE (0)
#define MANTA_DEADGATE (FF_TRANS80)

#define MANTA_SIZE (2 * FRACUNIT)
#define MANTA_SIZEUP (10)
#define MANTA_SIZESTRENGTH (1500)
#define MANTA_MAXRAMP (80)

#define MANTA_COLLIDE (80 * FRACUNIT)

#define MANTA_TURBO (40)
#define MANTA_FASTRAMP (17)
#define MANTA_MINPWR (10)

#define manta_delay(o) ((o)->fuse)
#define manta_timealive(o) ((o)->movecount)
#define manta_boostval(o) ((o)->extravalue1)
#define manta_laps(o) ((o)->extravalue2)
#define manta_touched(o) ((o)->cusval)

#define manta_owner(o) ((o)->target)
#define manta_chase(o) ((o)->tracer)

static boolean MantaAlreadyTouched(mobj_t *manta, player_t *player)
{
	INT32 touchFlag = 0;

	if (manta_chase(manta) != NULL && P_MobjWasRemoved(manta_chase(manta)) == false
		&& player->mo == manta_chase(manta))
	{
		return true;
	}

#if 0
	if (manta_laps(manta) < player->laps)
	{
		return true;
	}
#endif

	touchFlag = 1 << (player - players);
	return (manta_touched(manta) & touchFlag);
}

static void Obj_MantaCollide(mobj_t *manta, mobj_t *other)
{
	// Could hook this into actual mobj collide if desired.
	fixed_t distance = INT32_MAX;
	fixed_t size = INT32_MAX;

	INT32 addBoost = 0;
	INT32 touchFlag = 0;

	size_t i;

	distance = P_AproxDistance(P_AproxDistance(
		other->x - manta->x,
		other->y - manta->y),
		other->z - manta->z) - other->radius - manta->radius;

	size = FixedMul(MANTA_COLLIDE, mapobjectscale);

	if (distance > size)
	{
		return;
	}

	if (other->player != NULL) // Just in case other objects should be added?
	{
		if (MantaAlreadyTouched(manta, other->player))
		{
			return;
		}

		touchFlag = 1 << (other->player - players);
	}

	addBoost = manta_boostval(manta);

	if (manta_timealive(manta) < MANTA_FASTRAMP)
	{
		// Ramp up to max power.
		addBoost = FixedMul(addBoost * FRACUNIT, (manta_timealive(manta) * FRACUNIT) / MANTA_FASTRAMP);

		// Convert to integer
		addBoost = (addBoost + (FRACUNIT/2)) / FRACUNIT;

		// Cap it
		addBoost = max(MANTA_MINPWR, addBoost);
	}

	if (other->player != NULL)
	{
		UINT8 snd = 0;

		if (other->player->speedboost > FRACUNIT/4)
		{
			snd = other->player->gateSound;
			other->player->gateSound++;

			if (other->player->gateSound > 4)
			{
				other->player->gateSound = 4;
			}
		}
		else
		{
			other->player->gateSound = 0;
		}

		K_SpawnDriftBoostExplosion(other->player, 3);
		K_SpawnDriftElectricSparks(other->player, SKINCOLOR_CRIMSON, true);

		for (i = 0; i < 5; i++)
		{
			S_StopSoundByID(other, sfx_gate01 + i);
		}

		S_StartSound(other, sfx_gate01 + snd);
		other->player->gateBoost += addBoost/2;

		P_StartQuakeFromMobj(6, 12 * other->scale, 512 * other->scale, other);
	}

	if (touchFlag > 0)
	{
		manta_touched(manta) |= touchFlag;
	}
}

static void RunMantaCollide(mobj_t *manta)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			// Invalid
			continue;
		}

		player = &players[i];
		if (player->spectator == true)
		{
			// Not playing.
			continue;
		}

		if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			// Invalid object
			continue;
		}

		if (player->mo == manta_chase(manta))
		{
			// Don't allow the person being chased to touch this.
			continue;
		}

		Obj_MantaCollide(manta, player->mo);
	}
}

static void RunMantaVisual(mobj_t *manta)
{
	INT32 i;

	if (manta->fuse < 5*TICRATE)
	{
		if (leveltime & 1)
		{
			manta->renderflags |= RF_DONTDRAW;
		}
		else
		{
			manta->renderflags &= ~RF_DONTDRAW;
		}
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		const UINT8 pID = displayplayers[i];
		player_t *player = &players[pID];

		if (MantaAlreadyTouched(manta, player) == false)
		{
			break;
		}
	}

	if (i > r_splitscreen)
	{
		manta->frame = (manta->frame & ~FF_TRANSMASK) | MANTA_DEADGATE;
	}
	else
	{
		manta->frame = (manta->frame & ~FF_TRANSMASK) | MANTA_ALIVEGATE;
	}
}

void Obj_MantaRingThink(mobj_t *manta)
{
	RunMantaVisual(manta);

	if (manta_delay(manta) % MANTA_SIZEUP == 0)
	{
		manta->destscale += FixedMul(MANTA_SIZESTRENGTH, mapobjectscale);
		manta_boostval(manta) = min(MANTA_MAXRAMP, manta_boostval(manta) + 1);
	}

	manta_timealive(manta)++;

	RunMantaCollide(manta);
}

mobj_t *Obj_MantaRingCreate(mobj_t *spb, mobj_t *owner, mobj_t *chase)
{
	mobj_t *manta = NULL;
	INT32 delay = 0;

	manta = P_SpawnMobjFromMobj(spb, 0, 0, 0, MT_MANTARING);

	manta->color = SKINCOLOR_KETCHUP;

	manta->destscale = FixedMul(MANTA_SIZE, spb->scale);
	P_SetScale(manta, manta->destscale);

	manta->angle = R_PointToAngle2(0, 0, spb->momx, spb->momy) + ANGLE_90;

	// Set boost value
	manta_boostval(manta) = MANTA_TURBO;

	// Set despawn delay
	delay = max(MANTA_MINTIME, MANTA_RACETIME / mapheaderinfo[gamemap - 1]->numlaps);

	if (mapheaderinfo[gamemap - 1]->levelflags & LF_SECTIONRACE)
	{
		delay = MANTA_SPRINTTIME;
	}

	manta_delay(manta) = delay * TICRATE;

	// Default if neither object exists
	manta_laps(manta) = INT32_MAX;

	// Set owner
	if (owner != NULL && P_MobjWasRemoved(owner) == false)
	{
		P_SetTarget(&manta_owner(manta), owner);

		if (owner->player != NULL)
		{
			// Default if chaser doesn't exist
			manta_laps(manta) = owner->player->laps;
		}
	}

	// Set chaser
	if (chase != NULL && P_MobjWasRemoved(chase) == false)
	{
		P_SetTarget(&manta_chase(manta), chase);

		if (chase->player != NULL)
		{
			manta_laps(manta) = chase->player->laps;
		}
	}

	return manta;
}
