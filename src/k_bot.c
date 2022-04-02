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
#include "k_race.h" // finishBeamLine
#include "k_boss.h"


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

	if (difficulty == 0 || !(gametyperules & GTR_BOTS) || bossinfo.boss == true)
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
			i--;

			if (playeringame[i] && players[i].bot)
			{
				buf[0] = i;
				buf[1] = KR_LEAVE;
				SendNetXCmd(XD_REMOVEPLAYER, &buf, 2);

				numbots--;
			}
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
	if (player->exiting)
		return true;

	if (player->bot)
		return true;

	return false;
}

/*--------------------------------------------------
	boolean K_BotCanTakeCut(player_t *player)

		See header file for description.
--------------------------------------------------*/
boolean K_BotCanTakeCut(player_t *player)
{
	if (
#if 1
		K_TripwirePass(player) == true
#else
		K_ApplyOffroad(player) == false
#endif
		|| player->itemtype == KITEM_SNEAKER
		|| player->itemtype == KITEM_ROCKETSNEAKER
		|| player->itemtype == KITEM_INVINCIBILITY
		)
	{
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static fixed_t K_BotSpeedScaled(player_t *player, fixed_t speed)

		Gets the bot's speed value, adjusted for predictions.
		Mainly to make bots brake earlier when on friction sectors.

	Input Arguments:-
		player - The bot player to calculate speed for.
		speed - Raw speed value.

	Return:-
		The bot's speed value for calculations.
--------------------------------------------------*/
static fixed_t K_BotSpeedScaled(player_t *player, fixed_t speed)
{
	fixed_t result = speed;

	if (player->mo->movefactor != FRACUNIT)
	{
		fixed_t moveFactor = player->mo->movefactor;

		if (moveFactor == 0)
		{
			moveFactor = 1;
		}

		// Reverse against friction. Allows for bots to
		// acknowledge they'll be moving faster on ice,
		// and to steer harder / brake earlier.
		moveFactor = FixedDiv(FRACUNIT, moveFactor);

		// The full value is way too strong, reduce it.
		moveFactor -= (moveFactor - FRACUNIT)*3/4;

		result = FixedMul(result, moveFactor);
	}

	if (player->mo->standingslope != NULL)
	{
		const pslope_t *slope = player->mo->standingslope;

		if (!(slope->flags & SL_NOPHYSICS) && abs(slope->zdelta) >= FRACUNIT/21)
		{
			fixed_t slopeMul = FRACUNIT;
			angle_t angle = K_MomentumAngle(player->mo) - slope->xydirection;

			if (P_MobjFlip(player->mo) * slope->zdelta < 0)
				angle ^= ANGLE_180;

			// Going uphill: 0
			// Going downhill: FRACUNIT*2
			slopeMul = FRACUNIT + FINECOSINE(angle >> ANGLETOFINESHIFT);

			// Range: 0.9 to 1.1
			result = FixedMul(result, (FRACUNIT*9/10) + (slopeMul/10));
		}
	}

	return result;
}

/*--------------------------------------------------
	static line_t *K_FindBotController(mobj_t *mo)

		Finds if any bot controller linedefs are tagged to the bot's sector.

	Input Arguments:-
		mo - The bot player's mobj.

	Return:-
		Linedef of the bot controller. NULL if it doesn't exist.
--------------------------------------------------*/
static line_t *K_FindBotController(mobj_t *mo)
{
	msecnode_t *node;
	ffloor_t *rover;
	INT16 lineNum = -1;
	mtag_t tag;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
		{
			continue;
		}

		tag = Tag_FGet(&node->m_sector->tags);
		lineNum = P_FindSpecialLineFromTag(2004, tag, -1); // todo: needs to not use P_FindSpecialLineFromTag

		if (lineNum != -1)
		{
			break;
		}

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			sector_t *rs = NULL;

			if (!(rover->flags & FF_EXISTS))
			{
				continue;
			}

			if (mo->z > *rover->topheight || mo->z + mo->height < *rover->bottomheight)
			{
				continue;
			}

			rs = &sectors[rover->secnum];
			tag = Tag_FGet(&rs->tags);
			lineNum = P_FindSpecialLineFromTag(2004, tag, -1);

			if (lineNum != -1)
			{
				break;
			}
		}
	}

	if (lineNum != -1)
	{
		return &lines[lineNum];
	}
	else
	{
		return NULL;
	}
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
	const UINT32 spacing = FixedDiv(640 * FRACUNIT, K_GetKartGameSpeedScalar(gamespeed)) / FRACUNIT;
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
	line_t *botController = NULL;
	UINT8 i;

	if (player->exiting)
	{
		// You're done, we don't need to rubberband anymore.
		return FRACUNIT;
	}

	botController = K_FindBotController(player->mo);

	if (botController != NULL)
	{
		// No Climb Flag: Disable rubberbanding
		if (botController->flags & ML_NOCLIMB)
		{
			return FRACUNIT;
		}
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
			rubberband += (DIFFICULTBOT - min(DIFFICULTBOT, player->botvars.difficulty)) * (distdiff / 3);
		}
		else
		{
			// Catch up to your position!
			rubberband += player->botvars.difficulty * distdiff;
		}
	}

	// Lv.   1: x1.0 max
	// Lv.   5: x1.4 max
	// Lv.   9: x1.8 max
	// Lv. MAX: x2.2 max
	max = FRACUNIT + ((FRACUNIT * (player->botvars.difficulty - 1)) / 10);

	// Lv.   1: x0.75 min
	// Lv.   5: x0.875 min
	// Lv.   9: x1.0 min
	// Lv. MAX: x1.0 min
	min = FRACUNIT - (((FRACUNIT/4) * (DIFFICULTBOT - min(DIFFICULTBOT, player->botvars.difficulty))) / (DIFFICULTBOT - 1));

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

	if (rubberband <= FRACUNIT)
	{
		// Never go below your regular top speed
		rubberband = FRACUNIT;
	}
	else
	{
		// Max at +20% for level 9 bots
		rubberband = FRACUNIT + ((rubberband - FRACUNIT) / 5);
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
	const fixed_t value = 20776;
	fixed_t rubberband = K_BotRubberband(player) - FRACUNIT;
	fixed_t newFrict = frict;

	if (rubberband <= 0)
	{
		// Never get weaker than normal friction
		return frict;
	}

	if (player->tiregrease > 0)
	{
		// Bots will lose all of their momentum without this.
		return frict;
	}

	newFrict = frict - FixedMul(value, rubberband);

	if (newFrict < 0)
		newFrict = 0;
	if (newFrict > FRACUNIT)
		newFrict = FRACUNIT;

	return newFrict;
}

