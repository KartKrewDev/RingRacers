// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bot.c
/// \brief Bot logic & ticcmd generation code

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


/*--------------------------------------------------
	boolean K_AddBot(UINT8 skin, UINT8 difficulty, UINT8 *p)

		See header file for description.
--------------------------------------------------*/
boolean K_AddBot(UINT8 skin, UINT8 difficulty, UINT8 *p)
{
	UINT8 buf[3];
	UINT8 *buf_p = buf;
	UINT8 newplayernum = *p;

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

	while (playeringame[newplayernum]
		&& players[newplayernum].bot
		&& newplayernum < MAXPLAYERS)
	{
		newplayernum++;
	}

	if (newplayernum >= MAXPLAYERS)
	{
		*p = newplayernum;
		return false;
	}

	WRITEUINT8(buf_p, newplayernum);

	if (skin > numskins)
	{
		skin = numskins;
	}

	WRITEUINT8(buf_p, skin);

	if (difficulty < 1)
	{
		difficulty = 1;
	}
	else if (difficulty > MAXBOTDIFFICULTY)
	{
		difficulty = MAXBOTDIFFICULTY;
	}

	WRITEUINT8(buf_p, difficulty);

	SendNetXCmd(XD_ADDBOT, buf, buf_p - buf);

	DEBFILE(va("Server added bot %d\n", newplayernum));
	// use the next free slot (we can't put playeringame[newplayernum] = true here)
	newplayernum++;

	*p = newplayernum;
	return true;
}

/*--------------------------------------------------
	void K_UpdateMatchRaceBots(void)

		See header file for description.
--------------------------------------------------*/
void K_UpdateMatchRaceBots(void)
{
	const UINT8 difficulty = cv_kartbot.value;
	UINT8 pmax = min((dedicated ? MAXPLAYERS-1 : MAXPLAYERS), cv_maxplayers.value);
	UINT8 numplayers = 0;
	UINT8 numbots = 0;
	UINT8 numwaiting = 0;
	SINT8 wantedbots = 0;
	boolean skinusable[MAXSKINS];
	UINT8 i;

	if (!server)
	{
		return;
	}

	// init usable bot skins list
	for (i = 0; i < MAXSKINS; i++)
	{
		if (i < numskins)
		{
			skinusable[i] = true;
		}
		else
		{
			skinusable[i] = false;
		}
	}

	if (cv_ingamecap.value > 0)
	{
		pmax = min(pmax, cv_ingamecap.value);
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (!players[i].spectator)
			{
				skinusable[players[i].skin] = false;

				if (players[i].bot)
				{
					numbots++;

					// While we're here, we should update bot difficulty to the proper value.
					players[i].botvars.difficulty = difficulty;
				}
				else
				{
					numplayers++;
				}
			}
			else if (players[i].pflags & PF_WANTSTOJOIN)
			{
				numwaiting++;
			}
		}
	}

	if (difficulty == 0)
	{
		wantedbots = 0;
	}
	else
	{
		wantedbots = pmax - numplayers - numwaiting;

		if (wantedbots < 0)
		{
			wantedbots = 0;
		}
	}

	if (numbots < wantedbots)
	{
		// We require MORE bots!
		UINT8 newplayernum = 0;
		boolean usedallskins = false;

		if (dedicated)
		{
			newplayernum = 1;
		}

		while (numbots < wantedbots)
		{
			UINT8 skin = M_RandomKey(numskins);

			if (usedallskins == false)
			{
				UINT8 loops = 0;

				while (!skinusable[skin])
				{
					if (loops >= numskins)
					{
						// no more skins, stick to our first choice
						usedallskins = true;
						break;
					}

					skin++;

					if (skin >= numskins)
					{
						skin = 0;
					}

					loops++;
				}
			}

			if (!K_AddBot(skin, difficulty, &newplayernum))
			{
				// Not enough player slots to add the bot, break the loop.
				break;
			}

			skinusable[skin] = false;
			numbots++;
		}
	}
	else if (numbots > wantedbots)
	{
		UINT8 buf[2];

		i = MAXPLAYERS;

		while (numbots > wantedbots && i > 0)
		{
			if (playeringame[i] && players[i].bot)
			{
				buf[0] = i;
				buf[1] = KR_LEAVE;
				SendNetXCmd(XD_REMOVEPLAYER, &buf, 2);

				numbots--;
			}

			i--;
		}
	}

	// We should have enough bots now :)
}

/*--------------------------------------------------
	boolean K_PlayerUsesBotMovement(player_t *player)

		See header file for description.
--------------------------------------------------*/
boolean K_PlayerUsesBotMovement(player_t *player)
{
	if (player->bot || player->exiting || player->quittime)
		return true;

	return false;
}

