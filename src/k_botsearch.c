// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_botsearch.c
/// \brief Bot blockmap search functions

#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "r_main.h"
#include "p_local.h"
#include "k_bot.h"
#include "lua_hook.h"
#include "byteptr.h"
#include "d_net.h" // nodetoplayer
#include "k_kart.h"
#include "z_zone.h"
#include "i_system.h"
#include "p_maputl.h"
#include "d_ticcmd.h"
#include "m_random.h"
#include "r_things.h" // numskins
#include "p_slopes.h" // P_GetZAt
#include "m_perfstats.h"

struct globalsmuggle
{
	mobj_t *botmo;
	botprediction_t *predict;
	fixed_t distancetocheck;

	INT64 gotoAvgX[2], gotoAvgY[2];
	UINT32 gotoObjs[2];

	INT64 avoidAvgX[2], avoidAvgY[2];
	UINT32 avoidObjs[2];

	fixed_t annoyscore;
	mobj_t *annoymo;

	fixed_t closestlinedist;

	fixed_t eggboxx, eggboxy;
	UINT8 randomitems;
	UINT8 eggboxes;
} globalsmuggle;

/*--------------------------------------------------
	static BlockItReturn_t K_FindEggboxes(mobj_t *thing)

		Blockmap search function.
		Increments the random items and egg boxes counters.

	Input Arguments:-
		thing - Object passed in from iteration.

	Return:-
		BlockItReturn_t enum, see its definition for more information.
--------------------------------------------------*/
static BlockItReturn_t K_FindEggboxes(mobj_t *thing)
{
	fixed_t dist;

	if (thing->type != MT_RANDOMITEM && thing->type != MT_EGGMANITEM)
	{
		return BMIT_CONTINUE;
	}

	if (!thing->health)
	{
		return BMIT_CONTINUE;
	}

	dist = P_AproxDistance(thing->x - globalsmuggle.eggboxx, thing->y - globalsmuggle.eggboxy);

	if (dist > globalsmuggle.distancetocheck)
	{
		return BMIT_CONTINUE;
	}

	if (thing->type == MT_RANDOMITEM)
	{
		globalsmuggle.randomitems++;
	}
	else
	{
		globalsmuggle.eggboxes++;
	}

	return BMIT_CONTINUE;
}

