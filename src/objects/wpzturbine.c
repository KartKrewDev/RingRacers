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

// distance to spawn the fan from the center:
#define TURBINE_RADIUS 128

// default distance at which players activate the turbine:
#define TURBINE_RANGE (2048*FRACUNIT)

// default distance at which players spin away from the turbine
#define TURBINE_SPIN 1536

// spawns the hnext visual list for the turbine.
// whether we use it or not will depend on its flags.
void Obj_WPZTurbineSpawn(mobj_t *mo)
{
	mobj_t *ptr = mo;
	UINT8 i;

	// spawn the visuals regardless of flags, make em invisible.
	// we'll care about updating em if it's worth doing later.

	for (i = 0; i < 8; i++)
	{
		mobj_t *vis = P_SpawnMobj(mo->x, mo->y, mo->z, MT_THOK);
		P_SetMobjState(vis, S_INVISIBLE);
		vis->tics = 4;	// if we don't use it just despawn it later.

		P_SetTarget(&ptr->hnext, vis);
		ptr = vis;
	}
}

// visually update the turbine's hnext visuals if need be.
static void Obj_WPZTurbineUpdate(mobj_t *mo)
{
	angle_t ang = (angle_t)mo->extravalue1;
	mapthing_t *mt = mo->spawnpoint;

	if (!mt)
		return;

	// fans
	if (!mt->thing_args[1])
	{
		UINT8 i;
		mobj_t *ptr = mo;

		for (i = 0; i < 8; i++)
		{

			fixed_t x = mo->x + FixedMul(mapobjectscale, TURBINE_RADIUS*FINECOSINE(ang>>ANGLETOFINESHIFT));
			fixed_t y = mo->y + FixedMul(mapobjectscale, TURBINE_RADIUS*FINESINE(ang>>ANGLETOFINESHIFT));

			// get the mobj
			if (ptr && !P_MobjWasRemoved(ptr) && ptr->hnext && !P_MobjWasRemoved(ptr->hnext))
			{
				ptr = ptr->hnext;
				P_MoveOrigin(ptr, x, y, mo->z);
				ptr->tics = 4;
				ptr->sprite = SPR_WPWL;
				ptr->frame = 1|FF_PAPERSPRITE;
				P_SetScale(ptr, mapobjectscale*4);
				ptr->destscale = mapobjectscale*4;
				ptr->angle = ang;
			}

			ang += (360/8)*ANG1;
		}
	}

// Not my code, no clue why this doesn't work, John Fucking Madden
#if 0
	// bubbles if we're underwater
	if (mo->z < mo->watertop && leveltime%10 == 0)
	{

		INT32 dradius = TURBINE_SPIN;
		INT32 bubbleradius;
		angle_t bubbleang;
		fixed_t bx, by, bz;
		mobj_t *bubble;

		if (mt->thing_args[7])
			dradius = mt->thing_args[7];

		bubbleradius = P_RandomRange(PR_BUBBLE, dradius/4, (dradius*3)/2);
		bubbleang = P_RandomRange(PR_BUBBLE, 0, 359)*ANG1;

		bx = mo->x + FixedMul(mapobjectscale, bubbleradius*FINECOSINE(bubbleang>>ANGLETOFINESHIFT));
		by = mo->y + FixedMul(mapobjectscale, bubbleradius*FINECOSINE(bubbleang>>ANGLETOFINESHIFT));
		bz = R_PointInSubsector(bx, by)->sector->floorheight;

		bubble = P_SpawnMobj(bx, by, bz, MT_WATERPALACEBUBBLE);
		bubble->fuse = TICRATE*10;
		bubble->angle = bubbleang;
		bubble->movecount = bubbleradius;
		P_SetTarget(&bubble->tracer, mo);
	}
#endif
}

