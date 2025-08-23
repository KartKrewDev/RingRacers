// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sean "Sryder" Ryder
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_waypoint.cpp
/// \brief Waypoint handling from the relevant mobjs
///        Setup and interfacing with waypoints for the main game

#include "k_waypoint.h"

#include "d_netcmd.h"
#include "p_local.h"
#include "p_tick.h"
#include "r_local.h"
#include "z_zone.h"
#include "g_game.h"
#include "p_slopes.h"

#include "cxxutil.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "core/string.h"
#include "core/vector.hpp"

// The number of sparkles per waypoint connection in the waypoint visualisation
static const UINT32 SPARKLES_PER_CONNECTION = 16U;

// Some defaults for the size of the dynamically allocated sets for pathfinding. These are kept for the purpose of
// allocating a size that is less likely to need reallocating again during the pathfinding.
#define OPENSET_BASE_SIZE    (16U)
#define CLOSEDSET_BASE_SIZE  (256U)
#define NODESARRAY_BASE_SIZE (256U)

static waypoint_t *waypointheap  = NULL;
static waypoint_t *firstwaypoint = NULL;
static waypoint_t *finishline    = NULL;
static waypoint_t *startingwaypoint = NULL;

static UINT32 circuitlength = 0U;

#define BASE_TRACK_COMPLEXITY (-5000) // Arbritrary, vibes-based value
static INT32 trackcomplexity = 0;

static size_t numwaypoints       = 0U;
static size_t numwaypointmobjs   = 0U;
static size_t baseopensetsize    = OPENSET_BASE_SIZE;
static size_t baseclosedsetsize  = CLOSEDSET_BASE_SIZE;
static size_t basenodesarraysize = NODESARRAY_BASE_SIZE;


/*--------------------------------------------------
	waypoint_t *K_GetFinishLineWaypoint(void)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetFinishLineWaypoint(void)
{
	return finishline;
}

/*--------------------------------------------------
	waypoint_t *K_GetStartingWaypoint(void)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetStartingWaypoint(void)
{
	return startingwaypoint;
}

/*--------------------------------------------------
	boolean K_GetWaypointIsFinishline(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsFinishline(waypoint_t *waypoint)
{
	boolean waypointisfinishline = false;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsFinishline.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsFinishline.\n");
	}
	else
	{
		waypointisfinishline = (waypoint->mobj->extravalue2 == 1);
	}

	return waypointisfinishline;
}

/*--------------------------------------------------
	boolean K_GetWaypointIsShortcut(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsShortcut(waypoint_t *waypoint)
{
	boolean waypointisshortcut = false;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsShortcut.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsShortcut.\n");
	}
	else
	{
		waypointisshortcut = (waypoint->mobj->lastlook == 1);
	}

	return waypointisshortcut;
}

/*--------------------------------------------------
	boolean K_GetWaypointIsEnabled(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsEnabled(waypoint_t *waypoint)
{
	boolean waypointisenabled = true;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsEnabled.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsEnabled.\n");
	}
	else
	{
		waypointisenabled = (waypoint->mobj->extravalue1 == 1);
	}

	return waypointisenabled;
}

/*--------------------------------------------------
	boolean K_SetWaypointIsEnabled(waypoint_t *waypoint, boolean enabled)

		See header file for description.
--------------------------------------------------*/
void K_SetWaypointIsEnabled(waypoint_t *waypoint, boolean enabled)
{
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_SetWaypointIsEnabled.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_SetWaypointIsEnabled.\n");
	}
	else
	{
		waypoint->mobj->extravalue1 = enabled ? 1 : 0;
	}
}

/*--------------------------------------------------
	boolean K_GetWaypointIsSpawnpoint(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsSpawnpoint(waypoint_t *waypoint)
{
	boolean waypointisspawnpoint = true;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsSpawnpoint.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsSpawnpoint.\n");
	}
	else
	{
		waypointisspawnpoint = (waypoint->mobj->reactiontime == 1);
	}

	return waypointisspawnpoint;
}

/*--------------------------------------------------
	static boolean K_GetWaypointIsOnLine(waypoint_t *const waypoint)

		Checks if a waypoint is exactly on a line. Moving to an exact point
		on a line won't count as crossing it. Moving off of that point does.
		Respawning to a waypoint which is exactly on a line is the easiest
		way to for this to occur.

	Return:-
		Whether the waypoint is exactly on a line.
--------------------------------------------------*/
static boolean K_GetWaypointIsOnLine(waypoint_t *const waypoint)
{
	const fixed_t x = waypoint->mobj->x;
	const fixed_t y = waypoint->mobj->y;

	line_t *line = P_FindNearestLine(x, y,
			waypoint->mobj->subsector->sector, -1);

	vertex_t point;

	if (line != NULL)
	{
		P_ClosestPointOnLine(x, y, line, &point);

		if (x == point.x && y == point.y)
			return true;
	}

	return false;
}

/*--------------------------------------------------
	INT32 K_GetWaypointNextID(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
INT32 K_GetWaypointNextID(waypoint_t *waypoint)
{
	INT32 nextwaypointid = -1;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointNextID.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointNextID.\n");
	}
	else
	{
		nextwaypointid = waypoint->mobj->threshold;
	}

	return nextwaypointid;
}

/*--------------------------------------------------
	INT32 K_GetWaypointID(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
INT32 K_GetWaypointID(waypoint_t *waypoint)
{
	INT32 waypointid = -1;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointID.\n");
	}
	else if ((waypoint->mobj == NULL) || (P_MobjWasRemoved(waypoint->mobj) == true))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointID.\n");
	}
	else
	{
		waypointid = waypoint->mobj->movecount;
	}

	return waypointid;
}

/*--------------------------------------------------
	waypoint_t *K_GetWaypointFromID(INT32 waypointID)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetWaypointFromID(INT32 waypointID)
{
	waypoint_t *waypoint = NULL;
	size_t i = SIZE_MAX;

	for (i = 0; i < numwaypoints; i++)
	{
		waypoint = &waypointheap[i];

		if (K_GetWaypointID(waypoint) == waypointID)
		{
			return waypoint;
		}
	}

	return NULL;
}

/*--------------------------------------------------
	UINT32 K_GetCircuitLength(void)

		See header file for description.
--------------------------------------------------*/
UINT32 K_GetCircuitLength(void)
{
	return circuitlength;
}

/*--------------------------------------------------
	INT32 K_GetTrackComplexity(void)

		See header file for description.
--------------------------------------------------*/
INT32 K_GetTrackComplexity(void)
{
	return trackcomplexity;
}

/*--------------------------------------------------
	waypoint_t *K_GetClosestWaypointToMobj(mobj_t *const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetClosestWaypointToMobj(mobj_t *const mobj)
{
	waypoint_t *closestwaypoint = NULL;

	if ((mobj == NULL) || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_GetClosestWaypointToMobj.\n");
	}
	else
	{
		size_t     i              = 0U;
		waypoint_t *checkwaypoint = NULL;
		fixed_t    closestdist    = INT32_MAX;
		fixed_t    checkdist      = INT32_MAX;

		for (i = 0; i < numwaypoints; i++)
		{
			checkwaypoint = &waypointheap[i];

			checkdist = P_AproxDistance(
				(mobj->x / FRACUNIT) - (checkwaypoint->mobj->x / FRACUNIT),
				(mobj->y / FRACUNIT) - (checkwaypoint->mobj->y / FRACUNIT));
			checkdist = P_AproxDistance(checkdist, (mobj->z / FRACUNIT) - (checkwaypoint->mobj->z / FRACUNIT));

			if (checkdist < closestdist)
			{
				closestwaypoint = checkwaypoint;
				closestdist = checkdist;
			}
		}
	}

	return closestwaypoint;
}

/*--------------------------------------------------
	static void K_CompareOverlappingWaypoint
	(		waypoint_t  *const checkwaypoint,
			waypoint_t **const bestwaypoint,
			fixed_t     *const bestfindist)

		Solves touching overlapping waypoint radiuses by sorting by distance to
		finish line.
--------------------------------------------------*/
static void K_CompareOverlappingWaypoint
(		waypoint_t  *const checkwaypoint,
		waypoint_t **const bestwaypoint,
		fixed_t     *const bestfindist)
{
	const boolean useshortcuts = false;
	const boolean huntbackwards = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};

	if (K_GetWaypointIsShortcut(*bestwaypoint) == false
		&& K_GetWaypointIsShortcut(checkwaypoint) == true)
	{
		// If it's a shortcut, don't use it.
		return;
	}

	pathfindsuccess =
		K_PathfindToWaypoint(checkwaypoint, finishline, &pathtofinish, useshortcuts, huntbackwards);

	if (pathfindsuccess == true)
	{
		if ((INT32)(pathtofinish.totaldist) < *bestfindist)
		{
			*bestwaypoint = checkwaypoint;
			*bestfindist = pathtofinish.totaldist;
		}

		Z_Free(pathtofinish.array);
	}
}

/*--------------------------------------------------
	waypoint_t *K_GetBestWaypointForMobj(mobj_t *const mobj, waypoint_t *const hint)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetBestWaypointForMobj(mobj_t *const mobj, waypoint_t *const hint)
{
	waypoint_t *bestwaypoint = NULL;

	if ((mobj == NULL) || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_GetBestWaypointForMobj.\n");
	}
	else
	{
		fixed_t    closestdist    = INT32_MAX;
		fixed_t    checkdist      = INT32_MAX;
		fixed_t    bestfindist    = INT32_MAX;

		auto sort_waypoint = [&](waypoint_t *const checkwaypoint)
		{
			if (!K_GetWaypointIsEnabled(checkwaypoint))
			{
				return;
			}

			checkdist = P_AproxDistance(
				(mobj->x / FRACUNIT) - (checkwaypoint->mobj->x / FRACUNIT),
				(mobj->y / FRACUNIT) - (checkwaypoint->mobj->y / FRACUNIT));

			UINT8 zMultiplier = 4; // Heavily weight z distance, for the sake of overlapping paths

			if (hint != NULL)
			{
				boolean connectedToHint = (checkwaypoint == hint);

				if (connectedToHint == false && hint->numnextwaypoints > 0)
				{
					for (size_t i = 0U; i < hint->numnextwaypoints; i++)
					{
						if (hint->nextwaypoints[i] == checkwaypoint)
						{
							connectedToHint = true;
							break;
						}
					}
				}

				if (connectedToHint == false && hint->numprevwaypoints > 0)
				{
					for (size_t i = 0U; i < hint->numprevwaypoints; i++)
					{
						if (hint->prevwaypoints[i] == checkwaypoint)
						{
							connectedToHint = true;
							break;
						}
					}
				}

				// Do not consider z height for next/prev waypoints of current waypoint.
				// This helps the current waypoint not be behind you when you're taking a jump.
				if (connectedToHint == true)
				{
					zMultiplier = 0;
				}
			}

			if (zMultiplier > 0)
			{
				checkdist = P_AproxDistance(checkdist, ((mobj->z / FRACUNIT) - (checkwaypoint->mobj->z / FRACUNIT)) * zMultiplier);
			}

			fixed_t rad = (checkwaypoint->mobj->radius / FRACUNIT);

			// remember: huge radius
			if (closestdist <= rad && checkdist <= rad && finishline != NULL)
			{
				if (!P_TraceWaypointTraversal(mobj, checkwaypoint->mobj))
				{
					// Save sight checks when all of the other checks pass, so we only do it if we have to
					return;
				}

				// If the mobj is touching multiple waypoints at once,
				// then solve ties by taking the one closest to the finish line.
				// Prevents position from flickering wildly when taking turns.

				// For the first couple overlapping, check the previous best too.
				if (bestfindist == INT32_MAX)
					K_CompareOverlappingWaypoint(bestwaypoint, &bestwaypoint, &bestfindist);

				K_CompareOverlappingWaypoint(checkwaypoint, &bestwaypoint, &bestfindist);
			}
			else if (checkdist < closestdist && bestfindist == INT32_MAX)
			{
				if (!P_TraceWaypointTraversal(mobj, checkwaypoint->mobj))
				{
					// Save sight checks when all of the other checks pass, so we only do it if we have to
					return;
				}

				bestwaypoint = checkwaypoint;
				closestdist = checkdist;
			}
		};

		if (hint != NULL)
		{
			// The hint is a waypoint that is already known to be close to the player. It is used to exclude
			// most of the other waypoints by distance so fewer expensive sight checks are performed.
			sort_waypoint(hint);
		}

		for (size_t i = 0U; i < numwaypoints; i++)
		{
			sort_waypoint(&waypointheap[i]);
		}
	}

	return bestwaypoint;
}

/*--------------------------------------------------
	size_t K_GetWaypointHeapIndex(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
size_t K_GetWaypointHeapIndex(waypoint_t *waypoint)
{
	size_t waypointindex = SIZE_MAX;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointHeapIndex.\n");
	}
	else
	{
		waypointindex = waypoint - waypointheap;
	}

	return waypointindex;
}

/*--------------------------------------------------
	size_t K_GetNumWaypoints(void)

		See header file for description.
--------------------------------------------------*/
size_t K_GetNumWaypoints(void)
{
	return numwaypoints;
}

