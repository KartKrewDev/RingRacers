// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "objects.hpp"

#include "../tables.h"

using namespace srb2::objects;

namespace
{

struct Shield : Mobj
{
	void target() = delete;
	Mobj* follow() const { return Mobj::target(); }
	void follow(Mobj* n) { Mobj::target(n); }

	player_t* player() const { return follow()->player; }

	bool valid() const { return Mobj::valid(follow()) && player(); }
};

struct Visual : Mobj
{
	void target() = delete;
	Shield* shield() const { return Mobj::target<Shield>(); }
	void shield(Shield* n) { Mobj::target(n); }

	bool valid() const { return Mobj::valid(shield()) && shield()->valid(); }

	static void spawn
	(	Shield* shield,
		statenum_t state,
		int offset)
	{
		if (!shield->valid())
			return;

		Visual* x = Mobj::spawn<Visual>(shield->pos(), MT_FLAMESHIELD_VISUAL);
		x->scale(5 * shield->follow()->scale() / 4);
		x->state(state);
		x->shield(shield);
		x->linkdraw(shield->follow(), offset);
	}

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		move_origin(shield()->pos());
		scale(5 * shield()->follow()->scale() / 4);

		renderflags = (renderflags & ~RF_DONTDRAW) |
			(shield()->state()->num() == S_INVISIBLE ? 0 : RF_DONTDRAW);

		return true;
	}
};

}; // namespace

void Obj_SpawnFlameShieldVisuals(mobj_t *shield)
{
	Visual::spawn(static_cast<Shield*>(shield), S_FLMA1, 1);
	Visual::spawn(static_cast<Shield*>(shield), S_FLMB1, -1);
}

boolean Obj_TickFlameShieldVisual(mobj_t *mobj)
{
	return static_cast<Visual*>(mobj)->tick();
}