/*--------------------------------------------------
	fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy)

		See header file for description.
--------------------------------------------------*/
fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t px, fixed_t py)
{
	// Copy+paste from P_ClosestPointOnLine :pensive:
	fixed_t startx = v1x;
	fixed_t starty = v1y;
	fixed_t dx = v2x - v1x;
	fixed_t dy = v2y - v1y;

	fixed_t cx, cy;
	fixed_t vx, vy;
	fixed_t magnitude;
	fixed_t t;

	cx = px - startx;
	cy = py - starty;

	vx = dx;
	vy = dy;

	magnitude = R_PointToDist2(v2x, v2y, startx, starty);
	vx = FixedDiv(vx, magnitude);
	vy = FixedDiv(vy, magnitude);

	t = (FixedMul(vx, cx) + FixedMul(vy, cy));

	vx = FixedMul(vx, t);
	vy = FixedMul(vy, t);

	return R_PointToDist2(px, py, startx + vx, starty + vy);
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
	// Stair janking makes it harder to steer, so attempt to steer harder.
	const UINT8 jankDiv = (player->stairjank > 0 ? 2 : 1);

	const INT16 handling = K_GetKartTurnValue(player, KART_FULLTURN) / jankDiv; // Reduce prediction based on how fast you can turn
	const INT16 normal = KART_FULLTURN; // "Standard" handling to compare to

	const tic_t futuresight = (TICRATE * normal) / max(1, handling); // How far ahead into the future to try and predict
	const fixed_t speed = K_BotSpeedScaled(player, P_AproxDistance(player->mo->momx, player->mo->momy));

	const INT32 startDist = (768 * mapobjectscale) / FRACUNIT;
	const INT32 distance = ((speed / FRACUNIT) * futuresight) + startDist;

	botprediction_t *predict = Z_Calloc(sizeof(botprediction_t), PU_STATIC, NULL);
	waypoint_t *wp = player->nextwaypoint;

	INT32 distanceleft = distance;
	fixed_t smallestradius = INT32_MAX;
	angle_t angletonext = ANGLE_MAX;

	// Halves radius when encountering a wall on your way to your destination.
	fixed_t radreduce = FRACUNIT;

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
		predict->radius = wp->mobj->radius;
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
				if (K_GetWaypointIsEnabled(wp->nextwaypoints[i]) == false)
				{
					continue;
				}

				if (K_GetWaypointIsShortcut(wp->nextwaypoints[i]) == true && K_BotCanTakeCut(player) == false)
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

		if (P_TraceBotTraversal(player->mo, wp->nextwaypoints[nwp]->mobj) == false)
		{
			// If we can't get a direct path to this waypoint, we don't want to check much further...
			disttonext *= 2;
			radreduce = FRACUNIT/2;
		}

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
	static UINT8 K_TrySpindash(player_t *player)

		Determines conditions where the bot should attempt to spindash.

	Input Arguments:-
		player - Bot player to check.

	Return:-
		0 to make the bot drive normally, 1 to e-brake, 2 to e-brake & charge spindash.
		(TODO: make this an enum)
--------------------------------------------------*/
static UINT8 K_TrySpindash(player_t *player)
{
	const tic_t difficultyModifier = (TICRATE/6);

	const fixed_t oldSpeed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);
	const fixed_t baseAccel = K_GetNewSpeed(player) - oldSpeed;
	const fixed_t speedDiff = player->speed - player->lastspeed;

	if (player->spindashboost || player->tiregrease)
	{
		// You just released a spindash, you don't need to try again yet, jeez.
		player->botvars.spindashconfirm = 0;
		return 0;
	}

	// Try "start boosts" first
	if (leveltime == starttime)
	{
		// Forces them to release, even if they haven't fully charged.
		// Don't want them to keep charging if they didn't have time to.
		return 0;
	}

	if (leveltime < starttime)
	{
		INT32 boosthold = starttime - K_GetSpindashChargeTime(player);

		boosthold -= (DIFFICULTBOT - min(DIFFICULTBOT, player->botvars.difficulty)) * difficultyModifier;

		if (leveltime >= (unsigned)boosthold)
		{
			// Start charging...
			return 2;
		}
		else
		{
			// Just hold your ground and e-brake.
			return 1;
		}
	}

	// Logic for normal racing.
	if (player->flashing > 0)
	{
		// Don't bother trying to spindash.
		// Trying to spindash while flashing is fine during POSITION, but not during the actual race.
		return 0;
	}

	if (speedDiff < (baseAccel / 4))
	{
		if (player->botvars.spindashconfirm < BOTSPINDASHCONFIRM)
		{
			player->botvars.spindashconfirm++;
		}
	}
	else
	{
		if (player->botvars.spindashconfirm > 0)
		{
			player->botvars.spindashconfirm--;
		}
	}

	if (player->botvars.spindashconfirm >= BOTSPINDASHCONFIRM)
	{
		INT32 chargingPoint = (K_GetSpindashChargeTime(player) + difficultyModifier);

		// Release quicker the higher the difficulty is.
		// Sounds counter-productive, but that's actually the best strategy after the race has started.
		chargingPoint -= player->botvars.difficulty * difficultyModifier;

		if (player->spindash > chargingPoint)
		{
			// Time to release.
			return 0;
		}

		return 2;
	}

	// We're doing just fine, we don't need to spindash, thanks.
	return 0;
}

