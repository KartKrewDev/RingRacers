// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <array>

#include "objects.hpp"

#include "../core/static_vec.hpp"
#include "../d_player.h"
#include "../doomdef.h"
#include "../doomtype.h"
#include "../g_game.h"
#include "../k_collide.h"
#include "../k_hud.h" // transflag
#include "../m_easing.h"
#include "../m_fixed.h"
#include "../m_random.h"
#include "../r_main.h"
#include "../tables.h"

using namespace srb2::objects;

namespace
{

Fixed distance3d(const Mobj* a, const Mobj* b)
{
	return FixedHypot(FixedHypot(a->x - b->x, a->y - b->y), a->z - b->z);
}

Vec2<Fixed> angle_vector(angle_t x)
{
	return Vec2<Fixed> {FCOS(x), FSIN(x)};
}

// copied from objects/hyudoro.c
static void
sine_bob
(		mobj_t * hyu,
		INT32 height,
		angle_t a,
		fixed_t sineofs)
{
	hyu->sprzoff = FixedMul(height * hyu->scale,
			sineofs + FINESINE(a >> ANGLETOFINESHIFT)) * P_MobjFlip(hyu);
}

static void
bob_in_place
(		mobj_t * hyu,
		INT32 height,
		INT32 bob_speed)
{
	sine_bob(hyu,
			height,
			(leveltime & (bob_speed - 1)) *
			(ANGLE_MAX / bob_speed), -(3*FRACUNIT/4));
}

struct Eye;
struct Pole;
struct Cloud;

struct Eye : Mobj
{
	static constexpr INT32 kOrbitRadius = 24;

	bool valid() const { return Mobj::valid(owner()) && owner()->health > 0; }

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		return true;
	}
};

struct Pole : Mobj
{
	static constexpr sfxenum_t kSound = sfx_s3kdal;

	void extravalue1() = delete;
	tic_t last_touch0() const { return mobj_t::extravalue1; }
	void last_touch0(tic_t n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	bool clouds_spawned() const { return mobj_t::extravalue2; }
	void clouds_spawned(bool n) { mobj_t::extravalue2 = n; }

	void reactiontime() = delete;
	tic_t sound_started() const { return mobj_t::reactiontime; }
	void sound_started(tic_t n) { mobj_t::reactiontime = n; }

	void tracer() = delete;
	Eye* eye() const { return Mobj::tracer<Eye>(); }
	void eye(Eye* n) { Mobj::tracer(n); }

	bool valid() const
	{
		if (!Mobj::valid(eye()))
			return false;

		return true;
	}

	void init()
	{
		Eye* p_eye = spawn_from<Eye>(MT_TOXOMISTER_EYE);

		p_eye->owner(this);
		p_eye->spriteyoffset(96*FRACUNIT);

		last_touch0(leveltime);
		clouds_spawned(false);
		eye(p_eye);

		flags |= MF_SPECIAL;
	}

	void spawn_clouds_in_orbit();

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		if (P_IsObjectOnGround(this))
		{
			if (!clouds_spawned())
			{
				spawn_clouds_in_orbit();
				clouds_spawned(true);
				voice(sfx_s3k9e);
			}

			if (!voice_playing(kSound))
			{
				voice(kSound);
				sound_started(leveltime);
			}

			if ((leveltime - sound_started()) % 256 == 0)
				voice(kSound);
		}
		else
		{
			P_SpawnGhostMobj(this);
		}

		tick_eye();

		return true;
	}

	void tick_eye()
	{
		Mobj::PosArg p = {pos2d(), z};

		p.x += momx;
		p.y += momy;
		p.z += momz;

		Mobj* targ = find_nearest_eyeball_target();
		if (targ)
		{
			INT32 angle_to_targ = angle_to2d(targ);
			Vec2<Fixed> v = angle_vector(angle_to_targ) * Fixed {Eye::kOrbitRadius * mapobjectscale};

			p.x += v.x;
			p.y += v.y;

			eye()->angle = angle_to_targ;
		}

		eye()->move_origin(p);
	}

