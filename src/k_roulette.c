// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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
	{25, 10}, // sneaker
	{63, 12}, // rocketsneaker
	{60, 19}, // invincibility
	{8, 4}, // banana
	{3, 1}, // eggmark
	{11, 4}, // orbinaut
	{16, 4}, // jawz
	{19, 4}, // mine
	{1, 3}, // landmine
	{25, 3}, // ballhog
	{58, 6}, // selfpropelledbomb
	{60, 7}, // grow
	{70, 8}, // shrink
	{1, 1}, // lightningshield
	{25, 4}, // bubbleshield
	{66, 9}, // flameshield
	{1, 2}, // hyudoro
	{0, 0}, // pogospring
	{30, 8}, // superring (SPECIAL! distance value specifies when this can NO LONGER appear)
	{0, 0}, // kitchensink
	{1, 2}, // droptarget
	{43, 5}, // gardentop
	{0, 0}, // gachabom
	{1, 2}, // stoneshoe
	{1, 2}, // toxomister
	{45, 6}, // dualsneaker
	{55, 8}, // triplesneaker
	{25, 2}, // triplebanana
	{25, 1}, // tripleorbinaut
	{35, 2}, // quadorbinaut
	{30, 4}, // dualjawz
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
	{0, 0}, // stoneshoe
	{0, 0}, // toxomister
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
	{0, 0}, // stoneshoe
	{0, 0}, // toxomister
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
	{ 0, 1 }, // Stone Shoe
	{ 0, 1 }, // Toxomister
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
		case KITEM_STONESHOE:
		case KITEM_TOXOMISTER:
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
		case KITEM_LIGHTNINGSHIELD:
		case KITEM_BUBBLESHIELD:
		case KITEM_FLAMESHIELD:
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

		See header file for description.
--------------------------------------------------*/
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

/*--------------------------------------------------
	UINT32 K_UndoMapScaling(UINT32 distance)

		Takes a raw map distance and adjusts it to
		be in x1 scale.

	Input Arguments:-
		distance - Original distance.

	Return:-
		Distance unscaled by mapobjectscale.
--------------------------------------------------*/
UINT32 K_UndoMapScaling(UINT32 distance)
{
	if (mapobjectscale != FRACUNIT)
	{
		// Bring back to normal scale.
		return FixedDiv(distance, mapobjectscale);
	}

	return distance;
}

/*--------------------------------------------------
	UINT32 K_ScaleItemDistance(UINT32 distance, UINT8 numPlayers)

		Adjust item distance for lobby-size scaling
		as well as Frantic Items.

	Input Arguments:-
		player - The player to get the distance of.
		distance - Original distance.
		numPlayers - Number of players in the game.

	Return:-
		New distance after scaling.
--------------------------------------------------*/
UINT32 K_ScaleItemDistance(INT32 distance, UINT8 numPlayers)
{
#if 0
	if (franticitems == true)
	{
		// Frantic items pretends everyone's farther apart, for crazier items.
		distance = FixedMul(distance, FRANTIC_ITEM_SCALE);
	}
#endif

	// Items get crazier with the fewer players that you have.
	distance = FixedMul(
		distance,
		FRACUNIT + (K_ItemOddsScale(numPlayers) / 2)
	);

	return distance;
}

static UINT32 K_GetUnscaledFirstDistance(const player_t *player)
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

	return pdis;
}


/*--------------------------------------------------
	static UINT32 K_GetItemRouletteDistance(const player_t *player, UINT8 numPlayers)

		See header file for description.
--------------------------------------------------*/
UINT32 K_GetItemRouletteDistance(const player_t *player, UINT8 numPlayers)
{
	UINT32 pdis = K_GetUnscaledFirstDistance(player);
	pdis = K_ScaleItemDistance(pdis, numPlayers);

	if (player->bot && (player->botvars.rival || cv_levelskull.value))
	{
		// Rival has better odds :)
		pdis = FixedMul(pdis, FRANTIC_ITEM_SCALE);
	}

	return pdis;
}

/*--------------------------------------------------
	boolean K_DenyShieldOdds(kartitems_t item)

		See header file for description.
--------------------------------------------------*/
boolean K_DenyShieldOdds(kartitems_t item)
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

		return multiplier;
	}
}


/*--------------------------------------------------
	INT32 K_KartGetBattleOdds(const player_t *player, UINT8 pos, kartitems_t item)

		See header file for description.
--------------------------------------------------*/

