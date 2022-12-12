// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Kart Krew
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_roulette.c
/// \brief Item roulette code.

#include "k_roulette.h"

#include "d_player.h"
#include "doomdef.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "p_slopes.h"
#include "p_setup.h"
#include "r_draw.h"
#include "r_local.h"
#include "r_things.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_cond.h"
#include "f_finale.h"
#include "lua_hud.h"	// For Lua hud checks
#include "lua_hook.h"	// For MobjDamage and ShouldDamage
#include "m_cheat.h"	// objectplacing
#include "p_spec.h"

#include "k_kart.h"
#include "k_battle.h"
#include "k_boss.h"
#include "k_pwrlv.h"
#include "k_color.h"
#include "k_respawn.h"
#include "k_waypoint.h"
#include "k_bot.h"
#include "k_hud.h"
#include "k_terrain.h"
#include "k_director.h"
#include "k_collide.h"
#include "k_follower.h"
#include "k_objects.h"
#include "k_grandprix.h"
#include "k_specialstage.h"

// Magic number distance for use with item roulette tiers
#define DISTVAR (2048)

// Distance when SPB can start appearing
#define SPBSTARTDIST (8*DISTVAR)

// Distance when SPB is forced onto the next person who rolls an item
#define SPBFORCEDIST (14*DISTVAR)

// Distance when the game stops giving you bananas
#define ENDDIST (12*DISTVAR)

// Consistent seed used for item reels
#define ITEM_REEL_SEED (0x22D5FAA8)

static UINT8 K_KartItemOddsRace[NUMKARTRESULTS-1][8] =
{
	{ 0, 0, 2, 3, 4, 0, 0, 0 }, // Sneaker
	{ 0, 0, 0, 0, 0, 3, 4, 5 }, // Rocket Sneaker
	{ 0, 0, 0, 0, 2, 5, 5, 7 }, // Invincibility
	{ 2, 3, 1, 0, 0, 0, 0, 0 }, // Banana
	{ 1, 2, 0, 0, 0, 0, 0, 0 }, // Eggman Monitor
	{ 5, 5, 2, 2, 0, 0, 0, 0 }, // Orbinaut
	{ 0, 4, 2, 1, 0, 0, 0, 0 }, // Jawz
	{ 0, 3, 3, 2, 0, 0, 0, 0 }, // Mine
	{ 3, 0, 0, 0, 0, 0, 0, 0 }, // Land Mine
	{ 0, 0, 2, 2, 0, 0, 0, 0 }, // Ballhog
	{ 0, 0, 0, 0, 0, 2, 4, 0 }, // Self-Propelled Bomb
	{ 0, 0, 0, 0, 2, 5, 0, 0 }, // Grow
	{ 0, 0, 0, 0, 0, 2, 4, 2 }, // Shrink
	{ 1, 0, 0, 0, 0, 0, 0, 0 }, // Lightning Shield
	{ 0, 1, 2, 1, 0, 0, 0, 0 }, // Bubble Shield
	{ 0, 0, 0, 0, 0, 1, 3, 5 }, // Flame Shield
	{ 3, 0, 0, 0, 0, 0, 0, 0 }, // Hyudoro
	{ 0, 0, 0, 0, 0, 0, 0, 0 }, // Pogo Spring
	{ 2, 1, 1, 0, 0, 0, 0, 0 }, // Super Ring
	{ 0, 0, 0, 0, 0, 0, 0, 0 }, // Kitchen Sink
	{ 3, 0, 0, 0, 0, 0, 0, 0 }, // Drop Target
	{ 0, 0, 0, 3, 5, 0, 0, 0 }, // Garden Top
	{ 0, 0, 2, 2, 2, 0, 0, 0 }, // Sneaker x2
	{ 0, 0, 0, 0, 4, 4, 4, 0 }, // Sneaker x3
	{ 0, 1, 1, 0, 0, 0, 0, 0 }, // Banana x3
	{ 0, 0, 1, 0, 0, 0, 0, 0 }, // Orbinaut x3
	{ 0, 0, 0, 2, 0, 0, 0, 0 }, // Orbinaut x4
	{ 0, 0, 1, 2, 1, 0, 0, 0 }  // Jawz x2
};

