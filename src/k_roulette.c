// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
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
#include "k_hud.h" // distribution debugger
#include "m_easing.h"

// Magic number distance for use with item roulette tiers
#define DISTVAR (2048)

// Distance when SPB can start appearing
#define SPBSTARTDIST (8*DISTVAR)

// Distance when SPB is forced onto the next person who rolls an item
#define SPBFORCEDIST (16*DISTVAR)

// Distance when the game stops giving you bananas
#define ENDDIST (14*DISTVAR)

// Consistent seed used for item reels
#define ITEM_REEL_SEED (0x22D5FAA8)

#define FRANTIC_ITEM_SCALE (FRACUNIT*6/5)

#define ROULETTE_SPEED_SLOWEST (20)
#define ROULETTE_SPEED_FASTEST (2)
#define ROULETTE_SPEED_DIST (150*DISTVAR)
#define ROULETTE_SPEED_TIMEATTACK (9)
#define ROULETTE_SPEED_VERSUS_SLOWEST (12)

static UINT32 K_DynamicItemOddsRace[NUMKARTRESULTS-1][2] = 
{
	// distance, duplication tolerance
	{43, 9}, // sneaker
	{73, 12}, // rocketsneaker
	{70, 19}, // invincibility
	{18, 6}, // banana
	{17, 3}, // eggmark
	{21, 14}, // orbinaut
	{26, 7}, // jawz
	{29, 8}, // mine
	{10, 3}, // landmine
	{35, 4}, // ballhog
	{68, 6}, // selfpropelledbomb
	{58, 7}, // grow
	{71, 8}, // shrink
	{10, 1}, // lightningshield
	{30, 4}, // bubbleshield
	{76, 9}, // flameshield
	{10, 3}, // hyudoro
	{0, 0}, // pogospring
	{17, 4}, // superring
	{0, 0}, // kitchensink
	{10, 3}, // droptarget
	{53, 5}, // gardentop
	{0, 0}, // gachabom
	{44, 9}, // dualsneaker
	{61, 12}, // triplesneaker
	{25, 2}, // triplebanana
	{30, 1}, // tripleorbinaut
	{40, 2}, // quadorbinaut
	{40, 4}, // dualjawz
	{0, 0}, // triplegachabom
};

static UINT32 K_DynamicItemOddsBattle[NUMKARTRESULTS-1][2] = 
{
	// distance, duplication tolerance
	{20, 1}, // sneaker
	{0, 0}, // rocketsneaker
	{20, 1}, // invincibility
	{0, 0}, // banana
	{0, 0}, // eggmark
	{10, 2}, // orbinaut
	{12, 4}, // jawz
	{13, 3}, // mine
	{0, 0}, // landmine
	{13, 3}, // ballhog
	{0, 0}, // selfpropelledbomb
	{15, 2}, // grow
	{0, 0}, // shrink
	{0, 0}, // lightningshield
	{10, 1}, // bubbleshield
	{0, 0}, // flameshield
	{0, 0}, // hyudoro
	{0, 0}, // pogospring
	{0, 0}, // superring
	{0, 0}, // kitchensink
	{0, 0}, // droptarget
	{0, 0}, // gardentop
	{10, 5}, // gachabom
	{0, 0}, // dualsneaker
	{20, 1}, // triplesneaker
	{0, 0}, // triplebanana
	{10, 2}, // tripleorbinaut
	{13, 3}, // quadorbinaut
	{13, 3}, // dualjawz
	{10, 2}, // triplegachabom
};

static UINT32 K_DynamicItemOddsSpecial[NUMKARTRESULTS-1][2] = 
{
	// distance, duplication tolerance
	{15, 2}, // sneaker
	{0, 0}, // rocketsneaker
	{0, 0}, // invincibility
	{0, 0}, // banana
	{0, 0}, // eggmark
	{20, 3}, // orbinaut
	{15, 2}, // jawz
	{0, 0}, // mine
	{0, 0}, // landmine
	{0, 0}, // ballhog
	{70, 1}, // selfpropelledbomb
	{0, 0}, // grow
	{0, 0}, // shrink
	{0, 0}, // lightningshield
	{0, 0}, // bubbleshield
	{0, 0}, // flameshield
	{0, 0}, // hyudoro
	{0, 0}, // pogospring
	{0, 0}, // superring
	{0, 0}, // kitchensink
	{0, 0}, // droptarget
	{0, 0}, // gardentop
	{0, 0}, // gachabom
	{35, 2}, // dualsneaker
	{0, 0}, // triplesneaker
	{0, 0}, // triplebanana
	{35, 2}, // tripleorbinaut
	{0, 0}, // quadorbinaut
	{35, 2}, // dualjawz
	{0, 0}, // triplegachabom
};


static UINT8 K_KartLegacyBattleOdds[NUMKARTRESULTS-1][2] =
{
	{ 0, 1 }, // Sneaker
	{ 0, 0 }, // Rocket Sneaker
	{ 0, 1 }, // Invincibility
	{ 0, 0 }, // Banana
	{ 0, 0 }, // Eggman Monitor
	{ 2, 0 }, // Orbinaut
	{ 3, 1 }, // Jawz
	{ 2, 1 }, // Mine
	{ 0, 0 }, // Land Mine
	{ 2, 1 }, // Ballhog
	{ 0, 0 }, // Self-Propelled Bomb
	{ 1, 1 }, // Grow
	{ 0, 0 }, // Shrink
	{ 0, 0 }, // Lightning Shield
	{ 1, 0 }, // Bubble Shield
	{ 0, 0 }, // Flame Shield
	{ 0, 0 }, // Hyudoro
	{ 0, 0 }, // Pogo Spring
	{ 0, 0 }, // Super Ring
	{ 0, 0 }, // Kitchen Sink
	{ 0, 0 }, // Drop Target
	{ 0, 0 }, // Garden Top
	{ 5, 0 }, // Gachabom
	{ 0, 0 }, // Sneaker x2
	{ 0, 1 }, // Sneaker x3
	{ 0, 0 }, // Banana x3
	{ 2, 0 }, // Orbinaut x3
	{ 2, 1 }, // Orbinaut x4
	{ 2, 1 }, // Jawz x2
	{ 2, 0 }  // Gachabom x3
};

static kartitems_t K_KartItemReelSpecialEnd[] =
{
	KITEM_SUPERRING,
	KITEM_NONE
};