/*--------------------------------------------------
	waypoint_t *K_GetWaypointFromIndex(size_t waypointindex)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetWaypointFromIndex(size_t waypointindex)
{
	waypoint_t *waypoint = NULL;

	if (waypointindex >= numwaypoints)
	{
		CONS_Debug(DBG_GAMELOGIC, "waypointindex higher than number of waypoints in K_GetWaypointFromIndex");
	}
	else
	{
		waypoint = &waypointheap[waypointindex];
	}

	return waypoint;
}

/*--------------------------------------------------
	static UINT32 K_DistanceBetweenWaypoints(waypoint_t *const waypoint1, waypoint_t *const waypoint2)

		Gets the Euclidean distance between 2 waypoints by using their mobjs. Used for the heuristic.

	Input Arguments:-
		waypoint1 - The first waypoint
		waypoint2 - The second waypoint

	Return:-
		Euclidean distance between the 2 waypoints
--------------------------------------------------*/
static UINT32 K_DistanceBetweenWaypoints(waypoint_t *const waypoint1, waypoint_t *const waypoint2)
{
	UINT32 finaldist = UINT32_MAX;
	I_Assert(waypoint1 != NULL);
	I_Assert(waypoint2 != NULL);

	{
		const fixed_t xydist =
			P_AproxDistance(waypoint1->mobj->x - waypoint2->mobj->x, waypoint1->mobj->y - waypoint2->mobj->y);
		const fixed_t xyzdist = P_AproxDistance(xydist, waypoint1->mobj->z - waypoint2->mobj->z);
		finaldist = ((UINT32)xyzdist >> FRACBITS);
	}

	return finaldist;
}

/*--------------------------------------------------
	void K_DebugWaypointsSpawnLine(waypoint_t *const waypoint1, waypoint_t *const waypoint2)

		Draw a debugging line between 2 waypoints

	Input Arguments:-
		waypoint1 - A waypoint to draw the line between
		waypoint2 - The other waypoint to draw the line between
--------------------------------------------------*/
static void K_DebugWaypointsSpawnLine(waypoint_t *const waypoint1, waypoint_t *const waypoint2)
{
	mobj_t *waypointmobj1, *waypointmobj2;
	mobj_t *spawnedmobj;
	fixed_t stepx, stepy, stepz;
	fixed_t x, y, z;
	UINT32 waypointdist;
	INT32 n;
	UINT16 linkcolour = SKINCOLOR_GREEN;

	// This array is used to choose which colour should be on this connection
	const UINT16 linkcolours[] = {
		SKINCOLOR_RED,
		SKINCOLOR_BLUE,
		SKINCOLOR_ORANGE,
		SKINCOLOR_PINK,
		SKINCOLOR_DREAM,
		SKINCOLOR_CYAN,
		SKINCOLOR_WHITE,
	};
	const size_t linkcolourssize = sizeof(linkcolours) / sizeof(UINT16);

	// Error conditions
	I_Assert(waypoint1 != NULL);
	I_Assert(waypoint1->mobj != NULL);
	I_Assert(waypoint2 != NULL);
	I_Assert(waypoint2->mobj != NULL);
	I_Assert(cv_kartdebugwaypoints.value != 0);

	linkcolour = linkcolours[K_GetWaypointID(waypoint1) % linkcolourssize];

	if (!K_GetWaypointIsEnabled(waypoint1) || !K_GetWaypointIsEnabled(waypoint2))
	{
		linkcolour = SKINCOLOR_BLACK;
	}

	waypointmobj1 = waypoint1->mobj;
	waypointmobj2 = waypoint2->mobj;

	n = SPARKLES_PER_CONNECTION;

	// For every 2048 fracunits, double the number of sparkles
	waypointdist = K_DistanceBetweenWaypoints(waypoint1, waypoint2);
	n *= (waypointdist / 2048) + 1;

	// Draw the line
	stepx = (waypointmobj2->x - waypointmobj1->x) / n;
	stepy = (waypointmobj2->y - waypointmobj1->y) / n;
	stepz = (waypointmobj2->z - waypointmobj1->z) / n;
	x = waypointmobj1->x;
	y = waypointmobj1->y;
	z = waypointmobj1->z;
	do
	{
		if ((leveltime + n) % 16 <= 4)
		{
			spawnedmobj = P_SpawnMobj(x, y, z, MT_SPARK);
			P_SetMobjState(spawnedmobj, S_THOK);
			spawnedmobj->tics = 1;
			spawnedmobj->frame &= ~FF_TRANSMASK;
			spawnedmobj->frame |= FF_FULLBRIGHT;
			spawnedmobj->color = linkcolour;
			spawnedmobj->scale = FixedMul(spawnedmobj->scale, FixedMul(FRACUNIT/4, FixedDiv((15 - ((leveltime + n) % 16))*FRACUNIT, 15*FRACUNIT)));
		}

		x += stepx;
		y += stepy;
		z += stepz;
	} while (n--);
}

/*--------------------------------------------------
	void K_DebugWaypointDrawRadius(waypoint_t *const waypoint)

		Draw a debugging circle to represent a waypoint's radius

	Input Arguments:-
		waypoint - A waypoint to draw the radius of
--------------------------------------------------*/
static void K_DebugWaypointDrawRadius(waypoint_t *const waypoint)
{
	const fixed_t spriteRadius = 96*FRACUNIT;
	mobj_t *radiusOrb;
	mobj_t *waypointmobj;
	fixed_t spawnX= 0;
	fixed_t spawnY= 0;
	fixed_t spawnZ= 0;

	I_Assert(waypoint != NULL);
	I_Assert(waypoint->mobj != NULL);

	waypointmobj = waypoint->mobj;

	spawnX = waypointmobj->x;
	spawnY = waypointmobj->y;
	spawnZ = waypointmobj->z;

	radiusOrb = P_SpawnMobj(spawnX, spawnY, spawnZ, MT_SPARK);

	P_SetMobjState(radiusOrb, S_WAYPOINTSPLAT);
	radiusOrb->tics = 1;

	radiusOrb->frame &= ~FF_TRANSMASK;
	radiusOrb->frame |= FF_FULLBRIGHT|FF_REVERSESUBTRACT;
	radiusOrb->color = SKINCOLOR_PURPLE;
	radiusOrb->renderflags |= RF_ALWAYSONTOP;

	radiusOrb->destscale = FixedDiv(waypointmobj->radius, spriteRadius);
	P_SetScale(radiusOrb, radiusOrb->destscale);
}

/*--------------------------------------------------
	void K_DebugWaypointsVisualise(void)

		See header file for description.
--------------------------------------------------*/
void K_DebugWaypointsVisualise(void)
{
	mobj_t *waypointmobj;
	mobj_t *debugmobj;
	waypoint_t *waypoint;
	waypoint_t *otherwaypoint;
	UINT32 i;

	if (waypointcap == NULL)
	{
		// No point putting a debug message here when it could easily happen when turning on the cvar in battle
		return;
	}
	if (cv_kartdebugwaypoints.value == 0)
	{
		// Going to nip this in the bud and say no drawing all this without the cvar, it's not particularly optimised
		return;
	}

	// Hunt through the waypointcap so we can show all waypoint mobjs and not just ones that were able to be graphed
	for (waypointmobj = waypointcap; waypointmobj != NULL; waypointmobj = waypointmobj->tracer)
	{
		// If this waypoint is outside of draw distance, don't spawn all the debug crap because it is SLOW
		if (cv_drawdist.value != 0 &&
			R_PointToDist(waypointmobj->x, waypointmobj->y) > cv_drawdist.value * mapobjectscale)
		{
			continue;
		}

		waypoint = K_SearchWaypointHeapForMobj(waypointmobj);

		debugmobj = P_SpawnMobj(waypointmobj->x, waypointmobj->y, waypointmobj->z, MT_SPARK);
		P_SetMobjState(debugmobj, S_WAYPOINTORB);

		debugmobj->frame &= ~FF_TRANSMASK;
		debugmobj->frame |= FF_FULLBRIGHT; //FF_TRANS20
		debugmobj->renderflags |= RF_ALWAYSONTOP;

		// There's a waypoint setup for this mobj! So draw that it's a valid waypoint and draw lines to its connections
		if (waypoint != NULL)
		{
			if (waypoint->numnextwaypoints == 0 && waypoint->numprevwaypoints == 0)
			{
				P_SetMobjState(debugmobj, S_EGOORB);
				debugmobj->color = SKINCOLOR_RED;
				debugmobj->colorized = true;
			}
			else if (waypoint->numnextwaypoints == 0 || waypoint->numprevwaypoints == 0)
			{
				P_SetMobjState(debugmobj, S_EGOORB);
				debugmobj->color = SKINCOLOR_YELLOW;
				debugmobj->colorized = true;
			}
			else if (waypoint == players[displayplayers[0]].nextwaypoint)
			{
				debugmobj->color = SKINCOLOR_GREEN;
				K_DebugWaypointDrawRadius(waypoint);
			}
			else
			{
				debugmobj->color = SKINCOLOR_BLUE;

				if (K_GetWaypointIsShortcut(waypoint))
				{
					debugmobj->color = SKINCOLOR_PINK;
				}
			}

			if (!K_GetWaypointIsEnabled(waypoint))
			{
				debugmobj->color = SKINCOLOR_GREY;
			}

			if (!K_GetWaypointIsSpawnpoint(waypoint))
			{
				debugmobj->frame |= FF_TRANS40;
			}

			// Valid waypoint, so draw lines of SPARKLES to its next or previous waypoints
			if (cv_kartdebugwaypoints.value == 1)
			{
				for (i = 0; i < waypoint->numnextwaypoints; i++)
				{
					if (waypoint->nextwaypoints[i] != NULL)
					{
						otherwaypoint = waypoint->nextwaypoints[i];
						K_DebugWaypointsSpawnLine(waypoint, otherwaypoint);
					}
				}
			}
			else if (cv_kartdebugwaypoints.value == 2)
			{
				for (i = 0; i < waypoint->numprevwaypoints; i++)
				{
					if (waypoint->prevwaypoints[i] != NULL)
					{
						otherwaypoint = waypoint->prevwaypoints[i];
						K_DebugWaypointsSpawnLine(waypoint, otherwaypoint);
					}
				}
			}
		}
		else
		{
			debugmobj->color = SKINCOLOR_RED;
		}
		debugmobj->tics = 1;
	}
}