INT32 K_KartGetBattleOdds(const player_t *player, UINT8 pos, kartitems_t item)
{
	(void)player;

	I_Assert(item > KITEM_NONE); // too many off by one scenarioes.
	I_Assert(item < NUMKARTRESULTS);

	I_Assert(pos < 2); // DO NOT allow positions past the bounds of the table

	fixed_t newOdds = K_KartLegacyBattleOdds[item-1][pos];
	newOdds <<= FRACBITS;

	return newOdds;
}

/*--------------------------------------------------
	static boolean K_ForcedSPB(const player_t *player, itemroulette_t *const roulette)

		See header file for description.
--------------------------------------------------*/
boolean K_ForcedSPB(const player_t *player, itemroulette_t *const roulette)
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
void K_InitRoulette(itemroulette_t *const roulette)
{
	size_t i;

#ifndef ITEM_LIST_SIZE
	if (roulette->itemList.items == NULL)
	{
		roulette->itemList.cap = 32;
		roulette->itemList.items = Z_Calloc(
			sizeof(SINT8) * roulette->itemList.cap,
			PU_STATIC,
			NULL
		);

		if (roulette->itemList.items == NULL)
		{
			I_Error("Not enough memory for item roulette list\n");
		}
	}
#endif

	roulette->itemList.len = 0;
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

	roulette->popcorn = 1;

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
		roulette->secondToFirst = K_ScaleItemDistance(roulette->secondToFirst, 16 - roulette->playing); // Reversed scaling
	}
}

/*--------------------------------------------------
	void K_PushToRouletteItemList(itemroulette_t *const roulette, INT32 item)

		See header file for description.
--------------------------------------------------*/
void K_PushToRouletteItemList(itemroulette_t *const roulette, INT32 item)
{
#ifdef ITEM_LIST_SIZE
	if (roulette->itemList.len >= ITEM_LIST_SIZE)
	{
		I_Error("Out of space for item reel! Go and make ITEM_LIST_SIZE bigger I guess?\n");
		return;
	}
#else
	I_Assert(roulette->itemList.items != NULL);

	if (!roulette->ringbox && item >= NUMKARTRESULTS)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Item Roulette rejected an out-of-range item.\n"));
		return;
	}

	if (roulette->ringbox && item >= KSM__MAX)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Casino Roulette rejected an out-of-range item.\n"));
		return;
	}

	if (roulette->itemList.len >= roulette->itemList.cap)
	{
		roulette->itemList.cap *= 2;
		roulette->itemList.items = Z_Realloc(
			roulette->itemList.items,
			sizeof(SINT8) * roulette->itemList.cap,
			PU_STATIC,
			NULL
		);

		if (roulette->itemList.items == NULL)
		{
			I_Error("Not enough memory for item roulette list\n");
		}
	}
#endif

	roulette->itemList.items[ roulette->itemList.len ] = item;
	roulette->itemList.len++;
}

/*--------------------------------------------------
	void K_AddItemToReel(const player_t *player, itemroulette_t *const roulette, kartitems_t item)

		See header file for description.
--------------------------------------------------*/
void K_AddItemToReel(const player_t *player, itemroulette_t *const roulette, kartitems_t item)
{
	if (player && K_PlayerUsesBotMovement(player) && !K_BotUnderstandsItem(item))
		return;

	K_PushToRouletteItemList(roulette, item);

	if (player == NULL)
	{
		return;
	}

	// If we're in ring debt, pad out the reel with
	// a BUNCH of Super Rings.
	if (K_ItemEnabled(KITEM_SUPERRING) == true
		&& player->rings <= -10
		&& player->position == 1
		&& (gametyperules & GTR_SPHERES) == 0)
	{
		K_PushToRouletteItemList(roulette, KITEM_SUPERRING);
	}
}

/*--------------------------------------------------
	void K_CalculateRouletteSpeed(itemroulette_t *const roulette)

		See header file for description.
--------------------------------------------------*/
void K_CalculateRouletteSpeed(itemroulette_t *const roulette)
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

// Honestly, the "power item" class is kind of a vestigial concept,
// but we'll faithfully port it over since it's not hurting anything so far
// (and it's at least ostensibly a Rival balancing mechanism, wheee).
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
		case KITEM_STONESHOE:
		case KITEM_TOXOMISTER:
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

