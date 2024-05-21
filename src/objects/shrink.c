// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  shrink.c
/// \brief Shrink laser item code.

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
#include "../music.h"
#include "../m_easing.h"

#define POHBEE_HOVER (128 << FRACBITS)
#define POHBEE_SPEED (128 << FRACBITS)
#define POHBEE_TIME (30 * TICRATE)
#define POHBEE_DIST (4096 << FRACBITS)

#define GUN_SWING (ANGLE_90 - ANG10)
#define GUN_SWINGTIME (4 * TICRATE)

#define CHAIN_SIZE (52)

#define EXTRA_FOR_FIRST (7)

enum
{
	POHBEE_MODE_SPAWN,
	POHBEE_MODE_ACT,
	POHBEE_MODE_DESPAWN,
};

#define pohbee_mode(o) ((o)->cusval)
#define pohbee_timer(o) ((o)->reactiontime)
#define pohbee_waypoint_cur(o) ((o)->extravalue1)
#define pohbee_waypoint_dest(o) ((o)->extravalue2)
#define pohbee_height(o) ((o)->movefactor)
#define pohbee_destangle(o) ((o)->movedir)

#define pohbee_owner(o) ((o)->cvmem)
#define pohbee_guns(o) ((o)->hnext)

#define gun_offset(o) ((o)->movecount)
#define gun_numsegs(o) ((o)->extravalue1)

#define gun_pohbee(o) ((o)->target)
#define gun_laser(o) ((o)->tracer)
#define gun_chains(o) ((o)->hprev)
#define gun_overlay(o) ((o)->itnext)

#define chain_index(o) ((o)->extravalue1)

enum
{
	LASER_SHRINK,
	LASER_GROW,
};

static player_t *get_pohbee_owner(mobj_t *pohbee)
{
	UINT8 p = pohbee_owner(pohbee);
	return p < MAXPLAYERS && playeringame[p] ? &players[p] : NULL;
}

static skincolornum_t ShrinkLaserColor(mobj_t *pohbee)
{
	UINT8 laserState = LASER_SHRINK;
	player_t *owner = get_pohbee_owner(pohbee);

	if (owner != NULL && P_IsDisplayPlayer(owner) == true)
	{
		laserState = LASER_GROW;

		if (r_splitscreen > 0 && (leveltime & 1))
		{
			// TODO: make this properly screen dependent,
			// instead of flashing.
			laserState = LASER_SHRINK;
		}
	}

	switch (laserState)
	{
		default:
		case LASER_SHRINK:
			return SKINCOLOR_KETCHUP;

		case LASER_GROW:
			return SKINCOLOR_SAPPHIRE;
	}
}

static boolean ShrinkLaserActive(mobj_t *pohbee)
{
	return (pohbee_mode(pohbee) == POHBEE_MODE_ACT);
}

static void PohbeeMoveTo(mobj_t *pohbee, fixed_t destx, fixed_t desty, fixed_t destz)
{
	pohbee->momx = destx - pohbee->x;
	pohbee->momy = desty - pohbee->y;
	pohbee->momz = destz - pohbee->z;
}

static fixed_t GenericDistance(
	fixed_t curx, fixed_t cury, fixed_t curz,
	fixed_t destx, fixed_t desty, fixed_t destz)
{
	return P_AproxDistance(P_AproxDistance(destx - curx, desty - cury), destz - curz);
}

static fixed_t PohbeeWaypointZ(mobj_t *pohbee, mobj_t *dest)
{
	return dest->z + (pohbee_height(pohbee) + FixedMul(POHBEE_HOVER, mapobjectscale)) * P_MobjFlip(dest);
}

