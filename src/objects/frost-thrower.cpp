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
#include <cstdlib>

#include "../math/fixed.hpp"
#include "../math/vec.hpp"
#include "../mobj.hpp"

#include "../d_player.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../k_bot.h" // FIXME
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
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

struct IceCube : Mobj
{
	static constexpr int kRadius = 33;

	void extravalue1() = delete;
	angle_t angle_offset() const { return mobj_t::extravalue1; }
	void angle_offset(angle_t n) { mobj_t::extravalue1 = n; }

	static void spawn_cube(Mobj* source)
	{
		angle_t offset = 0;
		for (int i = 0; i < 4; ++i)
		{
			spawn_side(source, offset);
			offset += ANGLE_90;
		}
	}

	bool valid() const { return Mobj::valid() && Mobj::valid(owner()) && player() && player()->icecube.frozen; }
	player_t* player() const { return owner()->player; }

	bool think()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		angle_t facing = player()->drawangle + angle_offset();

		scale(owner()->scale());
		move_origin({owner()->pos2d() + (vector(facing) * scale()), owner()->z});
		frame = state()->frame + player()->icecube.wiggle;
		angle = facing + ANGLE_90;

		return true;
	}

private:
	static Vec2<Fixed> vector(angle_t x) { return angle_vector(x) * kRadius; }

	static IceCube* spawn_side(Mobj* source, angle_t offset)
	{
		angle_t facing = source->angle + offset;
		IceCube* cube = source->spawn_from<IceCube>({vector(facing) * source->scale(), 0}, MT_GGZICECUBE);
		cube->angle_offset(offset);
		cube->angle = facing + ANGLE_90;
		cube->owner(source);
		return cube;
	}
};

struct Frost : Mobj
{
	void extravalue1() = delete;
	bool skip_invincible_checks() const { return mobj_t::extravalue1; }
	void skip_invincible_checks(bool n) { mobj_t::extravalue1 = n; }

	static Frost* spawn(Mobj* source, const Mobj::PosArg& p)
	{
		Frost* frost = source->spawn_from<Frost>(p, MT_GGZICEDUST);

		auto rng = [] { return P_RandomRange(PR_FROSTTHROWERS, -2, 2) * (4 * mapobjectscale); };

		frost->momx = rng();
		frost->momy = rng();
		frost->scale_between(mapobjectscale / 2, mapobjectscale * 2, mapobjectscale / 6);

		return frost;
	}

	void collide(Mobj* mo) const
	{
		player_t* p = mo->player;
		icecubevars_t& vars = p->icecube;

		if (vars.hitat < leveltime && leveltime - vars.hitat < TICRATE)
		{
			// avoid spamming instashield
			return;
		}

		vars.hitat = leveltime;

		if (!skip_invincible_checks())
		{
			if (p->flashing || p->growshrinktimer > 0 || p->invincibilitytimer || p->hyudorotimer)
			{
				K_DoInstashield(p);
				return;
			}
		}

		if (vars.frozen)
		{
			return;
		}

		vars.frozen = true;
		vars.wiggle = 0;
		vars.frozenat = leveltime;
		vars.shaketimer = 0;

		IceCube::spawn_cube(mo);
	}
};

struct FreezeThruster : Mobj
{
	static constexpr tic_t kFlashRate = TICRATE*3/2;

	void extravalue1() = delete;
	bool frosting() const { return mobj_t::extravalue1; }
	void frosting(bool n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	tic_t default_timer() const { return mobj_t::extravalue2; }
	void default_timer(tic_t n) { mobj_t::extravalue2 = n; }

	void threshold() = delete;
	tic_t timer() const { return mobj_t::threshold; }
	void timer(tic_t n) { mobj_t::threshold = n; }

	void thing_args() = delete;
	bool double_speed() const { return mobj_t::thing_args[0]; }
	bool skip_invincible_checks() const { return mobj_t::thing_args[1]; }
	bool default_frosting() const { return !mobj_t::thing_args[2]; }

	void init()
	{
		// FIXME: this should use a thing arg, not angle!
		if (spawnpoint)
		{
			// Object Angle = Tics Between Activations
			default_timer(spawnpoint->angle);
		}

		frosting(default_timer() ? default_frosting() : true);
		timer(default_timer());
	}

	void think()
	{
		if (timer())
		{
			timer(timer() - 1);

			if (timer() < 1)
			{
				frosting(!frosting());
				timer(default_timer());
			}
		}

		// flash before turning on
		if (!frosting() && timer() < kFlashRate && (leveltime % 4) < 2)
		{
			colorized = true;
			color = SKINCOLOR_RED;
		}
		else
		{
			colorized = false;
		}

		spew();
	}

private:
	void spew()
	{
		if (!frosting())
		{
			return;
		}

		// Object Height = Frostthrower Speed
		fixed_t mz = is_flipped() ? floorz + 1 : ceilingz;
		fixed_t frostspeed = std::abs(z - mz) / 10;

		if (double_speed())
		{
			frostspeed *= 2;
		}

		if (leveltime % 3 == 0)
		{
			Frost* frost = Frost::spawn(this, {});

			if (is_flipped())
			{
				frost->momz = -frostspeed;
			}
			else
			{
				frost->momz = frostspeed;
				frost->flags2 |= MF2_OBJECTFLIP;
			}

			frost->skip_invincible_checks(skip_invincible_checks());
		}

		voice_loop(sfx_s3k7f);
	}
};

struct SidewaysFreezeThruster : Mobj
{
	static constexpr int kBaseRadius = 12;

	bool skip_invincible_checks() const { return mobj_t::thing_args[0]; }