static kartitems_t K_KartItemReelRingSneaker[] =
{
	KITEM_SNEAKER,
	KITEM_SUPERRING,
	KITEM_NONE
};

static kartitems_t K_KartItemReelSPBAttack[] =
{
	KITEM_DROPTARGET,
	KITEM_SUPERRING,
	KITEM_NONE
};

static kartitems_t K_KartItemReelBreakTheCapsules[] =
{
	KITEM_GACHABOM,
	KRITEM_TRIPLEGACHABOM,
	KITEM_NONE
};

static kartitems_t K_KartItemReelBoss[] =
{
	KITEM_GACHABOM,
	KITEM_ORBINAUT,
	KITEM_ORBINAUT,
	KITEM_ORBINAUT,
	KITEM_ORBINAUT,
	KITEM_GACHABOM,
	KITEM_ORBINAUT,
	KITEM_ORBINAUT,
	KITEM_ORBINAUT,
	KITEM_NONE
};

static kartslotmachine_t K_KartItemReelRingBox[] =
{
	KSM_BAR,
	KSM_DOUBLEBAR,
	KSM_TRIPLEBAR,
	KSM_RING,
	KSM_SEVEN,
	KSM_JACKPOT,
	KSM__MAX
};

static sfxenum_t ringboxsound[] = 
{
	sfx_slot00,
	sfx_slot01,
	sfx_slot02,
	sfx_slot03,
	sfx_slot04,
	sfx_slot05
};

/*--------------------------------------------------
	boolean K_ItemEnabled(kartitems_t item)

		See header file for description.
--------------------------------------------------*/
boolean K_ItemEnabled(kartitems_t item)
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