static UINT8 K_KartItemOddsBattle[NUMKARTRESULTS][2] =
{
	//K  L
	{ 2, 1 }, // Sneaker
	{ 0, 0 }, // Rocket Sneaker
	{ 4, 1 }, // Invincibility
	{ 0, 0 }, // Banana
	{ 1, 0 }, // Eggman Monitor
	{ 8, 0 }, // Orbinaut
	{ 8, 1 }, // Jawz
	{ 6, 1 }, // Mine
	{ 2, 0 }, // Land Mine
	{ 2, 1 }, // Ballhog
	{ 0, 0 }, // Self-Propelled Bomb
	{ 2, 1 }, // Grow
	{ 0, 0 }, // Shrink
	{ 4, 0 }, // Lightning Shield
	{ 1, 0 }, // Bubble Shield
	{ 1, 0 }, // Flame Shield
	{ 2, 0 }, // Hyudoro
	{ 3, 0 }, // Pogo Spring
	{ 0, 0 }, // Super Ring
	{ 0, 0 }, // Kitchen Sink
	{ 2, 0 }, // Drop Target
	{ 4, 0 }, // Garden Top
	{ 0, 0 }, // Sneaker x2
	{ 0, 1 }, // Sneaker x3
	{ 0, 0 }, // Banana x3
	{ 2, 0 }, // Orbinaut x3
	{ 1, 1 }, // Orbinaut x4
	{ 5, 1 }  // Jawz x2
};

static kartitems_t K_KartItemReelTimeAttack[] =
{
	KITEM_SNEAKER,
	KITEM_SUPERRING,
	KITEM_NONE
};

static kartitems_t K_KartItemReelBreakTheCapsules[] =
{
	KRITEM_TRIPLEORBINAUT,
	KITEM_BANANA,
	KITEM_NONE
};

#if 0
static kartitems_t K_KartItemReelBoss[] =
{
	KITEM_ORBINAUT,
	KITEM_BANANA,
	KITEM_NONE
};
#endif

boolean K_ItemEnabled(SINT8 item)
{
	if (item < 1 || item >= NUMKARTRESULTS)
	{
		// Not a real item.
		return false;
	}

	if (K_CanChangeRules(true) == false)
	{
		// Force all items to be enabled.
		return true;
	}

	// Allow the user preference.
	return cv_items[item - 1].value;
}

fixed_t K_ItemOddsScale(UINT8 playerCount)
{
	const UINT8 basePlayer = 8; // The player count we design most of the game around.
	fixed_t playerScaling = 0;

	if (playerCount < 2)
	{
		// Cap to 1v1 scaling
		playerCount = 2;
	}

	// Then, it multiplies it further if the player count isn't equal to basePlayer.
	// This is done to make low player count races more interesting and high player count rates more fair.
	if (playerCount < basePlayer)
	{
		// Less than basePlayer: increase odds significantly.
		// 2P: x2.5
		playerScaling = (basePlayer - playerCount) * (FRACUNIT / 4);
	}
	else if (playerCount > basePlayer)
	{
		// More than basePlayer: reduce odds slightly.
		// 16P: x0.75
		playerScaling = (basePlayer - playerCount) * (FRACUNIT / 32);
	}

	return playerScaling;
}

UINT32 K_ScaleItemDistance(UINT32 distance, UINT8 numPlayers)
{
	if (mapobjectscale != FRACUNIT)
	{
		// Bring back to normal scale.
		distance = FixedDiv(distance, mapobjectscale);
	}

	if (franticitems == true)
	{
		// Frantic items pretends everyone's farther apart, for crazier items.
		distance = (15 * distance) / 14;
	}

	// Items get crazier with the fewer players that you have.
	distance = FixedMul(
		distance,
		FRACUNIT + (K_ItemOddsScale(numPlayers) / 2)
	);

	return distance;
}

