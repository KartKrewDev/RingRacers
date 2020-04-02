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
	fixed_t t = FixedDiv(dot, mag);

	fixed_t px = v1x + FixedMul(v1tov2[0], t);
	fixed_t py = v1y + FixedMul(v1tov2[1], t);

	return P_AproxDistance(cx - px, cy - py);
}

static botprediction_t *K_CreateBotPrediction(player_t *player)
{
	const INT32 futuresight = (3*TICRATE/4); // How far ahead into the future to try and predict
	const INT32 distance = (player->speed / FRACUNIT) * futuresight;
	INT32 distanceleft = distance;
	botprediction_t *predictcoords = Z_Calloc(sizeof(botprediction_t), PU_LEVEL, NULL);
	waypoint_t *wp = player->nextwaypoint;
	fixed_t smallestradius = wp->mobj->radius;
	size_t nwp;
	size_t i;
	INT32 lp = 0;

	if (distance <= 0)
	{
		predictcoords->x = wp->mobj->x;
		predictcoords->y = wp->mobj->y;
		predictcoords->radius = smallestradius;
		return predictcoords;
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

	predictcoords->x = wp->mobj->x;
	predictcoords->y = wp->mobj->y;
	predictcoords->radius = smallestradius;

	if (distanceleft > 0)
	{
		angle_t a = R_PointToAngle2(wp->mobj->x, wp->mobj->y, wp->nextwaypoints[nwp]->mobj->x, wp->nextwaypoints[nwp]->mobj->y);

		predictcoords->x += P_ReturnThrustX(NULL, a, distanceleft * FRACUNIT);
		predictcoords->y += P_ReturnThrustY(NULL, a, distanceleft * FRACUNIT);
	}

	return predictcoords;
}

mobj_t *botmo = NULL;
INT16 badsteerglobal = 0;
INT32 predictx = 0, predicty = 0;

static void K_SteerFromWall(mobj_t *bot, line_t *ld)
{
	const INT16 amount = KART_FULLTURN + (KART_FULLTURN/4); // KART_FULLTURN/4, but cancel out full turn from earlier turning
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
		badsteerglobal = -amount;
	}
	else
	{
		badsteerglobal = amount;
	}
}

static inline boolean K_FindBlockingWalls(line_t *ld)
{
	// Condensed version of PIT_CheckLine
	const fixed_t maxstepmove = FixedMul(MAXSTEPMOVE, mapobjectscale);
	fixed_t maxstep = maxstepmove;

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

	// one sided line
	if (!ld->backsector)
	{
		if (P_PointOnLineSide(botmo->x, botmo->y, ld))
		{
			// don't hit the back side
			return true;
		}

		K_SteerFromWall(botmo, ld);
		return false;
	}

	if ((ld->flags & ML_IMPASSABLE) || (ld->flags & ML_BLOCKPLAYERS))
	{
		K_SteerFromWall(botmo, ld);
		return false;
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
		return false;
	}

	return true;
}

