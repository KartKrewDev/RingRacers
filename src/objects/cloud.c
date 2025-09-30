// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  cloud.c
/// \brief Launcher clouds and tulips used for Aerial Highlands, Avant Garden, and Sky Sanctuary.

#include "../p_local.h"
#include "../k_objects.h"
#include "../g_game.h"
#include "../info.h"
#include "../s_sound.h"
#include "../r_main.h"
#include "../m_random.h"
#include "../k_hitlag.h"


#define BULB_ZTHRUST 96*FRACUNIT
#define CLOUD_ZTHRUST 32*FRACUNIT
#define CLOUDB_ZTHRUST 16*FRACUNIT

void Obj_CloudSpawn(mobj_t *mobj)
{
	mobjtype_t cloudtype;

	switch (mobj->type)
	{
		case MT_AHZ_CLOUDCLUSTER:
			cloudtype = MT_AHZ_CLOUD;
			break;
		case MT_AGZ_CLOUDCLUSTER:
			cloudtype = MT_AGZ_CLOUD;
			break;
		case MT_SSZ_CLOUDCLUSTER:
			cloudtype = MT_SSZ_CLOUD;
			break;
		default:
			return;
	}

	mobj->destscale = mapobjectscale * 4;
	P_SetScale(mobj, mobj->destscale);

	mobj_t *cloud = P_SpawnMobj(mobj->x, mobj->y, mobj->z, cloudtype);
	angle_t ang = mobj->angle;
	UINT8 dist = 128;

	cloud->destscale = cloud->scale * 2;
	P_SetScale(cloud, cloud->destscale);

	for (UINT8 i = 0; i < 4; i++)
	{
		fixed_t x = mobj->x + FixedMul(mapobjectscale, dist * FINECOSINE(ang >> ANGLETOFINESHIFT));
		fixed_t y = mobj->y + FixedMul(mapobjectscale, dist * FINESINE(ang >> ANGLETOFINESHIFT));

		cloud = P_SpawnMobj(x, y, mobj->z, cloudtype);

		cloud->destscale = cloud->scale * 2;
		P_SetScale(cloud, cloud->destscale);

		if (cloudtype == MT_AGZ_CLOUD)
		{
			cloud->frame = P_RandomRange(PR_DECORATION, 0, 3);
		}

		ang += ANGLE_90;
	}
}

void Obj_TulipSpawnerThink(mobj_t *mobj)
{
	if (!mobj->tracer)
	{
		// I have no idea if doing it this way is correct
		mobj_t *part1 = P_SpawnMobj(0, 0, 0, MT_AGZ_BULB_PART);
		mobj_t *part2 = P_SpawnMobj(0, 0, 0, MT_AGZ_BULB_PART);
		mobj_t *tracer = P_SpawnMobj(0, 0, 0, MT_AGZ_BULB_PART);

		P_SetTarget(&mobj->hnext, part1);
		P_SetTarget(&mobj->hnext->hnext, part2);

		P_SetMobjState(mobj->hnext, S_AGZBULB_BASE);
		P_SetMobjState(mobj->hnext->hnext, S_AGZBULB_BASE);

		P_SetTarget(&mobj->tracer, tracer);
		P_SetMobjState(mobj->tracer, S_AGZBULB_NEUTRAL);
	}

	angle_t a = mobj->angle + ANG1*45;
	mobj_t *part = mobj->hnext;

	while (part)
	{
		P_MoveOrigin(part, mobj->x, mobj->y, mobj->z);
		part->angle = a;
		part->destscale = mobj->scale;
		P_SetScale(part, part->destscale);
		part->flags2 = mobj->flags2;
		part->eflags = mobj->eflags;
		a += ANG1*90;
		part = part->hnext;
	}

	mobj_t *b = mobj->tracer;

	P_MoveOrigin(b, mobj->x, mobj->y, mobj->z);
	b->destscale = mobj->scale;
	P_SetScale(b, b->destscale);
	b->flags2 = mobj->flags2;
	b->eflags = mobj->eflags;
	b->color = SKINCOLOR_MAGENTA;

	if (b->state == &states[S_AGZBULB_ANIM2])
	{
		if (leveltime & 1)
			b->colorized = true;
		else
			b->colorized = false;
	}
	else
		b->colorized = false;
}

void Obj_PlayerCloudThink(player_t *player)
{
	mobj_t *mo = player->mo;

	if (player->cloudbuf)
		player->cloudbuf--;

	if (player->cloudlaunch)
	{
		player->cloudlaunch--;

		if (leveltime % 6 == 0)
		{
			fixed_t rand_x;
			fixed_t rand_y;

			// note: determinate random argument eval order
			rand_y = P_RandomRange(PR_DECORATION, -8, 8);
			rand_x = P_RandomRange(PR_DECORATION, -8, 8);
			P_SpawnMobj(mo->x + rand_x*mapobjectscale, mo->y + rand_y*mapobjectscale, mo->z, MT_DRIFTDUST);
		}
	}

	if (player->cloud)
	{
		player->cloud--;
		mo->momz = 0;
		player->fastfall = 0;

		if (!player->cloud)
		{
			if (P_MobjWasRemoved(mo->tracer))
				return;

			mo->momz = FixedMul(mapobjectscale,
				(mo->tracer->type == MT_AHZ_CLOUD ? CLOUDB_ZTHRUST : CLOUD_ZTHRUST) * P_MobjFlip(mo->tracer));
			player->cloudlaunch = TICRATE;

			P_InstaThrust(mo, mo->cusval, mo->cvmem);
			K_AddHitLag(mo, 6, false);
		}
	}
}

