#include <algorithm>
#include <iterator>
#include <set>

#include "../doomdef.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../k_battle.h"
#include "../k_objects.h"
#include "../k_kart.h"

#define BATTLEUFO_LEG_ZOFFS (3*FRACUNIT) // Spawn height offset from the body
#define BATTLEUFO_LEGS (3) // Number of UFO legs to spawn
#define BATTLEUFO_BOB_AMP (4) // UFO bob strength
#define BATTLEUFO_BOB_SPEED (TICRATE*2) // UFO bob speed

#define spawner_id(o) ((o)->thing_args[0])

#define ufo_spawner(o) ((o)->target)

namespace
{

struct Spawner : mobj_t
{
	INT32 id() const { return spawner_id(this); }
};

struct UFO : mobj_t
{
	Spawner* spawner() const { return static_cast<Spawner*>(ufo_spawner(this)); }
	void spawner(Spawner* n) { P_SetTarget(&ufo_spawner(this), n); }
	void spawn_beam()
	{
		mobj_t *x;

		x = P_SpawnMobjFromMobj(this, 0, 0, FixedDiv(this->height / 4, this->scale), MT_BATTLEUFO_BEAM);
		x->renderflags |= RF_FLOORSPRITE|RF_NOSPLATBILLBOARD|RF_SLOPESPLAT|RF_NOSPLATROLLANGLE;
		x->colorized = true;
		x->color = SKINCOLOR_SAPPHIRE;
	}
};

struct SpawnerCompare
{
	bool operator()(const Spawner* a, const Spawner* b) const
	{
		return a->id() < b->id();
	}
};

class SpawnerList
{
private:
	std::set<Spawner*, SpawnerCompare> set_;

public:
	void insert(Spawner* spawner)
	{
		auto [it, inserted] = set_.insert(spawner);

		if (inserted)
		{
			mobj_t* dummy = nullptr;
			P_SetTarget(&dummy, spawner);
		}
	}

	void erase(Spawner* spawner)
	{
		if (set_.erase(spawner))
		{
			mobj_t* dummy = spawner;
			P_SetTarget(&dummy, nullptr);
		}
	}

	Spawner* next(INT32 order) const
	{
		auto it = std::upper_bound(
			set_.begin(),
			set_.end(),
			order,
			[](INT32 a, const Spawner* b) { return a < b->id(); }
		);

		return it != set_.end() ? *it : *set_.begin();
	}

	INT32 random_id() const
	{
		if (set_.empty())
		{
			return 0;
		}

		auto it = set_.begin();

		std::advance(it, P_RandomKey(PR_BATTLEUFO, set_.size()));

		return (*std::prev(it == set_.begin() ? set_.end() : it))->id();
	}

	void spawn_ufo() const
	{
		if (set_.empty())
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

	// Copied and slightly modified from k_kart.c
	fixed_t sine = (BATTLEUFO_BOB_AMP * FINESINE((((M_TAU_FIXED * BATTLEUFO_BOB_SPEED) * leveltime) >> ANGLETOFINESHIFT) & FINEMASK)) / 4;
	fixed_t targz = FixedMul(ufo->scale, sine) * P_MobjFlip(ufo);
	ufo->momz = targz;

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

INT32 Obj_GetFirstBattleUFOSpawnerID(void)
{
	return g_spawners.random_id();
}

void Obj_ResetUFOSpawners(void)
{
	g_spawners = {};
}

void Obj_BattleUFOBeamThink(mobj_t *beam)
{
	P_SetObjectMomZ(beam, beam->info->speed, true);

	if (P_IsObjectOnGround(beam))
	{
		P_RemoveMobj(beam);
	}
}
