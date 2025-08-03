// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../k_objects.h"

#include "../doomdef.h"
#include "../info.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../k_hitlag.h"

#define ball_pad(o) ((o)->target)
#define ball_instawhipped(o) ((o)->extravalue1) // see instawhip collide

#define ball_cooldown(o) ((o)->cvmem)

#define ball_activedefer(o) ((o)->extravalue2)
#define ball_activator(o) ((o)->tracer)

namespace
{

struct BallSwitch_Pad : mobj_t
{
	statenum_t Anim() const { return static_cast<statenum_t>(this->state - states); }
	void Anim(statenum_t n)
	{
		if (Anim() != n)
		{
			P_SetMobjState(this, n);
		}
	}

	void Spawned()
	{
		renderflags |= RF_FLOORSPRITE|RF_NOSPLATBILLBOARD|RF_SLOPESPLAT|RF_NOSPLATROLLANGLE;
	}

	void Tick(boolean active)
	{
		if (active == true)
		{
			Anim(S_BALLSWITCH_PAD_ACTIVE);
		}
		else
		{
			Anim(S_BALLSWITCH_PAD);
		}
	}
};

struct BallSwitch_Ball : mobj_t
{
	BallSwitch_Pad *Pad() const { return static_cast<BallSwitch_Pad *>( ball_pad(this) ); }
	void Pad(BallSwitch_Pad *n) { P_SetTarget(&ball_pad(this), n); }

	statenum_t Anim() const { return static_cast<statenum_t>(this->state - states); }
	void Anim(statenum_t n)
	{
		if (Anim() != n)
		{
			P_SetMobjState(this, n);
		}
	}

	INT32 Cooldown() const { return ball_cooldown(this); }
	void Cooldown(INT32 n) { ball_cooldown(this) = n; }
	boolean Active() const { return (ball_cooldown(this) != 0); }

	boolean DeferActivation() const { return ball_activedefer(this); }
	mobj_t *Activator() const { return ball_activator(this); }

	void DeferActivation(boolean n, mobj_t *src)
	{
		ball_activedefer(this) = n;
		P_SetTarget(&ball_activator(this), src);
	}

	SINT8 IntSign(int value) const
	{
		if (value > 0)
		{
			return 1;
		}

		if (value < 0)
		{
			return -1;
		}

		return 0;
	}

	void Spawned()
	{
		Pad( static_cast<BallSwitch_Pad *>( P_SpawnMobjFromMobj(this, 0, 0, 0, MT_BALLSWITCH_PAD) ) );
		Pad()->Spawned();

		this->z += Pad()->height * P_MobjFlip(this);
	}

	void Tick()
	{
		if (P_MobjWasRemoved(Pad()) == true)
		{
			P_RemoveMobj(this);
			return;
		}

		ball_instawhipped(this) = 0;

		if (DeferActivation() == true)
		{
			P_ActivateThingSpecial(this, Activator());
			Cooldown(-1); // maybe later?
			DeferActivation(false, nullptr);
		}

		fixed_t ourZ = P_GetMobjFeet(this);
		fixed_t theirZ = P_GetMobjHead(Pad());

		fixed_t dist = P_AproxDistance(P_AproxDistance(Pad()->x - this->x, Pad()->y - this->y), theirZ - ourZ);
		fixed_t move = P_AproxDistance(P_AproxDistance(this->momx, this->momy), this->momz);

		constexpr INT32 accelScale = 4;

		if (dist < accelScale * this->scale && move < accelScale * this->scale)
		{
			P_SetOrigin(this, Pad()->x, Pad()->y, theirZ);
			this->momx = this->momy = this->momz = 0;
		}
		else
		{
			static constexpr const INT32 accel[2] = { FRACUNIT*3/4, FRACUNIT*3/16 };
			constexpr fixed_t frict = FRACUNIT*99/100;

			this->momx = FixedMul(this->momx, frict);
			this->momy = FixedMul(this->momy, frict);
			this->momz = FixedMul(this->momz, frict);

			SINT8 xSign = IntSign(Pad()->x - this->x);
			SINT8 ySign = IntSign(Pad()->y - this->y);
			SINT8 zSign = IntSign(theirZ - ourZ);

			boolean xAway = (IntSign(this->momx) == xSign);
			boolean yAway = (IntSign(this->momy) == ySign);
			boolean zAway = (IntSign(this->momz) == zSign);

			this->momx += FixedMul(accel[xAway], accelScale * this->scale) * xSign;
			this->momy += FixedMul(accel[yAway], accelScale * this->scale) * ySign;
			this->momz += FixedMul(accel[zAway], accelScale * this->scale) * zSign;

			this->angle += FixedAngle(move * 2);
			if (dist > this->radius * 2)
			{
				P_Thrust(this, this->angle, (move / accelScale) * 2 / 3);
			}
		}

		if (Active() == true)
		{
			INT32 cool = Cooldown();
			if (cool > 0)
			{
				Cooldown(cool - 1);
			}

			Anim(S_BALLSWITCH_BALL_ACTIVE);
		}
		else
		{
			Anim(S_BALLSWITCH_BALL);
		}

		Pad()->Tick(Active());
	}

