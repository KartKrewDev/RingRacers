// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by "Lat'"
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  rideroid.c
/// \brief Rideroid / Rideroid Node object code. This also has the player behaviour code to be used in k_kart.

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
#include "../k_color.h"

#define NODERADIUS 260
#define NODEPULLOK 48
#define NODEROTSPEED ANG1

#define RIDEROIDSPEED 80
#define RIDEROIDMAXADD 8


// static functions that only really get used here...
static void plr_undoRespawn(player_t *p)
{
	p->respawn.state = 0;
	p->respawn.timer = 0;
}

static void plr_resetRideroidVars(player_t *p)
{
	p->rdnodepull = false;
	p->rideroid = false;
	p->rideroidangle = 0;
	p->rideroidspeed = 0;
	p->rideroidrollangle = 0;

	p->rdaddmomx = 0;
	p->rdaddmomy = 0;
	p->rdaddmomz = 0;

	P_SetTarget(&p->mo->tracer, NULL);
}

// kills the rideroid and removes it from the map.
static void Obj_killRideroid(mobj_t *mo)
{
	UINT8 i;

	for (i = 0; i < 32; i++)
	{
		mobj_t *t = P_SpawnMobj(mo->x, mo->y, mo->z, MT_THOK);
		t->color = SKINCOLOR_TEAL;
		t->frame = FF_FULLBRIGHT;
		t->destscale = 1;
		t->momx = P_RandomRange(PR_EXPLOSION, -32, 32)*mapobjectscale;
		t->momy = P_RandomRange(PR_EXPLOSION, -32, 32)*mapobjectscale;
		t->momz = P_RandomRange(PR_EXPLOSION, -32, 32)*mapobjectscale;
	}
	P_RemoveMobj(mo);
}

// makes the player get off of the rideroid.
void Obj_getPlayerOffRideroid(mobj_t *mo)
{
	mobj_t *pmo = mo->target;

	if (pmo && !P_MobjWasRemoved(pmo))
	{
		player_t *p = pmo->player;

		pmo->flags &= ~MF_NOGRAVITY;

		if (p)
			plr_resetRideroidVars(p);

		mo->fuse = TICRATE/2;
		mo->momx = mo->momx*2;
		mo->momy = mo->momy*2;
		mo->momz = 0;
		mo->target = NULL;

		S_StartSound(mo, sfx_ridr4);

	}
}

// this assumes mo->target and mo->target->player is valid.
// if it's not, uuuh well too bad.
static void Obj_explodeRideroid(mobj_t *mo)
{
	mobj_t *pmo = mo->target;

	Obj_getPlayerOffRideroid(mo);
	K_SpawnMineExplosion(pmo, pmo->color, 3);
	S_StartSound(pmo, sfx_s3k4e);
	Obj_killRideroid(mo);

	// @TODO: quake.

}

// used to create a smooth trail.
static fixed_t Obj_rideroidLerp(INT32 start, INT32 finish, INT32 percent)
{
	return start + FixedMul(finish-start, FRACUNIT-percent);
}