/*--------------------------------------------------
	static size_t K_GetOpensetBaseSize(void)

		Gets the base size the Openset hinary heap should have

	Input Arguments:-
		None

	Return:-
		The base size the Openset binary heap should have
--------------------------------------------------*/
static size_t K_GetOpensetBaseSize(void)
{
	size_t returnsize = 0;

	returnsize = baseopensetsize;

	return returnsize;
}

/*--------------------------------------------------
	static size_t K_GetClosedsetBaseSize(void)

		Gets the base size the Closedset heap should have

	Input Arguments:-
		None

	Return:-
		The base size the Closedset heap should have
--------------------------------------------------*/
static size_t K_GetClosedsetBaseSize(void)
{
	size_t returnsize = 0;

	returnsize = baseclosedsetsize;

	return returnsize;
}

/*--------------------------------------------------
	static size_t K_GetNodesArrayBaseSize(void)

		Gets the base size the Nodes array should have

	Input Arguments:-
		None

	Return:-
		The base size the Nodes array should have
--------------------------------------------------*/
static size_t K_GetNodesArrayBaseSize(void)
{
	size_t returnsize = 0;

	returnsize = basenodesarraysize;

	return returnsize;
}

/*--------------------------------------------------
	static void K_UpdateOpensetBaseSize(size_t newbaseopensetsize)

		Sets the new base size of the openset binary heap, if it is bigger than before.

	Input Arguments:-
		newbaseopensetsize - The size to try and set the base Openset size to

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateOpensetBaseSize(size_t newbaseopensetsize)
{
	if (newbaseopensetsize > baseopensetsize)
	{
		baseopensetsize = newbaseopensetsize;
	}
}

/*--------------------------------------------------
	static void K_UpdateClosedsetBaseSize(size_t newbaseclosedsetsize)

		Sets the new base size of the closedset heap, if it is bigger than before.

	Input Arguments:-
		newbaseclosedsetsize - The size to try and set the base Closedset size to

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateClosedsetBaseSize(size_t newbaseclosedsetsize)
{
	if (newbaseclosedsetsize > baseopensetsize)
	{
		baseclosedsetsize = newbaseclosedsetsize;
	}
}

/*--------------------------------------------------
	static void K_UpdateNodesArrayBaseSize(size_t newnodesarraysize)

		Sets the new base size of the nodes array, if it is bigger than before.

	Input Arguments:-
		newnodesarraysize - The size to try and set the base nodes array size to

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateNodesArrayBaseSize(size_t newnodesarraysize)
{
	if (newnodesarraysize > basenodesarraysize)
	{
		basenodesarraysize = newnodesarraysize;
	}
}

/*--------------------------------------------------
	static void **K_WaypointPathfindGetNext(void *data, size_t *numconnections)

		Gets the list of next waypoints as the connecting waypoints. For pathfinding only.

	Input Arguments:-
		data           - Should point to a waypoint_t to get nextwaypoints from
		numconnections - Should point to a size_t to return the number of next waypoints

	Return:-
		None
--------------------------------------------------*/
static void **K_WaypointPathfindGetNext(void *data, size_t *numconnections)
{
	waypoint_t **connectingwaypoints = NULL;

	if (data == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetNext received NULL data.\n");
	}
	else if (numconnections == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetNext received NULL numconnections.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		connectingwaypoints = waypoint->nextwaypoints;
		*numconnections = waypoint->numnextwaypoints;
	}

	return (void**)connectingwaypoints;
}

/*--------------------------------------------------
	static void **K_WaypointPathfindGetPrev(void *data, size_t *numconnections)

		Gets the list of previous waypoints as the connecting waypoints. For pathfinding only.

	Input Arguments:-
		data           - Should point to a waypoint_t to get prevwaypoints from
		numconnections - Should point to a size_t to return the number of previous waypoints

	Return:-
		None
--------------------------------------------------*/
static void **K_WaypointPathfindGetPrev(void *data, size_t *numconnections)
{
	waypoint_t **connectingwaypoints = NULL;

	if (data == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetPrev received NULL data.\n");
	}
	else if (numconnections == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetPrev received NULL numconnections.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		connectingwaypoints = waypoint->prevwaypoints;
		*numconnections = waypoint->numprevwaypoints;
	}

	return (void**)connectingwaypoints;
}

/*--------------------------------------------------
	static UINT32 *K_WaypointPathfindGetNextCosts(void* data)

		Gets the list of costs the next waypoints have. For pathfinding only.

	Input Arguments:-
		data - Should point to a waypoint_t to get nextwaypointdistances from

	Return:-
		A pointer to an array of UINT32's describing the cost of going from a waypoint to a next waypoint
--------------------------------------------------*/
static UINT32 *K_WaypointPathfindGetNextCosts(void* data)
{
	UINT32 *connectingnodecosts = NULL;

	if (data == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetNextCosts received NULL data.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		connectingnodecosts = waypoint->nextwaypointdistances;
	}

	return connectingnodecosts;
}

/*--------------------------------------------------
	static UINT32 *K_WaypointPathfindGetPrevCosts(void* data)

		Gets the list of costs the previous waypoints have. For pathfinding only.

	Input Arguments:-
		data - Should point to a waypoint_t to get prevwaypointdistances from

	Return:-
		A pointer to an array of UINT32's describing the cost of going from a waypoint to a previous waypoint
--------------------------------------------------*/
static UINT32 *K_WaypointPathfindGetPrevCosts(void* data)
{
	UINT32 *connectingnodecosts = NULL;

	if (data == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetPrevCosts received NULL data.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		connectingnodecosts = waypoint->prevwaypointdistances;
	}

	return connectingnodecosts;
}

/*--------------------------------------------------
	static UINT32 K_WaypointPathfindGetHeuristic(void *data1, void *data2)

		Gets the heuristic (euclidean distance) between 2 waypoints. For pathfinding only.

	Input Arguments:-
		data1 - Should point to a waypoint_t for the first waypoint
		data2 - Should point to a waypoint_t for the second waypoint

	Return:-
		A UINT32 for the heuristic of the 2 waypoints.
--------------------------------------------------*/
static UINT32 K_WaypointPathfindGetHeuristic(void *data1, void *data2)
{
	UINT32 nodeheuristic = UINT32_MAX;

	if (data1 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetHeuristic received NULL data1.\n");
	}
	else if (data2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindGetHeuristic received NULL data2.\n");
	}
	else
	{
		waypoint_t *waypoint1 = (waypoint_t *)data1;
		waypoint_t *waypoint2 = (waypoint_t *)data2;

		nodeheuristic = K_DistanceBetweenWaypoints(waypoint1, waypoint2);
	}

	return nodeheuristic;
}

/*--------------------------------------------------
	static boolean K_WaypointPathfindTraversableAllEnabled(void *data)

		Checks if a waypoint used as a pathfindnode is traversable. For pathfinding only.
		Variant that accepts shortcut waypoints as traversable.

	Input Arguments:-
		data - Should point to a waypoint_t to check traversability of

	Return:-
		True if the waypoint is traversable, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointPathfindTraversableAllEnabled(void *data, void *prevdata)
{
	boolean traversable = false;

	(void)prevdata;

	if (data == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindTraversableAllEnabled received NULL data.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		traversable = (K_GetWaypointIsEnabled(waypoint) == true);
	}

	return traversable;
}

/*--------------------------------------------------
	static boolean K_WaypointPathfindTraversableNoShortcuts(void *data)

		Checks if a waypoint used as a pathfindnode is traversable. For pathfinding only.
		Variant that does not accept shortcut waypoints as traversable.

	Input Arguments:-
		data - Should point to a waypoint_t to check traversability of

	Return:-
		True if the waypoint is traversable, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointPathfindTraversableNoShortcuts(void *data, void *prevdata)
{
	boolean traversable = false;

	if (data == NULL || prevdata == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindTraversableNoShortcuts received NULL data.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		waypoint_t *prevWaypoint = (waypoint_t *)prevdata;

		traversable = ((K_GetWaypointIsEnabled(waypoint) == true)
			&& (K_GetWaypointIsShortcut(waypoint) == false || K_GetWaypointIsShortcut(prevWaypoint) == true)); // Allow shortcuts to be used if the starting waypoint is already a shortcut.
	}

	return traversable;
}

/*--------------------------------------------------
	static boolean K_WaypointPathfindReachedEnd(void *data, void *setupData)

		Returns if the current waypoint data is our end point of our pathfinding.

	Input Arguments:-
		data - Should point to a pathfindnode_t to compare
		setupData - Should point to the pathfindsetup_t to compare

	Return:-
		True if the waypoint is the pathfind end point, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointPathfindReachedEnd(void *data, void *setupData)
{
	boolean isEnd = false;

	if (data == NULL || setupData == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindReachedEnd received NULL data.\n");
	}
	else
	{
		pathfindnode_t *node = (pathfindnode_t *)data;
		pathfindsetup_t *setup = (pathfindsetup_t *)setupData;

		isEnd = (node->nodedata == setup->endnodedata);
	}

	return isEnd;
}

/*--------------------------------------------------
	static boolean K_WaypointPathfindNextValid(void *data, void *setupData)

		Returns if the current waypoint data has a next waypoint.

	Input Arguments:-
		data - Should point to a pathfindnode_t to compare
		setupData - Should point to the pathfindsetup_t to compare

	Return:-
		True if the waypoint has a next waypoint, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointPathfindNextValid(void *data, void *setupData)
{
	boolean nextValid = false;

	if (data == NULL || setupData == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindNextValid received NULL data.\n");
	}
	else
	{
		pathfindnode_t *node = (pathfindnode_t *)data;
		pathfindsetup_t *setup = (pathfindsetup_t *)setupData;
		waypoint_t *wp = (waypoint_t *)node->nodedata;

		if (setup->getconnectednodes == K_WaypointPathfindGetPrev)
		{
			nextValid = (wp->numprevwaypoints > 0U);
		}
		else
		{
			nextValid = (wp->numnextwaypoints > 0U);
		}
	}

	return nextValid;
}

/*--------------------------------------------------
	static boolean K_WaypointPathfindReachedGScore(void *data, void *setupData)

		Returns if the current waypoint data reaches our end G score.

	Input Arguments:-
		data - Should point to a pathfindnode_t to compare
		setupData - Should point to the pathfindsetup_t to compare

	Return:-
		True if the waypoint reached the G score, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointPathfindReachedGScore(void *data, void *setupData)
{
	boolean scoreReached = false;

	if (data == NULL || setupData == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindReachedGScore received NULL data.\n");
	}
	else
	{
		pathfindnode_t *node = (pathfindnode_t *)data;
		pathfindsetup_t *setup = (pathfindsetup_t *)setupData;
		boolean nextValid = K_WaypointPathfindNextValid(data, setupData);

		scoreReached = (node->gscore >= setup->endgscore) || (nextValid == false);
	}

	return scoreReached;
}

/*--------------------------------------------------
	static boolean K_WaypointPathfindReachedGScoreSpawnable(void *data, void *setupData)

		Returns if the current waypoint data reaches our end G score.

	Input Arguments:-
		data - Should point to a pathfindnode_t to compare
		setupData - Should point to the pathfindsetup_t to compare

	Return:-
		True if the waypoint reached the G score, false otherwise.
--------------------------------------------------*/
static boolean K_WaypointPathfindReachedGScoreSpawnable(void *data, void *setupData)
{
	boolean scoreReached = false;
	boolean spawnable = false;

	if (data == NULL || setupData == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindReachedGScoreSpawnable received NULL data.\n");
	}
	else
	{
		pathfindnode_t *node = (pathfindnode_t *)data;
		pathfindsetup_t *setup = (pathfindsetup_t *)setupData;
		waypoint_t *wp = (waypoint_t *)node->nodedata;
		boolean nextValid = K_WaypointPathfindNextValid(data, setupData);

		scoreReached = (node->gscore >= setup->endgscore) || (nextValid == false);
		spawnable = K_GetWaypointIsSpawnpoint(wp);
	}

	return (scoreReached && spawnable);
}

