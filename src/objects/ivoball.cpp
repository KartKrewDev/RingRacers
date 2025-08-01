// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

//
// CREDITS
// Original Lua script by Callmore
// Edits by Ivo, Angular and Sal
// Hardcoded by jartha
//

#include <algorithm>

#include "../math/fixed.hpp"
#include "../math/vec.hpp"
#include "../mobj.hpp"

#include "../d_player.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../r_defs.h"
#include "../s_sound.h"
#include "../sounds.h"
#include "../tables.h"

using srb2::Mobj;
using srb2::math::Fixed;
using srb2::math::Vec2;

namespace
{

Vec2<Fixed> angle_vector(angle_t x)
{
	return Vec2<Fixed> {FCOS(x), FSIN(x)};
}

struct IvoBall : Mobj
{
	static constexpr tic_t kCooldown = TICRATE*2;
	static constexpr tic_t kFlashTime = TICRATE/2;
	static constexpr Fixed kRippleFactor = 128*FRACUNIT/3;
	static constexpr Fixed kBobHeight = 8*FRACUNIT;
	static constexpr tic_t kBobTime = kFlashTime * 16;
	static constexpr int kFloat = 24;

	void extravalue1() = delete;
	tic_t timer() const { return mobj_t::extravalue1; }
	void timer(tic_t n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	fixed_t offset() const { return mobj_t::extravalue2; }
	void offset(fixed_t n) { mobj_t::extravalue2 = n; }

	void init()
	{
		Fixed wave{(x / mapobjectscale) + (y / mapobjectscale)};
		offset(wave / kRippleFactor);
		color = SKINCOLOR_TANGERINE;
		sprzoff(kFloat * mapobjectscale);
	}

	void think()
	{
		if (timer())
		{
			timer(timer() - 1);

			if (timer() == 0)
			{
				renderflags &= ~RF_DONTDRAW;
			}
		}

		fixed_t ballTimer = leveltime + offset();
		Fixed bob = kBobHeight * Fixed {FSIN((M_TAU_FIXED * kBobTime) * ballTimer)};
		spriteyoffset(bob);

		colorized = !((ballTimer / kFlashTime) & 1);
	}

	void touch(Mobj* toucher)
	{
		if (timer())
		{
			return;
		}

		if (!P_CanPickupItem(toucher->player, PICKUP_RINGORSPHERE))
		{
			return;
		}

		renderflags |= RF_DONTDRAW;
		timer(kCooldown);

		toucher->player->ringboost += 10;
		K_AwardPlayerRings(toucher->player, 1, false);

		if (P_IsDisplayPlayer(toucher->player))
		{
			S_StartSoundAtVolume(nullptr, sfx_ivobal, 160);
		}
	}
};

struct PatrolIvoBall : IvoBall
{
	void init()
	{
		Vec2<Fixed> v = angle_vector(angle) * Fixed {info->speed} * Fixed {mapobjectscale};
		momx = -v.x;
		momy = v.y;

		IvoBall::init();
	}

	void think()
	{
		if (!P_TryMove(this, x + momx, y + momy, true, nullptr))
		{
			angle += ANGLE_180;
			momx = -momx;
			momy = -momy;
		}

		IvoBall::think();
	}
};

}; // namespace

void Obj_IvoBallInit(mobj_t* mobj)
{
	static_cast<IvoBall*>(mobj)->init();
}

void Obj_IvoBallThink(mobj_t* mobj)
{
	static_cast<IvoBall*>(mobj)->think();
}

void Obj_IvoBallTouch(mobj_t* special, mobj_t* toucher)
{
	static_cast<IvoBall*>(special)->touch(static_cast<Mobj*>(toucher));
}

void Obj_PatrolIvoBallInit(mobj_t* mobj)
{
	static_cast<PatrolIvoBall*>(mobj)->init();
}

void Obj_PatrolIvoBallThink(mobj_t* mobj)
{
	static_cast<PatrolIvoBall*>(mobj)->think();
}

void Obj_PatrolIvoBallTouch(mobj_t* special, mobj_t* toucher)
{
	static_cast<PatrolIvoBall*>(special)->touch(static_cast<Mobj*>(toucher));
}