/*--------------------------------------------------
	boolean K_BotCanTakeCut(player_t *player)

		See header file for description.
--------------------------------------------------*/
boolean K_BotCanTakeCut(player_t *player)
{
	if (!K_ApplyOffroad(player)
		|| player->kartstuff[k_itemtype] == KITEM_SNEAKER
		|| player->kartstuff[k_itemtype] == KITEM_ROCKETSNEAKER
		|| player->kartstuff[k_itemtype] == KITEM_INVINCIBILITY
		|| player->kartstuff[k_itemtype] == KITEM_HYUDORO)
		return true;

	return false;
}

/*--------------------------------------------------
	static UINT32 K_BotRubberbandDistance(player_t *player)

		Calculates the distance away from 1st place that the
		bot should rubberband to.

	Input Arguments:-
		player - Player to compare.

	Return:-
		Distance to add, as an integer.
--------------------------------------------------*/
static UINT32 K_BotRubberbandDistance(player_t *player)
{
	const UINT32 spacing = FixedDiv(512 * FRACUNIT, K_GetKartGameSpeedScalar(gamespeed)) / FRACUNIT;
	const UINT8 portpriority = player - players;
	UINT8 pos = 0;
	UINT8 i;

	if (player->botvars.rival)
	{
		// The rival should always try to be the front runner for the race.
		return 0;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (i == portpriority)
		{
			continue;
		}

		if (playeringame[i] && players[i].bot)
		{
			// First check difficulty levels, then score, then settle it with port priority!
			if (player->botvars.difficulty < players[i].botvars.difficulty)
			{
				pos += 3;
			}
			else if (player->score < players[i].score)
			{
				pos += 2;
			}
			else if (i < portpriority)
			{
				pos += 1;
			}
		}
	}

	return (pos * spacing);
}

/*--------------------------------------------------
	fixed_t K_BotRubberband(player_t *player)

		See header file for description.
--------------------------------------------------*/
fixed_t K_BotRubberband(player_t *player)
{
	fixed_t rubberband = FRACUNIT;
	fixed_t max, min;
	player_t *firstplace = NULL;
	UINT8 i;

	if (player->exiting)
	{
		// You're done, we don't need to rubberband anymore.
		return FRACUNIT;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

#if 0
		// Only rubberband up to players.
		if (players[i].bot)
		{
			continue;
		}
#endif

		if (firstplace == NULL || players[i].distancetofinish < firstplace->distancetofinish)
		{
			firstplace = &players[i];
		}
	}

	if (firstplace != NULL)
	{
		const UINT32 wanteddist = firstplace->distancetofinish + K_BotRubberbandDistance(player);
		const INT32 distdiff = player->distancetofinish - wanteddist;

		if (wanteddist > player->distancetofinish)
		{
			// Whoa, you're too far ahead! Slow back down a little.
			rubberband += (MAXBOTDIFFICULTY - player->botvars.difficulty) * (distdiff / 3);
		}
		else
		{
			// Catch up to your position!
			rubberband += (2*player->botvars.difficulty) * distdiff;
		}
	}

	// Lv. 1: x1.0 max
	// Lv. 5: x1.5 max
	// Lv. 9: x2.0 max
	max = FRACUNIT + ((FRACUNIT * (player->botvars.difficulty - 1)) / (MAXBOTDIFFICULTY - 1));

	// Lv. 1: x0.75 min
	// Lv. 5: x0.875 min
	// Lv. 9: x1.0 min
	min = FRACUNIT - (((FRACUNIT/4) * (MAXBOTDIFFICULTY - player->botvars.difficulty)) / (MAXBOTDIFFICULTY - 1));

	if (rubberband > max)
	{
		rubberband = max;
	}
	else if (rubberband < min)
	{
		rubberband = min;
	}

	return rubberband;
}

/*--------------------------------------------------
	fixed_t K_BotTopSpeedRubberband(player_t *player)

		See header file for description.
--------------------------------------------------*/
fixed_t K_BotTopSpeedRubberband(player_t *player)
{
	fixed_t rubberband = K_BotRubberband(player);

	if (rubberband < FRACUNIT)
	{
		// Never go below your regular top speed
		rubberband = FRACUNIT;
	}

	// Only allow you to go faster than your regular top speed if you're facing the right direction
	if (rubberband > FRACUNIT && player->mo != NULL && player->nextwaypoint != NULL)
	{
		const INT16 mindiff = 30;
		const INT16 maxdiff = 60;
		INT16 anglediff = 0;
		fixed_t amt = rubberband - FRACUNIT;
		angle_t destangle = R_PointToAngle2(
			player->mo->x, player->mo->y,
			player->nextwaypoint->mobj->x, player->nextwaypoint->mobj->y
		);
		angle_t angle = player->mo->angle - destangle;

		if (angle < ANGLE_180)
		{
			anglediff = AngleFixed(angle) >> FRACBITS;
		}
		else 
		{
			anglediff = 360 - (AngleFixed(angle) >> FRACBITS);
		}

		anglediff = abs(anglediff);

		if (anglediff >= maxdiff)
		{
			rubberband = FRACUNIT;
		}
		else if (anglediff > mindiff)
		{
			amt = (amt * (maxdiff - anglediff)) / mindiff;
			rubberband = FRACUNIT + amt;
		}
	}

	return rubberband;
}

