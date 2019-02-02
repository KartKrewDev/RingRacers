#include "k_waypoint.h"

#include "doomdef.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "z_zone.h"

static waypoint_t **waypointheap = NULL;
static waypoint_t *firstwaypoint = NULL;
static size_t numwaypoints = 0;
static size_t numwaypointmobjs = 0;

/*--------------------------------------------------
	waypoint_t *K_SearchWaypoints(waypoint_t *waypoint, mobj_t * const mobj, boolean * const visitedarray)

		Searches through the waypoint list for a waypoint that has an mobj, just does a simple flood search for the
		waypoint that uses this mobj, no pathfinding

	Input Arguments:-
		waypoint - The waypoint that is currently being checked, goes through nextwaypoints after this one
		mobj - the mobj that we are searching for, cannot be changed to a different pointer
		visitedarray - An array of booleans that let us know if a waypoint has already been checked, marked to true when
			one is, so we don't repeat going down a path. Cannot be changed to a different pointer

  Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/
static waypoint_t *K_SearchWaypoints(waypoint_t *waypoint, mobj_t * const mobj, boolean * const visitedarray)
{
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypoints.\n");
		return NULL;
	}
	if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypoints. Type=%d.\n", mobj->type);
		return NULL;
	}
	if (visitedarray == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL visitedarray in K_SearchWaypoints.\n");
		return NULL;
	}

searchwaypointstart:
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL waypoint in K_SearchWaypoints.\n");
		return NULL;
	}

	// If we've already visited this waypoint, we've already checked the next waypoints, and continuing is useless
	if (visitedarray[waypoint->id] != true)
	{
		// Mark this waypoint as being visited
		visitedarray[waypoint->id] = true;

		if (waypoint->mobj == mobj)
		{
			foundwaypoint = waypoint;
		}
		else
		{
			// If this waypoint only has one next waypoint, set the waypoint to be the next one and jump back
			// to the start, this is to avoid going too deep into the stack where we can
			// Yes this is a horrible horrible goto, but the alternative is a do while loop with an extra variable,
			// which is slightly more confusing. This is probably the fastest and least confusing option that keeps
			// this functionality
			if (waypoint->numnextwaypoints == 1 && waypoint->nextwaypoints[0] != NULL)
			{
				waypoint = waypoint->nextwaypoints[0];
				goto searchwaypointstart;
			}
			else if (waypoint->numnextwaypoints != 0)
			{
				UINT32 i;
				// For each next waypoint, Search through it's path continuation until we hopefully find the one we're
				// looking for
				for (i = 0; i < waypoint->numnextwaypoints; i++)
				{
					if (waypoint->nextwaypoints[i] != NULL)
					{
						foundwaypoint = K_SearchWaypoints(waypoint->nextwaypoints[i], mobj, visitedarray);

						if (foundwaypoint != NULL)
						{
							break;
						}
					}
				}
			}
		}
	}

	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointGraphForMobj(mobj_t * const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_SearchWaypointGraphForMobj(mobj_t * const mobj)
{
	boolean *visitedarray = NULL;
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypointsForMobj.\n");
		return NULL;
	}
	if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypointsForMobj. Type=%d.\n", mobj->type);
		return NULL;
	}
	if (firstwaypoint == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_SearchWaypointsForMobj called when no first waypoint.\n");
		return NULL;
	}

	visitedarray = Z_Calloc(numwaypoints * sizeof(boolean), PU_STATIC, NULL);
	foundwaypoint = K_SearchWaypoints(firstwaypoint, mobj, visitedarray);
	Z_Free(visitedarray);
	return foundwaypoint;
}

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointHeapForMobj(mobj_t * const mobj)

		See header file for description.
--------------------------------------------------*/
waypoint_t *K_SearchWaypointHeapForMobj(mobj_t * const mobj)
{
	UINT32 i = 0;
	waypoint_t *foundwaypoint = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_GAMELOGIC, "NULL mobj in K_SearchWaypointsForMobj.\n");
		return NULL;
	}
	if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_GAMELOGIC, "Non MT_WAYPOINT mobj in K_SearchWaypointsForMobj. Type=%d.\n", mobj->type);
		return NULL;
	}
	if (waypointheap == NULL)
	{
		CONS_Debug(DBG_GAMELOGIC, "K_SearchWaypointsForMobj called when no waypointheap.\n");
		return NULL;
	}

	// Simply search through the waypointheap for the waypoint with the mobj! Much simpler when no pathfinding needed.
	// search up to numwaypoints and NOT numwaypointmobjs as numwaypoints is the real number of waypoints setup in
	// the heap while numwaypointmobjs ends up being the capacity
	for (i = 0; i < numwaypoints; i++)
	{
		if (waypointheap[i]->mobj == mobj)
		{
			foundwaypoint = waypointheap[i];
			break;
		}
	}

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
	// Error conditions
	if (waypoint == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL waypoint in K_AddPrevToWaypoint.\n");
		return;
	}
	if (prevwaypoint == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL prevwaypoint in K_AddPrevToWaypoint.\n");
		return;
	}

	waypoint->numprevwaypoints++;
	waypoint->prevwaypoints =
		Z_Realloc(waypoint->prevwaypoints, waypoint->numprevwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);

	if (!waypoint->prevwaypoints)
	{
		I_Error("K_AddPrevToWaypoint: Failed to reallocate memory for previous waypoints.");
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
	waypoint_t *waypoint;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_NewWaypoint.\n");
		return NULL;
	}
	if (waypointheap == NULL)
	{
		CONS_Debug(DBG_SETUP, "NULL waypointheap in K_NewWaypoint.\n");
		return NULL;
	}

	// Each made waypoint is placed directly into the waypoint heap to be able to search it during creation
	waypointheap[numwaypoints] = Z_Calloc(sizeof(waypoint_t), PU_LEVEL, NULL);
	waypoint = waypointheap[numwaypoints];
	// numwaypoints is incremented later when waypoint->id is set

	if (!waypoint)
	{
		I_Error("K_NewWaypoint: Failed to allocate memory for waypoint.");
	}

	P_SetTarget(&waypoint->mobj, mobj);
	waypoint->id = numwaypoints++;

	return waypoint;
}