ATTRUNUSED static boolean K_IsItemUselessAlone(kartitems_t item)
{
	switch (item)
	{
		case KITEM_JAWZ:
		case KRITEM_DUALJAWZ:
		case KITEM_LIGHTNINGSHIELD:
		case KITEM_ORBINAUT:
		case KRITEM_TRIPLEORBINAUT:
		case KRITEM_QUADORBINAUT:
		case KITEM_BALLHOG:
		case KITEM_BUBBLESHIELD: // shhhhhh
			return true;
		default:
			return false;
	}
}

static boolean K_IsItemSpeed(kartitems_t item)
{
	switch (item)
	{
		case KITEM_ROCKETSNEAKER:
		case KITEM_GROW:
		case KITEM_INVINCIBILITY:
		case KITEM_SNEAKER:
		case KRITEM_DUALSNEAKER:
		case KRITEM_TRIPLESNEAKER:
		case KITEM_FLAMESHIELD:
		case KITEM_SHRINK:
		case KITEM_SUPERRING:
			return true;
		default:
			return false;
	}
}

static fixed_t K_RequiredXPForItem(kartitems_t item)
{
	switch (item)
	{
		case KITEM_GARDENTOP:
		case KITEM_SHRINK:
			return FRACUNIT; // "Base" item odds
		default:
			return 0;
	}
}

// Which items are disallowed for this player's specific placement?
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
		// A little inelegant: filter the most chaotic items from courses with early sets and tight layouts.
		if (K_IsItemPower(item) && (leveltime < ((15*TICRATE) + starttime)))
			return false;

		// GIGA power items reserved only for players who were doing great and died.
		if (K_EffectiveGradingFactor(player) < K_RequiredXPForItem(item))
			return false;

		// Expert items are G2+ only, no Top in Relaxed!
		if (K_RequiredXPForItem(item) >= FRACUNIT && gamespeed == KARTSPEED_EASY)
			return false;

		return !K_IsItemFirstOnly(item);
	}
}

// Which items are disallowed because it's the wrong time for them?
static boolean K_TimingPermitsItem(kartitems_t item, const itemroulette_t *roulette)
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
			// In Race, we reintroduce and reenable this item to counter breakaway frontruns.
			// No need to roll it if that's not the case.
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

	return true;
}

static void K_FixEmptyRoulette(const player_t *player, itemroulette_t *const roulette)
{
	if (roulette->itemList.len > 0)
		return;

	if (K_PlayerUsesBotMovement(player)) // Bots can't use certain items. Give them _something_.
		K_PushToRouletteItemList(roulette, KITEM_SUPERRING);
	else // Players can use all items, so this should never happen.
		K_PushToRouletteItemList(roulette, KITEM_SAD);
}

/*--------------------------------------------------
	void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette, boolean ringbox)

		See header file for description.
--------------------------------------------------*/
void K_FillItemRoulette(player_t *const player, itemroulette_t *const roulette, boolean ringbox)
{
	K_InitRoulette(roulette);

	if (player != NULL)
	{
		roulette->baseDist = K_UndoMapScaling(player->distancetofinish);

		if (player->pflags & PF_AUTOROULETTE)
			roulette->autoroulette = true;

		K_CalculateRouletteSpeed(roulette);
	}

	// Lua may want to intercept reelbuilder entirely.
	LUA_HookPreFillItemRoulette(player, roulette, ringbox);

	// If prehook did something, no need to continue.
	if (roulette->itemList.len != 0) {
		return;
	}

	K_FillItemRouletteData(player, roulette, ringbox, false);

	// Lua can modify the final result.
	LUA_HookFillItemRoulette(player, roulette, ringbox);

	// If somehow there's no items, add sad.
	if (roulette->itemList.len == 0) {
		if (roulette->ringbox)
			K_PushToRouletteItemList(roulette, KSM_BAR);
		else
			K_AddItemToReel(player, roulette, KITEM_SAD);
	}
}