/*--------------------------------------------------
	fixed_t K_BotFrictionRubberband(player_t *player, fixed_t frict)

		See header file for description.
--------------------------------------------------*/
fixed_t K_BotFrictionRubberband(player_t *player, fixed_t frict)
{
	fixed_t rubberband = K_BotRubberband(player) - FRACUNIT;
	fixed_t newfrict;

	if (rubberband <= 0)
	{
		// Never get stronger than normal friction
		return frict;
	}

	newfrict = FixedDiv(frict, FRACUNIT + (rubberband / 2));

	if (newfrict < 0)
		newfrict = 0;
	if (newfrict > FRACUNIT)
		newfrict = FRACUNIT;

	return newfrict;
}

/*--------------------------------------------------
	fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy)

		See header file for description.
--------------------------------------------------*/
fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy)
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

/*--------------------------------------------------
	static botprediction_t *K_CreateBotPrediction(player_t *player)

		Calculates a point further along the track to attempt to drive towards.

	Input Arguments:-
		player - Player to compare.

	Return:-
		Bot prediction struct.
--------------------------------------------------*/
static botprediction_t *K_CreateBotPrediction(player_t *player)
{
	const INT16 handling = K_GetKartTurnValue(player, KART_FULLTURN); // Reduce prediction based on how fast you can turn
	const INT16 normal = KART_FULLTURN; // "Standard" handling to compare to

	const fixed_t distreduce = K_BotReducePrediction(player);
	const fixed_t radreduce = min(distreduce + FRACUNIT/4, FRACUNIT);

	const tic_t futuresight = (TICRATE * normal) / max(1, handling); // How far ahead into the future to try and predict
	const fixed_t speed = max(P_AproxDistance(player->mo->momx, player->mo->momy), K_GetKartSpeed(player, false) / 4);
	const INT32 distance = (FixedMul(speed, distreduce) / FRACUNIT) * futuresight;

	botprediction_t *predict = Z_Calloc(sizeof(botprediction_t), PU_LEVEL, NULL);
	waypoint_t *wp = player->nextwaypoint;

	INT32 distanceleft = distance;
	fixed_t smallestradius = INT32_MAX;
	angle_t angletonext = ANGLE_MAX;

	size_t nwp;
	size_t i;

	// Reduce distance left by your distance to the starting waypoint.
	// This prevents looking too far ahead if the closest waypoint is really far away.
	distanceleft -= P_AproxDistance(player->mo->x - wp->mobj->x, player->mo->y - wp->mobj->y) / FRACUNIT;

	// We don't want to look ahead at all, just go to the first waypoint.
	if (distanceleft <= 0)
	{
		predict->x = wp->mobj->x;
		predict->y = wp->mobj->y;
		predict->radius = FixedMul(wp->mobj->radius, radreduce);
		return predict;
	}

	angletonext = R_PointToAngle2(
		player->mo->x, player->mo->y,
		wp->mobj->x, wp->mobj->y
	);

	// Go through waypoints until we've traveled the distance we wanted to predict ahead!
	while (distanceleft > 0)
	{
		INT32 disttonext = INT32_MAX;

		if (wp->mobj->radius < smallestradius)
		{
			smallestradius = wp->mobj->radius;
		}

		if (wp->numnextwaypoints == 0)
		{
			// Well, this is where I get off.
			distanceleft = 0;
			break;
		}

		// Calculate nextwaypoints index to use
		// nextwaypoints[0] by default
		nwp = 0;

		// There are multiple nextwaypoints,
		// so we need to find the most convenient one to us.
		// Let's compare the angle to the player's!
		if (wp->numnextwaypoints > 1)
		{
			angle_t delta = ANGLE_MAX;
			angle_t a     = ANGLE_MAX;

			for (i = 0; i < wp->numnextwaypoints; i++)
			{

				if (K_GetWaypointIsShortcut(wp->nextwaypoints[i]) && !K_BotCanTakeCut(player))
				{
					continue;
				}

				// Unlike the other parts of this function, we're comparing the player's physical position, NOT the position of the waypoint!!
				// This should roughly correspond with how players will think about path splits.
				a = R_PointToAngle2(
					player->mo->x, player->mo->y,
					wp->nextwaypoints[i]->mobj->x, wp->nextwaypoints[i]->mobj->y
				);
				if (a > ANGLE_180)
				{
					a = InvAngle(a);
				}

				a = player->mo->angle - a;

				if (a < delta)
				{
					nwp = i;
					delta = a;
				}
			}
		}
 
		angletonext = R_PointToAngle2(
			wp->mobj->x, wp->mobj->y,
			wp->nextwaypoints[nwp]->mobj->x, wp->nextwaypoints[nwp]->mobj->y
		);

		disttonext = (INT32)wp->nextwaypointdistances[nwp];

		if (disttonext > distanceleft)
		{
			break;
		}

		distanceleft -= disttonext;

		wp = wp->nextwaypoints[nwp];
	}

	// Set our predicted point's coordinates,
	// and use the smallest radius of all of the waypoints in the chain!
	predict->x = wp->mobj->x;
	predict->y = wp->mobj->y;
	predict->radius = FixedMul(smallestradius, radreduce);

	// Set the prediction coordinates between the 2 waypoints if there's still distance left.
	if (distanceleft > 0)
	{
		// Scaled with the leftover anglemul!
		predict->x += P_ReturnThrustX(NULL, angletonext, distanceleft * FRACUNIT);
		predict->y += P_ReturnThrustY(NULL, angletonext, distanceleft * FRACUNIT);
	}

	return predict;
}

