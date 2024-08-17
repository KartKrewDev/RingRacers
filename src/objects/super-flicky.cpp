// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Super Flicky power-up, hunts other players

#include <algorithm>

#include "../d_player.h"
#include "../doomdef.h"
#include "../g_game.h"
#include "../k_battle.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../k_respawn.h"
#include "../m_fixed.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../tables.h"

#define flicky_controller(o) ((o)->target)
#define flicky_chasing(o) ((o)->tracer)
#define flicky_next(o) ((o)->hnext)
#define flicky_next_target(o) ((o)->hprev)
#define flicky_phase(o) ((o)->threshold)
#define flicky_delay(o) ((o)->movecount)
#define flicky_mode(o) ((o)->extravalue1)
#define flicky_fly(o) ((o)->extravalue2)
#define flicky_soundtimer(o) ((o)->cusval)

#define controller_source(o) ((o)->target)
#define controller_chasing(o) ((o)->tracer)
#define controller_flicky(o) ((o)->hnext)
#define controller_mode(o) ((o)->movecount)
#define controller_zofs(o) ((o)->sprzoff)
#define controller_expiry(o) ((o)->fuse)
#define controller_intangible(o) ((o)->threshold)

namespace
{

constexpr tic_t kOrbitSpeed = 2*TICRATE;
constexpr int kOrbitSpacing = ANGLE_90;

// Multiples of player radius
constexpr fixed_t kOrbitRadiusInitial = 32*FRACUNIT;
constexpr fixed_t kOrbitRadius = 2*FRACUNIT;

constexpr int kDescendHeight = 256;
constexpr int kDescendSmoothing = 16;

constexpr int kSearchRadius = 1920;
constexpr int kScaredRadius = 2048;
constexpr int kFlightRadius = 1280;
constexpr int kPeckingRadius = 256;

constexpr fixed_t kFlightSpeed = 2*FRACUNIT;
constexpr fixed_t kPeckingSpeed = FRACUNIT/2;
constexpr fixed_t kWeakSpeed = FRACUNIT/2;

constexpr fixed_t kRebound = 8*FRACUNIT/9;

constexpr tic_t kDelay = 8;
constexpr tic_t kDamageCooldown = TICRATE;
constexpr tic_t kStunTime = 10*TICRATE;
constexpr tic_t kBlockTime = 10*TICRATE;
constexpr tic_t kOwnerStunnedCooldown = 2*TICRATE;

constexpr int kRiseTime = 1*TICRATE;
constexpr int kRiseSpeed = 4;

constexpr int kMinKnockback = 50;

// TODO: skincolor must be updated to 2.2 palette
constexpr skincolornum_t kSuperStart = SKINCOLOR_SUPERGOLD1;
constexpr skincolornum_t kSuperEnd = SKINCOLOR_SUPERGOLD5;

// copied from objects/hyudoro.c
void
sine_bob
(		mobj_t * hyu,
		angle_t a,
		fixed_t sineofs)
{
	const fixed_t kBobHeight = 4 * mapobjectscale;

	// slightly modified from objects/hyudoro.c
	hyu->sprzoff = FixedMul(kBobHeight,
			sineofs + FINESINE(a >> ANGLETOFINESHIFT)) * P_MobjFlip(hyu);
			
	if (P_IsObjectFlipped(hyu))
		hyu->sprzoff -= hyu->height;
}

void
bob_in_place
(		mobj_t * hyu,
		INT32 phase,
		INT32 bob_speed)
{
	sine_bob(hyu,
			((leveltime + phase) & (bob_speed - 1)) *
			(ANGLE_MAX / bob_speed), -(3*FRACUNIT/4));
}

struct Flicky;

struct Controller : mobj_t
{
	enum class Mode : int
	{
		kDescend,
		kOrbit,
		kEnRoute,
		kAttached,
		kReturning,
	};

	mobj_t* source() const { return controller_source(this); }
	void source(mobj_t* n) { P_SetTarget(&controller_source(this), n); }

