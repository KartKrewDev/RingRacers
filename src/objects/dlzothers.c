// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dlzothers.c
/// \brief Dead Line Zone other objects (Hover + Ring Vaccum), they're small enough that we can just lump em together instead of having 2 more small files...

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_respawn.h"
#include "../k_collide.h"

// Hover:
void Obj_DLZHoverSpawn(mobj_t *mo)
{
	P_SetScale(mo, mapobjectscale*4);
	mo->destscale = mapobjectscale*4;
}

// collision between MT_PLAYER and hover
void Obj_DLZHoverCollide(mobj_t *mo, mobj_t *mo2)
{
	player_t *p = mo->player;

	if (!p || p->lasthover == leveltime)
		return;

	if (abs(mo->z - mo2->z) < 512*mapobjectscale)
	{
		// momz adjust
		if (mo2->eflags & MFE_VERTICALFLIP)
		{
			if (mo->momz > -16*mapobjectscale)
			{
				mo->momz -= 8*mapobjectscale;
			}
		}
		else
		{
			if (mo->momz < 16*mapobjectscale)
			{
				mo->momz += 8*mapobjectscale;
			}
		}

		// speed adjust
		if (p->speed > K_GetKartSpeed(p, false, false))
			P_Thrust(mo, R_PointToAngle2(0, 0, -mo->momx, -mo->momy), mapobjectscale/16);

		if (!S_SoundPlaying(mo, sfx_s3kc6s))
			S_StartSound(mo, sfx_s3kc6s);

		p->lasthover = leveltime;
	}
}

// Ring Vaccum:
void Obj_DLZRingVaccumSpawn(mobj_t *mo)
{
	P_SetScale(mo, mapobjectscale*4);
	mo->destscale = mapobjectscale*4;
}

// collision between MT_FLINGRING and ring vaccum
void Obj_DLZRingVaccumCollide(mobj_t *mo, mobj_t *mo2)
{
	mobj_t *fake;

	if (mo->z + mo->height < mo2->z)
		return;

	if (mo->z > mo2->z + mo2->height)
		return;

	if (!P_IsObjectOnGround(mo) || mo->momz)
		return;

	fake = P_SpawnMobj(mo->x, mo->y, mo->z, MT_DLZ_SUCKEDRING);
	P_SetScale(fake, mo->scale);
	fake->scalespeed = mapobjectscale/64;
	fake->destscale = 1;

	P_SetTarget(&fake->target, mo2);

	fake->angle = R_PointToAngle2(mo2->x, mo2->y, fake->x, fake->y);
	fake->movefactor = R_PointToDist2(mo2->x, mo2->y, fake->x, fake->y);

	P_RemoveMobj(mo);
}

void Obj_DLZSuckedRingThink(mobj_t *mo)
{
	mobj_t *t = mo->target;
	fixed_t x, y;

	// commit die if the target disappears for some fucking reason
	if (!t || P_MobjWasRemoved(t))
	{
		P_RemoveMobj(mo);
		return;
	}

	x = t->x + FixedMul(mo->movefactor, FINECOSINE(mo->angle>>ANGLETOFINESHIFT));
	y = t->y + FixedMul(mo->movefactor, FINESINE(mo->angle>>ANGLETOFINESHIFT));

	P_MoveOrigin(mo, x, y, mo->z);

	if (mo->cusval < 24)
		mo->cusval++;

	mo->angle += mo->cusval*ANG1;

	if (mo->cusval > 8 && mo->movefactor)
		mo->movefactor -= 1;

	if (mo->scale < mapobjectscale/12)
		P_RemoveMobj(mo);
}
