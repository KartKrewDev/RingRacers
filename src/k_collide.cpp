// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_collide.cpp
/// \brief SRB2Kart item collision hooks

#include <algorithm>

#include "k_collide.h"
#include "doomtype.h"
#include "p_mobj.h"
#include "k_kart.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_main.h" // R_PointToAngle2, R_PointToDist2
#include "hu_stuff.h" // Sink snipe print
#include "doomdef.h" // Sink snipe print
#include "g_game.h" // Sink snipe print
#include "k_objects.h"
#include "k_roulette.h"
#include "k_podium.h"
#include "k_powerup.h"
#include "k_hitlag.h"
#include "m_random.h"
#include "k_hud.h" // K_AddMessage
#include "m_easing.h"
#include "r_skins.h"

angle_t K_GetCollideAngle(mobj_t *t1, mobj_t *t2)
{
	fixed_t momux, momuy;
	angle_t test;

	if (!(t1->flags & MF_PAPERCOLLISION))
	{
		return R_PointToAngle2(t1->x, t1->y, t2->x, t2->y)+ANGLE_90;
	}

	test = R_PointToAngle2(0, 0, t2->momx, t2->momy) + ANGLE_90 - t1->angle;
	if (test > ANGLE_180)
		test = t1->angle + ANGLE_180;
	else
		test = t1->angle;

	// intentional way around - sine...
	momuy = P_AproxDistance(t2->momx, t2->momy);
	momux = t2->momx - P_ReturnThrustY(t2, test, 2*momuy);
	momuy = t2->momy - P_ReturnThrustX(t2, test, 2*momuy);

	return R_PointToAngle2(0, 0, momux, momuy);
}

extern "C" consvar_t cv_debugpickmeup;

boolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2)
{
	boolean damageitem = false;

	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (((t1->type == MT_BANANA_SHIELD) && (t2->type == MT_BANANA_SHIELD))
		&& (t1->target == t2->target)) // Don't hit each other if you have the same target
		return true;

	if (t1->type == MT_BALLHOG && t2->type == MT_BALLHOG)
		return true; // Ballhogs don't collide with eachother

	if (t1->type == MT_BALLHOGBOOM && t2->type == MT_BALLHOGBOOM)
		return true; // Ballhogs don't collide with eachother

	if (t1->type == MT_BALLHOGBOOM && t2->type == MT_PLAYER && t1->target == t2 && !cv_debugpickmeup.value)
		return true; // Allied hog explosion, not snatchable but shouldn't damage

	if (K_TryPickMeUp(t1, t2, false))
		return true;

	if (t2->player)
	{
		if (t2->player->flashing > 0 && t2->hitlag == 0)
			return true;

		// Banana snipe!
		if (t1->type == MT_BANANA && t1->health > 1)
			S_StartSound(t2, sfx_bsnipe);

		if (t1->type != MT_BALLHOGBOOM) // ballhog booms linger and expire after their anim is done
		{
			damageitem = true;
		}

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
		}
		else if (K_IsRidingFloatingTop(t2->player))
		{
			// Float over silly banana
			damageitem = false;
		}
		else
		{
			P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL|DMG_WOMBO);
		}
	}
	else if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_ORBINAUT || t2->type == MT_ORBINAUT_SHIELD
		|| t2->type == MT_JAWZ || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BALLHOG || t2->type == MT_GACHABOM)
	{
		// Other Item Damage
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		P_SetObjectMomZ(t2, 24*FRACUNIT, false);
		P_InstaThrust(t2, bounceangle, 16*FRACUNIT);

		P_SpawnMobj(t2->x/2 + t1->x/2, t2->y/2 + t1->y/2, t2->z/2 + t1->z/2, MT_ITEMCLASH);

		damageitem = true;
	}
	else if (t2->type == MT_SSMINE_SHIELD || t2->type == MT_SSMINE || t2->type == MT_LANDMINE)
	{
		damageitem = true;
		// Bomb death
		P_KillMobj(t2, t1, t1, DMG_NORMAL);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL);
		damageitem = true;
	}

	if (damageitem && P_MobjWasRemoved(t1) == false)
	{
		angle_t bounceangle;

		if (P_MobjWasRemoved(t2) == false)
		{
			bounceangle = K_GetCollideAngle(t2, t1);
		}
		else
		{
			bounceangle = K_MomentumAngle(t1) + ANGLE_90;
			t2 = NULL; // handles the arguments to P_KillMobj
		}

		// This Item Damage
		S_StartSound(t1, t1->info->deathsound);
		P_KillMobj(t1, t2, t2, DMG_NORMAL);

		P_SetObjectMomZ(t1, 24*FRACUNIT, false);

		P_InstaThrust(t1, bounceangle, 16*FRACUNIT);
	}

	return true;
}

boolean K_EggItemCollide(mobj_t *t1, mobj_t *t2)
{
	// Push fakes out of other item boxes
	if (t2->type == MT_RANDOMITEM || t2->type == MT_EGGMANITEM)
	{
		P_InstaThrust(t1, R_PointToAngle2(t2->x, t2->y, t1->x, t1->y), t2->radius/4);
		return true;
	}

	if (t2->player)
	{
		if ((t1->target == t2 || t1->target == t2->target) && (t1->threshold > 0))
			return true;

		if (t1->health <= 0 || t2->health <= 0)
			return true;

		if (K_TryPickMeUp(t1, t2, false))
			return true;

		if (!P_CanPickupItem(t2->player, PICKUP_EGGBOX))
			return true;

		K_DropItems(t2->player);
		K_StartEggmanRoulette(t2->player);

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
			P_KillMobj(t1, t2, t2, DMG_NORMAL);
			return true;
		}
		else
		{
			Obj_SpawnItemDebrisEffects(t1, t2);

#if 0
			// Eggbox snipe!
			if (t1->type == MT_EGGMANITEM && t1->health > 1)
				S_StartSound(t2, sfx_bsnipe);
#endif

			if (t1->target && t1->target->player)
			{
				t2->player->eggmanblame = t1->target->player - players;

				if (t1->target->hnext == t1)
				{
					P_SetTarget(&t1->target->hnext, NULL);
					t1->target->player->itemflags &= ~IF_EGGMANOUT;
				}
			}

			P_RemoveMobj(t1);
			return true;
		}
	}

	return true;
}