static void PohbeeSpawn(mobj_t *pohbee)
{
	waypoint_t *curWaypoint = NULL;
	waypoint_t *destWaypoint = NULL;

	fixed_t distLeft = INT32_MAX;
	fixed_t newX = pohbee->x;
	fixed_t newY = pohbee->y;
	fixed_t newZ = pohbee->z;

	boolean finalize = false;

	const boolean useshortcuts = false;
	const boolean huntbackwards = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};
	size_t pathIndex = 0;

	curWaypoint = K_GetWaypointFromIndex((size_t)pohbee_waypoint_cur(pohbee));
	destWaypoint = K_GetWaypointFromIndex((size_t)pohbee_waypoint_dest(pohbee));

	if (curWaypoint == NULL || destWaypoint == NULL)
	{
		// Waypoints aren't valid.
		// Just transition into the next state.
		pohbee_mode(pohbee) = POHBEE_MODE_ACT;
		return;
	}

	distLeft = FixedMul(POHBEE_SPEED, mapobjectscale);

	while (distLeft > 0)
	{
		fixed_t wpX = curWaypoint->mobj->x;
		fixed_t wpY = curWaypoint->mobj->y;
		fixed_t wpZ = PohbeeWaypointZ(pohbee, curWaypoint->mobj);

		fixed_t distToNext = GenericDistance(
			newX, newY, newZ,
			wpX, wpY, wpZ
		);

		if (distToNext > distLeft)
		{
			// Only made it partially there.
			newX += FixedMul(FixedDiv(wpX - newX, distToNext), distLeft);
			newY += FixedMul(FixedDiv(wpY - newY, distToNext), distLeft);
			newZ += FixedMul(FixedDiv(wpZ - newZ, distToNext), distLeft);

			distLeft = 0;
		}
		else
		{
			// Close enough to the next waypoint,
			// move there and remove the distance.
			newX = wpX;
			newY = wpY;
			newZ = wpZ;

			distLeft -= distToNext;

			if (curWaypoint == destWaypoint)
			{
				// Reached the end.
				finalize = true;
				break;
			}

			// Create waypoint path to our destination.
			// Crazy over-engineered, just to catch when
			// waypoints are insanely close to each other :P
			if (pathfindsuccess == false)
			{
				pathfindsuccess = K_PathfindToWaypoint(
					curWaypoint, destWaypoint,
					&pathtofinish,
					useshortcuts, huntbackwards
				);

				if (pathfindsuccess == false)
				{
					// Path isn't valid.
					// Just transition into the next state.
					finalize = true;
					break;
				}
			}

			pathIndex++;

			if (pathIndex >= pathtofinish.numnodes)
			{
				// Successfully reached the end of the path.
				finalize = true;
				break;
			}

			// Now moving to the next waypoint.
			curWaypoint = (waypoint_t *)pathtofinish.array[pathIndex].nodedata;
			pohbee_waypoint_cur(pohbee) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
		}
	}

	PohbeeMoveTo(pohbee, newX, newY, newZ);
	pohbee_destangle(pohbee) = K_MomentumAngle(pohbee);

	if (finalize == true)
	{
		// Move to next state
		pohbee_mode(pohbee) = POHBEE_MODE_ACT;
		pohbee_destangle(pohbee) += ANGLE_180;
	}

	if (pathfindsuccess == true)
	{
		Z_Free(pathtofinish.array);
	}
}

static void PohbeeAct(mobj_t *pohbee)
{
	pohbee_timer(pohbee)--;

	if (pohbee_timer(pohbee) <= 0)
	{
		// Move to next state
		pohbee_mode(pohbee) = POHBEE_MODE_DESPAWN;
		pohbee->fuse = 5*TICRATE;
	}
}

static void PohbeeDespawn(mobj_t *pohbee)
{
	pohbee->momz = 16 * pohbee->scale * P_MobjFlip(pohbee);
}

