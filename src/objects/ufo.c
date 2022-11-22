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

#define UFO_BASE_SPEED (12 * FRACUNIT) // UFO's slowest speed.
#define UFO_SPEEDUP (FRACUNIT)
#define UFO_SLOWDOWN (FRACUNIT >> 2)
#define UFO_SPACING (1024 * FRACUNIT)
#define UFO_DEADZONE (512 * FRACUNIT)
#define UFO_SPEEDFACTOR (FRACUNIT * 9 / 10)

#define ufo_waypoint(o) ((o)->extravalue1)
#define ufo_distancetofinish(o) ((o)->extravalue2)
#define ufo_speed(o) ((o)->watertop)

static void UFOMoveTo(mobj_t *ufo, fixed_t destx, fixed_t desty, fixed_t destz)
{
	ufo->momx = destx - ufo->x;
	ufo->momy = desty - ufo->y;
	ufo->momz = destz - ufo->z;
}

static fixed_t GenericDistance(
	fixed_t curx, fixed_t cury, fixed_t curz,
	fixed_t destx, fixed_t desty, fixed_t destz)
{
	return P_AproxDistance(P_AproxDistance(destx - curx, desty - cury), destz - curz);
}

static void UFOUpdateDistanceToFinish(mobj_t *ufo)
{
	waypoint_t *finishLine = K_GetFinishLineWaypoint();
	waypoint_t *nextWaypoint = K_GetWaypointFromIndex((size_t)ufo_waypoint(ufo));

	if (nextWaypoint != NULL && finishLine != NULL)
	{
		const boolean useshortcuts = false;
		const boolean huntbackwards = false;
		boolean pathfindsuccess = false;
		path_t pathtofinish = {0};

		pathfindsuccess =
			K_PathfindToWaypoint(nextWaypoint, finishLine, &pathtofinish, useshortcuts, huntbackwards);

		// Update the UFO's distance to the finish line if a path was found.
		if (pathfindsuccess == true)
		{
			// Add euclidean distance to the next waypoint to the distancetofinish
			UINT32 adddist;
			fixed_t disttowaypoint =
				P_AproxDistance(
					(ufo->x >> FRACBITS) - (nextWaypoint->mobj->x >> FRACBITS),
					(ufo->y >> FRACBITS) - (nextWaypoint->mobj->y >> FRACBITS));
			disttowaypoint = P_AproxDistance(disttowaypoint, (ufo->z >> FRACBITS) - (nextWaypoint->mobj->z >> FRACBITS));

			adddist = (UINT32)disttowaypoint;

			ufo_distancetofinish(ufo) = pathtofinish.totaldist + adddist;
			Z_Free(pathtofinish.array);
		}
	}
}

static void UFOUpdateSpeed(mobj_t *ufo)
{
	const fixed_t baseSpeed = FixedMul(UFO_BASE_SPEED, K_GetKartGameSpeedScalar(gamespeed));
	const UINT32 spacing = FixedMul(FixedMul(UFO_SPACING, mapobjectscale), K_GetKartGameSpeedScalar(gamespeed)) >> FRACBITS;
	const UINT32 deadzone = FixedMul(FixedMul(UFO_DEADZONE, mapobjectscale), K_GetKartGameSpeedScalar(gamespeed)) >> FRACBITS;

	// Best values of all of the players.
	UINT32 bestDist = UINT32_MAX;
	fixed_t bestSpeed = 0;

	// Desired values for the UFO itself.
	UINT32 wantedDist = UINT32_MAX;
	fixed_t wantedSpeed = ufo_speed(ufo);
	fixed_t speedDelta = 0;

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];
		if (player->spectator == true)
		{
			continue;
		}

		if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			continue;
		}

		if (player->distancetofinish < bestDist)
		{
			bestDist = player->distancetofinish;

			// Doesn't matter if a splitscreen player behind is moving faster behind the one most caught up.
			bestSpeed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);

			bestSpeed = FixedDiv(bestSpeed, mapobjectscale); // Unscale from mapobjectscale to FRACUNIT
			bestSpeed = FixedMul(bestSpeed, UFO_SPEEDFACTOR); // Make it a bit more lenient
		}
	}

	if (bestDist == UINT32_MAX)
	{
		// Invalid, lets go back to base speed.
		wantedSpeed = baseSpeed;
	}
	else
	{
		INT32 distDelta = 0;

		if (bestDist > spacing)
		{
			wantedDist = bestDist - spacing;
		}
		else
		{
			wantedDist = 0;
		}

		distDelta = ufo_distancetofinish(ufo) - wantedDist;

		if (abs(distDelta) <= deadzone)
		{
			// We're in a good spot, try to match the player.
			wantedSpeed = max(bestSpeed >> 1, baseSpeed);
		}
		else
		{
			if (distDelta > 0)
			{
				// Too far behind! Start speeding up!
				wantedSpeed = max(bestSpeed << 1, baseSpeed << 2);
			}
			else
			{
				// Too far ahead! Start slowing down!
				wantedSpeed = baseSpeed;
			}
		}
	}

	// Slowly accelerate or decelerate to
	// get to our desired speed.
	speedDelta = wantedSpeed - ufo_speed(ufo);
	if (speedDelta > 0)
	{
		if (abs(speedDelta) <= UFO_SPEEDUP)
		{
			ufo_speed(ufo) = wantedSpeed;
		}
		else
		{
			ufo_speed(ufo) += UFO_SPEEDUP;
		}
	}
	else if (speedDelta < 0)
	{
		if (abs(speedDelta) <= UFO_SLOWDOWN)
		{
			ufo_speed(ufo) = wantedSpeed;
		}
		else
		{
			ufo_speed(ufo) -= UFO_SLOWDOWN;
		}
	}
}

