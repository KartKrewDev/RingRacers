#include "../doomdef.h"
#include "../p_local.h"
#include "../k_objects.h"

#define BATTLEUFO_LEG_ZOFFS (3*FRACUNIT) // Spawn height offset from the body
#define BATTLEUFO_LEGS (3) // Number of UFO legs to spawn
#define BATTLEUFO_BOB_AMP (4) // UFO bob strength
#define BATTLEUFO_BOB_SPEED (TICRATE*2) // UFO bob speed

void Obj_BattleUFOThink(mobj_t *ufo)
{
	// Copied and slightly modified from k_kart.c
	fixed_t sine = FixedMul(ufo->scale, BATTLEUFO_BOB_AMP * FINESINE((((M_TAU_FIXED * BATTLEUFO_BOB_SPEED) * leveltime) >> ANGLETOFINESHIFT) & FINEMASK));
	fixed_t targz = FixedMul(ufo->scale, sine) * P_MobjFlip(ufo);
	ufo->momz = targz;
}

void Obj_BattleUFODeath(mobj_t *ufo)
{
	ufo->momz = -(8*mapobjectscale)/2;
	ufo->fuse = TICRATE;
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

	if (leg->target->hitlag)
	{
		leg->hitlag = leg->target->hitlag;
		leg->eflags |= (leg->target->eflags & MFE_DAMAGEHITLAG);
	}
}
