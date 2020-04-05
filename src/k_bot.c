// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2011-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bot.c
/// \brief Basic bot handling

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

void K_AddBots(SINT8 numbots)
{
	UINT8 newplayernum = 0;

	if (dedicated)
		newplayernum = 1;

	while (numbots > 0)
	{
		UINT8 buf[2];
		UINT8 *buf_p = buf;

		numbots--;

		// search for a free playernum
		// we can't use playeringame since it is not updated here
		for (; newplayernum < MAXPLAYERS; newplayernum++)
		{
			UINT8 n;

			for (n = 0; n < MAXNETNODES; n++)
				if (nodetoplayer[n] == newplayernum
				|| nodetoplayer2[n] == newplayernum
				|| nodetoplayer3[n] == newplayernum
				|| nodetoplayer4[n] == newplayernum)
					break;

			if (n == MAXNETNODES)
				break;
		}

		WRITEUINT8(buf_p, newplayernum);

		// test skins
		if (numbots == 6)
			WRITEUINT8(buf_p, 0);
		else if (numbots == 5)
			WRITEUINT8(buf_p, 1);
		else if (numbots == 4)
			WRITEUINT8(buf_p, 2);
		else if (numbots == 3)
			WRITEUINT8(buf_p, 3);
		else if (numbots == 2)
			WRITEUINT8(buf_p, 5);
		else if (numbots == 1)
			WRITEUINT8(buf_p, 9);
		else
			WRITEUINT8(buf_p, 10);

		SendNetXCmd(XD_ADDBOT, buf, buf_p - buf);

		DEBFILE(va("Server added bot %d\n", newplayernum));
		// use the next free slot (we can't put playeringame[newplayernum] = true here)
		newplayernum++;
	}
}

boolean K_PlayerUsesBotMovement(player_t *player)
{
	if (player->bot || player->exiting)
		return true;

	return false;
}

boolean K_BotCanTakeCut(player_t *player)
{
	if (!K_ApplyOffroad(player)
		|| player->kartstuff[k_itemtype] == KITEM_SNEAKER
		|| player->kartstuff[k_itemtype] == KITEM_INVINCIBILITY
		|| player->kartstuff[k_itemtype] == KITEM_HYUDORO)
		return true;

	return false;
}

static fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy)
{
	fixed_t v1toc[2] = {cx - v1x, cy - v1y};
	fixed_t v1tov2[2] = {v2x - v1x, v2y - v1y};

	fixed_t mag = FixedMul(v1tov2[0], v1tov2[0]) + FixedMul(v1tov2[1], v1tov2[1]);
	fixed_t dot = FixedMul(v1toc[0], v1tov2[0]) + FixedMul(v1toc[1], v1tov2[1]);

	fixed_t t;
	fixed_t px, py;

	if (mag == 0)
	{
		return 0;
	}

	t = FixedDiv(dot, mag);

	px = v1x + FixedMul(v1tov2[0], t);
	py = v1y + FixedMul(v1tov2[1], t);

	return P_AproxDistance(cx - px, cy - py);
}

static botprediction_t *K_CreateBotPrediction(player_t *player)
{
	const INT32 futuresight = (3*TICRATE/4); // How far ahead into the future to try and predict
	const INT32 distance = (player->speed / FRACUNIT) * futuresight;
	INT32 distanceleft = distance;
	botprediction_t *predict = Z_Calloc(sizeof(botprediction_t), PU_LEVEL, NULL);
	waypoint_t *wp = player->nextwaypoint;
	fixed_t smallestradius = wp->mobj->radius;
	size_t nwp;
	size_t i;
	INT32 lp = 0;

	if (distance <= 0)
	{
		predict->x = wp->mobj->x;
		predict->y = wp->mobj->y;
		predict->radius = smallestradius;
		return predict;
	}

	while (distanceleft > 0)
	{
		lp++;

		nwp = 0;

		if (wp->numnextwaypoints == 0)
		{
			distanceleft = 0;
			break;
		}

		if (wp->numnextwaypoints > 1)
		{
			fixed_t closest = INT32_MAX;
			fixed_t dist    = INT32_MAX;

			for (i = 0; i < wp->numnextwaypoints; i++)
			{
				if (K_GetWaypointIsShortcut(wp->nextwaypoints[i]) && !K_BotCanTakeCut(player))
				{
					continue;
				}

				dist = P_AproxDistance(
					player->mo->x - wp->nextwaypoints[i]->mobj->x,
					player->mo->y - wp->nextwaypoints[i]->mobj->y
				);

				if (dist < closest)
				{
					nwp = i;
					closest = dist;
				}
			}
		}

		if ((INT32)(wp->nextwaypointdistances[nwp]) > distanceleft)
		{
			break;
		}

		distanceleft -= wp->nextwaypointdistances[nwp];

		if (wp->nextwaypoints[nwp]->mobj->radius < smallestradius)
		{
			smallestradius = wp->nextwaypoints[nwp]->mobj->radius;
		}

		wp = wp->nextwaypoints[nwp];
	}

	predict->x = wp->mobj->x;
	predict->y = wp->mobj->y;
	predict->radius = smallestradius;

	if (distanceleft > 0)
	{
		angle_t a = R_PointToAngle2(wp->mobj->x, wp->mobj->y, wp->nextwaypoints[nwp]->mobj->x, wp->nextwaypoints[nwp]->mobj->y);

		predict->x += P_ReturnThrustX(NULL, a, distanceleft * FRACUNIT);
		predict->y += P_ReturnThrustY(NULL, a, distanceleft * FRACUNIT);
	}

	return predict;
}

