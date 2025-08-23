// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sean "Sryder" Ryder
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_waypoint.h
/// \brief Waypoint handling from the relevant mobjs
///        Setup and interfacing with waypoints for the main game

#ifndef __K_WAYPOINT__
#define __K_WAYPOINT__

#include "doomtype.h"
#include "p_mobj.h"
#include "k_pathfind.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_WAYPOINT_RADIUS (384)

struct waypoint_t
{
	mobj_t             *mobj;
	boolean             onaline;
	waypoint_t        **nextwaypoints;
	waypoint_t        **prevwaypoints;
	UINT32             *nextwaypointdistances;
	UINT32             *prevwaypointdistances;
	size_t              numnextwaypoints;
	size_t              numprevwaypoints;
};


// AVAILABLE FOR LUA


/*--------------------------------------------------
	waypoint_t *K_GetFinishLineWaypoint(void);

		Returns the waypoint actually being used as the finish line.

	Input Arguments:-
		None

	Return:-
		The waypoint that is being used as the finishline.
--------------------------------------------------*/

waypoint_t *K_GetFinishLineWaypoint(void);


/*--------------------------------------------------
	waypoint_t *K_GetStartingWaypoint(void);

		Return the waypoint farthest from the finish line.

	Input Arguments:-
		None

	Return:-
		The waypoint that is being used as the startingwaypoint.
--------------------------------------------------*/

waypoint_t *K_GetStartingWaypoint(void);


/*--------------------------------------------------
	boolean K_GetWaypointIsFinishline(waypoint_t *waypoint)

		Returns whether the waypoint is marked as the finishline. This may not actually be the finishline.

	Input Arguments:-
		waypoint - The waypoint to return finishline status of.

	Return:-
		true if the waypoint is marked as being the finishline, false if it isn't.
--------------------------------------------------*/

boolean K_GetWaypointIsFinishline(waypoint_t *waypoint);


/*--------------------------------------------------
	boolean K_GetWaypointIsShortcut(waypoint_t *waypoint)

		Returns whether the waypoint is part of a shortcut.

	Input Arguments:-
		waypoint - The waypoint to return shortcut status of.

	Return:-
		true if the waypoint is a shortcut, false if it isn't.
--------------------------------------------------*/

boolean K_GetWaypointIsShortcut(waypoint_t *waypoint);


/*--------------------------------------------------
	boolean K_GetWaypointIsEnabled(waypoint_t *waypoint)

		Returns whether the waypoint is enabled.

	Input Arguments:-
		waypoint - The waypoint to return enabled status of.

	Return:-
		true if the waypoint is enabled, false if it isn't.
--------------------------------------------------*/

boolean K_GetWaypointIsEnabled(waypoint_t *waypoint);


/*--------------------------------------------------
	boolean K_SetWaypointIsEnabled(waypoint_t *waypoint, boolean enabled)

		Sets whether the waypoint is enabled or not.

	Input Arguments:-
		waypoint - The waypoint to set its enabled status to.
		enabled - Boolean that sets the waypoint's enabled status.
--------------------------------------------------*/

void K_SetWaypointIsEnabled(waypoint_t *waypoint, boolean enabled);

/*--------------------------------------------------
	boolean K_GetWaypointIsSpawnpoint(waypoint_t *waypoint)

		Returns whether the waypoint is a spawnpoint.

	Input Arguments:-
		waypoint - The waypoint to return spawnpoint status of.

	Return:-
		true if the waypoint is a spawnpoint, false if it isn't.
--------------------------------------------------*/

boolean K_GetWaypointIsSpawnpoint(waypoint_t *waypoint);


/*--------------------------------------------------
	INT32 K_GetWaypointNextID(waypoint_t *waypoint)

		Returns the waypoint's next waypoint ID.

	Input Arguments:-
		waypoint - The waypoint to return the next waypoint ID of.

	Return:-
		The next waypoint ID, -1 if there is no waypoint or mobj.
--------------------------------------------------*/