	void Push(mobj_t *toucher, const fixed_t pushValue, const fixed_t repelValue)
	{
		fixed_t push = FixedMul(pushValue, toucher->scale);
		fixed_t repel = FixedMul(repelValue, this->scale);

		angle_t thrustAngle = R_PointToAngle2(toucher->x, toucher->y, this->x, this->y);
		fixed_t thrustAngleCos = FINECOSINE(thrustAngle >> ANGLETOFINESHIFT);
		fixed_t thrustAngleSin =   FINESINE(thrustAngle >> ANGLETOFINESHIFT);

		fixed_t thisZ = this->z + (this->height / 2);
		fixed_t toucherZ = toucher->z + (toucher->height / 2);

		angle_t thrustPitch = R_PointToAngle2(0, toucherZ, R_PointToDist2(toucher->x, toucher->y, this->x, this->y), thisZ);
		fixed_t thrustPitchCos = FINECOSINE(thrustPitch >> ANGLETOFINESHIFT);
		fixed_t thrustPitchSin =   FINESINE(thrustPitch >> ANGLETOFINESHIFT);

		this->momx += FixedMul(FixedMul(push, thrustAngleCos), thrustPitchCos);
		this->momy += FixedMul(FixedMul(push, thrustAngleSin), thrustPitchCos);
		this->momz += FixedMul(push, thrustPitchSin);

		toucher->momx -= FixedMul(FixedMul(repel, thrustAngleCos), thrustPitchCos);
		toucher->momy -= FixedMul(FixedMul(repel, thrustAngleSin), thrustPitchCos);
		toucher->momz -= FixedMul(repel, thrustPitchSin);
	}

	void Touch(mobj_t *toucher)
	{
		Push(toucher, 4 << FRACBITS, 6 << FRACBITS);
	}

	void Hit(mobj_t *inflictor, mobj_t *source)
	{
		if (inflictor->type == MT_BUBBLESHIELD && source->player)
		{
			if (source->player->pflags2 & PF2_BUBBLECONTACT)
				return;
			source->player->pflags2 |= PF2_BUBBLECONTACT;
		}

		Push(inflictor, 64 << FRACBITS, 1 << FRACBITS);
		K_SetHitLagForObjects(this, inflictor, source, 4, true);

		if (Active() == false)
		{
			DeferActivation(true, source);
		}
	}
};

}; // namespace

void Obj_BallSwitchInit(mobj_t *mobj)
{
	BallSwitch_Ball *ball = static_cast<BallSwitch_Ball *>(mobj);
	ball->Spawned();
}

void Obj_BallSwitchThink(mobj_t *mobj)
{
	BallSwitch_Ball *ball = static_cast<BallSwitch_Ball *>(mobj);
	ball->Tick();
}

void Obj_BallSwitchTouched(mobj_t *mobj, mobj_t *toucher)
{
	BallSwitch_Ball *ball = static_cast<BallSwitch_Ball *>(mobj);
	ball->Touch(toucher);
}

void Obj_BallSwitchDamaged(mobj_t *mobj, mobj_t *inflictor, mobj_t *source)
{
	BallSwitch_Ball *ball = static_cast<BallSwitch_Ball *>(mobj);
	ball->Hit(inflictor, source);
}