UINT32 K_GetItemRouletteDistance(player_t *const player, UINT8 pingame)
{
	UINT32 pdis = 0;

#if 0
	if (specialStage.active == true)
	{
		UINT32 ufoDis = K_GetSpecialUFODistance();

		if (player->distancetofinish <= ufoDis)
		{
			// You're ahead of the UFO.
			pdis = 0;
		}
		else
		{
			// Subtract the UFO's distance from your distance!
			pdis = player->distancetofinish - ufoDis;
		}
	}
	else
#endif
	{
		UINT8 i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator
				&& players[i].position == 1)
			{
				// This player is first! Yay!

				if (player->distancetofinish <= players[i].distancetofinish)
				{
					// Guess you're in first / tied for first?
					pdis = 0;
				}
				else
				{
					// Subtract 1st's distance from your distance, to get your distance from 1st!
					pdis = player->distancetofinish - players[i].distancetofinish;
				}
				break;
			}
		}
	}

	pdis = K_ScaleItemDistance(pdis, pingame);

	if (player->bot && player->botvars.rival)
	{
		// Rival has better odds :)
		pdis = (15 * pdis) / 14;
	}

	return pdis;
}

/**	\brief	Item Roulette for Kart

	\param	player	player object passed from P_KartPlayerThink

	\return	void
*/

