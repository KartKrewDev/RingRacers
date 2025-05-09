// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <optional>
#include <utility>

#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../p_mobj.h"
#include "../p_tick.h"
#include "../typedef.h"

#define shadow_follow(o) ((o)->tracer)

struct Shadow : mobj_t
{
	mobj_t* follow() const { return shadow_follow(this); }
	void follow(mobj_t* n) { P_SetTarget(&shadow_follow(this), n); }

	static Shadow* spawn(mobj_t* from)
	{
		return static_cast<Shadow*>(P_SpawnMobjFromMobj(from, 0, 0, 0, MT_SHADOW))->init(from);
	}

	bool valid() const { return !P_MobjWasRemoved(this) && !P_MobjWasRemoved(follow()); }

	std::optional<std::pair<fixed_t, pslope_t*>> z_position() const
	{
		switch (follow()->type)
		{
		case MT_HYUDORO: {
			fixed_t z;
			pslope_t* slope;

			if (Obj_HyudoroShadowZ(follow(), &z, &slope))
			{
				return {{z, slope}};
			}
			break;
		}

		default:
			break;
		}

		return {};
	}

	void destroy() { P_RemoveMobj(this); }

	void move()
	{
		whiteshadow = follow()->whiteshadow;
		shadowcolor = follow()->shadowcolor;

		P_MoveOrigin(this, follow()->x, follow()->y, P_GetMobjFeet(follow()));
	}

private:
	Shadow* init(mobj_t* from)
	{
		shadowscale = from->shadowscale;
		from->shadowscale = 0;

		follow(from);

		return this;
	}
};

mobj_t* Obj_SpawnFakeShadow(mobj_t* from)
{
	return Shadow::spawn(from);
}

void Obj_FakeShadowThink(mobj_t* shadow)
{
	auto x = static_cast<Shadow*>(shadow);

	if (!x->valid())
	{
		x->destroy();
		return;
	}

	x->move();
}

boolean Obj_FakeShadowZ(const mobj_t* shadow, fixed_t* return_z, pslope_t** return_slope)
{
	auto x = static_cast<const Shadow*>(shadow);

	if (!x->valid())
	{
		return false;
	}

	auto pair = x->z_position();

	if (!pair)
	{
		return false;
	}

	auto [z, slope] = *pair;

	*return_z = z;
	*return_slope = slope;

	return true;
}