/*--------------------------------------------------
	static void K_DrawPredictionDebug(botprediction_t *predict, player_t *player)

		Draws objects to show where the viewpoint bot is trying to go.

	Input Arguments:-
		predict - The prediction to visualize.
		player - The bot player this prediction is for.

	Return:-
		None
--------------------------------------------------*/
static void K_DrawPredictionDebug(botprediction_t *predict, player_t *player)
{
	mobj_t *debugMobj = NULL;
	angle_t sideAngle = ANGLE_MAX;
	UINT8 i = UINT8_MAX;

	I_Assert(predict != NULL);
	I_Assert(player != NULL);
	I_Assert(player->mo != NULL && P_MobjWasRemoved(player->mo) == false);

	sideAngle = player->mo->angle + ANGLE_90;

	debugMobj = P_SpawnMobj(predict->x, predict->y, player->mo->z, MT_SPARK);
	P_SetMobjState(debugMobj, S_THOK);

	debugMobj->frame &= ~FF_TRANSMASK;
	debugMobj->frame |= FF_TRANS20|FF_FULLBRIGHT;

	debugMobj->color = SKINCOLOR_ORANGE;
	debugMobj->scale *= 2;

	debugMobj->tics = 2;

	for (i = 0; i < 2; i++)
	{
		mobj_t *radiusMobj = NULL;
		fixed_t radiusX = predict->x, radiusY = predict->y;

		if (i & 1)
		{
			radiusX -= FixedMul(predict->radius, FINECOSINE(sideAngle >> ANGLETOFINESHIFT));
			radiusY -= FixedMul(predict->radius, FINESINE(sideAngle >> ANGLETOFINESHIFT));
		}
		else
		{
			radiusX += FixedMul(predict->radius, FINECOSINE(sideAngle >> ANGLETOFINESHIFT));
			radiusY += FixedMul(predict->radius, FINESINE(sideAngle >> ANGLETOFINESHIFT));
		}

		radiusMobj = P_SpawnMobj(radiusX, radiusY, player->mo->z, MT_SPARK);
		P_SetMobjState(radiusMobj, S_THOK);

		radiusMobj->frame &= ~FF_TRANSMASK;
		radiusMobj->frame |= FF_TRANS20|FF_FULLBRIGHT;

		radiusMobj->color = SKINCOLOR_YELLOW;
		radiusMobj->scale /= 2;

		radiusMobj->tics = 2;
	}
}