void Obj_PlayerBulbThink(player_t *player)
{
	mobj_t *mo = player->mo;

	if (player->tuliplaunch)
	{
		player->tuliplaunch--;

		if (leveltime % 2 == 0)
		{
			fixed_t rand_x;
			fixed_t rand_y;

			// note: determinate random argument eval order
			rand_y = P_RandomRange(PR_DECORATION, -8, 8);
			rand_x = P_RandomRange(PR_DECORATION, -8, 8);
			P_SpawnMobj(mo->x + rand_x*mapobjectscale, mo->y + rand_y*mapobjectscale, mo->z, MT_DRIFTDUST);
		}
	}

	if (player->tulipbuf)
		player->tulipbuf--;

	if (player->tulip)
	{
		player->tulip--;
		P_MoveOrigin(mo, mo->tracer->x, mo->tracer->y, mo->tracer->z);
		mo->flags &= ~MF_SHOOTABLE;
		mo->renderflags |= RF_DONTDRAW;
	}

	if (player->tulip == 1)	// expired
	{

		S_StartSound(mo, sfx_s3k81);

		for (UINT8 i = 1; i < 16; i++)
		{
			mobj_t *d = P_SpawnMobj(mo->x, mo->y, mo->z, MT_DRIFTDUST);
			d->angle = ANGLE_MAX/16 * i;
			P_InstaThrust(d, d->angle, mapobjectscale*23);
			d->momz = mapobjectscale*8*P_MobjFlip(mo->tracer);
		}

		mo->renderflags &= ~RF_DONTDRAW;
		mo->player->nocontrol = 0;
		P_InstaThrust(mo, mo->tracer->extravalue2, mo->tracer->extravalue1);
		mo->momz = FixedMul(mapobjectscale, BULB_ZTHRUST)*P_MobjFlip(mo->tracer);

		mo->flags |= MF_SHOOTABLE;
		player->tuliplaunch = TICRATE;
		player->tulipbuf = 8;
		player->tulip = 0;
		P_SetTarget(&mo->tracer->target, NULL);
		P_SetTarget(&mo->tracer, NULL);
	}
}

void Obj_CloudTouched(mobj_t *special, mobj_t *toucher)
{
	player_t *player = toucher->player;

	if (player->cloudbuf || player->cloud)
		return;

	player->cloud = TICRATE/8;
	player->cloudbuf = TICRATE/3;

	for (UINT8 i = 1; i < 6; i++)
	{
		fixed_t rand_x;
		fixed_t rand_y;

		// note: determinate argument eval order
		rand_y = P_RandomRange(PR_DECORATION, -32, 32);
		rand_x = P_RandomRange(PR_DECORATION, -32, 32);
		mobj_t *spawn = P_SpawnMobj(toucher->x + rand_x*mapobjectscale, toucher->y + rand_y*mapobjectscale, toucher->z, MT_DRIFTDUST);
		spawn->angle = R_PointToAngle2(toucher->x, toucher->y, spawn->x, spawn->y);
		P_InstaThrust(spawn, spawn->angle, P_RandomRange(PR_DECORATION, 1, 8)*mapobjectscale);
		P_SetObjectMomZ(spawn, P_RandomRange(PR_DECORATION, 4, 10)<<FRACBITS, false);
		spawn->destscale = mapobjectscale * 3;
	}

	toucher->cvmem = FixedHypot(toucher->momx, toucher->momy);

	if (toucher->cvmem)
		toucher->cusval = R_PointToAngle2(0, 0, toucher->momx, toucher->momy);

	if (toucher->cvmem < mapobjectscale*8)
		toucher->cvmem = mapobjectscale*8;

	P_SetTarget(&toucher->tracer, special);
	S_StartSound(toucher, sfx_s3k8a);

}

void Obj_BulbTouched(mobj_t *special, mobj_t *toucher)
{
	if (toucher->player->tulip || toucher->player->tulipbuf)
		return;

	if (special && special->target)
		return; // player already using it

	if (toucher->player->respawn.timer)
		return;

	toucher->player->tulip = 8*2 +1;

	fixed_t spd = 0;
	angle_t ang = 0;

	if (!special->spawnpoint || special->spawnpoint->thing_args[0] == 0)
	{
		spd = FixedHypot(toucher->momx, toucher->momy);
		ang = R_PointToAngle2(0, 0, toucher->momx, toucher->momy);
	}

	P_InstaThrust(toucher, 0, 0);
	P_MoveOrigin(toucher, special->x, special->y, special->z);
	toucher->player->nocontrol = 1;
	P_SetTarget(&toucher->tracer, special);
	toucher->flags &= ~(MF_SHOOTABLE|MF_NOGRAVITY);
	toucher->renderflags |= RF_DONTDRAW;
	P_SetTarget(&special->target, toucher);
	special->extravalue1 = spd;
	special->extravalue2 = ang;

	S_StartSound(special, sfx_s254);

	// set bulb state:
	P_SetMobjState(special->tracer, S_AGZBULB_ANIM1);
}
