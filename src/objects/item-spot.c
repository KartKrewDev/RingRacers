// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../m_fixed.h"
#include "../k_objects.h"
#include "../k_battle.h"
#include "../p_tick.h"
#include "../p_local.h"

#define spot_monitor(o) ((o)->target)
#define spot_cool(o) ((o)->threshold)

boolean
Obj_ItemSpotIsAvailable (const mobj_t *spot)
{
	return P_MobjWasRemoved(spot_monitor(spot)) &&
		(leveltime - spot_cool(spot)) > BATTLE_SPAWN_INTERVAL;
}

void
Obj_ItemSpotAssignMonitor
(		mobj_t * spot,
		mobj_t * monitor)
{
	P_SetTarget(&spot_monitor(spot), monitor);
	Obj_MonitorSetItemSpot(monitor, spot);
}

void
Obj_ItemSpotUpdate (mobj_t *spot)
{
	if (P_MobjWasRemoved(spot_monitor(spot)) ||
			spot_monitor(spot)->health <= 0)
	{
		spot_cool(spot) = leveltime;
	}
}