	mobj_t* chasing() const { return controller_chasing(this); }
	void chasing(mobj_t* n) { P_SetTarget(&controller_chasing(this), n); }

	Flicky* flicky() const;
	void flicky(Flicky* n);

	Mode mode() const { return static_cast<Mode>(controller_mode(this)); }
	void mode(Mode n) { controller_mode(this) = static_cast<int>(n); }

	fixed_t zofs() const { return controller_zofs(this); }
	void zofs(fixed_t n) { controller_zofs(this) = n; }

	tic_t expiry() const { return controller_expiry(this); }
	void expiry(tic_t n) { controller_expiry(this) = n; }

	tic_t intangible() const { return controller_intangible(this); }
	void intangible(tic_t n) { controller_intangible(this) = n; }

	static Controller* spawn(player_t* player, tic_t time)
	{
		Controller* x = static_cast<Controller*>(P_SpawnMobjFromMobjUnscaled(
				player->mo,
				0,
				0,
				kDescendHeight * mapobjectscale,
				MT_SUPER_FLICKY_CONTROLLER
		));

		x->source(player->mo);
		x->mode(Mode::kDescend);
		x->zofs(0);
		x->expiry(leveltime + time + kRiseTime);

		P_SetTarget(&player->powerup.flickyController, x);

		S_StartSound(x, sfx_s3k46);

		return x;
	}

	bool valid() { return !P_MobjWasRemoved(source()); }
	tic_t time_remaining() const { return expiry() - leveltime; }
	tic_t powerup_remaining() const { return ending() ? 0u : time_remaining() - kRiseTime; }
	bool ending() const { return time_remaining() <= kRiseTime; }

	void descend()
	{
		fixed_t head = P_GetMobjHead(source());
		fixed_t tz = head;

		if (mode() == Mode::kDescend)
		{
			tz = z - ((z - head) / kDescendSmoothing);

			if ((tz - head) < mapobjectscale)
			{
				mode(Mode::kOrbit);
				tz = head;
			}
		}

		z = tz + zofs();

		if (ending())
		{
			zofs(zofs() + (kRiseSpeed * mapobjectscale * P_MobjFlip(this)));
		}
	}

	void expand()
	{
		fixed_t n = FixedMul(kOrbitRadiusInitial, ((z - P_GetMobjHead(source())) / kDescendHeight));

		radius = FixedMul(FixedMul(kOrbitRadius, source()->radius), FRACUNIT + n);
	}

	void end()
	{
		// +1 in case flicky already thunk this tic
		expiry(leveltime + kRiseTime + 1);
	}

	void search();
	void collect();
};

struct Flicky : mobj_t
{
	enum class Mode : int
	{
		kReserved,
		kHunting,
		kStunned,
		kWeak,
	};

	enum class Fly : int
	{
		kNormal,
		kZoom,
		kSlow,
	};

	Controller* controller() const { return static_cast<Controller*>(flicky_controller(this)); }
	void controller(Controller* n) { P_SetTarget(&flicky_controller(this), n); }

	mobj_t* chasing() const { return flicky_chasing(this); }
	void chasing(mobj_t* n) { P_SetTarget(&flicky_chasing(this), n); }

	Flicky* next() const { return static_cast<Flicky*>(flicky_next(this)); }
	void next(Flicky* n) { P_SetTarget(&flicky_next(this), n); }

	mobj_t* next_target() const { return flicky_next_target(this); }
	void next_target(mobj_t* n) { P_SetTarget(&flicky_next_target(this), n); }

	int phase() const { return flicky_phase(this); }
	void phase(int n) { flicky_phase(this) = n; }

	int delay() const { return flicky_delay(this); }
	void delay(int n) { flicky_delay(this) = n; }

	Mode mode() const { return static_cast<Mode>(flicky_mode(this)); }
	void mode(Mode n) { flicky_mode(this) = static_cast<int>(n); }

	Fly fly() const { return static_cast<Fly>(flicky_fly(this)); }
	void fly(Fly n) { flicky_fly(this) = static_cast<int>(n); }