/*--------------------------------------------------
	boolean K_PathfindToWaypoint(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		See header file for description.
--------------------------------------------------*/
boolean K_PathfindToWaypoint(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	boolean pathfound    = false;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_PathfindToWaypoint.\n");
	}
	else if (destinationwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL destinationwaypoint in K_PathfindToWaypoint.\n");
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_PathfindToWaypoint: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else if (((huntbackwards == false) && (destinationwaypoint->numprevwaypoints == 0))
		|| ((huntbackwards == true) && (destinationwaypoint->numnextwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_PathfindToWaypoint: destinationwaypoint with ID %d has no previous waypoint\n",
			K_GetWaypointID(destinationwaypoint));
	}
	else
	{
		pathfindsetup_t            pathfindsetup   = {0};
		getconnectednodesfunc      nextnodesfunc   = K_WaypointPathfindGetNext;
		getnodeconnectioncostsfunc nodecostsfunc   = K_WaypointPathfindGetNextCosts;
		getnodeheuristicfunc       heuristicfunc   = K_WaypointPathfindGetHeuristic;
		getnodetraversablefunc     traversablefunc = K_WaypointPathfindTraversableNoShortcuts;
		getpathfindfinishedfunc    finishedfunc    = K_WaypointPathfindReachedEnd;

		if (huntbackwards)
		{
			nextnodesfunc = K_WaypointPathfindGetPrev;
			nodecostsfunc = K_WaypointPathfindGetPrevCosts;
		}

		if (useshortcuts)
		{
			traversablefunc = K_WaypointPathfindTraversableAllEnabled;
		}

		pathfindsetup.opensetcapacity    = K_GetOpensetBaseSize();
		pathfindsetup.closedsetcapacity  = K_GetClosedsetBaseSize();
		pathfindsetup.nodesarraycapacity = K_GetNodesArrayBaseSize();
		pathfindsetup.startnodedata      = sourcewaypoint;
		pathfindsetup.endnodedata        = destinationwaypoint;
		pathfindsetup.getconnectednodes  = nextnodesfunc;
		pathfindsetup.getconnectioncosts = nodecostsfunc;
		pathfindsetup.getheuristic       = heuristicfunc;
		pathfindsetup.gettraversable     = traversablefunc;
		pathfindsetup.getfinished        = finishedfunc;

		pathfound = K_PathfindAStar(returnpath, &pathfindsetup);

		K_UpdateOpensetBaseSize(pathfindsetup.opensetcapacity);
		K_UpdateClosedsetBaseSize(pathfindsetup.closedsetcapacity);
		K_UpdateNodesArrayBaseSize(pathfindsetup.nodesarraycapacity);
	}

	return pathfound;
}

/*--------------------------------------------------
	boolean K_PathfindThruCircuit(
		waypoint_t *const sourcewaypoint,
		const UINT32      traveldistance,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		See header file for description.
--------------------------------------------------*/
boolean K_PathfindThruCircuit(
	waypoint_t *const sourcewaypoint,
	const UINT32      traveldistance,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	boolean pathfound = false;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_PathfindThruCircuit.\n");
	}
	else if (finishline == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL finishline in K_PathfindThruCircuit.\n");
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_PathfindThruCircuit: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else
	{
		pathfindsetup_t            pathfindsetup   = {0};
		getconnectednodesfunc      nextnodesfunc   = K_WaypointPathfindGetNext;
		getnodeconnectioncostsfunc nodecostsfunc   = K_WaypointPathfindGetNextCosts;
		getnodeheuristicfunc       heuristicfunc   = K_WaypointPathfindGetHeuristic;
		getnodetraversablefunc     traversablefunc = K_WaypointPathfindTraversableNoShortcuts;
		getpathfindfinishedfunc    finishedfunc    = K_WaypointPathfindReachedGScore;

		if (huntbackwards)
		{
			nextnodesfunc = K_WaypointPathfindGetPrev;
			nodecostsfunc = K_WaypointPathfindGetPrevCosts;
		}

		if (useshortcuts)
		{
			traversablefunc = K_WaypointPathfindTraversableAllEnabled;
		}

		pathfindsetup.opensetcapacity    = K_GetOpensetBaseSize();
		pathfindsetup.closedsetcapacity  = K_GetClosedsetBaseSize();
		pathfindsetup.nodesarraycapacity = K_GetNodesArrayBaseSize();
		pathfindsetup.startnodedata      = sourcewaypoint;
		pathfindsetup.endnodedata        = finishline;
		pathfindsetup.endgscore          = traveldistance;
		pathfindsetup.getconnectednodes  = nextnodesfunc;
		pathfindsetup.getconnectioncosts = nodecostsfunc;
		pathfindsetup.getheuristic       = heuristicfunc;
		pathfindsetup.gettraversable     = traversablefunc;
		pathfindsetup.getfinished        = finishedfunc;

		pathfound = K_PathfindAStar(returnpath, &pathfindsetup);

		K_UpdateOpensetBaseSize(pathfindsetup.opensetcapacity);
		K_UpdateClosedsetBaseSize(pathfindsetup.closedsetcapacity);
		K_UpdateNodesArrayBaseSize(pathfindsetup.nodesarraycapacity);
	}

	return pathfound;
}

/*--------------------------------------------------
	boolean K_PathfindThruCircuitSpawnable(
		waypoint_t *const sourcewaypoint,
		const UINT32      traveldistance,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		See header file for description.
--------------------------------------------------*/
boolean K_PathfindThruCircuitSpawnable(
	waypoint_t *const sourcewaypoint,
	const UINT32      traveldistance,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	boolean pathfound = false;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_PathfindThruCircuitSpawnable.\n");
	}
	else if (finishline == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL finishline in K_PathfindThruCircuitSpawnable.\n");
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_PathfindThruCircuitSpawnable: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else
	{
		pathfindsetup_t            pathfindsetup   = {0};
		getconnectednodesfunc      nextnodesfunc   = K_WaypointPathfindGetNext;
		getnodeconnectioncostsfunc nodecostsfunc   = K_WaypointPathfindGetNextCosts;
		getnodeheuristicfunc       heuristicfunc   = K_WaypointPathfindGetHeuristic;
		getnodetraversablefunc     traversablefunc = K_WaypointPathfindTraversableNoShortcuts;
		getpathfindfinishedfunc    finishedfunc    = K_WaypointPathfindReachedGScoreSpawnable;

		if (huntbackwards)
		{
			nextnodesfunc = K_WaypointPathfindGetPrev;
			nodecostsfunc = K_WaypointPathfindGetPrevCosts;
		}

		if (useshortcuts)
		{
			traversablefunc = K_WaypointPathfindTraversableAllEnabled;
		}

		pathfindsetup.opensetcapacity    = K_GetOpensetBaseSize();
		pathfindsetup.closedsetcapacity  = K_GetClosedsetBaseSize();
		pathfindsetup.nodesarraycapacity = K_GetNodesArrayBaseSize();
		pathfindsetup.startnodedata      = sourcewaypoint;
		pathfindsetup.endnodedata        = finishline;
		pathfindsetup.endgscore          = traveldistance;
		pathfindsetup.getconnectednodes  = nextnodesfunc;
		pathfindsetup.getconnectioncosts = nodecostsfunc;
		pathfindsetup.getheuristic       = heuristicfunc;
		pathfindsetup.gettraversable     = traversablefunc;
		pathfindsetup.getfinished        = finishedfunc;

		pathfound = K_PathfindAStar(returnpath, &pathfindsetup);

		K_UpdateOpensetBaseSize(pathfindsetup.opensetcapacity);
		K_UpdateClosedsetBaseSize(pathfindsetup.closedsetcapacity);
		K_UpdateNodesArrayBaseSize(pathfindsetup.nodesarraycapacity);
	}

	return pathfound;
}

/*--------------------------------------------------
	waypoint_t *K_GetNextWaypointToDestination(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_GetNextWaypointToDestination(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	const boolean     useshortcuts,
	const boolean     huntbackwards)
{
	waypoint_t *nextwaypoint = NULL;

	if (sourcewaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL sourcewaypoint in K_GetNextWaypointToDestination.\n");
	}
	else if (destinationwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL destinationwaypoint in K_GetNextWaypointToDestination.\n");
	}
	else if (sourcewaypoint == destinationwaypoint)
	{
		// Source and destination waypoint are the same, we're already there
		nextwaypoint = destinationwaypoint;
	}
	else if (((huntbackwards == false) && (sourcewaypoint->numnextwaypoints == 0))
		|| ((huntbackwards == true) && (sourcewaypoint->numprevwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_GetNextWaypointToDestination: sourcewaypoint with ID %d has no next waypoint\n",
			K_GetWaypointID(sourcewaypoint));
	}
	else if (((huntbackwards == false) && (destinationwaypoint->numprevwaypoints == 0))
		|| ((huntbackwards == true) && (destinationwaypoint->numnextwaypoints == 0)))
	{
		CONS_Debug(DBG_GAMELOGIC,
			"K_GetNextWaypointToDestination: destinationwaypoint with ID %d has no previous waypoint\n",
			K_GetWaypointID(destinationwaypoint));
	}
	else
	{
		// If there is only 1 next waypoint it doesn't matter if it's a shortcut
		if ((huntbackwards == false) && sourcewaypoint->numnextwaypoints == 1)
		{
			nextwaypoint = sourcewaypoint->nextwaypoints[0];
		}
		else if ((huntbackwards == true) && sourcewaypoint->numprevwaypoints == 1)
		{
			nextwaypoint = sourcewaypoint->prevwaypoints[0];
		}
		else
		{
			path_t                     pathtowaypoint  = {0};
			pathfindsetup_t            pathfindsetup   = {0};
			boolean                    pathfindsuccess = false;
			getconnectednodesfunc      nextnodesfunc   = K_WaypointPathfindGetNext;
			getnodeconnectioncostsfunc nodecostsfunc   = K_WaypointPathfindGetNextCosts;
			getnodeheuristicfunc       heuristicfunc   = K_WaypointPathfindGetHeuristic;
			getnodetraversablefunc     traversablefunc = K_WaypointPathfindTraversableNoShortcuts;
			getpathfindfinishedfunc    finishedfunc    = K_WaypointPathfindReachedEnd;

			if (huntbackwards)
			{
				nextnodesfunc = K_WaypointPathfindGetPrev;
				nodecostsfunc = K_WaypointPathfindGetPrevCosts;
			}

			if (useshortcuts)
			{
				traversablefunc = K_WaypointPathfindTraversableAllEnabled;
			}

			pathfindsetup.opensetcapacity    = K_GetOpensetBaseSize();
			pathfindsetup.closedsetcapacity  = K_GetClosedsetBaseSize();
			pathfindsetup.nodesarraycapacity = K_GetNodesArrayBaseSize();
			pathfindsetup.startnodedata      = sourcewaypoint;
			pathfindsetup.endnodedata        = destinationwaypoint;
			pathfindsetup.getconnectednodes  = nextnodesfunc;
			pathfindsetup.getconnectioncosts = nodecostsfunc;
			pathfindsetup.getheuristic       = heuristicfunc;
			pathfindsetup.gettraversable     = traversablefunc;
			pathfindsetup.getfinished        = finishedfunc;

			pathfindsuccess = K_PathfindAStar(&pathtowaypoint, &pathfindsetup);

			K_UpdateOpensetBaseSize(pathfindsetup.opensetcapacity);
			K_UpdateClosedsetBaseSize(pathfindsetup.closedsetcapacity);
			K_UpdateNodesArrayBaseSize(pathfindsetup.nodesarraycapacity);

			if (pathfindsuccess)
			{
				// A direct path to the destination has been found.
				if (pathtowaypoint.numnodes > 1)
				{
					nextwaypoint = (waypoint_t*)pathtowaypoint.array[1].nodedata;
				}
				else
				{
					// Shouldn't happen, as this is the source waypoint.
					CONS_Debug(DBG_GAMELOGIC, "Only one waypoint pathfound in K_GetNextWaypointToDestination.\n");
					nextwaypoint = (waypoint_t*)pathtowaypoint.array[0].nodedata;
				}

				Z_Free(pathtowaypoint.array);
			}
			else
			{
				size_t     i                   = 0U;
				waypoint_t **nextwaypointlist  = NULL;
				size_t     numnextwaypoints    = 0U;
				boolean waypointisenabled      = true;
				boolean waypointisshortcut     = false;

				if (huntbackwards)
				{
					nextwaypointlist = sourcewaypoint->prevwaypoints;
					numnextwaypoints = sourcewaypoint->numprevwaypoints;
				}
				else
				{
					nextwaypointlist = sourcewaypoint->nextwaypoints;
					numnextwaypoints = sourcewaypoint->numnextwaypoints;
				}

				// No direct path to the destination has been found, choose a next waypoint from what is available
				// 1. If shortcuts are allowed, pick the first shortcut path that is enabled
				// 2. If shortcuts aren't allowed, or there are no shortcuts, pick the first enabled waypoint
				// 3. If there's no waypoints enabled, then nothing can be done and there is no next waypoint
				if (useshortcuts)
				{
					for (i = 0U; i < numnextwaypoints; i++)
					{
						waypointisenabled  = K_GetWaypointIsEnabled(nextwaypointlist[i]);
						waypointisshortcut = K_GetWaypointIsShortcut(nextwaypointlist[i]);

						if (waypointisenabled && waypointisshortcut)
						{
							nextwaypoint = nextwaypointlist[i];
							break;
						}
					}
				}

				if (nextwaypoint == NULL)
				{
					for (i = 0U; i < numnextwaypoints; i++)
					{
						waypointisenabled  = K_GetWaypointIsEnabled(nextwaypointlist[i]);

						if (waypointisenabled)
						{
							nextwaypoint = nextwaypointlist[i];
							break;
						}
					}
				}
			}
		}
	}

	return nextwaypoint;
}

/*--------------------------------------------------
	boolean K_CheckWaypointForMobj(waypoint_t *const waypoint, void *const mobjpointer)

		Compares a waypoint's mobj and a void pointer that *should* point to an mobj. Intended for use with the
		K_SearchWaypoint functions ONLY. No, it is not my responsibility to make sure the pointer you sent in is
		actually an mobj.

	Input Arguments:-
		waypoint    - The waypoint that is currently being compared against
		mobjpointer - A pointer that should be to an mobj to check with the waypoint for matching

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static boolean K_CheckWaypointForMobj(waypoint_t *const waypoint, void *const mobjpointer)
{
	boolean mobjsmatch = false;

	// Error Conditions
	I_Assert(waypoint != NULL);
	I_Assert(waypoint->mobj != NULL);
	I_Assert(mobjpointer != NULL);

	{
		mobj_t *const mobj = (mobj_t *)mobjpointer;

		if (P_MobjWasRemoved(mobj))
		{
			CONS_Debug(DBG_GAMELOGIC, "Mobj Was Removed in K_CheckWaypointForMobj");
		}
		else if (mobj->type != MT_WAYPOINT)
		{
			CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_CheckWaypointForMobj. Type=%d.\n", mobj->type);
		}
		else
		{
			// All that error checking for 3 lines :^)
			if (waypoint->mobj == mobj)
			{
				mobjsmatch = true;
			}
		}
	}

	return mobjsmatch;
}

/*--------------------------------------------------
	waypoint_t *K_TraverseWaypoints(
		waypoint_t *waypoint,
		boolean    (*conditionalfunc)(waypoint_t *const, void *const),
		void       *const condition,
		boolean    *const visitedarray)

		Searches through the waypoint list for a waypoint that matches a condition, just does a simple flood search
		of the graph with no pathfinding

	Input Arguments:-
		waypoint        - The waypoint that is currently being checked, goes through nextwaypoints after this one
		conditionalfunc - The function that will be used to check a waypoint against condition
		condition       - the condition being checked by conditionalfunc
		visitedarray    - An array of booleans that let us know if a waypoint has already been checked, marked to true
			when one is, so we don't repeat going down a path. Cannot be changed to a different pointer

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_TraverseWaypoints(
	waypoint_t *waypoint,
	boolean    (*conditionalfunc)(waypoint_t *const, void *const),
	void       *const condition,
	boolean    *const visitedarray)
{
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	I_Assert(condition != NULL);
	I_Assert(conditionalfunc != NULL);
	I_Assert(visitedarray != NULL);

searchwaypointstart:
	I_Assert(waypoint != NULL);

	{
		size_t waypointindex = K_GetWaypointHeapIndex(waypoint);
		// If we've already visited this waypoint, we've already checked the next waypoints, no point continuing
		if ((waypointindex != SIZE_MAX) && (visitedarray[waypointindex] != true))
		{
			// Mark this waypoint as being visited
			visitedarray[waypointindex] = true;

			if (conditionalfunc(waypoint, condition) == true)
			{
				foundwaypoint = waypoint;
			}
			else
			{
				// If this waypoint only has one next waypoint, set the waypoint to be the next one and jump back
				// to the start, this is to avoid going too deep into the stack where we can
				// Yes this is a horrible horrible goto, but the alternative is a do while loop with an extra
				// variable, which is slightly more confusing. This is probably the fastest and least confusing
				// option that keeps this functionality
				if (waypoint->numnextwaypoints == 1 && waypoint->nextwaypoints[0] != NULL)
				{
					waypoint = waypoint->nextwaypoints[0];
					goto searchwaypointstart;
				}
				else if (waypoint->numnextwaypoints != 0)
				{
					// The nesting here is a bit nasty, but it's better than potentially a lot of function calls on
					// the stack, and another function would be very small in this case
					UINT32 i;
					// For each next waypoint, Search through it's path continuation until we hopefully find the one
					// we're looking for
					for (i = 0; i < waypoint->numnextwaypoints; i++)
					{
						if (waypoint->nextwaypoints[i] != NULL)
						{
							foundwaypoint = K_TraverseWaypoints(waypoint->nextwaypoints[i], conditionalfunc,
								condition, visitedarray);

							if (foundwaypoint != NULL)
							{
								break;
							}
						}
					}
				}
				else
				{
					// No next waypoints, this function will be returned from
				}
			}
		}
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointGraph(
		boolean (*conditionalfunc)(waypoint_t *const, void *const),
		void    *const condition)

		Searches through the waypoint graph for a waypoint that matches the conditional

	Input Arguments:-
		conditionalfunc - The function that will be used to check a waypoint against condition
		condition       - the condition being checked by conditionalfunc

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_SearchWaypointGraph(
	boolean (*conditionalfunc)(waypoint_t *const, void *const),
	void    *const condition)
{
	boolean *visitedarray = NULL;
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	I_Assert(condition != NULL);
	I_Assert(conditionalfunc != NULL);
	I_Assert(firstwaypoint != NULL);

	visitedarray = static_cast<boolean*>(Z_Calloc(numwaypoints * sizeof(boolean), PU_STATIC, NULL));
	foundwaypoint = K_TraverseWaypoints(firstwaypoint, conditionalfunc, condition, visitedarray);
	Z_Free(visitedarray);

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointGraphForMobj(mobj_t * const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_SearchWaypointGraphForMobj(mobj_t *const mobj)
{
	waypoint_t *foundwaypoint = NULL;

	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypointGraphForMobj.\n");
	}
	else if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypointGraphForMobj. Type=%d.\n", mobj->type);
	}
	else
	{
		foundwaypoint = K_SearchWaypointGraph(K_CheckWaypointForMobj, (void *)mobj);
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointHeap(
		boolean (*conditionalfunc)(waypoint_t *const, void *const),
		void    *const condition)

		Searches through the waypoint heap for a waypoint that matches the conditional

	Input Arguments:-
		conditionalfunc - The function that will be used to check a waypoint against condition
		condition       - the condition being checked by conditionalfunc

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_SearchWaypointHeap(
	boolean (*conditionalfunc)(waypoint_t *const, void *const),
	void    *const condition)
{
	UINT32 i = 0;
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	I_Assert(condition != NULL);
	I_Assert(conditionalfunc != NULL);
	I_Assert(waypointheap != NULL);

	// Simply search through the waypointheap for the waypoint which matches the condition. Much simpler when no
	// pathfinding is needed. Search up to numwaypoints and NOT numwaypointmobjs as numwaypoints is the real number of
	// waypoints setup in the heap while numwaypointmobjs ends up being the capacity
	for (i = 0; i < numwaypoints; i++)
	{
		if (conditionalfunc(&waypointheap[i], condition) == true)
		{
			foundwaypoint = &waypointheap[i];
			break;
		}
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointHeapForMobj(mobj_t *const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_SearchWaypointHeapForMobj(mobj_t *const mobj)
{
	waypoint_t *foundwaypoint = NULL;

	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypointHeapForMobj.\n");
	}
	else if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypointHeapForMobj. Type=%d.\n", mobj->type);
	}
	else
	{
		foundwaypoint = K_SearchWaypointHeap(K_CheckWaypointForMobj, (void *)mobj);
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	static UINT32 K_SetupCircuitLength(void)

		Sets up the Circuit Length by getting the best path from the finishwaypoint back to itself.
		On sprint maps, circuitlength is 0.

	Input Arguments:-
		None

	Return:-
		Length of the circuit
--------------------------------------------------*/
static UINT32 K_SetupCircuitLength(void)
{
	I_Assert(firstwaypoint != NULL);
	I_Assert(numwaypoints > 0U);
	I_Assert(finishline != NULL);

	// The circuit length only makes sense in circuit maps, sprint maps do not need to use it
	// The main usage of the circuit length is to add onto a player's distance to finish line so crossing the finish
	// line places people correctly relative to each other
	if ((mapheaderinfo[gamemap - 1]->levelflags & LF_SECTIONRACE) == LF_SECTIONRACE)
	{
		path_t bestsprintpath = {0};
		auto sprint_finally = srb2::finally([&bestsprintpath]() { Z_Free(bestsprintpath.array); });

		const boolean useshortcuts = false;
		const boolean huntbackwards = true;
		const UINT32 traveldist = UINT32_MAX - UINT16_MAX; // Go as far back as possible. Not exactly UINT32_MAX to avoid possible overflow.

		boolean pathfindsuccess = K_PathfindThruCircuit(
			finishline, traveldist,
			&bestsprintpath,
			useshortcuts, huntbackwards
		);

		circuitlength = bestsprintpath.totaldist;

		if (pathfindsuccess == true)
		{
			startingwaypoint = (waypoint_t *)bestsprintpath.array[ bestsprintpath.numnodes - 1 ].nodedata;
		}
	}
	else
	{
		// Create a fake finishline waypoint, then try and pathfind to the finishline from it
		waypoint_t    fakefinishline  = *finishline;

		path_t        bestcircuitpath = {0};
		auto circuit_finally = srb2::finally([&bestcircuitpath]() { Z_Free(bestcircuitpath.array); });

		const boolean useshortcuts    = false;
		const boolean huntbackwards   = false;

		K_PathfindToWaypoint(&fakefinishline, finishline, &bestcircuitpath, useshortcuts, huntbackwards);

		circuitlength = bestcircuitpath.totaldist;

		if (finishline->numnextwaypoints > 0)
		{
			// TODO: Implementing a version of the fakefinishline hack for
			// this instead would be the most ideal
			startingwaypoint = finishline->nextwaypoints[0];
		}
	}

	return circuitlength;
}

/*--------------------------------------------------
	static void K_AddPrevToWaypoint(waypoint_t *const waypoint, waypoint_t *const prevwaypoint)

		Adds another waypoint to a waypoint's previous waypoint list, this needs to be done like this because there is no
		way to identify previous waypoints from just IDs, so we need to reallocate the memory for every previous waypoint

	Input Arguments:-
		waypoint     - The waypoint which is having its previous waypoint list added to
		prevwaypoint - The waypoint which is being added to the previous waypoint list

	Return:-
		Pointer to waypoint_t for the rest of the waypoint data to be placed into
--------------------------------------------------*/
static void K_AddPrevToWaypoint(waypoint_t *const waypoint, waypoint_t *const prevwaypoint)
{
	// Error conditions
	I_Assert(waypoint != NULL);
	I_Assert(prevwaypoint != NULL);

	waypoint->numprevwaypoints++;
	waypoint->prevwaypoints = static_cast<waypoint_t**>(
		Z_Realloc(waypoint->prevwaypoints, waypoint->numprevwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL)
	);

	if (!waypoint->prevwaypoints)
	{
		I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoints.");
	}

	waypoint->prevwaypointdistances = static_cast<UINT32*>(
		Z_Realloc(waypoint->prevwaypointdistances, waypoint->numprevwaypoints * sizeof(fixed_t), PU_LEVEL, NULL)
	);

	if (!waypoint->prevwaypointdistances)
	{
		I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoint distances.");
	}

	waypoint->prevwaypoints[waypoint->numprevwaypoints - 1] = prevwaypoint;
}

/*--------------------------------------------------
	static waypoint_t *K_MakeWaypoint(mobj_t *const mobj)

		Make a new waypoint from a map object. Setups up most of the data for it, and allocates most memory
		Remaining creation is handled in K_SetupWaypoint

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_MakeWaypoint(mobj_t *const mobj)
{
	waypoint_t *madewaypoint = NULL;
	mobj_t *otherwaypointmobj = NULL;

	// Error conditions
	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));
	I_Assert(waypointcap != NULL); // No waypoint mobjs in map load
	I_Assert(numwaypoints < numwaypointmobjs); // waypoint array reached max capacity

	madewaypoint = &waypointheap[numwaypoints];
	numwaypoints++;

	madewaypoint->mobj = NULL;
	P_SetTarget(&madewaypoint->mobj, mobj);

	// Don't allow a waypoint that has its next ID set to itself to work
	if (mobj->threshold != mobj->movecount) {
		// Go through the other waypoint mobjs in the map to find out how many waypoints are after this one
		for (otherwaypointmobj = waypointcap; otherwaypointmobj != NULL; otherwaypointmobj = otherwaypointmobj->tracer)
		{
			// threshold = next waypoint id, movecount = my id
			if (mobj->threshold == otherwaypointmobj->movecount)
			{
				madewaypoint->numnextwaypoints++;
			}
		}
	}

	// No next waypoints
	if (madewaypoint->numnextwaypoints != 0)
	{
		// Allocate memory to hold enough pointers to all of the next waypoints
		madewaypoint->nextwaypoints = static_cast<waypoint_t**>(
			Z_Calloc(madewaypoint->numnextwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL)
		);
		if (madewaypoint->nextwaypoints == NULL)
		{
			I_Error("K_MakeWaypoint: Out of Memory allocating next waypoints.");
		}
		madewaypoint->nextwaypointdistances = static_cast<UINT32*>(
			Z_Calloc(madewaypoint->numnextwaypoints * sizeof(fixed_t), PU_LEVEL, NULL)
		);
		if (madewaypoint->nextwaypointdistances == NULL)
		{
			I_Error("K_MakeWaypoint: Out of Memory allocating next waypoint distances.");
		}
	}

	return madewaypoint;
}

/*--------------------------------------------------
	static waypoint_t *K_SetupWaypoint(mobj_t *const mobj)

		Either gets an already made waypoint, or sets up a new waypoint for an mobj,
		including next and previous waypoints

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_SetupWaypoint(mobj_t *const mobj)
{
	waypoint_t *thiswaypoint = NULL;

	// Error conditions
	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));
	I_Assert(mobj->type == MT_WAYPOINT);
	I_Assert(waypointcap != NULL); // no waypoint mobjs in map load

	// If we have waypoints already created, search through them first to see if this mobj is already added.
	if (firstwaypoint != NULL)
	{
		thiswaypoint = K_SearchWaypointHeapForMobj(mobj);
	}

	// The waypoint hasn't already been made, so make it
	if (thiswaypoint == NULL)
	{
		mobj_t *otherwaypointmobj = NULL;
		UINT32 nextwaypointindex = 0;

		thiswaypoint = K_MakeWaypoint(mobj);

		if (thiswaypoint != NULL)
		{
			// Set the first waypoint if it isn't already
			if (firstwaypoint == NULL)
			{
				firstwaypoint = thiswaypoint;
			}

			if (K_GetWaypointIsFinishline(thiswaypoint))
			{
				if (finishline != NULL)
				{
					const INT32 oldfinishlineid = K_GetWaypointID(finishline);
					const INT32 thiswaypointid  = K_GetWaypointID(thiswaypoint);
					CONS_Alert(
						CONS_WARNING, "Multiple finish line waypoints with IDs %d and %d! Using %d.",
						oldfinishlineid, thiswaypointid, thiswaypointid);
				}
				finishline = thiswaypoint;
			}

			/* only relevant for respawning */
			if (K_GetWaypointIsSpawnpoint(thiswaypoint))
			{
				thiswaypoint->onaline = K_GetWaypointIsOnLine(thiswaypoint);
			}

			if (thiswaypoint->numnextwaypoints > 0)
			{
				waypoint_t *nextwaypoint = NULL;
				fixed_t nextwaypointdistance = 0;
				// Go through the waypoint mobjs to setup the next waypoints and make this waypoint know they're its
				// next. I kept this out of K_MakeWaypoint so the stack isn't gone down as deep
				for (otherwaypointmobj = waypointcap;
					otherwaypointmobj != NULL;
					otherwaypointmobj = otherwaypointmobj->tracer)
				{
					// threshold = next waypoint id, movecount = my id
					if (mobj->threshold == otherwaypointmobj->movecount)
					{
						nextwaypoint = K_SetupWaypoint(otherwaypointmobj);
						nextwaypointdistance = K_DistanceBetweenWaypoints(thiswaypoint, nextwaypoint);
						thiswaypoint->nextwaypoints[nextwaypointindex] = nextwaypoint;
						thiswaypoint->nextwaypointdistances[nextwaypointindex] = nextwaypointdistance;
						K_AddPrevToWaypoint(nextwaypoint, thiswaypoint);
						nextwaypoint->prevwaypointdistances[nextwaypoint->numprevwaypoints - 1] = nextwaypointdistance;
						nextwaypointindex++;
					}
					if (nextwaypointindex >= thiswaypoint->numnextwaypoints)
					{
						break;
					}
				}
			}
			else
			{
				CONS_Debug(DBG_SETUP, "Waypoint with ID %d has no next waypoint.\n", K_GetWaypointID(thiswaypoint));
			}
		}
		else
		{
			CONS_Debug(DBG_SETUP, "K_SetupWaypoint failed to make new waypoint with ID %d.\n", mobj->movecount);
		}
	}

	return thiswaypoint;
}