/*--------------------------------------------------
	boolean K_ItemSingularity(kartitems_t item)

		See header file for description.
--------------------------------------------------*/
boolean K_ItemSingularity(kartitems_t item)
{
	switch (item)
	{
		case KITEM_SPB:
		case KITEM_SHRINK:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

/*--------------------------------------------------
	botItemPriority_e K_GetBotItemPriority(kartitems_t result)

		See header file for description.
--------------------------------------------------*/
botItemPriority_e K_GetBotItemPriority(kartitems_t result)
{
	result = K_ItemResultToType(result);

	switch (result)
	{
		case KITEM_SPB:
		{
			// Items that are intended to improve the game balance for everyone.
			return BOT_ITEM_PR_SPB;
		}
		case KITEM_INVINCIBILITY:
		case KITEM_GROW:
		case KITEM_SHRINK:
		case KITEM_LIGHTNINGSHIELD:
		case KITEM_BUBBLESHIELD:
		case KITEM_FLAMESHIELD:
		{
			// Items that drastically improve your own defense and/or speed.
			return BOT_ITEM_PR_POWER;
		}
		case KITEM_SUPERRING:
		{
			// Items that get you out of ring debt.
			return BOT_ITEM_PR_RINGDEBT;
		}
		case KITEM_SNEAKER:
		case KITEM_ROCKETSNEAKER:
		case KITEM_GARDENTOP:
		case KITEM_POGOSPRING:
		{
			// Used when not in 1st place and relatively far from players.
			// Items that give you speed with no protection.
			return BOT_ITEM_PR_SPEED;
		}
		case KITEM_HYUDORO:
		case KITEM_LANDMINE:
		case KITEM_DROPTARGET:
		case KITEM_EGGMAN:
		case KITEM_GACHABOM:
		case KITEM_KITCHENSINK:
		{
			// Used when in 1st place and relatively far from players.
			// Typically attack items that don't give you protection.
			return BOT_ITEM_PR_FRONTRUNNER;
		}
		case KITEM_ORBINAUT:
		case KITEM_BALLHOG:
		case KITEM_JAWZ:
		case KITEM_BANANA:
		case KITEM_MINE:
		{
			// Used in all other instances (close to other players, no priority override)
			// Typically attack items that give you protection.
			return BOT_ITEM_PR_NEUTRAL;
		}
		default:
		{
			return BOT_ITEM_PR__FALLBACK;
		}
	}
}

/*--------------------------------------------------
	static fixed_t K_ItemOddsScale(UINT8 playerCount)

		A multiplier for odds and distances to scale
		them with the player count.

	Input Arguments:-
		playerCount - Number of players in the game.

	Return:-
		Fixed point number, to multiply odds or
		distances by.
--------------------------------------------------*/
static fixed_t K_ItemOddsScale(UINT8 playerCount)
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

/*--------------------------------------------------
	static UINT32 K_UndoMapScaling(UINT32 distance)

		Takes a raw map distance and adjusts it to
		be in x1 scale.

	Input Arguments:-
		distance - Original distance.

	Return:-
		Distance unscaled by mapobjectscale.
--------------------------------------------------*/
static UINT32 K_UndoMapScaling(UINT32 distance)
{
	if (mapobjectscale != FRACUNIT)
	{
		// Bring back to normal scale.
		return FixedDiv(distance, mapobjectscale);
	}

	return distance;
}

/*--------------------------------------------------
	static UINT32 K_ScaleItemDistance(UINT32 distance, UINT8 numPlayers)

		Adjust item distance for lobby-size scaling
		as well as Frantic Items.

	Input Arguments:-
		player - The player to get the distance of.
		distance - Original distance.
		numPlayers - Number of players in the game.

	Return:-
		New distance after scaling.
--------------------------------------------------*/
static UINT32 K_ScaleItemDistance(const player_t* player, UINT32 distance, UINT8 numPlayers)
{
	if (franticitems == true)
	{
		// Frantic items pretends everyone's farther apart, for crazier items.
		distance = FixedMul(distance, FRANTIC_ITEM_SCALE);
	}

	// Items get crazier with the fewer players that you have.
	distance = FixedMul(
		distance,
		FRACUNIT + (K_ItemOddsScale(numPlayers) / 2)
	);

	// Distance is reduced based on the player's exp
	// distance = FixedMul(distance, player->exp);

	return distance;
}

/*--------------------------------------------------
	static UINT32 K_GetItemRouletteDistance(const player_t *player, UINT8 numPlayers)

		Gets a player's distance used for the item
		roulette, including all scaling factors.

	Input Arguments:-
		player - The player to get the distance of.
		numPlayers - Number of players in the game.

	Return:-
		The player's finalized item distance.
--------------------------------------------------*/
UINT32 K_GetItemRouletteDistance(const player_t *player, UINT8 numPlayers)
{
	UINT32 pdis = 0;

	if (player == NULL)
	{
		return 0;
	}

	if (specialstageinfo.valid == true)
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

	pdis = K_UndoMapScaling(pdis);
	pdis = K_ScaleItemDistance(player, pdis, numPlayers);

	if (player->bot && (player->botvars.rival || cv_levelskull.value))
	{
		// Rival has better odds :)
		pdis = FixedMul(pdis, FRANTIC_ITEM_SCALE);
	}

	return pdis;
}

/*--------------------------------------------------
	static boolean K_DenyShieldOdds(kartitems_t item)

		Checks if this type of shield already exists in
		another player's inventory.

	Input Arguments:-
		item - The item type of the shield.

	Return:-
		Whether this item is a shield and may not be awarded
		at this time.
--------------------------------------------------*/
static boolean K_DenyShieldOdds(kartitems_t item)
{
	const INT32 shieldType = K_GetShieldFromItem(item);
	size_t i;

	if ((gametyperules & GTR_CIRCUIT) == 0)
	{
		return false;
	}

	if (shieldType == KSHIELD_NONE)
	{
		return false;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			continue;
		}

		if (shieldType == K_GetShieldFromItem(players[i].itemtype))
		{
			// Don't allow more than one of each shield type at a time
			return true;
		}
	}

	return false;
}

static boolean K_DenyAutoRouletteOdds(kartitems_t item)
{
	// Deny items that are too hard for newbies
	switch (item)
	{
		case KITEM_GARDENTOP:
			return true;
		default:
			return false;
	}
}

/*--------------------------------------------------
	static fixed_t K_PercentSPBOdds(const itemroulette_t *roulette, UINT8 position)

	Provide odds of SPB according to distances of first and
	second place players.

	Input Arguments:-
		roulette - The roulette data that we intend to
			insert this item into.
		position - Position of player to consider for these
			odds.

	Return:-
		New item odds.
--------------------------------------------------*/
static fixed_t K_PercentSPBOdds(const itemroulette_t *roulette, UINT8 position)
{
	I_Assert(roulette != NULL);

	if (roulette->firstDist < ENDDIST*2 // No SPB when 1st is almost done
		|| position == 1) // No SPB for 1st ever
	{
		return 0;
	}
	else
	{
		const UINT32 dist = max(0, ((signed)roulette->secondToFirst) - SPBSTARTDIST);
		const UINT32 distRange = SPBFORCEDIST - SPBSTARTDIST;
		fixed_t multiplier = FixedDiv(dist, distRange);

		if (multiplier < 0)
		{
			multiplier = 0;
		}

		if (multiplier > FRACUNIT)
		{
			multiplier = FRACUNIT;
		}

		CONS_Printf("%d; %d / %d\n", leveltime, dist, roulette->secondToFirst);

		return multiplier;
	}
}


/*--------------------------------------------------
	INT32 K_KartGetBattleOdds(const player_t *player, UINT8 pos, kartitems_t item)

		See header file for description.
--------------------------------------------------*/

INT32 K_KartGetBattleOdds(const player_t *player, UINT8 pos, kartitems_t item)
{
	fixed_t newOdds = 0;

	I_Assert(item > KITEM_NONE); // too many off by one scenarioes.
	I_Assert(item < NUMKARTRESULTS);

	I_Assert(pos < 2); // DO NOT allow positions past the bounds of the table
	newOdds = K_KartLegacyBattleOdds[item-1][pos];

	newOdds <<= FRACBITS;

	return newOdds;
}

/*--------------------------------------------------
	static boolean K_ForcedSPB(const player_t *player, itemroulette_t *const roulette)

		Determines special conditions where we want
		to forcefully give the player an SPB.

	Input Arguments:-
		player - The player the roulette is for.
		roulette - The item roulette data.

	Return:-
		true if we want to give the player a forced SPB,
		otherwise false.
--------------------------------------------------*/
static boolean K_ForcedSPB(const player_t *player, itemroulette_t *const roulette)
{
	if (K_ItemEnabled(KITEM_SPB) == false)
	{
		return false;
	}

	if (!(gametyperules & GTR_CIRCUIT))
	{
		return false;
	}

	if (specialstageinfo.valid == true)
	{
		return false;
	}

	if (player == NULL)
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

#if 0
	if (roulette->playing <= 2)
	{
		return false;
	}
#endif

	return (roulette->secondToFirst >= SPBFORCEDIST);
}

/*--------------------------------------------------
	static void K_InitRoulette(itemroulette_t *const roulette)

		Initializes the data for a new item roulette.

	Input Arguments:-
		roulette - The item roulette data to initialize.

	Return:-
		N/A
--------------------------------------------------*/
static void K_InitRoulette(itemroulette_t *const roulette)
{
	size_t i;

#ifndef ITEM_LIST_SIZE
	if (roulette->itemList == NULL)
	{
		roulette->itemListCap = 8;
		roulette->itemList = Z_Calloc(
			sizeof(SINT8) * roulette->itemListCap,
			PU_STATIC,
			&roulette->itemList
		);

		if (roulette->itemList == NULL)
		{
			I_Error("Not enough memory for item roulette list\n");
		}
	}
#endif

	roulette->itemListLen = 0;
	roulette->index = 0;

	roulette->baseDist = roulette->dist = 0;
	roulette->playing = roulette->exiting = 0;
	roulette->firstDist = roulette->secondDist = UINT32_MAX;
	roulette->secondToFirst = 0;

	roulette->elapsed = 0;
	roulette->tics = roulette->speed = ROULETTE_SPEED_TIMEATTACK; // Some default speed

	roulette->active = true;
	roulette->eggman = false;
	roulette->ringbox = false;
	roulette->autoroulette = false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			continue;
		}

		roulette->playing++;

		if (players[i].exiting)
		{
			roulette->exiting++;
		}

		if (specialstageinfo.valid == true)
		{
			UINT32 dis = K_UndoMapScaling(players[i].distancetofinish);
			if (dis < roulette->secondDist)
			{
				roulette->secondDist = dis;
			}
		}
		else
		{
			if (players[i].position == 1)
			{
				roulette->firstDist = K_UndoMapScaling(players[i].distancetofinish);
			}

			if (players[i].position == 2)
			{
				roulette->secondDist = K_UndoMapScaling(players[i].distancetofinish);
			}
		}
	}

	if (specialstageinfo.valid == true)
	{
		roulette->firstDist = K_UndoMapScaling(K_GetSpecialUFODistance());
	}

	// Calculate 2nd's distance from 1st, for SPB
	if (roulette->firstDist != UINT32_MAX && roulette->secondDist != UINT32_MAX
		&& roulette->secondDist > roulette->firstDist)
	{
		roulette->secondToFirst = roulette->secondDist - roulette->firstDist;
		roulette->secondToFirst = K_ScaleItemDistance(&players[i], roulette->secondToFirst, 16 - roulette->playing); // Reversed scaling
	}
}

