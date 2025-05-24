// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dlzseasaw.c
/// \brief Dead Line Zone seasaw. Amplifies momentum in a stylish way... and hellish as far as code is concerned, heh!

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

// updates the seasaw's visuals and hitboxes using the hnext/hprev list.
static void Obj_DLZSeasawUpdate(mobj_t *mo, boolean ghostme)
{

	mobj_t *ptr = mo;
	mobj_t *ptrp = mo;
	UINT8 i, j;
	angle_t visan = (angle_t)mo->extravalue1 + ANGLE_90;

	if (mo->tracer && !P_MobjWasRemoved(mo->tracer))
	{
		mo->tracer->tics = 3;
		P_MoveOrigin(mo->tracer, mo->x, mo->y, mo->z);
		P_SetScale(mo->tracer, mo->scale);

		if (mo->eflags & MFE_VERTICALFLIP)
		{
			mo->tracer->eflags |= MFE_VERTICALFLIP;
			mo->tracer->flags2 |= MF2_OBJECTFLIP;
		}

	}


	for (i = 0; i < 2; i++)
	{
		INT32 dist = 32;	// visuals dist
		INT32 hdist = 16;	// hitbox dist

		// visuals
		for (j = 0; j < 2; j++)
		{
			// get our mobj.
			if (ptr && !P_MobjWasRemoved(ptr) && ptr->hnext && !P_MobjWasRemoved(ptr->hnext))
			{
				fixed_t x = mo->x + FixedMul(mo->scale, dist*FINECOSINE(visan>>ANGLETOFINESHIFT));
				fixed_t y = mo->y + FixedMul(mo->scale, dist*FINESINE(visan>>ANGLETOFINESHIFT));
				ptr = ptr->hnext;

				P_MoveOrigin(ptr, x, y, mo->z + 8*mapobjectscale*P_MobjFlip(mo));
				ptr->angle = visan;
				ptr->tics = 3;
				P_SetScale(ptr, mo->scale);

				if (mo->eflags & MFE_VERTICALFLIP)
				{
					ptr->eflags |= MFE_VERTICALFLIP;
					ptr->flags2 |= MF2_OBJECTFLIP;
				}

				if (ghostme && leveltime&1)
				{
					mobj_t *g = P_SpawnGhostMobj(ptr);
					g->colorized = true;
					g->color = mo->color;
					g->fuse = 3;
				}

				dist += 55;
			}
		}

		// hitboxes:
		for (j = 0; j < 8; j++)
		{
			// get our mobj.
			if (ptrp && !P_MobjWasRemoved(ptrp) && ptrp->hprev && !P_MobjWasRemoved(ptrp->hprev))
			{

				fixed_t x = mo->x + FixedMul(mo->scale, hdist*FINECOSINE(visan>>ANGLETOFINESHIFT));
				fixed_t y = mo->y + FixedMul(mo->scale, hdist*FINESINE(visan>>ANGLETOFINESHIFT));

				ptrp = ptrp->hprev;

				P_SetOrigin(ptrp, x, y, mo->z + 8*mapobjectscale*P_MobjFlip(mo));	// it's invisible so nobody cares about interpolating it.
				ptrp->angle = visan;
				ptrp->tics = 3;


				if (mo->eflags & MFE_VERTICALFLIP)
				{
					ptrp->eflags |= MFE_VERTICALFLIP;
					ptrp->flags2 |= MF2_OBJECTFLIP;
				}

				hdist += 16;
			}

		}

		visan += ANGLE_180;
	}
}