/*--------------------------------------------------
	static boolean K_AllocateWaypointHeap(void)

		Allocates the waypoint heap enough space for the number of waypoint mobjs on the map

	Return:-
		True if the allocation was successful, false if it wasn't. Will I_Error if out of memory still.
--------------------------------------------------*/
static boolean K_AllocateWaypointHeap(void)
{
	mobj_t *waypointmobj = NULL;
	boolean allocationsuccessful = false;

	// Error conditions
	I_Assert(waypointheap == NULL); // waypointheap is already allocated
	I_Assert(waypointcap != NULL); // no waypoint mobjs at map load

	// This should be an allocation for the first time. Reset the number of mobjs back to 0 if it's not already
	numwaypointmobjs = 0;

	// Find how many waypoint mobjs there are in the map, this is the maximum number of waypoints there CAN be
	for (waypointmobj = waypointcap; waypointmobj != NULL; waypointmobj = waypointmobj->tracer)
	{
		if (waypointmobj->type != MT_WAYPOINT)
		{
			CONS_Debug(DBG_SETUP,
				"Non MT_WAYPOINT mobj in waypointcap in K_AllocateWaypointHeap. Type=%d\n.", waypointmobj->type);
			continue;
		}

		numwaypointmobjs++;
	}

	if (numwaypointmobjs > 0)
	{
		// Allocate space in the heap for every mobj, it's possible some mobjs aren't linked up and not all of the
		// heap allocated will be used, but it's a fairly reasonable assumption that this isn't going to be awful
		waypointheap = static_cast<waypoint_t*>(Z_Calloc(numwaypointmobjs * sizeof(waypoint_t), PU_LEVEL, NULL));

		if (waypointheap == NULL)
		{
			// We could theoretically CONS_Debug here and continue without using waypoints, but I feel that will
			// require error checks that will end up spamming the console when we think waypoints SHOULD be working.
			// Safer to just exit if out of memory
			I_Error("K_AllocateWaypointHeap: Out of memory.");
		}
		allocationsuccessful = true;
	}
	else
	{
		CONS_Debug(DBG_SETUP, "No waypoint mobjs in waypointcap.\n");
	}

	return allocationsuccessful;
}