/*--------------------------------------------------
	void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)

		See header file for description.
--------------------------------------------------*/
void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)
{
	botprediction_t *predict = NULL;
	INT32 turnamt = 0;

	// Can't build a ticcmd if we aren't spawned...
	if (!player->mo)
	{
		return;
	}

	// Remove any existing controls
	memset(cmd, 0, sizeof(ticcmd_t));

	if (gamestate != GS_LEVEL
		|| player->mo->scale <= 1) // funny post-finish death
	{
		// No need to do anything else.
		return;
	}

	if (player->playerstate == PST_DEAD)
	{
		cmd->buttons |= BT_ACCELERATE;
		return;
	}

	// Complete override of all ticcmd functionality
	if (LUAh_BotTiccmd(player, cmd))
		return;

	// Start boost handler
	if (leveltime <= starttime)
	{
		tic_t length = (TICRATE/6);
		tic_t boosthold = starttime - K_GetSpindashChargeTime(player);

		cmd->buttons |= BT_EBRAKEMASK;

		boosthold -= (MAXBOTDIFFICULTY - player->botvars.difficulty) * length;

		if (leveltime >= boosthold)
		{
			cmd->buttons |= BT_DRIFT;
		}

		return;
	}

	// Handle steering towards waypoints!
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
			cmd->forwardmove = -MAXPLMOVE;
			cmd->buttons |= BT_BRAKE;
		}
		else
		{
			const fixed_t playerwidth = (player->mo->radius * 2);
			const fixed_t realrad = predict->radius - (playerwidth * 4); // Remove a "safe" distance away from the edges of the road
			fixed_t rad = realrad;
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
				rad = FixedMul(rad, ((135 - anglediff) * FRACUNIT) / 135);
			}

			if (rad > realrad)
			{
				rad = realrad;
			}
			else if (rad < playerwidth)
			{
				rad = playerwidth;
			}

			cmd->buttons |= BT_ACCELERATE;

			// Full speed ahead!
			cmd->forwardmove = MAXPLMOVE;

			if (dirdist <= rad)
			{
				fixed_t speedmul = FixedDiv(player->speed, K_GetKartSpeed(player, false));
				fixed_t speedrad = rad/4;

				if (speedmul > FRACUNIT)
				{
					speedmul = FRACUNIT;
				}

				// Increase radius with speed
				// At low speed, the CPU will try to be more accurate
				// At high speed, they're more likely to lawnmower
				speedrad += FixedMul(speedmul, rad - speedrad);

				if (speedrad < playerwidth)
				{
					speedrad = playerwidth;
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

			if (anglediff > 60)
			{
				// Actually, don't go too fast...
				cmd->forwardmove /= 2;
				cmd->buttons |= BT_BRAKE;
			}
			else if (dirdist <= realrad)
			{
				// Steer towards/away from objects!
				turnamt += K_BotFindObjects(player, turnamt);
			}
		}
	}

	// Handle item usage
	K_BotItemUsage(player, cmd, turnamt);

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

		if (turnamt > 0)
		{
			if (player->botvars.turnconfirm < BOTTURNCONFIRM)
			{
				player->botvars.turnconfirm++;
			}
		}
		else if (turnamt < 0)
		{
			if (player->botvars.turnconfirm > -BOTTURNCONFIRM)
			{
				player->botvars.turnconfirm--;
			}
		}

		if (abs(player->botvars.turnconfirm) >= BOTTURNCONFIRM)
		{
			// You're commiting to your turn, you're allowed!
			cmd->turning = turnamt;
		}
	}

	// Free the prediction we made earlier
	if (predict != NULL)
	{
		Z_Free(predict);
	}
}