	angle_t angle_to2d(Mobj* mobj) const
	{
		return R_PointToAngle2(x, y, mobj->x, mobj->y);
	}

	Mobj* find_nearest_eyeball_target() const
	{
		srb2::StaticVec<Mobj*, MAXPLAYERS> targets;

		for (INT32 i = 0; i < MAXPLAYERS; ++i)
		{
			if (!playeringame[i])
				continue;

			if (!players[i].mo)
				continue;

			targets.push_back(static_cast<Mobj*>(players[i].mo));
		}

		if (targets.empty())
			return nullptr;

		return *std::min_element(
			targets.begin(),
			targets.end(),
			[this](Mobj* a, Mobj* b) { return distance3d(this, a) < distance3d(this, b); }
		);
	}

	bool touch(Mobj* toucher)
	{
		if (touch_cooldown(toucher, 0))
			return false;

		if (K_TryPickMeUp(this, toucher, false))
			return false;

		// Adapted from P_XYMovement, MT_JAWZ
		voice(info->deathsound);
		P_KillMobj(this, NULL, NULL, DMG_NORMAL);

		P_SetObjectMomZ(this, 24*FRACUNIT, false);
		instathrust(R_PointToAngle2(toucher->x, toucher->y, x, y), 32 * mapobjectscale);

		flags &= ~MF_NOGRAVITY;
		hitlag(toucher, toucher, 8, true);

		return false;
	}

	bool touch_cooldown
	(	Mobj* toucher,
		UINT8 k)
	{
		tic_t cooldown = leveltime - last_touch0();

		if (toucher == target() && cooldown < 10)
		{
			last_touch0(leveltime);
			return true;
		}

		return false;
	}
};

struct Cloud : Mobj
{
	static constexpr INT32 kMaxFuse = 5*TICRATE;

	void hnext() = delete;
	Mobj* follow() const { return Mobj::hnext<Mobj>(); }
	void follow(Mobj* n) { Mobj::hnext(n); }

	void tracer() = delete;
	Pole* pole() const { return Mobj::tracer<Pole>(); }
	void pole(Pole* n) { Mobj::tracer(n); }

	Fixed fuse_frac() const { return FRACUNIT - fuse * FRACUNIT / kMaxFuse; }
	Fixed drag_var() const { return Easing_Linear(fuse_frac(), FRACUNIT/3, FRACUNIT); }

	bool tick()
	{
		if (Mobj::valid(follow()))
			return tick_follow();

		return tick_patrol();
	}

	bool tick_follow()
	{
		if (!Mobj::valid(follow()))
		{
			remove();
			return false;
		}

		if (K_PuntCollide(this, follow()))
		{
			remove();
			return false;
		}

		if (follow()->player->tripwireLeniency)
		{
			remove();
			return false;
		}

		move_origin(follow()->pos());
		momx = 0;
		momy = 0;
		momz = 0;

		bob_in_place(this, 8, 64);
		voice_loop(sfx_s3kcfl);

		if (leveltime % (TICRATE/3) == 0 && follow()->player->rings > -20) // toxomister ring drain
		{
			follow()->player->rings--;
			K_DefensiveOverdrive(follow()->player);
			S_StartSound(follow()->player->mo, sfx_antiri);
		}

		if (fuse < 3*TICRATE && leveltime % (1 + fuse / TICRATE) == 0)
		{
			renderflags ^= RF_DONTDRAW;
		}

		if (fuse < kMaxFuse && (kMaxFuse - fuse) % 20 == 0 && Mobj::valid(target()) && target()->player && follow()->player)
		{
			K_SpawnAmps(target()->player, K_PvPAmpReward(2, target()->player, follow()->player), this);
		}

		follow()->player->stunned = fuse; // stunned as long as cloud is here
		return true;
	}