// sets up seasaw spawn objects to update each frame later
void Obj_DLZSeasawSpawn(mobj_t *mo)
{
	mobj_t *pole;
	mobj_t *ptr = mo;
	mobj_t *ptrp = mo;
	UINT8 i, j;

	P_SetScale(mo, 2*mapobjectscale);
	mo->destscale = 2*mapobjectscale;

	// setup vars
	mo->extravalue1 = (INT32)mo->angle;

	// center pole:
	pole = P_SpawnMobj(mo->x, mo->y, mo->z, MT_THOK);
	pole->tics = -1;
	pole->sprite = SPR_DLZS;
	pole->frame = 0;
	P_SetTarget(&pole->target, mo);
	P_SetTarget(&mo->tracer, pole);

	if (mo->eflags & MFE_VERTICALFLIP)
		pole->eflags |= MFE_VERTICALFLIP;

	// spawn visuals / hitboxes.
	for (i = 0; i < 2; i++)	// for each side...
	{
		for (j = 0; j < 2; j++)	// spawn the 2 visual papersprites on each side.
		{
			// right now we don't care if the objects are positionned properly.

			mobj_t *vis = P_SpawnMobj(mo->x, mo->y, mo->z + 8*mapobjectscale*P_MobjFlip(mo), MT_SEASAW_VISUAL);
			vis->sprite = SPR_DLZS;
			vis->frame = (j+1)|FF_PAPERSPRITE;
			vis->tics = -1;

			if (mo->eflags & MFE_VERTICALFLIP)
			{
				vis->eflags |= MFE_VERTICALFLIP;
				vis->flags2 |= MF2_OBJECTFLIP;
			}

			P_SetTarget(&vis->target, mo);
			P_SetTarget(&ptr->hnext, vis);	// save in an hnext list for updating later.
			ptr = vis;
		}

		for (j = 0; j < 8; j++) // spawn the 8 hitboxes on each side.
		{
			// right now we don't care if the objects are positionned properly.

			mobj_t *h = P_SpawnMobj(mo->x, mo->y, mo->z + 8*mapobjectscale*P_MobjFlip(mo), MT_DLZ_SEASAW_HITBOX);
			h->extravalue1 = i;	// keep track of which side we're on.
			h->tics = -1;

			if (mo->eflags & MFE_VERTICALFLIP)
			{
				h->eflags |= MFE_VERTICALFLIP;
				h->flags2 |= MF2_OBJECTFLIP;
			}

			P_SetTarget(&h->target, mo);
			P_SetTarget(&ptrp->hprev, h);	// save in an hprev list for updating later.
			ptrp = h;
		}
	}

	// update after spawning the objects so that they appear in the right spot when the map loads.
	Obj_DLZSeasawUpdate(mo, false);
}

static void Obj_DLZSeasawReset(mobj_t *mo)
{
	mo->extravalue1 = (INT32)mo->angle;
	P_SetTarget(&mo->target, NULL);
	Obj_DLZSeasawUpdate(mo, false);
}

// main seasaw thinker.
void Obj_DLZSeasawThink(mobj_t *mo)
{
	boolean ghost = false;
	SINT8 rot = 1;
	fixed_t px, py;

	if (mo->target && !P_MobjWasRemoved(mo->target))
	{
		mobj_t *t = mo->target;
		player_t *p = t->player;	// our target should always be a player, do NOT porceed if it isn't.

		if (!p)	// untarget this instantly.
		{
			Obj_DLZSeasawReset(mo);
			return;
		}

		if (!mo->extravalue2)
			rot = -1;

		// first half of the animation...
		if (!p->seasawdir)
		{
			INT32 angleadd = ANG1*max(4, (mo->movefactor/3)/mapobjectscale) * rot;

			if (p->seasawangleadd > 175)
				angleadd /= max(1, (p->seasawangleadd - 160)/8);

			mo->extravalue1 += angleadd;
			p->seasawangle += angleadd;

			p->seasawangleadd += max(4, (mo->movefactor/3)/mapobjectscale);
			P_SetPlayerAngle(p, (angle_t)(p->seasawangle + ANGLE_90*rot));
			//t->angle = p->seasawangle + ANGLE_90*rot;

			p->seasawdist++;
			p->seasawdist = min(p->seasawdist, (160*mapobjectscale)/FRACUNIT);

			if (abs((angleadd)/ANG1 ) < 2)	// if we get to 1, invert the rotation!
			{
				p->seasawdir = true;
				p->seasawangleadd = 0;	// reset, we're gonna do a full 360!
				p->seasawmoreangle = p->seasawangleadd - 170;
				S_StartSound(t, sfx_s3k88);
				S_StartSound(t, sfx_s3ka2);
			}
		}
		else
		{
			INT32 angleadd = (mo->cvmem*2 +1)*(-rot);
			mo->cvmem++;

			p->seasawangleadd += abs(angleadd)/2;	// for some reason i need to do this and i'm actually not sure why.
			mo->extravalue1 += angleadd*ANG1;
			p->seasawangle += angleadd*ANG1;

			P_SetPlayerAngle(p, (angle_t)(p->seasawangle - ANGLE_90*rot));
			ghost = true;

			if (p->seasawangleadd >= 340 + p->seasawmoreangle)
			{
				// reset everything and send the player zooming
				Obj_DLZSeasawReset(mo);

				P_SetPlayerAngle(p, mo->angle);
				P_MoveOrigin(t, t->x, t->y, t->z);	// miscall that to set the position properly.
				P_InstaThrust(t, mo->angle, mo->movefactor*3);	// send the player flying at triple the speed they came at us with.
				S_StartSound(t, sfx_cdfm62);

				K_SetTireGrease(p, 3*TICRATE);

				p->seasawangleadd = 0;
				p->seasawangle = 0;
				p->seasawmoreangle = 0;
				p->seasaw = false;

				Obj_DLZSeasawUpdate(mo, true);
				return;

			}
		}
		// update the player
		px = mo->x + p->seasawdist*FINECOSINE((angle_t)p->seasawangle>>ANGLETOFINESHIFT);
		py = mo->y + p->seasawdist*FINESINE((angle_t)p->seasawangle>>ANGLETOFINESHIFT);

		P_MoveOrigin(t, px, py, mo->z + mapobjectscale*8);
	}
	else
		Obj_DLZSeasawReset(mo);

	// finally, update the visuals.
	Obj_DLZSeasawUpdate(mo, ghost);
}