	tic_t soundtimer() const { return flicky_soundtimer(this); }
	void soundtimer(tic_t n) { flicky_soundtimer(this) = n; }

	mobj_t* source() const { return controller()->source(); }

	static void spawn(Controller* controller, int phase)
	{
		Flicky* x = static_cast<Flicky*>(P_SpawnMobjFromMobj(controller, 0, 0, 0, MT_SUPER_FLICKY));

		x->controller(controller);
		x->phase(phase);
		x->delay(0);
		x->mode(Mode::kReserved);
		x->light_up(true);

		x->next(controller->flicky());
		controller->flicky(x);
	}

	static skincolornum_t super_color()
	{
		return static_cast<skincolornum_t>(kSuperStart + ((leveltime / 4) % ((kSuperEnd - kSuperStart) + 1)));
	}

	bool valid() const { return !P_MobjWasRemoved(controller()) && controller()->valid(); }

	bool stunned() const { return mode() == Mode::kStunned || mode() == Mode::kWeak; }

	bool owner_in_pain() const { return source()->player && P_PlayerInPain(source()->player); }

	void light_up(bool n)
	{
		if (n)
		{
			renderflags |= RF_FULLBRIGHT;
			color = super_color();
		}
		else
		{
			renderflags &= ~(RF_FULLBRIGHT);
			color = source()->player ? source()->player->skincolor : source()->color;
		}
	}

	void animate()
	{
		destscale = source()->scale * (chasing() ? 3 : 1);

		if (color >= kSuperStart && color <= kSuperEnd)
		{
			color = super_color();
		}

		K_FlipFromObject(this, source());
		bob_in_place(this, phase() * 8, 32);
	}

	void refocus()
	{
		if (soundtimer() > 0)
		{
			soundtimer(soundtimer() - 1);
		}

		if (controller()->ending())
		{
			if (controller()->time_remaining() == kRiseTime)
			{
				light_up(false);
				rise();
			}

			return;
		}

		if (delay() > 0)
		{
			delay(delay() - 1);
		}
		else
		{
			if (stunned())
			{
				unnerf();
				mode(Mode::kHunting);
			}

			if (chasing() != next_target())
			{
				chasing(next_target());
				mode(Mode::kHunting);

				S_StartSound(this, sfx_fhurt2);
			}
		}
	}

	angle_t orbit_angle() const { return controller()->angle + (phase() * kOrbitSpacing); }

	vector3_t orbit_position() const
	{
		return {
			source()->x + FixedMul(FCOS(orbit_angle()), controller()->radius),
			source()->y + FixedMul(FSIN(orbit_angle()), controller()->radius),
			controller()->z
		};
	}

	void orbit()
	{
		vector3_t pos = orbit_position();

		P_MoveOrigin(this, pos.x, pos.y, pos.z);

		momx = 0;
		momy = 0;
		momz = 0;

		angle = orbit_angle() + ANGLE_90; // face direction of orbit
	}


	// copied from objects/spb.c
	void spawn_speed_lines(angle_t direction)
	{
		if (mode() != Mode::kHunting)
		{
			return;
		}

		mobj_t *fast = P_SpawnMobjFromMobjUnscaled(
			this,
			P_RandomRange(PR_DECORATION, -24, 24) * mapobjectscale,
			P_RandomRange(PR_DECORATION, -24, 24) * mapobjectscale,
			(height / 2) + (P_RandomRange(PR_DECORATION, -24, 24) * mapobjectscale),
			MT_FASTLINE
		);

		P_SetTarget(&fast->target, this);
		fast->angle = direction;

		fast->color = source()->color;
		fast->colorized = true;
		fast->renderflags |= RF_ADD;

		K_MatchGenericExtraFlags(fast, this);
	}

