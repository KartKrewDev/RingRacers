// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

// Original Lua script by Sal
// Hardcoded by jartha

// Faucet & rain effect
// I actually thought these were a hazard until I went back and looked at the original game...
// I just never got hit by 'em, so I assumed they were hazardous!
// In that same research session, I noticed the tiny droplet effect at the only "exposed" sky area...

#include "../mobj.hpp"

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../m_fixed.h"
#include "../m_random.h"
#include "../tables.h"
#include "../p_slopes.h"
#include "../r_defs.h"
#include "../r_main.h"
#include "../r_sky.h"

using srb2::Mobj;

namespace
{

struct Drip : Mobj
{
	void splatter()
	{
		vfx(0);
		vfx(90);
		vfx(180);
		vfx(270);
	}

private:
	void vfx(int deg)
	{
		Mobj* h = spawn<Mobj>(pos(), MT_EMFAUCET_PARTICLE);
		h->angle = (deg + P_RandomKey(PR_DECORATION, 90)) * ANG1;
		h->instathrust(h->angle, 8 * h->scale());
		h->momz = h->flip(4 * h->scale());
	}
};

struct Faucet : Mobj
{
	void tick()
	{
		if (leveltime % (4*TICRATE) == 0)
		{
			spawn<Drip>(pos(), MT_EMFAUCET_DRIP);
		}
	}
};

struct RainGenerator : Mobj
{
	void tick()
	{
		if (leveltime % 2)
		{
			return;
		}

		for (int i = 0; i < 16; ++i)
		{
			if (rain())
			{
				break;
			}
		}
	}

private:
	bool rain()
	{
		auto rng = [](int x, int y) { return P_RandomRange(PR_DECORATION, x, y); };

		const fixed_t x = this->x + (rng(-15, 15) * 256 * scale());
		const fixed_t y = this->y + (rng(-15, 15) * 256 * scale());

		const sector_t* sector = R_PointInSubsector(x, y)->sector;

		if (sector->ceilingpic != skyflatnum)
		{
			return false;
		}

		Mobj* h = spawn<Mobj>({x, y, P_GetSectorCeilingZAt(sector, x, y)}, MT_EMFAUCET_PARTICLE);
		h->angle = rng(0, 359) * ANG1;
		h->instathrust(h->angle, 4 * h->scale());

		return true;
	}
};

}; // namespace

void Obj_EMZFaucetThink(mobj_t* mo)
{
	static_cast<Faucet*>(mo)->tick();
}

void Obj_EMZDripDeath(mobj_t* mo)
{
	static_cast<Drip*>(mo)->splatter();
}

void Obj_EMZRainGenerator(mobj_t* mo)
{
	static_cast<RainGenerator*>(mo)->tick();
}