// ported just for convenience of not needing to rewrite the code to account for UINT32 angles...
// the precision loss hardly matters whatsoever.
static INT32 angtoint(angle_t a)
{
	return a/ANG1;
}

// to use in mobjcollide and movemobjcollide just like the lua, woo.
// mo is the player's mo, mo2 is the seasaw hitbox.
void Obj_DLZSeasawCollide(mobj_t *mo, mobj_t *mo2)
{
	player_t *p = mo->player;
	INT32 momangle;
	boolean invert = false;

	// cooldown / respawning
	if (p->seasawcooldown || p->respawn.timer)
		return;

	// other wacko state that'd do very weird shit if we overwrote it.
	if (K_isPlayerInSpecialState(p))
		return;

	// another player is already using the seasar
	if (mo2->target && !P_MobjWasRemoved(mo2->target) && mo2->target->target && !P_MobjWasRemoved(mo2->target->target))
		return;

	// height checks
	if (mo->z + mo->height < mo2->z)
		return;

	if (mo->z > mo2->z + mo2->height)
		return;

	// too slow.
	if (p->speed < K_GetKartSpeed(p, false, false)/3)
		return;


	momangle = angtoint(R_PointToAngle2(0, 0, mo->momx, mo->momy));

	//CONS_Printf("%d / %d -> %d\n", momangle, angtoint(mo2->target->angle), (abs(((momangle - angtoint(mo2->target->angle) +180) % 360) - 180)));

	// this depends on the side we hit the thing from.
	if (abs(((momangle - angtoint(mo2->target->angle) +180) % 360) - 180) > 60)
	{
		mo2->target->angle += ANGLE_180;
		mo2->target->extravalue1 += ANGLE_180;
		invert = true;
	}

	mo2->target->movefactor = p->speed;	// keep the speed the player was going at.
	mo2->target->extravalue2 = mo2->extravalue1;	// which side of the pole are we on?

	// if inverted, then invert the value too.
	if (invert)
		mo2->target->extravalue2 = (!mo2->target->extravalue2) ? 1 : 0;

	P_SetTarget(&mo2->target->target, mo);
	mo2->target->cvmem = 0;

	// set player vars now:
	p->seasawdist = R_PointToDist2(mo->x, mo->y, mo2->target->x, mo2->target->y) /FRACUNIT; // distance from us to the center
	p->seasawangle = (INT32)R_PointToAngle2(mo2->target->x, mo2->target->y, mo->x, mo->y);	// angle from the center to us
	p->seasawangleadd = 0;
	p->seasawdir = false;
	p->seasaw = true;
	p->pflags |= PF_STASIS;
	p->seasawcooldown = TICRATE/2;

	S_StartSound(mo, sfx_s3k88);
}