static void DoGunSwing(mobj_t *gun, mobj_t *pohbee)
{
	const angle_t angle = gun->angle + ANGLE_90;
	const tic_t swingTimer = leveltime + gun_offset(gun);

	const angle_t swingAmt = swingTimer * (ANGLE_MAX / GUN_SWINGTIME);
	const fixed_t swingCos = FINECOSINE(swingAmt >> ANGLETOFINESHIFT);

	const angle_t pitch = -ANGLE_90 + FixedMul(swingCos, GUN_SWING);
	const fixed_t dist = gun_numsegs(gun) * CHAIN_SIZE * gun->scale;

	fixed_t offsetX = FixedMul(
		dist, FixedMul(
			FINECOSINE(angle >> ANGLETOFINESHIFT),
			FINECOSINE(pitch >> ANGLETOFINESHIFT)
		)
	);

	fixed_t offsetY = FixedMul(
		dist, FixedMul(
			FINESINE(angle >> ANGLETOFINESHIFT),
			FINECOSINE(pitch >> ANGLETOFINESHIFT)
		)
	);

	fixed_t offsetZ = FixedMul(
		dist, FINESINE(pitch >> ANGLETOFINESHIFT)
	);

	// When in reverse gravity, flip the z offset and make up the difference in height between the Poh-Bee and its gun.
	if (P_IsObjectFlipped(gun))
	{
		offsetZ *= -1;
		offsetZ += pohbee->height - gun->height;
	}

	PohbeeMoveTo(gun, pohbee->x + offsetX, pohbee->y + offsetY, pohbee->z + offsetZ);
}

static void ShrinkLaserThinker(mobj_t *pohbee, mobj_t *gun, mobj_t *laser)
{
	const fixed_t gunX = gun->x + gun->momx;
	const fixed_t gunY = gun->y + gun->momy;
	const fixed_t gunZ = P_GetMobjFeet(gun) + gun->momz;
	fixed_t groundZ, spriteHeight;

	// Target the ceiling in reverse gravity, otherwise target the floor.
	if (P_IsObjectFlipped(laser))
	{
		groundZ = gun->ceilingz - laser->height;
		spriteHeight = gun->ceilingz - gunZ;
	}
	else
	{
		groundZ = gun->floorz;
		spriteHeight = gunZ - gun->floorz;
	}

	PohbeeMoveTo(laser, gunX, gunY, groundZ);

	if (ShrinkLaserActive(pohbee) == true)
	{
		mobj_t *particle = NULL;

		laser->renderflags &= ~RF_DONTDRAW;
		laser->color = ShrinkLaserColor(pohbee);

		if (leveltime & 1)
		{
			laser->spritexscale = 5*FRACUNIT/2;
		}
		else
		{
			laser->spritexscale = FRACUNIT;
		}

		laser->spriteyscale = FixedDiv(FixedDiv(spriteHeight, mapobjectscale), laser->info->height);

		particle = P_SpawnMobjFromMobj(laser, 0, 0, 0, MT_SHRINK_PARTICLE);

		particle->sprxoff = P_RandomRange(PR_DECORATION, -16, 16) * laser->scale;
		particle->spryoff = P_RandomRange(PR_DECORATION, -16, 16) * laser->scale;

		P_SetTarget(&gun_pohbee(particle), pohbee);

		particle->color = laser->color;

		P_SetScale(particle, particle->scale * 2);
		particle->cusval = particle->scale; // Store for later.
		particle->destscale = 0;

		//particle->momz = 2 * particle->scale * P_MobjFlip(particle);

		if (S_SoundPlaying(laser, sfx_beam01) == false)
		{
			S_StartSound(laser, sfx_beam01);
		}
	}
	else
	{
		laser->renderflags |= RF_DONTDRAW;

		if (S_SoundPlaying(laser, sfx_beam01) == true)
		{
			S_StopSound(laser);
		}
	}
}

