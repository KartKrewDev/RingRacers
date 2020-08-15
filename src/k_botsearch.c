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

struct globalsmuggle
{
	mobj_t *botmo;
	fixed_t distancetocheck;

	fixed_t closestlinedist;

	INT16 curturn;
	INT16 steer;

	fixed_t eggboxx, eggboxy;
	UINT8 randomitems;
	UINT8 eggboxes;
} globalsmuggle;

/*--------------------------------------------------
	static boolean K_FindEggboxes(mobj_t *thing)

		Blockmap search function.
		Increments the random items and egg boxes counters.

	Input Arguments:-
		thing - Object passed in from iteration.

	Return:-
		true continues searching, false ends the search early.
--------------------------------------------------*/
static boolean K_FindEggboxes(mobj_t *thing)
{
	fixed_t dist;

	if (thing->type != MT_RANDOMITEM && thing->type != MT_EGGMANITEM)
	{
		return true;
	}

	if (!thing->health)
	{
		return true;
	}

	dist = P_AproxDistance(thing->x - globalsmuggle.eggboxx, thing->y - globalsmuggle.eggboxy);

	if (dist > globalsmuggle.distancetocheck)
	{
		return true;
	}

	if (thing->type == MT_RANDOMITEM)
	{
		globalsmuggle.randomitems++;
	}
	else
	{
		globalsmuggle.eggboxes++;
	}

	return true;
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

	return (globalsmuggle.randomitems * globalsmuggle.eggboxes);
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
	switch (GETSECSPECIAL(sec->special, 1))
	{
		case 1: // Damage
		case 5: // Spikes
		case 6: case 7: // Death Pit
		case 8: // Instant Kill
			return true;
		//case 2: case 3: // Offroad (let's let them lawnmower)
		case 4: // Offroad (Strong)
			if (!K_BotCanTakeCut(player))
			{
				return true;
			}
			break;
		default:
			break;
	}

	return false;
}