static void Obj_rideroidTrail(mobj_t *mo)
{
	mobj_t *pmo = mo->target;
	player_t *p = NULL;

	UINT8 i, j;

	angle_t h_an = mo->angle + ANG1*90;

	if (pmo && !P_MobjWasRemoved(pmo))
	{
		p = pmo->player;	// used to make some graphics local to save on framerate
		mo->color = pmo->color;
		mo->colorized = pmo->colorized;
	}
	// from here, we will use the following:
	// extravalue1: prev x
	// extravalue2: prev y
	// cusval: prev z
	// cvmem: prev roll angle

	for (j = 0; j < 9; j++)
	{
		for (i = 0; i < 2; i++)
		{
			INT32 percent = FRACUNIT * (10-j)/10;
			angle_t roll = (angle_t)Obj_rideroidLerp((angle_t)mo->cvmem, mo->rollangle, percent);
			fixed_t x = (fixed_t)Obj_rideroidLerp((fixed_t)mo->extravalue1, mo->x, percent);
			fixed_t y = (fixed_t)Obj_rideroidLerp((fixed_t)mo->extravalue2, mo->y, percent);
			fixed_t z = (fixed_t)Obj_rideroidLerp((fixed_t)mo->cusval, mo->z, percent);

			angle_t v_an = i ? (roll+ANG1*90) : (roll-ANG1*90);

			fixed_t pos = FixedMul(mo->scale, FINESINE(v_an>>ANGLETOFINESHIFT)*60);
			fixed_t tx = x+FixedMul(FINECOSINE(h_an>>ANGLETOFINESHIFT), pos);
			fixed_t ty = y+FixedMul(FINESINE(h_an>>ANGLETOFINESHIFT), pos);
			fixed_t tz = z+FixedMul(FINECOSINE(v_an>>ANGLETOFINESHIFT)*60, mo->scale);

			mobj_t *t = P_SpawnMobj(tx, ty, tz, MT_THOK);
			t->color = SKINCOLOR_TEAL;
			t->frame = FF_FULLBRIGHT|FF_TRANS50;
			// 120 is no magic number, the base scale speed is mapobjectscale/12
			P_SetScale(t, max(1, mapobjectscale*5/6 - ((10-j)*mapobjectscale/120)));
			t->destscale = 1;

			if (p)
			{
				if (j)
					t->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(p));

				if (p->startboost)
					t->color = K_RainbowColor(leveltime);
			}

		}
	}

	mo->extravalue1 = (INT32)mo->x;
	mo->extravalue2 = (INT32)mo->y;
	mo->cusval = (INT32)mo->z;
	mo->cvmem = (INT32)mo->rollangle;
}


static void Obj_updateRideroidPos(mobj_t *mo)
{
	mobj_t *pmo = mo->target;

	fixed_t x = pmo->x + 2*FINECOSINE(pmo->angle>>ANGLETOFINESHIFT);
	fixed_t y = pmo->y + 2*FINESINE(pmo->angle>>ANGLETOFINESHIFT);

	P_MoveOrigin(mo, x, y, pmo->z - 10*mapobjectscale);
	mo->momx = pmo->momx;
	mo->momy = pmo->momy;
	mo->momz = pmo->momz;

	Obj_rideroidTrail(mo);
}