/*--------------------------------------------------
	static void K_PushToRouletteItemList(itemroulette_t *const roulette, INT32 item)

		Pushes a new item to the end of the item
		roulette's item list. Also accepts slot machine
		values instead of items.

	Input Arguments:-
		roulette - The item roulette data to modify.
		item - The item / slot machine index to push to the list.

	Return:-
		N/A
--------------------------------------------------*/
static void K_PushToRouletteItemList(itemroulette_t *const roulette, INT32 item)
{
#ifdef ITEM_LIST_SIZE
	if (roulette->itemListLen >= ITEM_LIST_SIZE)
	{
		I_Error("Out of space for item reel! Go and make ITEM_LIST_SIZE bigger I guess?\n");
		return;
	}
#else
	I_Assert(roulette->itemList != NULL);

	if (roulette->itemListLen >= roulette->itemListCap)
	{
		roulette->itemListCap *= 2;
		roulette->itemList = Z_Realloc(
			roulette->itemList,
			sizeof(SINT8) * roulette->itemListCap,
			PU_STATIC,
			&roulette->itemList
		);

		if (roulette->itemList == NULL)
		{
			I_Error("Not enough memory for item roulette list\n");
		}
	}
#endif

	roulette->itemList[ roulette->itemListLen ] = item;
	roulette->itemListLen++;
}

/*--------------------------------------------------
	static void K_AddItemToReel(const player_t *player, itemroulette_t *const roulette, kartitems_t item)

		Adds an item to a player's item reel. Unlike
		pushing directly with K_PushToRouletteItemList,
		this function handles special behaviors (like
		padding with extra Super Rings).

	Input Arguments:-
		player - The player to add to the item roulette.
			This is valid to be NULL.
		roulette - The player's item roulette data.
		item - The item to push to the list.

	Return:-
		N/A
--------------------------------------------------*/
static void K_AddItemToReel(const player_t *player, itemroulette_t *const roulette, kartitems_t item)
{
	K_PushToRouletteItemList(roulette, item);

	if (player == NULL)
	{
		return;
	}

	// If we're in ring debt, pad out the reel with
	// a BUNCH of Super Rings.
	if (K_ItemEnabled(KITEM_SUPERRING) == true
		&& player->rings <= 0
		&& player->position == 1
		&& (gametyperules & GTR_SPHERES) == 0)
	{
		K_PushToRouletteItemList(roulette, KITEM_SUPERRING);
	}
}

/*--------------------------------------------------
	static void K_CalculateRouletteSpeed(itemroulette_t *const roulette)

		Determines the speed for the item roulette,
		adjusted for progress in the race and front
		running.

	Input Arguments:-
		roulette - The item roulette data to modify.

	Return:-
		N/A
--------------------------------------------------*/
static void K_CalculateRouletteSpeed(itemroulette_t *const roulette)
{
	fixed_t frontRun = 0;
	fixed_t progress = 0;
	fixed_t total = 0;

	if (K_CheckBossIntro() == true)
	{
		// Boss in action, use a speed controlled by boss health
		total = FixedDiv(bossinfo.healthbar, BOSSHEALTHBARLEN);
		roulette->tics = roulette->speed = ROULETTE_SPEED_FASTEST + FixedMul(ROULETTE_SPEED_VERSUS_SLOWEST - ROULETTE_SPEED_FASTEST, total);
		return;
	}

	if (roulette->autoroulette == true)
	{
		roulette->speed = ROULETTE_SPEED_FASTEST;
		return;
	}

	if (K_TimeAttackRules() == true && !(modeattacking & ATTACKING_SPB))
	{
		// Time Attack rules; use a consistent speed.
		roulette->tics = roulette->speed = ROULETTE_SPEED_TIMEATTACK;
		return;
	}

	if (roulette->baseDist > ENDDIST)
	{
		// Being farther in the course makes your roulette faster.
		progress = min(FRACUNIT, FixedDiv(roulette->baseDist - ENDDIST, ROULETTE_SPEED_DIST));
	}

	if (roulette->baseDist > roulette->firstDist)
	{
		// Frontrunning makes your roulette faster.
		frontRun = min(FRACUNIT, FixedDiv(roulette->baseDist - roulette->firstDist, ENDDIST));
	}

	// Combine our two factors together.
	total = min(FRACUNIT, (frontRun / 2) + (progress / 2));

	if (leveltime < starttime + 30*TICRATE)
	{
		// Don't impact as much at the start.
		// This makes it so that everyone gets to enjoy the lowest speed at the start.
		if (leveltime < starttime)
		{
			total = FRACUNIT;
		}
		else
		{
			const fixed_t lerp = FixedDiv(leveltime - starttime, 30*TICRATE);
			total = FRACUNIT + FixedMul(lerp, total - FRACUNIT);
		}
	}

	roulette->tics = roulette->speed = ROULETTE_SPEED_FASTEST + FixedMul(ROULETTE_SPEED_SLOWEST - ROULETTE_SPEED_FASTEST, total);
}

static boolean K_IsItemPower(kartitems_t item)
{
	switch (item)
	{
		case KITEM_ROCKETSNEAKER:
		case KITEM_JAWZ:
		case KITEM_LANDMINE:
		case KITEM_DROPTARGET:
		case KITEM_BALLHOG:
		case KRITEM_TRIPLESNEAKER:
		case KRITEM_TRIPLEORBINAUT:
		case KRITEM_QUADORBINAUT:
		case KRITEM_DUALJAWZ:
		case KITEM_HYUDORO:
		case KRITEM_TRIPLEBANANA:
		case KITEM_FLAMESHIELD:
		case KITEM_GARDENTOP:
		case KITEM_SHRINK:
		case KITEM_LIGHTNINGSHIELD:
			return true;
		default:
			return false;
	}
}

static boolean K_IsItemFirstOnly(kartitems_t item)
{
	switch (item)
	{
		case KITEM_LANDMINE:
		case KITEM_LIGHTNINGSHIELD:
		case KITEM_HYUDORO:
		case KITEM_DROPTARGET:
			return true;
		default:
			return false;
	}
}

