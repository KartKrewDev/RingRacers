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

void K_AddBots(UINT8 numbots)
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

void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)
{
	// Can't build a ticcmd if we aren't spawned...
	if (!player->mo)
		return;

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

	if (player->nextwaypoint != NULL && player->nextwaypoint->mobj != NULL && !P_MobjWasRemoved(player->nextwaypoint->mobj))
	{
		INT16 turnamt = KART_FULLTURN;
		SINT8 turnsign = 0;
		angle_t wpangle, moveangle, angle;
		INT16 anglediff;

		wpangle = R_PointToAngle2(player->mo->x, player->mo->y, player->nextwaypoint->mobj->x, player->nextwaypoint->mobj->y);
		moveangle = player->mo->angle;

		angle = (moveangle - wpangle);

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

		if (anglediff > 90)
		{
			// Wrong way!
			cmd->forwardmove = -25;
			cmd->buttons |= BT_BRAKE;
		}
		else
		{
			fixed_t rad = player->nextwaypoint->mobj->radius - (player->mo->radius*2);
			fixed_t dirdist = K_DistanceOfLineFromPoint(
				player->mo->x, player->mo->y,
				player->mo->x + FINECOSINE(moveangle >> ANGLETOFINESHIFT), player->mo->y + FINESINE(moveangle >> ANGLETOFINESHIFT),
				player->nextwaypoint->mobj->x, player->nextwaypoint->mobj->y
			);

			if (player == &players[displayplayers[0]])
				CONS_Printf("perpendicular dist: %d\n", dirdist / FRACUNIT);

			cmd->buttons |= BT_ACCELERATE;

			// Full speed ahead!
			cmd->forwardmove = 50;

			if (dirdist <= rad)
			{
				if (dirdist < rad/2)
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
			/*else
			{
				// Actually, don't go too fast...
				cmd->forwardmove /= 2;
				cmd->buttons |= BT_BRAKE;
			}*/
		}

		if (turnamt != 0)
		{
			cmd->driftturn = KART_FULLTURN * turnsign;
			cmd->angleturn += KART_FULLTURN * turnsign;
		}
	}
}