/*--------------------------------------------------
	void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette, boolean ringbox, boolean dryrun)

		See header file for description.
--------------------------------------------------*/
void K_FillItemRouletteData(player_t *player, itemroulette_t *const roulette, boolean ringbox, boolean dryrun)
{
	UINT32 spawnChance[NUMKARTRESULTS] = {0};
	UINT32 totalSpawnChance = 0;
	size_t rngRoll = 0;

	UINT8 numItems = 0;
	kartitems_t singleItem = KITEM_SAD;

	size_t i, j;

	if (roulette->itemList.items == NULL)
	{
		K_InitRoulette(roulette);
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
			P_SetRandSeed(PR_ITEM_ROULETTE, ITEM_REEL_SEED);

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
		K_FixEmptyRoulette(player, roulette);
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
		K_FixEmptyRoulette(player, roulette);
		return;
	}

	// Special cases are all handled, we can now
	// actually calculate actual item reels.
	roulette->preexpdist = K_GetItemRouletteDistance(player, roulette->playing);
	roulette->dist = roulette->preexpdist;

	// ===============================================================================
	// Dynamic Roulette. Oh boy!
	// Alright, here's the broad plan:
	// 1: Determine what items are permissible
	// 2: Determine the permitted item that's most appropriate for our distance from leader
	// 3: Pick that item, then penalize it so it's less likely to be repicked
	// 4: Repeat 3 until we've picked enough stuff
	// 5: Skim any items that are much weaker than the reel's average out of the roulette
	// 6: Cram it all in

	UINT32 powers[NUMKARTRESULTS]; // how strong is each item? think of this as a "target distance" for this item to spawn at
	UINT32 deltas[NUMKARTRESULTS]; // how different is that strength from target?
	UINT32 candidates[NUMKARTRESULTS]; // how many of this item should we try to insert?
	UINT32 dupetolerance[NUMKARTRESULTS]; // how willing are we to select this item after already selecting it? higher values = lower dupe penalty
	boolean permit[NUMKARTRESULTS]; // is this item allowed?

	UINT32 lonelinessSuppressor = DISTVAR; // This close to 1st? Dampen loneliness (you have a target!)
	UINT32 maxEXPDistanceCut = 3*DISTVAR; // The maximum amount you can be displaced by EXP

	// If we're too close to 1st in absolute units, crush our top-end item odds down.
	fixed_t crowdingFirst = 0;
	if (player->position != 1)
		crowdingFirst = FixedRescale(K_GetUnscaledFirstDistance(player), 0, 4*DISTVAR, Easing_InCubic, FRACUNIT, 0);

	if ((gametyperules & GTR_CIRCUIT) && !K_Cooperative())
	{
		roulette->dist = FixedMul(roulette->preexpdist, K_EffectiveGradingFactor(player));

		if (roulette->dist < roulette->preexpdist)
		{
			if (roulette->preexpdist - roulette->dist > maxEXPDistanceCut)
			{
				roulette->dist = roulette->preexpdist - maxEXPDistanceCut;
			}
		}
	}

	fixed_t largegamescaler = roulette->playing * 11 + 115; // Spread out item odds in large games for a less insane experience.
	if (franticitems)
		largegamescaler = 100; // Except in Frantic, where you know what you're getting

	UINT32 targetpower = 100 * roulette->dist / largegamescaler; // fill roulette with items around this value!
	if (!(specialstageinfo.valid))
		targetpower = Easing_Linear(crowdingFirst, targetpower, targetpower/2);

	boolean rival = (player->bot && (player->botvars.rival || cv_levelskull.value));
	boolean filterweakitems = true; // strip unusually weak items from reel?
	UINT8 reelsize = 15; // How many items to attempt to add in prepass?
	UINT32 humanscaler = 250; // Scaler that converts "useodds" style distances in odds tables to raw distances. Affects general item distance scale.

	// == ARE THESE ITEMS ALLOWED?
	// We have a fuckton of rules about when items are allowed to show up,
	// like limiting trap items at the end of the race, limiting strong
	// items at the start of the race... Dynamic stuff, not always trivial.
	// We're about to do a bunch of work with items, so let's cache them all.
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		if (!K_TimingPermitsItem(i, roulette))
			permit[i] = false;
		else if (!K_ShouldPlayerAllowItem(i, player))
			permit[i] = false;
		else if (K_GetItemCooldown(i))
			permit[i] = false;
		else if (!K_ItemEnabled(i))
			permit[i] = false;
		else if (K_DenyShieldOdds(i))
			permit[i] = false;
		else if (roulette && roulette->autoroulette == true && K_DenyAutoRouletteOdds(i))
			permit[i] = false;
		else
			permit[i] = true;
	}

	// == ODDS TIME
	// Set up the right item odds for the gametype we're in.

	UINT32 maxpower = 0; // Clamp target power to the lowest item that exists, or some of the math gets hard to reason about.

	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		// NOTE: Battle odds are underspecified, we don't invoke roulettes in this mode!
		if (gametyperules & GTR_BUMPERS)
		{
			powers[i] = humanscaler * K_DynamicItemOddsBattle[i-1][0];
			dupetolerance[i] = K_DynamicItemOddsBattle[i-1][1];
			filterweakitems = false;
		}
		else if (specialstageinfo.valid == true)
		{
			powers[i] = humanscaler * K_DynamicItemOddsSpecial[i-1][0];
			dupetolerance[i] = K_DynamicItemOddsSpecial[i-1][1];
			reelsize = 8; // Smaller roulette in Special because there are much fewer standard items.
			filterweakitems = false;
		}
		else
		{
			powers[i] = humanscaler * K_DynamicItemOddsRace[i-1][0];
			dupetolerance[i] = K_DynamicItemOddsRace[i-1][1];

			// Bias towards attack items when close to the leader, gotta work for the slingshot pass!
			if (K_IsItemSpeed(i) && i != KITEM_SUPERRING)
				powers[i] = Easing_Linear(crowdingFirst, powers[i], 2*powers[i]);
		}

		maxpower = max(maxpower, powers[i]);
	}

	targetpower = min(maxpower, targetpower); // Make sure that we don't fall out of the bottom of the odds table.

	// == GTFO WEIRD ITEMS
	// If something is set to distance 0 in its odds table, that means the item
	// is completely ineligible for the gametype we're in, and should never be selected.
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		if (powers[i] == 0)
		{
			permit[i] = false;
		}
	}

	// == REEL CANDIDATE PREP
	// Dynamic Roulette works by comparing an item's "ideal" distance to our current distance from 1st.
	// It'll pick the most suitable item, do some math, then move on to the next most suitable item.
	// Calculate starting deltas and clear out the "candidates" array that stores what we pick.
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		candidates[i] = 0;
		deltas[i] = min(targetpower - powers[i], powers[i] - targetpower);
	}

	// == "POPCORN" Super Ring in Race.
	// This can appear anywhere from 0 to its specified distance, to pad the
	// reels with non-disruptive catchup (since we have a ton of offensive items
	// and not many front/mid speed items).
	boolean canfiltersuperring = true;
	if ((gametyperules & GTR_CIRCUIT) && (specialstageinfo.valid == false) && K_ItemEnabled(KITEM_SUPERRING))
	{
		if (targetpower > powers[KITEM_SUPERRING])
		{
			permit[KITEM_SUPERRING] = false;
		}
		else
		{
			permit[KITEM_SUPERRING] = true;
			deltas[KITEM_SUPERRING] = 0;
			canfiltersuperring = false;
			roulette->popcorn = (player->position > 1) ? max(1, targetpower/humanscaler/3) : 1;
		}
	}

	// == LONELINESS DETECTION
	// A lot of items suck if no players are nearby to interact with them.
	// Should we bias towards items that get us back to the action?
	// This will set the "loneliness" percentage to be used later.
	UINT32 lonelinessThreshold = 4*DISTVAR; // How far away can we be before items are considered useless?
	UINT32 toFront = lonelinessThreshold; // Distance to the player trying to kill us.
	UINT32 toBack = lonelinessThreshold; // Distance to the player we are trying to kill.
	fixed_t loneliness = 0;

	if (player->position > 1) // Loneliness is expected when frontrunnning, don't influence their item table.
	{
		if ((gametyperules & GTR_CIRCUIT) && specialstageinfo.valid == false)
		{
			player_t *front = NULL;
			player_t *back = NULL;

			// Find the closest enemy players ahead of and behind us.
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] == false || players[i].spectator == true || players[i].exiting || G_SameTeam(&players[i], player))
					continue;

				player_t *check = &players[i];

				if (check->distancetofinish > player->distancetofinish)
				{
					if (!back || check->distancetofinish < back->distancetofinish)
						back = check;
				}
				else if (check->distancetofinish < player->distancetofinish)
				{
					if (!front || check->distancetofinish > front->distancetofinish)
						front = check;
				}
			}

			if (front)
				toFront = K_UndoMapScaling(player->distancetofinish - front->distancetofinish);

			if (back)
				toBack = K_UndoMapScaling(back->distancetofinish - player->distancetofinish);
		}

		// Your relationship to each closest player counts for half, but will be eased later.
		// If you're far from an attacker but close to a defender, that Ballhog is still useful!
		loneliness += min(FRACUNIT/2, FRACUNIT * toFront / lonelinessThreshold / 2);
		loneliness += min(FRACUNIT/2, FRACUNIT * toBack / lonelinessThreshold / 2);

		loneliness = Easing_InCubic(loneliness, 0, FRACUNIT);

		// You are not lonely if you're super close to 1st, even if 3nd is far away.
		if (roulette->preexpdist < lonelinessSuppressor)
		{
			loneliness = FixedRescale(roulette->preexpdist, 0, lonelinessSuppressor, Easing_InCubic, 0, loneliness);
		}

		// Give interaction items a nudge against initial selection if you're lonely..
		for (i = 1; i < NUMKARTRESULTS; i++)
		{
			if (!K_IsItemSpeed(i))
			{
				deltas[i] = Easing_Linear(loneliness, deltas[i], deltas[i] + (4*DISTVAR));
			}
		}
	}

	// == INTRODUCE TRYHARD-EATING PREDATOR
	// If the frontrunner's making a major breakaway, "break the rules"
	// and insert the SPB into the roulette. This doesn't have to be
	// incredibly forceful; there's a truly forced special case above.
	fixed_t spb_odds = K_PercentSPBOdds(roulette, player->position);

	if ((gametyperules & GTR_CIRCUIT)
		&& specialstageinfo.valid == false
		&& (spb_odds > 0) & (spbplace == -1)
		&& (roulette->preexpdist >= powers[KITEM_SPB]) // SPECIAL CASE: Check raw distance instead of EXP-influenced target distance.
		&& !K_GetItemCooldown(KITEM_SPB)
		&& K_ItemEnabled(KITEM_SPB))
	{
		// When reenabling the SPB, we also adjust its delta to ensure that it has good odds of showing up.
		// Players who are _seriously_ struggling are more likely to see Invinc or Rockets, since those items
		// have a lower target distance, so we nudge the SPB towards them.
		permit[KITEM_SPB] = true;
		deltas[KITEM_SPB] = Easing_Linear(spb_odds, deltas[KITEM_SPB], 0);
	}

	// == ITEM SELECTION
	// All the prep work's done: let's pick out a sampler platter of items until we fill the reel.
	UINT8 added = 0; // How many items added so far?
	UINT32 totalreelpower = 0; // How much total item power in the reel? Used for an average later.

	UINT32 basepenalty = 4*DISTVAR; // How much to penalize repicked items, to ensure item variety.
	// BUT, keep the item distribution tighter if we're close to the frontrunner...
	UINT32 penalty = Easing_Linear(crowdingFirst, basepenalty, basepenalty/2);
	if (player->position == 1) // ...unless we ARE the frontrunner.
		penalty = basepenalty;


	for (i = 0; i < reelsize; i++)
	{
		UINT32 lowestdelta = INT32_MAX;
		size_t bestitem = 0;

		// Each rep, get the legal item with the lowest delta...
		for (j = 1; j < NUMKARTRESULTS; j++)
		{
			if (!permit[j])
				continue;

			if (lowestdelta > deltas[j])
			{
				bestitem = j;
				lowestdelta = deltas[j];
			}
		}

		// Couldn't find any eligible items at all? GTFO.
		// (This should never trigger, but you never know with the item switch menu.)
		if (bestitem == 0)
			break;

		// Impose a penalty to this item's delta, to bias against selecting it again.
		// This is naively slashed by an item's "duplicate tolerance":
		// lower tolerance means that an item is less likely to be reselected (it's "rarer").
		UINT32 deltapenalty = penalty*(1+candidates[bestitem])/dupetolerance[bestitem];

		// Power items get better odds in frantic, or if you're the rival.
		// (For the rival, this is way more likely to matter at lower skills, where they're
		// worse at selecting their item—but it always matters in frantic gameplay.)
		if (K_IsItemPower(bestitem) && rival)
			deltapenalty = 3 * deltapenalty / 4;
#if 0
		if (K_IsItemPower(bestitem) && franticitems)
			deltapenalty = 3 * deltapenalty / 4;
#endif

		// Conversely, if we're lonely, try not to reselect an item that wouldn't be useful to us
		// without any players to use it on.
		if (!K_IsItemSpeed(bestitem))
			deltapenalty = Easing_Linear(loneliness, deltapenalty, 3*deltapenalty);

		// Draw complex odds debugger. This one breaks down all the calcs in order.
		if (cv_kartdebugdistribution.value > 1)
		{
			UINT16 BASE_X = 18;
			UINT16 BASE_Y = 5+12*i;
			INT32 FLAGS = V_SNAPTOTOP|V_SNAPTOLEFT;
			V_DrawRightAlignedThinString(BASE_X + 35, BASE_Y, FLAGS, va("P%d", powers[bestitem]/humanscaler));
			V_DrawRightAlignedThinString(BASE_X + 65, BASE_Y, FLAGS, va("D%d", deltas[bestitem]/humanscaler));
			V_DrawRightAlignedThinString(BASE_X + 20, BASE_Y, FLAGS, va("%d", dupetolerance[bestitem]));
			V_DrawFixedPatch(BASE_X*FRACUNIT, (BASE_Y-7)*FRACUNIT, (FRACUNIT >> 1), FLAGS, K_GetSmallStaticCachedItemPatch(bestitem), NULL);
			UINT8 amount = K_ItemResultToAmount(bestitem, roulette);
			if (amount > 1)
				V_DrawThinString(BASE_X, BASE_Y, FLAGS, va("x%d", amount));
		}

		// Add the selected item to our list of candidates and update its working delta.
		candidates[bestitem]++;
		deltas[bestitem] += deltapenalty;

		// Then update our ongoing average of the reel's power.
		totalreelpower += powers[bestitem];
		added++;
	}

	// No items?!
	if (added == 0)
	{
		// Guess we're making circles now.
		// Just do something that doesn't crash.
		K_AddItemToReel(player, roulette, singleItem);
		return;
	}

	// Frontrunner roulette is precise, no need to filter it.
	if (player->position <= 1)
		filterweakitems = false;

	UINT8 debugcount = 0; // For the "simple" odds debugger.
	UINT32 meanreelpower = totalreelpower/max(added, 1); // Average power for the "moth filter".
	UINT32 maxreduction = -1 * min(2 * DISTVAR, meanreelpower/2);

	// == PREP FOR ADDING TO THE ROULETTE REEL
	// Sal's prior work for this is rock-solid.
	// This fills the spawnChance array with a rolling count of items,
	// so that we can loop upward through it until we hit our random index.
	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		// If an item is far too week for this reel, reject it.
		// This can happen in regions of the odds with a lot of items that
		// don't really like to be duplicated. Favor the player; high-rolling
		// feels exciting, low-rolling feels punishing!
		boolean reject = (filterweakitems) && (powers[i] + DISTVAR < meanreelpower);

		// If we're far away from interactions, be extra aggressive about tossing attack items.
		if (filterweakitems && !reject && !K_IsItemSpeed(i))
			reject = (powers[i] + Easing_Linear(loneliness, DISTVAR, maxreduction) < meanreelpower);

		// Popcorn Super Ring is always strong enough, we put it there on purpose.
		if (i == KITEM_SUPERRING && !canfiltersuperring)
			reject = false;

		// Before we actually apply that rejection, draw the simple odds debugger.
		// This one is just to watch the distribution for vibes as you drive around.
		if (cv_kartdebugdistribution.value && candidates[i])
		{
			UINT16 BASE_X = 280;
			UINT16 BASE_Y = 5+12*debugcount;
			INT32 FLAGS = V_SNAPTOTOP|V_SNAPTORIGHT;

			if (reject)
				FLAGS |= V_TRANSLUCENT;
			V_DrawRightAlignedThinString(BASE_X - 12, 5, FLAGS, va("TP %d", targetpower/humanscaler));
			V_DrawRightAlignedThinString(BASE_X - 12, 5+12, FLAGS, va("FB %d / %d", toFront, toBack));
			V_DrawRightAlignedThinString(BASE_X - 12, 5+24, FLAGS, va("L %d / CF %d", loneliness, crowdingFirst));
			V_DrawRightAlignedThinString(BASE_X - 12, 5+36, FLAGS, va("D %d / %d", roulette->preexpdist, roulette->dist));
			for(UINT8 k = 0; k < candidates[i]; k++)
				V_DrawFixedPatch((BASE_X + 3*k)*FRACUNIT, (BASE_Y-7)*FRACUNIT, (FRACUNIT >> 1), FLAGS, K_GetSmallStaticCachedItemPatch(i), NULL);
			UINT8 amount = K_ItemResultToAmount(i, roulette);
			if (amount > 1)
				V_DrawThinString(BASE_X, BASE_Y, FLAGS, va("x%d", amount));

			/*
			if (reject)
				V_DrawThinString(BASE_X, BASE_Y, FLAGS|V_60TRANS, va("WEAK"));
			*/
			debugcount++;
		}

		// Okay, apply the rejection now.
		if (reject)
			candidates[i] = 0;

		// Bump totalSpawnChance, write that rolling counter, and move on.
		spawnChance[i] = (
			totalSpawnChance += candidates[i]
		);
	}

	if (dryrun) // We're being called from the debugger on a view conditional!
		return; // This is net unsafe if we do things with side effects. GTFO!

	// == FINALLY ADD THIS SHIT TO THE REEL
	// Super simple: generate a random index,
	// count up until we hit that index,
	// insert that item and decrement everything after.
	while (totalSpawnChance > 0)
	{
		rngRoll = P_RandomKey(PR_ITEM_ROULETTE, totalSpawnChance);
		for (i = 1; i < NUMKARTRESULTS && spawnChance[i] <= rngRoll; i++)
		{
			continue;
		}

		K_AddItemToReel(player, roulette, i);

		for (; i < NUMKARTRESULTS; i++)
		{
			if (spawnChance[i] > 0)
			{
				spawnChance[i]--;
			}
		}

		totalSpawnChance--;
	}

	K_FixEmptyRoulette(player, roulette);
}