static boolean K_IsItemFirstPermitted(kartitems_t item)
{
	if (K_IsItemFirstOnly(item))
		return true;

	switch (item)
	{
		case KITEM_BANANA:
		case KITEM_EGGMAN:
		case KITEM_ORBINAUT:
		case KITEM_SUPERRING:
			return true;
		default:
			return false;
	}
}

// Which items are disallowed for THIS player?
static boolean K_ShouldPlayerAllowItem(kartitems_t item, const player_t *player)
{
	if (!(gametyperules & GTR_CIRCUIT))
		return true;
	if (specialstageinfo.valid == true)
		return true;

	if (player->position == 1)
		return K_IsItemFirstPermitted(item);
	else
	{
		if (K_IsItemPower(item) && (leveltime < ((15*TICRATE) + starttime)))
			return false;
		return !K_IsItemFirstOnly(item);
	}
}

// Which items are disallowed for ALL players?
static boolean K_ShouldAllowItem(kartitems_t item, const itemroulette_t *roulette)
{
	if (!(gametyperules & GTR_CIRCUIT))
		return true;
	if (specialstageinfo.valid == true)
		return true;

	boolean notNearEnd = false;
	boolean cooldownOnStart = false;
	
	switch (item)
	{
		case KITEM_BANANA:
		case KITEM_EGGMAN:
		case KITEM_SUPERRING:
		{
			notNearEnd = true;
			break;
		}

		case KITEM_HYUDORO:
		case KRITEM_TRIPLEBANANA:
		{
			notNearEnd = true;
			break;
		}

		case KITEM_INVINCIBILITY:
		case KITEM_MINE:
		case KITEM_GROW:
		case KITEM_BUBBLESHIELD:
		{
			cooldownOnStart = true;
			break;
		}

		case KITEM_FLAMESHIELD:
		case KITEM_GARDENTOP:
		{
			cooldownOnStart = true;
			notNearEnd = true;
			break;
		}

		case KITEM_SPB:
		{
			cooldownOnStart = true;
			notNearEnd = true;
			// TODO forcing, just disable for now
			return false;
			break;
		}

		case KITEM_SHRINK:
		{
			cooldownOnStart = true;
			notNearEnd = true;
			break;
		}

		case KITEM_LIGHTNINGSHIELD:
		{
			cooldownOnStart = true;
			if ((gametyperules & GTR_CIRCUIT) && spbplace != -1)
			{
				return false;
			}
			break;
		}

		default:
			break;
	}

	if (cooldownOnStart && (leveltime < ((30*TICRATE) + starttime)))
		return false;
	if (notNearEnd && (roulette != NULL && roulette->baseDist < ENDDIST))
		return false;
	if (K_DenyShieldOdds(item))
		return false;

	if (roulette && roulette->autoroulette == true)
	{
		if (K_DenyAutoRouletteOdds(item))
		{
			return false;
		}
	}

	return true;
}

