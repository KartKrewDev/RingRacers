// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

// Original Lua script by Lat
// Hardcoded by jartha

#include <algorithm>
#include <array>
#include <tuple>

#include "../math/fixed.hpp"
#include "../math/line_segment.hpp"
#include "../math/vec.hpp"
#include "../mobj.hpp"
#include "../mobj_list_view.hpp"

#include "../d_player.h"
#include "../doomtype.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../p_maputl.h"
#include "../p_pspr.h"
#include "../r_defs.h"

using srb2::math::Fixed;
using srb2::math::LineSegment;
using srb2::math::Vec2;
using srb2::Mobj;
using srb2::MobjListView;

namespace
{

using frame_layout = std::array<int, 6>; // -z, +z, -x, -y, +x, +y

struct SA2CrateConfig
{
	static constexpr spritenum_t kSprite = SPR_SABX;
	static constexpr frame_layout kFrames = {3, 2, 0, 0, 0, 0};
	static constexpr statenum_t kDefaultDebris = S_SA2_CRATE_DEBRIS;
};

struct Graphic : Mobj
{
	void hnext() = delete;
	Graphic* next() const { return Mobj::hnext<Graphic>(); }
	void next(Graphic* n) { Mobj::hnext(n); }

	Graphic* dress(spritenum_t sprite, UINT32 frame)
	{
		this->sprite = sprite;
		this->frame = frame;
		this->renderflags |= RF_NOSPLATBILLBOARD;
		return this;
	}

	Graphic* xy(fixed_t x, fixed_t y)
	{
		this->sproff2d({x, y});
		return this;
	}

	Graphic* z(fixed_t z)
	{
		this->sprzoff(z);
		return this;
	}

	Graphic* turn(angle_t angle)
	{
		this->angle = angle;
		return this;
	}
};

struct Side : Graphic
{
	bool valid() const { return Mobj::valid() && Mobj::valid(owner()); }

	void think()
	{
		if (!valid())
		{
			remove();
			return;
		}

		move_origin(owner());
		renderflags = owner()->renderflags;
	}
};

struct Toucher : Mobj
{
	bool boosting() const { return player && (player->sneakertimer || K_PlayerCanPunt(player)); }
};

struct AnyBox : Graphic
{
	template <typename F>
	bool visit(F&& visitor);

	void update()
	{
		visit([](auto box) { box->mobj_t::z++; });
	}
};

template <class Config>
struct Box : AnyBox
{
	static constexpr Fixed kIntendedSize = 128*FRACUNIT;
	static constexpr Vec2<Fixed> kScrunch = {4*FRACUNIT/5, 6*FRACUNIT/5};

	void extravalue1() = delete;
	statenum_t debris_state() const { return static_cast<statenum_t>(mobj_t::extravalue1); }
	void debris_state(statenum_t n) { mobj_t::extravalue1 = n; }

	auto gfx() { return MobjListView(static_cast<Graphic*>(this), [](Graphic* g) { return g->next(); }); }

	void init()
	{
		scale(scale() * (kIntendedSize / Fixed {info->height}));

		Graphic* node = this;
		int i = 0;
		auto dress = [&](Graphic* g, UINT32 ff) { return g->dress(Config::kSprite, Config::kFrames[i++] | ff); };
		auto side = [&](UINT32 ff)
		{
			Side* side = spawn_from<Side>({}, MT_BOX_SIDE);
			side->owner(this);
			node->next(side); // link
			node = side;
			return dress(side, ff);
		};

		dress(this, FF_FLOORSPRITE); // bottom (me)
		side(FF_FLOORSPRITE)->z(height); // top

		// sides
		side(FF_PAPERSPRITE)->xy(-radius, 0)->turn(ANGLE_270);
		side(FF_PAPERSPRITE)->xy(0, -radius);
		side(FF_PAPERSPRITE)->xy(+radius, 0)->turn(ANGLE_90);
		side(FF_PAPERSPRITE)->xy(0, +radius)->turn(ANGLE_180);

		debris_state(Config::kDefaultDebris);
	}

	bool think()
	{
		if (fuse)
		{
			fuse--;
			renderflags ^= RF_DONTDRAW;

			if (!fuse)
			{
				update_nearby();
				remove();
				return false;
			}
		}

		return true;
	}

	void touch(Toucher* toucher)
	{
		if (fuse)
		{
			return;
		}

		P_DamageMobj(this, toucher, nullptr, 1, DMG_NORMAL);

		if (!toucher->boosting())
		{
			toucher->solid_bounce(this);
		}
	}

	bool damage_valid(const Mobj* inflictor) const { return !fuse && Mobj::valid(inflictor); }