/*--------------------------------------------------
	static waypoint_t *K_MakeWaypoint(mobj_t *mobj)

		Make a new waypoint from a map object. Setups up most of the data for it, and allocates most memory
		Remaining creation is handled in K_SetupWaypoint

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_MakeWaypoint(mobj_t *mobj)
{
	waypoint_t *madewaypoint = NULL;
	mobj_t *otherwaypointmobj = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_MakeWaypoint.\n");
		return NULL;
	}
	if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_MakeWaypoint called with NULL waypointcap.");
		return false;
	}

	madewaypoint = K_NewWaypoint(mobj);

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
		madewaypoint->nextwaypoints = Z_Calloc(madewaypoint->numnextwaypoints * sizeof(waypoint_t *), PU_LEVEL, NULL);
		if (madewaypoint->nextwaypoints == NULL)
		{
			I_Error("K_MakeWaypoint: Out of Memory");
		}
	}

	return madewaypoint;
}

/*--------------------------------------------------
	static waypoint_t *K_SetupWaypoint(mobj_t *mobj)

		Either gets an already made waypoint, or sets up a new waypoint for an mobj,
		including next and previous waypoints

	Input Arguments:-
		mobj - The map object that this waypoint is represented by

	Return:-
		Pointer to the setup waypoint, NULL if one was not setup
--------------------------------------------------*/
static waypoint_t *K_SetupWaypoint(mobj_t *mobj)
{
	waypoint_t *thiswaypoint = NULL;

	// Error conditions
	if (mobj == NULL || P_MobjWasRemoved(mobj))
	{
		CONS_Debug(DBG_SETUP, "NULL mobj in K_SetupWaypoint.\n");
		return NULL;
	}
	if (mobj->type != MT_WAYPOINT)
	{
		CONS_Debug(DBG_SETUP, "Non MT_WAYPOINT mobj in K_SetupWaypoint. Type=%d.\n", mobj->type);
		return NULL;
	}
	if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_SetupWaypoint called with NULL waypointcap.");
		return false;
	}

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

		// Temporarily set the first waypoint to be the first waypoint we setup, this is so that we can search
		// through them as they're made and added to the linked list
		if (firstwaypoint == NULL)
		{
			firstwaypoint = thiswaypoint;
		}

		if (thiswaypoint->numnextwaypoints > 0)
		{
			// Go through the waypoint mobjs to setup the next waypoints and make this waypoint know they're its next
			// I kept this out of K_MakeWaypoint so the stack isn't gone down as deep
			for (otherwaypointmobj = waypointcap; otherwaypointmobj != NULL; otherwaypointmobj = otherwaypointmobj->tracer)
			{
				// threshold = next waypoint id, movecount = my id
				if (mobj->threshold == otherwaypointmobj->movecount)
				{
					thiswaypoint->nextwaypoints[nextwaypointindex] = K_SetupWaypoint(otherwaypointmobj);
					K_AddPrevToWaypoint(thiswaypoint->nextwaypoints[nextwaypointindex], thiswaypoint);
					nextwaypointindex++;
				}
				if (nextwaypointindex >= thiswaypoint->numnextwaypoints)
				{
					break;
				}
			}
		}
	}

	return thiswaypoint;
}