INT32 K_KartGetItemOdds(UINT8 pos, SINT8 item, UINT32 ourDist, boolean bot, boolean rival)
{
	fixed_t newOdds;
	INT32 i;

	UINT8 pingame = 0, pexiting = 0;

	player_t *first = NULL;
	player_t *second = NULL;

	UINT32 firstDist = UINT32_MAX;
	UINT32 secondDist = UINT32_MAX;
	UINT32 secondToFirst = 0;
	boolean isFirst = false;

	boolean powerItem = false;
	boolean cooldownOnStart = false;
	boolean notNearEnd = false;

	INT32 shieldType = KSHIELD_NONE;

	I_Assert(item > KITEM_NONE); // too many off by one scenarioes.
	I_Assert(cv_items[NUMKARTRESULTS-2] != NULL); // Make sure this exists

	if (K_ItemEnabled(item) == false)
	{
		return 0;
	}

	if (K_GetItemCooldown(item) > 0)
	{
		// Cooldown is still running, don't give another.
		return 0;
	}

	/*
	if (bot)
	{
		// TODO: Item use on bots should all be passed-in functions.
		// Instead of manually inserting these, it should return 0
		// for any items without an item use function supplied

		switch (item)
		{
			case KITEM_SNEAKER:
				break;
			default:
				return 0;
		}
	}
	*/
	(void)bot;

	if (gametype == GT_BATTLE)
	{
		I_Assert(pos < 2); // DO NOT allow positions past the bounds of the table
		newOdds = K_KartItemOddsBattle[item-1][pos];
	}
	else
	{
		I_Assert(pos < 8); // Ditto
		newOdds = K_KartItemOddsRace[item-1][pos];
	}

	newOdds <<= FRACBITS;

	shieldType = K_GetShieldFromItem(item);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (!(gametyperules & GTR_BUMPERS) || players[i].bumpers)
		{
			pingame++;
		}

		if (players[i].exiting)
		{
			pexiting++;
		}

		switch (shieldType)
		{
			case KSHIELD_NONE:
				/* Marble Garden Top is not REALLY
					a Sonic 3 shield */
			case KSHIELD_TOP:
			{
				break;
			}

			default:
			{
				if (shieldType == K_GetShieldFromItem(players[i].itemtype))
				{
					// Don't allow more than one of each shield type at a time
					return 0;
				}
			}
		}

		if (players[i].position == 1)
		{
			first = &players[i];
		}

		if (players[i].position == 2)
		{
			second = &players[i];
		}
	}

	if (first != NULL) // calculate 2nd's distance from 1st, for SPB
	{
		firstDist = first->distancetofinish;
		isFirst = (ourDist <= firstDist);
	}

	if (second != NULL)
	{
		secondDist = second->distancetofinish;
	}

	if (first != NULL && second != NULL)
	{
		secondToFirst = secondDist - firstDist;
		secondToFirst = K_ScaleItemDistance(secondToFirst, 16 - pingame); // Reversed scaling, so 16P is like 1v1, and 1v1 is like 16P
	}

	switch (item)
	{
		case KITEM_BANANA:
		case KITEM_EGGMAN:
		case KITEM_SUPERRING:
		{
			notNearEnd = true;
			break;
		}

		case KITEM_ROCKETSNEAKER:
		case KITEM_JAWZ:
		case KITEM_LANDMINE:
		case KITEM_DROPTARGET:
		case KITEM_BALLHOG:
		case KITEM_HYUDORO:
		case KRITEM_TRIPLESNEAKER:
		case KRITEM_TRIPLEORBINAUT:
		case KRITEM_QUADORBINAUT:
		case KRITEM_DUALJAWZ:
		{
			powerItem = true;
			break;
		}

		case KRITEM_TRIPLEBANANA:
		{
			powerItem = true;
			notNearEnd = true;
			break;
		}

		case KITEM_INVINCIBILITY:
		case KITEM_MINE:
		case KITEM_GROW:
		case KITEM_BUBBLESHIELD:
		case KITEM_FLAMESHIELD:
		{
			cooldownOnStart = true;
			powerItem = true;
			break;
		}

		case KITEM_SPB:
		{
			cooldownOnStart = true;
			notNearEnd = true;

			if (firstDist < ENDDIST*2 // No SPB when 1st is almost done
				|| isFirst == true) // No SPB for 1st ever
			{
				newOdds = 0;
			}
			else
			{
				const UINT32 dist = max(0, ((signed)secondToFirst) - SPBSTARTDIST);
				const UINT32 distRange = SPBFORCEDIST - SPBSTARTDIST;
				const UINT8 maxOdds = 20;
				fixed_t multiplier = (dist * FRACUNIT) / distRange;

				if (multiplier < 0)
				{
					multiplier = 0;
				}

				if (multiplier > FRACUNIT)
				{
					multiplier = FRACUNIT;
				}

				newOdds = FixedMul(maxOdds << FRACBITS, multiplier);
			}
			break;
		}

		case KITEM_SHRINK:
			cooldownOnStart = true;
			powerItem = true;
			notNearEnd = true;

			if (pingame-1 <= pexiting)
			{
				newOdds = 0;
			}
			break;

		case KITEM_LIGHTNINGSHIELD:
			cooldownOnStart = true;
			powerItem = true;

			if (spbplace != -1)
			{
				newOdds = 0;
			}
			break;

		default:
			break;
	}

	if (newOdds == 0)
	{
		// Nothing else we want to do with odds matters at this point :p
		return newOdds;
	}

	if ((cooldownOnStart == true) && (leveltime < (30*TICRATE)+starttime))
	{
		// This item should not appear at the beginning of a race. (Usually really powerful crowd-breaking items)
		newOdds = 0;
	}
	else if ((notNearEnd == true) && (ourDist < ENDDIST))
	{
		// This item should not appear at the end of a race. (Usually trap items that lose their effectiveness)
		newOdds = 0;
	}
	else if (powerItem == true)
	{
		// This item is a "power item". This activates "frantic item" toggle related functionality.
		if (franticitems == true)
		{
			// First, power items multiply their odds by 2 if frantic items are on; easy-peasy.
			newOdds *= 2;
		}

		if (rival == true)
		{
			// The Rival bot gets frantic-like items, also :p
			newOdds *= 2;
		}

		newOdds = FixedMul(newOdds, FRACUNIT + K_ItemOddsScale(pingame));
	}

	return FixedInt(FixedRound(newOdds));
}