/*--------------------------------------------------
	void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette, boolean ringbox)

		See header file for description.
--------------------------------------------------*/
void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette, boolean ringbox)
{
	UINT32 spawnChance[NUMKARTRESULTS] = {0};
	UINT32 totalSpawnChance = 0;
	size_t rngRoll = 0;

	UINT8 numItems = 0;
	kartitems_t singleItem = KITEM_SAD;

	size_t i, j;

	K_InitRoulette(roulette);

	if (player != NULL)
	{
		roulette->baseDist = K_UndoMapScaling(player->distancetofinish);
		
		if (player->pflags & PF_AUTOROULETTE)
			roulette->autoroulette = true;

		K_CalculateRouletteSpeed(roulette);
	}

	if (ringbox == true)
	{
		// If this is being invoked by a Ring Box, it should literally never produce items.
		kartslotmachine_t *presetlist = K_KartItemReelRingBox;
		roulette->ringbox = true;

		for (i = 0; presetlist[i] != KSM__MAX; i++)
		{
			K_PushToRouletteItemList(roulette, presetlist[i]);
		}

		return;
	}

	// SPECIAL CASE No. 1:
	// Give only the debug item if specified
	if (cv_kartdebugitem.value != KITEM_NONE)
	{
		K_PushToRouletteItemList(roulette, cv_kartdebugitem.value);
		return;
	}

	// SPECIAL CASE No. 2:
	// Use a special, pre-determined item reel for Time Attack / Free Play / End of Sealed Stars
	if (specialstageinfo.valid)
	{
		if (K_GetPossibleSpecialTarget() == NULL)
		{
			for (i = 0; K_KartItemReelSpecialEnd[i] != KITEM_NONE; i++)
			{
				K_PushToRouletteItemList(roulette, K_KartItemReelSpecialEnd[i]);
			}
			return;
		}
	}
	else if (K_CheckBossIntro() == true)
	{
		for (i = 0; K_KartItemReelBoss[i] != KITEM_NONE; i++)
		{
			K_PushToRouletteItemList(roulette, K_KartItemReelBoss[i]);
		}

		return;
	}
	else if (K_TimeAttackRules() == true)
	{
		kartitems_t *presetlist = NULL;

		// If the objective is not to go fast, it's to cause serious damage.
		if (battleprisons == true)
		{
			presetlist = K_KartItemReelBreakTheCapsules;
		}
		else if (modeattacking & ATTACKING_SPB)
		{
			presetlist = K_KartItemReelSPBAttack;
		}
		else if (K_CanChangeRules(true) == false) // GT_TUTORIAL, time attack
		{
			presetlist = K_KartItemReelRingSneaker;
		}

		if (presetlist != NULL)
		{
			for (i = 0; presetlist[i] != KITEM_NONE; i++)
			{
				K_PushToRouletteItemList(roulette, presetlist[i]);
			}
		}
		else
		{
			// New FREE PLAY behavior;
			// every item in the game!

			// Create the same item reel given the same inputs.
			// P_SetRandSeed(PR_ITEM_ROULETTE, ITEM_REEL_SEED);

			for (i = 1; i < NUMKARTRESULTS; i++)
			{
				if (K_ItemEnabled(i) == true)
				{
					spawnChance[i] = ( totalSpawnChance += 1 );
				}
			}

			while (totalSpawnChance > 0)
			{
				rngRoll = P_RandomKey(PR_ITEM_ROULETTE, totalSpawnChance);
				for (i = 1; i < NUMKARTRESULTS && spawnChance[i] <= rngRoll; i++)
				{
					continue;
				}

				K_PushToRouletteItemList(roulette, i);

				for (; i < NUMKARTRESULTS; i++)
				{
					// Be sure to fix the remaining items' odds too.
					if (spawnChance[i] > 0)
					{
						spawnChance[i]--;
					}
				}

				totalSpawnChance--;
			}
		}

		return;
	}

	// SPECIAL CASE No. 3:
	// Only give the SPB if conditions are right
	if (K_ForcedSPB(player, roulette) == true)
	{
		K_AddItemToReel(player, roulette, KITEM_SPB);
		return;
	}

	// SPECIAL CASE No. 4:
	// If only one item is enabled, always use it
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		if (K_ItemEnabled(i) == true)
		{
			numItems++;
			if (numItems > 1)
			{
				break;
			}

			singleItem = i;
		}
	}

	if (numItems < 2)
	{
		// singleItem = KITEM_SAD by default,
		// so it will be used when all items are turned off.
		K_AddItemToReel(player, roulette, singleItem);
		return;
	}

	// Special cases are all handled, we can now
	// actually calculate actual item reels.
	roulette->preexpdist = K_GetItemRouletteDistance(player, roulette->playing);
	roulette->dist = roulette->preexpdist;

	if (gametyperules & GTR_CIRCUIT)
		roulette->dist = FixedMul(roulette->preexpdist, max(player->exp, FRACUNIT/2));

	// Dynamic Roulette. Oh boy!

	// STAGE 1: Determine what items are permissible
	// STAGE 2: Determine the item that's most appropriate for our distance from leader
	// STAGE 3: Pick that item, then penalize it
	// STAGE 4: Repeat 3 until the reel is full, then cram everything in

	UINT32 targetpower = roulette->dist; // fill roulette with items around this value!
	UINT32 powers[NUMKARTRESULTS]; // how strong is each item?
	UINT32 deltas[NUMKARTRESULTS]; // how different is that strength from target?
	UINT32 candidates[NUMKARTRESULTS]; // how many of this item should we try to insert?
	UINT32 dupetolerance[NUMKARTRESULTS]; // how willing are we to select this item after already selecting it? higher values = lower dupe penalty
	boolean permit[NUMKARTRESULTS]; // is this item allowed?

	boolean rival = (player->bot && (player->botvars.rival || cv_levelskull.value));
	boolean mothfilter = true; // strip unusually weak items from reel?
	UINT8 reelsize = 15; // How many items to attempt to add in prepass?
	UINT32 humanscaler = 250 + (roulette->playing * 15); // Scaler that converts "useodds" style distances in odds tables to raw distances.

	// Cache which items are permissible
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		permit[i] = K_ShouldAllowItem(i, roulette);

		// CONS_Printf("%s permit prepass %d\n", cv_items[i-1].name, permit[i]);

		if (permit[i])
			permit[i] = K_ShouldPlayerAllowItem(i, player);

		// CONS_Printf("%s permit postpass %d\n", cv_items[i-1].name, permit[i]);
	}

	// temp - i have no fucking clue how pointers work i am so sorry
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		// NOTE: Battle odds are underspecified, we don't invoke roulettes in this mode!
		if (gametyperules & GTR_BUMPERS)
		{
			powers[i] = humanscaler * K_DynamicItemOddsBattle[i-1][0];
			dupetolerance[i] = K_DynamicItemOddsBattle[i-1][1];
			mothfilter = false;
		}
		else if (specialstageinfo.valid == true)
		{
			powers[i] = humanscaler * K_DynamicItemOddsSpecial[i-1][0];
			dupetolerance[i] = K_DynamicItemOddsSpecial[i-1][1];
			reelsize = 8;
			mothfilter = false;
		}
		else
		{
			powers[i] = humanscaler * K_DynamicItemOddsRace[i-1][0];
			dupetolerance[i] = K_DynamicItemOddsRace[i-1][1];
		}
	}

	// null stuff that doesn't have odds
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		if (powers[i] == 0)
		{
			// CONS_Printf("%s nulled\n", cv_items[i-1].name);
			permit[i] = false;
		}
	}	

	// Starting deltas
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		candidates[i] = 0;
		deltas[i] = min(targetpower - powers[i], powers[i] - targetpower);
		// CONS_Printf("starting delta for %s is %d\n", cv_items[i-1].name, deltas[i]);
	}

	UINT8 added = 0;
	UINT32 totalreelpower = 0;

	// let's start finding items to list
	for (i = 0; i < reelsize; i++)
	{
		UINT32 lowestdelta = INT32_MAX;
		size_t bestitem = 0;

		// CONS_Printf("LOOP %d\n", i);

		// check each kartitem to see which is the best fit,
		// based on what's closest to our target power
		// (but ignore items that aren't allowed now)
		for (j = 1; j < NUMKARTRESULTS; j++)
		{
			// CONS_Printf("precheck %s, perm %d CD %d\n", cv_items[j-1].name, permit[j], K_GetItemCooldown(j));

			if (!permit[j])
				continue;
			if (K_GetItemCooldown(j))
				continue;
			if (!K_ItemEnabled(j))
				continue;

			// CONS_Printf("checking %s, delta %d\n", cv_items[j-1].name, deltas[j]);

			if (lowestdelta > deltas[j])
			{
				bestitem = j;
				lowestdelta = deltas[j];
			}
		}

		// couldn't find an item? goodbye lol
		if (bestitem == 0)
			break;

		// UINT32 deltapenalty = (DISTVAR*4)^(candidates[bestitem])/dupetolerance[bestitem];
		UINT32 deltapenalty = 4*DISTVAR*(1+candidates[bestitem])/dupetolerance[bestitem];

		if (K_IsItemPower(i) && rival)
			deltapenalty = 3 * deltapenalty / 4;
		if (K_IsItemPower(i) && franticitems)
			deltapenalty = 3 * deltapenalty / 4;

		if (cv_kartdebugdistribution.value > 1)
		{
			UINT16 BASE_X = 18;
			UINT16 BASE_Y = 5+12*i;
			INT32 FLAGS = V_SNAPTOTOP|V_SNAPTOLEFT;
			// V_DrawThinString(BASE_X + 100, BASE_Y, FLAGS, va("%s", cv_items[lowestindex-1].name));
			V_DrawThinString(BASE_X + 35, BASE_Y, FLAGS, va("P%d", powers[bestitem]/humanscaler));
			V_DrawThinString(BASE_X + 65, BASE_Y, FLAGS, va("D%d", deltas[bestitem]/humanscaler));
			V_DrawThinString(BASE_X + 20, BASE_Y, FLAGS, va("%d", dupetolerance[bestitem]));
			//V_DrawThinString(BASE_X + 70, BASE_Y, FLAGS, va("+%d", deltapenalty));
			V_DrawFixedPatch(BASE_X*FRACUNIT, (BASE_Y-7)*FRACUNIT, (FRACUNIT >> 1), FLAGS, K_GetSmallStaticCachedItemPatch(bestitem), NULL);
			UINT8 amount = K_ItemResultToAmount(bestitem);
			if (amount > 1)
			{
				V_DrawThinString(BASE_X, BASE_Y, FLAGS, va("x%d", amount));
			}
		}

		// otherwise, prep it to be added and give it a duplicaton penalty,
		// so that a different item is more likely to be inserted next
		candidates[bestitem]++;
		deltas[bestitem] += deltapenalty;

		totalreelpower += powers[bestitem];
		added++;

		// CONS_Printf("added %s with candidates %d\n", cv_items[lowestindex-1].name, candidates[lowestindex]);
	}

	fixed_t spb_odds = K_PercentSPBOdds(roulette, player->position);

	if ((gametyperules & GTR_CIRCUIT) & (spb_odds > 0) & (spbplace == -1))
	{
		permit[KITEM_SPB] = true;
		deltas[KITEM_SPB] = Easing_Linear(spb_odds, 3000, 0);
	}

	UINT8 debugcount = 0;
	UINT32 meanreelpower = totalreelpower/max(added, 1);

	// set up the list indices used to random-shuffle the ro ulette
	for (i = 1; i < NUMKARTRESULTS; i++)
	{	
		// filter items vastly too weak for this reel
		boolean reject = (powers[i] + DISTVAR < meanreelpower);

		if (!mothfilter)
			reject = false;

		if (cv_kartdebugdistribution.value && candidates[i])
		{
			UINT16 BASE_X = 280;
			UINT16 BASE_Y = 5+12*debugcount;
			INT32 FLAGS = V_SNAPTOTOP|V_SNAPTORIGHT;

			V_DrawThinString(BASE_X - 12, 5, FLAGS, va("%d", targetpower/humanscaler));

			for(UINT8 k = 0; k < candidates[i]; k++)
				V_DrawFixedPatch((BASE_X + 3*k)*FRACUNIT, (BASE_Y-7)*FRACUNIT, (FRACUNIT >> 1), FLAGS, K_GetSmallStaticCachedItemPatch(i), NULL);

			UINT8 amount = K_ItemResultToAmount(i);
			if (amount > 1)
			{
				V_DrawThinString(BASE_X, BASE_Y, FLAGS, va("x%d", amount));
			}

			if (reject)
				V_DrawThinString(BASE_X, BASE_Y, FLAGS|V_60TRANS, va("WEAK"));
			debugcount++;
		}

		if (!reject)
		{
			spawnChance[i] = (
				totalSpawnChance += candidates[i]
			);
		}
	}

	if (totalSpawnChance == 0)
	{
		// why did this fucking happen LOL
		// don't crash
		K_AddItemToReel(player, roulette, singleItem);
		return;
	}

	// and insert all of our candidates into the roulette in a random order
	while (totalSpawnChance > 0)
	{
		rngRoll = P_RandomKey(PR_ITEM_ROULETTE, totalSpawnChance);
		for (i = 1; i < NUMKARTRESULTS && spawnChance[i] <= rngRoll; i++)
		{
			continue;
		}

		// CONS_Printf("adding %s, tsp %d\n", cv_items[i-1].name, totalSpawnChance);

		K_AddItemToReel(player, roulette, i);

		for (; i < NUMKARTRESULTS; i++)
		{
			// Be sure to fix the remaining items' odds too.
			if (spawnChance[i] > 0)
			{
				spawnChance[i]--;
			}
		}

		totalSpawnChance--;
	}
}