/*--------------------------------------------------
	void K_FreeWaypoints(void)

		For safety, this will free the waypointheap and all the waypoints allocated if they aren't already before they
		are setup. If the PU_LEVEL tag is cleared, make sure to call K_ClearWaypoints or this will try to free already
		freed memory!
--------------------------------------------------*/
static void K_FreeWaypoints(void)
{
	if (waypointheap != NULL)
	{
		// Free the waypointheap
		Z_Free(waypointheap);
	}

	K_ClearWaypoints();
}

namespace
{

/*--------------------------------------------------
	BlockItReturn_t K_TrackWaypointNearOffroad(line_t *line)

		Blockmap iteration function to check in an extra radius
		around a waypoint to find any solid walls around it.
--------------------------------------------------*/
static fixed_t g_track_wp_x = INT32_MAX;
static fixed_t g_track_wp_y = INT32_MAX;
static fixed_t g_track_wp_radius = INT32_MAX;

static BlockItReturn_t K_TrackWaypointNearOffroad(line_t *line)
{
	fixed_t dist = INT32_MAX;
	vertex_t v = {0};

	P_ClosestPointOnLine(
		g_track_wp_x, g_track_wp_y,
		line,
		&v
	);

	dist = R_PointToDist2(
		g_track_wp_x, g_track_wp_y,
		v.x, v.y
	);

	const fixed_t buffer = FixedMul(mobjinfo[MT_PLAYER].radius * 2, mapobjectscale) * 3;
	dist -= buffer;

	if (dist <= 0) // line gets crossed
	{
		if ((line->flags & (ML_TWOSIDED|ML_IMPASSABLE|ML_BLOCKPLAYERS|ML_MIDSOLID)) == ML_TWOSIDED)
		{
			// double-sided, and no blocking flags -- it's not a wall
			const INT32 side = P_PointOnLineSide(g_track_wp_x, g_track_wp_y, line);
			const sector_t *sec = side ? line->frontsector : line->backsector;

			if (sec != nullptr && (sec->damagetype == SD_DEATHPIT || sec->damagetype == SD_INSTAKILL))
			{
				// force kill sectors to be more complex
				return BMIT_STOP;
			}
		}
		else
		{
			// actually is a wall
			return BMIT_ABORT;
		}
	}

	// not crossed, or not a wall
	return BMIT_CONTINUE;
}

/*--------------------------------------------------
	boolean K_SneakerPanelOverlap(struct sneakerpanel &panelA, struct sneakerpanel &panelB)

		Returns whenever or not a sneaker panel sector / thing overlap
--------------------------------------------------*/
struct complexity_sneaker_s
{
	fixed_t bbox[4];
	//srb2::Vector<sector_t *> sectors;
	//srb2::Vector<mapthing_t *> things;

