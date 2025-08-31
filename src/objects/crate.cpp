// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
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
	static constexpr sfxenum_t kDefaultSound = sfx_cratew;
};

struct IceCapBlockConfig
{
	static constexpr spritenum_t kSprite = SPR_ICBL;
	static constexpr frame_layout kFrames = {6, 6, 0, 0, 0, 0};
	static constexpr statenum_t kDefaultDebris = S_ICECAPBLOCK_DEBRIS;
	static constexpr sfxenum_t kDefaultSound = sfx_s3k82;
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
	bool boosting() const { return player && (player->sneakertimer || player->panelsneakertimer || player->weaksneakertimer || K_PlayerCanPunt(player)); }
};

struct AnyBox : Graphic
{
	template <typename F>
	bool visit(F&& visitor);

	void update()
	{
		visit([](auto box) { box->mobj_t::eflags &= ~MFE_ONGROUND; });
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

	void extravalue2() = delete;
	sfxenum_t debris_sound() const {return static_cast<sfxenum_t>(mobj_t::extravalue2); }
	void debris_sound(sfxenum_t n) {mobj_t::extravalue2 = n; }

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

		dress(this, FF_FLOORSPRITE)->turn(0); // bottom (me)
		side(FF_FLOORSPRITE)->z(height); // top

		// sides
		side(FF_PAPERSPRITE)->xy(-radius, 0)->turn(ANGLE_270);
		side(FF_PAPERSPRITE)->xy(0, -radius);
		side(FF_PAPERSPRITE)->xy(+radius, 0)->turn(ANGLE_90);
		side(FF_PAPERSPRITE)->xy(0, +radius)->turn(ANGLE_180);

		debris_state(Config::kDefaultDebris);
		debris_sound(Config::kDefaultSound);
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

	bool damage(Mobj* inflictor)
	{
		if (!damage_valid(inflictor))
		{
			return false;
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

		return true;
	}

private:
	void debris(Mobj* inflictor)
	{
		if (debris_state() >= NUMSTATES)
		{
			return;
		}

		auto rng = [&](int x, int y) { return P_RandomRange(PR_DECORATION, x, y) * scale(); };
		auto rng_xyz = [&](int x)
		{
			// note: determinate random argument eval order
			auto rand_z = rng(0, x);
			auto rand_y = rng(-x, x);
			auto rand_x = rng(-x, x);
			return std::tuple(rand_x, rand_y, rand_z);
		};

		auto spawn = [&](bool playsound)
		{
			auto [x, y, z] = rng_xyz(info->height / FRACUNIT);
			Mobj* p = spawn_from<Mobj>({x, y, z}, MT_BOX_DEBRIS);

			p->scale_between(scale() / 2, scale());
			p->state(debris_state());

			std::tie(x, y, z) = rng_xyz(4);

			p->momx = (inflictor->momx / 8) + x;
			p->momy = (inflictor->momy / 8) + y;
			p->momz = (Fixed::hypot(inflictor->momx, inflictor->momy) / 4) + z;

			if (playsound && debris_sound())
			{
				p->voice(debris_sound());
			}
		};

		spawn(true);
		spawn(false);
		spawn(false);
		spawn(false);
		spawn(false);
		spawn(false);
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
			debris_sound(sfx_cratem);
		}
	}

	bool damage(Toucher* inflictor)
	{
		if (!Box::damage_valid(inflictor))
		{
			return false;
		}

		if (metal() && !inflictor->boosting())
		{
			crush(inflictor);

			return false;
		}

		return Box::damage(inflictor);
	}

private:
	bool clip2d(const Toucher* inflictor) const
	{
		LineSegment a = aabb();
		LineSegment b = inflictor->aabb();

		return a.a.x < b.b.x && b.a.x < a.b.x && a.a.y < b.b.y && b.a.y < a.b.y;
	}

	void crush(Toucher* inflictor)
	{
		if (!momz)
		{
			return;
		}

		if ((momz < 0 ? mobj_t::z - inflictor->floorz : inflictor->ceilingz - top()) > inflictor->height)
		{
			return;
		}

		if (!clip2d(inflictor))
		{
			// Bumping the side of a falling crate should not
			// kill you.
			// Note: this check is imperfect. That's why
			// everything is guarded by momz anyway.
			return;
		}

		P_DamageMobj(inflictor, this, nullptr, 1, DMG_CRUSHED);
	}
};

struct Ice : Box<IceCapBlockConfig>
{
};

template <typename F>
bool AnyBox::visit(F&& visitor)
{
	switch (type)
	{
	case MT_SA2_CRATE:
		visitor(static_cast<Crate*>(this));
		break;

	case MT_ICECAPBLOCK:
		visitor(static_cast<Ice*>(this));
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

boolean Obj_TryCrateDamage(mobj_t* target, mobj_t* inflictor)
{
	bool c = false;
	static_cast<AnyBox*>(target)->visit([&](auto box) { c = box->damage(static_cast<Toucher*>(inflictor)); });
	return c;
}

boolean Obj_SA2CrateIsMetal(mobj_t* mobj)
{
	return static_cast<Crate*>(mobj)->metal();
}