//{ SRB2kart Roulette Code - Distance Based, yes waypoints

UINT8 K_FindUseodds(player_t *const player, UINT32 playerDist)
{
	UINT8 i;
	UINT8 useOdds = 0;
	UINT8 distTable[14];
	UINT8 distLen = 0;
	UINT8 totalSize = 0;
	boolean oddsValid[8];

	for (i = 0; i < 8; i++)
	{
		UINT8 j;

		if (gametype == GT_BATTLE && i > 1)
		{
			oddsValid[i] = false;
			break;
		}

		for (j = 1; j < NUMKARTRESULTS; j++)
		{
			if (K_KartGetItemOdds(
					i, j,
					player->distancetofinish,
					player->bot, (player->bot && player->botvars.rival)
				) > 0)
			{
				break;
			}
		}

		oddsValid[i] = (j < NUMKARTRESULTS);
	}

#define SETUPDISTTABLE(odds, num) \
	totalSize += num; \
	if (oddsValid[odds]) \
		for (i = num; i; --i) \
			distTable[distLen++] = odds;

	if (gametype == GT_BATTLE) // Battle Mode
	{
		useOdds = 0;
	}
	else
	{
		SETUPDISTTABLE(0,1);
		SETUPDISTTABLE(1,1);
		SETUPDISTTABLE(2,1);
		SETUPDISTTABLE(3,2);
		SETUPDISTTABLE(4,2);
		SETUPDISTTABLE(5,3);
		SETUPDISTTABLE(6,3);
		SETUPDISTTABLE(7,1);

		for (i = 0; i < totalSize; i++)
		{
			fixed_t pos = 0;
			fixed_t dist = 0;
			UINT8 index = 0;

			if (i == totalSize-1)
			{
				useOdds = distTable[distLen - 1];
				break;
			}

			pos = ((i << FRACBITS) * distLen) / totalSize;
			dist = FixedMul(DISTVAR << FRACBITS, pos) >> FRACBITS;
			index = FixedInt(FixedRound(pos));

			if (playerDist <= (unsigned)dist)
			{
				useOdds = distTable[index];
				break;
			}
		}
	}

#undef SETUPDISTTABLE

	return useOdds;
}

boolean K_ForcedSPB(player_t *const player)
{
	player_t *first = NULL;
	player_t *second = NULL;
	UINT32 secondToFirst = UINT32_MAX;
	UINT8 pingame = 0;
	UINT8 i;

	if (K_ItemEnabled(KITEM_SPB) == false)
	{
		return false;
	}

	if (!(gametyperules & GTR_CIRCUIT))
	{
		return false;
	}

	if (player->position <= 1)
	{
		return false;
	}

	if (spbplace != -1)
	{
		return false;
	}

	if (itemCooldowns[KITEM_SPB - 1] > 0)
	{
		return false;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (players[i].exiting)
		{
			return false;
		}

		pingame++;

		if (players[i].position == 1)
		{
			first = &players[i];
		}

		if (players[i].position == 2)
		{
			second = &players[i];
		}
	}

#if 0
	if (pingame <= 2)
	{
		return false;
	}
#endif

	if (first != NULL && second != NULL)
	{
		secondToFirst = second->distancetofinish - first->distancetofinish;
		secondToFirst = K_ScaleItemDistance(secondToFirst, 16 - pingame);
	}

	return (secondToFirst >= SPBFORCEDIST);
}

static void K_InitRoulette(itemroulette_t *const roulette)
{
	if (roulette->itemList == NULL)
	{
		roulette->itemListCap = 8;
		roulette->itemList = Z_Calloc(
			sizeof(SINT8) * roulette->itemListCap,
			PU_LEVEL,
			&roulette->itemList
		);
	}

	memset(roulette->itemList, KITEM_NONE, sizeof(SINT8) * roulette->itemListCap);
	roulette->itemListLen = 0;

	roulette->index = 0;
	roulette->elapsed = 0;
	roulette->tics = roulette->speed = 3; // Some default speed
	roulette->active = true;
}