static void DoGunChains(mobj_t *gun, mobj_t *pohbee)
{
	const fixed_t gunX = gun->x + gun->momx;
	const fixed_t gunY = gun->y + gun->momy;
	const fixed_t gunZ = P_GetMobjHead(gun) + gun->momz;

	const fixed_t beeX = pohbee->x + pohbee->momx;
	const fixed_t beeY = pohbee->y + pohbee->momy;
	const fixed_t beeZ = P_GetMobjFeet(pohbee) + pohbee->momz;

	const fixed_t offsetX = (beeX - gunX) / gun_numsegs(gun);
	const fixed_t offsetY = (beeY - gunY) / gun_numsegs(gun);
	const fixed_t offsetZ = (beeZ - gunZ) / gun_numsegs(gun);

	mobj_t *chain = NULL;

	fixed_t curX = gunX + (offsetX / 2);
	fixed_t curY = gunY + (offsetY / 2);
	fixed_t curZ = gunZ + (offsetZ / 2);

	// The starting z coordinate is the bottom of the chain at the top of the gun.
	// In reverse gravity, offset the z coordinate by the height of a chain, so that it's the top of the chain at the bottom of the gun.
	if (P_IsObjectFlipped(gun))
	{
		curZ -= FixedMul(gun->scale, mobjinfo[MT_SHRINK_CHAIN].height);
	}

	chain = gun_chains(gun);
	while (chain != NULL && P_MobjWasRemoved(chain) == false)
	{
		PohbeeMoveTo(chain, curX, curY, curZ);

		curX += offsetX;
		curY += offsetY;
		curZ += offsetZ;

		chain = gun_chains(chain);
	}
}

static void ShrinkGunThinker(mobj_t *gun)
{
	mobj_t *pohbee = gun_pohbee(gun);
	skincolornum_t gunColor = SKINCOLOR_GREY;

	if (pohbee == NULL || P_MobjWasRemoved(pohbee) == true)
	{
		P_RemoveMobj(gun);
		return;
	}

	if (pohbee_mode(pohbee) == POHBEE_MODE_SPAWN)
	{
		gun->angle = pohbee->angle;
	}

	if (get_pohbee_owner(pohbee) != NULL)
	{
		gunColor = get_pohbee_owner(pohbee)->skincolor;
	}

	gun->color = gunColor;

	if (gun_overlay(gun) != NULL && P_MobjWasRemoved(gun_overlay(gun)) == false)
	{
		gun_overlay(gun)->color = ShrinkLaserColor(pohbee);
	}

	DoGunSwing(gun, pohbee);

	if (gun_laser(gun) != NULL && P_MobjWasRemoved(gun_laser(gun)) == false)
	{
		ShrinkLaserThinker(pohbee, gun, gun_laser(gun));
	}

	DoGunChains(gun, pohbee);
}

void Obj_PohbeeThinker(mobj_t *pohbee)
{
	mobj_t *gun = NULL;

	K_SetItemCooldown(KITEM_SHRINK, 60*TICRATE);

	pohbee->momx = pohbee->momy = pohbee->momz = 0;
	pohbee->spritexscale = pohbee->spriteyscale = 2*FRACUNIT;

	switch (pohbee_mode(pohbee))
	{
		case POHBEE_MODE_SPAWN:
			PohbeeSpawn(pohbee);
			break;

		case POHBEE_MODE_ACT:
			PohbeeAct(pohbee);
			break;

		case POHBEE_MODE_DESPAWN:
			PohbeeDespawn(pohbee);
			break;

		default:
			// failsafe
			pohbee_mode(pohbee) = POHBEE_MODE_SPAWN;
			break;
	}

	pohbee->angle += AngleDeltaSigned(pohbee_destangle(pohbee), pohbee->angle) / 8;

	gun = pohbee_guns(pohbee);
	while (gun != NULL && P_MobjWasRemoved(gun) == false)
	{
		ShrinkGunThinker(gun);
		gun = pohbee_guns(gun);
	}
}

void Obj_PohbeeRemoved(mobj_t *pohbee)
{
	mobj_t *gun = pohbee_guns(pohbee);

	while (gun != NULL && P_MobjWasRemoved(gun) == false)
	{
		mobj_t *nextGun = pohbee_guns(gun);
		P_RemoveMobj(gun);
		gun = nextGun;
	}

	P_SetTarget(&pohbee_guns(pohbee), NULL);
}