static mobj_t *grenade;
static fixed_t explodedist;
static boolean explodespin;
static INT32 minehitlag;

static inline boolean PIT_SSMineChecks(mobj_t *thing)
{
	if (thing == grenade) // Don't explode yourself! Endless loop!
		return true;

	if (thing->health <= 0)
		return true;

	if (!(thing->flags & MF_SHOOTABLE) || (thing->flags & MF_SCENERY))
		return true;

	if (thing->player && (thing->player->spectator || thing->player->hyudorotimer > 0))
		return true;

	if (P_AproxDistance(P_AproxDistance(thing->x - grenade->x, thing->y - grenade->y), thing->z - grenade->z) > explodedist)
		return true; // Too far away

	if (P_CheckSight(grenade, thing) == false)
		return true; // Not in sight

	return false;
}

extern "C" consvar_t cv_debugpickmeup;

static inline BlockItReturn_t PIT_SSMineSearch(mobj_t *thing)
{
	if (grenade == NULL || P_MobjWasRemoved(grenade))
		return BMIT_ABORT; // There's the possibility these can chain react onto themselves after they've already died if there are enough all in one spot

	if (grenade->flags2 & MF2_DEBRIS) // don't explode twice
		return BMIT_ABORT;

	switch (thing->type)
	{
	case MT_PLAYER: // Don't explode for anything but an actual player.
	case MT_SPECIAL_UFO: // Also UFO catcher
		break;

	default:
		return BMIT_CONTINUE;
	}

	if (!cv_debugpickmeup.value)
	{
		if (grenade->target && !P_MobjWasRemoved(grenade->target))
		{
			if (thing == grenade->target) // Don't blow up at your owner instantly.
				return BMIT_CONTINUE;

			if (grenade->target->player && thing->player && G_SameTeam(grenade->target->player, thing->player))
				return BMIT_CONTINUE;
		}
	}

	if (PIT_SSMineChecks(thing) == true)
		return BMIT_CONTINUE;

	// Explode!
	P_SetMobjState(grenade, grenade->info->deathstate);
	return BMIT_ABORT;
}

