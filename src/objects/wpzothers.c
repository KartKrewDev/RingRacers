// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  wpzturbine.c
/// \brief Water Palace Zone turbines and associated bubble object. Yep, this is going to suck.

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

// foutains
void Obj_WPZFountainThink(mobj_t *mo)
{
	if (mo->state == &states[S_WPZFOUNTAIN]
	&& !(mo->eflags & MFE_UNDERWATER))
		P_SetMobjState(mo, S_WPZFOUNTAINANIM);

	else if (mo->state == &states[S_WPZFOUNTAINANIM]
	&& mo->eflags & MFE_UNDERWATER)
		P_SetMobjState(mo, S_WPZFOUNTAIN);
}

// kuragens
void Obj_WPZKuragenThink(mobj_t *mo)
{
	//(void)mo;
	boolean active = false;

	// .....and i need to do this... because?
	if (!mo->cusval)
	{
		P_SetScale(mo, mapobjectscale*2);
		mo->destscale = mapobjectscale*2;
		mo->cusval = 1;
	}

	if (!(mo->spawnpoint->options & 1 || mo->spawnpoint->thing_args[0]))	// extra flag skips player checks, making it a decoration.
	{
		UINT8 i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t *p;
			mobj_t *pmo;

			if (!playeringame[i] || players[i].spectator)
				continue;

			p = &players[i];
			pmo = p->mo;

			if (R_PointToDist2(pmo->x, pmo->y, mo->x, mo->y) < mapobjectscale*6144)
			{
				active = true;
				break;
			}
		}
	}

	if (active && mo->extravalue1)
	{
		mo->extravalue1--;

		if (!mo->extravalue1)
		{
			mobj_t *b = P_SpawnMobj(mo->x, mo->y, mo->z, MT_KURAGENBOMB);
			b->flags2 = mo->flags2 & MF2_OBJECTFLIP;
			P_SetScale(b, mapobjectscale*2);
			b->destscale = mapobjectscale*2;
			mo->extravalue1 = TICRATE*5;
		}
	}
	else
		mo->extravalue1 = TICRATE*5/2;
}

// kuragen bomb
void Obj_WPZKuragenBombThink(mobj_t *mo)
{
	if (P_IsObjectOnGround(mo))
	{
		P_SetScale(mo, mapobjectscale/2);
		P_RadiusAttack(mo, mo, FRACUNIT*192, DMG_EXPLODE, false);
		A_MineExplode(mo);

		P_RemoveMobj(mo);
	}
}