static void UFOUpdateAngle(mobj_t *ufo)
{
	angle_t dest = K_MomentumAngle(ufo);
	INT32 delta = AngleDeltaSigned(ufo->angle, dest);
	ufo->angle += delta >> 2;
}

static void UFOMove(mobj_t *ufo)
{
	waypoint_t *curWaypoint = NULL;
	waypoint_t *destWaypoint = NULL;

	fixed_t distLeft = INT32_MAX;
	fixed_t newX = ufo->x;
	fixed_t newY = ufo->y;
	fixed_t newZ = ufo->z;

	const boolean useshortcuts = false;
	const boolean huntbackwards = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};
	size_t pathIndex = 0;

	curWaypoint = K_GetWaypointFromIndex((size_t)ufo_waypoint(ufo));
	destWaypoint = K_GetFinishLineWaypoint();

	if (curWaypoint == NULL || destWaypoint == NULL)
	{
		// Waypoints aren't valid.
		// Just stand still.
		ufo->momx = 0;
		ufo->momy = 0;
		ufo->momz = 0;
		return;
	}

	distLeft = FixedMul(ufo_speed(ufo), mapobjectscale);

	while (distLeft > 0)
	{
		fixed_t wpX = curWaypoint->mobj->x;
		fixed_t wpY = curWaypoint->mobj->y;
		fixed_t wpZ = curWaypoint->mobj->z;

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
					break;
				}
			}

			pathIndex++;

			if (pathIndex >= pathtofinish.numnodes)
			{
				// Successfully reached the end of the path.
				break;
			}

			// Now moving to the next waypoint.
			curWaypoint = (waypoint_t *)pathtofinish.array[pathIndex].nodedata;
			ufo_waypoint(ufo) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
		}
	}

	UFOMoveTo(ufo, newX, newY, newZ);

	if (pathfindsuccess == true)
	{
		Z_Free(pathtofinish.array);
	}
}

void Obj_SpecialUFOThinker(mobj_t *ufo)
{
	UFOMove(ufo);
	UFOUpdateAngle(ufo);
	UFOUpdateDistanceToFinish(ufo);
	UFOUpdateSpeed(ufo);
}

static mobj_t *InitSpecialUFO(waypoint_t *start)
{
	mobj_t *ufo = NULL;

	if (start == NULL)
	{
		// Simply create at the origin with default values.
		ufo = P_SpawnMobj(0, 0, 0, MT_SPECIAL_UFO);
		ufo_waypoint(ufo) = -1; // Invalidate
		ufo_distancetofinish(ufo) = INT32_MAX;
	}
	else
	{
		// Create with a proper waypoint track!
		ufo = P_SpawnMobj(start->mobj->x, start->mobj->y, start->mobj->z, MT_SPECIAL_UFO);
		ufo_waypoint(ufo) = (INT32)K_GetWaypointHeapIndex(start);
		UFOUpdateDistanceToFinish(ufo);
	}

	ufo_speed(ufo) = UFO_BASE_SPEED;

	return ufo;
}

mobj_t *Obj_CreateSpecialUFO(void)
{
	waypoint_t *finishWaypoint = K_GetFinishLineWaypoint();
	waypoint_t *startWaypoint = NULL;

	if (finishWaypoint != NULL)
	{
		const boolean huntbackwards = true;
		const boolean useshortcuts = false;
		const UINT32 traveldist = UINT32_MAX; // Go as far back as possible.
		boolean pathfindsuccess = false;
		path_t pathtofinish = {0};

		pathfindsuccess = K_PathfindThruCircuit(
			finishWaypoint, traveldist,
			&pathtofinish,
			useshortcuts, huntbackwards
		);

		if (pathfindsuccess == true)
		{
			startWaypoint = (waypoint_t *)pathtofinish.array[ pathtofinish.numnodes - 1 ].nodedata;
			Z_Free(pathtofinish.array);
		}
	}

	return InitSpecialUFO(startWaypoint);
}