/*--------------------------------------------------
	static void K_BotTrick(player_t *player, ticcmd_t *cmd, line_t *botController)

		Determines inputs for trick panels.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		botController - Linedef for the bot controller. 

	Return:-
		None
--------------------------------------------------*/
static void K_BotTrick(player_t *player, ticcmd_t *cmd, line_t *botController)
{
	// Trick panel state -- do nothing until a controller line is found, in which case do a trick.
	if (botController == NULL)
	{
		return;
	}

	if (player->trickpanel == 1)
	{
		INT32 type = (sides[botController->sidenum[0]].rowoffset / FRACUNIT);

		// Y Offset: Trick type
		switch (type)
		{
			case 1:
				cmd->turning = KART_FULLTURN;
				break;
			case 2:
				cmd->turning = -KART_FULLTURN;
				break;
			case 3:
				cmd->buttons |= BT_FORWARD;
				break;
			case 4:
				cmd->buttons |= BT_BACKWARD;
				break;
		}
	}
}

/*--------------------------------------------------
	static angle_t K_BotSmoothLanding(player_t *player, angle_t destangle)

		Calculates a new destination angle while in the air,
		to be able to successfully smooth land.

	Input Arguments:-
		player - Bot player to check.
		destangle - Previous destination angle.

	Return:-
		New destination angle.
--------------------------------------------------*/
static angle_t K_BotSmoothLanding(player_t *player, angle_t destangle)
{
	angle_t newAngle = destangle;
	boolean air = !P_IsObjectOnGround(player->mo);
	angle_t steepVal = air ? STUMBLE_STEEP_VAL_AIR : STUMBLE_STEEP_VAL;
	angle_t slopeSteep = max(AngleDelta(player->mo->pitch, 0), AngleDelta(player->mo->roll, 0));

	if (slopeSteep > steepVal)
	{
		fixed_t pitchMul = -FINESINE(destangle >> ANGLETOFINESHIFT);
		fixed_t rollMul = FINECOSINE(destangle >> ANGLETOFINESHIFT);
		angle_t testAngles[2];
		angle_t testDeltas[2];
		UINT8 i;

		testAngles[0] = R_PointToAngle2(0, 0, rollMul, pitchMul);
		testAngles[1] = R_PointToAngle2(0, 0, -rollMul, -pitchMul);

		for (i = 0; i < 2; i++)
		{
			testDeltas[i] = AngleDelta(testAngles[i], destangle);
		}

		if (testDeltas[1] < testDeltas[0])
		{
			return testAngles[1];
		}
		else
		{
			return testAngles[0];
		}
	}

	return newAngle;
}

