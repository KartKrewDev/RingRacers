#ifndef __K_WAYPOINT__
#define __K_WAYPOINT__

#include "doomdef.h"
#include "p_mobj.h"

typedef struct waypoint_s
{
	mobj_t             *mobj;
	size_t              id;
	struct waypoint_s **nextwaypoints;
	struct waypoint_s **prevwaypoints;
	fixed_t            *nextwaypointdistances;
	fixed_t            *prevwaypointdistances;
	size_t              numnextwaypoints;
	size_t              numprevwaypoints;
} waypoint_t;


// AVAILABLE FOR LUA


/*--------------------------------------------------
	waypoint_t *K_SearchWaypointGraphForMobj(mobj_t *const mobj)

		Searches through the waypoint graph for a waypoint that has an mobj, if a waypoint can be found through here it
		does mean that the waypoint graph can be traversed to find it

	Input Arguments:-
		mobj - The mobj that we are searching for, cannot be changed to a different pointer

	Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/

waypoint_t *K_SearchWaypointGraphForMobj(mobj_t * const mobj);

/*--------------------------------------------------
	waypoint_t *K_SearchWaypointHeapForMobj(mobj_t *const mobj)

		Searches through the waypoint heap for a waypoint that has an mobj, this does not necessarily mean the waypoint
		can be reached from another waypoint

	Input Arguments:-
		mobj - The mobj that we are searching for, cannot be changed to a different pointer

	Return:-
		The waypoint that uses that mobj, NULL if it wasn't found, NULL if it isn't an MT_WAYPOINT
--------------------------------------------------*/

waypoint_t *K_SearchWaypointHeapForMobj(mobj_t * const mobj);

// NOT AVAILABLE FOR LUA


/*--------------------------------------------------
	void K_DebugWaypointsVisualise()

		Creates mobjs in order to visualise waypoints for debugging.
--------------------------------------------------*/

void K_DebugWaypointsVisualise(void);


/*--------------------------------------------------
	boolean K_SetupWaypointList(void)

		Sets up the waypoint list for Kart race maps, prints out warnings if something is wrong.

	Return:-
		true if waypoint setup was seemingly successful, false if no waypoints were setup
		A true return value does not necessarily mean that the waypoints on the map are completely correct
--------------------------------------------------*/

boolean K_SetupWaypointList(void);


/*--------------------------------------------------
	void K_ClearWaypoints(void)

		Clears waypointheap, firstwaypoint, numwaypoints, and numwaypointmobjs
		WARNING: This does *not* Free waypointheap or any waypoints! They are stored in PU_LEVEL so they are freed once
		the level is completed! This is called just before K_SetupWaypointList in P_SetupLevel as they are freed then.
		A separate method is called in K_SetupWaypointList that will free everything specifically if they aren't already
--------------------------------------------------*/

void K_ClearWaypoints(void);

#endif