// handles the rideroid and the player attached to it.
void Obj_RideroidThink(mobj_t *mo)
{
	player_t *p;
	mobj_t *pmo = mo->target;

	fixed_t basemomx;
	fixed_t basemomy;
	fixed_t xthreshold;
	fixed_t ythreshold;


	// speed values...
	fixed_t maxspd = RIDEROIDSPEED*mapobjectscale;

	if (!pmo || P_MobjWasRemoved(pmo))
	{
		if (!mo->fuse)
		{
			mo->fuse = TICRATE/2;
		}
		else if (mo->fuse == 1)
		{
			Obj_killRideroid(mo);
		}
		else
		{
			Obj_rideroidTrail(mo);
			mo->rollangle += ANG1*24;
		}
		return;
	}

	// if we're here, our player should still exist which is kinda crazy!
	p = pmo->player;

	// pulling towards the node, AKA towards where the rideroid is, which just so happens to be us right now.
	if (p->rdnodepull)
	{
		pmo->momx = (mo->x - pmo->x)/6;
		pmo->momy = (mo->y - pmo->y)/6;
		pmo->momz = (mo->z - pmo->z)/6;

		//CONS_Printf("%d\n", R_PointToDist2(mo->x, mo->y, pmo->x, pmo->y)/FRACUNIT);

		if (R_PointToDist2(mo->x, mo->y, pmo->x, pmo->y) < NODEPULLOK*mapobjectscale)
		{
			p->rideroid = true;
			p->rdnodepull = false;

			S_StartSound(pmo, sfx_ridr2);
		}

		return;
	}

	// if we're here, we made it to the rideroid and we can use it, or something like that!

	P_SetTarget(&p->mo->tracer, mo);	// keep a reference of the rideroid in the player for convenience.

	// calculate the maximum speed we can move at.
	// the values are a little arbitrary but they work for how little use these have.

	if (p->ringboost)
		maxspd = (maxspd*12)/10;	// Ring Boost: 120% max speed.

	if (p->draftpower)
	{
		UINT8 draftperc = (p->draftpower*100 / FRACUNIT);	// 0-100%
		maxspd += (draftperc/5) / 100;
	}

	if (p->startboost)
		maxspd = (maxspd*15)/10;	// 150% speed

	// increase speed as we go unless we're turning harshly.
	if (p->rideroidspeed*mapobjectscale < maxspd)
	{
		if (abs(p->cmd.turning < 400))
			p->rideroidspeed += (p->ringboost ? 2 : 1);	// acceleration is also higher with a ring boost.
	}
	else
		p->rideroidspeed -= 1;


	// sounds

	mo->movecount++;	// we use this as a timer for sounds and whatnot.

	if (mo->movecount == 1 || !(mo->movecount%TICRATE))
		S_StartSound(mo, sfx_ridr3);


	// aaaaand the actual gameplay and shit... wooooo
	pmo->angle = mo->angle;
	pmo->flags |= MF_NOGRAVITY;

	// do not let the player touch the ground
	// @TODO: check all 4 corners of the player and use P_GetZAt to account for slopes if pmo->standslope isn't NULL.
	// right now it's not important as LV doesn't mix rdr and slopes but if somehow i manage to pull through w this shit it'll need to be done
	if (pmo->eflags & MFE_VERTICALFLIP)
	{
		fixed_t minz = pmo->ceilingz - 2*mapobjectscale;
		if (pmo->z > minz)
			pmo->z = minz;
	}
	else
	{
		fixed_t minz = pmo->floorz + 2*mapobjectscale;
		if (pmo->z < minz)
			pmo->z = minz;
	}


	// if we hit a wall or get hit, get off of the rideroid.
	if (pmo->eflags & MFE_JUSTBOUNCEDWALL || P_PlayerInPain(p) || p->respawn.state != RESPAWNST_NONE)
	{
		Obj_explodeRideroid(mo);
		return;
	}

	// now actual movement:

	// first, do the movement for this frame
	P_InstaThrust(pmo, (angle_t)p->rideroidangle, p->rideroidspeed*mapobjectscale);
	basemomx = p->mo->momx;
	basemomy = p->mo->momy;

	pmo->momx += p->rdaddmomx;
	pmo->momy += p->rdaddmomy;
	pmo->momz += p->rdaddmomz;
	pmo->angle = (angle_t)p->rideroidangle;
	p->drawangle = (angle_t)p->rideroidangle;
	P_SetPlayerAngle(pmo->player, (angle_t)p->rideroidangle);
	pmo->rollangle = p->rideroidrollangle;
	mo->rollangle = p->rideroidrollangle;
	pmo->pitch = 0;

	// update the rideroid object (me) to be below the target player
	Obj_updateRideroidPos(mo);

	// turning left/right
	if (p->cmd.turning)
	{
		fixed_t savemomx = pmo->momx;
		fixed_t savemomy = pmo->momy;
		SINT8 dir = 0;
		INT32 a;

		if (p->cmd.turning < -400)
		{
			a = (INT32)(mo->angle) - ANG1*90;
			P_Thrust(pmo, mo->angle - ANGLE_90, 2*mapobjectscale);
			p->rideroidrollangle -= ANG1*3;

			if (p->rideroidrollangle < -ANG1*25)
				p->rideroidrollangle = -ANG1*25;

			dir = 1;

		}
		else if (p->cmd.turning > 400)
		{
			a = (INT32)(mo->angle) + ANG1*90;
			P_Thrust(pmo, mo->angle + ANGLE_90, 2*mapobjectscale);
			p->rideroidrollangle += ANG1*3;

			if (p->rideroidrollangle > ANG1*25)
				p->rideroidrollangle = ANG1*25;

			dir = -1;
		}

		if (dir != 0 && leveltime & 1 && p->rideroidspeed > RIDEROIDSPEED/2)
		{
			p->rideroidspeed -= 1;
		}

		if (dir != 0)
		{

			// save the added momentum
			p->rdaddmomx = pmo->momx - basemomx;
			p->rdaddmomy = pmo->momy - basemomy;

			//CONS_Printf("AX1: %d, AY1: %d\n", p->rdaddmomx/mapobjectscale, p->rdaddmomy/mapobjectscale);

			pmo->momx = basemomx;
			pmo->momy = basemomy;

			/*CONS_Printf("CURR: %d, %d\n", pmo->momx/mapobjectscale, pmo->momy/mapobjectscale);
			CONS_Printf("BASE: %d, %d\n", basemomx/mapobjectscale, basemomy/mapobjectscale);
			CONS_Printf("ADD: %d, %d\n", p->rdaddmomx/mapobjectscale, p->rdaddmomy/mapobjectscale);*/

			// find out how much addmomx and addmomy we can actually get.
			// we do this by misusing P_Thrust to calc our values then immediately cancelling it.
			basemomx = pmo->momx;
			basemomy = pmo->momy;

			a = (INT32)(mo->angle) - dir*ANG1*90;
			P_Thrust(pmo, (angle_t)a, RIDEROIDMAXADD*3*mapobjectscale);

			xthreshold = pmo->momx - basemomx;
			ythreshold = pmo->momy - basemomy;

			//CONS_Printf("XT: %d (%d), YT: %d (%d)\n", xthreshold/mapobjectscale, abs(xthreshold/mapobjectscale), ythreshold/mapobjectscale, abs(ythreshold/mapobjectscale));

			// clamp the momentums using the calculated thresholds.

			// the fixedmul check checks if both numbers are of the same sign.
			if (abs(p->rdaddmomx) > abs(xthreshold))
				p->rdaddmomx = xthreshold;

			if (abs(p->rdaddmomy) > abs(ythreshold))
				p->rdaddmomy = ythreshold;

			//CONS_Printf("AX2: %d, AY2: %d\n", p->rdaddmomx/mapobjectscale, p->rdaddmomy/mapobjectscale);

			// now cancel it.
			pmo->momx = savemomx;
			pmo->momy = savemomy;
			//CONS_Printf("NEWCURR: %d, %d\n", pmo->momx/mapobjectscale, pmo->momy/mapobjectscale);
		}
	}
	else	// not turning
	{
		// for some reason doing *= 9/10 causes it to get set to 0 instantly? so it's done like this.
		p->rdaddmomx = (p->rdaddmomx*9)/10;
		p->rdaddmomy = (p->rdaddmomy*9)/10;
		p->rideroidrollangle /= 2;
	}

	// and now, going up/down

	if (p->cmd.throwdir > 0)
	{
		// if we were going the opposite direction, this helps us change our height very easily.
		if (p->rdaddmomz < 0)
			p->rdaddmomz /= 2;

		p->rdaddmomz = min(RIDEROIDMAXADD*mapobjectscale/7, p->rdaddmomz + mapobjectscale/16);

		if (p->rideroidspeed > RIDEROIDSPEED/2
		&& abs(p->cmd.turning) > 400
		&& leveltime & 1)
			p->rideroidspeed -= 1;

	}
	else if (p->cmd.throwdir < 0)
	{
		// if we were going the opposite direction, this helps us change our height very easily.
		if (p->rdaddmomz > 0)
			p->rdaddmomz /= 2;

		p->rdaddmomz = max(-RIDEROIDMAXADD*mapobjectscale/7, p->rdaddmomz - mapobjectscale/16);

		if (p->rideroidspeed > RIDEROIDSPEED/2
		&& abs(p->cmd.turning) > 400
		&& leveltime & 1)
			p->rideroidspeed -= 1;
	}
	else
		p->rdaddmomz = (p->rdaddmomz*6)/10;

}

