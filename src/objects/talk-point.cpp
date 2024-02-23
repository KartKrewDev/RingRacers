// DR. ROBOTNIK'S RING RACERS
//-------------------------------
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "objects.hpp"

#include "../doomdef.h"
#include "../g_game.h"
#include "../p_spec.h"

using namespace srb2::objects;

namespace
{

struct TalkPoint : Mobj
{
	void thing_args() = delete;
	Fixed radius() const { return mobj_t::thing_args[0] * FRACUNIT; }
	bool oneshot() const { return !mobj_t::thing_args[1]; }
	bool disabled() const { return mobj_t::thing_args[2]; }

	void think()
	{
		if (disabled())
		{
			// turned off
			return;
		}

		for (UINT8 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] == false || players[i].spectator == true)
			{
				continue;
			}

			Mobj* player_mobj = static_cast<Mobj*>(players[i].mo);
			if (!Mobj::valid(player_mobj))
			{
				continue;
			}

			Fixed dist = (pos2d() - player_mobj->pos2d()).magnitude();
			if (dist < radius())
			{
				P_ActivateThingSpecial(this, player_mobj);

				if (oneshot())
				{
					remove();
					return;
				}

				break;
			}
		}
	}
};

}; // namespace

void Obj_TalkPointThink(mobj_t* mo)
{
	static_cast<TalkPoint*>(mo)->think();
}