	void range_check()
	{
		if (!chasing())
		{
			return;
		}

		if (FixedHypot(source()->x - chasing()->x, source()->y - chasing()->y) < kScaredRadius * mapobjectscale)
		{
			return;
		}

		if (chasing()->player)
		{
			P_SetTarget(&chasing()->player->flickyAttacker, nullptr);
		}

		next_target(nullptr);
		chasing(nullptr);

		if (controller()->mode() != Controller::Mode::kReturning)
		{
			for (Flicky* x = controller()->flicky(); x; x = x->next())
			{
				if (x != this)
				{
					continue;
				}

				controller()->chasing(nullptr);
				controller()->mode(Controller::Mode::kReturning);
			}
		}
	}

	void relink()
	{
		Flicky* prev = nullptr;

		for (Flicky* x = controller()->flicky(); x; x = x->next())
		{
			if (x == this)
			{
				return;
			}

			prev = x;
		}

		if (prev)
		{
			prev->next(this);
		}
		else
		{
			controller()->flicky(this);
		}

		next(nullptr);
	}

	void chase()
	{
		if (controller()->ending())
		{
			return;
		}

		range_check();

		vector3_t pos = chasing() ? vector3_t{chasing()->x, chasing()->y, P_GetMobjFeet(chasing())} : orbit_position();
		angle_t th = R_PointToAngle2(x, y, pos.x, pos.y);

		momz = (pos.z - z) / kDescendSmoothing;
		angle = K_MomentumAngleReal(this);

		angle_t d = AngleDelta(th, angle);
		fixed_t dist = FixedHypot(x - pos.x, y - pos.y);

		const Fly oldFly = fly();

		if (mode() == Mode::kWeak)
		{
			P_Thrust(this, th, FixedMul(kWeakSpeed, mapobjectscale));
			fly(Fly::kNormal);
		}
		else if (d < ANGLE_11hh && dist < kPeckingRadius * mapobjectscale)
		{
			// Drastically speed up when about to intersect
			P_Thrust(this, th, FixedMul(kPeckingSpeed, mapobjectscale));
			fly(Fly::kZoom);
		}
		else
		{
			P_Thrust(this, th, FixedMul(kFlightSpeed, mapobjectscale));
			fly(Fly::kNormal);
		}

		auto speed_lines = [&](angle_t dir)
		{
			if (dist > kPeckingRadius * mapobjectscale)
			{
				spawn_speed_lines(dir);
			}
		};

		if (d > ANGLE_45 && dist > kFlightRadius * mapobjectscale)
		{
			// Cut momentum when too far outside of intended trajectory
			momx = FixedMul(momx, kRebound);
			momy = FixedMul(momy, kRebound);

			speed_lines(th);

			fly(Fly::kSlow);
		}
		else
		{
			speed_lines(angle);
		}

		// Returning to owner
		if (!chasing())
		{
			if (AngleDelta(th, R_PointToAngle2(x + momx, y + momy, pos.x, pos.y)) > ANG1)
			{
				unnerf();
				mode(Mode::kReserved);
				relink();
				controller()->collect();
			}
			else
			{
				P_InstaThrust(this, th, FixedHypot(momx, momy));
			}
		}

		if (fly() != oldFly)
		{
			switch (fly())
			{
			case Fly::kNormal:
				break;

			case Fly::kZoom:
				S_StartSound(this, sfx_fbird);
				break;

			case Fly::kSlow:
				S_StartSound(this, sfx_fbost1);
				break;
			}
		}

		noclip(chasing() && dist > kFlightRadius * mapobjectscale);
	}

	void rise()
	{
		P_SetObjectMomZ(this, kRiseSpeed * FRACUNIT, false);
	}

	void damage(mobj_t* mobj)
	{
		if (!mobj->player)
		{
			return;
		}

		if (mobj != chasing())
		{
			return;
		}

		if (mode() != Mode::kHunting)
		{
			return;
		}

		if (leveltime < controller()->intangible())
		{
			return;
		}

		if (P_DamageMobj(mobj, this, source(), 1, DMG_NORMAL))
		{
			P_InstaThrust(mobj, K_MomentumAngleReal(this), std::max(FixedHypot(momx, momy), kMinKnockback * mapobjectscale));
			K_StumblePlayer(mobj->player);

			mobj->player->spinouttimer = 1; // need invulnerability for one tic

			P_SetTarget(&mobj->player->flickyAttacker, this);

			controller()->mode(Controller::Mode::kAttached);
			controller()->intangible(leveltime + mobj->hitlag + kDamageCooldown);
		}

		S_StartSound(this, sfx_supflk);
	}

