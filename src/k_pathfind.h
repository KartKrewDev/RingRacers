#ifndef __K_PATHFIND__
#define __K_PATHFIND__

#include "doomtype.h"

// function pointer for returning a node's connected node data
// should return a pointer to an array of pointers to the data, as arguments takes a node's data and a pointer that the
// number of connected nodes should be placed into
typedef void**(*getconnectednodesfunc)(void*, size_t*);

// function pointer for getting the list of connected node costs/distances
typedef UINT32*(*getnodeconnectioncostsfunc)(void*);

// function pointer for getting a heuristic between 2 nodes from their base data
typedef UINT32(*getnodeheuristicfunc)(void*, void*);

// function pointer for getting if a node is traversable from its base data
typedef boolean(*getnodetraversablefunc)(void*);


// A pathfindnode contains information about a node from the pathfinding
// heapindex is only used within the pathfinding algorithm itself, and is always 0 after it is completed
typedef struct pathfindnode_s {
	size_t heapindex;     // The index in the openset binary heap. Only valid while the node is in the openset.
	void   *nodedata;
	struct pathfindnode_s *camefrom; // should eventually be the most efficient predecessor node
	UINT32     gscore;    // The accumulated distance from the start to this node
	UINT32     hscore;    // The heuristic from this node to the goal
} pathfindnode_t;

// Contains the final created path after pathfinding is completed
typedef struct path_s {
	size_t numnodes;
	struct pathfindnode_s *array;
	UINT32 totaldist;
} path_t;

// Contains info about the pathfinding used to setup the algorithm
// (e.g. the base capacities of the dynamically allocated arrays)
// should be setup by the caller before starting pathfinding
// base capacities will be 8 if they aren't setup, missing callback functions will cause an error.
// Can be accessed after the pathfinding is complete to get the final capacities of them
typedef struct pathfindsetup_s {
	size_t opensetcapacity;
	size_t closedsetcapacity;
	size_t nodesarraycapacity;
	void   *startnodedata;
	void   *endnodedata;
	getconnectednodesfunc getconnectednodes;
	getnodeconnectioncostsfunc getconnectioncosts;
	getnodeheuristicfunc getheuristic;
	getnodetraversablefunc gettraversable;
} pathfindsetup_t;


/*--------------------------------------------------
	boolean K_PathfindAStar(path_t *const path, pathfindsetup_t *const pathfindsetup);

		From a source waypoint and destination waypoint, find the best path between them using the A* algorithm.

	Input Arguments:-
		path          - The return location of the found path
		pathfindsetup - The information regarding pathfinding setup, see pathfindsetup_t

	Return:-
		True if a path was found between source and destination, false otherwise.
--------------------------------------------------*/
boolean K_PathfindAStar(path_t *const path, pathfindsetup_t *const pathfindsetup);

#endif