// transposed lua code.
// the lua used to continuously P_SpawnMobj the letters which was fine for the intended use case in the original LV iteration.
// however the LV remake spams a lot of these rideroid nodes close to each other which created a huge overhead whether or not they were being displayed.
// so now it's more optimized and only spawns things once.

void Obj_RideroidNodeSpawn(mobj_t *mo)
{

	fixed_t radius = NODERADIUS*mapobjectscale;	// radius for the text to rotate at.
	mobj_t *ptr = mo;
	UINT8 i;
	UINT8 j;


	// make it bigger.
	P_SetScale(mo, mo->scale*3);

	// spawn the letter things.
	for (i = 0; i < 2; i++)
	{

		angle_t ang = mo->angle + (i)*180;
		fixed_t zpos = mo->z + 64*mapobjectscale + mapobjectscale*96*i;

		ang *= ANG1;	// this has to be done here or the warning prevents the compile, we don't care about overflowing here.

		for (j = 0; j < 7; j++)
		{
			fixed_t xpos = mo->x + FixedMul(radius, FINECOSINE(ang>>ANGLETOFINESHIFT));
			fixed_t ypos = mo->y + FixedMul(radius, FINESINE(ang>>ANGLETOFINESHIFT));

			mobj_t *let = P_SpawnMobj(xpos, ypos, zpos, MT_THOK);
			let->sprite = SPR_RDRL;
			let->frame = j|FF_FULLBRIGHT|FF_PAPERSPRITE;
			let->fuse = -1;
			let->tics = -1;
			let->angle = ang + ANG1*90;
			let->scale = 2*mapobjectscale;

			// set letter in previous thing's hnext, this will let us loop em easily in the looping thinker.
			P_SetTarget(&ptr->hnext, let);

			// set the ptr to the last letter spawned.
			ptr = let;

			ang += ANG1*8;
		}
	}
}