	void reflect()
	{
		momx = -(momx);
		momy = -(momy);
		P_SetObjectMomZ(this, 8*FRACUNIT, false);
	}

	void noclip(bool n)
	{
		constexpr UINT32 kNoClipFlags = MF_NOCLIP | MF_NOCLIPHEIGHT;
		flags = (flags & ~kNoClipFlags) | (kNoClipFlags * n);
	}

	void nerf()
	{
		light_up(false);

		flags &= ~(MF_NOGRAVITY|MF_NOCLIP|MF_NOCLIPHEIGHT);
	}

	void unnerf()
	{
		light_up(true);
		flags = info->flags;

		// Put this sound on a cooldown specifically because
		// it plays rapidly "at the edge of transform range".
		// I don't know why that happens so just never play
		// the sound close together.
		if (!soundtimer())
		{
			S_StartSound(this, sfx_s3k9f);
			soundtimer(2*TICRATE);
		}
	}

	void try_nerf()
	{
		if (owner_in_pain())
		{
			mode(Mode::kWeak);
			delay(2);
			nerf();
			flags |= MF_NOGRAVITY;
		}
	}

	void preserve_nerf()
	{
		if (owner_in_pain())
		{
			delay(kOwnerStunnedCooldown);
		}
	}

	void whip()
	{
		reflect();

		nerf();

		mode(Mode::kStunned);
		delay(kStunTime);

		S_StartSound(this, sfx_fhurt1);
	}

	void block()
	{
		reflect();

		mode(Mode::kStunned);
		delay(kBlockTime);
	}

	void land()
	{
		flags |= MF_NOGRAVITY;

		mode(Mode::kWeak);
	}
};

Flicky* Controller::flicky() const
{
	return static_cast<Flicky*>(controller_flicky(this));
}

void Controller::flicky(Flicky* n)
{
	P_SetTarget(&controller_flicky(this), n);
}

void Controller::search()
{
	if (ending())
	{
		return;
	}

	fixed_t nearestDistance = INT32_MAX;
	mobj_t* nearestMobj = nullptr;

	mobj_t* origin = chasing() ? chasing() : source();

	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		player_t* player = &players[i];
		mobj_t* mobj = player->mo;

		if (!playeringame[i] || P_MobjWasRemoved(mobj))
		{
			continue;
		}

		if (player->spectator)
		{
			continue;
		}

		// Do not retarget existing target or owner.
		if (mobj == chasing() || mobj == source())
		{
			continue;
		}

		// Target is already being hunted.
		if (player->flickyAttacker)
		{
			continue;
		}

		if (player->respawn.state != RESPAWNST_NONE)
		{
			continue;
		}

		fixed_t dist = FixedHypot(origin->x - mobj->x, origin->y - mobj->y);

		if (dist < kSearchRadius * mapobjectscale && dist < nearestDistance)
		{
			nearestDistance = dist;
			nearestMobj = mobj;
		}
	}

	if (nearestMobj)
	{
		if (chasing() && flicky())
		{
			// Detach flicky from swarm. This one keeps its previous target.
			// FIXME: when this one's target dies, it will
			// become dormant and not return to controller.
			flicky(flicky()->next());
		}

		chasing(flicky() ? nearestMobj : nullptr);
		mode(Mode::kEnRoute);

		// Update entire swarm
		for (Flicky* x = flicky(); x; x = x->next())
		{
			x->next_target(chasing());
			x->delay(x->phase() * kDelay);
		}
	}
}

void Controller::collect()
{
	// Resume searching once all Flickys return
	for (Flicky* x = flicky(); x; x = x->next())
	{
		if (x->mode() != Flicky::Mode::kReserved)
		{
			return;
		}
	}

	mode(Mode::kOrbit);
}

}; // namespace