/*--------------------------------------------------
	static boolean K_AllocateWaypointHeap()

		Allocates the waypoint heap enough space for the number of waypoint mobjs on the map

	Return:-
		True if the allocation was successful, false if it wasn't. Will I_Error if out of memory still.
--------------------------------------------------*/
boolean K_AllocateWaypointHeap()
{
	mobj_t *waypointmobj = NULL;
	boolean allocationsuccessful = false;

	// Error conditions
	if (waypointheap != NULL)
	{
		CONS_Debug(DBG_SETUP, "K_AllocateWaypointHeap called when waypointheap is already allocated.");
		return false;
	}
	if (waypointcap == NULL)
	{
		CONS_Debug(DBG_SETUP, "K_AllocateWaypointHeap called with NULL waypointcap.");
		return false;
	}

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
		// Allocate space in the heap for every mobj, it's possible some mobjs aren't linked up and not all of the heap
		// allocated will be used, but it's a fairly reasonable assumption that this isn't going to be awful if true
		waypointheap = Z_Calloc(numwaypointmobjs * sizeof(waypoint_t **), PU_LEVEL, NULL);

		if (waypointheap == NULL)
		{
			// We could theoretically CONS_Debug here and continue without using waypoints, but I feel that will require
			// error checks that will end up spamming the console when we think waypoints SHOULD be working.
			// Safer to just exit if out of memory and not end up constantly trying to use the waypoints in this case
			I_Error("K_AllocateWaypointHeap: Out of memory.");
		}
		allocationsuccessful = true;
	}
	else
	{
		CONS_Debug(DBG_SETUP, "No waypoint mobjs in waypointcap.");
	}

	return allocationsuccessful;
}

/*--------------------------------------------------
	void K_FreeWaypoints()

		For safety, this will free the waypointheap and all the waypoints allocated if they aren't already before they
		are setup. If the PU_LEVEL tag is cleared, make sure to call K_ClearWaypoints or this will try to free already
		freed memory!
--------------------------------------------------*/
void K_FreeWaypoints()
{
	if (waypointheap != NULL)
	{
		// Free each waypoint if it's not already
		INT32 i;
		for (i = 0; i < numwaypoints; i++)
		{
			if (waypointheap[i] != NULL)
			{
				Z_Free(waypointheap[i]);
			}
			else
			{
				CONS_Debug(DBG_SETUP, "NULL waypoint %d attempted to be freed.", i);
			}
		}

		// Free the waypointheap
		Z_Free(waypointheap);
	}

	K_ClearWaypoints();
}

/*--------------------------------------------------
	boolean K_SetupWaypointList()

		See header file for description.
--------------------------------------------------*/
boolean K_SetupWaypointList()
{
	boolean setupsuccessful = false;

	K_FreeWaypoints();

	if (!waypointcap)
	{
		CONS_Debug(DBG_SETUP, "K_SetupWaypointList called with no waypointcap.\n");
	}
	else
	{
		if (K_AllocateWaypointHeap() == true)
		{
			// The waypoint in the waypointcap is going to be considered our first waypoint
			K_SetupWaypoint(waypointcap);

			if (!firstwaypoint)
			{
				CONS_Debug(DBG_SETUP, "K_SetupWaypointList made no waypoints.\n");
			}
			else
			{
				CONS_Debug(DBG_SETUP, "Successfully setup %zu waypoints.\n", numwaypoints);
				if (numwaypoints < numwaypointmobjs)
				{
					CONS_Printf("Not all waypoints in the map are connected! %zu waypoints setup but %zu mobjs.",
						numwaypoints, numwaypointmobjs);
				}
				setupsuccessful = true;
			}
		}
	}

	return setupsuccessful;
}

/*--------------------------------------------------
	void K_ClearWaypoints()

		See header file for description.
--------------------------------------------------*/
void K_ClearWaypoints()
{
	waypointheap = NULL;
	numwaypoints = 0;
	numwaypointmobjs = 0;
}