INT32 K_GetWaypointNextID(waypoint_t *waypoint);


/*--------------------------------------------------
	INT32 K_GetWaypointID(waypoint_t *waypoint)

		Returns the waypoint's ID.

	Input Arguments:-
		waypoint - The waypoint to return the ID of.

	Return:-
		The waypoint ID, -1 if there is no waypoint or mobj.
--------------------------------------------------*/

INT32 K_GetWaypointID(waypoint_t *waypoint);


/*--------------------------------------------------
	waypoint_t *K_GetWaypointFromID(INT32 waypointID)

		Returns the first waypoint with the specified ID.

	Input Arguments:-
		waypointID - The ID of the waypoint to get

	Return:-
		The first waypoint with this ID, NULL if the ID doesn't exist at all in the map
--------------------------------------------------*/

waypoint_t *K_GetWaypointFromID(INT32 waypointID);


/*--------------------------------------------------
	UINT32 K_GetCircuitLength(void)

		Returns the circuit length, 0 on sprint maps.

	Input Arguments:-

	Return:-
		The circuit length.
--------------------------------------------------*/

UINT32 K_GetCircuitLength(void);


/*--------------------------------------------------
	INT32 K_GetTrackComplexity(void)

		Returns the track complexity values. This depends
		on how many turns the map has, and is used for
		bot code to determine their rubberbanding.

	Input Arguments:-

	Return:-
		The track complexity value.
--------------------------------------------------*/

INT32 K_GetTrackComplexity(void);


/*--------------------------------------------------
	waypoint_t *K_GetClosestWaypointToMobj(mobj_t *const mobj)

		Returns the closest waypoint to an mobj

	Input Arguments:-
		mobj - mobj to get the closest waypoint of.

	Return:-
		The closest waypoint to the mobj
--------------------------------------------------*/
waypoint_t *K_GetClosestWaypointToMobj(mobj_t *const mobj);


/*--------------------------------------------------
	waypoint_t *K_GetBestWaypointForMobj(mobj_t *const mobj, waypoint_t *const hint);

		Similar to K_GetClosestWaypointToMobj, but prioritizes horizontal distance over vertical distance, and
		sight checks to ensure that the waypoint and mobj are the in same area. Can potentially return NULL if
		there are no visible waypoints.

	Input Arguments:-
		mobj - mobj to get the waypoint for.
		hint - a previously known nearby waypoint to optimize searching.

	Return:-
		The best waypoint for the mobj, or NULL if there were no matches
--------------------------------------------------*/
waypoint_t *K_GetBestWaypointForMobj(mobj_t *const mobj, waypoint_t *const hint);


/*--------------------------------------------------
	boolean K_PathfindToWaypoint(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		Use pathfinding to try and find the best route to the destination. Data is allocated into the returnpath,
		and should be freed when done with. A call to this with a path already in the returnpath will free the data
		already in there if one is found.

	Input Arguments:-
		sourcewaypoint      - The waypoint to start searching from
		destinationwaypoint - The waypoint to try and get to.
		returnpath          - The path_t that will contain the final found path
		useshortcuts        - Whether to use waypoints that are marked as being shortcuts in the search
		huntbackwards       - Goes through the waypoints backwards if true

	Return:-
		True if a path was found to the waypoint, false if there wasn't.
--------------------------------------------------*/

boolean K_PathfindToWaypoint(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards);


/*--------------------------------------------------
	boolean K_PathfindThruCircuit(
		waypoint_t *const sourcewaypoint,
		const UINT32      traveldistance,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		Tries a pathfind to the finish line waypoint, similar to K_PathfindToWaypoint, but it will continue
		until it reaches the specified distance. The final path returned will only have the waypoints up to the
		specified distance.

	Input Arguments:-
		sourcewaypoint      - The waypoint to start searching from
		traveldistance      - How far along the circuit it will try to pathfind.
		returnpath          - The path_t that will contain the final found path
		useshortcuts        - Whether to use waypoints that are marked as being shortcuts in the search
		huntbackwards       - Goes through the waypoints backwards if true

	Return:-
		True if a circuit path could be constructed, false if it couldn't.
--------------------------------------------------*/

