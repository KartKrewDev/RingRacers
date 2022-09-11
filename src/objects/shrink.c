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

#define POHBEE_HOVER (300 << FRACBITS)
#define POHBEE_SPEED (128 << FRACBITS)
#define POHBEE_TIME (15 * TICRATE)
#define POHBEE_DIST (2048 << FRACBITS)
#define POHBEE_CHAIN (16)

#define LASER_SPEED (20 << FRACBITS)

enum
{
	POHBEE_MODE_SPAWN,
	POHBEE_MODE_ACT,
	POHBEE_MODE_DESPAWN,
};

#define pohbee_mode(o) ((o)->cusval)

#define pohbee_waypoint_cur(o) ((o)->extravalue1)
#define pohbee_waypoint_dest(o) ((o)->extravalue2)

#define pohbee_owner(o) ((o)->target)
#define pohbee_laser(o) ((o)->tracer)
#define pohbee_chain(o) ((o)->hnext)

#define laser_owner(o) ((o)->target)
#define laser_pohbee(o) ((o)->tracer)

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

	if (finalize == true)
	{
		pohbee_mode(pohbee) = POHBEE_MODE_ACT;
	}

	if (pathfindsuccess == true)
	{
		Z_Free(pathtofinish.array);
	}
}

void Obj_PohbeeThinker(mobj_t *pohbee)
{
	pohbee->momx = 0;
	pohbee->momy = 0;
	pohbee->momz = 0;

	switch (pohbee_mode(pohbee))
	{
		case POHBEE_MODE_SPAWN:
			PohbeeSpawn(pohbee);
			break;

		case POHBEE_MODE_ACT:
			//PohbeeSpawn(pohbee);
			break;

		case POHBEE_MODE_DESPAWN:
			//PohbeeSpawn(pohbee);
			break;

		default:
			// failsafe
			pohbee_mode(pohbee) = POHBEE_MODE_SPAWN;
			break;
	}
}

static void CreatePohbeeForPlayer(player_t *target, player_t *owner)
{
	const UINT32 traveldist = FixedMul(POHBEE_DIST, mapobjectscale) / FRACUNIT;

	mobj_t *pohbee = NULL;
	//mobj_t *laser = NULL;

	waypoint_t *startWaypoint = NULL;
	waypoint_t *endWaypoint = NULL;

	I_Assert(target != NULL);
	I_Assert(owner != NULL);

	if (target->mo == NULL || P_MobjWasRemoved(target->mo) == true)
	{
		// No object for the target.
		return;
	}

	// First, go backwards to find our starting point.
	{
		const boolean useshortcuts = false;
		const boolean huntbackwards = true;
		boolean pathfindsuccess = false;
		path_t pathtofinish = {0};

		pathfindsuccess = K_PathfindThruCircuit(
			target->nextwaypoint, traveldist,
			&pathtofinish,
			useshortcuts, huntbackwards
		);

		if (pathfindsuccess == true)
		{
			startWaypoint = (waypoint_t *)pathtofinish.array[ pathtofinish.numnodes - 1 ].nodedata;
			Z_Free(pathtofinish.array);
		}
		else
		{
			startWaypoint = target->nextwaypoint;
		}
	}

	// Now, go forwards to get our ending point.
	{
		const boolean useshortcuts = false;
		const boolean huntbackwards = false;
		boolean pathfindsuccess = false;
		path_t pathtofinish = {0};

		pathfindsuccess = K_PathfindThruCircuit(
			target->nextwaypoint, traveldist * 2,
			&pathtofinish,
			useshortcuts, huntbackwards
		);

		if (pathfindsuccess == true)
		{
			endWaypoint = (waypoint_t *)pathtofinish.array[ pathtofinish.numnodes - 1 ].nodedata;
			Z_Free(pathtofinish.array);
		}
		else
		{
			endWaypoint = target->nextwaypoint;
		}
	}

	// Try to repair an incomplete pair, just in case.
	if (startWaypoint == NULL && endWaypoint != NULL)
	{
		startWaypoint = endWaypoint;
	}

	if (endWaypoint == NULL && startWaypoint != NULL)
	{
		endWaypoint = startWaypoint;
	}

	if (startWaypoint == NULL || endWaypoint == NULL)
	{
		// Unable to create shrink lasers
		// due to invalid waypoint structure...
		return;
	}

	// Valid spawning conditions,
	// we can start creating each individual part.
	pohbee = P_SpawnMobjFromMobj(startWaypoint->mobj, 0, 0, POHBEE_HOVER, MT_SHRINK_POHBEE);

	P_SetTarget(&pohbee_owner(pohbee), owner->mo);

	pohbee_mode(pohbee) = POHBEE_MODE_SPAWN;

	pohbee_waypoint_cur(pohbee) = (INT32)K_GetWaypointHeapIndex(startWaypoint);
	pohbee_waypoint_dest(pohbee) = (INT32)K_GetWaypointHeapIndex(endWaypoint);
}

void Obj_CreateShrinkPohbees(player_t *owner)
{
	UINT8 ownerPos = 1;
	UINT8 i;

	I_Assert(owner != NULL);

	ownerPos = owner->position;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

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

		CreatePohbeeForPlayer(player, owner);
	}
}
