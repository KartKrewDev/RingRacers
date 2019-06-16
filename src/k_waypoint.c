#include "k_waypoint.h"

#include "d_netcmd.h"
#include "p_local.h"
#include "p_tick.h"
#include "z_zone.h"

// The number of sparkles per waypoint connection in the waypoint visualisation
static const UINT32 SPARKLES_PER_CONNECTION = 16U;

// Some defaults for the size of the dynamically allocated sets for pathfinding. These are kept for the purpose of
// allocating a size that is less likely to need reallocating again during the pathfinding.
static const size_t OPENSET_BASE_SIZE    = 16U;
static const size_t CLOSEDSET_BASE_SIZE  = 256U;
static const size_t NODESARRAY_BASE_SIZE = 256U;

static waypoint_t *waypointheap = NULL;
static waypoint_t *firstwaypoint = NULL;
static waypoint_t *finishline    = NULL;

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
	boolean K_GetWaypointIsFinishline(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
boolean K_GetWaypointIsFinishline(waypoint_t *waypoint)
{
	boolean waypointisfinishline = false;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointIsShortcut.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint mobj in K_GetWaypointIsShortcut.\n");
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
	else if (waypoint->mobj == NULL)
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
	else if (waypoint->mobj == NULL)
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
	else if (waypoint->mobj == NULL)
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
	else if (waypoint->mobj == NULL)
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
	size_t K_GetWaypointHeapIndex(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
size_t K_GetWaypointHeapIndex(waypoint_t *waypoint)
{
	size_t waypointindex = SIZE_MAX;

	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_GetWaypointID.\n");
	}
	else
	{
		waypointindex = waypoint - waypointheap;
	}

	return waypointindex;
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
	INT32 n;
	UINT32 numofframes = 1; // If this was 0 it could divide by 0

	// Error conditions
	if (waypoint1 == NULL || waypoint2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_DebugWaypointsSpawnLine.\n");
		return;
	}
	if (waypoint1->mobj == NULL || waypoint2->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj on waypoint in K_DebugWaypointsSpawnLine.\n");
		return;
	}
	if (cv_kartdebugwaypoints.value == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "In K_DebugWaypointsSpawnLine when kartdebugwaypoints is off.\n");
		return;
	}

	waypointmobj1 = waypoint1->mobj;
	waypointmobj2 = waypoint2->mobj;

	n = SPARKLES_PER_CONNECTION;
	numofframes = S_SPRK16 - S_SPRK1;

	// Draw the sparkles
	stepx = (waypointmobj2->x - waypointmobj1->x) / n;
	stepy = (waypointmobj2->y - waypointmobj1->y) / n;
	stepz = (waypointmobj2->z - waypointmobj1->z) / n;
	x = waypointmobj1->x;
	y = waypointmobj1->y;
	z = waypointmobj1->z;
	do
	{
		spawnedmobj = P_SpawnMobj(x, y, z, MT_SPARK);
		P_SetMobjState(spawnedmobj, S_SPRK1 + ((leveltime + n) % (numofframes + 1)));
		spawnedmobj->state->nextstate = S_NULL;
		spawnedmobj->state->tics = 1;
		x += stepx;
		y += stepy;
		z += stepz;
	} while (n--);
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
		waypoint = K_SearchWaypointHeapForMobj(waypointmobj);

		debugmobj = P_SpawnMobj(waypointmobj->x, waypointmobj->y, waypointmobj->z, MT_SPARK);
		P_SetMobjState(debugmobj, S_THOK);

		// There's a waypoint setup for this mobj! So draw that it's a valid waypoint and draw lines to its connections
		if (waypoint != NULL)
		{
			if (waypoint->numnextwaypoints == 0 && waypoint->numprevwaypoints == 0)
			{
				debugmobj->color = SKINCOLOR_RED;
			}
			else if (waypoint->numnextwaypoints == 0 || waypoint->numprevwaypoints == 0)
			{
				debugmobj->color = SKINCOLOR_ORANGE;
			}
			else
			{
				debugmobj->color = SKINCOLOR_BLUE;
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
		debugmobj->state->tics = 1;
		debugmobj->state->nextstate = S_NULL;
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
	if (waypoint1 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint1 in K_DistanceBetweenWaypoints.\n");
	}
	else if (waypoint2 == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint2 in K_DistanceBetweenWaypoints.\n");
	}
	else
	{
		const fixed_t xydist =
			P_AproxDistance(waypoint1->mobj->x - waypoint2->mobj->x, waypoint1->mobj->y - waypoint2->mobj->y);
		const fixed_t xyzdist = P_AproxDistance(xydist, waypoint1->mobj->z - waypoint2->mobj->z);
		finaldist = ((UINT32)xyzdist >> FRACBITS);
	}

	return finaldist;
}

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

static boolean K_WaypointPathfindTraversableAllEnabled(void *data)
{
	boolean traversable = false;

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

static boolean K_WaypointPathfindTraversableNoShortcuts(void *data)
{
	boolean traversable = false;

	if (data == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_WaypointPathfindTraversableNoShortcuts received NULL data.\n");
	}
	else
	{
		waypoint_t *waypoint = (waypoint_t *)data;
		traversable = ((K_GetWaypointIsShortcut(waypoint) == false) && (K_GetWaypointIsEnabled(waypoint) == true));
	}

	return traversable;
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
		pathfindsetup_t            pathfindsetup   = {};
		getconnectednodesfunc      nextnodesfunc   = K_WaypointPathfindGetNext;
		getnodeconnectioncostsfunc nodecostsfunc   = K_WaypointPathfindGetNextCosts;
		getnodeheuristicfunc       heuristicfunc   = K_WaypointPathfindGetHeuristic;
		getnodetraversablefunc     traversablefunc = K_WaypointPathfindTraversableNoShortcuts;

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
			path_t                     pathtowaypoint  = {};
			pathfindsetup_t            pathfindsetup   = {};
			boolean                    pathfindsuccess = false;
			getconnectednodesfunc      nextnodesfunc   = K_WaypointPathfindGetNext;
			getnodeconnectioncostsfunc nodecostsfunc   = K_WaypointPathfindGetNextCosts;
			getnodeheuristicfunc       heuristicfunc   = K_WaypointPathfindGetHeuristic;
			getnodetraversablefunc     traversablefunc = K_WaypointPathfindTraversableNoShortcuts;

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
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_CheckWaypointForMobj.\n");
	}
	else if (waypoint->mobj == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "Waypoint has NULL mobj in K_CheckWaypointForMobj.\n");
	}
	else if (mobjpointer == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobjpointer in K_CheckWaypointForMobj.\n");
	}
	else
	{
		mobj_t *mobj = (mobj_t *)mobjpointer;

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
	if (condition == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL condition in K_TraverseWaypoints.\n");
	}
	else if (conditionalfunc == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL conditionalfunc in K_TraverseWaypoints.\n");
	}
	else if (visitedarray == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL visitedarray in K_TraverseWaypoints.\n");
	}
	else
	{

searchwaypointstart:
		if (waypoint == NULL)
		{
			CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_TraverseWaypoints.\n");
		}
		else
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
	if (condition == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL condition in K_SearchWaypointGraph.\n");
	}
	else if (conditionalfunc == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL conditionalfunc in K_SearchWaypointGraph.\n");
	}
	else if (firstwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_SearchWaypointsForMobj called when no first waypoint.\n");
	}
	else
	{
		visitedarray = Z_Calloc(numwaypoints * sizeof(boolean), PU_STATIC, NULL);
		foundwaypoint = K_TraverseWaypoints(firstwaypoint, conditionalfunc, condition, visitedarray);
		Z_Free(visitedarray);
	}

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
	if (condition == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL condition in K_SearchWaypointHeap.\n");
	}
	else if (conditionalfunc == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL conditionalfunc in K_SearchWaypointHeap.\n");
	}
	else if (waypointheap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_SearchWaypointHeap called when no waypointheap.\n");
	}
	else
	{
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
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL waypoint in K_AddPrevToWaypoint.\n");
	}
	else if (prevwaypoint == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL prevwaypoint in K_AddPrevToWaypoint.\n");
	}
	else
	{
		waypoint->numprevwaypoints++;
		waypoint->prevwaypoints =
			Z_Realloc(waypoint->prevwaypoints, waypoint->numprevwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);

		if (!waypoint->prevwaypoints)
		{
			I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoints.");
		}

		waypoint->prevwaypointdistances =
			Z_Realloc(waypoint->prevwaypointdistances, waypoint->numprevwaypoints * sizeof(fixed_t), PU_LEVEL, NULL);

		if (!waypoint->prevwaypointdistances)
		{
			I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoint distances.");
		}

		waypoint->prevwaypoints[waypoint->numprevwaypoints - 1] = prevwaypoint;
	}
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
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_MakeWaypoint.\n");
	}
	else if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_MakeWaypoint called with NULL waypointcap.\n");
	}
	else if (numwaypoints >= numwaypointmobjs)
	{
		CONS_Debug(DBG_SETUP, "K_MakeWaypoint called with max waypoint capacity reached.\n");
	}
	else
	{
		// numwaypoints is incremented later in K_SetupWaypoint
		madewaypoint = &waypointheap[numwaypoints];
		numwaypoints++;

		P_SetTarget(&madewaypoint->mobj, mobj);

		// Go through the other waypoint mobjs in the map to find out how many waypoints are after this one
		for (otherwaypointmobj = waypointcap; otherwaypointmobj != NULL; otherwaypointmobj = otherwaypointmobj->tracer)
		{
			// threshold = next waypoint id, movecount = my id
			if (mobj->threshold == otherwaypointmobj->movecount)
			{
				madewaypoint->numnextwaypoints++;
			}
		}

		// No next waypoints
		if (madewaypoint->numnextwaypoints != 0)
		{
			// Allocate memory to hold enough pointers to all of the next waypoints
			madewaypoint->nextwaypoints =
				Z_Calloc(madewaypoint->numnextwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);
			if (madewaypoint->nextwaypoints == NULL)
			{
				I_Error("K_MakeWaypoint: Out of Memory allocating next waypoints.");
			}
			madewaypoint->nextwaypointdistances =
				Z_Calloc(madewaypoint->numnextwaypoints * sizeof(fixed_t), PU_LEVEL, NULL);
			if (madewaypoint->nextwaypointdistances == NULL)
			{
				I_Error("K_MakeWaypoint: Out of Memory allocating next waypoint distances.");
			}
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
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_SetupWaypoint.\n");
	}
	else if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_SETUP, "Non MT_WAYPOINT mobj in K_SetupWaypoint. Type=%d.\n", mobj->type);
	}
	else if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_SetupWaypoint called with NULL waypointcap.\n");
	}
	else
	{
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
					CONS_Alert(
						CONS_WARNING, "Waypoint with ID %d has no next waypoint.\n", K_GetWaypointID(thiswaypoint));
				}
			}
			else
			{
				CONS_Debug(DBG_SETUP, "K_SetupWaypoint failed to make new waypoint with ID %d.\n", mobj->movecount);
			}
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
	if (waypointheap != NULL)
	{
		CONS_Debug(DBG_SETUP, "K_AllocateWaypointHeap called when waypointheap is already allocated.\n");
	}
	else if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_AllocateWaypointHeap called with NULL waypointcap.\n");
	}
	else
	{
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
			waypointheap = Z_Calloc(numwaypointmobjs * sizeof(waypoint_t), PU_LEVEL, NULL);

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
	numwaypoints     = 0;
	numwaypointmobjs = 0;
}