void Obj_SpawnSuperFlickySwarm(player_t* owner, tic_t time)
{
	Controller* controller = Controller::spawn(owner, time);

	Flicky::spawn(controller, 0);
	Flicky::spawn(controller, 1);
	Flicky::spawn(controller, 2);
	Flicky::spawn(controller, 3);
}

void Obj_SuperFlickyThink(mobj_t* mobj)
{
	Flicky* x = static_cast<Flicky*>(mobj);

	if (!x->valid())
	{
		P_RemoveMobj(x);
		return;
	}

	x->animate();
	x->refocus();

	switch (x->mode())
	{
	case Flicky::Mode::kReserved:
		x->orbit();
		break;

	case Flicky::Mode::kHunting:
		x->try_nerf();
		x->chase();
		break;

	case Flicky::Mode::kStunned:
		break;

	case Flicky::Mode::kWeak:
		x->preserve_nerf();
		x->chase();
		break;
	}
}

void Obj_SuperFlickyControllerThink(mobj_t* mobj)
{
	Controller* x = static_cast<Controller*>(mobj);

	if (!x->valid())
	{
		P_RemoveMobj(x);
		return;
	}

	if (x->time_remaining() <= 1)
	{
		P_RemoveMobj(x);
		return;
	}

	x->angle += ANGLE_MAX / kOrbitSpeed;

	switch (x->mode())
	{
	case Controller::Mode::kDescend:
		x->descend();
		x->expand();
		break;

	case Controller::Mode::kOrbit:
		x->descend();
		x->expand();
		x->search();
		break;

	case Controller::Mode::kEnRoute:
		break;

	case Controller::Mode::kAttached:
		x->search();
		break;

	case Controller::Mode::kReturning:
		x->descend();
		break;
	}
}

void Obj_WhipSuperFlicky(mobj_t* t1)
{
	Flicky* x = static_cast<Flicky*>(t1);

	if (x->valid())
	{
		x->whip();
	}
}

void Obj_BlockSuperFlicky(mobj_t* t1)
{
	Flicky* x = static_cast<Flicky*>(t1);

	if (x->valid())
	{
		x->block();
	}
}

void Obj_SuperFlickyPlayerCollide(mobj_t* t1, mobj_t* t2)
{
	Flicky* x = static_cast<Flicky*>(t1);

	if (x->valid())
	{
		x->damage(t2);
	}
}

void Obj_SuperFlickyLanding(mobj_t* mobj)
{
	Flicky* x = static_cast<Flicky*>(mobj);

	if (x->valid())
	{
		x->land();
	}
}

void Obj_EndSuperFlickySwarm(mobj_t* mobj)
{
	Controller* x = static_cast<Controller*>(mobj);

	if (x->valid())
	{
		x->end();
	}
}

void Obj_ExtendSuperFlickySwarm(mobj_t* mobj, tic_t time)
{
	Controller* x = static_cast<Controller*>(mobj);

	x->expiry(x->expiry() + time);
}

tic_t Obj_SuperFlickySwarmTime(mobj_t* mobj)
{
	Controller* x = static_cast<Controller*>(mobj);

	return !P_MobjWasRemoved(x) ? x->powerup_remaining() : 0u;
}

mobj_t *Obj_SuperFlickyOwner(const mobj_t* mobj)
{
	const Flicky* x = static_cast<const Flicky*>(mobj);

	return x->valid() ? x->source() : nullptr;
}

boolean Obj_IsSuperFlickyWhippable(const mobj_t* mobj, const mobj_t* target)
{
	const Flicky* x = static_cast<const Flicky*>(mobj);

	return target == x->chasing() && !x->stunned();
}

boolean Obj_IsSuperFlickyTargettingYou(const mobj_t* mobj, mobj_t *player)
{
	const Flicky* x = static_cast<const Flicky*>(mobj);

	return player == x->chasing();
}
