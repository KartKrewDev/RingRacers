/// \file  k_collide.c
/// \brief SRB2Kart item collision hooks

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

boolean K_OrbinautJawzCollide(mobj_t *t1, mobj_t *t2)
{
	boolean damageitem = false;
	boolean sprung = false;

	if ((t1->threshold > 0 && t2->hitlag > 0) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if ((t1->type == MT_ORBINAUT_SHIELD || t1->type == MT_JAWZ_SHIELD) && t1->lastlook
		&& (t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD) && t2->lastlook
		&& (t1->target == t2->target)) // Don't hit each other if you have the same target
		return true;

	if (t2->player)
	{
		if ((t2->player->flashing > 0 && t2->hitlag == 0)
			&& !(t1->type == MT_ORBINAUT || t1->type == MT_JAWZ || t1->type == MT_JAWZ_DUD))
			return true;

		if (t2->player->hyudorotimer)
			return true; // no interaction

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
		}
		else
		{
			// Player Damage
			P_DamageMobj(t2, t1, t1->target, 1, DMG_WIPEOUT|DMG_WOMBO);
			K_KartBouncing(t2, t1);
			S_StartSound(t2, sfx_s3k7b);
		}

		damageitem = true;
	}
	else if (t2->type == MT_ORBINAUT || t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_BALLHOG)
	{
		// Other Item Damage
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
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
	else if (t2->flags & MF_SPRING && (t1->type != MT_ORBINAUT_SHIELD && t1->type != MT_JAWZ_SHIELD))
	{
		// Let thrown items hit springs!
		sprung = P_DoSpring(t2, t1);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL);
		damageitem = true;
	}

	if (damageitem)
	{
		// This Item Damage
		angle_t bounceangle = K_GetCollideAngle(t2, t1);
		S_StartSound(t1, t1->info->deathsound);
		P_KillMobj(t1, t2, t2, DMG_NORMAL);

		P_SetObjectMomZ(t1, 8*FRACUNIT, false);
		P_InstaThrust(t1, bounceangle, 16*FRACUNIT);
	}

	if (sprung)
	{
		return false;
	}

	return true;
}

boolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2)
{
	boolean damageitem = false;

	if ((t1->threshold > 0 && t2->hitlag > 0) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (((t1->type == MT_BANANA_SHIELD) && (t2->type == MT_BANANA_SHIELD))
		&& (t1->target == t2->target)) // Don't hit each other if you have the same target
		return true;

	if (t1->type == MT_BALLHOG && t2->type == MT_BALLHOG)
		return true; // Ballhogs don't collide with eachother

	if (t2->player)
	{
		if (t2->player->flashing > 0 && t2->hitlag == 0)
			return true;

		// Banana snipe!
		if (t1->type == MT_BANANA && t1->health > 1)
			S_StartSound(t2, sfx_bsnipe);

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
		}
		else
		{
			P_DamageMobj(t2, t1, t1->target, 1, DMG_NORMAL|DMG_WOMBO);
		}

		damageitem = true;
	}
	else if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_ORBINAUT || t2->type == MT_ORBINAUT_SHIELD
		|| t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BALLHOG)
	{
		// Other Item Damage
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
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

	if (damageitem)
	{
		// This Item Damage
		angle_t bounceangle = K_GetCollideAngle(t2, t1);

		S_StartSound(t1, t1->info->deathsound);
		P_KillMobj(t1, t2, t2, DMG_NORMAL);

		P_SetObjectMomZ(t1, 8*FRACUNIT, false);
		P_InstaThrust(t1, bounceangle, 16*FRACUNIT);
	}

	return true;
}

