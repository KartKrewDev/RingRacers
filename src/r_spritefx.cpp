// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Special effects for sprite rendering

#include "d_player.h"
#include "p_tick.h"
#include "r_things.h"

INT32 R_ThingLightLevel(mobj_t* thing)
{
	INT32 lightlevel = thing->lightlevel;

	player_t* player = thing->player;

	if (player)
	{
		if (player->instaShieldCooldown && (player->rings <= 0) && (leveltime & 1))
		{
			// Darken on every other frame of instawhip cooldown
			lightlevel -= 128;
		}
	}

	return lightlevel;
}
