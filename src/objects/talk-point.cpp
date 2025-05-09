// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <array>

#include "objects.hpp"

#include "../doomdef.h"
#include "../g_game.h"
#include "../p_spec.h"
#include "../tables.h"

using namespace srb2::objects;

namespace
{

Vec2<Fixed> angle_vector(angle_t x)
{
	return Vec2<Fixed> {FCOS(x), FSIN(x)};
}

struct TalkPoint : Mobj
{
	static constexpr angle_t kSpinSpeed = ANGLE_11hh;
	static constexpr tic_t kCollectDuration = 12;

	void thing_args() = delete;
	Fixed radius() const { return mobj_t::thing_args[0] * FRACUNIT; }
	bool oneshot() const { return !mobj_t::thing_args[1]; }
	bool disabled() const { return mobj_t::thing_args[2]; }
	bool invisible() const { return mobj_t::thing_args[3]; }

	void extravalue1() = delete;
	tic_t collect() const { return mobj_t::extravalue1; }
	void collect(tic_t n) { mobj_t::extravalue1 = n; }

	// This value scales up as the radius gets larger
	Fixed scaling() const { return std::min<Fixed>(FRACUNIT, Fixed {320 * mapobjectscale} / radius()); }

	void init();

	void think()
	{
		if (disabled())
		{
			// turned off
			return;
		}

		if (collect() > 0)
		{
			collect(collect() - 1);

			if (collect() == 0)
			{
				remove();
				return;
			}

			// Collect animation: stretch upward, spin faster, fade out
			spriteyscale(kCollectDuration * FRACUNIT / collect());
			angle += kCollectDuration * kSpinSpeed / collect();
			renderflags = (renderflags & ~RF_TRANSMASK) |
				((9 - (collect() * 9 / kCollectDuration)) << RF_TRANSSHIFT);
			return;
		}

		angle += kSpinSpeed;

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
					// Collect animation: appear brighter during fade-out
					renderflags |= RF_FULLBRIGHT;
					collect(kCollectDuration);
				}

				if (!invisible() && P_IsDisplayPlayer(&players[i]))
				{
					S_StartSound(nullptr, sfx_hint);
				}

				break;
			}
		}
	}
};

struct Orb : Mobj
{
	static constexpr std::array<skincolornum_t, 10> kColors = {
		SKINCOLOR_NONE,
		SKINCOLOR_JAWZ,
		SKINCOLOR_AQUAMARINE,
		SKINCOLOR_LIME,
		SKINCOLOR_BANANA,
		SKINCOLOR_CREAMSICLE,
		SKINCOLOR_DAWN,
		SKINCOLOR_TAFFY,
		SKINCOLOR_VIOLET,
		SKINCOLOR_RUST,
	};

	void target() = delete;
	TalkPoint* origin() const { return Mobj::target<TalkPoint>(); }
	void origin(TalkPoint* n) { Mobj::target(n); }

	void extravalue1() = delete;
	angle_t speed() const { return mobj_t::extravalue1; }
	void speed(angle_t n) { mobj_t::extravalue1 = n; }

	bool valid() const { return Mobj::valid(origin()) && origin()->valid(); }

	static Orb* spawn(TalkPoint* origin, UINT8 index, angle_t angle)
	{
		Orb* orb = Mobj::spawn<Orb>({origin->pos2d() + radius_vector(origin, angle), origin->z}, MT_SCRIPT_THING_ORB);
		orb->origin(origin);

		orb->color = kColors[index % kColors.size()]; // cycle colors
		orb->colorized = true;

		orb->angle = angle;
		orb->sprzoff(20 * mapobjectscale); // roughly eye level with the player
		orb->speed(FixedAngle(Fixed {45*FRACUNIT/8} * origin->scaling())); // slower with larger radius

		return orb;
	}

	void think()
	{
		if (!valid())
		{
			remove();
			return;
		}

		// rotate
		angle += speed();
		move_origin({origin()->pos2d() + radius_vector(), origin()->z});

		// bob
		spriteyoffset(8 * FSIN(angle + (leveltime * speed())));
	}

private:
	static vec2 radius_vector(TalkPoint* origin, angle_t angle)
	{
		// Collect animation: orbs fly outward
		return angle_vector(angle) * origin->radius() * origin->spriteyscale();
	}

	vec2 radius_vector() const { return radius_vector(origin(), angle); }
};

void TalkPoint::init()
{
	if (invisible())
	{
		renderflags |= RF_DONTDRAW;
		return;
	}

	if (scaling() == 0)
	{
		return;
	}

	// Orbs get spaced further apart as the radius increases
	fixed_t orb_rad = Fixed {88 * mapobjectscale} / scaling();
	if (orb_rad == 0)
	{
		return;
	}

	// Spawn more orbs within a larger radius
	INT32 count = (Fixed {M_TAU_FIXED} * radius()) / orb_rad;
	if (count == 0)
	{
		return;
	}

	angle_t step = ANGLE_MAX / count;
	angle_t angle = 0;
	for (INT32 i = 0; i < count; ++i)
	{
		Orb::spawn(this, i, angle);
		angle += step;
	}
}

}; // namespace

void Obj_TalkPointInit(mobj_t* mo)
{
	static_cast<TalkPoint*>(mo)->init();
}

void Obj_TalkPointThink(mobj_t* mo)
{
	static_cast<TalkPoint*>(mo)->think();
}

void Obj_TalkPointOrbThink(mobj_t* mo)
{
	static_cast<Orb*>(mo)->think();
}
