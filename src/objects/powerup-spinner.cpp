// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>

#include "objects.hpp"

#include "../m_easing.h"
#include "../m_fixed.h"
#include "../tables.h"

using namespace srb2::objects;

namespace
{

struct Spinner : Mobj
{
	static constexpr int kDuration = 40;

	void extravalue1() = delete;
	INT32 powerup() const { return mobj_t::extravalue1; }
	void powerup(INT32 n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	INT32 duration() const { return mobj_t::extravalue2; }
	void duration(INT32 n) { mobj_t::extravalue2 = n; }

	static void spawn(Mobj* source, INT32 powerup, tic_t duration)
	{
		Spinner* x = Mobj::spawn<Spinner>(source->pos(), MT_GOTPOWERUP);
		K_UpdateMobjItemOverlay(x, powerup, 1);
		x->frame |= FF_PAPERSPRITE | FF_ADD;
		x->fuse = duration;
		x->powerup(powerup);
		x->duration(duration);
	}

	void think()
	{
		fixed_t f = FRACUNIT - std::clamp(fuse, 0, duration()) * FRACUNIT / std::max(duration(), 1);

		angle += Easing_InQuad(f, ANGLE_11hh, ANGLE_45);
		renderflags = (renderflags & ~RF_TRANSMASK) | (Easing_Linear(f, 0, 9) << RF_TRANSSHIFT);
		spritescale({Easing_Linear(f, 4*FRACUNIT, FRACUNIT/4), Easing_Linear(f, FRACUNIT, 6*FRACUNIT)});

		if (--fuse <= 0)
		{
			remove();
		}
	}
};

}; // namespace

void Obj_SpawnPowerUpSpinner(mobj_t *source, INT32 powerup, tic_t duration)
{
	Spinner::spawn(static_cast<Mobj*>(source), powerup, duration);
}

void Obj_TickPowerUpSpinner(mobj_t *mobj)
{
	static_cast<Spinner*>(mobj)->think();
}