void Obj_RideroidNodeThink(mobj_t *mo)
{
	fixed_t radius = NODERADIUS*mapobjectscale;	// radius for the text to rotate at.
	mobj_t *ptr = mo->hnext;
	mobj_t *pmo;
	UINT8 i;

	mo->angle -= NODEROTSPEED;	// continuously rotate.

	while (ptr && !P_MobjWasRemoved(ptr))
	{
		// get the new position, move us here, and move on to the next object in line.
		angle_t newang = ptr->angle - NODEROTSPEED;
		fixed_t newxpos = mo->x + FixedMul(radius, FINECOSINE((newang - ANG1*90)>>ANGLETOFINESHIFT));
		fixed_t newypos = mo->y + FixedMul(radius, FINESINE((newang - ANG1*90)>>ANGLETOFINESHIFT));

		P_MoveOrigin(ptr, newxpos, newypos, ptr->z);
		ptr->angle = newang;

		ptr = ptr->hnext;
	}

	// check for players coming near us.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || players[i].rideroid || players[i].respawn.state != RESPAWNST_NONE ||
		players[i].rdnodepull || K_isPlayerInSpecialState(&players[i]) || P_PlayerInPain(&players[i]))
			continue;

		pmo = players[i].mo;
		//CONS_Printf("rd: %d\n", players[i].rideroid);

		if (R_PointToDist2(mo->x, mo->y, pmo->x, pmo->y) < NODERADIUS*mapobjectscale
		&& pmo->z + pmo->height >= mo->z
		&& pmo->z <= mo->z + 512*mapobjectscale)
		{

			mobj_t *rd;

			plr_undoRespawn(&players[i]);
			plr_resetRideroidVars(&players[i]);

			players[i].rdnodepull = true;
			players[i].rideroidangle = mo->spawnpoint->angle*ANG1;	// reminder that mo->angle changes, so we use the spawnpoint angle.
			players[i].rideroidspeed = RIDEROIDSPEED;

			P_SetTarget(&pmo->tracer, mo);

			// spawn the rideroid.
			rd = P_SpawnMobj(mo->x, mo->y, mo->z, MT_RIDEROID);
			rd->angle = players[i].rideroidangle;
			P_SetTarget(&rd->target, pmo);

			S_StartSound(rd, sfx_ridr1);

			//CONS_Printf("rd pull\n");

		}
	}
}
