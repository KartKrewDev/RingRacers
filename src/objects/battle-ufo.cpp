// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by SteelT.
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cstddef>
#include <iterator>

#include "../math/fixed.hpp"
#include "../mobj.hpp"
#include "../mobj_list.hpp"

#include "../command.h"
#include "../doomdef.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../k_battle.h"
#include "../k_objects.h"
#include "../k_powerup.h"
#include "../k_kart.h"
#include "../k_hud.h" // K_AddMessage

using srb2::math::Fixed;
using srb2::Mobj;
using srb2::MobjList;

extern "C" consvar_t cv_battleufotest;

extern mobj_t* svg_battleUfoSpawners;

#define BATTLEUFO_LEG_ZOFFS (3*FRACUNIT) // Spawn height offset from the body
#define BATTLEUFO_LEGS (3) // Number of UFO legs to spawn
#define BATTLEUFO_BOB_AMP (4) // UFO bob strength
#define BATTLEUFO_BOB_SPEED (TICRATE*2) // UFO bob speed

namespace
{

struct Spawner : Mobj
{
	void thing_args() = delete;
	INT32 id() const { return this->mobj_t::thing_args[0]; }

	void hnext() = delete;
	Spawner* next() const { return Mobj::hnext<Spawner>(); }
	void next(Spawner* n) { Mobj::hnext(n); }
};

struct UFO : Mobj
{
	void target() = delete;
	Spawner* spawner() const { return Mobj::target<Spawner>(); }
	void spawner(Spawner* n) { Mobj::target(n); }

	void spawn_beam()
	{
		Mobj *x = spawn_from<Mobj>({0, 0, sprzoff() + 26}, MT_BATTLEUFO_BEAM);

		x->renderflags |= RF_FLOORSPRITE|RF_NOSPLATBILLBOARD|RF_SLOPESPLAT|RF_NOSPLATROLLANGLE;
		x->colorized = true;
		x->color = SKINCOLOR_SAPPHIRE;
	}

	void bob()
	{
		// Copied and slightly modified from k_kart.c
		Fixed sine = (BATTLEUFO_BOB_AMP * Fixed {FSIN(M_TAU_FIXED * BATTLEUFO_BOB_SPEED * leveltime)}) / 4;

		momz = flip(sine * scale());
	}
};

class SpawnerList
{
private:
	MobjList<Spawner, svg_battleUfoSpawners> list_;

public:
	void insert(Spawner* spawner) { list_.push_front(spawner); }
	void erase(Spawner* spawner) { list_.erase(spawner); }

	Spawner* next(INT32 order) const
	{
		using T = const Spawner*;

		auto it = std::find_if(list_.begin(), list_.end(), [order](T p) { return order < p->id(); });
		auto min = [&](auto cmp) { return std::min_element(list_.begin(), list_.end(), cmp); };

		return *(it != list_.end()
			? min([order](T a, T b) { return order < a->id() && (b->id() <= order || a->id() < b->id()); })
			: min([](T a, T b) { return a->id() < b->id(); }));
	}

	INT32 random_id() const
	{
		if (list_.empty())
		{
			return 0;
		}

		auto it = list_.begin();
		std::size_t count = std::distance(it, list_.end());

		if (count > 1u)
		{
			std::advance(it, P_RandomKey(PR_BATTLEUFO, count - 1u));
		}

		return it->id();
	}

	void spawn_ufo() const
	{
		if (list_.empty())
		{
			return;
		}

		if (exitcountdown)
		{
			return;
		}

		Fixed ofs = mobjinfo[MT_BATTLEUFO].height / 4;

		Spawner* spawner = next(g_battleufo.previousId);
		UFO* ufo = static_cast<UFO*>(P_SpawnMobjFromMobj(spawner, 0, 0, 250*FRACUNIT - ofs, MT_BATTLEUFO));

		K_AddMessage("Crack the Combat UFO!", true, false);
		S_StartSound(NULL, sfx_mbs54);

		ufo->sprzoff(ofs * spawner->scale());

		ufo->spawner(spawner);

		ufo->extravalue1 = 0; // Lifetime
	}
};

SpawnerList g_spawners;

}; // namespace