	bool tick_patrol()
	{
		if (Mobj::valid(pole()) && pole()->health > 0)
		{
			move_origin(pole()->pos());
			instathrust(angle, 64 * mapobjectscale);
		}
		else
		{
			if (FixedHypot(momx, momy) > 2 * mapobjectscale)
			{
				instathrust(angle, 2 * mapobjectscale);
			}

			if (fuse > 3*TICRATE)
			{
				fuse = 3*TICRATE;
			}
		}

		if (fuse <= 3*TICRATE && (leveltime & 1))
		{
			renderflags ^= RF_DONTDRAW;
		}

		return true;
	}

	bool touch(Mobj* toucher)
	{
		if (toucher == target())
			return false;

		if (K_PuntCollide(this, toucher))
			return true;

		if (toucher->player && toucher->player->tripwireLeniency)
			return true;

		if (toucher->player && target() && !P_MobjWasRemoved(target()) && target()->player && G_SameTeam(toucher->player, target()->player))
			return false;

		if (toucher->player)
		{
			if (this == toucher->player->toxomisterCloud)  // already attached
				return true;

			if (!P_MobjWasRemoved(toucher->player->toxomisterCloud))
			{
				toucher->player->pflags |= PF_CASTSHADOW;
				return true;
			}

			P_SetTarget(&toucher->player->toxomisterCloud, this);
		}

		toucher->hitlag(8);
		scale_to(destscale);
		follow(toucher);
		fuse = kMaxFuse;
		renderflags &= ~RF_DONTDRAW;
		voice(sfx_s3k8a);

		return true;
	}
};

void Pole::spawn_clouds_in_orbit()
{
	constexpr INT32 kNumClouds = 6;
	std::array<UINT32, kNumClouds> weights;
	std::array<INT32, kNumClouds> order;

	angle_t a = 0;
	angle_t a_incr = ANGLE_MAX / kNumClouds;

	for (INT32 i = 0; i < kNumClouds; ++i)
	{
		weights[i] = P_Random(PR_TRACKHAZARD);
		order[i] = i;
	}

	std::stable_sort(order.begin(), order.end(), [&](INT32 a, INT32 b) { return weights[a] < weights[b]; });

	for (INT32 i : order)
	{
		Cloud* cloud = spawn_from<Cloud>({}, MT_TOXOMISTER_CLOUD);

		cloud->pole(this);
		cloud->angle = a;
		cloud->target(target());
		cloud->spriteyoffset(24*FRACUNIT);
		cloud->hitlag(2 + i * 4);
		cloud->scale_between(1, cloud->scale(), cloud->scale() / 5);
		cloud->fuse = 15*TICRATE;

		if (target()->player && target()->player->team)
		{
			cloud->colorized = true;
			cloud->color = target()->color;
		}

		a += a_incr;
	}
}

}; // namespace

void Obj_InitToxomisterPole(mobj_t *pole)
{
	static_cast<Pole*>(pole)->init();
}

boolean Obj_TickToxomisterPole(mobj_t *pole)
{
	return static_cast<Pole*>(pole)->tick();
}

boolean Obj_TickToxomisterEye(mobj_t *eye)
{
	return static_cast<Eye*>(eye)->tick();
}

boolean Obj_TickToxomisterCloud(mobj_t *cloud)
{
	return static_cast<Cloud*>(cloud)->tick();
}

boolean Obj_ToxomisterPoleCollide(mobj_t *pole, mobj_t *toucher)
{
	return static_cast<Pole*>(pole)->touch(static_cast<Mobj*>(toucher));
}

boolean Obj_ToxomisterCloudCollide(mobj_t *cloud, mobj_t *toucher)
{
	return static_cast<Cloud*>(cloud)->touch(static_cast<Mobj*>(toucher));
}

fixed_t Obj_GetToxomisterCloudDrag(mobj_t *cloud)
{
	return static_cast<Cloud*>(cloud)->drag_var();
}