boolean K_PathfindThruCircuit(
	waypoint_t *const sourcewaypoint,
	const UINT32      traveldistance,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards);


/*--------------------------------------------------
	boolean K_PathfindThruCircuitSpawnable(
		waypoint_t *const sourcewaypoint,
		const UINT32      traveldistance,
		path_t *const     returnpath,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		The same as K_PathfindThruCircuit, but continues until hitting a waypoint that
		can be respawned at.

	Input Arguments:-
		sourcewaypoint      - The waypoint to start searching from
		traveldistance      - How far along the circuit it will try to pathfind.
		returnpath          - The path_t that will contain the final found path
		useshortcuts        - Whether to use waypoints that are marked as being shortcuts in the search
		huntbackwards       - Goes through the waypoints backwards if true

	Return:-
		True if a circuit path could be constructed, false if it couldn't.
--------------------------------------------------*/

boolean K_PathfindThruCircuitSpawnable(
	waypoint_t *const sourcewaypoint,
	const UINT32      traveldistance,
	path_t *const     returnpath,
	const boolean     useshortcuts,
	const boolean     huntbackwards);


/*--------------------------------------------------
	waypoint_t *K_GetNextWaypointToDestination(
		waypoint_t *const sourcewaypoint,
		waypoint_t *const destinationwaypoint,
		const boolean     useshortcuts,
		const boolean     huntbackwards)

		Uses pathfinding to find the next waypoint to go to in order to get to the destination waypoint, from the source
		waypoint. If the source waypoint only has one next waypoint it will always pick that one and not do any
		pathfinding.

	Input Arguments:-
		sourcewaypoint      - The waypoint to start searching from
		destinationwaypoint - The waypoint to try and get to.
		useshortcuts        - Whether to use waypoints that are marked as being shortcuts in the search
		huntbackwards       - Goes through the waypoints backwards if true

	Return:-
		The next waypoint on the way to the destination waypoint. Returns the source waypoint if the source and
		destination are the same.
--------------------------------------------------*/

waypoint_t *K_GetNextWaypointToDestination(
	waypoint_t *const sourcewaypoint,
	waypoint_t *const destinationwaypoint,
	const boolean     useshortcuts,
	const boolean     huntbackwards);


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
	size_t K_GetWaypointHeapIndex(waypoint_t *waypoint)

		Returns the waypoint's index in the waypoint heap.

	Input Arguments:-
		waypoint - The waypoint to return the index of.

	Return:-
		The waypoint heap index, SIZE_MAX if there's an issue with the waypoint.
--------------------------------------------------*/
size_t K_GetWaypointHeapIndex(waypoint_t *waypoint);

/*--------------------------------------------------
	size_t K_GetNumWaypoints(void)

		Returns the number of waypoints that are in the heap.
		Intended for Net Archiving/Unarchiving

	Return:-
		The number of waypoints in the heap
--------------------------------------------------*/
size_t K_GetNumWaypoints(void);

/*--------------------------------------------------
	waypoint_t *K_GetWaypointFromIndex(size_t waypointindex)

		Returns the waypoint from an index to the heap.

	Input Arguments:-
		waypointindex - The index of the waypoint to get

	Return:-
		The waypoint from the heap index, NULL if the index if too high
--------------------------------------------------*/
waypoint_t *K_GetWaypointFromIndex(size_t waypointindex);


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

/*--------------------------------------------------
	void K_AdjustWaypointsParameters(void)

		Adjusts waypoint parameters after P_SpawnSpecials. This is for
		raising waypoints to an FOF, which requires that the FOF is
		already spawned.
--------------------------------------------------*/

void K_AdjustWaypointsParameters (void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
