// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dlzrocket.c
/// \brief Dead Line Zone free flight rockets! They cool af doe.

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

#define DLZROCKETDIST 96
#define DLZROCKETSPEED 80
#define DLZROCKETTURNSPEED ((ANG1*3)/2)
#define DLZROCKETVERTSPEED (ANG1)
#define DLZROCKETMAXVERT (ANG1*60)

void Obj_DLZRocketThink(mobj_t *mo)
{
	UINT8 i;
	angle_t an = mo->angle + ANGLE_90;

	if (mo->extravalue1)
		return;

	for (i = 0; i < 2; i++)
	{
		fixed_t x = mo->x + FixedMul(mapobjectscale, DLZROCKETDIST*FINECOSINE(an>>ANGLETOFINESHIFT));
		fixed_t y = mo->y + FixedMul(mapobjectscale, DLZROCKETDIST*FINESINE(an>>ANGLETOFINESHIFT));

		mobj_t *r = P_SpawnMobj(x, y, mo->z, MT_THOK);
		P_SetMobjState(r, i ? S_DLZROCKET_L : S_DLZROCKET_R);
		P_SetScale(r, (mapobjectscale*3)/2);
		r->destscale = (mapobjectscale*3)/2;
		r->angle = mo->spawnpoint->angle*ANG1;
		r->tics = -1;

		an += ANGLE_180;
	}

	mo->extravalue1 = 1;
}

void Obj_DLZRocketDismount(player_t *p)
{
	// we aren't mounted on one.
	if (!p->dlzrocket)
		return;

	p->dlzrocket = 0;
	K_SpawnMineExplosion(p->mo, p->mo->color, 3);
	S_StartSound(p->mo, sfx_s3k4e);
}

// touching the rocket, initialize player vars etc...
void Obj_DLZRocketSpecial(mobj_t *mo, player_t *p)
{
	if (K_isPlayerInSpecialState(p))	// already on one, don't bother resetting, duh.
		return;

	p->mo->z = mo->z + 16*P_MobjFlip(p->mo)*mapobjectscale;
	P_SetPlayerAngle(p->mo->player, mo->angle);
	p->dlzrocket = 1;
	p->dlzrocketangle = mo->angle;
	p->dlzrocketanglev = 0;
	p->dlzrocketspd = DLZROCKETSPEED;

	p->spinouttimer = 0;
	p->wipeoutslow = 0;

	S_StartSound(mo, sfx_s262);
}

void Obj_playerDLZRocket(player_t *p)
{

	fixed_t maxspd = DLZROCKETSPEED;
	angle_t visangle;
	UINT8 i, j;

	p->dlzrocket++;

	// helper arrows at the start of the ride to tell players they can move freely
	if (p->dlzrocket < TICRATE*2
	&& leveltime%10 < 5)
	{
		mobj_t *arr = P_SpawnMobj(p->mo->x, p->mo->y, p->mo->z, MT_THOK);
		arr->sprite = SPR_DLZA;
		arr->frame = FF_FULLBRIGHT;
		P_SetScale(arr, 2*mapobjectscale);
		arr->tics = 2;
	}

	// calc max speed
	if (p->ringboost)
		maxspd += 10;

	if (p->startboost)
		maxspd += 30;

	// set player speed
	if (p->dlzrocketspd < maxspd)
		p->dlzrocketspd++;
	else if (p->dlzrocketspd > maxspd)
		p->dlzrocketspd--;

	// so long as PF_STASIS is applied, let the angle be overwritten freely.
	// this is used by seasaws but can be used for misc modding purposes too.
	if (p->pflags & PF_STASIS)
		p->dlzrocketangle = p->mo->angle;
	else
	{
		SINT8 turndir = 0;
		P_SetPlayerAngle(p->mo->player, p->dlzrocketangle);

		if (p->cmd.turning > 0)
			turndir = 1;
		else if (p->cmd.turning < 0)
			turndir = -1;

		p->dlzrocketangle += turndir*DLZROCKETTURNSPEED;

		if (p->cmd.throwdir > 0)
			p->dlzrocketanglev = min(DLZROCKETMAXVERT, p->dlzrocketanglev + DLZROCKETVERTSPEED);
		else if (p->cmd.throwdir < 0)
			p->dlzrocketanglev = max(-DLZROCKETMAXVERT, p->dlzrocketanglev - DLZROCKETVERTSPEED);

	}

	// angle correction on ceilings (THIS CODE LOOKS AWFUL AND IT CAN PROBABLY BE DONE BETTER......)
	if ( (!(p->mo->eflags & MFE_VERTICALFLIP) && (p->mo->z+p->mo->height >= p->mo->ceilingz))
	|| (p->mo->eflags & MFE_VERTICALFLIP && p->mo->z <= p->mo->floorz))
		if ( (!(p->mo->eflags & MFE_VERTICALFLIP) && p->dlzrocketanglev > 0)
		|| (p->mo->eflags & MFE_VERTICALFLIP && p->dlzrocketanglev < 0))
			p->dlzrocketanglev = 0;


	if (!(p->pflags & PF_STASIS))
	{
		angle_t van = p->dlzrocketanglev /4;
		P_InstaThrust(p->mo, p->dlzrocketangle, FixedMul(mapobjectscale, p->dlzrocketspd*FINECOSINE(van>>ANGLETOFINESHIFT)));
		p->mo->momz = FixedMul(mapobjectscale, p->dlzrocketspd*FINESINE((angle_t)p->dlzrocketanglev>>ANGLETOFINESHIFT));
	}

	if (leveltime%4 == 0)
		S_StartSound(p->mo, sfx_s1c8);

	// finally, visuals.
	visangle = p->mo->angle + ANGLE_90;

	for (i = 0; i < 2; i++)
	{
		fixed_t x = p->mo->x + FixedMul(mapobjectscale, 56*FINECOSINE(visangle>>ANGLETOFINESHIFT));
		fixed_t y = p->mo->y + FixedMul(mapobjectscale, 56*FINESINE(visangle>>ANGLETOFINESHIFT));
		mobj_t *r = P_SpawnMobj(x, y, p->mo->z + 16*mapobjectscale, MT_THOK);
		r->fuse = 2;
		P_SetMobjState(r, i ? S_DLZROCKET_L : S_DLZROCKET_R);
		P_SetScale(r, (mapobjectscale*3)/2);
		r->angle = p->mo->angle;

		for (j = 0; j < 2; j++)
		{
			fixed_t xoffs = P_RandomRange(PR_EXPLOSION, -6, 6)*mapobjectscale;
			fixed_t yoffs = P_RandomRange(PR_EXPLOSION, -6, 6)*mapobjectscale;
			fixed_t soffs = P_RandomRange(PR_EXPLOSION, 0, 3);

			mobj_t *expl = P_SpawnMobj(r->x + xoffs, r->y + yoffs, r->z + xoffs, MT_THOK);
			P_SetMobjState(expl, S_QUICKBOOM1+soffs);
			expl->color = p->mo->color;
			P_SetScale(expl, mapobjectscale);
			expl->destscale = 2*mapobjectscale;

			if (p->startboost)
				expl->color = K_RainbowColor(leveltime);


		}
		visangle += ANGLE_180;
	}

	if ((p->dlzrocket > 10 && (P_IsObjectOnGround(p->mo) || p->mo->eflags & MFE_JUSTBOUNCEDWALL))
	|| p->spinouttimer || p->wipeoutslow || p->tumbleBounces
	|| p->respawn.state != RESPAWNST_NONE)
		Obj_DLZRocketDismount(p);
}
