// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "broly.hpp"

#include "../doomstat.h"
#include "../k_kart.h"
#include "../sounds.h"

using namespace srb2::objects;

mobj_t *
Obj_SpawnCustomBrolyKi
(		mobj_t * source,
		tic_t duration,
		fixed_t start,
		fixed_t end)
{
	Broly* x = Broly::spawn<Broly>(static_cast<Mobj*>(source), duration, {start, end});

	if (!x)
	{
		return nullptr;
	}

	x->colorized = true;
	x->color = source->color;

	K_ReduceVFXForEveryone(x);
	x->voice(sfx_cdfm74);

	return x;
}

mobj_t *
Obj_SpawnBrolyKi
(		mobj_t * source,
		tic_t duration)
{
	return Obj_SpawnCustomBrolyKi(source, duration, 64 * mapobjectscale, 0);
}

boolean
Obj_BrolyKiThink (mobj_t *x)
{
	return static_cast<Broly*>(x)->think();
}