void Obj_BattleUFOThink(mobj_t *mobj)
{
	UFO* ufo = static_cast<UFO*>(mobj);

	ufo->extravalue1++;

	ufo->bob();

	if ((leveltime/2) & 1)
	{
		ufo->spawn_beam();
	}

	if (!exitcountdown && (ufo->extravalue1 % (TICRATE*2)) == 0)
	{
		S_StartSound(ufo, sfx_s3ka5);
	}

	if (!battleovertime.enabled && ufo->extravalue1 <= 5*TICRATE)
	{
		Obj_PointPlayersToXY(mobj->x, mobj->y);
	}

	K_BattleOvertimeKiller(mobj);
}

void Obj_BattleUFODeath(mobj_t *mobj, mobj_t *inflictor)
{
	UFO* ufo = static_cast<UFO*>(mobj);
	const SINT8 flip = P_MobjFlip(ufo);
	const kartitems_t pwrup = static_cast<kartitems_t>(P_RandomRange(PR_BATTLEUFO, FIRSTPOWERUP, LASTPOWERUP));

	ufo->momz = -(8*mapobjectscale)/2;

	if (!P_MobjWasRemoved(inflictor) && inflictor->type == MT_INSTAWHIP &&
		!P_MobjWasRemoved(inflictor->target) && inflictor->target->player)
	{
		// Just give it to the player, they earned it.
		K_GivePowerUp(inflictor->target->player, pwrup, BATTLE_POWERUP_TIME);
	}
	else
	{
		mobj_t *drop = K_CreatePaperItem(
			ufo->x,
			ufo->y,
			ufo->z + ufo->sprzoff() + (flip),
			0,
			flip,
			pwrup,
			BATTLE_POWERUP_TIME
		);

		drop->hitlag = ufo->hitlag();
	}

	if (ufo->spawner())
	{
		g_battleufo.previousId = ufo->spawner()->id();
		g_battleufo.due = leveltime + (cv_battleufotest.value ? 1 : BATTLE_UFO_TIME);
	}
}

void Obj_SpawnBattleUFOLegs(mobj_t *ufo)
{
	INT32 i;
	angle_t ang = 0;
	const fixed_t angle_factor = ANGLE_MAX / BATTLEUFO_LEGS;

	for (i = 0; i < BATTLEUFO_LEGS; i++)
	{
		mobj_t *leg = P_SpawnMobjFromMobj(ufo, 0, 0, BATTLEUFO_LEG_ZOFFS, MT_BATTLEUFO_LEG);
		P_SetTarget(&leg->target, ufo);
		ang += angle_factor;
		leg->angle = ang;
	}
}

void Obj_BattleUFOLegThink(mobj_t *leg)
{
	if (!leg->target || P_MobjWasRemoved(leg->target))
	{
		P_RemoveMobj(leg);
		return;
	}

	// Rotate around the UFO
	if (leg->target->health > 0)
	{
		leg->angle += FixedAngle(leg->info->speed);

		const angle_t fa = leg->angle>>ANGLETOFINESHIFT;
		const fixed_t radius = FixedMul(14*leg->info->speed, leg->target->scale);
		fixed_t x = leg->target->x + FixedMul(FINECOSINE(fa),radius);
		fixed_t y = leg->target->y + FixedMul(FINESINE(fa),radius);

		// TODO: Take gravflip into account
		P_MoveOrigin(leg, x, y, leg->z);
		leg->sprzoff = leg->target->sprzoff;
	}

	leg->momz = leg->target->momz;
	leg->fuse = leg->target->fuse;

	if (leg->target->hitlag)
	{
		leg->hitlag = leg->target->hitlag;
		leg->eflags |= (leg->target->eflags & MFE_DAMAGEHITLAG);
	}
}

void Obj_LinkBattleUFOSpawner(mobj_t *spawner)
{
	g_spawners.insert(static_cast<Spawner*>(spawner));
}

void Obj_UnlinkBattleUFOSpawner(mobj_t *spawner)
{
	g_spawners.erase(static_cast<Spawner*>(spawner));
}

void Obj_SpawnBattleUFOFromSpawner(void)
{
	g_spawners.spawn_ufo();
}

INT32 Obj_RandomBattleUFOSpawnerID(void)
{
	return g_spawners.random_id();
}

void Obj_BattleUFOBeamThink(mobj_t *beam)
{
	P_SetObjectMomZ(beam, beam->info->speed, true);
}

INT32 Obj_BattleUFOSpawnerID(const mobj_t *spawner)
{
	return static_cast<const Spawner*>(spawner)->id();
}

mobj_t *Obj_GetNextUFOSpawner(void)
{
	return g_spawners.next(g_battleufo.previousId);
}