	complexity_sneaker_s(sector_t *sec)
	{
		M_ClearBox(bbox);

		for (size_t i = 0; i < sec->linecount; i++)
		{
			line_t *const ld = sec->lines[i];

			M_AddToBox(bbox, ld->bbox[BOXRIGHT], ld->bbox[BOXTOP]);
			M_AddToBox(bbox, ld->bbox[BOXLEFT], ld->bbox[BOXBOTTOM]);
		}
	}

	complexity_sneaker_s(mapthing_t *mt)
	{
		M_ClearBox(bbox);

		fixed_t x = mt->x << FRACBITS;
		fixed_t y = mt->y << FRACBITS;
		fixed_t radius = FixedMul(FixedMul(mobjinfo[MT_SNEAKERPANEL].radius, mt->scale), mapobjectscale);

		M_AddToBox(bbox, x - radius, y - radius);
		M_AddToBox(bbox, x + radius, y + radius);
	}
};

static boolean K_SneakerPanelOverlap(complexity_sneaker_s &panelA, complexity_sneaker_s &panelB)
{
	const fixed_t overlap_extra = 528 * mapobjectscale; // merge ones this close together

	const fixed_t a_width_half = (panelA.bbox[BOXRIGHT] - panelA.bbox[BOXLEFT]) / 2;
	const fixed_t a_height_half = (panelA.bbox[BOXTOP] - panelA.bbox[BOXBOTTOM]) / 2;
	const fixed_t a_x = panelA.bbox[BOXLEFT] + a_width_half;
	const fixed_t a_y = panelA.bbox[BOXBOTTOM] + a_height_half;

	const fixed_t b_width_half = (panelB.bbox[BOXRIGHT] - panelB.bbox[BOXLEFT]) / 2;
	const fixed_t b_height_half = (panelB.bbox[BOXTOP] - panelB.bbox[BOXBOTTOM]) / 2;
	const fixed_t b_x = panelB.bbox[BOXLEFT] + b_width_half;
	const fixed_t b_y = panelB.bbox[BOXBOTTOM] + b_height_half;

	const fixed_t dx = b_x - a_x;
	const fixed_t px = (b_width_half - a_width_half) - abs(dx);
	if (px <= -overlap_extra)
	{
		return false;
	}

	const fixed_t dy = b_y - a_y;
	const fixed_t py = (b_height_half - a_height_half) - abs(dy);
	if (py <= -overlap_extra)
	{
		return false;
	}

	return true;
}

/*--------------------------------------------------
	INT32 K_CalculateTrackComplexity(void)

		Sets the value of trackcomplexity. This value accumulates all of the
		turn angle deltas to get an idea of how complicated the map is.
--------------------------------------------------*/
static INT32 K_CalculateTrackComplexity(void)
{
	const boolean huntbackwards = false;
	const boolean useshortcuts = false;

	boolean pathfindsuccess = false;
	path_t path = {0};

	trackcomplexity = BASE_TRACK_COMPLEXITY;

	if (startingwaypoint == NULL || finishline == NULL)
	{
		return trackcomplexity;
	}

	pathfindsuccess = K_PathfindToWaypoint(
		startingwaypoint, finishline,
		&path,
		useshortcuts, huntbackwards
	);

	if (pathfindsuccess == true)
	{
		auto path_finally = srb2::finally([&path]() { Z_Free(path.array); });

		for (size_t i = 1; i < path.numnodes-1; i++)
		{
			waypoint_t *const start = (waypoint_t *)path.array[ i - 1 ].nodedata;
			waypoint_t *const mid = (waypoint_t *)path.array[ i ].nodedata;
			waypoint_t *const end = (waypoint_t *)path.array[ i + 1 ].nodedata;

			const INT32 turn_id = K_GetWaypointID(mid);

			// would it be better to just check mid?
			if (K_GetWaypointIsSpawnpoint(start) == false
				|| K_GetWaypointIsSpawnpoint(mid) == false
				|| K_GetWaypointIsSpawnpoint(end) == false)
			{
				CONS_Debug(DBG_SETUP, "%s", fmt::format("TURN [{}]: skipped\n", turn_id).c_str());
				continue;
			}

			const fixed_t start_mid_dist = R_PointToDist2(
				start->mobj->x, start->mobj->y,
				mid->mobj->x, mid->mobj->y
			);
			const fixed_t mid_end_dist = R_PointToDist2(
				mid->mobj->x, mid->mobj->y,
				end->mobj->x, end->mobj->y
			);

			const angle_t start_mid_angle = R_PointToAngle2(
				start->mobj->x, start->mobj->y,
				mid->mobj->x, mid->mobj->y
			);
			const angle_t mid_end_angle = R_PointToAngle2(
				mid->mobj->x, mid->mobj->y,
				end->mobj->x, end->mobj->y
			);

			const angle_t start_mid_pitch = R_PointToAngle2(
				0, start->mobj->z,
				start_mid_dist, mid->mobj->z
			);
			const angle_t mid_end_pitch = R_PointToAngle2(
				0, mid->mobj->z,
				mid_end_dist, end->mobj->z
			);

			const fixed_t avg_radius = (start->mobj->radius + mid->mobj->radius + end->mobj->radius) / 3;
			const fixed_t base_scale = DEFAULT_WAYPOINT_RADIUS * mapobjectscale;

			// Reduce complexity with wider turns.
			fixed_t radius_factor = FixedDiv(
				base_scale,
				std::max<fixed_t>(
					1,
					avg_radius
				)
			);
			radius_factor = FRACUNIT + ((radius_factor - FRACUNIT) / 2); // reduce how much it's worth

			// Reduce complexity with wider spaced waypoints.
			fixed_t dist_factor = FixedDiv(
				base_scale,
				std::max<fixed_t>(
					1,
					start_mid_dist + mid_end_dist
				)
			);

			fixed_t wall_factor = FRACUNIT;

			constexpr fixed_t minimum_turn = 10 * FRACUNIT; // If the delta is lower than this, it's practically a straight-away.
			fixed_t delta = AngleFixed(
				AngleDelta(
					start_mid_angle,
					mid_end_angle
				)
			) - minimum_turn;

			if (delta < 0)
			{
				dist_factor = FixedDiv(FRACUNIT, std::max<fixed_t>(1, dist_factor));
				radius_factor = FixedDiv(FRACUNIT, std::max<fixed_t>(1, radius_factor));
			}
			else
			{
				// Weight turns hard enough
				delta = FixedMul(delta, delta);

				// Reduce turn complexity in walled maps.
				wall_factor = FRACUNIT;

				g_track_wp_x = mid->mobj->x;
				g_track_wp_y = mid->mobj->y;
				g_track_wp_radius = mid->mobj->radius;

				const fixed_t searchRadius = /*g_track_wp_radius +*/ MAXRADIUS;
				INT32 xl, xh, yl, yh;
				INT32 bx, by;

				const fixed_t c = FixedMul(g_track_wp_radius, FINECOSINE((start_mid_angle + ANGLE_90) >> ANGLETOFINESHIFT));
				const fixed_t s = FixedMul(g_track_wp_radius,   FINESINE((start_mid_angle + ANGLE_90) >> ANGLETOFINESHIFT));

				validcount++; // used to make sure we only process a line once

				xl = (unsigned)((g_track_wp_x + c - searchRadius) - bmaporgx)>>MAPBLOCKSHIFT;
				xh = (unsigned)((g_track_wp_x + c + searchRadius) - bmaporgx)>>MAPBLOCKSHIFT;
				yl = (unsigned)((g_track_wp_y + s - searchRadius) - bmaporgy)>>MAPBLOCKSHIFT;
				yh = (unsigned)((g_track_wp_y + s + searchRadius) - bmaporgy)>>MAPBLOCKSHIFT;

				BMBOUNDFIX(xl, xh, yl, yh);

				for (bx = xl; bx <= xh; bx++)
				{
					for (by = yl; by <= yh; by++)
					{
						if (P_BlockLinesIterator(bx, by, K_TrackWaypointNearOffroad) == false)
						{
							wall_factor /= 4;
							bx = xh + 1;
							by = yh + 1;
						}
					}
				}

				validcount++; // used to make sure we only process a line once

				xl = (unsigned)((g_track_wp_x - c - searchRadius) - bmaporgx)>>MAPBLOCKSHIFT;
				xh = (unsigned)((g_track_wp_x - c + searchRadius) - bmaporgx)>>MAPBLOCKSHIFT;
				yl = (unsigned)((g_track_wp_y - s - searchRadius) - bmaporgy)>>MAPBLOCKSHIFT;
				yh = (unsigned)((g_track_wp_y - s + searchRadius) - bmaporgy)>>MAPBLOCKSHIFT;

				BMBOUNDFIX(xl, xh, yl, yh);

				for (bx = xl; bx <= xh; bx++)
				{
					for (by = yl; by <= yh; by++)
					{
						if (P_BlockLinesIterator(bx, by, K_TrackWaypointNearOffroad) == false)
						{
							wall_factor /= 4;
							bx = xh + 1;
							by = yh + 1;
						}
					}
				}
			}

			fixed_t pitch_delta = AngleFixed(
				AngleDelta(
					start_mid_pitch,
					mid_end_pitch
				)
			);

			constexpr fixed_t minimum_drop = 30 * FRACUNIT; // If the delta is lower than this, it's probably just a slope.
			if (pitch_delta > minimum_drop)
			{
				// bonus complexity for drop-off / ramp
				constexpr fixed_t drop_factor = 10 * FRACUNIT;
				const fixed_t drop_off_mul = FRACUNIT + FixedDiv(pitch_delta - minimum_drop, drop_factor);
				delta += FixedMul(pitch_delta, drop_off_mul);
			}

			delta = FixedMul(delta, FixedMul(FixedMul(dist_factor, radius_factor), wall_factor));

			srb2::String msg = srb2::format(
				"TURN [{}]: r: {:.2f}, d: {:.2f}, w: {:.2f}, r*d*w: {:.2f}, DELTA: {}\n",
				turn_id,
				FixedToFloat(radius_factor),
				FixedToFloat(dist_factor),
				FixedToFloat(wall_factor),
				FixedToFloat(FixedMul(FixedMul(dist_factor, radius_factor), wall_factor)),
				(delta / FRACUNIT)
			);
			CONS_Debug(DBG_SETUP, "%s", msg.c_str());
			trackcomplexity += (delta / FRACUNIT);
		}

		srb2::Vector<complexity_sneaker_s> sneaker_panels;

		for (size_t i = 0; i < numsectors; i++)
		{
			sector_t *const sec = &sectors[i];
			if (sec->linecount == 0)
			{
				continue;
			}

			terrain_t *terrain_f = K_GetTerrainForFlatNum(sec->floorpic);
			terrain_t *terrain_c = K_GetTerrainForFlatNum(sec->ceilingpic);

			if ((terrain_f != nullptr && (terrain_f->flags & TRF_SNEAKERPANEL))
				|| (terrain_c != nullptr && (terrain_c->flags & TRF_SNEAKERPANEL)))
			{
				complexity_sneaker_s new_panel(sec);
				boolean create_new = true;

				for (size_t j = 0; j < sec->linecount; j++)
				{
					line_t *const ld = sec->lines[j];

					M_AddToBox(new_panel.bbox, ld->bbox[BOXRIGHT], ld->bbox[BOXTOP]);
					M_AddToBox(new_panel.bbox, ld->bbox[BOXLEFT], ld->bbox[BOXBOTTOM]);
				}

				for (auto &panel : sneaker_panels)
				{
					if (K_SneakerPanelOverlap(new_panel, panel) == true)
					{
						// merge together
						M_AddToBox(panel.bbox, new_panel.bbox[BOXRIGHT], new_panel.bbox[BOXTOP]);
						M_AddToBox(panel.bbox, new_panel.bbox[BOXLEFT], new_panel.bbox[BOXBOTTOM]);
						//panel.sectors.push_back(sec);
						create_new = false;
						break;
					}
				}

				if (create_new == true)
				{
					//new_panel.sectors.push_back(sec);
					sneaker_panels.push_back(new_panel);
				}
			}
		}

		for (size_t i = 0; i < nummapthings; i++)
		{
			mapthing_t *const mt = &mapthings[i];
			if (mt->type != mobjinfo[MT_SNEAKERPANEL].doomednum)
			{
				continue;
			}

			complexity_sneaker_s new_panel(mt);
			boolean create_new = true;

			for (auto &panel : sneaker_panels)
			{
				if (K_SneakerPanelOverlap(new_panel, panel) == true)
				{
					// merge together
					M_AddToBox(panel.bbox, new_panel.bbox[BOXRIGHT], new_panel.bbox[BOXTOP]);
					M_AddToBox(panel.bbox, new_panel.bbox[BOXLEFT], new_panel.bbox[BOXBOTTOM]);
					create_new = false;
					break;
				}
			}

			if (create_new == true)
			{
				sneaker_panels.push_back(new_panel);
			}
		}

		CONS_Debug(DBG_SETUP, "%s", fmt::format("Num sneaker panel sets: {}\n", sneaker_panels.size()).c_str());
		trackcomplexity -= sneaker_panels.size() * 1250;

		CONS_Debug(DBG_SETUP, " ** MAP COMPLEXITY: %d\n", trackcomplexity);
	}

	return trackcomplexity;
}

}; // namespace

