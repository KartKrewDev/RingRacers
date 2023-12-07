// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../math/fixed.hpp"
#include "../math/vec.hpp"
#include "../mobj.hpp"

#include "../doomdef.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_easing.h"
#include "../p_local.h"
#include "../s_sound.h"

using srb2::Mobj;
using srb2::math::Fixed;
using srb2::math::Vec2;

namespace
{

struct Broly : Mobj
{
	/* An object may not be visible on the same tic:
	   1) that it spawned
	   2) that it cycles to the next state */
	static constexpr int kBufferTics = 2;

	void extravalue1() = delete;
	tic_t duration() const { return mobj_t::extravalue1; }
	void duration(tic_t n) { mobj_t::extravalue1 = n; }

	void threshold() = delete;
	void extravalue2() = delete;
	Vec2<Fixed> size() const { return {mobj_t::threshold, mobj_t::extravalue2}; }
	void size(const Vec2<Fixed>& n)
	{
		mobj_t::threshold = n.x;
		mobj_t::extravalue2 = n.y;
	}

	bool valid() const { return duration(); }

	tic_t remaining() const { return tics - kBufferTics; }

	Fixed linear() const { return (remaining() * FRACUNIT) / duration(); }

	static Broly* spawn(Mobj* source, tic_t duration, const Vec2<Fixed>& size)
	{
		if (duration == 0)
		{
			return nullptr;
		}

		Broly* x = source->spawn_from<Broly>({}, MT_BROLY);

		x->target(source);

		// Shrink into center of source object.
		x->z = source->center().z - (x->height / 2);

		x->colorized = true;
		x->color = source->color;
		x->mobj_t::hitlag = 0; // do not copy source hitlag

		x->size(size);
		x->duration(duration);

		x->tics = (duration + kBufferTics);

		K_ReduceVFXForEveryone(x);

		x->voice(sfx_cdfm74);

		return x;
	}

	bool think()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		const Fixed center = z + (height / 2);
		const Vec2<Fixed> v = size();

		scale(Easing_OutSine(linear(), v.y, v.x));
		z = center - (height / 2);

		return true;
	}
};

}; // namespace

mobj_t *
Obj_SpawnBrolyKi
(		mobj_t * source,
		tic_t duration)
{
	return Broly::spawn(static_cast<Mobj*>(source), duration, {64 * mapobjectscale, 0});
}

boolean
Obj_BrolyKiThink (mobj_t *x)
{
	return static_cast<Broly*>(x)->think();
}
