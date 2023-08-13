#include "../k_battle.h"
#include "../k_objects.h"
#include "../info.h"
#include "../m_random.h"
#include "../p_local.h"

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

void Obj_EmeraldThink(mobj_t *emerald)
{
	if (emerald->threshold > 0)
	{
		emerald->threshold--;
	}

	A_AttractChase(emerald);

	Obj_SpawnEmeraldSparks(emerald);

	K_BattleOvertimeKiller(emerald);
}