static INT16 K_BotSteerFromWalls(player_t *player, botprediction_t *predict)
{
	INT32 xl, xh, yl, yh, bx, by;
	fixed_t radius = predict->radius / 3;

	badsteerglobal = 0;

	botmo = player->mo;
	tmx = player->mo->x + P_ReturnThrustX(NULL, player->mo->angle, player->speed);
	tmy = player->mo->y + P_ReturnThrustY(NULL, player->mo->angle, player->speed);

	predictx = predict->x;
	predicty = predict->y;

	xl = (unsigned)(tmx - radius - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (unsigned)(tmx + radius - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (unsigned)(tmy - radius - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (unsigned)(tmy + radius - bmaporgy)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	tmbbox[BOXTOP] = tmy + radius;
	tmbbox[BOXBOTTOM] = tmy - radius;
	tmbbox[BOXRIGHT] = tmx + radius;
	tmbbox[BOXLEFT] = tmx - radius;

	// check lines
	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			P_BlockLinesIterator(bx, by, K_FindBlockingWalls);
		}
	}

	return badsteerglobal;
}

void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)
{
	botprediction_t *predict = NULL;
	boolean ontrack = false;
	INT16 turnamt = 0;

	// Can't build a ticcmd if we aren't spawned...
	if (!player->mo)
		return;

	cmd->forwardmove = 0;
	cmd->driftturn = 0;
	cmd->buttons = 0;

	if (player->playerstate == PST_DEAD)
	{
		cmd->buttons |= BT_ACCELERATE;
		return;
	}

#ifdef HAVE_BLUA
	// Let Lua scripts build ticcmds
	if (LUAh_BotTiccmd(player, cmd))
		return;
#endif

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
			fixed_t rad = predict->radius - (player->mo->radius*4);
			fixed_t dirdist = K_DistanceOfLineFromPoint(
				player->mo->x, player->mo->y,
				player->mo->x + FINECOSINE(moveangle >> ANGLETOFINESHIFT), player->mo->y + FINESINE(moveangle >> ANGLETOFINESHIFT),
				predict->x, predict->y
			);

			if (rad < 0)
			{
				rad = 0;
			}

			cmd->buttons |= BT_ACCELERATE;

			// Full speed ahead!
			cmd->forwardmove = 50;

			if (dirdist <= rad)
			{
				fixed_t speedmul = FixedMul(player->speed, K_GetKartSpeed(player, false));
				fixed_t speedrad = rad/4;

				ontrack = true;

				if (speedmul > FRACUNIT)
				{
					speedmul = FRACUNIT;
				}

				// Increase radius with speed
				// At low speed, the CPU will try to be more accurate
				// At high speed, they're more likely to lawnmower
				speedrad += FixedMul(speedmul, rad/2);

				if (dirdist < speedrad)
				{
					// Don't need to turn!
					turnamt = 0;
				}
				else
				{
					// Make minor adjustments
					turnamt /= 4;
				}
			}
			else if (anglediff > 60)
			{
				// Actually, don't go too fast...
				cmd->forwardmove /= 2;
				cmd->buttons |= BT_BRAKE;
			}
		}

		turnamt += K_BotSteerFromWalls(player, predict);
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

		cmd->driftturn = turnamt;
		cmd->angleturn = (player->mo->angle >> 16) + turnamt;
	}

	(void)ontrack;

	if (player->kartstuff[k_userings] == 1)
	{
		if (!player->exiting)
		{
			INT32 saferingsval = 8 + K_GetKartRingPower(player);

			if (player->speed < K_GetKartSpeed(player, false)/2 // Being slowed down too much
				|| player->kartstuff[k_speedboost] > 0) // Have another type of boost (tethering)
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
			return;
		}

		if (player->kartstuff[k_rocketsneakertimer] > 0)
		{
			if (player->kartstuff[k_botitemconfirm] > TICRATE)
			{
				if (player->kartstuff[k_sneakertimer] <= (TICRATE/3) && !(player->pflags & PF_ATTACKDOWN))
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
				case KITEM_SNEAKER:
					if ((player->kartstuff[k_offroad] && K_ApplyOffroad(player)) // Stuck in offroad, use it NOW
						|| K_GetWaypointIsShortcut(player->nextwaypoint) == true // Going toward a shortcut!
						|| player->speed < K_GetKartSpeed(player, false)/2 // Being slowed down too much
						|| player->kartstuff[k_speedboost] > 0 // Have another type of boost (tethering)
						|| player->kartstuff[k_botitemconfirm] > 4*TICRATE) // Held onto it for too long
					{
						if (player->kartstuff[k_sneakertimer] <= (TICRATE/3) && !(player->pflags & PF_ATTACKDOWN))
						{
							cmd->buttons |= BT_ATTACK;
							player->kartstuff[k_botitemconfirm] -= 2*TICRATE;
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
				case KITEM_INVINCIBILITY:
				case KITEM_SPB:
				case KITEM_GROW:
				case KITEM_SHRINK:
				case KITEM_HYUDORO:
				case KITEM_SUPERRING:
					cmd->buttons |= BT_ATTACK;
					player->kartstuff[k_botitemconfirm] = 0;
					break;
				default:
					player->kartstuff[k_botitemconfirm] = 0;
					break;
			}
		}
	}

	if (predict != NULL)
	{
		Z_Free(predict);
	}
}

