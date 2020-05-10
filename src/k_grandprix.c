// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2011-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_grandprix.c
/// \brief Grand Prix mode specific code

#include "k_grandprix.h"
#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "k_bot.h"
#include "k_kart.h"
#include "m_random.h"
#include "r_things.h"

struct grandprixinfo grandprixinfo;

void K_InitGrandPrixBots(void)
{
	const UINT8 defaultbotskin = 9; // eggrobo

	// startingdifficulty: Easy = 3, Normal = 6, Hard = 9
	const UINT8 startingdifficulty = min(MAXBOTDIFFICULTY, (gamespeed + 1) * 3);
	UINT8 difficultylevels[MAXPLAYERS];

	UINT8 playercount = 8;
	UINT8 wantedbots = 0;

	UINT8 numplayers = 0;
	UINT8 competitors[4];

	boolean skinusable[MAXSKINS];
	UINT8 botskinlist[MAXPLAYERS];
	UINT8 botskinlistpos = 0;

	UINT8 newplayernum = 0;
	UINT8 i, j;

	memset(competitors, MAXPLAYERS, sizeof (competitors));
	memset(botskinlist, defaultbotskin, sizeof (botskinlist));

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

#if MAXPLAYERS != 16
	I_Error("GP bot difficulty levels need rebalacned for the new player count!\n");
#endif

	if (grandprixinfo.masterbots)
	{
		// Everyone is max difficulty!!
		memset(difficultylevels, MAXBOTDIFFICULTY, sizeof (difficultylevels));
	}
	else
	{
		// init difficulty levels list
		difficultylevels[0] = max(1, startingdifficulty);
		difficultylevels[1] = max(1, startingdifficulty-1);
		difficultylevels[2] = max(1, startingdifficulty-2);
		difficultylevels[3] = max(1, startingdifficulty-3);
		difficultylevels[4] = max(1, startingdifficulty-3);
		difficultylevels[5] = max(1, startingdifficulty-4);
		difficultylevels[6] = max(1, startingdifficulty-4);
		difficultylevels[7] = max(1, startingdifficulty-4);
		difficultylevels[8] = max(1, startingdifficulty-5);
		difficultylevels[9] = max(1, startingdifficulty-5);
		difficultylevels[10] = max(1, startingdifficulty-5);
		difficultylevels[11] = max(1, startingdifficulty-6);
		difficultylevels[12] = max(1, startingdifficulty-6);
		difficultylevels[13] = max(1, startingdifficulty-6);
		difficultylevels[14] = max(1, startingdifficulty-7);
		difficultylevels[15] = max(1, startingdifficulty-7);
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (numplayers < MAXSPLITSCREENPLAYERS && !players[i].spectator)
			{
				competitors[numplayers] = i;
				numplayers++;
			}
			else
			{
				players[i].spectator = true; // force spectate for all other players, if they happen to exist?
			}
		}
	}

	if (numplayers > 2)
	{
		// Add 3 bots per player beyond 2P
		playercount += (numplayers-2) * 3;
	}

	wantedbots = playercount - numplayers;

	// Create rival list
	if (numplayers > 0)
	{
		for (i = 0; i < SKINRIVALS; i++)
		{
			for (j = 0; j < numplayers; j++)
			{
				player_t *p = &players[competitors[j]];
				char *rivalname = skins[p->skin].rivals[i];
				SINT8 rivalnum = R_SkinAvailable(rivalname);

				if (rivalnum != -1 && skinusable[rivalnum])
				{
					botskinlist[botskinlistpos] = rivalnum;
					skinusable[rivalnum] = false;
					botskinlistpos++;
				}
			}
		}
	}

	// Pad the remaining list with random skins if we need to
	if (botskinlistpos < wantedbots)
	{
		for (i = botskinlistpos; i < wantedbots; i++)
		{
			UINT8 val = M_RandomKey(numskins);
			UINT8 loops = 0;

			while (!skinusable[val])
			{
				if (loops >= numskins)
				{
					// no more skins
					break;
				}

				val++;

				if (val >= numskins)
				{
					val = 0;
				}

				loops++;
			}

			if (loops >= numskins)
			{
				// leave the rest of the table as the default skin
				break;
			}

			botskinlist[i] = val;
			skinusable[val] = false;
		}
	}

	for (i = 0; i < wantedbots; i++)
	{
		if (!K_AddBot(botskinlist[i], difficultylevels[i], &newplayernum))
		{
			break;
		}
	}
}

void K_FakeBotResults(player_t *bot)
{
	const UINT32 distfactor = 32;
	UINT32 worstdist = 0;
	tic_t besttime = UINT32_MAX;
	UINT8 numplayers = 0;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			numplayers++;

			if (players[i].exiting && players[i].realtime < besttime)
			{
				besttime = players[i].realtime;
			}
		}
	}

	if (besttime == UINT32_MAX)
	{
		// No one finished, so you don't finish either.
		bot->pflags |= PF_TIMEOVER;
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			if (players[i].distancetofinish > worstdist)
			{
				worstdist = players[i].distancetofinish;
			}
		}
	}

	if (bot->distancetofinish >= worstdist)
	{
		// Last place, you aren't going to finish.
		bot->pflags |= PF_TIMEOVER;
		return;
	}

	// hey, you "won"
	bot->exiting = 2;
	bot->realtime = bot->realtime + (bot->distancetofinish / distfactor);
	bot->distancetofinish = 0;
}