/*--------------------------------------------------
	static boolean K_BotHatesThisSector(player_t *player, sector_t *sec, fixed_t x, fixed_t y)

		Tells us if a bot will play more careful around
		this sector. Checks FOFs in the sector, as well.

	Input Arguments:-
		player - Player to check against.
		sec - Sector to check against.
		x - Linedef cross X position, for slopes
		y - Linedef cross Y position, for slopes

	Return:-
		true if avoiding this sector, false otherwise.
--------------------------------------------------*/
static boolean K_BotHatesThisSector(player_t *player, sector_t *sec, fixed_t x, fixed_t y)
{
	const boolean flip = (player->mo->eflags & MFE_VERTICALFLIP);
	INT32 specialflag = 0;
	fixed_t highestfloor = INT32_MAX;
	sector_t *bestsector = NULL;
	ffloor_t *rover;

	if (flip == true)
	{
		specialflag = SF_FLIPSPECIAL_CEILING;
		highestfloor = P_GetZAt(sec->c_slope, x, y, sec->ceilingheight);
	}
	else
	{
		specialflag = SF_FLIPSPECIAL_FLOOR;
		highestfloor = P_GetZAt(sec->f_slope, x, y, sec->floorheight);
	}

	if (sec->flags & specialflag)
	{
		bestsector = sec;
	}

	for (rover = sec->ffloors; rover; rover = rover->next)
	{
		fixed_t top = INT32_MAX;
		fixed_t bottom = INT32_MAX;

		if (!(rover->flags & FF_EXISTS))
		{
			continue;
		}

		top = P_GetZAt(*rover->t_slope, x, y, *rover->topheight);
		bottom = P_GetZAt(*rover->b_slope, x, y, *rover->bottomheight);

		if (!(rover->flags & FF_BLOCKPLAYER))
		{
			if ((top >= player->mo->z) && (bottom <= player->mo->z + player->mo->height)
			&& K_BotHatesThisSectorsSpecial(player, rover->master->frontsector))
			{
				// Bad intangible sector at our height, so we DEFINITELY want to avoid
				return true;
			}
		}

		if ((rover->flags & FF_BLOCKPLAYER) && !(rover->master->frontsector->flags & specialflag))
		{
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
	static boolean K_FindBlockingWalls(line_t *line)

		Blockmap search function.
		Reels the bot prediction back in based on solid walls
		or other obstacles surrounding the bot.

	Input Arguments:-
		line - Linedef passed in from iteration.

	Return:-
		true continues searching, false ends the search early.
--------------------------------------------------*/
static boolean K_FindBlockingWalls(line_t *line)
{
	// Condensed version of PIT_CheckLine
	const fixed_t maxstepmove = FixedMul(MAXSTEPMOVE, mapobjectscale);
	fixed_t maxstep = maxstepmove;
	fixed_t linedist = INT32_MAX;
	INT32 lineside = 0;
	vertex_t pos;

	if (!globalsmuggle.botmo || P_MobjWasRemoved(globalsmuggle.botmo) || !globalsmuggle.botmo->player)
	{
		return false;
	}

	if (line->polyobj && !(line->polyobj->flags & POF_SOLID))
	{
		return true;
	}

	if (tmbbox[BOXRIGHT] <= line->bbox[BOXLEFT] || tmbbox[BOXLEFT] >= line->bbox[BOXRIGHT]
		|| tmbbox[BOXTOP] <= line->bbox[BOXBOTTOM] || tmbbox[BOXBOTTOM] >= line->bbox[BOXTOP])
	{
		return true;
	}

	if (P_BoxOnLineSide(tmbbox, line) != -1)
	{
		return true;
	}

	lineside = P_PointOnLineSide(globalsmuggle.botmo->x, globalsmuggle.botmo->y, line);

	// one sided line
	if (!line->backsector)
	{
		if (lineside)
		{
			// don't hit the back side
			return true;
		}

		goto blocked;
	}

	if ((line->flags & ML_IMPASSABLE) || (line->flags & ML_BLOCKPLAYERS))
	{
		goto blocked;
	}

	// set openrange, opentop, openbottom
	P_LineOpening(line, globalsmuggle.botmo);

	if (globalsmuggle.botmo->player->kartstuff[k_waterskip])
		maxstep += maxstepmove;

	if (P_MobjTouchingSectorSpecial(globalsmuggle.botmo, 1, 13, false))
		maxstep <<= 1;
	else if (P_MobjTouchingSectorSpecial(globalsmuggle.botmo, 1, 12, false))
		maxstep = 0;

	if ((openrange < globalsmuggle.botmo->height) // doesn't fit
		|| (opentop - globalsmuggle.botmo->z < globalsmuggle.botmo->height) // mobj is too high
		|| (openbottom - globalsmuggle.botmo->z > maxstep)) // too big a step up
	{
		goto blocked;
	}

	// Treat damage sectors like walls
	P_ClosestPointOnLine(globalsmuggle.botmo->x, globalsmuggle.botmo->y, line, &pos);

	if (lineside)
	{
		if (K_BotHatesThisSector(globalsmuggle.botmo->player, line->frontsector, pos.x, pos.y))
			goto blocked;
	}
	else
	{
		if (K_BotHatesThisSector(globalsmuggle.botmo->player, line->backsector, pos.x, pos.y))
			goto blocked;
	}

	// We weren't blocked!
	return true;

blocked:
	linedist = K_DistanceOfLineFromPoint(line->v1->x, line->v1->y, line->v2->x, line->v2->y, globalsmuggle.botmo->x, globalsmuggle.botmo->y);
	linedist -= (globalsmuggle.botmo->radius * 8); // Maintain a reasonable distance away from it

	if (linedist > globalsmuggle.distancetocheck)
	{
		return true;
	}

	if (linedist <= 0)
	{
		globalsmuggle.closestlinedist = 0;
		return false;
	}

	if (linedist < globalsmuggle.closestlinedist)
	{
		globalsmuggle.closestlinedist = linedist;
	}

	return true;
}

/*--------------------------------------------------
	fixed_t K_BotReducePrediction(player_t *player)

		See header file for description.
--------------------------------------------------*/
fixed_t K_BotReducePrediction(player_t *player)
{
	INT32 xl, xh, yl, yh, bx, by;

	globalsmuggle.botmo = player->mo;
	globalsmuggle.distancetocheck = (player->mo->radius * 16);
	globalsmuggle.closestlinedist = INT32_MAX;

	tmx = player->mo->x;
	tmy = player->mo->y;

	xl = (unsigned)(tmx - globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(tmx + globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(tmy - globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(tmy + globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	tmbbox[BOXTOP] = tmy + globalsmuggle.distancetocheck;
	tmbbox[BOXBOTTOM] = tmy - globalsmuggle.distancetocheck;
	tmbbox[BOXRIGHT] = tmx + globalsmuggle.distancetocheck;
	tmbbox[BOXLEFT] = tmx - globalsmuggle.distancetocheck;

	// Check for lines that the bot might collide with
	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockLinesIterator(bx, by, K_FindBlockingWalls);
		}
	}

	if (globalsmuggle.closestlinedist == INT32_MAX)
	{
		return FRACUNIT;
	}

	return FixedDiv(globalsmuggle.closestlinedist, globalsmuggle.distancetocheck);
}

/*--------------------------------------------------
	static void K_SteerFromObject(mobj_t *bot, mobj_t *thing, fixed_t fulldist, fixed_t xdist, boolean towards, INT16 amount)

		Handles steering away/towards the specified object.

	Input Arguments:-
		bot - Bot's mobj.
		thing - Mobj to steer towards/away from.
		fulldist - Distance away from object.
		xdist - Horizontal distance away from object.
		towards - If true, steer towards the object. Otherwise, steer away.
		amount - How hard to turn.

	Return:-
		None
--------------------------------------------------*/
static void K_SteerFromObject(mobj_t *bot, mobj_t *thing, fixed_t fulldist, fixed_t xdist, boolean towards, INT16 amount)
{
	angle_t destangle = R_PointToAngle2(bot->x, bot->y, thing->x, thing->y);
	angle_t angle;
	SINT8 flip = 1;

	amount = (amount * FixedDiv(globalsmuggle.distancetocheck - fulldist, globalsmuggle.distancetocheck)) / FRACUNIT;

	if (amount == 0)
	{
		// Shouldn't happen
		return;
	}

	if (towards)
	{
		if (xdist < FixedHypot(bot->radius, thing->radius))
		{
			// Don't need to turn any harder!

			if (abs(globalsmuggle.steer) <= amount)
			{
				globalsmuggle.steer = 0;
			}
			else
			{
				if (globalsmuggle.steer > 0)
				{
					globalsmuggle.steer -= amount;
				}
				else if (globalsmuggle.steer < 0)
				{
					globalsmuggle.steer += amount;
				}
			}

			return;
		}

		// Still turning towards it, flip.
		flip = -flip;
	}

	angle = (bot->angle - destangle);
	if (angle < ANGLE_180)
	{
		flip = -flip;
	}

	// If going in the opposite direction of where you wanted to turn,
	// then reduce the amount that you can turn in that direction.
	if ((flip == 1 && globalsmuggle.curturn < 0)
	|| (flip == -1 && globalsmuggle.curturn > 0))
	{
		amount /= 4;
	}

	globalsmuggle.steer += amount * flip;
}

/*--------------------------------------------------
	static boolean K_BotSteerObjects(mobj_t *thing)

		Blockmap search function.
		Finds objects around the bot to steer towards/away from.

	Input Arguments:-
		thing - Object passed in from iteration.

	Return:-
		true continues searching, false ends the search early.
--------------------------------------------------*/
static boolean K_BotSteerObjects(mobj_t *thing)
{
	INT16 anglediff;
	fixed_t xdist, ydist, fulldist;
	angle_t destangle, angle;
	INT16 attack = ((9 - globalsmuggle.botmo->player->kartspeed) * KART_FULLTURN) / 8; // Acceleration chars are more aggressive
	INT16 dodge = ((9 - globalsmuggle.botmo->player->kartweight) * KART_FULLTURN) / 8; // Handling chars dodge better

	if (!globalsmuggle.botmo || P_MobjWasRemoved(globalsmuggle.botmo) || !globalsmuggle.botmo->player)
	{
		return false;
	}

	if (!thing->health)
	{
		return true;
	}

	if (globalsmuggle.botmo == thing)
	{
		return true;
	}

	xdist = K_DistanceOfLineFromPoint(
		globalsmuggle.botmo->x, globalsmuggle.botmo->y,
		globalsmuggle.botmo->x + FINECOSINE(globalsmuggle.botmo->angle >> ANGLETOFINESHIFT), globalsmuggle.botmo->y + FINESINE(globalsmuggle.botmo->angle >> ANGLETOFINESHIFT),
		thing->x, thing->y
	) / 2; // weight x dist more heavily than y dist

	ydist = K_DistanceOfLineFromPoint(
		globalsmuggle.botmo->x, globalsmuggle.botmo->y,
		globalsmuggle.botmo->x + FINECOSINE((globalsmuggle.botmo->angle + ANGLE_90) >> ANGLETOFINESHIFT), globalsmuggle.botmo->y + FINESINE((globalsmuggle.botmo->angle + ANGLE_90) >> ANGLETOFINESHIFT),
		thing->x, thing->y
	);

	fulldist = FixedHypot(xdist, ydist);

	if (fulldist > globalsmuggle.distancetocheck)
	{
		return true;
	}

	if (!P_CheckSight(globalsmuggle.botmo, thing))
	{
		return true;
	}

	destangle = R_PointToAngle2(globalsmuggle.botmo->x, globalsmuggle.botmo->y, thing->x, thing->y);
	angle = (globalsmuggle.botmo->angle - destangle);

	if (angle < ANGLE_180)
	{
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else 
	{
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
	}

	anglediff = abs(anglediff);

#define PlayerAttackSteer(botcond, thingcond) \
	if ((botcond) && !(thingcond)) \
	{ \
		K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true, 2 * (KART_FULLTURN + attack)); \
	} \
	else if ((thingcond) && !(botcond)) \
	{ \
		K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, false, 2 * (KART_FULLTURN + dodge)); \
	}

	switch (thing->type)
	{
		case MT_BANANA:
		case MT_BANANA_SHIELD:
		case MT_EGGMANITEM_SHIELD:
		case MT_ORBINAUT:
		case MT_ORBINAUT_SHIELD:
		case MT_JAWZ:
		case MT_JAWZ_DUD:
		case MT_JAWZ_SHIELD:
		case MT_SSMINE:
		case MT_SSMINE_SHIELD:
		case MT_BALLHOG:
		case MT_SPB:
		case MT_BUBBLESHIELDTRAP:
			K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, false, 2 * (KART_FULLTURN + dodge));
			break;
		case MT_RANDOMITEM:
			if (anglediff >= 60)
			{
				break;
			}

			if (P_CanPickupItem(globalsmuggle.botmo->player, 1))
			{
				K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true, KART_FULLTURN + attack);
			}
			break;
		case MT_EGGMANITEM:
			if (anglediff >= 60)
			{
				break;
			}

			if (P_CanPickupItem(globalsmuggle.botmo->player, 1)) // Can pick up an actual item
			{
				const UINT8 stealth = K_EggboxStealth(thing->x, thing->y);
				const UINT8 requiredstealth = (globalsmuggle.botmo->player->botvars.difficulty * globalsmuggle.botmo->player->botvars.difficulty);

				if (stealth >= requiredstealth)
				{
					K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true, 2 * (KART_FULLTURN + attack));
				}
				else
				{
					K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, false, 2 * (KART_FULLTURN + dodge));
				}
			}
			break;
		case MT_FLOATINGITEM:
			if (anglediff >= 60)
			{
				break;
			}

			if (P_CanPickupItem(globalsmuggle.botmo->player, 3))
			{
				K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true, KART_FULLTURN + attack);
			}
			break;
		case MT_RING:
		case MT_FLINGRING:
			if (anglediff >= 60)
			{
				break;
			}

			if ((RINGTOTAL(globalsmuggle.botmo->player) < 20 && !globalsmuggle.botmo->player->kartstuff[k_ringlock]
				&& P_CanPickupItem(globalsmuggle.botmo->player, 0))
				&& !thing->extravalue1
				&& (globalsmuggle.botmo->player->kartstuff[k_itemtype] != KITEM_THUNDERSHIELD))
			{
				K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true,
					(RINGTOTAL(globalsmuggle.botmo->player) < 3
					? (4 * (KART_FULLTURN + attack))
					: (KART_FULLTURN + attack))
				);
			}
			break;
		case MT_PLAYER:
			if (thing->player
				&& !thing->player->kartstuff[k_hyudorotimer]
				&& !globalsmuggle.botmo->player->kartstuff[k_hyudorotimer])
			{
				// There REALLY ought to be a better way to handle this logic, right?!
				// Squishing
				PlayerAttackSteer(
					globalsmuggle.botmo->scale > thing->scale + (mapobjectscale/8),
					thing->scale > globalsmuggle.botmo->scale + (mapobjectscale/8)
				)
				// Invincibility
				else PlayerAttackSteer(
					globalsmuggle.botmo->player->kartstuff[k_invincibilitytimer],
					thing->player->kartstuff[k_invincibilitytimer]
				)
				// Thunder Shield
				else PlayerAttackSteer(
					globalsmuggle.botmo->player->kartstuff[k_itemtype] == KITEM_THUNDERSHIELD,
					thing->player->kartstuff[k_itemtype] == KITEM_THUNDERSHIELD
				)
				// Bubble Shield
				else PlayerAttackSteer(
					globalsmuggle.botmo->player->kartstuff[k_itemtype] == KITEM_BUBBLESHIELD,
					thing->player->kartstuff[k_itemtype] == KITEM_BUBBLESHIELD
				)
				// Flame Shield
				else PlayerAttackSteer(
					globalsmuggle.botmo->player->kartstuff[k_itemtype] == KITEM_FLAMESHIELD,
					thing->player->kartstuff[k_itemtype] == KITEM_FLAMESHIELD
				)
				// Has held item shield
				else PlayerAttackSteer(
					(globalsmuggle.botmo->player->kartstuff[k_itemheld] || globalsmuggle.botmo->player->kartstuff[k_eggmanheld]),
					(thing->player->kartstuff[k_itemheld] || thing->player->kartstuff[k_eggmanheld])
				)
				// Ring Sting
				else PlayerAttackSteer(
					thing->player->kartstuff[k_rings] <= 0,
					globalsmuggle.botmo->player->kartstuff[k_rings] <= 0
				)
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
						K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true, KART_FULLTURN + attack);
					}
					else
					{
						K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, false, KART_FULLTURN + dodge);
					}
				}
			}
			break;
		case MT_BOTHINT:
			if (anglediff >= 60)
			{
				break;
			}

			if (thing->extravalue1 == 0)
			{
				K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, false, thing->extravalue2 * (KART_FULLTURN + dodge));
			}
			{
				K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, true, thing->extravalue2 * (KART_FULLTURN + attack));
			}
			break;
		default:
			if (thing->flags & (MF_SOLID|MF_ENEMY|MF_BOSS|MF_PAIN|MF_MISSILE|MF_FIRE))
			{
				K_SteerFromObject(globalsmuggle.botmo, thing, fulldist, xdist, false, 2 * (KART_FULLTURN + dodge));
			}
			break;
	}

	return true;
}

/*--------------------------------------------------
	INT16 K_BotFindObjects(player_t *player, INT16 turn)

		See header file for description.
--------------------------------------------------*/
INT16 K_BotFindObjects(player_t *player, INT16 turn)
{
	INT32 xl, xh, yl, yh, bx, by;

	globalsmuggle.steer = 0;
	globalsmuggle.botmo = player->mo;
	globalsmuggle.curturn = turn;
	globalsmuggle.distancetocheck = (player->mo->radius * 32) + (player->speed * 4);

	xl = (unsigned)(globalsmuggle.botmo->x - globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(globalsmuggle.botmo->x + globalsmuggle.distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(globalsmuggle.botmo->y - globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(globalsmuggle.botmo->y + globalsmuggle.distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_BotSteerObjects);
		}
	}

	return globalsmuggle.steer;
}
