// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  eggball.c
/// \brief Leaf Storm giant Eggman Balls. And their spawner. Yes, that sounds horribly wrong. No, I'm not changing it.

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

#define BALLMINSPAWNTIME 3
#define BALLMAXSPAWNTIME 5

// spawns balls every BALLMINSPAWNTIME to BALLMAXSPAWNTIME seconds.
void Obj_EggBallSpawnerThink(mobj_t *mo)
{
	if (!mo->extravalue1)
	{
		mobj_t *ball = P_SpawnMobj(mo->x, mo->y, mo->z, MT_LSZ_EGGBALL);
		if (P_MobjWasRemoved(ball) == false)
		{
			ball->angle = mo->angle;
			P_SetScale(ball, (ball->destscale = 6*mapobjectscale));
		}

		mo->extravalue1 = P_RandomRange(PR_TRACKHAZARD, TICRATE*BALLMINSPAWNTIME, TICRATE*BALLMAXSPAWNTIME);
	}
	mo->extravalue1--;
}

// ball thinker, it's mostly for particles and some bouncing n stuff to make em fancy.
// vars:
// threshold -> prevmomz
// movedir -> prevz

void Obj_EggBallThink(mobj_t *mo)
{
	const boolean onground = P_IsObjectOnGround(mo);

	if (mo->eflags & MFE_JUSTHITFLOOR)
	{
		if (mo->extravalue1 && P_CheckDeathPitCollide(mo))
		{
			P_RemoveMobj(mo);
			return;
		}

		if (mo->threshold && mo->threshold < -10*mapobjectscale)
		{
			UINT8 i;

			mo->momz = (fixed_t)(-mo->threshold)/8;

			for (i=0; i<16; i++)
			{
				angle_t an = ANG1;
				mobj_t *dust = P_SpawnMobj(mo->x, mo->y, mo->z, MT_DRIFTDUST);
				P_SetScale(dust, mapobjectscale*3);
				P_InstaThrust(dust, (360/16)*an*i, mapobjectscale*24);	// the angle thing is to avoid a warning due to overflows.
				dust->momz = P_RandomRange(PR_DECORATION, 0, 7)*mapobjectscale;
			}

			S_StartSound(mo, sfx_s3k59);

			P_StartQuakeFromMobj(10, 15 * mo->scale, 512 * mo->scale, mo);
		}
	}

	if (!mo->extravalue1)
	{
		if (onground)
		{
			mo->extravalue1 = 1;
			mo->cusval = 24*mapobjectscale;
			mo->movedir = mo->z;
		}
	}
	else
	{
		if (onground && (mo->extravalue2 & 1))
		{
			fixed_t dx = mo->x + P_RandomRange(PR_DECORATION, -96, 96)*mapobjectscale - mo->momx*2;
			fixed_t dy = mo->y + P_RandomRange(PR_DECORATION, -96, 96)*mapobjectscale - mo->momy*2;
			fixed_t dz = mo->z;

			mobj_t *dust = P_SpawnMobj(dx, dy, dz, MT_DRIFTDUST);
			P_SetScale(dust, mapobjectscale*3);
			dust->momz = P_RandomRange(PR_DECORATION, 0, 7)*mapobjectscale;
			dust->destscale = mapobjectscale*8;
		}

		P_InstaThrust(mo, mo->angle, mo->cusval);
		mo->extravalue2 += 1;
		mo->frame = mo->extravalue2 % (24 * 2) / 2; // 24 is for frame Y.

		// build up speed
		if (onground)
		{
			if (mo->eflags & MFE_VERTICALFLIP)
			{
				if (mo->z > (fixed_t)mo->movedir)
				{
					mo->cusval += max(mapobjectscale/32, abs(mo->z - (fixed_t)mo->movedir)/16);
				}
			}
			else
			{
				if (mo->z < (fixed_t)mo->movedir)
				{
					mo->cusval += max(mapobjectscale/32, abs(mo->z - (fixed_t)mo->movedir)/16);
				}
			}
		}

		mo->movedir = mo->z;
	}
	mo->threshold = mo->momz;
}