void Obj_ShrinkGunRemoved(mobj_t *gun)
{
	mobj_t *chain = NULL;

	if (gun_laser(gun) != NULL && P_MobjWasRemoved(gun_laser(gun)) == false)
	{
		P_RemoveMobj(gun_laser(gun));
	}

	P_SetTarget(&gun_laser(gun), NULL);

	chain = gun_chains(gun);
	while (chain != NULL && P_MobjWasRemoved(chain) == false)
	{
		mobj_t *nextChain = gun_chains(chain);
		P_RemoveMobj(chain);
		chain = nextChain;
	}

	P_SetTarget(&gun_chains(gun), NULL);
}

boolean Obj_ShrinkLaserCollide(mobj_t *gun, mobj_t *victim)
{
	mobj_t *pohbee = gun_pohbee(gun);
	player_t *owner = NULL;
	INT32 prevTimer = 0;

	if (pohbee == NULL || P_MobjWasRemoved(pohbee) == true)
	{
		return true;
	}

	if (ShrinkLaserActive(pohbee) == false)
	{
		return true;
	}

	if (victim->player->shrinkLaserDelay > 0)
	{
		victim->player->shrinkLaserDelay = TICRATE;
		return true;
	}

	victim->player->shrinkLaserDelay = TICRATE;

	owner = get_pohbee_owner(pohbee);
	prevTimer = victim->player->growshrinktimer;

	fixed_t scale = FRACUNIT; // Used if you hit the gun/laser.

	if (gun->type == MT_SHRINK_PARTICLE && gun->cusval != 0) // Hit the laser trail, scale the punishment down.
	{
		fixed_t normalizer = FixedDiv(FRACUNIT, gun->cusval); // cusval = original scale of the particle, as it eases down to 0
		scale = FixedMul(gun->scale, normalizer);
	}

	if (owner != NULL && victim->player == owner)
	{
		// Belongs to us. Give us Grow!
		if (prevTimer < 0)
		{
			// Take away Shrink.
			K_RemoveGrowShrink(victim->player);
		}

		UINT8 oldGrow = max(victim->player->growshrinktimer, 0);
		fixed_t easePercent = min(oldGrow * 6*TICRATE / FRACUNIT, FRACUNIT);
		victim->player->growshrinktimer += Easing_OutSine(easePercent, 6*TICRATE, 2*TICRATE);

		S_StartSound(victim, sfx_kc5a);

		if (victim->player->roundconditions.consecutive_grow_lasers < UINT8_MAX)
		{
			victim->player->roundconditions.consecutive_grow_lasers++;
			if (victim->player->roundconditions.consecutive_grow_lasers > victim->player->roundconditions.best_consecutive_grow_lasers)
			{
				victim->player->roundconditions.best_consecutive_grow_lasers
					= victim->player->roundconditions.consecutive_grow_lasers;
			}
		}

		if (prevTimer <= 0)
		{
			victim->scalespeed = mapobjectscale/TICRATE;
			victim->destscale = FixedMul(mapobjectscale, GROW_SCALE);

			if (K_PlayerShrinkCheat(victim->player) == true)
			{
				victim->destscale = FixedMul(victim->destscale, SHRINK_SCALE);
			}

			if (P_IsPartyPlayer(victim->player) == false && victim->player->invincibilitytimer == 0)
			{
				// don't play this if the player has invincibility -- that takes priority
				S_StartSound(victim, sfx_alarmg);
			}
		}
	}
	else
	{
		// Bullshit contact. Let 'em off for free.
		if (scale < FRACUNIT/3)
			return true;

		scale = Easing_InSine(scale, 0, FRACUNIT);

		if (prevTimer > 0)
		{
			// Dock some Grow time.
			// (Hack-adjacent: Always make sure there's a tic left so standard timer handling can remove the effect properly.)
			victim->player->growshrinktimer -= min(FixedInt(FixedMul(FRACUNIT*3*TICRATE/2, scale)), victim->player->growshrinktimer - 1);
			S_StartSound(victim, sfx_s3k40);
		}
		else
		{
			// Start shrinking!
			victim->player->growshrinktimer -= FixedInt(FixedMul(FRACUNIT*5*TICRATE, scale));
			S_StartSound(victim, sfx_kc59); // I don't think you ever get to hear this while the pohbee laser is in your teeth, but best effort.

			if (prevTimer >= 0)
			{
				//K_DropItems(victim->player);

				victim->scalespeed = mapobjectscale/TICRATE;
				victim->destscale = FixedMul(mapobjectscale, SHRINK_SCALE);

				if (K_PlayerShrinkCheat(victim->player) == true)
				{
					victim->destscale = FixedMul(victim->destscale, SHRINK_SCALE);
				}
			}
		}
	}

	return true;
}