/*--------------------------------------------------
	boolean K_SetupWaypointList(void)

		See header file for description.
--------------------------------------------------*/
boolean K_SetupWaypointList(void)
{
	boolean setupsuccessful = false;

	K_FreeWaypoints();

	if (!waypointcap)
	{
		CONS_Alert(CONS_ERROR, "No waypoints in map.\n");
	}
	else
	{
		if (K_AllocateWaypointHeap() == true)
		{
			mobj_t *waypointmobj = NULL;

			// Loop through the waypointcap here so that all waypoints are added to the heap, and allow easier debugging
			for (waypointmobj = waypointcap; waypointmobj; waypointmobj = waypointmobj->tracer)
			{
				waypointmobj->cusval = (INT32)numwaypoints;
				K_SetupWaypoint(waypointmobj);
			}

			if (firstwaypoint == NULL)
			{
				CONS_Alert(CONS_ERROR, "No waypoints in map.\n");
			}
			else
			{
				CONS_Debug(DBG_SETUP, "Successfully setup %s waypoints.\n", sizeu1(numwaypoints));
				if (finishline == NULL)
				{
					CONS_Alert(
						CONS_WARNING, "No finish line waypoint in the map! Using first setup waypoint with ID %d.\n",
						K_GetWaypointID(firstwaypoint));
					finishline = firstwaypoint;
				}

				if (K_SetupCircuitLength() == 0)
				{
					CONS_Alert(CONS_ERROR, "Circuit track waypoints do not form a circuit.\n");
				}

				if (startingwaypoint != NULL)
				{
					K_CalculateTrackComplexity();
				}

				setupsuccessful = true;
			}
		}
	}

	return setupsuccessful;
}

/*--------------------------------------------------
	void K_ClearWaypoints(void)

		See header file for description.
--------------------------------------------------*/
void K_ClearWaypoints(void)
{
	waypointheap     = NULL;
	firstwaypoint    = NULL;
	finishline       = NULL;
	startingwaypoint = NULL;
	numwaypoints     = 0U;
	numwaypointmobjs = 0U;
	circuitlength    = 0U;
	trackcomplexity  = 0U;
}

/*--------------------------------------------------
	static boolean K_RaiseWaypoint(
		mobj_t *const       waypointmobj,
		const mobj_t *const riser)

		Raise a waypoint according a waypoint riser thing.

	Input Arguments:-
		waypointmobj - The mobj of the waypoint to raise
		riser        - The waypoint riser mobj

	Return:-
		True if the waypoint was risen, false if not.
--------------------------------------------------*/

static boolean K_RaiseWaypoint(
		mobj_t *const       waypointmobj,
		const mobj_t *const riser)
{
	fixed_t x;
	fixed_t y;

	const sector_t *sector;
	ffloor_t *rover;

	boolean descending;

	fixed_t sort;
	fixed_t z;

	if (
			!( riser->spawnpoint->options & MTF_OBJECTSPECIAL ) ||
			riser->spawnpoint->angle == waypointmobj->spawnpoint->angle
	){
		if (( riser->spawnpoint->options & MTF_AMBUSH ))
		{
			waypointmobj->z = riser->z;
		}
		else
		{
			x = waypointmobj->x;
			y = waypointmobj->y;

			descending = ( riser->spawnpoint->options & MTF_OBJECTFLIP );

			sector = waypointmobj->subsector->sector;

			if (descending)
				sort = sector->ceilingheight;
			else
				sort = sector->floorheight;

			for (
					rover = sector->ffloors;
					rover;
					rover = rover->next
			){
				if (descending)
				{
					z = P_GetZAt(*rover->b_slope, x, y, *rover->bottomheight);

					if (z > riser->z && z < sort)
						sort = z;
				}
				else
				{
					z = P_GetZAt(*rover->t_slope, x, y, *rover->topheight);

					if (z < riser->z && z > sort)
						sort = z;
				}
			}

			waypointmobj->z = sort;
		}

		// Keep changes for -writetextmap
		waypointmobj->spawnpoint->z = ((waypointmobj->spawnpoint->options & MTF_OBJECTFLIP)
			? waypointmobj->ceilingz - waypointmobj->z
			: waypointmobj->z - waypointmobj->floorz) / FRACUNIT;

		return true;
	}
	else
		return false;
}

/*--------------------------------------------------
	static boolean K_AnchorWaypointRadius(
		mobj_t *const       waypointmobj,
		const mobj_t *const anchor)

		Adjust a waypoint's radius by distance from an "anchor".

	Input Arguments:-
		waypointmobj - The mobj of the waypoint whose radius to adjust
		riser        - The waypoint anchor mobj

	Return:-
		True if the waypoint's radius was adjusted, false if not.
--------------------------------------------------*/

static boolean K_AnchorWaypointRadius(
		mobj_t *const       waypointmobj,
		const mobj_t *const anchor)
{
	if (anchor->spawnpoint->angle == waypointmobj->spawnpoint->angle)
	{
		waypointmobj->radius = R_PointToDist2(
				waypointmobj->x, waypointmobj->y,
				anchor->x, anchor->y);

		// Keep changes for -writetextmap
		waypointmobj->spawnpoint->thing_args[1] = waypointmobj->radius >> FRACBITS;
		return true;
	}
	else
		return false;
}

/*--------------------------------------------------
	void K_AdjustWaypointsParameters(void)

		See header file for description.
--------------------------------------------------*/

void K_AdjustWaypointsParameters (void)
{
	mobj_t *waypointmobj;
	const mobj_t *riser;

	const thinker_t *th;
	const mobj_t *anchor;

	const sector_t *sector;

	for (
			waypointmobj = waypointcap;
			waypointmobj;
			waypointmobj = waypointmobj->tracer
	){
		sector = waypointmobj->subsector->sector;

		for (
				riser = sector->thinglist;
				riser;
				riser = riser->snext
		){
			if (riser->type == MT_WAYPOINT_RISER)
			{
				if (K_RaiseWaypoint(waypointmobj, riser))
					break;
			}
		}
	}

	for (
		th = thlist[THINK_MOBJ].next;
		th != &thlist[THINK_MOBJ];
		th = th->next
	){
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		anchor = (const mobj_t *)th;

		if (anchor->type == MT_WAYPOINT_ANCHOR)
		{
			for (
					waypointmobj = waypointcap;
					waypointmobj;
					waypointmobj = waypointmobj->tracer
			){
				K_AnchorWaypointRadius(waypointmobj, anchor);
			}
		}
	}
}
