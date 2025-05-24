// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "broly.hpp"
#include "objects.hpp"

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_objects.h"
#include "../sounds.h"
#include "../tables.h"

using namespace srb2::objects;

namespace
{

struct FuelCanister : Mobj
{
	struct Emitter : Mobj
	{
		void thing_args() = delete;
		tic_t frequency() const { return mobj_t::thing_args[0]; }
		tic_t initial_timer() const { return mobj_t::thing_args[1]; }

		void extravalue1() = delete;
		tic_t timer() const { return mobj_t::extravalue1; }
		void timer(tic_t n) { mobj_t::extravalue1 = n; }

		void init()
		{
			timer(initial_timer());
		}

		bool think()
		{
			if (timer() > 0)
			{
				timer(timer() - 1);
				return true;
			}

			timer(frequency());

			FuelCanister::spawn(this);

			return true;
		}
	};

	struct Vis : Mobj
	{
		void extravalue1() = delete;
		angle_t phys_angle_ofs() const { return mobj_t::extravalue1; }
		void phys_angle_ofs(angle_t n) { mobj_t::extravalue1 = n; }

		void extravalue2() = delete;
		angle_t vis_angle_ofs() const { return mobj_t::extravalue2; }
		void vis_angle_ofs(angle_t n) { mobj_t::extravalue2 = n; }

		bool valid() const { return Mobj::valid() && Mobj::valid(target()); }

		bool think()
		{
			if (!valid())
			{
				remove();
				return false;
			}

			const angle_t angleOutward = target()->angle + phys_angle_ofs();

			move_origin({target()->pos2d() + (vector(angleOutward) * Fixed {radius}), target()->z});
			angle = angleOutward + vis_angle_ofs();

			return true;
		}
	};

	struct Explosion : Broly
	{
		static constexpr mobjtype_t kMobjType = MT_BETA_PARTICLE_EXPLOSION;

		static Explosion* spawn(Mobj* source)
		{
			Explosion* x = Broly::spawn<Explosion>(source, 3*TICRATE, {1, 8 * mapobjectscale});
			x->voice(sfx_lcfuel);
			return x;
		}

		void touch(Mobj* toucher)
		{
			if (!P_DamageMobj(toucher, this, this, 1, DMG_NORMAL))
			{
				auto& hitlag = toucher->mobj_t::hitlag;

				// Hitlag = remaining duration of explosion
				if (hitlag >= 0 && hitlag + 0u < remaining())
				{
					hitlag = remaining();
				}
			}
		}

		bool think() { return Broly::think(); }
	};

	bool valid() const { return Mobj::valid() && momz; }

	static FuelCanister* spawn(Mobj* source)
	{
		FuelCanister* caps = source->spawn_from<FuelCanister>({}, MT_BETA_PARTICLE_PHYSICAL);
		caps->init();
		return caps;
	}

	void init()
	{
		momz = 8 * scale();
		z -= momz;

		pieces<Wheel>();
		pieces<Icon>();
	}

	bool think()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		angle += 8 * ANG1;

		return true;
	}

	void touch(Mobj* toucher)
	{
		Explosion::spawn(toucher);
	}

private:
	struct Wheel
	{
		static constexpr int kSides = 6;
		static constexpr statenum_t kState = S_BETA_PARTICLE_WHEEL;
		static constexpr int kRadius = 8;
		static constexpr Fixed kScale = FRACUNIT;
		static constexpr angle_t kAngleOffset = 0;
		static constexpr int kZOffset = 0;
	};

	struct Icon
	{
		static constexpr int kSides = 2;
		static constexpr statenum_t kState = S_BETA_PARTICLE_ICON;
		static constexpr int kRadius = 8;
		static constexpr Fixed kScale = 3*FRACUNIT/4;
		static constexpr angle_t kAngleOffset = ANGLE_90;
		static constexpr int kZOffset = 64;
	};

	static Vec2<Fixed> vector(angle_t angle) { return {FCOS(angle), FSIN(angle)}; }

	template <class Config>
	void pieces()
	{
		constexpr angle_t kAngleBetween = ANGLE_MAX / Config::kSides;

		const Fixed zOfs = Config::kZOffset * (Fixed {FRACUNIT} / Config::kScale);
		const Fixed radius = Config::kRadius * scale();
		const Fixed scale = Config::kScale * this->scale();

		for (int i = 1; i <= Config::kSides; ++i)
		{
			angle_t angleOutward = i * kAngleBetween;

			Vis* vis = spawn_from<Vis>({vector(angle + angleOutward) * radius, 0}, MT_BETA_PARTICLE_VISUAL);

			vis->state(Config::kState);
			vis->target(this);
			vis->scale(scale);
			vis->radius = radius;
			vis->spriteyoffset(zOfs);

			vis->phys_angle_ofs(angleOutward);
			vis->vis_angle_ofs(Config::kAngleOffset);
		}
	}
};

}; // namespace

void Obj_FuelCanisterEmitterInit(mobj_t *mo)
{
	static_cast<FuelCanister::Emitter*>(mo)->init();
}

boolean Obj_FuelCanisterVisualThink(mobj_t *mo)
{
	return static_cast<FuelCanister::Vis*>(mo)->think();
}

boolean Obj_FuelCanisterEmitterThink(mobj_t *mo)
{
	return static_cast<FuelCanister::Emitter*>(mo)->think();
}

boolean Obj_FuelCanisterThink(mobj_t *mo)
{
	return static_cast<FuelCanister*>(mo)->think();
}

void Obj_FuelCanisterTouch(mobj_t *special, mobj_t *toucher)
{
	static_cast<FuelCanister*>(special)->touch(static_cast<Mobj*>(toucher));
}

void Obj_FuelCanisterExplosionTouch(mobj_t *special, mobj_t *toucher)
{
	static_cast<FuelCanister::Explosion*>(special)->touch(static_cast<Mobj*>(toucher));
}

boolean Obj_FuelCanisterExplosionThink(mobj_t *mo)
{
	return static_cast<FuelCanister::Explosion*>(mo)->think();
}
