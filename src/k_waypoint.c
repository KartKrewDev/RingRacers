#include "k_waypoint.h"

#include "doomdef.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "z_zone.h"

static waypoint_t *firstwaypoint = NULL;
static UINT32 numwaypoints = 0;

/*--------------------------------------------------
  waypoint_t *K_SearchWaypoints(waypoint_t *waypoint, mobj_t * const mobj, boolean * const visitedarray)

	  Searches through the waypoint list for a waypoint that has an mobj, just does a simple flood search for the
	  waypoint that uses this mobj, no pathfinding

  Input Arguments:-
	  waypoint - The waypoint that is currently being checked, goes through nextWaypoints after this one
	  mobj - the mobj that we are searching for, cannot be changed to a different pointer
	  visitedarray - An array of booleans that let us know if a waypoint has already been checked, marked to true when
		  one is, so we don't repeat going down a path. Cannot be changed to a different pointer

  Return:-
	  The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_SearchWaypoints(waypoint_t *waypoint, mobj_t * const mobj, boolean * const visitedarray)
{
	UINT32 i;
	waypoint_t *foundwaypoint;

	if (mobj->type != MT_WAYPOINT)
	{
		return NULL;
	}

searchwaypointstart:
	// If we've already visited this waypoint, we've already checked the next waypoint, and going further is useless
	if (visitedarray[waypoint->id] == true)
	{
		return NULL;
	}

	// Mark this waypoint as being visited
	visitedarray[waypoint->id] = true;

	if (waypoint->mobj == mobj)
	{
		return waypoint;
	}

	// If this waypoint only has one next waypoint, set the waypoint to be the only next one and jump back to the start
	// this is to avoid going too deep into the stack where we can
	if (waypoint->numnextwaypoints == 1)
	{
		waypoint = waypoint->nextwaypoints[0];
		goto searchwaypointstart;
	}

	// This waypoint has no next waypoint, so just stop searching
	if (waypoint->numnextwaypoints == 0)
	{
		return NULL;
	}

	// For each next waypoint, Search through it's path continuation until we hopefully find the one we're looking for
	for (i = 0; i < waypoint->numnextwaypoints; i++)
	{
		foundwaypoint = K_SearchWaypoints(waypoint->nextwaypoints[i], mobj, visitedarray);

		if (foundwaypoint != NULL)
		{
			return foundwaypoint;
		}
	}

	// Matching waypoint was found down this path
	return NULL;
}

/*--------------------------------------------------
  waypoint_t *K_SearchWaypointsForMobj(mobj_t * const mobj)

	  Searches through the waypoint list for a waypoint that has an mobj

  Input Arguments:-
	  mobj - The mobj that we are searching for, cannot be changed to a different pointer

  Return:-
	  The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
waypoint_t *K_SearchWaypointsForMobj(mobj_t * const mobj)
{
	boolean *visitedarray;
	waypoint_t *foundwaypoint;

	// If the mobj it's looking for isn't even a waypoint, then reject it
	if (mobj->type != MT_WAYPOINT)
	{
		return NULL;
	}

	visitedarray = Z_Calloc(numwaypoints * sizeof(boolean), PU_STATIC, NULL);
	foundwaypoint = K_SearchWaypoints(firstwaypoint, mobj, visitedarray);
	Z_Free(visitedarray);
	return foundwaypoint;
}

/*--------------------------------------------------
  static void K_AddPrevToWaypoint(waypoint_t *waypoint, waypoint_t *prevwaypoint)

	  Adds another waypoint to a waypoint's previous waypoint list, this needs to be done like this because there is no
	  way to identify previous waypoints from just IDs, so we need to reallocate the memory for every previous waypoint

  Input Arguments:-
	  waypoint - The waypoint which is having its previous waypoint list added to
	  prevwaypoint - The waypoint which is being added to the previous waypoint list

  Return:-
	  Pointer to waypoint_t for the rest of the waypoint data to be placed into
--------------------------------------------------*/
static void K_AddPrevToWaypoint(waypoint_t *waypoint, waypoint_t *prevwaypoint)
{
	waypoint->numprevwaypoints++;
	waypoint->prevwaypoints = Z_Realloc(waypoint->prevwaypoints, waypoint->numprevwaypoints * sizeof(waypoint_t *),
		PU_LEVEL, NULL);

	if (!waypoint->prevwaypoints)
	{
		I_Error("K_AddPrevToWaypoint: Out of Memory");
	}

	waypoint->prevwaypoints[waypoint->numprevwaypoints - 1] = prevwaypoint;
}