void Obj_WPZTurbineThinker(mobj_t *mo)
{

	UINT8 i;
	mapthing_t *mt = mo->spawnpoint;
	boolean opt1 = mt->thing_args[0] != 0;
	fixed_t baseheight = (mt->thing_args[2]) ? (mt->thing_args[2]*FRACUNIT) : (mo->z+mapobjectscale*1024);
	fixed_t sneakerheight = (mt->thing_args[3]) ? (mt->thing_args[3]*FRACUNIT) : (mo->z+mapobjectscale*1768);
	fixed_t range = (mt->thing_args[7]) ? (mt->thing_args[7]*FRACUNIT) : (FixedMul(mapobjectscale, TURBINE_RANGE));
	INT32 rotspeed = (mt->thing_args[5]) ? (mt->thing_args[5]*ANG1/10) : (ANG1*3);	// not angle_t for negatives.
	tic_t turbinetime = (mt->thing_args[4]) ? (mt->thing_args[4]) : (TICRATE*3);
	SINT8 mult = (opt1) ? (-1) : (1);

	mo->extravalue1 += rotspeed*mult;

	// find players in range and take their phones.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *p;
		mobj_t *pmo;

		if (!playeringame[i] || players[i].spectator || K_isPlayerInSpecialState(&players[i]))
			continue;

		p = &players[i];
		pmo = p->mo;

		if (R_PointToDist2(pmo->x, pmo->y, mo->x, mo->y) < range
		&& !p->turbine
		&& p->respawn.state == RESPAWNST_NONE)
		{
			P_SetTarget(&pmo->tracer, mo);
			p->turbine = turbinetime;

			// to be fully honest i dont rememebr what i was on while writing this
			// but it originally went by mo instead of pmo for angle??? how did it ever *work* ?
			p->turbineangle = ANGLE_180 + R_PointToAngle2(0, 0, mo->momx, mo->momy);
			if (!p->speed)
				p->turbineangle = ANGLE_180 + mo->angle;

			p->turbineangle += ANG1*45*mult;

			p->turbineheight = baseheight;
			p->turbinespd = false;

			if (FixedDiv(p->speed, K_GetKartSpeed(p, false, false)) > FRACUNIT*2	// 200% speed
			&& baseheight != sneakerheight)
			{
				p->turbineheight = sneakerheight;
				p->turbinespd = true;
			}

			pmo->flags |= MF_NOCLIP;
		}
	}
	Obj_WPZTurbineUpdate(mo);
}

// ported from my lua for convenience of not having to rewrite half the shit code.
static INT32 angtoint(angle_t a)
{
	return a/ANG1;
}

// controls player while using a turbine.
// i do not remember what i smoked before writing the lua version of this code.
// it's a fucking mess what the fuck does half of this even DO
void Obj_playerWPZTurbine(player_t *p)
{
	mobj_t *pmo = p->mo;
	mobj_t *t = pmo->tracer;
	mapthing_t *mt;
	boolean opt1;
	fixed_t dist = FixedMul(mapobjectscale, TURBINE_SPIN)*FRACUNIT;
	INT32 speed = ANG1*3;
	boolean mode = false;
	boolean distreached;

	fixed_t tx, ty, tz;
	fixed_t momz;


	if (!t || P_MobjWasRemoved(t) || p->respawn.state != RESPAWNST_NONE)
	{
		p->turbine = false;
		P_SetTarget(&pmo->tracer, NULL);
		return;	// wtf happened
	}

	if (t->type != MT_WATERPALACETURBINE)
	{
		p->turbine = false;
		return;
	}

	mt = t->spawnpoint;
	pmo->flags &= ~MF_NOGRAVITY;

	opt1 = (mt->thing_args[0] != 0);

	if (mt->thing_args[6])
		dist = mt->thing_args[6]*FRACUNIT;

	if (mt->thing_args[5])
		speed = mt->thing_args[5]*ANG1/10;

	if (mt->thing_args[9])
		mode = true;

	distreached = R_PointToDist2(t->x, t->y, pmo->x, pmo->y) <= dist+32*mapobjectscale;

	if (mode && !distreached)
		p->turbineangle = (INT32)R_PointToAngle2(t->x, t->y, pmo->x, pmo->y);

	pmo->pitch = 0;

	// determine target x/y/z
	tx = t->x + (dist/FRACUNIT)*FINECOSINE((angle_t)(p->turbineangle)>>ANGLETOFINESHIFT);
	ty = t->y + (dist/FRACUNIT)*FINESINE((angle_t)(p->turbineangle)>>ANGLETOFINESHIFT);
	tz = p->turbineheight;

	//CONS_Printf("%d %d\n", tx/FRACUNIT, ty/FRACUNIT);

	if (mode)
	{
		if (distreached)
		{
			P_MoveOrigin(pmo, tx, ty, pmo->z);
		}
		else
		{
			pmo->momx = FixedMul(FINECOSINE((angle_t)(p->turbineangle)>>ANGLETOFINESHIFT), -(max(p->speed, mapobjectscale*32)));
			pmo->momy = FixedMul(FINESINE((angle_t)(p->turbineangle)>>ANGLETOFINESHIFT), -(max(p->speed, mapobjectscale*32)));
		}
	}
	else
	{
		pmo->momx = (tx - pmo->x)/24 * (p->turbinespd ? 2 : 1);
		pmo->momy = (ty - pmo->y)/24 * (p->turbinespd ? 2 : 1);
	}

	momz = (tz - pmo->z)/128 * (p->turbinespd+1);

	if (mt->thing_args[8])
	{
		momz = (mt->thing_args[8]*FRACUNIT) * ((tz < pmo->z) ? -1 : 1);

		if (momz < 0)
		{
			if (pmo->z + momz < tz)
			{
				momz = pmo->z - tz;
			}
		}
		else if (momz > 0)
		{
			if (pmo->z + momz > tz)
			{
				momz = tz - pmo->z;
			}
		}

	}

	pmo->momz = momz;
	p->turbineangle += (speed * (p->turbinespd ? 2 : 1)) * (opt1 ? -1 : 1);
	P_SetPlayerAngle(p, (angle_t)p->turbineangle + ANGLE_90*(opt1 ? -1 : 1));

	if (pmo->eflags & MFE_UNDERWATER)
	{
		fixed_t rx = pmo->x + P_RandomRange(PR_DECORATION, -64, 64)*mapobjectscale;
		fixed_t ry = pmo->y + P_RandomRange(PR_DECORATION, -64, 64)*mapobjectscale;
		fixed_t rz = pmo->z + P_RandomRange(PR_DECORATION, -64, 64)*mapobjectscale;

		mobj_t *bubl = P_SpawnMobj(rx, ry, rz, MT_THOK);
		P_SetScale(bubl, pmo->scale*2);
		bubl->scalespeed = pmo->scale/12;
		bubl->destscale = 1;
		bubl->sprite = SPR_BUBL;
		bubl->frame = 0;
		bubl->tics = TICRATE;
	}


	if (pmo->momz < mapobjectscale*6)
	{
		INT32 myang = angtoint(pmo->angle);
		angle_t exitangle = t->angle;
		INT32 targetangle = angtoint(exitangle);
		INT32 launchangle = myang-targetangle;

		// WHAT WAS I SMOKING
		if ( (opt1 && launchangle > -60 && launchangle < -45)
		|| (!opt1 && launchangle > 45 && launchangle < 60))
		{

			P_SetPlayerAngle(p, targetangle*ANG1);

			if (mode)
				P_InstaThrust(pmo, targetangle*ANG1, 128*mapobjectscale);
			else
			{
				fixed_t spd = FixedHypot(pmo->momx, pmo->momy);
				P_InstaThrust(pmo, targetangle*ANG1, spd);
			}

			P_SetTarget(&pmo->tracer, NULL);
			p->turbineheight = 0;
			p->turbineangle = 0;

			if (p->turbinespd)
				pmo->momz = mapobjectscale*5 * (pmo->eflags & MFE_UNDERWATER ? 2 : 1);

			if (pmo->eflags & MFE_UNDERWATER)
			{
				pmo->momz = mapobjectscale*5;
				pmo->momx = (pmo->momx*17)/10;
				pmo->momy = (pmo->momy*17)/10;
			}

			pmo->flags &= ~MF_NOCLIP;
		}
	}
}