/*--------------------------------------------------
	void K_StartItemRoulette(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_StartItemRoulette(player_t *const player, boolean ringbox)
{
	itemroulette_t *const roulette = &player->itemRoulette;
	size_t i;

	K_FillItemRouletteData(player, roulette, ringbox);

	if (roulette->autoroulette)
		roulette->index = P_RandomRange(PR_AUTOROULETTE, 0, roulette->itemListLen - 1);

	if (K_PlayerUsesBotMovement(player) == true)
	{
		K_BotPickItemPriority(player);
	}

	// Prevent further duplicates of items that
	// are intended to only have one out at a time.
	for (i = 0; i < roulette->itemListLen; i++)
	{
		kartitems_t item = roulette->itemList[i];
		if (K_ItemSingularity(item) == true)
		{
			K_SetItemCooldown(item, TICRATE<<4);
		}
	}
}

/*--------------------------------------------------
	void K_StartEggmanRoulette(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_StartEggmanRoulette(player_t *const player)
{
	itemroulette_t *const roulette = &player->itemRoulette;
	K_StartItemRoulette(player, false);
	roulette->eggman = true;
}

/*--------------------------------------------------
	void K_StopRoulette(itemroulette_t *const roulette)

		See header file for description.
--------------------------------------------------*/
void K_StopRoulette(itemroulette_t *const roulette)
{
	roulette->active = false;
	roulette->eggman = false;
	roulette->ringbox = false;
	roulette->reserved = 0;
}

/*--------------------------------------------------
	fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge)

		See header file for description.
--------------------------------------------------*/
fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge)
{
	const fixed_t curTic = (roulette->tics << FRACBITS) - renderDelta;
	const fixed_t midTic = roulette->speed * (FRACUNIT >> 1);

	fixed_t result = FixedMul(FixedDiv(midTic - curTic, ((roulette->speed + 1) << FRACBITS)), ROULETTE_SPACING);

	if (fudge > 0)
	{
		result += (roulette->speed + 1) * fudge;
	}

	return result;
}

