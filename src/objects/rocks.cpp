// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

// Original Lua script by Sal
// Hardcoded by jartha

#include <array>
#include <cstdlib>

#include "../math/fixed.hpp"
#include "../math/vec.hpp"
#include "../mobj.hpp"
#include "../mobj_list.hpp"

#include "../d_player.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_objects.h"
#include "../m_fixed.h"
#include "../p_pspr.h"
#include "../sounds.h"
#include "../tables.h"

using srb2::Mobj;
using srb2::MobjList;
using srb2::math::Fixed;
using srb2::math::Vec2;

extern mobj_t* svg_rocks;

namespace
{

struct AngelIsland
{
	static constexpr statenum_t kRespawnState = S_AZROCKS_RESPAWN;
	static constexpr mobjtype_t kParticleType = MT_AZROCKS_PARTICLE;
	static constexpr std::array<statenum_t, 2> kParticleStates = {S_AZROCKS_PARTICLE1, S_AZROCKS_PARTICLE1};
};

struct EndlessMine
{
	static constexpr statenum_t kRespawnState = S_EMROCKS_RESPAWN;
	static constexpr mobjtype_t kParticleType = MT_EMROCKS_PARTICLE;
	static constexpr std::array<statenum_t, 2> kParticleStates = {S_EMROCKS_PARTICLE1, S_EMROCKS_PARTICLE2};
};

struct AnyRocks : Mobj
{
	void hnext() = delete;
	AnyRocks* next() const { return Mobj::hnext<AnyRocks>(); }
	void next(AnyRocks* n) { Mobj::hnext(n); }

	template <typename F>
	bool visit(F&& visitor);
};

template <class Config>
struct Rocks : AnyRocks
{
	bool busted() const { return state()->num() == Config::kRespawnState; }

	void respawn()
	{
		if (busted())
		{
			tics = 2; // respawn soon
		}
	}

	void touch(Mobj* toucher)
	{
		if (busted())
		{
			if (tics > 0)
			{
				tics = 2; // postpone respawn
			}
		}
		else
		{
			slow(toucher);
			bust(toucher);
		}
	}

private:
	static void slow(Mobj* toucher)
	{
		const player_t* p = toucher->player;

		if (p->sneakertimer || p->panelsneakertimer || p->weaksneakertimer || p->invincibilitytimer || p->growshrinktimer > 0 || p->hyudorotimer)
		{
			return;
		}

		toucher->momx /= 2;
		toucher->momy /= 2;
		toucher->momz = toucher->flip(std::abs(2 * toucher->momz / 3));
	}

	void bust(Mobj* toucher)
	{
		vfx(toucher, 1);
		vfx(toucher, 2);
		vfx(toucher, 3);

		voice(info->deathsound);
		state(Config::kRespawnState);
	}

	void vfx(Mobj* toucher, int i)
	{
		fixed_t zvar = flip((i + 1) * 4 * mapobjectscale);
		angle_t avar = ANGLE_45 * (i - 2);

		auto part = [&](angle_t angle, statenum_t state)
		{
			Mobj* h = spawn_from<Mobj>({Vec2<Fixed> {}, zvar}, Config::kParticleType);
			h->state(state);
			h->angle = angle;
			h->instathrust(angle, 4 * mapobjectscale);
			h->momz = zvar;
			return h;
		};

		static_assert(Config::kParticleStates.size() == 2);

		part(toucher->angle + ANGLE_90 - avar, Config::kParticleStates[0]);
		part(toucher->angle - ANGLE_90 + avar, Config::kParticleStates[1]);
	}
};

struct AngelIslandRocks : Rocks<AngelIsland>
{
};

struct EndlessMineRocks : Rocks<EndlessMine>
{
};

template <typename F>
bool AnyRocks::visit(F&& visitor)
{
	switch (type)
	{
	case MT_AZROCKS:
		visitor(static_cast<AngelIslandRocks*>(this));
		break;

	case MT_EMROCKS:
		visitor(static_cast<EndlessMineRocks*>(this));
		break;

	default:
		return false;
	}

	return true;
}

MobjList<AnyRocks, svg_rocks> rocks_list;

}; // namespace

void Obj_LinkRocks(mobj_t* mo)
{
	rocks_list.push_front(static_cast<AnyRocks*>(mo));
}

void Obj_UnlinkRocks(mobj_t* mo)
{
	rocks_list.erase(static_cast<AnyRocks*>(mo));
}

void Obj_TouchRocks(mobj_t* special, mobj_t* toucher)
{
	static_cast<AnyRocks*>(special)->visit([&](auto rocks) { rocks->touch(static_cast<Mobj*>(toucher)); });
}

void Obj_UpdateRocks(void)
{
	for (AnyRocks* h : rocks_list)
	{
		h->visit([](auto rocks) { rocks->respawn(); });
	}
}

void Obj_AnimateEndlessMineRocks(mobj_t *mo)
{
	// sync colors with sky animation
	constexpr int kFrames = 8;
	constexpr int kDiff = kFrames - 2;
	constexpr int kTotal = kFrames + kDiff;

	UINT8 f = ((leveltime / 6) % kTotal);

	if (f >= kFrames)
	{
		f = kTotal - f;
	}

	mo->frame = (mo->frame & ~FF_FRAMEMASK) | f;
}
