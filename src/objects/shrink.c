// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
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

#define POHBEE_HOVER (384 << FRACBITS)
#define POHBEE_SPEED (128 << FRACBITS)
#define POHBEE_TIME (15 * TICRATE)
#define POHBEE_DIST (4096 << FRACBITS)

#define LASER_SPEED (20 << FRACBITS)
#define LASER_SWINGTIME (3 * TICRATE)

#define CHAIN_SIZE (16)

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

#define pohbee_owner(o) ((o)->target)
#define pohbee_lasers(o) ((o)->hnext)

#define laser_offset(o) ((o)->movecount)
#define laser_swing(o) ((o)->movedir)
#define laser_numsegs(o) ((o)->extravalue1)

#define laser_pohbee(o) ((o)->target)
#define laser_collider(o) ((o)->tracer)
#define laser_chains(o) ((o)->hprev)

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

static fixed_t PohbeeWaypointZ(mobj_t *dest)
{
	return dest->z + (FixedMul(POHBEE_HOVER, mapobjectscale) * P_MobjFlip(dest));
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
		fixed_t wpZ = PohbeeWaypointZ(curWaypoint->mobj);

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
	pohbee->angle = K_MomentumAngle(pohbee);

	if (finalize == true)
	{
		// Move to next state
		pohbee_mode(pohbee) = POHBEE_MODE_ACT;
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

static void DoLaserSwing(mobj_t *laser, mobj_t *pohbee)
{
	const angle_t angle = laser->angle + ANGLE_90;
	const tic_t swingTimer = leveltime + laser_offset(laser);

	const angle_t swingAmt = swingTimer * (ANGLE_MAX / LASER_SWINGTIME);
	const fixed_t swingCos = FINECOSINE(swingAmt >> ANGLETOFINESHIFT);
	const fixed_t swing = FixedMul(laser_swing(laser), 9 * swingCos);

	const angle_t pitch = -ANGLE_90 + swing;
	const fixed_t dist = laser_numsegs(laser) * CHAIN_SIZE * laser->scale;

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

	PohbeeMoveTo(laser, pohbee->x + offsetX, pohbee->y + offsetY, pohbee->z + offsetZ);
}

static void ShrinkLaserThinker(mobj_t *laser)
{
	mobj_t *pohbee = laser_pohbee(laser);

	if (pohbee == NULL || P_MobjWasRemoved(pohbee) == true)
	{
		P_RemoveMobj(laser);
		return;
	}

	laser->angle = pohbee->angle;
	DoLaserSwing(laser, pohbee);

	//PohbeeMoveTo(laser_collider(laser), laser->x, laser->y, laser->z);
}

void Obj_PohbeeThinker(mobj_t *pohbee)
{
	mobj_t *laser = NULL;

	pohbee->momx = pohbee->momy = pohbee->momz = 0;

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

	laser = pohbee_lasers(pohbee);
	while (laser != NULL && P_MobjWasRemoved(laser) == false)
	{
		ShrinkLaserThinker(laser);
		laser = pohbee_lasers(laser);
	}
}

/*
void Obj_PohbeeRemoved(mobj_t *pohbee)
{
	mobj_t *chain = NULL;

	if (pohbee_laser(pohbee) != NULL)
	{
		P_RemoveMobj(pohbee_laser(pohbee));
	}

	chain = pohbee_chain(pohbee);
	while (chain != NULL)
	{
		mobj_t *temp = chain;
		chain = pohbee_chain(temp);
		P_RemoveMobj(temp);
	}
}
*/

static waypoint_t *GetPohbeeStart(waypoint_t *anchor)
{
	const UINT32 traveldist = FixedMul(POHBEE_DIST >> 1, mapobjectscale) / FRACUNIT;
	const boolean useshortcuts = false;
	const boolean huntbackwards = true;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};
	waypoint_t *ret = NULL;

	pathfindsuccess = K_PathfindThruCircuit(
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

static waypoint_t *GetPohbeeEnd(waypoint_t *anchor)
{
	const UINT32 traveldist = FixedMul(POHBEE_DIST, mapobjectscale) / FRACUNIT;
	const boolean useshortcuts = false;
	const boolean huntbackwards = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};
	waypoint_t *ret = NULL;

	pathfindsuccess = K_PathfindThruCircuit(
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

static void CreatePohbee(player_t *owner, waypoint_t *start, waypoint_t *end, UINT8 numLasers)
{
	mobj_t *pohbee = NULL;

	fixed_t size = INT32_MAX;
	INT32 baseSegs = INT32_MAX;
	INT32 segVal = INT32_MAX;
	mobj_t *prevLaser = NULL;

	size_t i, j;

	if (owner == NULL || owner->mo == NULL || P_MobjWasRemoved(owner->mo) == true
		|| start == NULL || end == NULL
		|| numLasers == 0)
	{
		// Invalid inputs
		return;
	}

	// Calculate number of chain segments added per laser.
	size = end->mobj->radius / mapobjectscale;
	baseSegs = 1 + (size / CHAIN_SIZE);

	if (baseSegs < MAXPLAYERS)
	{
		baseSegs = MAXPLAYERS;
	}

	segVal = baseSegs / numLasers;

	// Valid spawning conditions,
	// we can start creating each individual part.
	pohbee = P_SpawnMobjFromMobj(start->mobj, 0, 0, POHBEE_HOVER * 3, MT_SHRINK_POHBEE);
	P_SetTarget(&pohbee_owner(pohbee), owner->mo);

	pohbee_mode(pohbee) = POHBEE_MODE_SPAWN;
	pohbee_timer(pohbee) = POHBEE_TIME;

	pohbee_waypoint_cur(pohbee) = (INT32)K_GetWaypointHeapIndex(start);
	pohbee_waypoint_dest(pohbee) = (INT32)K_GetWaypointHeapIndex(end);

	prevLaser = pohbee;

	for (i = 0; i < numLasers; i++)
	{
		const UINT8 numSegs = segVal * (i + 1);

		mobj_t *laser = P_SpawnMobjFromMobj(pohbee, 0, 0, 0, MT_SHRINK_LASER);
		//mobj_t *collider = NULL;
		//mobj_t *prevChain = NULL;

		P_SetTarget(&laser_pohbee(laser), pohbee);
		P_SetTarget(&pohbee_lasers(prevLaser), laser);

		laser_numsegs(laser) = numSegs;
		laser_swing(laser) = (ANGLE_45 * baseSeg) / numSegs;
		laser_offset(laser) = P_RandomKey(LASER_SWINGTIME);

		/*
		prevChain = laser;
		for (j = 0; j < numSegs; j++)
		{
			mobj_t *chain = P_SpawnMobjFromMobj(laser, 0, 0, 0, MT_SHRINK_LASER);
			P_SetTarget(&laser_chains(prevChain), chain);
			prevChain = chain;
		}
		*/
		(void)j;

		prevLaser = laser;
	}
}

void Obj_CreateShrinkPohbees(player_t *owner)
{
	UINT8 ownerPos = 1;

	struct {
		waypoint_t *start;
		waypoint_t *end;
		UINT8 lasers;
	} pohbees[MAXPLAYERS];
	size_t numPohbees = 0;

	size_t i, j;

	if (owner == NULL || owner->mo == NULL || P_MobjWasRemoved(owner->mo) == true)
	{
		return;
	}

	ownerPos = owner->position;

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
			pohbees[j].lasers = 4;
			numPohbees++;
		}
	}

	for (i = 0; i < numPohbees; i++)
	{
		CreatePohbee(owner, pohbees[i].start, pohbees[i].end, pohbees[i].lasers);
	}
}
