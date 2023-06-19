#include "../p_mobj.h"
#include "../p_local.h"
#include "../k_objects.h"

#define SYMBOL_SCALE (2<<FRACBITS)
#define SYMBOL_ZOFFSET (19<<FRACBITS)
#define SYMBOL_BOBRANGE (64<<FRACBITS)
#define SYMBOL_BOBSPEED (5<<FRACBITS)
#define SYMBOL_OPTIONS 32

void Obj_SymbolSpawn(mobj_t *mobj)
{
	mobj->extravalue1 = mobj->z;
	mobj->extravalue2 = FixedMul(mobj->x + mobj->y, mapobjectscale);
}

void Obj_SymbolSetup(mobj_t *mobj, mapthing_t *mthing)
{
	fixed_t oldHeight = mobj->height;
	statenum_t stateNum = mobj->info->spawnstate + (mthing->args[0] % SYMBOL_OPTIONS);

	mobj->angle += ANGLE_90;
	P_SetScale(mobj, mobj->destscale = 4 * FixedMul(mobj->scale, SYMBOL_SCALE));
	mobj->z += 4 * FixedMul(mapobjectscale, SYMBOL_ZOFFSET) * P_MobjFlip(mobj);

	if (mthing->options & MTF_OBJECTFLIP)
	{
		mobj->z += oldHeight - mobj->height;
	}

	mobj->extravalue1 = mobj->old_z = mobj->z;
	P_SetMobjState(mobj, stateNum);
}

void Obj_SymbolThink(mobj_t *mobj)
{
	fixed_t offset = FixedMul(mapobjectscale,
		FixedMul(SYMBOL_BOBRANGE,
			FixedMul(FINESINE(FixedAngle(leveltime * SYMBOL_BOBSPEED + mobj->extravalue2) >> ANGLETOFINESHIFT) + FRACUNIT, FRACUNIT >> 1)
		)
	);

	mobj->z = mobj->extravalue1 + P_MobjFlip(mobj) * offset;
}