/*--------------------------------------------------
	fixed_t K_GetSlotOffset(itemroulette_t *const roulette, fixed_t renderDelta)

		See header file for description.
--------------------------------------------------*/
fixed_t K_GetSlotOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge)
{
	const fixed_t curTic = (roulette->tics << FRACBITS) - renderDelta;
	const fixed_t midTic = roulette->speed * (FRACUNIT >> 1);

	fixed_t result = FixedMul(FixedDiv(midTic - curTic, ((roulette->speed + 1) << FRACBITS)), SLOT_SPACING);

	if (fudge > 0)
	{
		result += (roulette->speed + 1) * fudge;
	}

	return result;
}

/*--------------------------------------------------
	static void K_KartGetItemResult(player_t *const player, kartitems_t getitem)

		Initializes a player's item to what was
		received from the roulette.

	Input Arguments:-
		player - The player receiving the item.
		getitem - The item to give to the player.

	Return:-
		N/A
--------------------------------------------------*/
static void K_KartGetItemResult(player_t *const player, kartitems_t getitem)
{
	if (K_ItemSingularity(getitem) == true)
	{
		K_SetItemCooldown(getitem, 20*TICRATE);
	}

	player->botvars.itemdelay = TICRATE;
	player->botvars.itemconfirm = 0;

	player->itemtype = K_ItemResultToType(getitem);
	UINT8 itemamount = K_ItemResultToAmount(getitem);
	if (cv_kartdebugitem.value != KITEM_NONE && cv_kartdebugitem.value == player->itemtype && cv_kartdebugamount.value > 1)
		itemamount = cv_kartdebugamount.value;
	player->itemamount = itemamount;

	if (player->itemtype == KITEM_SPB)
		Obj_SPBEradicateCapsules();
}

/*--------------------------------------------------
	void K_KartItemRoulette(player_t *const player, ticcmd_t *const cmd)

		See header file for description.
--------------------------------------------------*/
void K_KartItemRoulette(player_t *const player, ticcmd_t *const cmd)
{
	itemroulette_t *const roulette = &player->itemRoulette;
	boolean confirmItem = false;

	if (roulette->reserved > 0)
	{
		roulette->reserved--;
		return;
	}

	// This makes the roulette cycle through items.
	// If this isn't active, you shouldn't be here.
	if (roulette->active == false)
	{
		return;
	}

	if (roulette->itemListLen == 0
#ifndef ITEM_LIST_SIZE
		|| roulette->itemList == NULL
#endif
		)
	{
		// Invalid roulette setup.
		// Escape before we run into issues.
		roulette->active = false;
		return;
	}

	if (roulette->elapsed > TICRATE>>1) // Prevent accidental immediate item confirm
	{
		if (roulette->elapsed > TICRATE<<4 || (roulette->eggman && !roulette->autoroulette && roulette->elapsed > TICRATE*4))
		{
			// Waited way too long, forcefully confirm the item.
			confirmItem = true;
		}
		else if (roulette->autoroulette)
		{
			// confirmItem = (roulette->speed > 15);
			confirmItem = (roulette->elapsed >= TICRATE*2);
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
	if (confirmItem == true && ((roulette->autoroulette) || (player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT|IF_USERINGS)) == 0))
	{
		if (roulette->eggman == true)
		{
			// FATASS JUMPSCARE instead of your actual item
			player->eggmanexplode = 6*TICRATE;

			//player->karthud[khud_itemblink] = TICRATE;
			//player->karthud[khud_itemblinkmode] = 1;
			//player->karthud[khud_rouletteoffset] = K_GetRouletteOffset(roulette, FRACUNIT, 0);

			if (K_IsPlayingDisplayPlayer(player))
			{
				S_StartSound(NULL, sfx_itrole);
			}
		}
		else
		{
			UINT8 baseFudge = player->cmd.latency; // max(0, player->cmd.latency - 2);
			if (roulette->autoroulette)
			{
				baseFudge = 0; // We didn't manually stop this, you jackwagon
			}

			UINT8 fudgedDelay = baseFudge;
			while (fudgedDelay > 0)
			{
				UINT8 gap = (roulette->speed - roulette->tics); // How long has the roulette been on this entry?
				if (fudgedDelay > gap) // Did the roulette tick over in-flight?
				{
					fudgedDelay = fudgedDelay - gap; // We're compensating for this gap's worth of delay, so cut it down.
					roulette->index = roulette->index == 0 ? roulette->itemListLen - 1 : roulette->index - 1; // Roll the roulette index back...
					roulette->tics = 0; // And just in case our delay is SO high that a fast roulette needs to roll back again...
				}
				else
				{
					break;
				}
			}

			// And one more nudge for the remaining delay.
			roulette->tics = (roulette->tics + fudgedDelay) % roulette->speed;

			INT32 finalItem = roulette->itemList[ roulette->index ];

			if (roulette->ringbox == true)
			{
				player->ringboxdelay = TICRATE;
				player->ringboxaward = finalItem;
				player->karthud[khud_rouletteoffset] = K_GetSlotOffset(roulette, FRACUNIT, baseFudge);
			}
			else
			{
				K_KartGetItemResult(player, finalItem);
				player->karthud[khud_rouletteoffset] = K_GetRouletteOffset(roulette, FRACUNIT, baseFudge);
			}

			player->karthud[khud_itemblink] = TICRATE;
			player->karthud[khud_itemblinkmode] = 0;

			if (K_IsPlayingDisplayPlayer(player))
			{
				if (roulette->ringbox)
				{
					// Hi modders! Boost your treble and Loudness Normalize to 0 LUFS.
					// I'm a responsible audio engineer. -Tyron 2023-07-30
					UINT8 volume = (finalItem > 2) ? (15 * finalItem + 60) : 80;
					S_StartSoundAtVolume(NULL, ringboxsound[finalItem], volume);
				}
				else
					S_StartSound(NULL, sfx_itrolf);
			}
		}

		// We're done, disable the roulette
		roulette->active = false;
		return;
	}

	roulette->elapsed++;

	/*
	if (roulette->autoroulette && (roulette->elapsed % 5 == 0) && (roulette->elapsed > TICRATE))
		roulette->speed++;
	*/

	if (roulette->tics == 0)
	{
		roulette->index = (roulette->index + 1) % roulette->itemListLen;
		roulette->tics = roulette->speed;

		// This makes the roulette produce the random noises.
		roulette->sound = (roulette->sound + 1) % 8;

		if (K_IsPlayingDisplayPlayer(player))
		{
			if (roulette->ringbox)
				S_StartSound(NULL, sfx_s240);
			else
				S_StartSound(NULL, sfx_itrol1 + roulette->sound);
		}
	}
	else
	{
		roulette->tics--;
	}
}
