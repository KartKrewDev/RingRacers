// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Special effects for sprite rendering

#include "r_things.h"

INT32 R_ThingLightLevel(mobj_t* thing)
{
	INT32 lightlevel = thing->lightlevel;

	return lightlevel;
}