/*--------------------------------------------------
	static INT32 K_HandleBotTrack(player_t *player, ticcmd_t *cmd, botprediction_t *predict)

		Determines inputs for standard track driving.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		predict - Pointer to the bot's prediction.

	Return:-
		New value for turn amount.
--------------------------------------------------*/
static INT32 K_HandleBotTrack(player_t *player, ticcmd_t *cmd, botprediction_t *predict, angle_t destangle)
{
	// Handle steering towards waypoints!
	INT32 turnamt = 0;
	SINT8 turnsign = 0;
	angle_t moveangle, angle;
	INT16 anglediff;

	I_Assert(predict != NULL);

	destangle = K_BotSmoothLanding(player, destangle);

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
		fixed_t realrad = predict->radius - (playerwidth * 4); // Remove a "safe" distance away from the edges of the road
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
			fixed_t speedmul = FixedDiv(K_BotSpeedScaled(player, player->speed), K_GetKartSpeed(player, false));
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
		}
	}

	return turnamt;
}

/*--------------------------------------------------
	static INT32 K_HandleBotReverse(player_t *player, ticcmd_t *cmd, botprediction_t *predict)

		Determines inputs for reversing.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		predict - Pointer to the bot's prediction.

	Return:-
		New value for turn amount.
--------------------------------------------------*/
static INT32 K_HandleBotReverse(player_t *player, ticcmd_t *cmd, botprediction_t *predict, angle_t destangle)
{
	// Handle steering towards waypoints!
	INT32 turnamt = 0;
	SINT8 turnsign = 0;
	angle_t moveangle, angle;
	INT16 anglediff, momdiff;

	if (predict != NULL)
	{
		// TODO: Should we reverse through bot controllers?
		return K_HandleBotTrack(player, cmd, predict, destangle);
	}

	if (player->nextwaypoint == NULL
		|| player->nextwaypoint->mobj == NULL
		|| P_MobjWasRemoved(player->nextwaypoint->mobj))
	{
		// No data available...
		return 0;
	}

	if ((player->nextwaypoint->prevwaypoints != NULL)
		&& (player->nextwaypoint->numprevwaypoints > 0U))
	{
		size_t i;
		for (i = 0U; i < player->nextwaypoint->numprevwaypoints; i++)
		{
			if (!K_GetWaypointIsEnabled(player->nextwaypoint->prevwaypoints[i]))
			{
				continue;
			}

			destangle = R_PointToAngle2(
				player->nextwaypoint->prevwaypoints[i]->mobj->x, player->nextwaypoint->prevwaypoints[i]->mobj->y,
				player->nextwaypoint->mobj->x, player->nextwaypoint->mobj->y
			);

			break;
		}
	}

	destangle = K_BotSmoothLanding(player, destangle);

	// Calculate turn direction first.
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

	// Now calculate momentum
	momdiff = 180;
	if (player->speed > player->mo->scale)
	{
		momdiff = 0;
		moveangle = K_MomentumAngle(player->mo);
		angle = (moveangle - destangle);

		if (angle < ANGLE_180)
		{
			momdiff = AngleFixed(angle)>>FRACBITS;
		}
		else 
		{
			momdiff = 360-(AngleFixed(angle)>>FRACBITS);
		}

		momdiff = abs(momdiff);
	}

	if (anglediff > 90 || momdiff < 90)
	{
		// We're not facing the track,
		// or we're going too fast.
		// Let's E-Brake.
		cmd->forwardmove = 0;
		cmd->buttons |= BT_ACCELERATE|BT_BRAKE;
	}
	else
	{
		fixed_t slopeMul = FRACUNIT;

		if (player->mo->standingslope != NULL)
		{
			const pslope_t *slope = player->mo->standingslope;

			if (!(slope->flags & SL_NOPHYSICS) && abs(slope->zdelta) >= FRACUNIT/21)
			{
				angle_t sangle = player->mo->angle - slope->xydirection;

				if (P_MobjFlip(player->mo) * slope->zdelta < 0)
					sangle ^= ANGLE_180;

				slopeMul = FRACUNIT - FINECOSINE(sangle >> ANGLETOFINESHIFT);
			}
		}

#define STEEP_SLOPE (FRACUNIT*11/10)
		if (slopeMul > STEEP_SLOPE)
		{
			// Slope is too steep to reverse -- EBrake.
			cmd->forwardmove = 0;
			cmd->buttons |= BT_ACCELERATE|BT_BRAKE;
		}
		else
		{
			cmd->forwardmove = -MAXPLMOVE;
			cmd->buttons |= BT_BRAKE; //|BT_LOOKBACK
		}
#undef STEEP_SLOPE

		if (anglediff < 10)
		{
			turnamt = 0;
		}
	}

	return turnamt;
}