/*--------------------------------------------------
	void K_StartItemRoulette(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_StartItemRoulette(player_t *const player, boolean ringbox)
{
	itemroulette_t *const roulette = &player->itemRoulette;
	size_t i;

	K_FillItemRoulette(player, roulette, ringbox);

	if (roulette->autoroulette)
		roulette->index = P_RandomRange(PR_AUTOROULETTE, 0, roulette->itemList.len - 1);

	if (K_PlayerUsesBotMovement(player) == true)
	{
		K_BotPickItemPriority(player);
	}

	// Prevent further duplicates of items that
	// are intended to only have one out at a time.
	for (i = 0; i < roulette->itemList.len; i++)
	{
		kartitems_t item = roulette->itemList.items[i];
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
	void K_KartGetItemResult(player_t *const player, kartitems_t getitem)

		See header file for description.
--------------------------------------------------*/
void K_KartGetItemResult(player_t *const player, kartitems_t getitem)
{
	if (K_ItemSingularity(getitem) == true)
	{
		K_SetItemCooldown(getitem, 20*TICRATE);
	}

	player->botvars.itemdelay = TICRATE;
	player->botvars.itemconfirm = 0;

	player->itemtype = K_ItemResultToType(getitem);
	UINT8 itemamount = K_ItemResultToAmount(getitem, &player->itemRoulette);
	if (cv_kartdebugitem.value != KITEM_NONE && cv_kartdebugitem.value == player->itemtype && cv_kartdebugamount.value > 1)
		itemamount = cv_kartdebugamount.value;

	K_SetPlayerItemAmount(player, itemamount);

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

	if (roulette->itemList.len == 0
#ifndef ITEM_LIST_SIZE
		|| roulette->itemList.items == NULL
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
					roulette->index = roulette->index == 0 ? roulette->itemList.len - 1 : roulette->index - 1; // Roll the roulette index back...
					roulette->tics = 0; // And just in case our delay is SO high that a fast roulette needs to roll back again...
				}
				else
				{
					break;
				}
			}

			// And one more nudge for the remaining delay.
			roulette->tics = (roulette->tics + fudgedDelay) % roulette->speed;

			INT32 finalItem = roulette->itemList.items[ roulette->index ];

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
		roulette->index = (roulette->index + 1) % roulette->itemList.len;
		roulette->tics = roulette->speed;

		// This makes the roulette produce the random noises.
		roulette->sound = (roulette->sound + 1) % 8;

		if (K_IsPlayingDisplayPlayer(player))
		{
			if (roulette->ringbox)
				S_StartSound(NULL, sfx_s240);
			else
				S_StartSound(NULL, sfx_itrol1 + roulette->sound);

			if (roulette->index == 0 && roulette->itemList.len > 1)
			{
				S_StartSound(NULL, sfx_kc50);
				S_StartSound(NULL, sfx_kc50);
			}

		}
	}
	else
	{
		roulette->tics--;
	}
}