// bubbles that circle the turbine
void Obj_WPZBubbleThink(mobj_t *mo)
{
	angle_t ang = mo->angle - ANGLE_90;
	mobj_t *t = mo->tracer;
	fixed_t tx, ty;
	mapthing_t *mt;

	// where
	// where did it go
	if (!t || P_MobjWasRemoved(t))
	{
		P_RemoveMobj(mo);
		return;
	}

	mt = t->spawnpoint;
	if (!mt)
		return;

	mo->momz = mapobjectscale*16;
	tx = t->x + FixedMul(mapobjectscale, mo->movecount*FINECOSINE(ang>>ANGLETOFINESHIFT));
	ty = t->y + FixedMul(mapobjectscale, mo->movecount*FINESINE(ang>>ANGLETOFINESHIFT));

	mo->momx = (tx - mo->x)/24;
	mo->momy = (ty - mo->y)/24;

	if (leveltime & 1)
	{
		fixed_t rx = mo->x + P_RandomRange(PR_DECORATION, -64, 64)*mapobjectscale;
		fixed_t ry = mo->y + P_RandomRange(PR_DECORATION, -64, 64)*mapobjectscale;
		fixed_t rz = mo->z + P_RandomRange(PR_DECORATION, -64, 64)*mapobjectscale;
		mobj_t *bubl = P_SpawnMobj(rx, ry, rz, MT_THOK);
		P_SetScale(bubl, mapobjectscale*4);
		bubl->destscale = 1;
		bubl->sprite = SPR_BUBL;
		bubl->frame = 0;
		bubl->tics = TICRATE;
	}

	mo->angle += 3*ANG1 * (mt->thing_args[0] ? -1 : 1);

	if (mo->z > mo->watertop || mo->z > mo->ceilingz)
		P_RemoveMobj(mo);
}