static void K_PushToRouletteItemList(itemroulette_t *const roulette, kartitems_t item)
{
	I_Assert(roulette->itemList != NULL);

	if (roulette->itemListLen >= roulette->itemListCap)
	{
		roulette->itemListCap *= 2;
		roulette->itemList = Z_Realloc(
			roulette->itemList,
			sizeof(SINT8) * roulette->itemListCap,
			PU_LEVEL,
			&roulette->itemList
		);
	}

	roulette->itemList[ roulette->itemListLen ] = item;
	roulette->itemListLen++;
}

static void K_CalculateRouletteSpeed(player_t *const player, itemroulette_t *const roulette, UINT8 playing)
{
	// TODO: Change speed based on two factors:
	// - Get faster when your distancetofinish is closer to 1st place's distancetofinish. (winning)
	// - Get faster based on overall distancetofinish (race progress)
	// Slowest speed should be 12 tics, fastest should be 3 tics.

	(void)player;
	(void)playing;

	roulette->tics = roulette->speed = 7;
}

void K_StartItemRoulette(player_t *const player, itemroulette_t *const roulette)
{
	UINT8 playing = 0;
	UINT32 playerDist = UINT32_MAX;

	UINT8 useOdds = 0;
	UINT32 spawnChance[NUMKARTRESULTS] = {0};
	UINT32 totalSpawnChance = 0;
	size_t rngRoll = 0;

	size_t i;

	K_InitRoulette(roulette);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			continue;
		}

		playing++;
	}

	K_CalculateRouletteSpeed(player, roulette, playing);

	// SPECIAL CASE No. 1:
	// Give only the debug item if specified
	if (cv_kartdebugitem.value != KITEM_NONE)
	{
		K_PushToRouletteItemList(roulette, cv_kartdebugitem.value);
		return;
	}

	// SPECIAL CASE No. 2:
	// Use a special, pre-determined item reel for Time Attack / Free Play
	if (modeattacking || playing <= 1)
	{
		switch (gametype)
		{
			case GT_RACE:
			default:
			{
				for (i = 0; K_KartItemReelTimeAttack[i] != KITEM_NONE; i++)
				{
					K_PushToRouletteItemList(roulette, K_KartItemReelTimeAttack[i]);
					CONS_Printf("Added %d\n", K_KartItemReelTimeAttack[i]);
				}
				break;
			}
			case GT_BATTLE:
			{
				for (i = 0; K_KartItemReelBreakTheCapsules[i] != KITEM_NONE; i++)
				{
					K_PushToRouletteItemList(roulette, K_KartItemReelBreakTheCapsules[i]);
					CONS_Printf("Added %d\n", K_KartItemReelBreakTheCapsules[i]);
				}
				break;
			}
		}

		return;
	}

	// SPECIAL CASE No. 3:
	// Only give the SPB if conditions are right
	if (K_ForcedSPB(player) == true)
	{
		K_PushToRouletteItemList(roulette, KITEM_SPB);
		return;
	}

	playerDist = K_GetItemRouletteDistance(player, playing);

	useOdds = K_FindUseodds(player, playerDist);

	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		UINT8 thisItemsOdds = K_KartGetItemOdds(
			useOdds, i,
			player->distancetofinish,
			player->bot, (player->bot && player->botvars.rival)
		);

		spawnChance[i] = (totalSpawnChance += thisItemsOdds);
	}

	// SPECIAL CASE No. 4:
	// All items are off, so give a placeholder item
	if (totalSpawnChance == 0)
	{
		K_PushToRouletteItemList(roulette, KITEM_SAD);
		return;
	}

	// We always want the same result when making the same item reel.
	P_SetRandSeed(PR_ITEM_ROULETTE, ITEM_REEL_SEED);

	while (totalSpawnChance > 0)
	{
		rngRoll = P_RandomKey(PR_ITEM_ROULETTE, totalSpawnChance);

		for (i = 0; i < NUMKARTRESULTS && spawnChance[i] <= rngRoll; i++)
		{
			continue;
		}

		K_PushToRouletteItemList(roulette, i);

		// If we're in ring debt, pad out the reel with
		// a BUNCH of Super Rings.
		if (K_ItemEnabled(KITEM_SUPERRING)
			&& player->rings < 0
			&& !(gametyperules & GTR_SPHERES))
		{
			K_PushToRouletteItemList(roulette, KITEM_SUPERRING);
		}

		spawnChance[i]--;
		totalSpawnChance--;
	}
}

