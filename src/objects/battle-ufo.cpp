#include <algorithm>
#include <cstddef>
#include <iterator>

#include "../math/fixed.hpp"
#include "../mobj.hpp"
#include "../mobj_list.hpp"

#include "../doomdef.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../k_battle.h"
#include "../k_objects.h"
#include "../k_kart.h"

using srb2::math::Fixed;
using srb2::Mobj;
using srb2::MobjList;

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
		Mobj *x = spawn_from<Mobj>({0, 0, height / 4}, MT_BATTLEUFO_BEAM);

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
			? min([order](T a, T b) { return order < a->id() && a->id() < b->id(); })
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

		return std::next(it, P_RandomKey(PR_BATTLEUFO, count - 1u))->id();
	}

	void spawn_ufo() const
	{
		if (list_.empty())
		{
			return;
		}

		Spawner* spawner = next(g_battleufo.previousId);
		UFO* ufo = static_cast<UFO*>(P_SpawnMobjFromMobj(spawner, 0, 0, 250*FRACUNIT, MT_BATTLEUFO));

		ufo->spawner(spawner);
	}
};

SpawnerList g_spawners;

}; // namespace

void Obj_BattleUFOThink(mobj_t *mobj)
{
	UFO* ufo = static_cast<UFO*>(mobj);

	ufo->bob();

	if ((leveltime/2) & 1)
	{
		ufo->spawn_beam();
	}

	if (!battleovertime.enabled)
	{
		Obj_PointPlayersToXY(mobj->x, mobj->y);
	}

	K_BattleOvertimeKiller(mobj);
}

void Obj_BattleUFODeath(mobj_t *mobj)
{
	UFO* ufo = static_cast<UFO*>(mobj);
	const SINT8 flip = P_MobjFlip(ufo);

	ufo->momz = -(8*mapobjectscale)/2;

	K_CreatePaperItem(
		ufo->x,
		ufo->y,
		ufo->z + (flip),
		0,
		flip,
		P_RandomRange(PR_BATTLEUFO, FIRSTPOWERUP, LASTPOWERUP),
		BATTLE_POWERUP_TIME
	);

	if (ufo->spawner())
	{
		g_battleufo.previousId = ufo->spawner()->id();
		g_battleufo.due = leveltime + BATTLE_UFO_TIME;
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

	if (P_IsObjectOnGround(beam))
	{
		P_RemoveMobj(beam);
	}
}
