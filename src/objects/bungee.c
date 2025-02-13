// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  bungee.c
/// \brief Leaf Storm bungee interaction/behaviour code to be used in other files.

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

#define BUNGEE_NONE 0
#define BUNGEE_LATCH 1
#define BUNGEE_LAUNCH 2

// Touching the bungee, used in p_inter.c
void Obj_BungeeSpecial(mobj_t *mo, player_t *p)
{

	mobj_t *latch;

	if (P_IsObjectOnGround(p->mo) || p->springstars || K_isPlayerInSpecialState(p))
		return;

	P_InstaThrust(p->mo, 0, 0);
	p->bungee = BUNGEE_LATCH;
	p->mo->flags |= MF_NOCLIPTHING;	// prevent players from bumping if they latch onto the same bungee.
	p->pflags |= PF_NOFASTFALL;		// didn't know this flag existed but it's very convenient!!

	latch = P_SpawnMobj(p->mo->x, p->mo->y, p->mo->z, MT_THOK);
	P_SetMobjState(latch, S_INVISIBLE);
	latch->angle = mo->angle;

	S_StartSound(mo, sfx_s3k5a);
	P_SetTarget(&p->mo->tracer, latch);
}

// this is the thinker to call on the player when they get bungee'd.
void Obj_playerBungeeThink(player_t *p)
{

	mobj_t *bungee = p->mo->tracer;
	UINT8 i;

	// someone removed it
	if (!bungee || P_MobjWasRemoved(bungee))
		return;

	bungee->tics = 4;	// we set this to a low value so that it despawns if the player vanishes for some reason.

	if (p->bungee == BUNGEE_LATCH)
	{
		// rr has super high gravity which gets in the way.
		p->mo->flags |= MF_NOGRAVITY;
		p->mo->momz = (p->mo->momz*9)/10;

		if (abs(p->mo->momz) < 6*mapobjectscale)
		{
			p->bungee = BUNGEE_LAUNCH;
			p->mo->momz = P_MobjFlip(p->mo)*mapobjectscale;
			S_StartSound(p->mo, sfx_s3k81);
		}
	}
	else if (p->bungee == BUNGEE_LAUNCH)
	{
		p->mo->momz = (p->mo->momz*12)/10;

		// if we go above/below (depending on our flip flags) the bungee, release us!
		if ((p->mo->eflags & MFE_VERTICALFLIP && p->mo->z < bungee->z)
		|| (!(p->mo->eflags & MFE_VERTICALFLIP) && p->mo->z > bungee->z ))
		{
			P_InstaThrust(p->mo, bungee->angle, p->mo->momz/8);
			p->mo->momz = (p->mo->momz*3)/4;

			p->springstars = TICRATE;	// these are used as a buffer not to latch to vines again.
			p->springcolor = SKINCOLOR_EMERALD;

			Obj_EndBungee(p);
			return;
		}
	}

	// basic visuals (but hey they work fine enough!)
	for (i=0; i<8; i++)
	{
		fixed_t xpos = -(bungee->x - p->mo->x) /8 *i;
		fixed_t ypos = -(bungee->y - p->mo->y) /8 *i;
		fixed_t zpos = -(bungee->z - p->mo->z) /8 *i;

		mobj_t *seg = P_SpawnMobj(bungee->x + xpos, bungee->y + ypos, bungee->z + zpos, MT_THOK);

		P_SetScale(seg, mapobjectscale/3);
		seg->color = SKINCOLOR_EMERALD;
		seg->frame = 0;
		seg->fuse = 2;
	}
}

void Obj_EndBungee(player_t *p)
{
	if (p->bungee == BUNGEE_NONE)
	{
		return;
	}

	p->pflags &= ~PF_NOFASTFALL;
	p->bungee = BUNGEE_NONE;

	if (!P_MobjWasRemoved(p->mo))
	{
		p->mo->flags &= ~MF_NOGRAVITY;
		p->mo->flags &= ~MF_NOCLIPTHING;

		if (!P_MobjWasRemoved(p->mo->tracer))
		{
			P_RemoveMobj(p->mo->tracer);
		}
		P_SetTarget(&p->mo->tracer, NULL);
	}
}
