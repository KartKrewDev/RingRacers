#include "../k_battle.h"
#include "../k_objects.h"
#include "../info.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../tables.h"

void Obj_SpawnEmeraldSparks(mobj_t *mobj)
{
	if (leveltime % 3 != 0)
	{
		return;
	}

	mobj_t *sparkle = P_SpawnMobjFromMobj(
		mobj,
		P_RandomRange(PR_SPARKLE, -48, 48) * FRACUNIT,
		P_RandomRange(PR_SPARKLE, -48, 48) * FRACUNIT,
		P_RandomRange(PR_SPARKLE, 0, 64) * FRACUNIT,
		MT_EMERALDSPARK
	);

	sparkle->color = mobj->color;
	sparkle->momz += 8 * mobj->scale * P_MobjFlip(mobj);
	sparkle->sprzoff = mobj->sprzoff;
}

static void Obj_EmeraldOrbitPlayer(mobj_t *emerald)
{
	const int kOrbitTics = 64;
	const int kPhaseTics = 128;

	const fixed_t orbit_radius = 100 * mapobjectscale;
	const fixed_t orbit_height = 30 * mapobjectscale;

	mobj_t *targ = emerald->target;

	angle_t a = emerald->angle;

	fixed_t x = FixedMul(orbit_radius, FCOS(a));
	fixed_t y = FixedMul(orbit_radius, FSIN(a));

	angle_t phase = (ANGLE_MAX / kPhaseTics) * (leveltime % kPhaseTics);

	P_MoveOrigin(
			emerald,
			targ->x + x,
			targ->y + y,
			targ->z + targ->height + FixedMul(orbit_height, FSIN(a + phase))
	);

	emerald->angle += ANGLE_MAX / kOrbitTics;
}

void Obj_EmeraldThink(mobj_t *emerald)
{
	if (!P_MobjWasRemoved(emerald->target))
	{
		switch (emerald->target->type)
		{
			case MT_SPECIAL_UFO:
				Obj_UFOEmeraldThink(emerald);
				break;

			case MT_PLAYER:
				Obj_EmeraldOrbitPlayer(emerald);
				break;

			default:
				break;
		}

		return;
	}

	if (emerald->threshold > 0)
	{
		emerald->threshold--;
	}

	A_AttractChase(emerald);

	Obj_SpawnEmeraldSparks(emerald);

	K_BattleOvertimeKiller(emerald);
}
