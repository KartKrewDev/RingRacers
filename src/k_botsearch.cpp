// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_botsearch.cpp
/// \brief Bot blockmap search functions

#include <algorithm>

#include <tracy/tracy/Tracy.hpp>

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
#include "k_objects.h"

/*--------------------------------------------------
	static BlockItReturn_t K_FindEggboxes(mobj_t *thing)

		Blockmap search function.
		Increments the random items and egg boxes counters.

	Input Arguments:-
		thing - Object passed in from iteration.

	Return:-
		BlockItReturn_t enum, see its definition for more information.
--------------------------------------------------*/
static struct eggboxSearch_s
{
	fixed_t distancetocheck;
	fixed_t eggboxx, eggboxy;
	UINT8 randomitems;
	UINT8 eggboxes;
} g_eggboxSearch;

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

	dist = P_AproxDistance(thing->x - g_eggboxSearch.eggboxx, thing->y - g_eggboxSearch.eggboxy);

	if (dist > g_eggboxSearch.distancetocheck)
	{
		return BMIT_CONTINUE;
	}

	if (thing->type == MT_RANDOMITEM)
	{
		g_eggboxSearch.randomitems++;
	}
	else
	{
		g_eggboxSearch.eggboxes++;
	}

	return BMIT_CONTINUE;
}

/*--------------------------------------------------
	UINT8 K_EggboxStealth(fixed_t x, fixed_t y)

		See header file for description.
--------------------------------------------------*/
UINT8 K_EggboxStealth(fixed_t x, fixed_t y)
{
	ZoneScoped;

	INT32 xl, xh, yl, yh, bx, by;

	g_eggboxSearch.eggboxx = x;
	g_eggboxSearch.eggboxy = y;
	g_eggboxSearch.distancetocheck = (mapobjectscale * 256);
	g_eggboxSearch.randomitems = 0;
	g_eggboxSearch.eggboxes = 0;

	xl = (unsigned)(g_eggboxSearch.eggboxx - g_eggboxSearch.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(g_eggboxSearch.eggboxx + g_eggboxSearch.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(g_eggboxSearch.eggboxy - g_eggboxSearch.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(g_eggboxSearch.eggboxy + g_eggboxSearch.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_FindEggboxes);
		}
	}

	return (g_eggboxSearch.randomitems * (g_eggboxSearch.eggboxes + 1));
}

/*--------------------------------------------------
	static boolean K_BotHatesThisSectorsSpecial(const player_t *player, sector_t *sec)

		Tells us if a bot will play more careful around
		this sector's special type.

	Input Arguments:-
		player - Player to check against.
		sec - Sector to check the specials of.

	Return:-
		true if avoiding this sector special, false otherwise.
--------------------------------------------------*/
static boolean K_BotHatesThisSectorsSpecial(const player_t *player, sector_t *sec, const boolean flip)
{
	terrain_t *terrain = K_GetTerrainForFlatNum(flip ? sec->ceilingpic : sec->floorpic);

	if (terrain != NULL)
	{
		if (terrain->damageType != SD_NONE)
		{
			return true;
		}

		if (terrain->offroad > 0)
		{
			return !K_BotCanTakeCut(player);
		}
	}
	else
	{
		if (sec->damagetype != SD_NONE)
		{
			return true;
		}

		if (sec->offroad > 0)
		{
			return !K_BotCanTakeCut(player);
		}
	}

	return false;
}

/*--------------------------------------------------
	boolean K_BotHatesThisSector(const player_t *player, sector_t *sec, fixed_t x, fixed_t y)

		See header file for description.
--------------------------------------------------*/
boolean K_BotHatesThisSector(const player_t *player, sector_t *sec, fixed_t x, fixed_t y)
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
				&& K_BotHatesThisSectorsSpecial(player, rover->master->frontsector, flip))
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

	return K_BotHatesThisSectorsSpecial(player, bestsector, flip);
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
static struct nudgeSearch_s
{
	mobj_t *botmo;
	angle_t angle;
	fixed_t distancetocheck;

	INT64 gotoAvgX[2], gotoAvgY[2];
	UINT32 gotoObjs[2];

	INT64 avoidAvgX[2], avoidAvgY[2];
	UINT32 avoidObjs[2];
} g_nudgeSearch;