/*--------------------------------------------------
  static waypoint_t *K_NewWaypoint(mobj_t *mobj)

	  Creates memory for a new waypoint

  Input Arguments:-
	  mobj - The map object that this waypoint is represented by

  Return:-
	  Pointer to waypoint_t for the rest of the waypoint data to be placed into
--------------------------------------------------*/
static waypoint_t *K_NewWaypoint(mobj_t *mobj)
{
	waypoint_t *waypoint = Z_Calloc(sizeof(waypoint_t), PU_LEVEL, NULL);

	if (!waypoint)
	{
		I_Error("K_NewWaypoint: Failed to allocate memory for waypoint.");
	}
	P_SetTarget(&waypoint->mobj, mobj);
	waypoint->id = numwaypoints++;
	return waypoint;
}

/*--------------------------------------------------
  static waypoint_t *K_SetupWaypoint(mobj_t *mobj)

	  Either gets an already made waypoint, or sets up a new waypoint for an mobj, including next and previous waypoints

  Input Arguments:-
	  mobj - The map object that this waypoint is represented by

  Return:-
	  Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_SetupWaypoint(mobj_t *mobj)
{
	UINT32 nextwaypointindex = 0;
	waypoint_t *thiswaypoint = NULL;
	mobj_t *otherwpmobj = NULL;

	// If this mobj is not an MT_WAYPOINT, don't create waypoints from it
	if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "WARNING: Non MT_WAYPOINT mobj in K_SetupWaypoint.");
		return NULL;
	}

	// If we have waypoints already created, search through them first to see if this mobj is already added.
	if (firstwaypoint != NULL)
	{
		thiswaypoint = K_SearchWaypointsForMobj(mobj);
	}

	// return the waypoint if we already made it
	if (thiswaypoint != NULL)
	{
		return thiswaypoint;
	}

	// If we haven't already made the waypoint, make it now.
	thiswaypoint = K_NewWaypoint(mobj);

	// Temporarily set the first waypoint to be the first waypoint we setup, this is so that we can search through them
	// as they're made and added to the linked list
	if (firstwaypoint != NULL)
	{
		firstwaypoint = thiswaypoint;
	}

	// Go through the other waypoint mobjs in the map to find out how many waypoints are after this one
	for (otherwpmobj = waypointcap; otherwpmobj != NULL; otherwpmobj = otherwpmobj->tracer)
	{
		if (mobj->threshold == otherwpmobj->threshold)
		{
			thiswaypoint->numnextwaypoints++;
		}
	}

	// No next waypoints
	if (thiswaypoint->numnextwaypoints == 0)
	{
		return NULL;
	}

	// Allocate memory to hold enough pointers to all of the next waypoints
	thiswaypoint->nextwaypoints = Z_Malloc(thiswaypoint->numnextwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);
	if (!thiswaypoint->numnextwaypoints)
	{
		I_Error("K_SetupWaypoint: Out of Memory");
	}

	// Go through the waypoint mobjs again to setup the next waypoints and make this waypoint know they're the next one
	for (otherwpmobj = waypointcap; otherwpmobj != NULL; otherwpmobj = otherwpmobj->tracer)
	{
		if (mobj->threshold == otherwpmobj->movecount)
		{
			thiswaypoint->nextwaypoints[nextwaypointindex] = K_SetupWaypoint(otherwpmobj);
			K_AddPrevToWaypoint(thiswaypoint->nextwaypoints[nextwaypointindex], thiswaypoint);
			nextwaypointindex++;
		}
		if (nextwaypointindex >= thiswaypoint->numnextwaypoints)
		{
			break;
		}
	}

	// Should always be returning a valid waypoint here
	return thiswaypoint;
}

/*--------------------------------------------------
  boolean K_SetupWaypointList()

	  Sets up the waypoint list for Kart race maps, does not return a status to say whether creation was fully
	  successful, but we're able to print out warnings if something is wrong.
--------------------------------------------------*/
void K_SetupWaypointList()
{
	if (waypointcap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "WARNING: K_SetupWaypointList called with no waypointcap.");
	}
	else
	{
		// The waypoint in the waypointcap is going to be considered our first waypoint
		K_SetupWaypoint(waypointcap);

		if (!firstwaypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "WARNING: K_SetupWaypointList made no waypoints.");
		}
	}
}