/*--------------------------------------------------
	UINT8 K_EggboxStealth(fixed_t x, fixed_t y)

		See header file for description.
--------------------------------------------------*/
UINT8 K_EggboxStealth(fixed_t x, fixed_t y)
{
	INT32 xl, xh, yl, yh, bx, by;

	globalsmuggle.eggboxx = x;
	globalsmuggle.eggboxy = y;
	globalsmuggle.distancetocheck = (mapobjectscale * 256);
	globalsmuggle.randomitems = 0;
	globalsmuggle.eggboxes = 0;

	xl = (unsigned)(globalsmuggle.eggboxx - globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(globalsmuggle.eggboxx + globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(globalsmuggle.eggboxy - globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(globalsmuggle.eggboxy + globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_FindEggboxes);
		}
	}

	return (globalsmuggle.randomitems * (globalsmuggle.eggboxes + 1));
}

/*--------------------------------------------------
	static boolean K_BotHatesThisSectorsSpecial(player_t *player, sector_t *sec)

		Tells us if a bot will play more careful around
		this sector's special type.

	Input Arguments:-
		player - Player to check against.
		sec - Sector to check the specials of.

	Return:-
		true if avoiding this sector special, false otherwise.
--------------------------------------------------*/
static boolean K_BotHatesThisSectorsSpecial(player_t *player, sector_t *sec)
{
	if (sec->damagetype != SD_NONE)
	{
		return true;
	}

	if (sec->offroad > 0)
	{
		return !K_BotCanTakeCut(player);
	}

	return false;
}

/*--------------------------------------------------
	boolean K_BotHatesThisSector(player_t *player, sector_t *sec, fixed_t x, fixed_t y)

		See header file for description.
--------------------------------------------------*/
boolean K_BotHatesThisSector(player_t *player, sector_t *sec, fixed_t x, fixed_t y)
{
	const boolean flip = (player->mo->eflags & MFE_VERTICALFLIP);
	fixed_t highestfloor = INT32_MAX;
	sector_t *bestsector = NULL;
	ffloor_t *rover;

	// TODO: Properly support MSF_FLIPSPECIAL_FLOOR / MSF_FLIPSPECIAL_CEILING.
	// An earlier attempt at it caused lots of false positives and other weird
	// quirks with intangible FOFs.

	if (flip == true)
	{
		highestfloor = P_GetZAt(sec->c_slope, x, y, sec->ceilingheight);
	}
	else
	{
		highestfloor = P_GetZAt(sec->f_slope, x, y, sec->floorheight);
	}

	bestsector = sec;

	for (rover = sec->ffloors; rover; rover = rover->next)
	{
		fixed_t top = INT32_MAX;
		fixed_t bottom = INT32_MAX;

		if (!(rover->fofflags & FOF_EXISTS))
		{
			continue;
		}

		top = P_GetZAt(*rover->t_slope, x, y, *rover->topheight);
		bottom = P_GetZAt(*rover->b_slope, x, y, *rover->bottomheight);

		if (!(rover->fofflags & FOF_BLOCKPLAYER))
		{
			if ((top >= player->mo->z) && (bottom <= player->mo->z + player->mo->height)
				&& K_BotHatesThisSectorsSpecial(player, rover->master->frontsector))
			{
				// Bad intangible sector at our height, so we DEFINITELY want to avoid
				return true;
			}

			// Ignore them, we want the one below it.
			continue;
		}

		// Find the highest FOF floor beneath the player, and check it at the end.
		if (flip == true)
		{
			if (bottom < highestfloor
				&& bottom >= player->mo->z + player->mo->height)
			{
				bestsector = rover->master->frontsector;
				highestfloor = bottom;
			}
		}
		else
		{
			if (top > highestfloor
				&& top <= player->mo->z)
			{
				bestsector = rover->master->frontsector;
				highestfloor = top;
			}
		}
	}

	if (bestsector == NULL)
	{
		return false;
	}

	return K_BotHatesThisSectorsSpecial(player, bestsector);
}

/*--------------------------------------------------
	static void K_AddAttackObject(mobj_t *thing, UINT8 side, UINT8 weight)

		Adds an object to the list that the bot wants to go towards.

	Input Arguments:-
		thing - Object to move towards.
		side - Which side -- 0 for left, 1 for right
		weight - How important this object is.

	Return:-
		None
--------------------------------------------------*/
static void K_AddAttackObject(mobj_t *thing, UINT8 side, UINT8 weight)
{
	UINT8 i;

	I_Assert(side <= 1);

	if (weight == 0)
	{
		return;
	}

	for (i = 0; i < weight; i++)
	{
		globalsmuggle.gotoAvgX[side] += thing->x / mapobjectscale;
		globalsmuggle.gotoAvgY[side] += thing->y / mapobjectscale;
		globalsmuggle.gotoObjs[side]++;
	}
}

/*--------------------------------------------------
	static void K_AddDodgeObject(mobj_t *thing, UINT8 side, UINT8 weight)

		Adds an object to the list that the bot wants to dodge.

	Input Arguments:-
		thing - Object to move away from.
		side - Which side -- 0 for left, 1 for right
		weight - How important this object is.

	Return:-
		None
--------------------------------------------------*/
static void K_AddDodgeObject(mobj_t *thing, UINT8 side, UINT8 weight)
{
	UINT8 i;

	I_Assert(side <= 1);

	if (weight == 0)
	{
		return;
	}

	for (i = 0; i < weight; i++)
	{
		globalsmuggle.gotoAvgX[side] += thing->x / mapobjectscale;
		globalsmuggle.gotoAvgY[side] += thing->y / mapobjectscale;
		globalsmuggle.gotoObjs[side]++;
	}
}

/*--------------------------------------------------
	static boolean K_PlayerAttackSteer(mobj_t *thing, UINT8 side, UINT8 weight, boolean attackCond, boolean dodgeCond)

		Checks two conditions to determine if the object should be
		attacked or dodged.

	Input Arguments:-
		thing - Object to move towards/away from.
		side - Which side -- 0 for left, 1 for right
		weight - How important this object is.
		attackCond - If this is true, and dodgeCond isn't, then we go towards the object.
		dodgeCond - If this is true, and attackCond isn't, then we move away from the object.

	Return:-
		true if either condition is successful.
--------------------------------------------------*/
static boolean K_PlayerAttackSteer(mobj_t *thing, UINT8 side, UINT8 weight, boolean attackCond, boolean dodgeCond)
{
	if (attackCond == true && dodgeCond == false)
	{
		K_AddAttackObject(thing, side, weight);
		return true;
	}
	else if (dodgeCond == true && attackCond == false)
	{
		K_AddDodgeObject(thing, side, weight);
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static BlockItReturn_t K_FindObjectsForNudging(mobj_t *thing)

		Blockmap search function.
		Finds objects around the bot to steer towards/away from.

	Input Arguments:-
		thing - Object passed in from iteration.

	Return:-
		BlockItReturn_t enum, see its definition for more information.
--------------------------------------------------*/
static BlockItReturn_t K_FindObjectsForNudging(mobj_t *thing)
{
	INT16 anglediff;
	fixed_t fulldist;
	angle_t destangle, angle, predictangle;
	UINT8 side = 0;

	if (!globalsmuggle.botmo || P_MobjWasRemoved(globalsmuggle.botmo) || !globalsmuggle.botmo->player)
	{
		return BMIT_ABORT;
	}

	if (thing->health <= 0)
	{
		return BMIT_CONTINUE;
	}

	if (globalsmuggle.botmo == thing)
	{
		return BMIT_CONTINUE;
	}

	fulldist = R_PointToDist2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, thing->x, thing->y) - thing->radius;

	if (fulldist > globalsmuggle.distancetocheck)
	{
		return BMIT_CONTINUE;
	}

	if (P_CheckSight(globalsmuggle.botmo, thing) == false)
	{
		return BMIT_CONTINUE;
	}

	predictangle = R_PointToAngle2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, globalsmuggle.predict->x, globalsmuggle.predict->y);
	destangle = R_PointToAngle2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, thing->x, thing->y);
	angle = (predictangle - destangle);

	if (angle < ANGLE_180)
	{
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else 
	{
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
		side = 1;
	}

	anglediff = abs(anglediff);

	switch (thing->type)
	{
		case MT_BANANA:
		case MT_BANANA_SHIELD:
		case MT_EGGMANITEM_SHIELD:
		case MT_ORBINAUT:
		case MT_ORBINAUT_SHIELD:
		case MT_JAWZ:
		case MT_JAWZ_SHIELD:
		case MT_SSMINE:
		case MT_SSMINE_SHIELD:
		case MT_LANDMINE:
		case MT_DROPTARGET:
		case MT_DROPTARGET_SHIELD:
		case MT_BALLHOG:
		case MT_SPB:
		case MT_BUBBLESHIELDTRAP:
		case MT_DUELBOMB:
		case MT_GACHABOM:
			K_AddDodgeObject(thing, side, 20);
			break;
		case MT_SHRINK_GUN:
			if (thing->target == globalsmuggle.botmo)
			{
				K_AddAttackObject(thing, side, 20);
			}
			else
			{
				K_AddDodgeObject(thing, side, 20);
			}
			break;
		case MT_RANDOMITEM:
			if (anglediff >= 45)
			{
				break;
			}

			if (P_CanPickupItem(globalsmuggle.botmo->player, 1))
			{
				K_AddAttackObject(thing, side, 10);
			}
			break;
		case MT_EGGMANITEM:
			if (anglediff >= 45)
			{
				break;
			}

			if (P_CanPickupItem(globalsmuggle.botmo->player, 1)) // Can pick up an actual item
			{
				const UINT8 stealth = K_EggboxStealth(thing->x, thing->y);
				const UINT8 requiredstealth = (globalsmuggle.botmo->player->botvars.difficulty * globalsmuggle.botmo->player->botvars.difficulty);

				if (stealth >= requiredstealth)
				{
					K_AddAttackObject(thing, side, 10);
				}
				else
				{
					K_AddDodgeObject(thing, side, 10);
				}
			}
			break;
		case MT_FLOATINGITEM:
			if (anglediff >= 45)
			{
				break;
			}

			if (P_CanPickupItem(globalsmuggle.botmo->player, 3))
			{
				K_AddAttackObject(thing, side, 20);
			}
			break;
		case MT_RING:
		case MT_FLINGRING:
			if (anglediff >= 45)
			{
				break;
			}

			if ((RINGTOTAL(globalsmuggle.botmo->player) < 20 && !(globalsmuggle.botmo->player->pflags & PF_RINGLOCK)
				&& P_CanPickupItem(globalsmuggle.botmo->player, 0))
				&& !thing->extravalue1
				&& (globalsmuggle.botmo->player->itemtype != KITEM_LIGHTNINGSHIELD))
			{
				K_AddAttackObject(thing, side, (RINGTOTAL(globalsmuggle.botmo->player) < 3) ? 5 : 1);
			}
			break;
		case MT_PLAYER:
			if (thing->player
				&& !thing->player->hyudorotimer
				&& !globalsmuggle.botmo->player->hyudorotimer)
			{
				// There REALLY ought to be a better way to handle this logic, right?!
				// Squishing
				if (K_PlayerAttackSteer(thing, side, 20,
					K_IsBigger(globalsmuggle.botmo, thing),
					K_IsBigger(thing, globalsmuggle.botmo)
				))
				{
					break;
				}
				// Invincibility
				else if (K_PlayerAttackSteer(thing, side, 20,
					globalsmuggle.botmo->player->invincibilitytimer,
					thing->player->invincibilitytimer
				))
				{
					break;
				}
				// Lightning Shield
				else if (K_PlayerAttackSteer(thing, side, 20,
					globalsmuggle.botmo->player->itemtype == KITEM_LIGHTNINGSHIELD,
					thing->player->itemtype == KITEM_LIGHTNINGSHIELD
				))
				{
					break;
				}
				// Bubble Shield
				else if (K_PlayerAttackSteer(thing, side, 20,
					globalsmuggle.botmo->player->itemtype == KITEM_BUBBLESHIELD,
					thing->player->itemtype == KITEM_BUBBLESHIELD
				))
				{
					break;
				}
				// Flame Shield
				else if (K_PlayerAttackSteer(thing, side, 20,
					globalsmuggle.botmo->player->itemtype == KITEM_FLAMESHIELD,
					thing->player->itemtype == KITEM_FLAMESHIELD
				))
				{
					break;
				}
				// Has held item shield
				else if (K_PlayerAttackSteer(thing, side, 20,
					(thing->player->pflags & (PF_ITEMOUT|PF_EGGMANOUT)),
					(globalsmuggle.botmo->player->pflags & (PF_ITEMOUT|PF_EGGMANOUT))
				))
				{
					break;
				}
				// Ring Sting
				else if (K_PlayerAttackSteer(thing, side, 20,
					thing->player->rings <= 0,
					globalsmuggle.botmo->player->rings <= 0
				))
				{
					break;
				}
				else
				{
					// After ALL of that, we can do standard bumping
					fixed_t ourweight = K_GetMobjWeight(globalsmuggle.botmo, thing);
					fixed_t theirweight = K_GetMobjWeight(thing, globalsmuggle.botmo);
					fixed_t weightdiff = 0;

					if (anglediff >= 90)
					{
						weightdiff = theirweight - ourweight;
					}
					else
					{
						weightdiff = ourweight - theirweight;
					}

					if (weightdiff > mapobjectscale)
					{
						K_AddAttackObject(thing, side, 20);
					}
					else
					{
						K_AddDodgeObject(thing, side, 20);
					}
				}
			}
			break;
		case MT_BOTHINT:
			if (anglediff >= 45)
			{
				break;
			}
			else
			{
				UINT8 weight = 20;

				if (thing->extravalue2 > 0)
				{
					weight = thing->extravalue2 * 5;
				}

				if (thing->extravalue1 == 0)
				{
					K_AddDodgeObject(thing, side, weight);
				}
				else
				{
					K_AddAttackObject(thing, side, weight);
				}
			}
			break;
		default:
			if (thing->flags & (MF_SOLID|MF_ENEMY|MF_BOSS|MF_PAIN|MF_MISSILE))
			{
				K_AddDodgeObject(thing, side, 20);
			}
			break;
	}

	return BMIT_CONTINUE;
}

/*--------------------------------------------------
	void K_NudgePredictionTowardsObjects(botprediction_t *predict, player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_NudgePredictionTowardsObjects(botprediction_t *predict, player_t *player)
{
	const precise_t time = I_GetPreciseTime();

	INT32 xl, xh, yl, yh, bx, by;

	fixed_t distToPredict = R_PointToDist2(player->mo->x, player->mo->y, predict->x, predict->y);

	fixed_t avgX = 0, avgY = 0;
	fixed_t avgDist = 0;

	const fixed_t baseNudge = predict->radius;
	fixed_t maxNudge = distToPredict;
	fixed_t nudgeDist = 0;
	angle_t nudgeDir = 0;

	SINT8 gotoSide = -1;
	UINT8 i;

	globalsmuggle.botmo = player->mo;
	globalsmuggle.predict = predict;

	globalsmuggle.distancetocheck = distToPredict;

	for (i = 0; i < 2; i++)
	{
		globalsmuggle.gotoAvgX[i] = globalsmuggle.gotoAvgY[i] = 0;
		globalsmuggle.gotoObjs[i] = 0;

		globalsmuggle.avoidAvgX[i] = globalsmuggle.avoidAvgY[i] = 0;
		globalsmuggle.avoidObjs[i] = 0;
	}

	xl = (unsigned)(globalsmuggle.botmo->x - globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(globalsmuggle.botmo->x + globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(globalsmuggle.botmo->y - globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(globalsmuggle.botmo->y + globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_FindObjectsForNudging);
		}
	}

	// Handle dodge characters
	if (globalsmuggle.avoidObjs[1] > 0 || globalsmuggle.avoidObjs[0] > 0)
	{
		if (globalsmuggle.avoidObjs[1] > globalsmuggle.avoidObjs[0])
		{
			gotoSide = 1;
		}
		else
		{
			gotoSide = 0;
		}

		avgX = (globalsmuggle.avoidAvgX[gotoSide] / globalsmuggle.avoidObjs[gotoSide]) * mapobjectscale;
		avgY = (globalsmuggle.avoidAvgY[gotoSide] / globalsmuggle.avoidObjs[gotoSide]) * mapobjectscale;

		avgDist = R_PointToDist2(
			avgX, avgY,
			predict->x, predict->y
		);

		// High handling characters dodge better
		nudgeDist = ((9 - globalsmuggle.botmo->player->kartweight) + 1) * baseNudge;

		maxNudge = max(distToPredict - predict->radius, predict->radius);
		if (nudgeDist > maxNudge)
		{
			nudgeDist = maxNudge;
		}

		// Point away
		nudgeDir = R_PointToAngle2(
			avgX, avgY,
			predict->x, predict->y
		);

		predict->x += FixedMul(nudgeDist, FINECOSINE(nudgeDir >> ANGLETOFINESHIFT));
		predict->y += FixedMul(nudgeDist, FINESINE(nudgeDir >> ANGLETOFINESHIFT));

		distToPredict = R_PointToDist2(player->mo->x, player->mo->y, predict->x, predict->y);

		// Flip side, since we want to check for objects to steer towards on the side we're NOT dodging.
		if (gotoSide == 0)
		{
			gotoSide = 1;
		}
		else
		{
			gotoSide = 0;
		}
	}

	if (gotoSide == -1)
	{
		// Pick a side here if there were no objects to dodge.
		// We don't want to pick contradictory sides, so keep the old side otherwise,
		// even if there's more to grab on the other side.

		if (globalsmuggle.gotoObjs[1] > globalsmuggle.gotoObjs[0])
		{
			gotoSide = 1;
		}
		else
		{
			gotoSide = 0;
		}
	}

	// Check if our side is invalid, if so, don't do the code below.
	if (gotoSide != -1 && globalsmuggle.gotoObjs[gotoSide] == 0)
	{
		// Do not use a side
		gotoSide = -1;
	}

	if (gotoSide != -1)
	{
		avgX = (globalsmuggle.gotoAvgX[gotoSide] / globalsmuggle.gotoObjs[gotoSide]) * mapobjectscale;
		avgY = (globalsmuggle.gotoAvgY[gotoSide] / globalsmuggle.gotoObjs[gotoSide]) * mapobjectscale;

		avgDist = R_PointToDist2(
			predict->x, predict->y,
			avgX, avgY
		);

		// Acceleration characters are more aggressive
		nudgeDist = ((9 - globalsmuggle.botmo->player->kartspeed) + 1) * baseNudge;

		maxNudge = max(distToPredict - predict->radius, predict->radius);
		if (nudgeDist > maxNudge)
		{
			nudgeDist = maxNudge;
		}

		if (avgDist <= nudgeDist)
		{
			predict->x = avgX;
			predict->y = avgY;
		}
		else
		{
			// Point towards
			nudgeDir = R_PointToAngle2(
				predict->x, predict->y,
				avgX, avgY
			);

			predict->x += FixedMul(nudgeDist, FINECOSINE(nudgeDir >> ANGLETOFINESHIFT));
			predict->y += FixedMul(nudgeDist, FINESINE(nudgeDir >> ANGLETOFINESHIFT));

			//distToPredict = R_PointToDist2(player->mo->x, player->mo->y, predict->x, predict->y);
		}
	}

	ps_bots[player - players].nudge += I_GetPreciseTime() - time;
}

/*--------------------------------------------------
	static BlockItReturn_t K_FindPlayersToBully(mobj_t *thing)

		Blockmap search function.
		Finds players around the bot to bump.

	Input Arguments:-
		thing - Object passed in from iteration.

	Return:-
		BlockItReturn_t enum, see its definition for more information.
--------------------------------------------------*/
static BlockItReturn_t K_FindPlayersToBully(mobj_t *thing)
{
	INT16 anglediff;
	fixed_t fulldist;
	fixed_t ourweight, theirweight, weightdiff;
	angle_t ourangle, destangle, angle;

	if (!globalsmuggle.botmo || P_MobjWasRemoved(globalsmuggle.botmo) || !globalsmuggle.botmo->player)
	{
		return BMIT_ABORT;
	}

	if (thing->health <= 0)
	{
		return BMIT_CONTINUE;
	}

	if (!thing->player)
	{
		return BMIT_CONTINUE;
	}

	if (globalsmuggle.botmo == thing)
	{
		return BMIT_CONTINUE;
	}

	fulldist = R_PointToDist2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, thing->x, thing->y) - thing->radius;

	if (fulldist > globalsmuggle.distancetocheck)
	{
		return BMIT_CONTINUE;
	}

	if (P_CheckSight(globalsmuggle.botmo, thing) == false)
	{
		return BMIT_CONTINUE;
	}

	ourangle = globalsmuggle.botmo->angle;
	destangle = R_PointToAngle2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, thing->x, thing->y);
	angle = (ourangle - destangle);

	if (angle < ANGLE_180)
	{
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else 
	{
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
	}

	anglediff = abs(anglediff);

	ourweight = K_GetMobjWeight(globalsmuggle.botmo, thing);
	theirweight = K_GetMobjWeight(thing, globalsmuggle.botmo);
	weightdiff = 0;

	if (anglediff >= 90)
	{
		weightdiff = theirweight - ourweight;
	}
	else
	{
		weightdiff = ourweight - theirweight;
	}

	if (weightdiff > mapobjectscale && weightdiff > globalsmuggle.annoyscore)
	{
		globalsmuggle.annoyscore = weightdiff;
		globalsmuggle.annoymo = thing;
	}

	return BMIT_CONTINUE;
}