/*--------------------------------------------------
	void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)

		See header file for description.
--------------------------------------------------*/
void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)
{
	botprediction_t *predict = NULL;
	boolean trySpindash = true;
	angle_t destangle = 0;
	UINT8 spindash = 0;
	INT32 turnamt = 0;
	line_t *botController = NULL;

	// Can't build a ticcmd if we aren't spawned...
	if (!player->mo)
	{
		return;
	}

	// Remove any existing controls
	memset(cmd, 0, sizeof(ticcmd_t));

	if (gamestate != GS_LEVEL
		|| player->mo->scale <= 1
		|| player->playerstate == PST_DEAD
		|| leveltime <= introtime
		|| !(gametyperules & GTR_BOTS))
	{
		// No need to do anything else.
		return;
	}

	// Complete override of all ticcmd functionality
	if (LUAh_BotTiccmd(player, cmd) == true)
	{
		return;
	}

	botController = K_FindBotController(player->mo);

	if (player->trickpanel != 0)
	{
		K_BotTrick(player, cmd, botController);

		// Don't do anything else.
		return;
	}

	destangle = player->mo->angle;

	if (botController != NULL && (botController->flags & ML_EFFECT1))
	{
		const fixed_t dist = (player->mo->radius * 4);

		// X Offset: Movement direction
		destangle = FixedAngle(sides[botController->sidenum[0]].textureoffset);

		// Overwritten prediction
		predict = Z_Calloc(sizeof(botprediction_t), PU_STATIC, NULL);

		predict->x = player->mo->x + FixedMul(dist, FINECOSINE(destangle >> ANGLETOFINESHIFT));
		predict->y = player->mo->y + FixedMul(dist, FINESINE(destangle >> ANGLETOFINESHIFT));
		predict->radius = (DEFAULT_WAYPOINT_RADIUS / 4) * mapobjectscale;
	}

	if (leveltime <= starttime && finishBeamLine != NULL)
	{
		// Handle POSITION!!
		const fixed_t distBase = 384*mapobjectscale;
		const fixed_t distAdjust = 64*mapobjectscale;

		const fixed_t closeDist = distBase + (distAdjust * (9 - player->kartweight));
		const fixed_t farDist = closeDist + (distAdjust * 2);

		const tic_t futureSight = (TICRATE >> 1);

		fixed_t distToFinish = K_DistanceOfLineFromPoint(
			finishBeamLine->v1->x, finishBeamLine->v1->y,
			finishBeamLine->v2->x, finishBeamLine->v2->y,
			player->mo->x, player->mo->y
		) - (K_BotSpeedScaled(player, player->speed) * futureSight);

		// Don't run the spindash code at all until we're in the right place
		trySpindash = false;

		if (distToFinish < closeDist)
		{
			// We're too close, we need to start backing up.
			turnamt = K_HandleBotReverse(player, cmd, predict, destangle);
		}
		else if (distToFinish < farDist)
		{
			INT32 bullyTurn = INT32_MAX;

			// We're in about the right place, let's do whatever we want to.

			if (player->kartspeed >= 5)
			{
				// Faster characters want to spindash.
				// Slower characters will use their momentum.
				trySpindash = true;
			}

			// Look for characters to bully.
			bullyTurn = K_PositionBully(player);
			if (bullyTurn == INT32_MAX)
			{
				// No one to bully, just go for a spindash as anyone.
				if (predict == NULL)
				{
					// Create a prediction.
					if (player->nextwaypoint != NULL
						&& player->nextwaypoint->mobj != NULL
						&& !P_MobjWasRemoved(player->nextwaypoint->mobj))
					{
						predict = K_CreateBotPrediction(player);
						K_NudgePredictionTowardsObjects(predict, player);
						destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
					}
				}

				turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
				cmd->buttons &= ~(BT_ACCELERATE|BT_BRAKE);
				cmd->forwardmove = 0;
				trySpindash = true;
			}
			else
			{
				turnamt = bullyTurn;

				// If already spindashing, wait until we get a relatively OK charge first.
				if (player->spindash > 0 && player->spindash <= TICRATE)
				{
					trySpindash = true;
				}
				else
				{
					trySpindash = false;
					cmd->buttons |= BT_ACCELERATE;
					cmd->forwardmove = MAXPLMOVE;
				}
			}
		}
		else
		{
			// Too far away, we need to just drive up.
			if (predict == NULL)
			{
				// Create a prediction.
				if (player->nextwaypoint != NULL
					&& player->nextwaypoint->mobj != NULL
					&& !P_MobjWasRemoved(player->nextwaypoint->mobj))
				{
					predict = K_CreateBotPrediction(player);
					K_NudgePredictionTowardsObjects(predict, player);
					destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
				}
			}

			turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
		}
	}
	else
	{
		// Handle steering towards waypoints!
		if (predict == NULL)
		{
			// Create a prediction.
			if (player->nextwaypoint != NULL
				&& player->nextwaypoint->mobj != NULL
				&& !P_MobjWasRemoved(player->nextwaypoint->mobj))
			{
				predict = K_CreateBotPrediction(player);
				K_NudgePredictionTowardsObjects(predict, player);
				destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
			}
		}

		turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
	}

	if (trySpindash == true)
	{
		// Spindashing
		spindash = K_TrySpindash(player);

		if (spindash > 0)
		{
			cmd->buttons |= BT_EBRAKEMASK;
			cmd->forwardmove = 0;

			if (spindash == 2 && player->speed < 6*mapobjectscale)
			{
				cmd->buttons |= BT_DRIFT;
			}
		}
	}

	if (spindash == 0)
	{
		// Don't pointlessly try to use rings/sneakers while charging a spindash.
		// TODO: Allowing projectile items like orbinaut while e-braking would be nice, maybe just pass in the spindash variable?
		K_BotItemUsage(player, cmd, turnamt);
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

		if (turnamt > 0)
		{
			// Count up
			if (player->botvars.turnconfirm < BOTTURNCONFIRM)
			{
				player->botvars.turnconfirm++;
			}
		}
		else if (turnamt < 0)
		{
			// Count down
			if (player->botvars.turnconfirm > -BOTTURNCONFIRM)
			{
				player->botvars.turnconfirm--;
			}
		}
		else
		{
			// Back to neutral
			if (player->botvars.turnconfirm < 0)
			{
				player->botvars.turnconfirm++;
			}
			else if (player->botvars.turnconfirm > 0)
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
		if (cv_kartdebugbotpredict.value != 0 && player - players == displayplayers[0])
		{
			K_DrawPredictionDebug(predict, player);
		}

		Z_Free(predict);
	}
}