static void K_AddAttackObject(mobj_t *thing, UINT8 side, UINT8 weight)
{
	fixed_t x, y;
	angle_t a, dir;
	UINT8 i;

	I_Assert(side <= 1);

	if (weight == 0)
	{
		return;
	}

	x = thing->x;
	y = thing->y;
	a = R_PointToAngle2(g_nudgeSearch.botmo->x, g_nudgeSearch.botmo->y, x, y);

	dir = a + (side ? -ANGLE_90 : ANGLE_90);
	x += FixedMul(thing->radius, FINECOSINE(dir >> ANGLETOFINESHIFT));
	y += FixedMul(thing->radius, FINESINE(dir >> ANGLETOFINESHIFT));

	x /= mapobjectscale;
	y /= mapobjectscale;

	for (i = 0; i < weight; i++)
	{
		g_nudgeSearch.gotoAvgX[side] += x;
		g_nudgeSearch.gotoAvgY[side] += y;
		g_nudgeSearch.gotoObjs[side]++;
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
	fixed_t x, y;
	angle_t a, dir;
	UINT8 i;

	I_Assert(side <= 1);

	if (weight == 0)
	{
		return;
	}

	x = thing->x;
	y = thing->y;
	a = R_PointToAngle2(g_nudgeSearch.botmo->x, g_nudgeSearch.botmo->y, x, y);

	dir = a + (side ? -ANGLE_90 : ANGLE_90);
	x += FixedMul(thing->radius, FINECOSINE(dir >> ANGLETOFINESHIFT));
	y += FixedMul(thing->radius, FINESINE(dir >> ANGLETOFINESHIFT));

	x /= mapobjectscale;
	y /= mapobjectscale;

	for (i = 0; i < weight; i++)
	{
		g_nudgeSearch.avoidAvgX[side] += x;
		g_nudgeSearch.avoidAvgY[side] += y;
		g_nudgeSearch.avoidObjs[side]++;
	}
}

/*--------------------------------------------------
	static boolean K_PlayerAttackSteer(mobj_t *thing, boolean friendly_fire, UINT8 side, UINT8 weight, boolean attackCond, boolean dodgeCond)

		Checks two conditions to determine if the object should be
		attacked or dodged.

	Input Arguments:-
		thing - Object to move towards/away from.
		friendly_fire - If the attack would be against a friendly player.
		side - Which side -- 0 for left, 1 for right
		weight - How important this object is.
		attackCond - If this is true, and dodgeCond isn't, then we go towards the object.
		dodgeCond - If this is true, and attackCond isn't, then we move away from the object.

	Return:-
		true if either condition is successful.
--------------------------------------------------*/
static boolean K_PlayerAttackSteer(mobj_t *thing, boolean friendly_fire, UINT8 side, UINT8 weight, boolean attackCond, boolean dodgeCond)
{
	if (friendly_fire == true && attackCond == true && dodgeCond == false)
	{
		// Dodge, don't attack.
		attackCond = false;
		dodgeCond = true;
	}

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
	ZoneScoped;

	INT16 angledelta, anglediff;
	angle_t destangle, angle;
	UINT8 side = 0;

	if (!g_nudgeSearch.botmo || P_MobjWasRemoved(g_nudgeSearch.botmo) || !g_nudgeSearch.botmo->player)
	{
		return BMIT_ABORT;
	}

	if (thing->health <= 0)
	{
		return BMIT_CONTINUE;
	}

	if (g_nudgeSearch.botmo == thing)
	{
		return BMIT_CONTINUE;
	}

	const fixed_t xDelta = abs(g_nudgeSearch.botmo->x - thing->x);
	const fixed_t yDelta = abs(g_nudgeSearch.botmo->y - thing->y);
	const fixed_t fullDist = (FixedMul(xDelta, xDelta) + FixedMul(yDelta, yDelta)) - FixedMul(thing->radius, thing->radius);

	if (fullDist > g_nudgeSearch.distancetocheck)
	{
		return BMIT_CONTINUE;
	}

#if 0
	// this is very expensive to do, and probably not worth it.
	if (P_CheckSight(g_nudgeSearch.botmo, thing) == false)
	{
		return BMIT_CONTINUE;
	}
#endif

	destangle = R_PointToAngle2(g_nudgeSearch.botmo->x, g_nudgeSearch.botmo->y, thing->x, thing->y);
	angle = (g_nudgeSearch.angle - destangle);

	if (angle < ANGLE_180)
	{
		angledelta = AngleFixed(angle)>>FRACBITS;
	}
	else
	{
		angledelta = 360-(AngleFixed(angle)>>FRACBITS);
		side = 1;
	}

	anglediff = abs(angledelta);

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
		case MT_SPECIALSTAGEBOMB:
			K_AddDodgeObject(thing, side, 20);
			break;
		case MT_SHRINK_GUN:
			if (thing->target == g_nudgeSearch.botmo)
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

			if (P_CanPickupItem(g_nudgeSearch.botmo->player, PICKUP_ITEMBOX))
			{
				K_AddAttackObject(thing, side, ((thing->extravalue1 < RINGBOX_TIME) ? 10 : 20));
			}
			break;
		case MT_EGGMANITEM:
			if (anglediff >= 45)
			{
				break;
			}

			if (P_CanPickupItem(g_nudgeSearch.botmo->player, PICKUP_ITEMBOX)) // Can pick up an actual item
			{
				const UINT8 stealth = K_EggboxStealth(thing->x, thing->y);
				const UINT8 requiredstealth = (g_nudgeSearch.botmo->player->botvars.difficulty * g_nudgeSearch.botmo->player->botvars.difficulty);

				if (stealth >= requiredstealth)
				{
					K_AddAttackObject(thing, side, 20);
				}
				else
				{
					K_AddDodgeObject(thing, side, 20);
				}
			}
			break;
		case MT_FLOATINGITEM:
			if (anglediff >= 45)
			{
				break;
			}

			if (P_CanPickupItem(g_nudgeSearch.botmo->player, PICKUP_PAPERITEM))
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

			if ((RINGTOTAL(g_nudgeSearch.botmo->player) < 20
				&& P_CanPickupItem(g_nudgeSearch.botmo->player, PICKUP_RINGORSPHERE))
				&& !thing->extravalue1
				&& (g_nudgeSearch.botmo->player->itemtype != KITEM_LIGHTNINGSHIELD))
			{
				K_AddAttackObject(thing, side, (RINGTOTAL(g_nudgeSearch.botmo->player) < 3) ? 5 : 1);
			}
			break;
		case MT_PLAYER:
			if (thing->player
				&& !thing->player->spectator
				&& !thing->player->hyudorotimer
				&& !g_nudgeSearch.botmo->player->hyudorotimer)
			{
				const boolean same_team = G_SameTeam(g_nudgeSearch.botmo->player, thing->player);

				// There REALLY ought to be a better way to handle this logic, right?!
				// Squishing
				if (K_PlayerAttackSteer(thing, same_team, side, 20,
					K_IsBigger(g_nudgeSearch.botmo, thing),
					K_IsBigger(thing, g_nudgeSearch.botmo)
				))
				{
					break;
				}
				// Invincibility
				else if (K_PlayerAttackSteer(thing, same_team, side, 20,
					g_nudgeSearch.botmo->player->invincibilitytimer,
					thing->player->invincibilitytimer
				))
				{
					break;
				}
				// Lightning Shield
				else if (K_PlayerAttackSteer(thing, same_team, side, 20,
					g_nudgeSearch.botmo->player->itemtype == KITEM_LIGHTNINGSHIELD,
					thing->player->itemtype == KITEM_LIGHTNINGSHIELD
				))
				{
					break;
				}
				// Bubble Shield
				else if (K_PlayerAttackSteer(thing, same_team, side, 20,
					g_nudgeSearch.botmo->player->itemtype == KITEM_BUBBLESHIELD,
					thing->player->itemtype == KITEM_BUBBLESHIELD
				))
				{
					break;
				}
				// Flame Shield
				else if (K_PlayerAttackSteer(thing, same_team, side, 20,
					g_nudgeSearch.botmo->player->itemtype == KITEM_FLAMESHIELD,
					thing->player->itemtype == KITEM_FLAMESHIELD
				))
				{
					break;
				}
				// Has held item shield
				else if (K_PlayerAttackSteer(thing, same_team, side, 20,
					(thing->player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT)),
					(g_nudgeSearch.botmo->player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT))
				))
				{
					break;
				}
				// Ring Sting
				else if (K_PlayerAttackSteer(thing, same_team, side, 20,
					thing->player->rings <= 0,
					g_nudgeSearch.botmo->player->rings <= 0
				))
				{
					break;
				}
				else
				{
					// After ALL of that, we can do standard bumping
					fixed_t ourweight = K_GetMobjWeight(g_nudgeSearch.botmo, thing);
					fixed_t theirweight = K_GetMobjWeight(thing, g_nudgeSearch.botmo);
					fixed_t weightdiff = 0;

					if (anglediff >= 90)
					{
						weightdiff = theirweight - ourweight;
					}
					else
					{
						weightdiff = ourweight - theirweight;
					}

					if (weightdiff > mapobjectscale && same_team == false)
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
		case MT_RINGSHOOTER:
			if (anglediff >= 45)
			{
				break;
			}
			else
			{
				K_AddAttackObject(thing, side, 50);
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
	void K_NudgePredictionTowardsObjects(botprediction_t *predict, const player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_NudgePredictionTowardsObjects(botprediction_t *predict, const player_t *player)
{
	ZoneScoped;

	const precise_t time = I_GetPreciseTime();

	INT32 xl, xh, yl, yh, bx, by;

	fixed_t distToPredict = 0;
	fixed_t radToPredict = 0;
	angle_t angleToPredict = 0;

	fixed_t avgX = 0, avgY = 0;
	fixed_t avgDist = 0;

	fixed_t baseNudge = 0;
	fixed_t maxNudge = 0;
	fixed_t nudgeDist = 0;
	angle_t nudgeDir = 0;

	SINT8 gotoSide = -1;
	UINT8 i;

	if (predict == NULL)
	{
		ps_bots[player - players].nudge += I_GetPreciseTime() - time;
		return;
	}

	distToPredict = R_PointToDist2(player->mo->x, player->mo->y, predict->x, predict->y);
	angleToPredict = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);

	radToPredict = distToPredict >> 1;
	g_nudgeSearch.distancetocheck = FixedMul(radToPredict, radToPredict);

	baseNudge = predict->baseRadius >> 3;
	maxNudge = predict->baseRadius - baseNudge;

	g_nudgeSearch.botmo = player->mo;
	g_nudgeSearch.angle = angleToPredict;

	// silly variable reuse
	avgX = g_nudgeSearch.botmo->x + FixedMul(radToPredict, FINECOSINE(angleToPredict >> ANGLETOFINESHIFT));
	avgY = g_nudgeSearch.botmo->y + FixedMul(radToPredict, FINESINE(angleToPredict >> ANGLETOFINESHIFT));

	for (i = 0; i < 2; i++)
	{
		g_nudgeSearch.gotoAvgX[i] = g_nudgeSearch.gotoAvgY[i] = 0;
		g_nudgeSearch.gotoObjs[i] = 0;

		g_nudgeSearch.avoidAvgX[i] = g_nudgeSearch.avoidAvgY[i] = 0;
		g_nudgeSearch.avoidObjs[i] = 0;
	}

	xl = (unsigned)(avgX - (radToPredict + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(avgX + (radToPredict + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(avgY - (radToPredict + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(avgY + (radToPredict + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_FindObjectsForNudging);
		}
	}

	// Handle dodge characters
	if (g_nudgeSearch.avoidObjs[1] > 0 || g_nudgeSearch.avoidObjs[0] > 0)
	{
		if (g_nudgeSearch.avoidObjs[1] > g_nudgeSearch.avoidObjs[0])
		{
			gotoSide = 1;
		}
		else
		{
			gotoSide = 0;
		}

		avgX = (g_nudgeSearch.avoidAvgX[gotoSide] / g_nudgeSearch.avoidObjs[gotoSide]) * mapobjectscale;
		avgY = (g_nudgeSearch.avoidAvgY[gotoSide] / g_nudgeSearch.avoidObjs[gotoSide]) * mapobjectscale;

		avgDist = R_PointToDist2(
			avgX, avgY,
			predict->x, predict->y
		);

		// High handling characters dodge better
		nudgeDist = ((9 - g_nudgeSearch.botmo->player->kartweight) + 1) * baseNudge;
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
		predict->radius = std::max(predict->radius - nudgeDist, baseNudge);

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

		if (g_nudgeSearch.gotoObjs[1] > g_nudgeSearch.gotoObjs[0])
		{
			gotoSide = 1;
		}
		else
		{
			gotoSide = 0;
		}
	}

	// Check if our side is invalid, if so, don't do the code below.
	if (gotoSide != -1 && g_nudgeSearch.gotoObjs[gotoSide] == 0)
	{
		// Do not use a side
		gotoSide = -1;
	}

	if (gotoSide != -1)
	{
		avgX = (g_nudgeSearch.gotoAvgX[gotoSide] / g_nudgeSearch.gotoObjs[gotoSide]) * mapobjectscale;
		avgY = (g_nudgeSearch.gotoAvgY[gotoSide] / g_nudgeSearch.gotoObjs[gotoSide]) * mapobjectscale;

		avgDist = R_PointToDist2(
			predict->x, predict->y,
			avgX, avgY
		);

		// Acceleration characters are more aggressive
		nudgeDist = ((9 - g_nudgeSearch.botmo->player->kartspeed) + 1) * baseNudge;
		if (nudgeDist > maxNudge)
		{
			nudgeDist = maxNudge;
		}

		if (avgDist <= nudgeDist)
		{
			predict->x = avgX;
			predict->y = avgY;
			predict->radius = baseNudge;
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
			predict->radius = std::max(predict->radius - nudgeDist, baseNudge);

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
static struct bullySearch_s
{
	mobj_t *botmo;
	fixed_t distancetocheck;

	fixed_t annoyscore;
	mobj_t *annoymo;
} g_bullySearch;

static BlockItReturn_t K_FindPlayersToBully(mobj_t *thing)
{
	INT16 anglediff;
	fixed_t fulldist;
	fixed_t ourweight, theirweight, weightdiff;
	angle_t ourangle, destangle, angle;

	if (!g_bullySearch.botmo || P_MobjWasRemoved(g_bullySearch.botmo) || !g_bullySearch.botmo->player)
	{
		return BMIT_ABORT;
	}

	if (thing->health <= 0)
	{
		return BMIT_CONTINUE;
	}

	if (!thing->player || thing->player->spectator)
	{
		return BMIT_CONTINUE;
	}

	if (g_bullySearch.botmo == thing)
	{
		return BMIT_CONTINUE;
	}

	fulldist = R_PointToDist2(g_bullySearch.botmo->x, g_bullySearch.botmo->y, thing->x, thing->y) - thing->radius;

	if (fulldist > g_bullySearch.distancetocheck)
	{
		return BMIT_CONTINUE;
	}

	if (P_CheckSight(g_bullySearch.botmo, thing) == false)
	{
		return BMIT_CONTINUE;
	}

	ourangle = g_bullySearch.botmo->angle;
	destangle = R_PointToAngle2(g_bullySearch.botmo->x, g_bullySearch.botmo->y, thing->x, thing->y);
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

	ourweight = K_GetMobjWeight(g_bullySearch.botmo, thing);
	theirweight = K_GetMobjWeight(thing, g_bullySearch.botmo);
	weightdiff = 0;

	if (anglediff >= 90)
	{
		weightdiff = theirweight - ourweight;
	}
	else
	{
		weightdiff = ourweight - theirweight;
	}

	if (weightdiff > mapobjectscale && weightdiff > g_bullySearch.annoyscore)
	{
		g_bullySearch.annoyscore = weightdiff;
		g_bullySearch.annoymo = thing;
	}

	return BMIT_CONTINUE;
}

/*--------------------------------------------------
	INT32 K_PositionBully(const player_t *player)

		See header file for description.
--------------------------------------------------*/
INT32 K_PositionBully(const player_t *player)
{
	ZoneScoped;

	INT32 xl, xh, yl, yh, bx, by;

	angle_t ourangle, destangle, angle;
	INT16 anglediff;

	g_bullySearch.botmo = player->mo;
	g_bullySearch.distancetocheck = 1024*player->mo->scale;

	g_bullySearch.annoymo = NULL;
	g_bullySearch.annoyscore = 0;

	xl = (unsigned)(g_bullySearch.botmo->x - g_bullySearch.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(g_bullySearch.botmo->x + g_bullySearch.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(g_bullySearch.botmo->y - g_bullySearch.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(g_bullySearch.botmo->y + g_bullySearch.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_FindPlayersToBully);
		}
	}

	if (g_bullySearch.annoymo == NULL)
	{
		return INT32_MAX;
	}

	ourangle = g_bullySearch.botmo->angle;
	destangle = R_PointToAngle2(g_bullySearch.botmo->x, g_bullySearch.botmo->y, g_bullySearch.annoymo->x, g_bullySearch.annoymo->y);
	angle = (ourangle - destangle);

	if (angle < ANGLE_180)
	{
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else
	{
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
	}

	if (abs(anglediff) < 30)
		return 0;

	if (anglediff < 0)
		return -KART_FULLTURN;

	return KART_FULLTURN;
}