	void damage(Mobj* inflictor)
	{
		if (!damage_valid(inflictor))
		{
			return;
		}

		inflictor->hitlag(3);
		fuse = 10;

		// scrunch crate sides
		for (Graphic* g : gfx())
		{
			if (g->frame & FF_PAPERSPRITE)
			{
				g->frame++;
				g->spritescale(kScrunch);
			}
			else
			{
				g->spritescale(kScrunch.x);
			}

			// reset interp
			g->mobj_t::old_spritexscale = g->spritexscale();
			g->mobj_t::old_spriteyscale = g->spriteyscale();

			g->sproff2d(g->sproff2d() * kScrunch.x);
			g->sprzoff(g->sprzoff() * kScrunch.y);
		}

		debris(inflictor);
		update_nearby();
	}

private:
	void debris(Mobj* inflictor)
	{
		if (debris_state() >= NUMSTATES)
		{
			return;
		}

		auto rng = [&](int x, int y) { return P_RandomRange(PR_DECORATION, x, y) * scale(); };
		auto rng_xyz = [&](int x) { return std::tuple(rng(-x, x), rng(-x, x), rng(0, x)); };

		auto spawn = [&]
		{
			auto [x, y, z] = rng_xyz(info->height / FRACUNIT);
			Mobj* p = spawn_from<Mobj>({x, y, z}, MT_BOX_DEBRIS);

			p->scale_between(scale() / 2, scale());
			p->state(debris_state());

			std::tie(x, y, z) = rng_xyz(4);

			p->momx = (inflictor->momx / 8) + x;
			p->momy = (inflictor->momy / 8) + y;
			p->momz = (Fixed::hypot(inflictor->momx, inflictor->momy) / 4) + z;
		};

		spawn();
		spawn();
		spawn();
		spawn();
		spawn();
		spawn();
	}

	void update_nearby() const
	{
		LineSegment<Fixed> search = aabb();
		Vec2<Fixed> org{bmaporgx, bmaporgy};

		search.a -= org + MAXRADIUS;
		search.b -= org - MAXRADIUS;

		search.a.x = static_cast<UINT32>(search.a.x) >> MAPBLOCKSHIFT;
		search.b.x = static_cast<UINT32>(search.b.x) >> MAPBLOCKSHIFT;
		search.a.y = static_cast<UINT32>(search.a.y) >> MAPBLOCKSHIFT;
		search.b.y = static_cast<UINT32>(search.b.y) >> MAPBLOCKSHIFT;

		BMBOUNDFIX(search.a.x, search.b.x, search.b.x, search.b.y);

		for (INT32 bx = search.a.x; bx <= search.b.x; ++bx)
		{
			for (INT32 by = search.a.y; by <= search.b.y; ++by)
			{
				P_BlockThingsIterator(
					bx,
					by,
					[](mobj_t* thing)
					{
						static_cast<AnyBox*>(thing)->update();
						return BMIT_CONTINUE;
					}
				);
			}
		}
	}
};

struct Crate : Box<SA2CrateConfig>
{
	static constexpr int kMetalFrameStart = 8;

	void thing_args() = delete;
	bool metal() const { return mobj_t::thing_args[0]; }

	void init()
	{
		Box::init();

		if (metal())
		{
			for (Graphic* g : gfx())
			{
				g->frame += kMetalFrameStart;
			}

			debris_state(S_SA2_CRATE_DEBRIS_METAL);
		}
	}

	void damage(Toucher* inflictor)
	{
		if (!Box::damage_valid(inflictor))
		{
			return;
		}

		if (metal() && !inflictor->boosting())
		{
			return;
		}

		Box::damage(inflictor);
	}
};

template <typename F>
bool AnyBox::visit(F&& visitor)
{
	switch (type)
	{
	case MT_SA2_CRATE:
		visitor(static_cast<Crate*>(this));
		break;

	default:
		return false;
	}

	return true;
}

}; // namespace

void Obj_BoxSideThink(mobj_t* mobj)
{
	static_cast<Side*>(mobj)->think();
}

void Obj_TryCrateInit(mobj_t* mobj)
{
	static_cast<AnyBox*>(mobj)->visit([&](auto box) { box->init(); });
}

boolean Obj_TryCrateThink(mobj_t* mobj)
{
	bool c = false;
	static_cast<AnyBox*>(mobj)->visit([&](auto box) { c = box->think(); });
	return c;
}

void Obj_TryCrateTouch(mobj_t* special, mobj_t* toucher)
{
	static_cast<AnyBox*>(special)->visit([&](auto box) { box->touch(static_cast<Toucher*>(toucher)); });
}

void Obj_TryCrateDamage(mobj_t* target, mobj_t* inflictor)
{
	static_cast<AnyBox*>(target)->visit([&](auto box) { box->damage(static_cast<Toucher*>(inflictor)); });
}
