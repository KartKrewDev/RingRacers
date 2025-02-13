// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

// Original Lua script by Lat
// Hardcoded by jartha

#include <algorithm>

#include "../math/fixed.hpp"
#include "../math/vec.hpp"
#include "../mobj.hpp"
#include "../mobj_list_view.hpp"

#include "../doomstat.h"
#include "../doomtype.h"
#include "../info.h"
#include "../k_objects.h"
#include "../tables.h"

using srb2::math::Fixed;
using srb2::math::Vec2;
using srb2::Mobj;
using srb2::MobjListView;

namespace
{

Vec2<Fixed> angle_vector(angle_t x)
{
	return Vec2<Fixed> {FCOS(x), FSIN(x)};
}

struct Spear : Mobj
{
	enum Mode
	{
		kWait,
		kShake,
		kPush,
		kPull,
		kNumModes,
	};

	static constexpr tic_t kWaitTimes[kNumModes] = {
		TICRATE,
		TICRATE/2,
		TICRATE/2,
		TICRATE,
	};

	void extravalue1() = delete;
	int mode() const { return mobj_t::extravalue1; }
	void mode(int n)
	{
		mobj_t::extravalue1 = n;
		timer(kWaitTimes[n]);
	}

	void extravalue2() = delete;
	tic_t timer() const { return mobj_t::extravalue2; }
	void timer(tic_t n) { mobj_t::extravalue2 = n; }

	void threshold() = delete;
	Fixed dist() const { return mobj_t::threshold; }
	void dist(Fixed n) { mobj_t::threshold = n; }

	void thing_args() = delete;
	bool delayed_start() const { return mobj_t::thing_args[0]; }

	void init()
	{
		mode(kWait);

		if (delayed_start())
		{
			timer(timer() + TICRATE*3/2);
		}

		auto piece = [&](statenum_t state)
		{
			Mobj* vis = spawn_from<Mobj>({}, MT_SPEARVISUAL);
			vis->state(state);
			return vis;
		};

		Vec2<Fixed> v = angle_vector(angle) * scale();
		auto divider = [&](statenum_t state, int offset)
		{
			Mobj* vis = piece(state);
			vis->angle = angle - ANGLE_90;
			vis->sproff2d(v * offset);
			return vis;
		};

		Mobj* head = this;
		auto link = [&](Mobj* vis)
		{
			vis->punt_ref(this);
			head->hnext(vis);
			head = vis;
			return vis;
		};

		Mobj* wall = divider(S_SPEAR_WALL, 0); // never moves

		set_origin({pos2d() + (angle_vector(angle) * Fixed {radius}), z});

		link(divider(S_SPEAR_HILT_BACK, 26));
		Mobj* front = link(divider(S_SPEAR_HILT_FRONT, 34));

		Mobj* tip = piece(S_SPEAR_TIP);
		tip->angle = angle;
		link(tip);

		// Whether you use a positive or negative offset
		// depends on how the sprite would originally be
		// sorted...
		this->linkdraw(wall, -5); // this sorts the rod behind the wall plate
		tip->linkdraw(front, -5); // this sorts the tip IN FRONT of the rod
	}

	void think()
	{
		Vec2<Fixed> p = pos2d() - vector();
		dist(new_dist());
		Mobj::PosArg mpos{p + vector(), z};

		move_origin(mpos);
		for (Mobj* vis : MobjListView(hnext(), [](Mobj* vis) { return vis->hnext(); }))
		{
			vis->move_origin(mpos);
		}

		timer(timer() - 1);
		if (!timer())
		{
			mode((mode() + 1) % kNumModes);
		}
	}

private:
	Fixed new_dist() const
	{
		static constexpr int kMinDist = -96;
		static constexpr int kMaxDist = 0;

		switch (mode())
		{
		default:
			return kMinDist * scale();
		case kShake:
			return (leveltime & 1 ? kMinDist : kMinDist + 4) * scale();
		case kPush:
			return std::min(scale() * kMaxDist, dist() + (16 * scale()));
		case kPull:
			return std::max(scale() * kMinDist, dist() - (4 * scale()));
		}
	}

	Vec2<Fixed> vector() const { return angle_vector(angle) * dist(); }
};

}; // namespace

void Obj_SpearInit(mobj_t* mobj)
{
	static_cast<Spear*>(mobj)->init();
}

void Obj_SpearThink(mobj_t* mobj)
{
	static_cast<Spear*>(mobj)->think();
}