static waypoint_t *GetPohbeeWaypoint(waypoint_t *anchor, const UINT32 traveldist, const boolean huntbackwards)
{
	const boolean useshortcuts = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};
	waypoint_t *ret = NULL;

	pathfindsuccess = K_PathfindThruCircuitSpawnable(
		anchor, traveldist,
		&pathtofinish,
		useshortcuts, huntbackwards
	);

	if (pathfindsuccess == true)
	{
		ret = (waypoint_t *)pathtofinish.array[ pathtofinish.numnodes - 1 ].nodedata;
		Z_Free(pathtofinish.array);
	}
	else
	{
		ret = anchor;
	}

	return ret;

}

static waypoint_t *GetPohbeeStart(waypoint_t *anchor)
{
	const UINT32 traveldist = FixedMul(POHBEE_DIST >> 1, mapobjectscale) / FRACUNIT;
	const boolean huntbackwards = true;

	return GetPohbeeWaypoint(anchor, traveldist, huntbackwards);
}

static waypoint_t *GetPohbeeEnd(waypoint_t *anchor)
{
	const UINT32 traveldist = FixedMul(POHBEE_DIST, mapobjectscale) / FRACUNIT;
	const boolean huntbackwards = false;

	return GetPohbeeWaypoint(anchor, traveldist, huntbackwards);
}

static void CreatePohbee(player_t *owner, waypoint_t *start, waypoint_t *end, UINT8 numLasers)
{
	mobj_t *pohbee = NULL;

	fixed_t size = 0;
	INT32 baseSegs = INT32_MAX;
	INT32 segVal = INT32_MAX;
	mobj_t *prevGun = NULL;

	size_t i, j;

	if (owner == NULL || owner->mo == NULL || P_MobjWasRemoved(owner->mo) == true
		|| start == NULL || end == NULL
		|| numLasers == 0)
	{
		// Invalid inputs
		return;
	}

	// Calculate number of chain segments added per laser.
	size = FixedMul(end->mobj->radius, 3*FRACUNIT/2);
	segVal = max(1, 1 + ((size / start->mobj->scale) / CHAIN_SIZE) / numLasers);
	baseSegs = segVal * numLasers;

	// Valid spawning conditions,
	// we can start creating each individual part.
	pohbee = P_SpawnMobjFromMobj(start->mobj, 0, 0, (baseSegs * CHAIN_SIZE * FRACUNIT) + POHBEE_HOVER * 3, MT_SHRINK_POHBEE);
	pohbee_owner(pohbee) = owner - players;

	// Flip the Poh-Bee if its target waypoint is flipped.
	const boolean waypointflipped = P_IsObjectFlipped(end->mobj);
	if (waypointflipped != P_IsObjectFlipped(pohbee))
	{
		pohbee->flags2 ^= MF2_OBJECTFLIP;
		pohbee->eflags ^= MFE_VERTICALFLIP;
	}
	if (waypointflipped) // now equivalent to P_IsObjectFlipped(pohbee)
	{
		size += pohbee->height - end->mobj->height;
	}

	pohbee_mode(pohbee) = POHBEE_MODE_SPAWN;
	pohbee_timer(pohbee) = POHBEE_TIME;
	pohbee_height(pohbee) = size;

	pohbee_waypoint_cur(pohbee) = (INT32)K_GetWaypointHeapIndex(start);
	pohbee_waypoint_dest(pohbee) = (INT32)K_GetWaypointHeapIndex(end);

	prevGun = pohbee;

	for (i = 0; i < numLasers; i++)
	{
		const UINT8 numSegs = segVal * (i + 1);

		mobj_t *gun = P_SpawnMobjFromMobj(pohbee, 0, 0, 0, MT_SHRINK_GUN);
		mobj_t *overlay = NULL;
		mobj_t *laser = NULL;
		mobj_t *prevChain = NULL;

		P_SetTarget(&gun_pohbee(gun), pohbee);
		P_SetTarget(&pohbee_guns(prevGun), gun);

		gun_numsegs(gun) = numSegs;
		gun_offset(gun) = P_RandomKey(PR_ITEM_SHRINK, GUN_SWINGTIME);

		overlay = P_SpawnMobjFromMobj(gun, 0, 0, 0, MT_OVERLAY);

		P_SetTarget(&overlay->target, gun);
		P_SetTarget(&gun_overlay(gun), overlay);

		P_SetMobjState(overlay, S_SHRINK_GUN_OVERLAY);

		laser = P_SpawnMobjFromMobj(gun, 0, 0, 0, MT_SHRINK_LASER);
		P_SetTarget(&gun_laser(gun), laser);

		prevChain = gun;
		for (j = 0; j < numSegs; j++)
		{
			mobj_t *chain = P_SpawnMobjFromMobj(gun, 0, 0, 0, MT_SHRINK_CHAIN);

			P_SetTarget(&gun_chains(prevChain), chain);
			chain_index(chain) = j;

			prevChain = chain;
		}

		prevGun = gun;
	}
}