boolean K_EggItemCollide(mobj_t *t1, mobj_t *t2)
{
	if ((t1->threshold > 0 && t2->hitlag > 0) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

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

		if (!P_CanPickupItem(t2->player, 2))
			return true;

		if ((gametyperules & GTR_BUMPERS) && t2->player->bumpers <= 0)
		{
			return true;
		}
		else
		{
			K_DropItems(t2->player); //K_StripItems(t2->player);
			//K_StripOther(t2->player);
			t2->player->itemroulette = 1;
			t2->player->roulettetype = 2;
		}

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
			P_KillMobj(t1, t2, t2, DMG_NORMAL);
			return true;
		}
		else
		{
			mobj_t *poof = P_SpawnMobj(t1->x, t1->y, t1->z, MT_EXPLODE);
			S_StartSound(poof, t1->info->deathsound);

#if 0
			// Eggbox snipe!
			if (t1->type == MT_EGGMANITEM && t1->health > 1)
				S_StartSound(t2, sfx_bsnipe);
#endif

			if (t1->target && t1->target->player)
			{
				if ((gametyperules & GTR_CIRCUIT) || t1->target->player->bumpers > 0)
					t2->player->eggmanblame = t1->target->player-players;
				else
					t2->player->eggmanblame = t2->player-players;

				if (t1->target->hnext == t1)
				{
					P_SetTarget(&t1->target->hnext, NULL);
					t1->target->player->pflags &= ~PF_EGGMANOUT;
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

static inline boolean PIT_SSMineChecks(mobj_t *thing)
{
	if (thing == grenade) // Don't explode yourself! Endless loop!
		return true;

	if (thing->health <= 0)
		return true;

	if (!(thing->flags & MF_SHOOTABLE) || (thing->flags & MF_SCENERY))
		return true;

	if (thing->player && thing->player->spectator)
		return true;

	if (P_AproxDistance(P_AproxDistance(thing->x - grenade->x, thing->y - grenade->y), thing->z - grenade->z) > explodedist)
		return true; // Too far away

	if (P_CheckSight(grenade, thing) == false)
		return true; // Not in sight

	return false;
}

static inline BlockItReturn_t PIT_SSMineSearch(mobj_t *thing)
{
	if (grenade == NULL || P_MobjWasRemoved(grenade))
		return BMIT_ABORT; // There's the possibility these can chain react onto themselves after they've already died if there are enough all in one spot

	if (grenade->flags2 & MF2_DEBRIS) // don't explode twice
		return BMIT_ABORT;

	if (thing->type != MT_PLAYER) // Don't explode for anything but an actual player.
		return BMIT_CONTINUE;

	if (thing == grenade->target && grenade->threshold != 0) // Don't blow up at your owner instantly.
		return BMIT_CONTINUE;

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
	if (grenade == NULL || P_MobjWasRemoved(grenade))
		return BMIT_ABORT; // There's the possibility these can chain react onto themselves after they've already died if there are enough all in one spot

#if 0
	if (grenade->flags2 & MF2_DEBRIS) // don't explode twice
		return BMIT_ABORT;
#endif

	if (PIT_SSMineChecks(thing) == true)
		return BMIT_CONTINUE;

	P_DamageMobj(thing, grenade, grenade->target, 1, (explodespin ? DMG_NORMAL : DMG_EXPLODE));
	return BMIT_CONTINUE;
}

void K_MineExplodeAttack(mobj_t *actor, fixed_t size, boolean spin)
{
	INT32 bx, by, xl, xh, yl, yh;

	explodespin = spin;
	explodedist = FixedMul(size, actor->scale);
	grenade = actor;

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
}

boolean K_MineCollide(mobj_t *t1, mobj_t *t2)
{
	if ((t1->threshold > 0 && t2->hitlag > 0) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (t2->player)
	{
		if (t2->player->flashing > 0 && t2->hitlag == 0)
			return true;

		// Bomb punting
		if ((t1->state >= &states[S_SSMINE1] && t1->state <= &states[S_SSMINE4])
			|| (t1->state >= &states[S_SSMINE_DEPLOY8] && t1->state <= &states[S_SSMINE_DEPLOY13]))
		{
			P_KillMobj(t1, t2, t2, DMG_NORMAL);
		}
		else
		{
			K_PuntMine(t1, t2);
		}
	}
	else if (t2->type == MT_ORBINAUT || t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD)
	{
		// Bomb death
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		P_KillMobj(t1, t2, t2, DMG_NORMAL);

		// Other Item Damage
		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
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
	if ((t1->threshold > 0 && t2->hitlag > 0) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (t2->player)
	{
		if (t2->player->flashing)
			return true;

		// Banana snipe!
		if (t1->health > 1)
			S_StartSound(t2, sfx_bsnipe);

		if (t2->player->flamedash && t2->player->itemtype == KITEM_FLAMESHIELD)
		{
			// Melt item
			S_StartSound(t2, sfx_s3k43);
		}
		else
		{
			// Player Damage
			P_DamageMobj(t2, t1, t1->target, 1, DMG_TUMBLE);
		}

		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}
	else if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_ORBINAUT || t2->type == MT_ORBINAUT_SHIELD
		|| t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BALLHOG)
	{
		// Other Item Damage
		angle_t bounceangle = K_GetCollideAngle(t1, t2);

		if (t2->eflags & MFE_VERTICALFLIP)
			t2->z -= t2->height;
		else
			t2->z += t2->height;

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1, DMG_NORMAL);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
		P_InstaThrust(t2, bounceangle, 16*FRACUNIT);

		P_SpawnMobj(t2->x/2 + t1->x/2, t2->y/2 + t1->y/2, t2->z/2 + t1->z/2, MT_ITEMCLASH);

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
		P_KillMobj(t1, t2, t2, DMG_NORMAL);
	}

	return true;
}

boolean K_DropTargetCollide(mobj_t *t1, mobj_t *t2)
{
	mobj_t *draggeddroptarget = (t1->type == MT_DROPTARGET_SHIELD) ? t1->target : NULL;

	if ((t1->threshold > 0 && (t2->hitlag > 0 || !draggeddroptarget)) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

	if (((t1->target == t2) || (t1->target == t2->target)) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (t2->player && (t2->player->hyudorotimer || t2->player->justbumped))
		return true;

	// Intensify bumps if already spinning...
	P_Thrust(t1, R_PointToAngle2(t1->x, t1->y, t2->x, t2->y),
		(t1->reactiontime && !draggeddroptarget) ? 35*FRACUNIT : 20*FRACUNIT);

	if (draggeddroptarget)
	{
		// "Pass through" the shock of the impact, part 1.
		t1->momx = t1->target->momx;
		t1->momy = t1->target->momy;
		t1->momz = t1->target->momz;
	}

	{
		angle_t t2angle = R_PointToAngle2(t2->momx, t2->momy, 0, 0);
		angle_t t2deflect;
		fixed_t t1speed, t2speed;
		K_KartBouncing(t1, t2);
		t1speed = FixedHypot(t1->momx, t1->momy);
		t2speed = FixedHypot(t2->momx, t2->momy);

		t2deflect = t2angle - R_PointToAngle2(0, 0, t2->momx, t2->momy);
		if (t2deflect > ANGLE_180)
			t2deflect = InvAngle(t2deflect);
		if (t2deflect < ANG10)
			P_InstaThrust(t2, t2angle, t2speed);

		P_InitAngle(t1, R_PointToAngle2(0, 0, t1->momx, t1->momy));

		t1->reactiontime = 7*(t1speed+t2speed)/FRACUNIT;
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
			P_InitAngle(blast, blast->angle);

			blast->destscale *= 10;
		}
	}

	t1->flags |= MF_SHOOTABLE;
	// The following sets t1->target to t2, so draggeddroptarget keeps it persisting...
	P_DamageMobj(t1, t2, (t2->target ? t2->target : t2), 1, DMG_NORMAL);
	t1->color = (t1->health > 1)
		? SKINCOLOR_GOLD
		: SKINCOLOR_CRIMSON;
	t1->flags &= ~MF_SHOOTABLE;

	t1->spritexscale = 3*FRACUNIT;
	t1->spriteyscale = 3*FRACUNIT/2;

	if (!t2->player)
	{
		t2->angle += ANGLE_180;
		if (t2->type == MT_JAWZ)
			P_SetTarget(&t2->tracer, t2->target); // Back to the source!
		t2->threshold = 10;
	}

	if (t1->reactiontime > 1000) {
		S_StartSound(t2, sfx_kdtrg3);
	} else if (t1->reactiontime > 500) {
		S_StartSound(t2, sfx_kdtrg2);
	} else {
		S_StartSound(t2, sfx_kdtrg1);
	}

	if (draggeddroptarget && draggeddroptarget->player)
	{
		// The following removes t1, be warned
		// (its newly assigned properties are moved across)
		K_DropHnextList(draggeddroptarget->player, true);
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

	if (!(thing->flags & MF_SHOOTABLE) || (thing->flags & MF_SCENERY))
	{
		// Not shootable
		return BMIT_CONTINUE;
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

#if 0
	if (P_CheckSight(lightningSource, thing) == false)
	{
		// Not in sight
		return BMIT_CONTINUE;
	}
#endif

	P_DamageMobj(thing, lightningSource, lightningSource, 1, DMG_NORMAL|DMG_CANTHURTSELF|DMG_WOMBO);
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

boolean K_BubbleShieldCollide(mobj_t *t1, mobj_t *t2)
{
	if (t2->type == MT_PLAYER)
	{
		// Counter desyncs
		/*mobj_t *oldthing = thing;
		mobj_t *oldtmthing = tmthing;

		P_Thrust(tmthing, R_PointToAngle2(thing->x, thing->y, tmthing->x, tmthing->y), 4*thing->scale);

		thing = oldthing;
		P_SetTarget(&tmthing, oldtmthing);*/

		if (P_PlayerInPain(t2->player)
			|| t2->player->flashing || t2->player->hyudorotimer
			|| t2->player->justbumped || t2->scale > t1->scale + (mapobjectscale/8))
			return true;

		// Player Damage
		P_DamageMobj(t2, ((t1->type == MT_BUBBLESHIELD) ? t1->target : t1), t1, 1, DMG_NORMAL|DMG_WOMBO);
		S_StartSound(t1, sfx_s3k44);
	}
	else
	{
		if (!t2->threshold)
		{
			if (!t2->momx && !t2->momy)
			{
				t2->momz += (24*t2->scale) * P_MobjFlip(t2);
			}
			else
			{
				t2->momx = -4*t2->momx;
				t2->momy = -4*t2->momy;
				t2->momz = -4*t2->momz;
				t2->angle += ANGLE_180;
			}
			if (t2->type == MT_JAWZ)
				P_SetTarget(&t2->tracer, t2->target); // Back to the source!
			t2->threshold = 10;
			S_StartSound(t1, sfx_s3k44);
		}
	}

	// no interaction
	return true;
}

boolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2)
{
	if ((t1->threshold > 0 && t2->hitlag > 0) || (t2->threshold > 0 && t1->hitlag > 0))
		return true;

	if (((t1->target == t2) || (!(t2->flags & (MF_ENEMY|MF_BOSS)) && (t1->target == t2->target))) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t2->player)
	{
		if (t2->player->flashing > 0 && t2->hitlag == 0)
			return true;

		S_StartSound(NULL, sfx_bsnipe); // let all players hear it.

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

boolean K_SMKIceBlockCollide(mobj_t *t1, mobj_t *t2)
{
	if (!(t2->flags & MF_SOLID || t2->flags & MF_SHOOTABLE))
		return true;

	if (!(t2->health))
		return true;

	if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_EGGMANITEM || t2->type == MT_EGGMANITEM_SHIELD
		|| t2->type == MT_SSMINE || t2->type == MT_SSMINE_SHIELD
		|| t2->type == MT_DROPTARGET_SHIELD
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD)
		return false;

	if (t1->health)
		P_KillMobj(t1, t2, t2, DMG_NORMAL);

	/*
	if (t2->player && (t2->player->invincibilitytimer > 0
		|| t2->player->growshrinktimer > 0))
		return true;
	*/

	K_KartSolidBounce(t1, t2);
	return true;
}

boolean K_PvPTouchDamage(mobj_t *t1, mobj_t *t2)
{
	const boolean flameT1 = (t1->player->flamedash > 0 && t1->player->itemtype == KITEM_FLAMESHIELD);
	const boolean flameT2 = (t2->player->flamedash > 0 && t2->player->itemtype == KITEM_FLAMESHIELD);

	boolean t1Condition = false;
	boolean t2Condition = false;
	boolean stungT1 = false;
	boolean stungT2 = false;

	t1Condition = (t1->scale > t2->scale + (mapobjectscale/8)) || (t1->player->invincibilitytimer > 0);
	t2Condition = (t2->scale > t1->scale + (mapobjectscale/8)) || (t2->player->invincibilitytimer > 0);

	if ((t1Condition == true || flameT1 == true) && (t2Condition == true || flameT2 == true))
	{
		K_DoPowerClash(t1->player, t2->player);
		return false;
	}
	else if (t1Condition == true && t2Condition == false)
	{
		P_DamageMobj(t2, t1, t1, 1, DMG_TUMBLE);
		return true;
	}
	else if (t1Condition == false && t2Condition == true)
	{
		P_DamageMobj(t1, t2, t2, 1, DMG_TUMBLE);
		return true;
	}

	// Flame Shield dash damage
	t1Condition = flameT1;
	t2Condition = flameT2;

	if (t1Condition == true && t2Condition == false)
	{
		P_DamageMobj(t2, t1, t1, 1, DMG_WIPEOUT|DMG_WOMBO);
		return true;
	}
	else if (t1Condition == false && t2Condition == true)
	{
		P_DamageMobj(t1, t2, t2, 1, DMG_WIPEOUT|DMG_WOMBO);
		return true;
	}

	// Battle Mode Sneaker damage
	// (Pogo Spring damage is handled in head-stomping code)
	if (gametyperules & GTR_BUMPERS)
	{
		t1Condition = ((t1->player->sneakertimer > 0)
			&& !P_PlayerInPain(t1->player)
			&& (t1->player->flashing == 0));
		t2Condition = ((t2->player->sneakertimer > 0)
			&& !P_PlayerInPain(t2->player)
			&& (t2->player->flashing == 0));

		if (t1Condition == true && t2Condition == false)
		{
			P_DamageMobj(t2, t1, t1, 1, DMG_WIPEOUT|DMG_STEAL|DMG_WOMBO);
			return true;
		}
		else if (t1Condition == false && t2Condition == true)
		{
			P_DamageMobj(t1, t2, t2, 1, DMG_WIPEOUT|DMG_STEAL|DMG_WOMBO);
			return true;
		}
	}

	// Ring sting, this is a bit more unique
	t1Condition = (K_GetShieldFromItem(t2->player->itemtype) == KSHIELD_NONE);
	t2Condition = (K_GetShieldFromItem(t1->player->itemtype) == KSHIELD_NONE);

	if (t1Condition == true)
	{
		if (t2->player->rings <= 0)
		{
			P_DamageMobj(t2, t1, t1, 1, DMG_STING|DMG_WOMBO);
			stungT2 = true;
		}

		P_PlayerRingBurst(t2->player, 1);
	}

	if (t2Condition == true)
	{
		if (t1->player->rings <= 0)
		{
			P_DamageMobj(t1, t2, t2, 1, DMG_STING|DMG_WOMBO);
			stungT1 = true;
		}

		P_PlayerRingBurst(t1->player, 1);
	}

	// No damage hitlag for stinging.
	if (stungT1 == true && stungT2 == false)
	{
		t2->eflags &= ~MFE_DAMAGEHITLAG;
	}
	else if (stungT2 == true && stungT1 == false)
	{
		t1->eflags &= ~MFE_DAMAGEHITLAG;
	}

	return (stungT1 || stungT2);
}
