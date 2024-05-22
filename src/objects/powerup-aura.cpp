// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../doomtype.h"
#include "../info.h"
#include "../g_game.h"
#include "../m_fixed.h"
#include "../k_objects.h"
#include "../k_powerup.h"
#include "../p_local.h"
#include "../p_mobj.h"
#include "../tables.h"
#include "../k_kart.h"

// copied from objects/monitor.c
#define FINE90 (FINEANGLES/4)
#define FINE180 (FINEANGLES/2)
#define TRUETAN(n) FINETANGENT(FINE90 + (n)) // bruh

#define part_theta(o) ((o)->movedir)
#define part_seek(o) ((o)->extravalue1)

namespace
{

constexpr int kSpriteWidth = 32;
constexpr int kNumSides = 6;

struct Aura : mobj_t
{
	angle_t theta() const { return part_theta(this); }
	void theta(angle_t n) { part_theta(this) = n; }

	unsigned seek() const { return part_seek(this); }
	void seek(unsigned n) { part_seek(this) = n; }

	mobj_t* origin() const { return players[seek()].mo; }

	static void spawn(int player)
	{
		const fixed_t angle_factor = ANGLE_MAX / kNumSides;

		angle_t ang = 0u;

		for (int i = 0; i < kNumSides; ++i)
		{
			Aura* x = static_cast<Aura*>(P_SpawnMobj(0, 0, 0, MT_POWERUP_AURA));

			x->theta(ang);
			x->seek(player);

			ang += angle_factor;
		}
	}

	// copied from objects/monitor.c
	static fixed_t get_inradius(fixed_t length)
	{
		return FixedDiv(length, 2 * TRUETAN(FINE180 / kNumSides));
	}

	bool valid() const
	{
		if (seek() >= MAXPLAYERS)
		{
			return false;
		}

		if (!playeringame[seek()])
		{
			return false;
		}

		if (!K_AnyPowerUpRemaining(&players[seek()]))
		{
			return false;
		}

		return true;
	}

	void move()
	{
		if (P_MobjWasRemoved(origin()))
		{
			return;
		}
		
		K_FlipFromObject(this, origin());
		fixed_t flipoffset = P_IsObjectFlipped(origin()) ? origin()->height : 0;

		P_MoveOrigin(this, origin()->x, origin()->y, origin()->z - flipoffset);
		P_InstaScale(this, 11 * origin()->scale / 10);

		translate();

		if (K_AnyPowerUpRemaining(&players[seek()]) & ~POWERUP_BIT(POWERUP_BARRIER))
		{
			renderflags &= ~RF_DONTDRAW;
		}
		else
		{
			renderflags |= RF_DONTDRAW;
		}
	}

	void translate()
	{
		const fixed_t width = scale * kSpriteWidth;
		const fixed_t rad = get_inradius(width);
		const angle_t ang = theta() + origin()->angle;

		angle = (ang - ANGLE_90);

		sprxoff = FixedMul(FCOS(ang), rad);
		spryoff = FixedMul(FSIN(ang), rad);
	}
};

}; // namespace

void Obj_SpawnPowerUpAura(player_t* player)
{
	Aura::spawn(player - players);
}

void Obj_PowerUpAuraThink(mobj_t* mobj)
{
	Aura* x = static_cast<Aura*>(mobj);

	if (!x->valid())
	{
		P_RemoveMobj(x);
		return;
	}

	x->move();
}