mobj_t *botmo = NULL;
fixed_t distancetocheck = 0;
INT16 badsteerglobal = 0;
fixed_t predictx = 0, predicty = 0;

static void K_SteerFromWall(mobj_t *bot, line_t *ld)
{
	const INT16 amount = 4*KART_FULLTURN;
	INT32 side = P_PointOnLineSide(bot->x, bot->y, ld);
	angle_t lineangle = R_PointToAngle2(0, 0, ld->dx, ld->dy) - ANGLE_90;
	angle_t destangle = R_PointToAngle2(bot->x, bot->y, predictx, predicty);
	angle_t angle;

	if (side == 1)
	{
		lineangle += ANGLE_180;
	}

	angle = (destangle - lineangle);

	if (angle < ANGLE_180)
	{
		badsteerglobal -= amount;
	}
	else
	{
		badsteerglobal += amount;
	}
}

static inline boolean K_FindBlockingWalls(line_t *ld)
{
	// Condensed version of PIT_CheckLine
	const fixed_t maxstepmove = FixedMul(MAXSTEPMOVE, mapobjectscale);
	fixed_t maxstep = maxstepmove;
	INT32 lineside = 0;

	if (!botmo || P_MobjWasRemoved(botmo) || !botmo->player)
	{
		return false;
	}

	if (ld->polyobj && !(ld->polyobj->flags & POF_SOLID))
	{
		return true;
	}

	if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
		|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	{
		return true;
	}

	if (P_BoxOnLineSide(tmbbox, ld) != -1)
	{
		return true;
	}

	lineside = P_PointOnLineSide(botmo->x, botmo->y, ld);

	// one sided line
	if (!ld->backsector)
	{
		if (lineside)
		{
			// don't hit the back side
			return true;
		}

		K_SteerFromWall(botmo, ld);
		return true;
	}

	if ((ld->flags & ML_IMPASSABLE) || (ld->flags & ML_BLOCKPLAYERS))
	{
		K_SteerFromWall(botmo, ld);
		return true;
	}

	// set openrange, opentop, openbottom
	P_LineOpening(ld, botmo);

	if (botmo->player->kartstuff[k_waterskip])
		maxstep += maxstepmove;

	if (P_MobjTouchingSectorSpecial(botmo, 1, 13, false))
		maxstep <<= 1;
	else if (P_MobjTouchingSectorSpecial(botmo, 1, 12, false))
		maxstep = 0;

	if ((openrange < botmo->height) // doesn't fit
		|| (opentop - botmo->z < botmo->height) // mobj is too high
		|| (openbottom - botmo->z > maxstep)) // too big a step up
	{
		K_SteerFromWall(botmo, ld);
		return true;
	}

	return true;
}

