// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_boss.h
/// \brief Blend Eye boss encounter

#include "../../p_local.h"
#include "../../m_random.h"
#include "../../k_kart.h"
#include "../../k_hitlag.h"
#include "../../k_battle.h"
#include "../../k_boss.h"
#include "../../k_respawn.h"
#include "../../s_sound.h"
#include "../../g_game.h"
#include "../../r_main.h" // R_PointToAngle2, R_PointToDist2

#define PUYOARCMULTIPLIER (3)

typedef enum
{
	BLENDEYE_RESTART = 0,
	BLENDEYE_LOADAMMO,
	BLENDEYE_WAITAMMO,
	BLENDEYE_BLENDING,
	BLENDEYE_EXPLODING,
	BLENDEYE_PINCH_THROWN,
	BLENDEYE_PINCH_BOBBING,
	BLENDEYE_PINCH_WHISKING,
	BLENDEYE_PINCH_DRILLING,
	BLENDEYE_PINCH_EXPLODING,
	BLENDEYE_PINCH_BLOWNUP,
} blendeye_phase_e;

#define EYECOLOR \
		(encoremode \
			? SKINCOLOR_JAWZ \
			: SKINCOLOR_KETCHUP \
		)

#define K_SpawnBlendEyeExplosion(mo) \
	K_SpawnMineExplosion( \
		mo, \
		EYECOLOR, \
		0 \
	)

/// - MAIN BODY - ///

static void VS_BlendEye_Eye_Parts(mobj_t *mobj, INT32 angledelta)
{
	fixed_t x, y;

	// Before angle update, move the shield.
	if (mobj->tracer && mobj->tracer->hnext && mobj->movedir < BLENDEYE_EXPLODING)
	{
		mobj_t *ref = mobj->tracer->hnext;
		while (ref)
		{
			x = mobj->x + P_ReturnThrustX(mobj, mobj->tracer->movedir + ref->movedir, 40*mobj->scale);
			y = mobj->y + P_ReturnThrustY(mobj, mobj->tracer->movedir + ref->movedir, 40*mobj->scale);
			P_MoveOrigin(ref, x, y, mobj->z);
			ref->angle = mobj->tracer->movedir + ref->movedir - ANGLE_90;
			ref = ref->hnext;
		}
		mobj->tracer->movedir = mobj->angle;
	}

	if (mobj->movedir != BLENDEYE_PINCH_DRILLING || mobj->movecount > TICRATE/2)
		mobj->angle += angledelta;

	// And after, move the eye.
	if (mobj->tracer)
	{
		x = mobj->x + P_ReturnThrustX(mobj, mobj->angle, 38*mobj->scale);
		y = mobj->y + P_ReturnThrustY(mobj, mobj->angle, 38*mobj->scale);
		P_MoveOrigin(mobj->tracer, x, y, mobj->z + (mobj->height - mobj->tracer->height)/2);
		mobj->tracer->angle = mobj->angle - ANGLE_90;
		if (mobj->tracer->hprev)
		{
			P_MoveOrigin(mobj->tracer->hprev, mobj->x, mobj->y, max(mobj->floorz, mobj->z - (mobj->tracer->hprev->height)));
		}
	}
}

static mobj_t *VS_BlendEye_LoadAmmo(mobj_t *mobj, INT32 id)
{
	angle_t ang = mobj->tracer->cusval + FixedAngle(id*360*FRACUNIT/3);
	fixed_t h = mobj->z + mobj->height - 4*mapobjectscale;
	fixed_t dist = mobj->cvmem - 2*FRACUNIT;
	if (id <= 3)
		ang += ANGLE_180;

	mobj_t *ammo = P_SpawnMobjFromMobj(mobj,
		P_ReturnThrustX(mobj, ang, dist),
		P_ReturnThrustY(mobj, ang, dist),
		256*FRACUNIT,
		MT_BLENDEYE_PUYO);
	P_SetScale(ammo, (ammo->destscale *= 2));

	if (id <= 3)
		h += ammo->height;
	else
		ammo->cusval = (id-3)*TICRATE;

	P_SetObjectMomZ(ammo, -10*FRACUNIT, false);
	ammo->angle = ang;
	ammo->cvmem = dist;
	ammo->movefactor = (TICRATE/2) + P_RandomKey(PR_MOVINGTARGET, TICRATE/8);
	P_SetTarget(&ammo->tracer, mobj);
	ammo->extravalue1 = h;
	ammo->flags2 |= MF2_STRONGBOX;

	return ammo;
}

void VS_BlendEye_Init(mobj_t *mobj)
{
	UINT8 i;
	mobj_t *prev, *newmo;
	angle_t ang = 0;

	// necessary preamble
	P_SetScale(mobj, (mobj->destscale = mobj->scale*2));

	// spawn the glass
	prev = mobj;
	for (i = 0; i < 8; i++)
	{
		ang = mobj->angle + FixedAngle(i*360*FRACUNIT/8);
		newmo = P_SpawnMobjFromMobj(mobj,
			P_ReturnThrustX(mobj, ang, 39*FRACUNIT),
			P_ReturnThrustY(mobj, ang, 39*FRACUNIT),
			44*FRACUNIT,
			MT_BLENDEYE_GLASS);
		newmo->angle = ang - ANGLE_90;
		P_SetTarget(&prev->hnext, newmo);
		prev = newmo;
	}

	// spawn the generators
	prev = mobj;
	ang = mobj->angle + ANGLE_45;
	UINT8 numgenerators = (encoremode ? 4 : 3);
	for (i = 0; i < numgenerators; i++)
	{
		newmo = P_SpawnMobjFromMobj(mobj,
			P_ReturnThrustX(mobj, ang, 62*FRACUNIT),
			P_ReturnThrustY(mobj, ang, 62*FRACUNIT),
			0,
			MT_BLENDEYE_GENERATOR);
		P_SetScale(newmo, newmo->destscale = (3*newmo->destscale)/4);
		newmo->momz = -512*FRACUNIT; // cba to hardcode a diff. z coord
		newmo->angle = ang - ANGLE_90;
		P_SetTarget(&newmo->tracer, mobj);
		P_SetTarget(&prev->hprev, newmo);
		prev = newmo;
		if (i == 1 && numgenerators == 3)
			ang -= ANGLE_135;
		else
			ang -= ANGLE_90;
	}

	// spawn the eye...
	{
		P_SetTarget(&mobj->tracer, P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_BLENDEYE_EYE));
		P_SetTarget(&mobj->tracer->target, mobj);
		mobj->tracer->angle = mobj->angle - ANGLE_90;
		// ...and shield!
		prev = mobj->tracer;
		for (i = 0; i < 8; i++)
		{
			ang = FixedAngle(i*360*FRACUNIT/8);
			newmo = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_BLENDEYE_SHIELD);
			newmo->angle = mobj->angle + ang-ANGLE_90;
			newmo->movedir = ang;
			if (i == 0)
				P_SetMobjState(newmo, S_INVISIBLE);
			else if (i == 1)
				P_SetMobjState(newmo, S_BLENDEYE_SHIELD_R);
			else if (i == 7)
				P_SetMobjState(newmo, S_BLENDEYE_SHIELD_L);
			newmo->cvmem = i;
			P_SetTarget(&prev->hnext, newmo);
			prev = newmo;
		}
	}

	// initialise other important stuff
	if (encoremode)
		mobj->health++;
	mobj->tracer->health = mobj->health;
	mobj->health += numgenerators;
	mobj->tracer->cvmem = mobj->health;

	mobj->movedir = BLENDEYE_RESTART;
	mobj->movecount = TICRATE;

	mobj->reactiontime = mobj->z + (3*mobj->height)/2;

	mobj->cvmem = 24*FRACUNIT;

	VS_BlendEye_Eye_Parts(mobj, 0);

	{
		const char *enemyname, *subtitle;
		if (!encoremode)
		{
			enemyname = "Blend Eye";
			subtitle = "Here to serve";
		}
		else
		{
			enemyname = "Mean Blend Eye";
			subtitle = "Promoted to assistant manager";
		}

		K_InitBossHealthBar(enemyname, subtitle, 0, mobj->tracer->health*(FRACUNIT/mobj->health), mobj->tracer->cvmem);
		K_DeclareWeakspot(mobj, SPOT_NONE, EYECOLOR, true);
	}

	if (roundqueue.snapshotmaps)
	{
		mobj->tracer->cusval = ANGLE_45/2; // chosen by dice roll guaranteed to be random

		// Test load
		for (i = 6; i > 0; i--)
		{
			mobj_t *ammo = VS_BlendEye_LoadAmmo(mobj, i);
			ammo->momz = 0;
			ammo->z = ammo->extravalue1;
			ammo->flags |= MF_NOGRAVITY;
			P_SetMobjState(ammo, S_BLENDEYE_PUYO);
			if (i != 6) // one random
				ammo->sprite = mobj->movedir = SPR_PUYA + i - 1;
		}
	}
}