	void init()
	{
		scale(scale()*3/2);

		const Vec2<Fixed> base_vector = vector(angle) * kBaseRadius;

		angle_t an = angle + ANGLE_90;

		z += flip(24 * mapobjectscale);

		auto spawn_piece = [&](const Mobj::PosArg& p, angle_t angle, int frame)
		{
			Mobj* part = spawn_from<Mobj>(p, MT_THRUSTERPART);
			part->angle = angle;
			part->frame += frame;
			return part;
		};

		spawn_piece({}, an, 0); // base
		spawn_piece({vector(angle) * 24, 0}, an, 1); // cannon

		Vec2<Fixed> h_vector = angle_vector(an);

		// spawn the pipes:
		auto spawn_pipe = [&](int i)
		{
			angle_t v_an = ANGLE_45 + (ANGLE_90 * i);
			Vec2<Fixed> v_vector = vector(v_an) * 32;
			spawn_piece({h_vector * v_vector.y, v_vector.x}, angle, 2);
		};

		// This is unrolled because when it was a for loop,
		// it ran infinitely, but only under MinGW.
		// Tested: gcc.exe (Rev2, Built by MSYS2 project) 13.2.0 (32-bit version)
		spawn_pipe(0);
		spawn_pipe(1);
		spawn_pipe(2);
		spawn_pipe(3);

		// spawn the icons:
		Vec2<Fixed> v = vector(an) * 32;
		spawn_piece({base_vector + v, 0}, angle, 3);
		spawn_piece({base_vector - v, 0}, angle, 3);
	}

	void think()
	{
		if (leveltime % 2 == 0)
		{
			spew();
			spew();
		}
	}

private:
	Vec2<Fixed> vector(angle_t angle) const { return angle_vector(angle) * scale(); }

	void spew()
	{
		Frost* frost = Frost::spawn(this, {vector(angle) * kBaseRadius, 0});
		auto rng = [](int x, int y) { return P_RandomRange(PR_FROSTTHROWERS, x, y); };
		frost->angle = angle + (rng(-8, 8) * ANG1);
		frost->instathrust(frost->angle, mapobjectscale * 64);
		frost->momz = rng(-5, 5) * mapobjectscale;
		frost->scale_between(mapobjectscale * 3 / 4, mapobjectscale * 2, mapobjectscale / 6);
		frost->skip_invincible_checks(skip_invincible_checks());
	}
};

struct Shatter : Mobj
{
	static Shatter* spawn(Mobj* source)
	{
		auto rng = [source](int x, int y) { return P_RandomRange(PR_DECORATION, x, y) * source->scale(); };
		Shatter* part = source->spawn_from<Shatter>({rng(-64, 64), rng(-64, 64), rng(0, 64)}, MT_GGZICESHATTER);
		part->state(P_RandomRange(PR_DECORATION, 0, 1) ? S_GGZPARTICLE11 : S_GGZPARTICLE21);
		part->fuse = TICRATE * 4;
		return part;
	}
};

}; // namespace

void Obj_FreezeThrusterInit(mobj_t* mo)
{
	static_cast<FreezeThruster*>(mo)->init();
}

void Obj_FreezeThrusterThink(mobj_t* mo)
{
	static_cast<FreezeThruster*>(mo)->think();
}

void Obj_IceDustCollide(mobj_t* t1, mobj_t* t2)
{
	static_cast<Frost*>(t1)->collide(static_cast<Mobj*>(t2));
}

boolean Obj_IceCubeThink(mobj_t* mo)
{
	return static_cast<IceCube*>(mo)->think();
}

void Obj_IceCubeInput(player_t* player)
{
	// Must be mashing some buttons
	auto press = [player](buttoncode_t bt) { return (player->cmd.buttons & ~player->oldcmd.buttons) & bt; };
	if (!(press(BT_ACCELERATE) || press(BT_ATTACK) || press(BT_DRIFT)))
	{
		return;
	}

	if (leveltime - std::min(player->icecube.frozenat, leveltime) < TICRATE*2/3)
	{
		return;
	}

	Mobj* mo = static_cast<Mobj*>(player->mo);

	player->icecube.wiggle++;
	player->icecube.shaketimer = 10;

	mo->voice(sfx_s3k94);

	auto rng = [mo](int x, int y) { return P_RandomRange(PR_DECORATION, x, y) * mo->scale(); };

	for (int i = 0; i < 6; ++i)
	{
		Shatter* part = Shatter::spawn(mo);
		part->scale_between(mo->scale() * 3, 1);
		part->momx = mo->momx + rng(-14, 14);
		part->momy = mo->momy + rng(-14, 14);
		part->momz = mo->momz + rng(0, 14);
	}

	if (player->icecube.wiggle > 4)
	{
		Obj_IceCubeBurst(player);
		player->flashing = (TICRATE * 3) - 1;
	}
}

void Obj_IceCubeBurst(player_t* player)
{
	Mobj* mo = static_cast<Mobj*>(player->mo);

	auto rng = [mo](int x, int y) { return P_RandomRange(PR_DECORATION, x, y) * mo->scale(); };

	for (int i = 0; i < 20; ++i)
	{
		Shatter* part = Shatter::spawn(mo);
		part->scale_between(mo->scale() * 5, 1);
		part->momx = (mo->momx * 2) + rng(-64, 64);
		part->momy = (mo->momy * 2) + rng(-64, 64);
		part->momz = (mo->momz * 2) + rng(0, 20);
	}

	player->icecube.frozen = false;

	mo->voice(sfx_glgz1);
}

void Obj_SidewaysFreezeThrusterInit(mobj_t* mo)
{
	static_cast<SidewaysFreezeThruster*>(mo)->init();
}

void Obj_SidewaysFreezeThrusterThink(mobj_t* mo)
{
	static_cast<SidewaysFreezeThruster*>(mo)->think();
}