/*--------------------------------------------------
	INT32 K_PositionBully(player_t *player)

		See header file for description.
--------------------------------------------------*/
INT32 K_PositionBully(player_t *player)
{
	INT32 xl, xh, yl, yh, bx, by;

	angle_t ourangle, destangle, angle;
	INT16 anglediff;

	globalsmuggle.botmo = player->mo;
	globalsmuggle.distancetocheck = 1024*player->mo->scale;

	globalsmuggle.annoymo = NULL;
	globalsmuggle.annoyscore = 0;

	xl = (unsigned)(globalsmuggle.botmo->x - globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(globalsmuggle.botmo->x + globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(globalsmuggle.botmo->y - globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(globalsmuggle.botmo->y + globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_FindPlayersToBully);
		}
	}

	if (globalsmuggle.annoymo == NULL)
	{
		return INT32_MAX;
	}

	ourangle = globalsmuggle.botmo->angle;
	destangle = R_PointToAngle2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, globalsmuggle.annoymo->x, globalsmuggle.annoymo->y);
	angle = (ourangle - destangle);

	if (angle < ANGLE_180)
	{
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else 
	{
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
	}

	if (anglediff < 30)
		return 0;

	if (anglediff < 0)
		return -KART_FULLTURN;

	return KART_FULLTURN;
}