void K_StartEggmanRoulette(player_t *const player)
{
	itemroulette_t *const roulette = &player->itemRoulette;

	K_InitRoulette(roulette);
	K_PushToRouletteItemList(roulette, KITEM_EGGEXPLODE);
	roulette->eggman = true;
}

/**	\brief	Item Roulette for Kart

	\param	player		player
	\param	getitem		what item we're looking for

	\return	void
*/
static void K_KartGetItemResult(player_t *const player, kartitems_t getitem)
{
	if (getitem == KITEM_SPB || getitem == KITEM_SHRINK)
	{
		K_SetItemCooldown(getitem, 20*TICRATE);
	}

	player->botvars.itemdelay = TICRATE;
	player->botvars.itemconfirm = 0;

	player->itemtype = K_ItemResultToType(getitem);
	player->itemamount = K_ItemResultToAmount(getitem);
}

void K_KartItemRoulette(player_t *const player, ticcmd_t *const cmd)
{
	itemroulette_t *const roulette = &player->itemRoulette;
	boolean confirmItem = false;

	// This makes the roulette cycle through items.
	// If this isn't active, you shouldn't be here.
	if (roulette->active == false)
	{
		return;
	}

	if (roulette->itemList == NULL || roulette->itemListLen == 0)
	{
		// Invalid roulette setup.
		// Escape before we run into issues.
		roulette->active = false;
		return;
	}

	if (roulette->elapsed > TICRATE>>1) // Prevent accidental immediate item confirm
	{
		if (roulette->elapsed > TICRATE<<4)
		{
			// Waited way too long, forcefully confirm the item.
			confirmItem = true;
		}
		else
		{
			// We can stop our item when we choose.
			confirmItem = !!(cmd->buttons & BT_ATTACK);
		}
	}

	// If the roulette finishes or the player presses BT_ATTACK, stop the roulette and calculate the item.
	// I'm returning via the exact opposite, however, to forgo having another bracket embed. Same result either way, I think.
	// Finally, if you get past this check, now you can actually start calculating what item you get.
	if (confirmItem == true && (player->pflags & (PF_ITEMOUT|PF_EGGMANOUT|PF_USERINGS)) == 0)
	{
		kartitems_t finalItem = roulette->itemList[ roulette->index ];

		K_KartGetItemResult(player, finalItem);
		player->karthud[khud_itemblink] = TICRATE;

		if (P_IsDisplayPlayer(player) && !demo.freecam)
		{
			S_StartSound(NULL, sfx_itrolf);
		}

		// We're done, disable the roulette
		roulette->active = false;
		return;
	}

	roulette->elapsed++;

	if (roulette->tics == 0)
	{
		roulette->index = (roulette->index + 1) % roulette->itemListLen;
		roulette->tics = roulette->speed;

		// This makes the roulette produce the random noises.
		roulette->sound = (roulette->sound + 1) % 8;

		if (P_IsDisplayPlayer(player) && !demo.freecam)
		{
			S_StartSound(NULL, sfx_itrol1 + roulette->sound);
		}
	}
	else
	{
		roulette->tics--;
	}
}
