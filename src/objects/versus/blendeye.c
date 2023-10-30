// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2023 by Vivian "toastergrl" Grannell
// Copyright (C) 2018-2023 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_boss.h
/// \brief Blend Eye boss encounter

#include "../../p_local.h"
#include "../../m_random.h"
#include "../../k_boss.h"
#include "../../s_sound.h"
#include "../../r_main.h" // R_PointToAngle2, R_PointToDist2

boolean VS_PuyoTouched(mobj_t *special, mobj_t *toucher)
{
	if (!special->health || !toucher->health)
		return false; // too dead

	if (special->state-states < S_BLENDEYE_PUYO)
		return false; // too small

	P_DamageMobj(toucher, special, special->tracer, 1, DMG_NORMAL);

	special->momx = 0;
	special->momy = 0;
	special->momz = 0;

	return true;
}

void VS_PuyoDeath(mobj_t *mobj)
{
	mobjtype_t dusttype = (encoremode ? MT_BLENDEYE_PUYO_DUST : MT_BLENDEYE_PUYO_DUST_COFFEE);
	UINT8 i;
	fixed_t momx, momy;
	mobj_t *dustmo;

	mobj->renderflags &= ~RF_DONTDRAW;
	mobj->rollangle = 0;

	mobj->angle = FixedAngle(P_RandomKey(PR_DECORATION, 360)*FRACUNIT);
	for (i = 0; i <= 2; i++)
	{
		momx = P_ReturnThrustX(mobj, mobj->angle, 3*mobj->scale);
		momy = P_ReturnThrustY(mobj, mobj->angle, 3*mobj->scale);

		dustmo = P_SpawnMobjFromMobj(mobj, 0, 0, 0, dusttype);
		dustmo->momx = mobj->momx + momx;
		dustmo->momy = mobj->momy + momy;
		dustmo->momz = mobj->momz + 4*mobj->scale;
		dustmo->movedir = dustmo->sprite = mobj->movedir;

		dustmo = P_SpawnMobjFromMobj(mobj, 0, 0, 0, dusttype);
		dustmo->momx = mobj->momx - momx;
		dustmo->momy = mobj->momy - momy;
		dustmo->momz = mobj->momz - 4*mobj->scale;
		dustmo->movedir = dustmo->sprite = mobj->movedir;

		mobj->angle += ANGLE_135;
	}
	S_StartSound(NULL, ((mobj->tracer && mobj->tracer->type != MT_SPIKEDTARGET) ? sfx_mbs4c : sfx_mbs45));
}
