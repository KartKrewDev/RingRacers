// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  waterfall-particle.c
/// \brief Waterfall particle spawner.

#include "../p_local.h"
#include "../m_random.h"
#include "../k_objects.h"

void Obj_WaterfallParticleThink(mobj_t *mo)
{
	const INT32 radius = 320;
	const INT32 momz = (mo->spawnpoint->angle) ? mo->spawnpoint->angle : 8;

	INT32 x = P_RandomRange(PR_DECORATION, -radius, radius)*mapobjectscale;
	INT32 y = P_RandomRange(PR_DECORATION, -radius, radius)*mapobjectscale;

	mobj_t *particle = P_SpawnMobj(mo->x + x , mo->y + y , mo->z, MT_THOK);

	P_SetMobjState(particle, S_WATERFALLPARTICLE);
	particle->momx = P_RandomRange(PR_DECORATION, -5, 5)*mapobjectscale;
	particle->momy = P_RandomRange(PR_DECORATION, -5, 5)*mapobjectscale;
	particle->momz = P_RandomRange(PR_DECORATION, momz/2, momz)*mapobjectscale*P_MobjFlip(mo);

	P_InstaScale(particle, 3 * particle->scale);

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		particle->eflags |= MFE_VERTICALFLIP;
		particle->flags2 |= MF2_OBJECTFLIP;
	}
}