void Obj_CreateShrinkPohbees(player_t *owner)
{
	UINT8 ownerPos = 1;

	struct {
		waypoint_t *start;
		waypoint_t *end;
		UINT8 lasers;
		boolean first;
	} pohbees[MAXPLAYERS];
	size_t numPohbees = 0;

	size_t i, j;

	if (owner == NULL || owner->mo == NULL || P_MobjWasRemoved(owner->mo) == true)
	{
		return;
	}

	ownerPos = owner->position;

	g_darkness.start = leveltime;
	g_darkness.end = leveltime + POHBEE_TIME + DARKNESS_FADE_TIME;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;
		waypoint_t *endWaypoint = NULL;

		if (playeringame[i] == false)
		{
			// Not valid.
			continue;
		}

		player = &players[i];

		if (player->spectator == true)
		{
			// Not playing.
			continue;
		}

		if (player->position > ownerPos)
		{
			// Too far behind.
			continue;
		}

		if (player->nextwaypoint == NULL)
		{
			// No waypoint?
			continue;
		}

		endWaypoint = GetPohbeeEnd(player->nextwaypoint);

		for (j = 0; j < numPohbees; j++)
		{
			if (pohbees[j].end == endWaypoint)
			{
				// Increment laser count for the already existing poh-bee,
				// if another one would occupy the same space.
				pohbees[j].lasers++;
				break;
			}
		}

		if (j == numPohbees)
		{
			// Push a new poh-bee
			pohbees[j].start = GetPohbeeStart(player->nextwaypoint);
			pohbees[j].end = endWaypoint;
			pohbees[j].lasers = 1;
			pohbees[j].first = (player->position == 1);

			numPohbees++;
		}
	}

	for (i = 0; i < numPohbees; i++)
	{
		// omg pobby hi!!!
		CreatePohbee(owner, pohbees[i].start, pohbees[i].end, pohbees[i].lasers);

		if (pohbees[i].first == true)
		{
			// Add a chain of extra ones for 1st place.
			waypoint_t *prev = pohbees[i].end;

			for (j = 0; j < EXTRA_FOR_FIRST; j++)
			{
				waypoint_t *new = GetPohbeeEnd(prev);
				CreatePohbee(owner, prev, new, 1);
				prev = new;
			}
		}
	}
}