static INT16 K_BotSteerFromWalls(player_t *player, botprediction_t *predict)
{
	INT32 xl, xh, yl, yh, bx, by;

	badsteerglobal = 0;

	botmo = player->mo;
	distancetocheck = player->mo->radius * 8;

	tmx = player->mo->x + P_ReturnThrustX(NULL, player->mo->angle, player->speed);
	tmy = player->mo->y + P_ReturnThrustY(NULL, player->mo->angle, player->speed);

	predictx = predict->x;
	predicty = predict->y;

	xl = (unsigned)(tmx - distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(tmx + distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(tmy - distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(tmy + distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	tmbbox[BOXTOP] = tmy + distancetocheck;
	tmbbox[BOXBOTTOM] = tmy - distancetocheck;
	tmbbox[BOXRIGHT] = tmx + distancetocheck;
	tmbbox[BOXLEFT] = tmx - distancetocheck;

	// Check for lines that the bot might collide with
	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockLinesIterator(bx, by, K_FindBlockingWalls);
		}
	}

	return badsteerglobal;
}

static void K_SteerFromObject(mobj_t *bot, mobj_t *thing, boolean towards, INT16 amount)
{
	angle_t destangle = R_PointToAngle2(bot->x, bot->y, thing->x, thing->y);
	angle_t angle;

	if (towards)
	{
		fixed_t dist = K_DistanceOfLineFromPoint(
			bot->x, bot->y,
			bot->x + FINECOSINE(bot->angle >> ANGLETOFINESHIFT), bot->y + FINESINE(bot->angle >> ANGLETOFINESHIFT),
			thing->x, thing->y
		);

		if (dist <= (bot->radius + thing->radius))
		{
			// Don't need to turn any harder!
			return;
		}

		amount = -amount;
	}

	angle = (bot->angle - destangle);

	if (angle < ANGLE_180)
	{
		badsteerglobal -= amount;
	}
	else
	{
		badsteerglobal += amount;
	}
}

static inline boolean K_BotSteerObjects(mobj_t *thing)
{
	INT16 anglediff;
	fixed_t dist;
	angle_t destangle, angle;

	if (!botmo || P_MobjWasRemoved(botmo) || !botmo->player)
	{
		return false;
	}

	if (!thing->health)
	{
		return true;
	}

	if (botmo == thing)
	{
		return true;
	}

	dist = P_AproxDistance(P_AproxDistance(
		botmo->x - thing->x,
		botmo->y - thing->y),
		(botmo->z - thing->z) / 4
	);

	if (dist > distancetocheck)
	{
		return true;
	}

	if (!P_CheckSight(botmo, thing))
	{
		return true;
	}

	destangle = R_PointToAngle2(botmo->x, botmo->y, thing->x, thing->y);
	angle = (botmo->angle - destangle);

	if (angle < ANGLE_180)
	{
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else 
	{
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
	}

	anglediff = abs(anglediff);

	switch (thing->type)
	{
		case MT_BANANA:
		case MT_BANANA_SHIELD:
		case MT_EGGMANITEM:
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
			K_SteerFromObject(botmo, thing, false, 2*KART_FULLTURN);
			break;
		case MT_RANDOMITEM:
			if (anglediff > 90)
			{
				break;
			}

			if (P_CanPickupItem(botmo->player, 1))
			{
				K_SteerFromObject(botmo, thing, true, 2*KART_FULLTURN);
			}
			break;
		case MT_FLOATINGITEM:
			if (anglediff > 90)
			{
				break;
			}

			if (P_CanPickupItem(botmo->player, 3))
			{
				K_SteerFromObject(botmo, thing, true, 2*KART_FULLTURN);
			}
			break;
		case MT_RING:
		case MT_FLINGRING:
			if (anglediff > 90)
			{
				break;
			}

			if ((RINGTOTAL(botmo->player) < 20 && !botmo->player->kartstuff[k_ringlock]
				&& P_CanPickupItem(botmo->player, 0))
				&& (!thing->extravalue1))
			{
				K_SteerFromObject(botmo, thing, true, (RINGTOTAL(botmo->player) <= 0 ? 2*KART_FULLTURN : KART_FULLTURN));
			}
			break;
		case MT_PLAYER:
			if (thing->player
				&& !thing->player->kartstuff[k_hyudorotimer]
				&& !botmo->player->kartstuff[k_hyudorotimer])
			{
				INT16 attack = ((9 - botmo->player->kartspeed) * KART_FULLTURN) / 8; // Acceleration chars are more aggressive
				INT16 dodge = ((9 - botmo->player->kartweight) * KART_FULLTURN) / 8; // Handling chars dodge better

#define PlayerAttackSteer(botcond, thingcond) \
	if ((botcond) && !(thingcond)) \
	{ \
		K_SteerFromObject(botmo, thing, true, KART_FULLTURN + attack); \
	} \
	else if ((thingcond) && !(botcond)) \
	{ \
		K_SteerFromObject(botmo, thing, false, KART_FULLTURN + dodge); \
	}

				// There REALLY ought to be a better way to handle this logic, right?!
				// Squishing
				PlayerAttackSteer(
					botmo->scale > thing->scale + (mapobjectscale/8),
					thing->scale > botmo->scale + (mapobjectscale/8)
				)
				// Invincibility
				else PlayerAttackSteer(
					botmo->player->kartstuff[k_invincibilitytimer],
					thing->player->kartstuff[k_invincibilitytimer]
				)
				// Thunder Shield
				else PlayerAttackSteer(
					botmo->player->kartstuff[k_itemtype] == KITEM_THUNDERSHIELD,
					thing->player->kartstuff[k_itemtype] == KITEM_THUNDERSHIELD
				)
				// Bubble Shield
				else PlayerAttackSteer(
					botmo->player->kartstuff[k_itemtype] == KITEM_BUBBLESHIELD,
					thing->player->kartstuff[k_itemtype] == KITEM_BUBBLESHIELD
				)
				// Flame Shield
				else PlayerAttackSteer(
					botmo->player->kartstuff[k_itemtype] == KITEM_FLAMESHIELD,
					thing->player->kartstuff[k_itemtype] == KITEM_FLAMESHIELD
				)
				// Has held item shield
				else PlayerAttackSteer(
					(botmo->player->kartstuff[k_itemheld] || botmo->player->kartstuff[k_eggmanheld]),
					(thing->player->kartstuff[k_itemheld] || thing->player->kartstuff[k_eggmanheld])
				)
				// Ring Sting
				else PlayerAttackSteer(
					thing->player->kartstuff[k_rings] <= 0,
					botmo->player->kartstuff[k_rings] <= 0
				)
				else
				{
					// After ALL of that, we can do standard bumping
					const fixed_t ourweight = K_GetMobjWeight(botmo, thing);
					const fixed_t theirweight = K_GetMobjWeight(thing, botmo);
					fixed_t weightdiff = 0;

					if (anglediff > 90)
					{
						weightdiff = theirweight - ourweight;
					}
					else
					{
						weightdiff = ourweight - theirweight;
					}

					if (weightdiff > mapobjectscale)
					{
						K_SteerFromObject(botmo, thing, true, (KART_FULLTURN + attack) / 2);
					}
					else
					{
						K_SteerFromObject(botmo, thing, false, (KART_FULLTURN + dodge) * 2);
					}
				}
			}
			break;
		default:
			if (thing->flags & (MF_SOLID|MF_ENEMY|MF_BOSS|MF_PAIN|MF_MISSILE|MF_FIRE))
			{
				K_SteerFromObject(botmo, thing, false, 2*KART_FULLTURN);
			}
			break;
	}

	return true;
}

static INT16 K_BotFindObjects(player_t *player)
{
	INT32 xl, xh, yl, yh, bx, by;

	badsteerglobal = 0;

	botmo = player->mo;
	distancetocheck = (player->mo->radius * 16) + (player->speed * 4);

	xl = (unsigned)(botmo->x - distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(botmo->x + distancetocheck - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(botmo->y - distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(botmo->y + distancetocheck - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, K_BotSteerObjects);
		}
	}

	return badsteerglobal;
}

static boolean K_BotUseItemNearPlayer(player_t *player, ticcmd_t *cmd, fixed_t radius)
{
	UINT8 i;

	if (player->pflags & PF_ATTACKDOWN)
	{
		return false;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *target = NULL;
		fixed_t dist = INT32_MAX;

		if (!playeringame[i])
		{
			continue;
		}

		target = &players[i];

		if (target->mo == NULL || P_MobjWasRemoved(target->mo)
			|| player == target || target->spectator
			|| target->powers[pw_flashing])
		{
			continue;
		}

		dist = P_AproxDistance(P_AproxDistance(
			player->mo->x - target->mo->x,
			player->mo->y - target->mo->y),
			(player->mo->z - target->mo->z) / 4
		);

		if (dist <= radius)
		{
			cmd->buttons |= BT_ATTACK;
			return true;
		}
	}

	return false;
}

void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)
{
	botprediction_t *predict = NULL;
	INT16 turnamt = 0;

	// Can't build a ticcmd if we aren't spawned...
	if (!player->mo)
		return;

	// Remove any existing controls
	memset(cmd, 0, sizeof(ticcmd_t));
	cmd->angleturn = (player->mo->angle >> 16);

	if (player->playerstate == PST_DEAD)
	{
		cmd->buttons |= BT_ACCELERATE;
		return;
	}

#ifdef HAVE_BLUA
	// Complete override of all ticcmd functionality
	if (LUAh_BotTiccmd(player, cmd))
		return;
#endif

	// Start boost handler
	if (leveltime <= starttime)
	{
		if (leveltime >= starttime-35)
			cmd->buttons |= BT_ACCELERATE;
		return;
	}

	if (player->nextwaypoint != NULL && player->nextwaypoint->mobj != NULL && !P_MobjWasRemoved(player->nextwaypoint->mobj))
	{
		SINT8 turnsign = 0;
		angle_t destangle, moveangle, angle;
		INT16 anglediff;

		predict = K_CreateBotPrediction(player);

		destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
		moveangle = player->mo->angle;

		angle = (moveangle - destangle);

		if (angle < ANGLE_180)
		{
			turnsign = -1; // Turn right
			anglediff = AngleFixed(angle)>>FRACBITS;
		}
		else 
		{
			turnsign = 1; // Turn left
			anglediff = 360-(AngleFixed(angle)>>FRACBITS);
		}

		anglediff = abs(anglediff);
		turnamt = KART_FULLTURN * turnsign;

		if (anglediff > 90)
		{
			// Wrong way!
			cmd->forwardmove = -25;
			cmd->buttons |= BT_BRAKE;
		}
		else
		{
			INT16 wallsteer = K_BotSteerFromWalls(player, predict);
			INT16 objectsteer = 0;
			fixed_t rad = predict->radius - (player->mo->radius*4);
			fixed_t dirdist = K_DistanceOfLineFromPoint(
				player->mo->x, player->mo->y,
				player->mo->x + FINECOSINE(moveangle >> ANGLETOFINESHIFT), player->mo->y + FINESINE(moveangle >> ANGLETOFINESHIFT),
				predict->x, predict->y
			);

			if (anglediff > 0)
			{
				// Become more precise based on how hard you need to turn
				// This makes predictions into turns a little nicer
				// Facing 90 degrees away from the predicted point gives you a 1/3 radius
				rad = ((180 - anglediff) * rad) / 135;
			}

			if (rad < 2*player->mo->radius)
			{
				rad = 2*player->mo->radius;
			}

			cmd->buttons |= BT_ACCELERATE;

			// Full speed ahead!
			cmd->forwardmove = 50;

			if (anglediff > 60)
			{
				// Actually, don't go too fast...
				cmd->forwardmove /= 2;
				cmd->buttons |= BT_BRAKE;
			}
			else if (anglediff <= 23 || dirdist <= rad)
			{
				objectsteer = K_BotFindObjects(player);
			}

			if (dirdist <= rad)
			{
				fixed_t speedmul = FixedMul(player->speed, K_GetKartSpeed(player, false));
				fixed_t speedrad = rad/4;

				if (speedmul > FRACUNIT)
				{
					speedmul = FRACUNIT;
				}

				// Increase radius with speed
				// At low speed, the CPU will try to be more accurate
				// At high speed, they're more likely to lawnmower
				speedrad += FixedMul(speedmul, (3*rad/4) - speedrad);

				if (speedrad < 2*player->mo->radius)
				{
					speedrad = 2*player->mo->radius;
				}

				if (dirdist <= speedrad)
				{
					// Don't turn at all
					turnamt = 0;
				}
				else
				{
					// Make minor adjustments
					turnamt /= 4;
				}
			}

			if (wallsteer != 0)
			{
				turnamt += wallsteer;
			}

			if (objectsteer != 0)
			{
				turnamt += objectsteer;
			}
		}
	}

	if (player->kartstuff[k_userings] == 1)
	{
		if (!player->exiting)
		{
			INT32 saferingsval = 16 - K_GetKartRingPower(player);

			if (player->speed < K_GetKartSpeed(player, false)/2 // Being slowed down too much
				|| player->kartstuff[k_speedboost] > (FRACUNIT/5)) // Have another type of boost (tethering)
			{
				saferingsval -= 5;
			}

			if (player->kartstuff[k_rings] > saferingsval)
			{
				cmd->buttons |= BT_ATTACK;
			}
		}
	}
	else
	{
		if (player->kartstuff[k_botitemdelay])
		{
			player->kartstuff[k_botitemdelay]--;
			player->kartstuff[k_botitemconfirm] = 0;
		}
		else if (player->kartstuff[k_stealingtimer] == 0 && player->kartstuff[k_stolentimer] == 0)
		{
			if (player->kartstuff[k_eggmanexplode])
			{
				K_BotUseItemNearPlayer(player, cmd, 128*player->mo->scale);
			}
			else if (player->kartstuff[k_rocketsneakertimer] > 0)
			{
				if (player->kartstuff[k_botitemconfirm] > TICRATE)
				{
					if (!player->kartstuff[k_sneakertimer] && !(player->pflags & PF_ATTACKDOWN))
					{
						cmd->buttons |= BT_ATTACK;
						player->kartstuff[k_botitemconfirm] = 0;
					}
				}
				else
				{
					player->kartstuff[k_botitemconfirm]++;
				}
			}
			else
			{
				switch (player->kartstuff[k_itemtype])
				{
					case KITEM_INVINCIBILITY:
					case KITEM_SPB:
					case KITEM_GROW:
					case KITEM_SHRINK:
					case KITEM_HYUDORO:
					case KITEM_SUPERRING:
						cmd->buttons |= BT_ATTACK;
						player->kartstuff[k_botitemconfirm] = 0;
						break;
					case KITEM_SNEAKER:
						if ((player->kartstuff[k_offroad] && K_ApplyOffroad(player)) // Stuck in offroad, use it NOW
							|| K_GetWaypointIsShortcut(player->nextwaypoint) == true // Going toward a shortcut!
							|| player->speed < K_GetKartSpeed(player, false)/2 // Being slowed down too much
							|| player->kartstuff[k_speedboost] > (FRACUNIT/8) // Have another type of boost (tethering)
							|| player->kartstuff[k_botitemconfirm] > 4*TICRATE) // Held onto it for too long
						{
							if (!player->kartstuff[k_sneakertimer] && !(player->pflags & PF_ATTACKDOWN))
							{
								cmd->buttons |= BT_ATTACK;
								player->kartstuff[k_botitemconfirm] = 2*TICRATE;
							}
						}
						else
						{
							player->kartstuff[k_botitemconfirm]++;
						}
						break;
					case KITEM_ROCKETSNEAKER:
						if (player->kartstuff[k_rocketsneakertimer] <= 0)
						{
							cmd->buttons |= BT_ATTACK;
							player->kartstuff[k_botitemconfirm] = 0;
						}
						break;
					case KITEM_THUNDERSHIELD:
						if (!K_BotUseItemNearPlayer(player, cmd, 192*player->mo->scale))
						{
							if (player->kartstuff[k_botitemconfirm] > 10*TICRATE)
							{
								cmd->buttons |= BT_ATTACK;
								player->kartstuff[k_botitemconfirm] = 0;
							}
							else
							{
								player->kartstuff[k_botitemconfirm]++;
							}
						}
						break;
					default:
						player->kartstuff[k_botitemconfirm] = 0;
						break;
				}
			}
		}
	}

	if (turnamt != 0)
	{
		if (turnamt > KART_FULLTURN)
		{
			turnamt = KART_FULLTURN;
		}
		else if (turnamt < -KART_FULLTURN)
		{
			turnamt = -KART_FULLTURN;
		}

		if ((turnamt > 0 && player->kartstuff[k_botlastturn] >= 0)
			|| (turnamt < 0 && player->kartstuff[k_botlastturn] <= 0))
		{
			if (turnamt > 0)
			{
				player->kartstuff[k_botlastturn] = 1;
			}
			else if (turnamt < 0)
			{
				player->kartstuff[k_botlastturn] = -1;
			}

			cmd->driftturn = turnamt;
			cmd->angleturn += turnamt;
		}
		else
		{
			player->kartstuff[k_botlastturn] = 0;
		}
	}

	if (predict != NULL)
	{
		Z_Free(predict);
	}
}