static mobj_t *sourceofmurder;

static inline BlockItReturn_t PIT_MurderPuyos(mobj_t *thing)
{
	if (thing->type != MT_BLENDEYE_PUYO)
		return BMIT_CONTINUE; // not a puyo

	if (thing->z < sourceofmurder->z)
		return BMIT_CONTINUE; // too low

	P_InstaThrust(thing, R_PointToAngle2(sourceofmurder->x, sourceofmurder->y, thing->x, thing->y), (thing->z - sourceofmurder->z)/4);
	thing->momz = (thing->z - sourceofmurder->z)/16;
	P_KillMobj(thing, sourceofmurder, sourceofmurder, DMG_NORMAL);

	return BMIT_CONTINUE; // Indiscriminate
}

static void VS_BlendEye_MurderPuyos(mobj_t *mobj)
{
	INT32 bx, by, xl, xh, yl, yh;

	sourceofmurder = mobj;

	yh = (unsigned)(mobj->y + (mobj->radius + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(mobj->y - (mobj->radius + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(mobj->x + (mobj->radius + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(mobj->x - (mobj->radius + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX (xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_MurderPuyos);
}

void VS_BlendEye_Thinker(mobj_t *mobj)
{
	boolean deathwatch = false; // may be useful for later

	// Init.
	{
		if (!mobj->tracer)
		{
			CONS_Alert(CONS_ERROR, "VS_BlendEye_Thinker: Reached thinker but init failed");
			P_RemoveMobj(mobj);
			return;
		}

		if (!mobj->tracer->tracer)
		{
			P_SetTarget(&mobj->tracer->tracer, VS_GetArena(mobj->thing_args[0]));
			if (!mobj->tracer->tracer)
			{
				P_RemoveMobj(mobj);
				return;
			}
		}
	}

	mobj->flags2 &= ~MF2_FRET;

	// Targeting.
	if (P_MobjWasRemoved(mobj->target)
		|| !mobj->target->player
		|| mobj->target->player->respawn.state != RESPAWNST_NONE
		|| mobj->target->health <= 0)
	{
		P_SupermanLook4Players(mobj);
		deathwatch = (P_MobjWasRemoved(mobj->target)
			|| !mobj->target->player
			|| mobj->target->player->respawn.state != RESPAWNST_NONE
			|| mobj->target->health <= 0);
	}

	INT32 i;
	fixed_t tiltstrength;
	
	// phases and attacks
	switch (mobj->movedir)
	{
		case BLENDEYE_LOADAMMO:
		{
			// SPAWN YOUR AMMO
			if ((mobj->movecount % 5) == 0)
			{
				VS_BlendEye_LoadAmmo(mobj, (mobj->movecount/5));
			}

			if ((--mobj->movecount) == 0)
			{
				mobj->movedir = BLENDEYE_WAITAMMO;
				mobj->movecount = TICRATE;
			}

			break;
		}
		case BLENDEYE_WAITAMMO:
		{
			// WAIT #1
			if (deathwatch)
				;
			else if ((--mobj->movecount) == 0)
			{
				mobj->movedir = BLENDEYE_BLENDING;
				mobj->movecount = 4*TICRATE;
				mobj->extravalue1 = mobj->reactiontime;
				mobj->flags2 |= MF2_AMBUSH;
				S_StartSound(NULL, sfx_befan1);
				if (mobj->tracer)
					P_SetMobjState(mobj->tracer, mobj->tracer->info->raisestate);
			}

			break;
		}
		case BLENDEYE_BLENDING:
		{
			// BLENDING IN ACTION
			if (!S_SoundPlaying(NULL, sfx_befan1) && !S_SoundPlaying(NULL, sfx_befan2))
				S_StartSound(NULL, sfx_befan2);

			tiltstrength = 5;
			if (leveltime & 1)
				tiltstrength = -tiltstrength;

			mobj->rollangle = FixedAngle(tiltstrength*FRACUNIT);

			if ((--mobj->movecount) == 0)
			{
				mobj->rollangle = 0;
				mobj->movedir = 0;
				mobj->movecount = 3*TICRATE/2;
				mobj->flags2 &= ~MF2_AMBUSH;
				//S_StopSoundByID(NULL, sfx_befan2);
				//S_StartSound(NULL, sfx_s3k85);
				if (mobj->tracer)
					P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
			}

			break;
		}
		case BLENDEYE_EXPLODING:
		{
			// BLOWS UP!?
			if ((--mobj->movecount) == 0)
			{
				// PINCH TRANSITION BEGIN !
				mobj->rollangle = 0;
				mobj->shadowscale = FRACUNIT;
				mobj->movedir = BLENDEYE_PINCH_THROWN;
				mobj_t *ref = mobj->hnext;
				mobj_t *refnext = NULL;
				while (ref)
				{
					refnext = ref->hnext;
					P_KillMobj(ref, mobj, mobj, DMG_NORMAL);
					ref = refnext;
				}
				S_StartSound(NULL, sfx_shattr);
				if (mobj->tracer)
				{
					boolean lastturned = false;
					ref = mobj->tracer->hnext;
					P_SetTarget(&mobj->tracer->hnext, NULL);
					while (ref)
					{
						if (ref->cvmem == 0)
							ref->flags |= MF_NOCLIPTHING;
						else if (ref->cvmem == 1)
							P_SetMobjState(ref, S_BLENDEYE_SHIELD_BUSTED_R);
						else if (ref->cvmem == 7)
							P_SetMobjState(ref, S_BLENDEYE_SHIELD_BUSTED_L);
						else if (lastturned == true)
							lastturned = false;
						else if (P_RandomChance(PR_DECORATION, FRACUNIT/2))
						{
							P_SetMobjState(ref, S_BLENDEYE_SHIELD_BUSTED);
							if (P_RandomChance(PR_DECORATION, FRACUNIT/2))
							{
								ref->frame++;
							}
							lastturned = true;
						}
						ref = ref->hnext;
					}
					mobj->tracer->renderflags |= RF_DONTDRAW;
					mobj->flags &= ~MF_NOGRAVITY;
					mobj->flags2 |= MF2_FRET;

					mobj_t *boomo = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_FZEROBOOM);
					boomo->extravalue1 = 2;
					P_SetScale(boomo, boomo->scale/3);
					boomo->destscale = mapobjectscale;
					K_SpawnBlendEyeExplosion(boomo);

					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;
						P_FlashPal(&players[i], PAL_WHITE, 1);
					}
					S_StartSound(NULL, sfx_s3k4e);
				}

				VS_BlendEye_MurderPuyos(mobj);

				mobj->momz = 12*mobj->scale;
				P_SetMobjState(mobj, mobj->info->raisestate);
				mobj->extravalue1 = mobj->reactiontime;
				if (!deathwatch)
				{
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
				}
				mobj->movecount = P_RandomChance(PR_MOVINGTARGET, FRACUNIT/2) ? 1 : -1;
				P_InstaThrust(mobj, mobj->angle + mobj->movecount*ANGLE_90, 4*mobj->scale); 
			}
			else if ((mobj->movecount % 3) == 0)
			{
				// SPAWN A MINI-POP
				angle_t rot = FixedAngle(P_RandomKey(PR_EXPLOSION, 360)*FRACUNIT);
				fixed_t offs = P_RandomKey(PR_EXPLOSION, (mobj->info->height - mobjinfo[MT_SONIC3KBOSSEXPLODE].height)/FRACUNIT)*FRACUNIT;
				P_SpawnMobjFromMobj(mobj,
					P_ReturnThrustX(mobj, rot, 56*FRACUNIT),
					P_ReturnThrustY(mobj, rot, 56*FRACUNIT),
					offs,
					MT_SONIC3KBOSSEXPLODE);
				S_StartSound(NULL, sfx_s3kb4);
				mobj_t *ref = mobj->hnext;
				while (ref)
				{
					P_SetMobjState(ref, ref->info->painstate);
					ref = ref->hnext;
				}

				tiltstrength = 5;
				if (((mobj->movecount/2) % 3) == 0)
					tiltstrength = -tiltstrength;

				mobj->rollangle = FixedAngle(tiltstrength*FRACUNIT);
			}
			else
			{
				mobj_t *ref = mobj->hnext;
				while (ref)
				{
					P_SetMobjState(ref, ref->info->spawnstate);
					ref = ref->hnext;
				}
				mobj->rollangle = 0;
			}

			return;
		}
		case BLENDEYE_PINCH_THROWN:
		{
			// WOOOAAAAH, MY HEAD'S SPINNING
			mobj->rollangle += (mobj->movecount*ANG20);
			if (mobj->momz < 0 && mobj->z < mobj->extravalue1)
			{
				mobj->movedir = BLENDEYE_PINCH_BOBBING;
				mobj->flags |= MF_NOGRAVITY|MF_SHOOTABLE;
				mobj->flags &= ~MF_NOCLIPTHING;
				mobj->flags2 &= ~MF2_FRET;
				mobj->momx = mobj->momy = mobj->momz = 0;
				mobj->rollangle = 0;
				mobj->movecount = -TICRATE/2;
				P_SetMobjState(mobj, mobj->info->spawnstate);
				if (mobj->tracer)
					mobj->tracer->renderflags &= ~RF_DONTDRAW;

				if (!deathwatch)
				{
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
				}

				P_SetTarget(&mobj->hprev, P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_OVERLAY));
				P_SetTarget(&mobj->hprev->target, mobj);
				mobj->hprev->flags |= MF_DONTENCOREMAP;
				mobj->hprev->threshold |= OV_DONTROLL;
				P_SetMobjState(mobj->hprev, S_BLENDEYE_FLAME);
				S_StartSound(NULL, sfx_s3k4f);
				mobj->cvmem = mobj->info->radius + 4*FRACUNIT;
			}

			//return;
			break;
		}
		case BLENDEYE_PINCH_BOBBING:
		case BLENDEYE_PINCH_WHISKING:
		{
			// BOBBING UP AND DOWN
			fixed_t bobsize = 6*mobj->scale;
			fixed_t bob = mobj->movecount;
			bob = -P_ReturnThrustY(mobj, FixedAngle(bob*10*FRACUNIT), bobsize);
			mobj->movecount++;
			if (mobj->movedir == BLENDEYE_PINCH_WHISKING)
			{
				if (encoremode) // RUN FOR LONGER
					mobj->extravalue1 -= (mobj->scale/4);
				else
					mobj->extravalue1 -= (2*mobj->scale/7);

				if (!S_SoundPlaying(NULL, sfx_befan1) && !S_SoundPlaying(NULL, sfx_befan2))
					S_StartSound(NULL, sfx_befan2);

				tiltstrength = 5;
				if (leveltime & 1)
					tiltstrength = -tiltstrength;
				mobj->rollangle = FixedAngle(tiltstrength*FRACUNIT);

				if (deathwatch)
				{
					mobj->movedir = BLENDEYE_PINCH_BOBBING;
					mobj->movecount = 0;
					mobj->rollangle = 0;
					mobj->extravalue1 = mobj->reactiontime;
					mobj->flags2 &= ~MF2_AMBUSH;
					if (mobj->tracer && mobj->tracer->hprev)
					{
						P_SetMobjState(mobj->tracer->hprev, S_BLENDEYE_EGGBEATER);
						P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
					}
				}
				else if (mobj->tracer && mobj->tracer->tracer && mobj->tracer->hprev)
				{
					// MOVE TOWARDS TARGET
					angle_t mompoint = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y); // replace first two with mobj->tracer->hnext->x and ->y for alt behaviour
					fixed_t mommag = FixedHypot(mobj->target->x - mobj->x, mobj->target->y - mobj->y);
					fixed_t bestspeed = 5*mobj->scale;
					fixed_t accel = (mobj->scale/2);
					boolean forcegoaround = false;
					angle_t test = R_PointToAngle2(mobj->tracer->tracer->x, mobj->tracer->tracer->y, mobj->x, mobj->y)
						- R_PointToAngle2(mobj->tracer->tracer->x, mobj->tracer->tracer->y, mobj->target->x, mobj->target->y);
					if (test >= ANGLE_180)
						test = InvAngle(test);

					if (test > ANGLE_90)
					{
						bestspeed += (AngleFixed(test-ANGLE_90)/(encoremode ? 20 : 30));
						forcegoaround = true;
					}

					if (mobj->tracer->hprev->extravalue1 + accel < bestspeed)
						mobj->tracer->hprev->extravalue1 += accel;
					else
						mobj->tracer->hprev->extravalue1 = bestspeed;

					if (mommag > mobj->tracer->hprev->extravalue1)
						mommag = mobj->tracer->hprev->extravalue1;

					if (mommag)
					{
						fixed_t *predict = VS_PredictAroundArena(mobj->tracer->tracer, mobj, mommag, mompoint, mobj->radius, forcegoaround, FRACUNIT);
						P_MoveOrigin(mobj, predict[0], predict[1], mobj->z);
					}

					// Slight bonus lifting hazard for those who get too close!
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;

						if (players[i].spectator)
							continue;

						if (P_MobjWasRemoved(players[i].mo))
							continue;

						if (players[i].mo->health == 0 || players[i].mo->hitlag)
							continue;

						if (players[i].mo->z > mobj->z)
							continue;

						if (FixedHypot(players[i].mo->x - mobj->x, players[i].mo->y - mobj->y) > (mobj->radius + 4*mapobjectscale))
							continue;

						players[i].mo->momz += 4*mapobjectscale;
					}
				}
			}
			else if (!mobj->tracer->hprev && mobj->movecount == TICRATE/3)
			{
				// SPAWN THE EGGBEATER
				P_SetTarget(&mobj->tracer->hprev,
					P_SpawnMobjFromMobj(mobj, 0, 0,
						-mobjinfo[MT_BLENDEYE_EGGBEATER].height,
						MT_BLENDEYE_EGGBEATER
					)
				);
				P_SetTarget(&mobj->tracer->hprev->tracer, mobj);
				mobj->tracer->hprev->extravalue1 = 0;
				S_StartSound(NULL, sfx_cdfm39);

				// Gainax sharpness
				mobj_t *shwing = P_SpawnMobjFromMobj(mobj->tracer->hprev, 0, 0, 0, MT_OVERLAY);
				shwing->threshold = OV_DONTSCREENOFFSET;
				P_SetTarget(&shwing->target, mobj->tracer->hprev);
				P_SetMobjState(shwing, S_EMERALDFLARE1);
				shwing->fuse = (TICRATE - mobj->movecount);
				shwing->spritexoffset = -24*FRACUNIT;
				shwing->spriteyoffset = 14*FRACUNIT;

				// Dr. Robontik's Rage Stagers ?!
				mobj_t *angery = P_SpawnMobjFromMobj(mobj->tracer,
					P_ReturnThrustX(mobj, mobj->angle, FRACUNIT),
					P_ReturnThrustY(mobj, mobj->angle, FRACUNIT),
					15*FRACUNIT,
					MT_GHOST
				);

				P_SetMobjState(angery, S_SPIKEDLENS);
				P_SetScale(angery, angery->scale/2);
				angery->height = 1; // prevent z correction in P_MobjScaleThink
				angery->destscale *= 4;
				angery->fuse = TICRATE/7; // 5
				angery->scalespeed = ((angery->destscale - angery->scale)/angery->fuse);
				angery->colorized = true;
				angery->color = EYECOLOR;
				angery->flags2 |= MF2_BOSSNOTRAP; // Quicker fadeout
			}
			else if (mobj->movecount >= TICRATE && !deathwatch)
			{
				mobj->movedir = BLENDEYE_PINCH_WHISKING;
				if (mobj->tracer)
				{
					P_SetMobjState(mobj->tracer, mobj->tracer->info->raisestate);
					if (mobj->tracer->hprev)
						P_SetMobjState(mobj->tracer->hprev, S_BLENDEYE_EGGBEATER_SPIN);

					mobj->tracer->cusval = (encoremode ? 6 : 5);
				}
				S_StartSound(NULL, sfx_befan1);
				mobj->flags2 |= MF2_AMBUSH;
			}
			if (mobj->z < mobj->extravalue1 - bobsize)
				mobj->z = (2*mobj->z + mobj->extravalue1 + bob)/3;
			else
				mobj->z = mobj->extravalue1 + bob;

			if (P_MobjWasRemoved(mobj->hprev))
				;
			else if (mobj->z <= mobj->extravalue1)
			{
				if ((mobj->hprev->state-states) != S_BLENDEYE_FLAME)
					P_SetMobjState(mobj->hprev, S_BLENDEYE_FLAME);

				if (!S_SoundPlaying(mobj->hprev, sfx_s3k7f))
					S_StartSound(mobj->hprev, sfx_s3k7f);
			}
			else
			{
				if ((mobj->hprev->state-states) != S_INVISIBLE)
					P_SetMobjState(mobj->hprev, S_INVISIBLE);
			}

			if (mobj->tracer && mobj->tracer->hprev)
			{
				if ((mobj->extravalue1 + bob) <= (mobj->floorz + mobj->tracer->hprev->height))
				{
					mobj->movedir = BLENDEYE_PINCH_DRILLING;
					mobj->movecount = 0;
					S_StopSoundByID(NULL, sfx_befan2);
					S_StartSound(NULL, sfx_s3k85);
					mobj->flags2 &= ~MF2_AMBUSH;
					if (mobj->hprev)
						P_SetMobjState(mobj->hprev, S_INVISIBLE);
					mobj->tracer->hprev->flags &= ~MF_PAIN;
				}
				else if (deathwatch)
					;
				else if (mobj->tracer->cusval && !(mobj->movecount % (TICRATE)))
				{
					angle_t ang = FixedAngle(P_RandomKey(PR_MOVINGTARGET, 360)*FRACUNIT);
					fixed_t dist = 2*FRACUNIT;
					mobj_t *ammo = P_SpawnMobjFromMobj(mobj,
						P_ReturnThrustX(mobj, ang, dist),
						P_ReturnThrustY(mobj, ang, dist),
						0,
						MT_BLENDEYE_PUYO);
					P_SetOrigin(ammo, ammo->x, ammo->y, ammo->floorz+1);
					P_SetScale(ammo, (ammo->destscale *= 2));
					ammo->angle = ang;
					ammo->cvmem = dist;
					ammo->movefactor = (TICRATE/2) + P_RandomKey(PR_MOVINGTARGET, TICRATE/8);
					P_SetTarget(&ammo->tracer, mobj);
					ammo->extravalue1 = mobj->floorz;
					if (mobj->tracer->cusval & 1)
						ammo->flags2 |= MF2_BOSSNOTRAP;
					mobj->tracer->cusval--;
				}
			}

			break;
		}
		case BLENDEYE_PINCH_DRILLING:
		{
			// ACCIDENTIALLY DRILLING INTO THE GROUND
			if (mobj->z > mobj->floorz+FRACUNIT)
			{
				tiltstrength = 10;
				if (leveltime & 1)
					tiltstrength = -tiltstrength;
				mobj->rollangle = FixedAngle(tiltstrength*FRACUNIT);
				mobj->angle -= (ANG10+ANG20);
				mobj->z = (2*mobj->z + mobj->floorz)/3;
				mobj->tracer->cusval = FixedAngle(P_RandomKey(PR_MOVINGTARGET, 360)*FRACUNIT);
				if (!mobj->tracer->hprev)
					;
				else for (i = 0; i < 6; i++)
				{
					angle_t angle = mobj->tracer->cusval + FixedAngle(i*360*FRACUNIT/12);
					fixed_t momx = P_ReturnThrustX(mobj, angle, mobj->tracer->hprev->radius);
					fixed_t momy = P_ReturnThrustY(mobj, angle, mobj->tracer->hprev->radius);
					mobj_t *dustmo = P_SpawnMobjFromMobj(
						mobj->tracer->hprev,
						0, 0, 0,
						(encoremode
							? MT_BLENDEYE_PUYO_DUST
							: MT_BLENDEYE_PUYO_DUST_COFFEE
						)
					);
					P_SetScale(dustmo, (dustmo->destscale *= 2));
					dustmo->momx = momx/(3-(i & 1));
					dustmo->momy = momy/(3-(i & 1));
					dustmo->momz = (mobj->z - mobj->floorz)/(2+(i & 1));
				}
			}
			else
			{
				mobj->z = mobj->floorz;
				if ((mobj->tracer->hprev->state-states) != S_BLENDEYE_EGGBEATER)
				{
					mobj->rollangle = 0;
					P_SetMobjState(mobj->tracer->hprev, S_BLENDEYE_EGGBEATER);
					P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
				}

				if ((mobj->movecount == TICRATE/2) && !deathwatch)
					K_DeclareWeakspot(mobj, SPOT_WEAK, SKINCOLOR_EMERALD, false);

				if ((mobj->movecount > 2*TICRATE) && (leveltime & 1))
				{
					if (mobj->hprev && (mobj->hprev->state-states) != S_BLENDEYE_FLAME)
					{
						P_SetMobjState(mobj->hprev, S_BLENDEYE_FLAME);
						S_StartSound(NULL, sfx_s3k4f);
					}
					mobj->z += FRACUNIT;
					if (!S_SoundPlaying(NULL, sfx_s3k69))
						S_StartSound(NULL, sfx_s3k69);
				}

				if ((++mobj->movecount) == 3*TICRATE)
				{
					mobj->movedir = BLENDEYE_PINCH_BOBBING;
					mobj->movecount = 0;
					mobj->rollangle = 0;
					mobj->extravalue1 = mobj->reactiontime;
					S_StopSoundByID(NULL, sfx_s3k69); // it's like poetry
					S_StartSound(NULL, sfx_mbs53);
					if (mobj->tracer)
					{
						P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
						if (mobj->tracer->hprev && !(mobj->tracer->hprev->flags & MF_PAIN))
						{
							mobj->tracer->hprev->flags |= MF_PAIN;
							mobj->tracer->hprev->extravalue1 = 0;
							if (mobj->tracer->hprev->z != mobj->floorz)
								;
							else for (i = 0; i < 6; i++)
							{
								angle_t angle = mobj->tracer->cusval + FixedAngle(i*360*FRACUNIT/12);
								fixed_t momx = P_ReturnThrustX(mobj, angle, mobj->tracer->hprev->radius);
								fixed_t momy = P_ReturnThrustY(mobj, angle, mobj->tracer->hprev->radius);
								mobj_t *dustmo = P_SpawnMobjFromMobj(
									mobj->tracer->hprev,
									0, 0, 0,
									(encoremode
										? MT_BLENDEYE_PUYO_DUST
										: MT_BLENDEYE_PUYO_DUST_COFFEE
									)
								);
								P_SetScale(dustmo, (dustmo->destscale *= 2));
								dustmo->momx = momx/(3-(i & 1));
								dustmo->momy = momy/(3-(i & 1));
								dustmo->momz = (mobj->tracer->hprev->radius)/(2+(i & 1));
							}
						}
					}
				}
			}

			break;
		}
		case BLENDEYE_PINCH_EXPLODING:
		{
			// SHE'S DYING...
			if ((--mobj->movecount) == 0)
			{
				// ALL DONE
				{
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;
						if (players[i].spectator)
							continue;
						if (players[i].pflags & PF_NOCONTEST)
							continue;

						P_DoAllPlayersExit(0, true);
						break;
					}
				}

				mobj->movedir = BLENDEYE_PINCH_BLOWNUP;

				fixed_t offs = P_RandomChance(PR_EXPLOSION, FRACUNIT/2)
					? ANGLE_135
					: InvAngle(ANGLE_135);
				P_InstaThrust(mobj, mobj->angle + offs, 8*mapobjectscale);
				mobj->momz = 16*mapobjectscale;
				mobj->flags |= MF_NOCLIPHEIGHT;
				mobj->flags &= ~MF_NOGRAVITY;
				mobj->z++;

				P_InstaThrust(mobj->tracer, mobj->angle, 8*mapobjectscale);
				mobj->tracer->momz = 12*mapobjectscale;
				mobj->tracer->flags |= MF_NOCLIPHEIGHT;
				mobj->tracer->flags &= ~MF_NOGRAVITY;
				mobj->tracer->z++;

				if (mobj->tracer->hprev)
				{
					P_InstaThrust(mobj->tracer->hprev, mobj->angle - offs, 8*mapobjectscale);
					mobj->tracer->hprev->momz = 6*mapobjectscale;
					mobj->tracer->hprev->flags |= MF_NOCLIPHEIGHT;
					mobj->tracer->hprev->flags &= ~MF_NOGRAVITY;
					mobj->tracer->hprev->z++;
					P_SetScale(mobj->hprev, mapobjectscale/4);
					K_SpawnBlendEyeExplosion(mobj->hprev);
					S_StartSound(NULL, sfx_s3k4e);
				}

				P_RemoveMobj(mobj->hprev);
				P_SetTarget(&mobj->hprev, NULL);
			}
			else if ((mobj->movecount % 2) == 0)
			{
				// SPAWN A MINI-POP (2)
				angle_t rot = FixedAngle(P_RandomKey(PR_EXPLOSION, 360)*FRACUNIT);
				fixed_t offs = P_RandomKey(PR_EXPLOSION, (mobj->info->height - mobjinfo[MT_SONIC3KBOSSEXPLODE].height)/FRACUNIT)*FRACUNIT;
				P_SpawnMobjFromMobj(mobj,
					P_ReturnThrustX(mobj, rot, mobj->info->radius),
					P_ReturnThrustY(mobj, rot, mobj->info->radius),
					offs,
					MT_SONIC3KBOSSEXPLODE);
				S_StartSound(NULL, sfx_s3kb4);
				tiltstrength = 5;
				if (((mobj->movecount/2) % 2) == 0)
					tiltstrength = -tiltstrength;

				mobj->rollangle = FixedAngle(tiltstrength*FRACUNIT);
			}
			else
				mobj->rollangle = 0;

			return;
		}
		case BLENDEYE_PINCH_BLOWNUP:
		{
			// CURTAINS FOR YOU
			if (mobj->z != mobj->floorz)
				mobj->rollangle += ANG20;
			else
				mobj->rollangle = ANGLE_180;

			if (mobj->tracer->hprev->z != mobj->floorz)
				mobj->tracer->hprev->rollangle += ANG20;
			else
				mobj->tracer->hprev->rollangle = (mobj->tracer->hprev->rollangle >= ANGLE_180)
					? ANGLE_135
					: InvAngle(ANGLE_135);

			mobj->renderflags ^= RF_DONTDRAW;
			mobj->tracer->renderflags ^= RF_DONTDRAW;
			if (mobj->tracer->hprev)
				mobj->tracer->hprev->renderflags ^= RF_DONTDRAW;

			return;
		}
		default:
		{
			// RESTART CYCLE
			if (deathwatch)
				;
			else if ((--mobj->movecount) == 0)
			{
				mobj->movedir = BLENDEYE_LOADAMMO;
				mobj->movecount = 6*5;
				mobj->tracer->cusval = FixedAngle(P_RandomKey(PR_MOVINGTARGET, 360)*FRACUNIT);
			}
			else if (mobj->movecount == TICRATE)
			{
				mobj_t *ref = mobj->hprev;
				while (ref)
				{
					if (ref->health > 0)
						K_DeclareWeakspot(ref, SPOT_WEAK, SKINCOLOR_EMERALD, false);
					ref = ref->hprev;
				}
			}

			break;
		}
	}

	// Look around.
	if (mobj->target) // !deathwatch
	{
		INT32 angledelta = AngleDeltaSigned(R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y), mobj->angle)/4;
		const INT32 maxdelta = (ANGLE_45/2);

		if (angledelta > maxdelta)
			angledelta = maxdelta;
		else if (angledelta < -maxdelta)
			angledelta = -maxdelta;

		VS_BlendEye_Eye_Parts(mobj, angledelta);
	}
}

boolean VS_BlendEye_Touched(mobj_t *special, mobj_t *toucher)
{
	if (toucher->hitlag > 0)
		return false;

	fixed_t thrust = FixedHypot(toucher->momx, toucher->momy);
	angle_t ang = R_PointToAngle2(special->x, special->y, toucher->x - toucher->momx, toucher->y - toucher->momy);
	P_InstaThrust(toucher, ang + ANGLE_90, P_ReturnThrustX(toucher, ang + ANGLE_90, thrust));
	thrust = P_ReturnThrustX(toucher, ang, thrust);
	if (thrust < 4*mapobjectscale)
		thrust = (4*mapobjectscale);
	P_Thrust(toucher, ang, thrust);
	return true;
}

void VS_BlendEye_Damage(mobj_t *mobj, mobj_t *inflictor, mobj_t *source, INT32 damage)
{
	(void)inflictor;

	S_StartSound(NULL, mobj->info->painsound);
	K_UpdateBossHealthBar((mobj->health - damage)*(FRACUNIT/mobj->tracer->cvmem), 8);
	if (mobj->tracer)
	{
		if (mobj->movedir == BLENDEYE_PINCH_BOBBING)
		{
			if (mobj->hprev)
				P_SetMobjState(mobj->hprev, S_INVISIBLE);
		}
		else if (mobj->movedir == BLENDEYE_PINCH_WHISKING)
		{
			mobj->movedir = BLENDEYE_PINCH_BOBBING;
			mobj->movecount = 0;
			mobj->rollangle = 0;
			mobj->flags2 &= ~MF2_AMBUSH;
			mobj->extravalue1 = mobj->reactiontime;
			if (mobj->hprev && (mobj->hprev->state-states) != S_BLENDEYE_FLAME)
			{
				P_SetMobjState(mobj->hprev, S_BLENDEYE_FLAME);
				S_StartSound(NULL, sfx_s3k4f);
			}
			if (mobj->tracer->hprev)
			{
				P_SetMobjState(mobj->tracer->hprev, S_BLENDEYE_EGGBEATER);
				mobj->tracer->hprev->flags |= MF_PAIN;
				P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
			}
		}
		else if (mobj->movedir == BLENDEYE_PINCH_DRILLING)
		{
			if (mobj->tracer->hprev)
				mobj->tracer->hprev->flags &= ~MF_PAIN;

			mobj->movecount = (2*TICRATE)+(TICRATE/2);
			P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
		}
		else if ((mobj->health - damage) == mobj->tracer->health) //pre-pinch
		{
			mobj->flags2 &= ~MF2_AMBUSH;
			mobj->rollangle = 0;
			mobj->movedir = BLENDEYE_EXPLODING;
			mobj->movecount = 2*TICRATE;
			P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
		}
	}

	// make the glass stress
	mobj_t *ref = mobj->hnext;
	while (ref)
	{
		P_SetMobjState(ref, ref->info->painstate);
		ref = ref->hnext;
	}

	if (source && source->player)
	{
		K_GivePointsToPlayer(source->player, NULL, 1);
	}
}

void VS_BlendEye_Death(mobj_t *mobj)
{
	mobj->flags2 &= ~MF2_AMBUSH;
	mobj->rollangle = 0;
	mobj->movedir = BLENDEYE_PINCH_EXPLODING;
	mobj->movecount = 3*TICRATE/2;

	if (mobj->tracer)
	{
		P_SetMobjState(mobj->tracer, mobj->tracer->info->spawnstate);
		mobj->tracer->flags |= MF_NOCLIP|MF_NOCLIPTHING;

		if (mobj->hprev)
			mobj->tracer->hprev->flags |= MF_NOCLIP|MF_NOCLIPTHING;

		if (mobj->tracer->hprev)
		{
			P_SetMobjState(mobj->tracer->hprev, S_BLENDEYE_EGGBEATER);
		}
	}

	if (mobj->hprev)
		P_SetMobjState(mobj->hprev, S_INVISIBLE);

	mobj->flags |= MF_NOCLIP|MF_NOCLIPTHING;

	K_AddHitLag(mobj, 6, true);
}

/// - AUXILLIARY OBJECTS - ///

boolean VS_BlendEye_Eye_Thinker(mobj_t *mobj)
{
	if (P_MobjWasRemoved(mobj->target))
	{
		P_RemoveMobj(mobj);
		return false;
	}

	if (mobj->target->hitlag)
	{
		P_InstaThrust(mobj, mobj->angle, 25*mobj->scale);
		K_AddHitLag(mobj, mobj->target->hitlag, (mobj->target->eflags & MFE_DAMAGEHITLAG) == MFE_DAMAGEHITLAG);
		return false;
	}

	if (!(mobj->flags & MF_NOCLIPHEIGHT))
	{
		mobj->momx = mobj->momy = 0;
	}

	return true;
}

void VS_BlendEye_Glass_Death(mobj_t *mobj)
{
	UINT8 i = 0;
	fixed_t thrust = 30*mapobjectscale;
	for (i = 0; i <= 8; i++)
	{
		fixed_t along = P_RandomRange(PR_DECORATION, -mobj->radius/FRACUNIT, mobj->radius/FRACUNIT)*FRACUNIT;
		fixed_t up = (mobj->height - FixedMul(mobjinfo[MT_BATTLEBUMPER_DEBRIS].height, mobj->scale));
		if (up > 0)
			up = P_RandomKey(PR_DECORATION, mobj->height/FRACUNIT)*FRACUNIT;
		mobj_t *glassmo = P_SpawnMobj(
			mobj->x + P_ReturnThrustX(mobj, mobj->angle, along),
			mobj->y + P_ReturnThrustY(mobj, mobj->angle, along),
			mobj->z + up,
			MT_BATTLEBUMPER_DEBRIS);
		glassmo->color = SKINCOLOR_WHITE;
		glassmo->renderflags |= RF_ADD;
		angle_t vang = R_PointToAngle2(0, mobj->z, 39*FRACUNIT, glassmo->z);
		P_InstaThrust(glassmo, mobj->angle + ANGLE_90, P_ReturnThrustX(glassmo, vang, thrust));
		glassmo->momz = P_ReturnThrustX(glassmo, vang, thrust);
	}
}

void VS_BlendEye_Eggbeater_Touched(mobj_t *t1, mobj_t *t2)
{
	if (t2->hitlag || (t1->health <= 0 || t2->health <= 0))
		return;

	if (t1->z == t1->floorz)
		return;

	INT32 minextravalue1 = -(P_IsObjectOnGround(t2) ? 5 : 15)*t1->scale;
	if (t1->extravalue1 > minextravalue1)
		t1->extravalue1 = minextravalue1;

	if ((t2->player->tumbleBounces > 0) && (t2->momz > 0))
		return;

	fixed_t thrust = FixedHypot(t2->momx, t2->momy);
	if (thrust < 20*mapobjectscale)
		thrust = 20*mapobjectscale;

	P_Thrust(
		t2,
		R_PointToAngle2(t1->x, t1->y, t2->x - t2->momx, t2->y - t2->momy),
		thrust
	);
}

void VS_BlendEye_Generator_DeadThinker(mobj_t *mobj)
{
	if (leveltime & 1)
		return;

	if (P_RandomChance(PR_DECORATION, FRACUNIT/4))
		return;

	angle_t twist = mobj->angle;
	fixed_t dist = 18*FRACUNIT;
	fixed_t z = 26*FRACUNIT;
	fixed_t fb = 6*FRACUNIT;

	if (P_RandomChance(PR_DECORATION, FRACUNIT/2))
	{
		z = 42*FRACUNIT;
		dist = 28*FRACUNIT;
		twist += ANGLE_180;
		fb = -6*FRACUNIT;
	}

	if ((mobj->state-states) != S_BLENDEYE_GENERATOR_BUSTED_R)
		twist += ANGLE_180;

	mobj_t *dust = P_SpawnMobjFromMobj(
		mobj,
		P_ReturnThrustX(mobj, twist, dist) + P_ReturnThrustY(mobj, twist, fb),
		P_ReturnThrustY(mobj, twist, dist) + P_ReturnThrustX(mobj, twist, fb),
		z,
		encoremode
			? MT_BLENDEYE_PUYO_DUST
			: MT_BLENDEYE_PUYO_DUST_COFFEE
	);
	dust->angle = FixedAngle(P_RandomKey(PR_DECORATION, 360)*FRACUNIT);
	P_InstaThrust(dust, dust->angle, 4*mapobjectscale);
	dust->momz = 8*mapobjectscale;
}

/// - PUYO HAZARDS - ///

boolean VS_PuyoTouched(mobj_t *special, mobj_t *toucher)
{
	if (!special->health || !toucher->health)
		return false; // too dead

	if (special->state-states < S_BLENDEYE_PUYO)
		return false; // too small

	P_DamageMobj(toucher, special, special, 1, DMG_NORMAL);

	special->momx = 0;
	special->momy = 0;
	special->momz = 0;

	return true;
}

// Basically a duplication of A_Boss5Jump. I'm not adding A_ action calls to header files.

static void VS_PuyoJump(mobj_t *shot, mobj_t *target)
{
	fixed_t v; // Velocity to jump at
	fixed_t a1, a2, aToUse; // Velocity squared
	fixed_t g; // Gravity
	fixed_t x; // Horizontal difference
	INT32 x_int; // x! But in integer form!
	fixed_t y; // Vertical difference (yes that's normally z in SRB2 shut up)
	INT32 y_int; // y! But in integer form!
	INT32 intHypotenuse; // x^2 + y^2. Frequently overflows fixed point, hence why we need integers proper.
	fixed_t fixedHypotenuse; // However, we can work around that and still get a fixed-point number.
	angle_t theta; // Angle of attack
	// INT32 locvar1 = var1;
	// INT32 locvar2 = var2;

	if (!shot || !target)
		return; // Don't even bother if we've got nothing to aim at.

	// Scale with map
	g = FixedMul(gravity, mapobjectscale);

	// Look up distance between shot and its target
	x = FixedHypot(target->x - shot->x, target->y - shot->y);
	// Look up height difference between shot and its target
	y = target->z - shot->z;

	// Get x^2 + y^2. Have to do it in a roundabout manner, because this overflows fixed_t way too easily otherwise.
	x_int = x>>FRACBITS;
	y_int = y>>FRACBITS;
	intHypotenuse = (x_int*x_int) + (y_int*y_int);
	fixedHypotenuse = FixedSqrt(intHypotenuse) *256;

	// a = g(y+/-sqrt(x^2+y^2)). a1 can be +, a2 can be -.
	a1 = FixedMul(g,y+fixedHypotenuse);
	a2 = FixedMul(g,y-fixedHypotenuse);

	// Determine which one isn't actually an imaginary number (or the smaller of the two, if both are real), and use that for v.
	if (a1 < 0 || a2 < 0)
	{
		if (a1 < 0 && a2 < 0)
		{
			//Somehow, v^2 is negative in both cases. v is therefore imaginary and something is horribly wrong. Abort!
			return;
		}
		// Just find which one's NOT negative, and use that
		aToUse = max(a1,a2);
	}
	else
	{
		// Both are positive; use whichever's smaller so it can decay faster
		aToUse = min(a1,a2);
	}
	v = FixedSqrt(aToUse);
	// Okay, so we know the velocity. Let's actually find theta.
	// We can cut the "+/- sqrt" part out entirely, since v was calculated specifically for it to equal zero. So:
	//theta = tantoangle[FixedDiv(aToUse,FixedMul(g,x)) >> DBITS];
	theta = tantoangle[SlopeDiv(aToUse,FixedMul(g,x))];

	// Okay, complicated math done. Let's make this object jump already.

	shot->angle = R_PointToAngle2(shot->x, shot->y, target->x, target->y);

	if (shot->eflags & MFE_VERTICALFLIP)
		shot->z--;
	else
		shot->z++;

	// Horizontal axes first. First parameter is initial horizontal impulse, second is to correct its angle.
	fixedHypotenuse = FixedMul(v, FINECOSINE(theta >> ANGLETOFINESHIFT)); // variable reuse
	shot->momx = FixedMul(fixedHypotenuse, FINECOSINE(shot->angle >> ANGLETOFINESHIFT));
	shot->momy = FixedMul(fixedHypotenuse, FINESINE(shot->angle >> ANGLETOFINESHIFT));
	// Then the vertical axis. No angle-correction needed here.
	shot->momz = FixedMul(v, FINESINE(theta >> ANGLETOFINESHIFT));
	// I hope that's all that's needed, ugh
}

static mobj_t *VS_PredictedPuyoShot(mobj_t *arena, mobj_t *source, mobj_t *shot, mobj_t *target, mobj_t *reticule)
{
	fixed_t x = 0, y = 0, z = INT32_MIN;
	fixed_t momx = 0, momy = 0, mommag = 0;
	angle_t mompoint = 0;

	if (P_MobjWasRemoved(arena))
	{
		CONS_Alert(CONS_ERROR, "VS_PredictedPuyoShot: No Versus arena provided.");
		return NULL;
	}

	// handle coords
	if (P_MobjWasRemoved(target) == false) // aimed leading shot
	{
		x = target->x;
		y = target->y;
		z = target->floorz;

		if (P_MobjWasRemoved(shot) == false
			&& (target->player == NULL || P_PlayerInPain(target->player) == false))
		{
			// a beyond mediocre guess - intentionally not DIRECTLY tied to maxarena because
			// this was determined via trial and error and shouldn't change
			fixed_t possiblearcmultiplier = (
				((TICRATE/4)*FRACUNIT)
				+ (3*TICRATE*
					FixedDiv(
						FixedHypot(x - shot->x, y - shot->y),
						2*280*FRACUNIT
					)
				)
			);

			momx = FixedMul(target->momx, possiblearcmultiplier);
			momy = FixedMul(target->momy, possiblearcmultiplier);

			mommag = FixedHypot(momx, momy);
			mompoint = R_PointToAngle2(0, 0, momx, momy);

			fixed_t *predict = VS_PredictAroundArena(
				arena,
				target,
				mommag,
				mompoint,
				shot->radius,
				false,
				0
			);

			momx = predict[0] - x;
			momy = predict[1] - y;
		}
	}
	else // unaimed random shot
	{
		fixed_t *predict = VS_RandomPointOnArena(
			arena,
			(P_MobjWasRemoved(shot) ? 0 : shot->radius)
		);

		x = predict[0];
		y = predict[1];
	}

	// handle reticule
	fixed_t tempz = ((z == INT32_MIN && !P_MobjWasRemoved(source)) ? source->z : z);

	if (P_MobjWasRemoved(reticule) == false)
	{
		P_SetOrigin(reticule, x, y, tempz);
	}
	else
	{
		reticule = P_SpawnMobj(x, y, tempz, MT_SPIKEDTARGET);
		reticule->renderflags |= RF_NOSPLATBILLBOARD;
		reticule->destscale = 2*reticule->destscale;
		reticule->radius = FixedMul(mobjinfo[MT_PLAYER].radius, mapobjectscale)/2;
		//P_SetScale(reticule, reticule->destscale); -- intentionally not here, for animation

		if (P_MobjWasRemoved(shot) == false)
			P_SetTarget(&reticule->target, shot);
		else if (P_MobjWasRemoved(source) == false)
			P_SetTarget(&reticule->target, source);
	}

	if (z == INT32_MIN)
	{
		z = reticule->old_z = reticule->floorz;
	}

	reticule->z = z + FixedMul(mobjinfo[MT_PLAYER].height, mapobjectscale);

	// handle reticle movement
	if (momx != 0 || momy != 0)
	{
		UINT8 attempt = 0;
		boolean lastsuccess = true;
		// tries to immediately jump to the final location.
		// if that fails, tries to xeno's paradox it:
		// halve the distance and try TWO steps at this magnitude
		while (attempt < 5)
		{
			if (P_TryMove(reticule, reticule->x + momx, reticule->y + momy, false, NULL))
			{
				if (lastsuccess)
					break;

				lastsuccess = true;
				continue;
			}
			lastsuccess = false;
			momx /= 2;
			momy /= 2;
			attempt++;
		}
	}

	// handle launching
	if (P_MobjWasRemoved(shot) == false)
	{
		P_SetTarget(&shot->tracer, reticule);

		VS_PuyoJump(shot, reticule);

		P_Thrust(shot, shot->angle, 3*mapobjectscale); // needs this little extra kick just to make it, for some reason
		shot->momz *= PUYOARCMULTIPLIER;
		shot->flags |= (MF_NOGRAVITY|MF_SHOOTABLE);
		if (P_RandomChance(PR_DECORATION, FRACUNIT/2))
			shot->flags2 |= MF2_BOSSNOTRAP;
	}

	// handle finalisation
	P_SetOrigin(reticule, reticule->x, reticule->y, z);
	return reticule;
}

static mobj_t *referencepuyo = NULL;
static mobj_t *bestpuyo = NULL;
fixed_t bestpuyodist = INT32_MAX;

static inline BlockItReturn_t PIT_GetBestLaunchablePuyo(mobj_t *thing)
{
	if (thing->type != MT_BLENDEYE_PUYO)
		return BMIT_CONTINUE; // not a puyo

	if (thing->cusval > 0)
		return BMIT_CONTINUE; // a bottom puyo

	if (P_MobjWasRemoved(thing->tracer))
		return BMIT_CONTINUE; // a launched puyo

	fixed_t dist = FixedHypot(referencepuyo->x - thing->x, referencepuyo->y - thing->y);

	if (dist >= bestpuyodist)
		return BMIT_CONTINUE; // too far away

	bestpuyo = thing;
	bestpuyodist = dist;

	return BMIT_CONTINUE; // Still could be a better selection
}

static void VS_FindBestPuyo(mobj_t *reference, mobj_t *source)
{
	INT32 bx, by, xl, xh, yl, yh;

	referencepuyo = reference;
	bestpuyo = NULL;
	bestpuyodist = INT32_MAX;

	yh = (unsigned)(source->y + (source->radius + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(source->y - (source->radius + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(source->x + (source->radius + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(source->x - (source->radius + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX (xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_GetBestLaunchablePuyo);
}

void VS_PuyoThinker(mobj_t *mobj)
{
	if (!mobj->health)
		return;

	if (mobj->z <= mobj->floorz)
	{
		if (mobj->flags & MF_NOCLIPHEIGHT)
		{
			mobj->z = mobj->floorz;
			mobj->momx = mobj->momy = mobj->momz = 0;
			mobj->flags &= ~(MF_NOCLIPHEIGHT);
			//P_KillMobj(mobj);
			P_SetMobjState(mobj, S_BLENDEYE_PUYO_LAND_1);
			mobj->fuse = 20*TICRATE;
		}
		return;
	}

	if (P_MobjWasRemoved(mobj->tracer) || (mobj->tracer->type == MT_SPIKEDTARGET)) // being thrown...
	{
		if (mobj->flags2 & MF2_BOSSNOTRAP)
			mobj->rollangle -= ANG10;
		else
			mobj->rollangle += ANG10;

		mobj->momz -= (PUYOARCMULTIPLIER*FixedMul(gravity, mapobjectscale));
	}
	else if (mobj->tracer->flags2 & MF2_AMBUSH) // being whirled
	{
		if (mobj->movefactor)
		{
			if (mobj->movefactor > 2 || ((mobj->state-states) == S_BLENDEYE_PUYO))
			{
				if ((--mobj->movefactor) == 0)
					P_SetMobjState(mobj, S_BLENDEYE_PUYO_SHOCK);
			}
		}
		else if (mobj->cusval > 0) // popping and throwing the object on top of them
		{
			if ((--mobj->cusval) == 1)
			{
				P_InstaThrust(mobj, mobj->angle + ANGLE_90, 5*FRACUNIT);
				mobj->momz = 6*FRACUNIT;

				VS_FindBestPuyo(mobj, mobj->tracer);
				if (bestpuyo != NULL)
				{
					VS_PredictedPuyoShot(
						(P_MobjWasRemoved(mobj->tracer->tracer) == false
							? mobj->tracer->tracer->tracer
							: NULL
						),
						mobj->tracer,
						bestpuyo,
						mobj->tracer->target,
						NULL
					);
				}

				P_KillMobj(mobj, mobj->tracer, mobj->tracer, DMG_NORMAL);
				return;
			}
		}
		fixed_t x = mobj->tracer->x, y = mobj->tracer->y, z = mobj->z+1;
		fixed_t dist = mobj->cvmem;

		if (mobj->extravalue1 == mobj->tracer->extravalue1 || mobj->cvmem == mobj->tracer->cvmem)
			;
		else if (mobj->z >= mobj->tracer->extravalue1)
			dist = mobj->tracer->cvmem;
		else
			dist = mobj->cvmem + FixedMul(FixedDiv(mobj->z - mobj->extravalue1, mobj->tracer->extravalue1 - mobj->extravalue1), mobj->tracer->cvmem - mobj->cvmem);

		mobj->angle += FixedAngle(((3*TICRATE/4) - mobj->movefactor)*FRACUNIT);
		x += P_ReturnThrustX(mobj, mobj->angle, dist/2);
		y += P_ReturnThrustY(mobj, mobj->angle, dist/2);
		if (mobj->flags2 & MF2_STRONGBOX)
		{
			z = mobj->extravalue1+1;
			if (((leveltime & 1) == 1) != (mobj->cusval > 0))
				z += mobj->scale;
		}
		else // being thrown off the side of pinch mode
		{
			if (mobj->z >= mobj->tracer->extravalue1)
			{
				mobj_t *target = ((mobj->flags2 & MF2_BOSSNOTRAP) == 0)
					? mobj->tracer->target
					: NULL;
				mobj->flags2 &= ~MF2_BOSSNOTRAP;
				VS_PredictedPuyoShot(
					(P_MobjWasRemoved(mobj->tracer->tracer) == false
						? mobj->tracer->tracer->tracer
						: NULL
					),
					mobj->tracer,
					mobj,
					target,
					NULL
				);
				S_StartSound(NULL, sfx_mbs5b);
				return;
			}
			z += mobj->scale;
		}
		P_MoveOrigin(mobj, x, y, z);
		mobj->flags |= MF_NOGRAVITY;
	}
	else // being inserted into the blender
	{
		if (mobj->momz < 0)
		{
			if (mobj->z < mobj->extravalue1)
			{
				mobj->momz = 0;
				mobj->flags |= MF_NOGRAVITY;
				mobj->z = mobj->extravalue1;
				S_StartSound(NULL, sfx_mbs42);
				P_SetMobjState(mobj, S_BLENDEYE_PUYO_LAND_1);
			}
		}
		else if (mobj->z > mobj->extravalue1)
			mobj->flags &= ~MF_NOGRAVITY;

		if (
			(mobj->state-states) != S_BLENDEYE_PUYO_SHOCK
			&& mobj->tracer->movedir == BLENDEYE_EXPLODING
			&& (
				(mobj->tracer->movecount == 3)
				|| (
					mobj->tracer->movecount == 4
					&& P_RandomChance(PR_DECORATION, FRACUNIT/2)
				)
			)
		)
		{
			P_SetMobjState(mobj, S_BLENDEYE_PUYO_SHOCK);
		}
	}
	mobj->sprite = mobj->movedir;
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
