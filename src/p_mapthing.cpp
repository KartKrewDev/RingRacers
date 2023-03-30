#include "doomstat.h" // mapobjectscale
#include "info.h"
#include "m_fixed.h"
#include "p_local.h" // ONFLOORZ
#include "p_mobj.h"
#include "p_slopes.h"
#include "r_defs.h"
#include "r_main.h"

fixed_t P_GetMobjSpawnHeight(
	const mobjtype_t mobjtype,
	const fixed_t x,
	const fixed_t y,
	const fixed_t dz,
	const fixed_t offset,
	const boolean flip,
	const fixed_t scale
)
{
	const fixed_t finalScale = FixedMul(scale, mapobjectscale);
	const sector_t* sector = R_PointInSubsector(x, y)->sector;

	// Axis objects snap to the floor.
	if (mobjtype == MT_AXIS || mobjtype == MT_AXISTRANSFER || mobjtype == MT_AXISTRANSFERLINE)
		return ONFLOORZ;

	// Establish height.
	if (flip)
		return P_GetSectorCeilingZAt(sector, x, y) - dz - FixedMul(finalScale, offset + mobjinfo[mobjtype].height);
	else
		return P_GetSectorFloorZAt(sector, x, y) + dz + FixedMul(finalScale, offset);
}
