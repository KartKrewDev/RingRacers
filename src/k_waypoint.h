#ifndef __K_WAYPOINT__
#define __K_WAYPOINT__

#include "doomdef.h"
#include "p_mobj.h"

typedef struct waypoint_s
{
	mobj_t             *mobj;
	UINT32              id;
	struct waypoint_s **nextwaypoints;
	struct waypoint_s **prevwaypoints;
	fixed_t            *nextwaypointdistances;
	fixed_t            *prevwaypointdistances;
	UINT32              numnextwaypoints;
	UINT32              numprevwaypoints;
} waypoint_t;


// AVAILABLE FOR LUA
waypoint_t *K_SearchWaypointsForMobj(mobj_t * const mobj);

// NOT AVAILABLE FOR LUA
void K_SetupWaypointList(void);

#endif