void K_DoMineSearch(mobj_t *actor, fixed_t size)
{
	INT32 bx, by, xl, xh, yl, yh;

	explodedist = FixedMul(size, actor->scale);
	grenade = actor;

	yh = (unsigned)(actor->y + (explodedist + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(actor->y - (explodedist + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(actor->x + (explodedist + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(actor->x - (explodedist + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX (xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_SSMineSearch);
}

static inline BlockItReturn_t PIT_SSMineExplode(mobj_t *thing)
{
	const INT32 oldhitlag = thing->hitlag;
	INT32 lagadded;

	if (grenade == NULL || P_MobjWasRemoved(grenade))
		return BMIT_ABORT; // There's the possibility these can chain react onto themselves after they've already died if there are enough all in one spot

#if 0
	if (grenade->flags2 & MF2_DEBRIS) // don't explode twice
		return BMIT_ABORT;
#endif

	if (PIT_SSMineChecks(thing) == true)
		return BMIT_CONTINUE;

	// Don't do Big Boy Damage to the UFO Catcher with
	// lingering spinout damage
	if (thing->type == MT_SPECIAL_UFO && explodespin)
	{
		return BMIT_CONTINUE;
	}

	P_DamageMobj(thing, grenade, grenade->target, 1, (explodespin ? DMG_NORMAL : DMG_EXPLODE));

	lagadded = (thing->hitlag - oldhitlag);

	if (lagadded > minehitlag)
	{
		minehitlag = lagadded;
	}

	return BMIT_CONTINUE;
}

tic_t K_MineExplodeAttack(mobj_t *actor, fixed_t size, boolean spin)
{
	INT32 bx, by, xl, xh, yl, yh;

	explodespin = spin;
	explodedist = FixedMul(size, actor->scale);
	grenade = actor;
	minehitlag = 0;

	// Use blockmap to check for nearby shootables
	yh = (unsigned)(actor->y + explodedist - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(actor->y - explodedist - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(actor->x + explodedist - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(actor->x - explodedist - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX (xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_SSMineExplode);

	// Set this flag to ensure that the inital action won't be triggered twice.
	actor->flags2 |= MF2_DEBRIS;

	if (minehitlag == 0)
	{
		minehitlag = actor->hitlag;
	}

	// Set this flag to ensure the hitbox timer doesn't get extended with every player hit
	actor->flags |= MF_NOHITLAGFORME;
	actor->hitlag = 0; // same deal

	if (!spin)
	{
		return minehitlag;
	}

	return 0;
}

boolean K_MineCollide(mobj_t *t1, mobj_t *t2)
{
	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (t2->player)
	{
		if (t2->player->flashing > 0 && t2->hitlag == 0)
			return true;

		if (K_TryPickMeUp(t1, t2, false))
			return true;

		// Bomb punting
		if ((t1->state >= &states[S_SSMINE1] && t1->state <= &states[S_SSMINE4])
			|| (t1->state >= &states[S_SSMINE_DEPLOY8] && t1->state <= &states[S_SSMINE_EXPLODE2]))
		{
			P_KillMobj(t1, t2, t2, DMG_NORMAL);
		}
		else
		{
			K_PuntMine(t1, t2);
		}
	}
	else if (t2->type == MT_ORBINAUT || t2->type == MT_JAWZ
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_GACHABOM)
	{
		// Bomb death
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		P_KillMobj(t1, t2, t2, DMG_NORMAL);

		// Other Item Damage
		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		P_SetObjectMomZ(t2, 24*FRACUNIT, false);
		P_InstaThrust(t2, bounceangle, 16*FRACUNIT);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Bomb death
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
		// Shootable damage
		P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL);
	}

	return true;
}

boolean K_LandMineCollide(mobj_t *t1, mobj_t *t2)
{
	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (K_TryPickMeUp(t1, t2, false))
		return true;

	if (t2->player)
	{
		const INT32 oldhitlag = t2->hitlag;

		if (t2->player->flashing)
			return true;

		// Banana snipe!
		if (t1->health > 1)
		{
			if (t1->target
			&& t1->target->player
			&& t2->player != t1->target->player)
			{
				t1->target->player->roundconditions.landmine_dunk = true;
				t1->target->player->roundconditions.checkthisframe = true;
			}

			S_StartSound(t2, sfx_bsnipe);
		}

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
			K_SetHitLagForObjects(t2, t1, t1->target, 3, false);
		}
		else
		{
			// Player Damage
			P_DamageMobj(t2, t1, t1->target, 1, DMG_TUMBLE);
		}

		t1->reactiontime = (t2->hitlag - oldhitlag);
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}
	else if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_ORBINAUT || t2->type == MT_ORBINAUT_SHIELD
		|| t2->type == MT_JAWZ || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BALLHOG || t2->type == MT_GACHABOM)
	{
		// Other Item Damage
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		if (t2->eflags & MFE_VERTICALFLIP)
			t2->z -= t2->height;
		else
			t2->z += t2->height;

		P_SpawnMobj(t2->x/2 + t1->x/2, t2->y/2 + t1->y/2, t2->z/2 + t1->z/2, MT_ITEMCLASH);

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		if (P_MobjWasRemoved(t2))
		{
			t2 = NULL; // handles the arguments to P_KillMobj
		}
		else
		{
			P_SetObjectMomZ(t2, 24*FRACUNIT, false);
			P_InstaThrust(t2, bounceangle, 16*FRACUNIT);

			t1->reactiontime = t2->hitlag;
		}
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}
	else if (t2->type == MT_SSMINE_SHIELD || t2->type == MT_SSMINE || t2->type == MT_LANDMINE)
	{
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
		// Bomb death
		P_KillMobj(t2, t1, t1, DMG_NORMAL);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL);

		if (P_MobjWasRemoved(t2))
		{
			t2 = NULL; // handles the arguments to P_KillMobj
		}
		else
		{
			t1->reactiontime = t2->hitlag;
		}

		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}

	return true;
}

boolean K_DropTargetCollide(mobj_t *t1, mobj_t *t2)
{
	mobj_t *draggeddroptarget = (t1->type == MT_DROPTARGET_SHIELD) ? t1->target : NULL;
	UINT8 strength;

	if (((t1->target == t2) || (t1->target == t2->target)) && ((t1->threshold > 0 && t2->type == MT_PLAYER) || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (t2->player && (t2->player->hyudorotimer || t2->player->justbumped))
		return true;

	if (K_TryPickMeUp(t1, t2, false))
		return true;

	if (draggeddroptarget && P_MobjWasRemoved(draggeddroptarget))
		draggeddroptarget = NULL; // Beware order-of-execution on crushers, I guess?!

	if (t1->health > 3) // forward thrown
	{
		strength = 0;
	}
	else if (t1->reactiontime == 0 || draggeddroptarget)
	{
		strength = 80;
	}
	else
	{
		strength = 140;
	}

	// Intensify bumps if already spinning...
	P_Thrust(t1, R_PointToAngle2(t1->x, t1->y, t2->x, t2->y), strength * t1->scale);

	if (draggeddroptarget)
	{
		// "Pass through" the shock of the impact, part 1.
		t1->momx = t1->target->momx;
		t1->momy = t1->target->momy;
		t1->momz = t1->target->momz;
	}

	fixed_t bumppower = FRACUNIT;
	if (t2->player)
	{
		fixed_t speeddampen = FixedDiv(t2->player->speed, 2*K_GetKartSpeed(t2->player, false, false));
		bumppower = Easing_InQuad(
			std::min(speeddampen, FRACUNIT),
			FRACUNIT,
			3*FRACUNIT/4
		);
		if (t2->player->tripwireLeniency || t2->player->tripwirePass != TRIPWIRE_NONE)
			bumppower = FRACUNIT/2;
	}

	if (t2->type == MT_INSTAWHIP)
		bumppower = 0;

	{
		angle_t t2angle = R_PointToAngle2(t2->momx, t2->momy, 0, 0);
		angle_t t2deflect;
		fixed_t t1speed, t2speed;

		if (t2->type == MT_INSTAWHIP && t2->target && !P_MobjWasRemoved(t2->target))
		{
			t2angle = R_PointToAngle2(t2->target->momx, t2->target->momy, 0, 0);
			t2speed = FixedHypot(t2->target->momx, t2->target->momy);
			P_InstaThrust(t1, ANGLE_180 + R_PointToAngle2(t1->x, t1->y, t2->x, t2->y), 100*t2->target->scale + t2speed);
		}
		else
		{
			K_KartBouncing(t1, t2);
			t2speed = FixedHypot(t2->momx, t2->momy);
		}

		t1speed = FixedHypot(t1->momx, t1->momy);

		t2deflect = t2angle - R_PointToAngle2(0, 0, t2->momx, t2->momy);
		if (t2deflect > ANGLE_180)
			t2deflect = InvAngle(t2deflect);
		if (t2deflect < ANG10)
			P_InstaThrust(t2, t2angle, FixedMul(t2speed, bumppower));

		t1->angle = t1->old_angle = R_PointToAngle2(0, 0, t1->momx, t1->momy);

		t1->reactiontime = (7 * (t1speed + t2speed)) / (4 * t1->scale);
		if (t1->reactiontime < 10)
			t1->reactiontime = 10;
		t1->threshold = 10;
	}

	t1->renderflags &= ~RF_FULLDARK; // brightest on the bump

	if (draggeddroptarget)
	{
		// "Pass through" the shock of the impact, part 2.
		draggeddroptarget->momx = t1->momx;
		draggeddroptarget->momy = t1->momy;
		draggeddroptarget->momz = t1->momz;

		// Have the drop target travel between them.
		t1->momx = (t1->momx + t2->momx)/2;
		t1->momy = (t1->momy + t2->momy)/2;
		t1->momz = (t1->momz + t2->momz)/2;

		K_AddHitLag(t1->target, 6, false);
	}

	K_AddHitLag(t1, 6, true);
	K_AddHitLag(t2, 6, false);

	if (t2->type == MT_INSTAWHIP && t2->target && !P_MobjWasRemoved(t2->target))
		K_AddHitLag(t2->target, 6, false);

	{
		mobj_t *ghost = P_SpawnGhostMobj(t1);
		UINT8 i;

		P_SetScale(ghost, 3*ghost->destscale/2);
		ghost->destscale = 15*ghost->destscale/2;
		ghost->fuse = 10;
		ghost->scalespeed = (ghost->destscale - ghost->scale)/ghost->fuse;

		for (i = 0; i < 2; i++)
		{
			mobj_t *blast = P_SpawnMobjFromMobj(t1, 0, 0, FixedDiv(t1->height, t1->scale), MT_BATTLEBUMPER_BLAST);
			P_SetScale(blast, 5*blast->scale/2);

			blast->angle = R_PointToAngle2(0, 0, t1->momx, t1->momy) + ANGLE_45;
			if (i & 1)
			{
				blast->angle += ANGLE_90;
			}

			blast->destscale *= 10;
		}
	}

	t1->flags |= MF_SHOOTABLE;
	// The following sets t1->target to t2, so draggeddroptarget keeps it persisting...
	P_DamageMobj(t1, t2, (t2->target ? t2->target : t2), 1, DMG_NORMAL);

	switch (t1->health)
	{
		case 3:
			t1->color = SKINCOLOR_LIME;
			break;

		case 2:
			t1->color = SKINCOLOR_GOLD;
			break;

		case 1:
			t1->color = SKINCOLOR_CRIMSON;
			break;
	}

	t1->flags &= ~MF_SHOOTABLE;

	t1->spritexscale = 3*FRACUNIT;
	t1->spriteyscale = 3*FRACUNIT/2;

	if (!t2->player)
	{
		t2->angle += ANGLE_180;
		if (t2->type == MT_JAWZ)
			P_SetTarget(&t2->tracer, t2->target); // Back to the source!

		// Reflected item becomes owned by the DT owner, so it becomes dangerous the the thrower
		if (t1->target && !P_MobjWasRemoved(t1->target))
			P_SetTarget(&t2->target, t1->target);

		t2->threshold = 10;
	}

	if (t1->reactiontime > 1000) {
		S_StartSound(t2, sfx_kdtrg3);
	} else if (t1->reactiontime > 500) {
		S_StartSound(t2, sfx_kdtrg2);
	} else {
		S_StartSound(t2, sfx_kdtrg1);
	}

	if (t1->tracer && t1->tracer->player && t2->player && t2->player != t1->tracer->player)
	{
		K_SpawnAmps(t1->tracer->player, K_PvPAmpReward(20, t1->tracer->player, t2->player), t1);
	}

	if (draggeddroptarget && !P_MobjWasRemoved(draggeddroptarget) && draggeddroptarget->player)
	{
		// The following removes t1, be warned
		// (its newly assigned properties are moved across)
		K_DropHnextList(draggeddroptarget->player);
		// Do NOT modify or reference t1 after this line
		// I mean it! Do not even absentmindedly try it
	}

	return true;
}

static mobj_t *lightningSource;
static fixed_t lightningDist;

static inline BlockItReturn_t PIT_LightningShieldAttack(mobj_t *thing)
{
	if (lightningSource == NULL || P_MobjWasRemoved(lightningSource))
	{
		// Invalid?
		return BMIT_ABORT;
	}

	if (thing == NULL || P_MobjWasRemoved(thing))
	{
		// Invalid?
		return BMIT_ABORT;
	}

	if (thing == lightningSource)
	{
		// Don't explode yourself!!
		return BMIT_CONTINUE;
	}

	if (thing->health <= 0)
	{
		// Dead
		return BMIT_CONTINUE;
	}

	if (thing->type != MT_SPB)
	{
		if (!(thing->flags & MF_SHOOTABLE) || (thing->flags & MF_SCENERY))
		{
			// Not shootable
			return BMIT_CONTINUE;
		}
	}

	if (thing->player && thing->player->spectator)
	{
		// Spectator
		return BMIT_CONTINUE;
	}

	if (P_AproxDistance(thing->x - lightningSource->x, thing->y - lightningSource->y) > lightningDist + thing->radius)
	{
		// Too far away
		return BMIT_CONTINUE;
	}

	// see if it went over / under
	if (lightningSource->z - lightningDist > thing->z + thing->height)
		return BMIT_CONTINUE; // overhead
	if (lightningSource->z + lightningSource->height + lightningDist < thing->z)
		return BMIT_CONTINUE; // underneath

#if 0
	if (P_CheckSight(lightningSource, thing) == false)
	{
		// Not in sight
		return BMIT_CONTINUE;
	}
#endif

	P_DamageMobj(thing, lightningSource, lightningSource, 1, DMG_VOLTAGE|DMG_CANTHURTSELF|DMG_WOMBO);
	return BMIT_CONTINUE;
}

void K_LightningShieldAttack(mobj_t *actor, fixed_t size)
{
	INT32 bx, by, xl, xh, yl, yh;

	lightningDist = FixedMul(size, actor->scale);
	lightningSource = actor;

	// Use blockmap to check for nearby shootables
	yh = (unsigned)(actor->y + lightningDist - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(actor->y - lightningDist - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(actor->x + lightningDist - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(actor->x - lightningDist - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX (xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_LightningShieldAttack);
}

boolean K_BubbleShieldCanReflect(mobj_t *t1, mobj_t *t2)
{
	return (t2->type == MT_ORBINAUT || t2->type == MT_JAWZ || t2->type == MT_GACHABOM
		|| t2->type == MT_BANANA || t2->type == MT_EGGMANITEM || t2->type == MT_BALLHOG
		|| t2->type == MT_SSMINE || t2->type == MT_LANDMINE || t2->type == MT_SINK
		|| t2->type == MT_GARDENTOP
		|| t2->type == MT_DROPTARGET
		|| t2->type == MT_KART_LEFTOVER
		|| (t2->type == MT_PLAYER && t1->target != t2));
}

boolean K_BubbleShieldReflect(mobj_t *t1, mobj_t *t2)
{
	mobj_t *owner = t1->player ? t1 : t1->target;

	if (t2->target != owner || !t2->threshold || t2->type == MT_DROPTARGET)
	{
		if (t1->player && K_PlayerGuard(t1->player))
		{
			K_KartSolidBounce(t1, t2);
			K_DoPowerClash(t1, t2);
		}
		if (!t2->momx && !t2->momy)
		{
			t2->momz += (24*t2->scale) * P_MobjFlip(t2);
		}
		else
		{
			t2->momx = -6*t2->momx;
			t2->momy = -6*t2->momy;
			t2->momz = -6*t2->momz;
			t2->angle += ANGLE_180;
		}
		if (t2->type == MT_JAWZ)
			P_SetTarget(&t2->tracer, t2->target); // Back to the source!
		P_SetTarget(&t2->target, owner); // Let the source reflect it back again!
		t2->threshold = 10;
		S_StartSound(t1, sfx_s3k44);
	}

	return true;
}

boolean K_BubbleShieldCollide(mobj_t *t1, mobj_t *t2)
{
	if (t2->type == MT_PLAYER)
	{
		// Counter desyncs
		/*mobj_t *oldthing = thing;
		mobj_t *oldg_tm.thing = g_tm.thing;

		P_Thrust(g_tm.thing, R_PointToAngle2(thing->x, thing->y, g_tm.thing->x, g_tm.thing->y), 4*thing->scale);

		thing = oldthing;
		P_SetTarget(&g_tm.thing, oldg_tm.thing);*/

		boolean hit = false;

		if (K_KartBouncing(t2, t1->target) == true)
		{
			if (t2->player && t1->target && t1->target->player)
			{
				hit = K_PvPTouchDamage(t2, t1->target);
			}

			// Don't play from t1 else it gets cut out... for some reason.
			S_StartSound(t2, sfx_s3k44);
		}

		if (hit && (gametyperules & GTR_BUMPERS))
		{
			K_PopBubbleShield(t1->target->player);
			return false;
		}
		else
		{
			return true;
		}
	}

	if (K_BubbleShieldCanReflect(t1, t2))
	{
		return K_BubbleShieldReflect(t1, t2);
	}

	if (t2->flags & MF_SHOOTABLE)
	{
		P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL);
	}

	return true;
}

boolean K_InstaWhipCollide(mobj_t *shield, mobj_t *victim)
{
	int victimHitlag = 10;
	int attackerHitlag = 4;

	// EV1 is used to indicate that we should no longer hit monitors.
	// EV2 indicates we should no longer hit anything.
	if (shield->extravalue2)
		return false;

	mobj_t *attacker = shield->target;

	if (!attacker || P_MobjWasRemoved(attacker) || !attacker->player)
		return false; // How did we even get here?

	player_t *attackerPlayer = attacker->player;

	if (victim->player)
	{
		player_t *victimPlayer = victim->player;

		if (victim == attacker)
			return false;

		// If both players have a whip, hits are order-of-execution dependent and that sucks.
		// Player expectation is a clash here.
		if (victimPlayer->whip && !P_MobjWasRemoved(victimPlayer->whip))
		{
			if (victim->hitlag != 0)
				return false;

			victimPlayer->whip->extravalue2 = 1;
			shield->extravalue2 = 1;

			K_DoPowerClash(victim, attacker);

			victim->renderflags &= ~RF_DONTDRAW;
			attacker->renderflags &= ~RF_DONTDRAW;

			angle_t thrangle = R_PointToAngle2(attacker->x, attacker->y, victim->x, victim->y);
			P_Thrust(victim, thrangle, mapobjectscale*28);
			P_Thrust(attacker, ANGLE_180 + thrangle, mapobjectscale*28);

			return false;
		}

		if (P_PlayerInPain(victimPlayer) ? victim->hitlag == 0 : victimPlayer->flashing == 0)
		{
			// Instawhip _always_ loses to guard.
			if (K_PlayerGuard(victimPlayer))
			//if (true)
			{
				victimHitlag = 3*victimHitlag;

				if (P_PlayerInPain(attackerPlayer))
					return false; // never punish shield more than once

				angle_t thrangle = R_PointToAngle2(victim->x, victim->y, shield->x, shield->y);
				attacker->momx = attacker->momy = 0;
				P_Thrust(attacker, thrangle, mapobjectscale*7);

				// target is inflictor: hack to let invincible players lose to guard
				P_DamageMobj(attacker, attacker, victim, 1, DMG_TUMBLE);

				// A little extra juice, so successful reads are usually positive or zero on spheres.
				victimPlayer->spheres = std::min(victimPlayer->spheres + 10, 40);

				shield->renderflags &= ~RF_DONTDRAW;
				shield->flags |= MF_NOCLIPTHING;

				// Attacker should be free to all reasonable followups.
				attacker->renderflags &= ~RF_DONTDRAW;
				attackerPlayer->spindashboost = 0;
				attackerPlayer->sneakertimer = 0;
				attackerPlayer->panelsneakertimer = 0;
				attackerPlayer->weaksneakertimer = 0;
				attackerPlayer->instaWhipCharge = 0;
				attackerPlayer->flashing = 0;

				K_AddMessageForPlayer(victimPlayer, "Whip Reflected!", false, false);
				K_AddMessageForPlayer(attackerPlayer, "COUNTERED!!", false, false);

				// Localized broly for a local event.
				if (mobj_t *broly = Obj_SpawnBrolyKi(victim, victimHitlag/2))
				{
					broly->extravalue2 = 16*mapobjectscale;
				}

				P_PlayVictorySound(victim);

				P_DamageMobj(attacker, attacker, victim, 1, DMG_TUMBLE);

				S_StartSound(victim, sfx_mbv92);
				K_AddHitLag(attacker, victimHitlag, true);
				K_AddHitLag(victim, attackerHitlag, false);

				K_DoPowerClash(shield, victim); // REJECTED

				shield->extravalue2 = 1;

				return true;
			}

			// if you're here, you're getting hit
			boolean hit = P_DamageMobj(victim, shield, attacker, 1, DMG_WHUMBLE);

			if (!hit)
				return false;

			K_DropPowerUps(victimPlayer);

			angle_t thrangle = ANGLE_180 + R_PointToAngle2(victim->x, victim->y, shield->x, shield->y);
			P_Thrust(victim, thrangle, mapobjectscale*40);

			K_AddHitLag(victim, victimHitlag, true);
			K_AddHitLag(attacker, attackerHitlag, false);
			shield->hitlag = attacker->hitlag;

			if (attackerPlayer->roundconditions.whip_hyuu == false
				&& attackerPlayer->hyudorotimer > 0)
			{
				attackerPlayer->roundconditions.whip_hyuu = true;
				attackerPlayer->roundconditions.checkthisframe = true;
			}

			return true;
		}
		return false;
	}
	else if (victim->type == MT_SUPER_FLICKY)
	{
		if (Obj_IsSuperFlickyWhippable(victim, attacker))
		{
			K_AddHitLag(victim, victimHitlag, true);
			K_AddHitLag(attacker, attackerHitlag, false);
			shield->hitlag = attacker->hitlag;

			Obj_WhipSuperFlicky(victim);
			return true;
		}
		return false;
	}
	else if (victim->type == MT_DROPTARGET || victim->type == MT_DROPTARGET_SHIELD)
	{
		if (K_TryPickMeUp(attacker, victim, true))
		{
			shield->hitlag = attacker->hitlag; // players hitlag is handled in K_TryPickMeUp, and we need to set for the shield too
		}
		else
		{
			K_DropTargetCollide(victim, shield);
		}
		return true;
	}
	else
	{
		if (victim->flags & MF_SHOOTABLE)
		{
			// Monitor hack. We can hit monitors once per instawhip, no multihit shredding!
			// Damage values in Obj_MonitorGetDamage.
			// Apply to UFO also -- steelt 29062023
			if (victim->type == MT_MONITOR || victim->type == MT_BATTLEUFO || victim->type == MT_BALLSWITCH_BALL)
			{
				if (shield->extravalue1 == 1)
					return false;
				shield->extravalue1 = 1;
			}

			if (K_TryPickMeUp(attacker, victim, true))
			{
				shield->hitlag = attacker->hitlag; // players hitlag is handled in K_TryPickMeUp, and we need to set for the shield too
				return true;
			}
			else
			{
				if (P_DamageMobj(victim, shield, attacker, 1, DMG_NORMAL))
				{
					K_AddHitLag(attacker, attackerHitlag, false);
					shield->hitlag = attacker->hitlag;
				}
				return true;
			}
		}
		return false;
	}
}

boolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2)
{
	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (K_TryPickMeUp(t1, t2, false))
		return true;

	if (t2->player)
	{
		if (t2->player->flashing > 0 && t2->hitlag == 0)
			return true;

		S_StartSound(NULL, sfx_bsnipe); // let all players hear it.

		if (t1->target && !P_MobjWasRemoved(t1->target) && t1->target->player)
			K_SpawnAmps(t1->target->player, 50, t2);

		HU_SetCEchoFlags(0);
		HU_SetCEchoDuration(5);
		HU_DoCEcho(va("%s\\was hit by a kitchen sink.\\\\\\\\", player_names[t2->player-players]));
		I_OutputMsg("%s was hit by a kitchen sink.\n", player_names[t2->player-players]);

		P_DamageMobj(t2, t1, t1->target, 1, DMG_INSTAKILL);
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_KillMobj(t2, t2, t1->target, DMG_NORMAL);
		if (P_MobjWasRemoved(t2))
		{
			t2 = NULL; // handles the arguments to P_KillMobj
		}
		// This item damage
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}

	return true;
}

boolean K_FallingRockCollide(mobj_t *t1, mobj_t *t2)
{
	if (t2->player || t2->type == MT_FALLINGROCK)
		K_KartBouncing(t2, t1);
	return true;
}

boolean K_PvPTouchDamage(mobj_t *t1, mobj_t *t2)
{
	if (K_PodiumSequence() == true)
	{
		// Always regular bumps, no ring toss.
		return false;
	}

	// What the fuck is calling this with stale refs? Whatever, validation's cheap.
	if (P_MobjWasRemoved(t1) || P_MobjWasRemoved(t2) || !t1->player || !t2->player)
		return false;

	if (G_SameTeam(t1->player, t2->player))
	{
		return false;
	}

	boolean guard1 = K_PlayerGuard(t1->player);
	boolean guard2 = K_PlayerGuard(t2->player);

	// Bubble Shield physically extends past guard when inflated,
	// makes some sense to suppress this behavior
	if (t1->player->bubbleblowup)
		guard1 = false;
	if (t2->player->bubbleblowup)
		guard2 = false;

	if (guard1 && guard2)
		K_DoPowerClash(t1, t2);
	else if (guard1)
		K_DoGuardBreak(t1, t2);
	else if (guard2)
		K_DoGuardBreak(t2, t1);

	if (guard1 || guard2)
		return false;

	// Clash instead of damage if both parties have any of these conditions
	auto canClash = [](mobj_t *t1, mobj_t *t2)
	{
		return (K_IsBigger(t1, t2) == true)
			|| (t1->player->invincibilitytimer > 0)
			|| (t1->player->flamedash > 0 && t1->player->itemtype == KITEM_FLAMESHIELD)
			|| (t1->player->curshield == KSHIELD_TOP && !K_IsHoldingDownTop(t1->player))
			|| (t1->player->bubbleblowup > 0);
	};

	if (canClash(t1, t2) && canClash(t2, t1))
	{
		K_DoPowerClash(t1, t2);
		return false;
	}

	auto forEither = [t1, t2](auto conditionCallable, auto damageCallable)
	{
		const bool t1Condition = conditionCallable(t1, t2);
		const bool t2Condition = conditionCallable(t2, t1);

		if (t1Condition == true && t2Condition == false)
		{
			damageCallable(t1, t2);
			return true;
		}
		else if (t1Condition == false && t2Condition == true)
		{
			damageCallable(t2, t1);
			return true;
		}

		return false;
	};

	auto doDamage = [](UINT8 damageType)
	{
		return [damageType](mobj_t *t1, mobj_t *t2)
		{
			P_DamageMobj(t2, t1, t1, 1, damageType);
		};
	};

	// Cause tumble on invincibility
	auto shouldTumble = [](mobj_t *t1, mobj_t *t2)
	{
		return (t1->player->invincibilitytimer > 0);
	};

	if (forEither(shouldTumble, doDamage(DMG_TUMBLE)))
	{
		return true;
	}

	// Flame Shield dash damage
	// Bubble Shield blowup damage
	auto shouldWipeout = [](mobj_t *t1, mobj_t *t2)
	{
		return (t1->player->flamedash > 0 && t1->player->itemtype == KITEM_FLAMESHIELD)
			|| (t1->player->bubbleblowup > 0);
	};

	if (forEither(shouldWipeout, doDamage(DMG_WIPEOUT | DMG_WOMBO)))
	{
		return true;
	}

	// Battle Mode Sneaker damage
	// (Pogo Spring damage is handled in head-stomping code)
	if (gametyperules & GTR_BUMPERS)
	{
		auto shouldSteal = [](mobj_t *t1, mobj_t *t2)
		{
			return ((t1->player->sneakertimer > 0 || t1->player->panelsneakertimer > 0 || t1->player->weaksneakertimer > 0)
				&& !P_PlayerInPain(t1->player)
				&& (t1->player->flashing == 0));
		};

		if (forEither(shouldSteal, doDamage(DMG_WIPEOUT | DMG_STEAL | DMG_WOMBO)))
		{
			return true;
		}
	}

	// Cause stumble on scale difference
	auto shouldStumble = [](mobj_t *t1, mobj_t *t2)
	{
		return K_IsBigger(t1, t2);
	};

	auto doStumble = [](mobj_t *t1, mobj_t *t2)
	{
		if (gametyperules & GTR_BUMPERS)
		{
			K_StumblePlayer(t2->player);
			K_SpawnAmps(t1->player, K_PvPAmpReward(20, t1->player, t2->player), t2);
		}
		else
		{
			P_DamageMobj(t2, t1, t1, 1, DMG_WHUMBLE);
		}
	};

	if (forEither(shouldStumble, doStumble))
	{
		return true;
	}

	// Ring sting, this is a bit more unique
	auto doSting = [](mobj_t *t1, mobj_t *t2)
	{
		if (t2->player->curshield != KSHIELD_NONE)
		{
			return false;
		}

		boolean damagedpresting = (t2->player->flashing || P_PlayerInPain(t2->player));

		// CONS_Printf("T1=%s T2=%s\n", player_names[t1->player - players], player_names[t2->player - players]);
		// CONS_Printf("DPS=%d\n", damagedpresting);

		if (P_PlayerInPain(t1->player) || t1->player->flashing)
		{
			// CONS_Printf("T1 pain\n");
			if (!(t1->player->pflags2 & PF2_SAMEFRAMESTUNG))
				return false;
			// CONS_Printf("...but ignored\n");
		}

		bool stung = false;

		if (RINGTOTAL(t2->player) <= 0 && t2->player->ringboostinprogress == 0 && t2->health == 1 && !(t2->player->pflags2 & PF2_UNSTINGABLE))
		{
			P_DamageMobj(t2, t1, t1, 1, DMG_STING|DMG_WOMBO);
			// CONS_Printf("T2 stung\n");
			if (!damagedpresting)
			{
				t2->player->pflags2 |= PF2_SAMEFRAMESTUNG;
				// CONS_Printf("T2 SFS\n");
			}
			stung = true;
		}

		P_PlayerRingBurst(t2->player, 1);

		return stung;
	};

	// No damage hitlag for stinging.
	auto removeDamageHitlag = [](mobj_t *t1, mobj_t *t2)
	{
		t1->eflags &= ~MFE_DAMAGEHITLAG;
	};

	// Looks bad, but "forEither" actually runs if t1 XOR t2 were damaged.
	// I don't even think we use the touchdamage return value but I'm too
	// afraid to change it now. Fix this if you're the next guy and annoyed
	if (forEither(doSting, removeDamageHitlag))
	{
		t1->player->pflags2 &= ~PF2_SAMEFRAMESTUNG;
		t2->player->pflags2 &= ~PF2_SAMEFRAMESTUNG;
		return true;
	}

	t1->player->pflags2 &= ~PF2_SAMEFRAMESTUNG;
	t2->player->pflags2 &= ~PF2_SAMEFRAMESTUNG;

	return false;
}

void K_PuntHazard(mobj_t *t1, mobj_t *t2)
{
	// TODO: spawn a unique mobjtype other than MT_GHOST
	mobj_t *img = P_SpawnGhostMobj(t1);

	K_MakeObjectReappear(t1);

	img->flags &= ~MF_NOGRAVITY;
	img->renderflags = t1->renderflags & ~RF_DONTDRAW;
	img->extravalue1 = 1;
	img->extravalue2 = 2;
	img->fuse = 2*TICRATE;

	struct Vector
	{
		fixed_t x_, y_, z_;
		fixed_t h_ = FixedHypot(x_, y_);
		fixed_t speed_ = std::max(60 * mapobjectscale, FixedHypot(h_, z_) * 2);

		explicit Vector(fixed_t x, fixed_t y, fixed_t z) : x_(x), y_(y), z_(z) {}
		explicit Vector(const mobj_t* mo) :
			Vector(std::max(
				Vector(mo->x - mo->old_x, mo->y - mo->old_y, mo->z - mo->old_z),
				Vector(mo->momx, mo->momy, mo->momz)
			))
		{
		}
		explicit Vector(const Vector&) = default;

		bool operator<(const Vector& b) const { return speed_ < b.speed_; }

		void invert()
		{
			x_ = -x_;
			y_ = -y_;
			z_ = -z_;
		}

		void thrust(mobj_t* mo) const
		{
			angle_t yaw = R_PointToAngle2(0, 0, h_, z_);
			yaw = std::max(AbsAngle(yaw), static_cast<angle_t>(ANGLE_11hh)) + (yaw & ANGLE_180);

			P_InstaThrust(mo, R_PointToAngle2(0, 0, x_, y_), FixedMul(speed_, FCOS(yaw)));
			mo->momz = FixedMul(speed_, FSIN(yaw));
		}
	};

	Vector h_vector(t1);
	Vector p_vector(t2);

	h_vector.invert();

	std::max(h_vector, p_vector).thrust(img);

	K_DoPowerClash(img, t2); // applies hitlag
	P_SpawnMobj(t2->x/2 + t1->x/2, t2->y/2 + t1->y/2, t2->z/2 + t1->z/2, MT_ITEMCLASH);
}

boolean K_PuntCollide(mobj_t *t1, mobj_t *t2)
{
	// MF_SHOOTABLE will get damaged directly, instead
	if (t1->flags & (MF_DONTPUNT | MF_SHOOTABLE))
	{
		return false;
	}

	if (!t2->player)
	{
		return false;
	}

	if (!K_PlayerCanPunt(t2->player))
	{
		return false;
	}

	if (t1->flags & MF_ELEMENTAL)
	{
		K_MakeObjectReappear(t1);

		// copied from MT_ITEMCAPSULE
		UINT8 i;
		INT16 spacing = (t1->radius >> 1) / t1->scale;
		// dust effects
		for (i = 0; i < 10; i++)
		{
			fixed_t rand_x;
			fixed_t rand_y;
			fixed_t rand_z;

			// note: determinate random argument eval order
			rand_z = P_RandomRange(PR_ITEM_DEBRIS, 0, 4*spacing) * FRACUNIT;
			rand_y = P_RandomRange(PR_ITEM_DEBRIS, -spacing, spacing) * FRACUNIT;
			rand_x = P_RandomRange(PR_ITEM_DEBRIS, -spacing, spacing) * FRACUNIT;
			mobj_t *puff = P_SpawnMobjFromMobj(
				t1,
				rand_x,
				rand_y,
				rand_z,
				MT_SPINDASHDUST
			);

			puff->momz = puff->scale * P_MobjFlip(puff);

			P_Thrust(puff, R_PointToAngle2(t2->x, t2->y, puff->x, puff->y), 3*puff->scale);

			puff->momx += t2->momx / 2;
			puff->momy += t2->momy / 2;
			puff->momz += t2->momz / 2;
		}
	}
	else
	{
		K_PuntHazard(t1, t2);
	}

	return true;
}
