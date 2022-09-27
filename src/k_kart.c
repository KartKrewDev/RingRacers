// SONIC ROBO BLAST 2 KART ~ ZarroTsu
//-----------------------------------------------------------------------------
/// \file  k_kart.c
/// \brief SRB2kart general.
///        All of the SRB2kart-unique stuff.

#include "k_kart.h"
#include "k_battle.h"
#include "k_boss.h"
#include "k_pwrlv.h"
#include "k_color.h"
#include "k_respawn.h"
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

#include "k_waypoint.h"
#include "k_bot.h"
#include "k_hud.h"
#include "k_terrain.h"
#include "k_director.h"
#include "k_collide.h"
#include "k_follower.h"
#include "k_objects.h"

// SOME IMPORTANT VARIABLES DEFINED IN DOOMDEF.H:
// gamespeed is cc (0 for easy, 1 for normal, 2 for hard)
// franticitems is Frantic Mode items, bool
// encoremode is Encore Mode (duh), bool
// comeback is Battle Mode's karma comeback, also bool
// battlewanted is an array of the WANTED player nums, -1 for no player in that slot
// mapreset is set when enough players fill an empty server

void K_TimerReset(void)
{
	starttime = introtime = 3;
	numbulbs = 1;
}

void K_TimerInit(void)
{
	UINT8 i;
	UINT8 numPlayers = 0;//, numspec = 0;

	if (!bossinfo.boss)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
			{
				continue;
			}

			if (players[i].spectator == true)
			{
				//numspec++;
				continue;
			}

			numPlayers++;
		}

		if (numPlayers >= 2)
		{
			rainbowstartavailable = true;
		}
		else
		{
			rainbowstartavailable = false;
		}

		// No intro in Record Attack / 1v1
		// Leave unset for the value in K_TimerReset
		if (numPlayers > 2)
		{
			introtime = (108) + 5; // 108 for rotation, + 5 for white fade
		}

		numbulbs = 5;

		if (numPlayers > 2)
		{
			numbulbs += (numPlayers-2);
		}

		starttime = (introtime + (3*TICRATE)) + ((2*TICRATE) + (numbulbs * bulbtime)); // Start countdown time, + buffer time
	}

	// NOW you can try to spawn in the Battle capsules, if there's not enough players for a match
	K_BattleInit();
	//CONS_Printf("numbulbs set to %d (%d players, %d spectators) on tic %d\n", numbulbs, numPlayers, numspec, leveltime);
}

UINT32 K_GetPlayerDontDrawFlag(player_t *player)
{
	UINT32 flag = 0;

	if (player == &players[displayplayers[0]])
		flag = RF_DONTDRAWP1;
	else if (r_splitscreen >= 1 && player == &players[displayplayers[1]])
		flag = RF_DONTDRAWP2;
	else if (r_splitscreen >= 2 && player == &players[displayplayers[2]])
		flag = RF_DONTDRAWP3;
	else if (r_splitscreen >= 3 && player == &players[displayplayers[3]])
		flag = RF_DONTDRAWP4;

	return flag;
}

player_t *K_GetItemBoxPlayer(mobj_t *mobj)
{
	fixed_t closest = INT32_MAX;
	player_t *player = NULL;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!(playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo) && !players[i].spectator))
		{
			continue;
		}

		// Always use normal item box rules -- could pass in "2" for fakes but they blend in better like this
		if (P_CanPickupItem(&players[i], 1))
		{
			fixed_t dist = P_AproxDistance(P_AproxDistance(
				players[i].mo->x - mobj->x,
				players[i].mo->y - mobj->y),
				players[i].mo->z - mobj->z
			);

			if (dist > 8192*mobj->scale)
			{
				continue;
			}

			if (dist < closest)
			{
				player = &players[i];
				closest = dist;
			}
		}
	}

	return player;
}

// Angle reflection used by springs & speed pads
angle_t K_ReflectAngle(angle_t yourangle, angle_t theirangle, fixed_t yourspeed, fixed_t theirspeed)
{
	INT32 angoffset;
	boolean subtract = false;

	angoffset = yourangle - theirangle;

	if ((angle_t)angoffset > ANGLE_180)
	{
		// Flip on wrong side
		angoffset = InvAngle((angle_t)angoffset);
		subtract = !subtract;
	}

	// Fix going directly against the spring's angle sending you the wrong way
	if ((angle_t)angoffset > ANGLE_90)
	{
		angoffset = ANGLE_180 - angoffset;
	}

	// Offset is reduced to cap it (90 / 2 = max of 45 degrees)
	angoffset /= 2;

	// Reduce further based on how slow your speed is compared to the spring's speed
	// (set both to 0 to ignore this)
	if (theirspeed != 0 && yourspeed != 0)
	{
		if (theirspeed > yourspeed)
		{
			angoffset = FixedDiv(angoffset, FixedDiv(theirspeed, yourspeed));
		}
	}

	if (subtract)
		angoffset = (signed)(theirangle) - angoffset;
	else
		angoffset = (signed)(theirangle) + angoffset;

	return (angle_t)angoffset;
}

//{ SRB2kart Net Variables

void K_RegisterKartStuff(void)
{
	CV_RegisterVar(&cv_sneaker);
	CV_RegisterVar(&cv_rocketsneaker);
	CV_RegisterVar(&cv_invincibility);
	CV_RegisterVar(&cv_banana);
	CV_RegisterVar(&cv_eggmanmonitor);
	CV_RegisterVar(&cv_orbinaut);
	CV_RegisterVar(&cv_jawz);
	CV_RegisterVar(&cv_mine);
	CV_RegisterVar(&cv_landmine);
	CV_RegisterVar(&cv_droptarget);
	CV_RegisterVar(&cv_ballhog);
	CV_RegisterVar(&cv_selfpropelledbomb);
	CV_RegisterVar(&cv_grow);
	CV_RegisterVar(&cv_shrink);
	CV_RegisterVar(&cv_lightningshield);
	CV_RegisterVar(&cv_bubbleshield);
	CV_RegisterVar(&cv_flameshield);
	CV_RegisterVar(&cv_hyudoro);
	CV_RegisterVar(&cv_pogospring);
	CV_RegisterVar(&cv_superring);
	CV_RegisterVar(&cv_kitchensink);

	CV_RegisterVar(&cv_dualsneaker);
	CV_RegisterVar(&cv_triplesneaker);
	CV_RegisterVar(&cv_triplebanana);
	CV_RegisterVar(&cv_decabanana);
	CV_RegisterVar(&cv_tripleorbinaut);
	CV_RegisterVar(&cv_quadorbinaut);
	CV_RegisterVar(&cv_dualjawz);

	CV_RegisterVar(&cv_kartminimap);
	CV_RegisterVar(&cv_kartcheck);
	CV_RegisterVar(&cv_kartspeed);
	CV_RegisterVar(&cv_kartbumpers);
	CV_RegisterVar(&cv_kartfrantic);
	CV_RegisterVar(&cv_kartcomeback);
	CV_RegisterVar(&cv_kartencore);
	CV_RegisterVar(&cv_kartvoterulechanges);
	CV_RegisterVar(&cv_kartgametypepreference);
	CV_RegisterVar(&cv_kartspeedometer);
	CV_RegisterVar(&cv_kartvoices);
	CV_RegisterVar(&cv_kartbot);
	CV_RegisterVar(&cv_karteliminatelast);
	CV_RegisterVar(&cv_kartusepwrlv);
	CV_RegisterVar(&cv_votetime);

	CV_RegisterVar(&cv_kartdebugitem);
	CV_RegisterVar(&cv_kartdebugamount);
	CV_RegisterVar(&cv_kartallowgiveitem);
	CV_RegisterVar(&cv_kartdebugdistribution);
	CV_RegisterVar(&cv_kartdebughuddrop);
	CV_RegisterVar(&cv_kartdebugwaypoints);
	CV_RegisterVar(&cv_kartdebugbotpredict);

	CV_RegisterVar(&cv_kartdebugcheckpoint);
	CV_RegisterVar(&cv_kartdebugnodes);
	CV_RegisterVar(&cv_kartdebugcolorize);
	CV_RegisterVar(&cv_kartdebugdirector);
}

//}

boolean K_IsPlayerLosing(player_t *player)
{
	INT32 winningpos = 1;
	UINT8 i, pcount = 0;

	if (battlecapsules && player->bumpers <= 0)
		return true; // DNF in break the capsules

	if (bossinfo.boss)
		return (player->bumpers <= 0); // anything short of DNF is COOL

	if (player->position == 1)
		return false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		if (players[i].position > pcount)
			pcount = players[i].position;
	}

	if (pcount <= 1)
		return false;

	winningpos = pcount/2;
	if (pcount % 2) // any remainder?
		winningpos++;

	return (player->position > winningpos);
}

fixed_t K_GetKartGameSpeedScalar(SINT8 value)
{
	// Easy = 81.25%
	// Normal = 100%
	// Hard = 118.75%
	// Nightmare = 137.5% ?!?!
	return ((13 + (3*value)) << FRACBITS) / 16;
}

//{ SRB2kart Roulette Code - Position Based

consvar_t *KartItemCVars[NUMKARTRESULTS-1] =
{
	&cv_sneaker,
	&cv_rocketsneaker,
	&cv_invincibility,
	&cv_banana,
	&cv_eggmanmonitor,
	&cv_orbinaut,
	&cv_jawz,
	&cv_mine,
	&cv_landmine,
	&cv_ballhog,
	&cv_selfpropelledbomb,
	&cv_grow,
	&cv_shrink,
	&cv_lightningshield,
	&cv_bubbleshield,
	&cv_flameshield,
	&cv_hyudoro,
	&cv_pogospring,
	&cv_superring,
	&cv_kitchensink,
	&cv_droptarget,
	&cv_dualsneaker,
	&cv_triplesneaker,
	&cv_triplebanana,
	&cv_decabanana,
	&cv_tripleorbinaut,
	&cv_quadorbinaut,
	&cv_dualjawz
};

#define NUMKARTODDS 	80

// Less ugly 2D arrays
static UINT8 K_KartItemOddsRace[NUMKARTRESULTS-1][8] =
{
				//P-Odds	 0  1  2  3  4  5  6  7
			   /*Sneaker*/ { 0, 0, 2, 4, 6, 0, 0, 0 }, // Sneaker
		/*Rocket Sneaker*/ { 0, 0, 0, 0, 0, 2, 4, 6 }, // Rocket Sneaker
		 /*Invincibility*/ { 0, 0, 0, 0, 3, 4, 5, 7 }, // Invincibility
				/*Banana*/ { 2, 3, 1, 0, 0, 0, 0, 0 }, // Banana
		/*Eggman Monitor*/ { 1, 2, 0, 0, 0, 0, 0, 0 }, // Eggman Monitor
			  /*Orbinaut*/ { 5, 5, 2, 2, 0, 0, 0, 0 }, // Orbinaut
				  /*Jawz*/ { 0, 4, 2, 1, 0, 0, 0, 0 }, // Jawz
				  /*Mine*/ { 0, 3, 3, 1, 0, 0, 0, 0 }, // Mine
			 /*Land Mine*/ { 3, 0, 0, 0, 0, 0, 0, 0 }, // Land Mine
			   /*Ballhog*/ { 0, 0, 2, 2, 0, 0, 0, 0 }, // Ballhog
   /*Self-Propelled Bomb*/ { 0, 0, 0, 0, 0, 0, 0, 0 }, // Self-Propelled Bomb
				  /*Grow*/ { 0, 0, 0, 1, 2, 3, 0, 0 }, // Grow
				/*Shrink*/ { 0, 0, 0, 0, 0, 1, 3, 2 }, // Shrink
	  /*Lightning Shield*/ { 1, 0, 0, 0, 0, 0, 0, 0 }, // Lightning Shield
		 /*Bubble Shield*/ { 0, 1, 2, 1, 0, 0, 0, 0 }, // Bubble Shield
		  /*Flame Shield*/ { 0, 0, 0, 0, 0, 1, 3, 5 }, // Flame Shield
			   /*Hyudoro*/ { 3, 0, 0, 0, 0, 0, 0, 0 }, // Hyudoro
		   /*Pogo Spring*/ { 0, 0, 0, 0, 0, 0, 0, 0 }, // Pogo Spring
			/*Super Ring*/ { 2, 1, 1, 0, 0, 0, 0, 0 }, // Super Ring
		  /*Kitchen Sink*/ { 0, 0, 0, 0, 0, 0, 0, 0 }, // Kitchen Sink
		   /*Drop Target*/ { 3, 0, 0, 0, 0, 0, 0, 0 }, // Drop Target
			/*Sneaker x2*/ { 0, 0, 2, 2, 2, 0, 0, 0 }, // Sneaker x2
			/*Sneaker x3*/ { 0, 0, 0, 1, 6, 9, 5, 0 }, // Sneaker x3
			 /*Banana x3*/ { 0, 1, 1, 0, 0, 0, 0, 0 }, // Banana x3
			/*Banana x10*/ { 0, 0, 0, 1, 0, 0, 0, 0 }, // Banana x10
		   /*Orbinaut x3*/ { 0, 0, 1, 0, 0, 0, 0, 0 }, // Orbinaut x3
		   /*Orbinaut x4*/ { 0, 0, 0, 2, 0, 0, 0, 0 }, // Orbinaut x4
			   /*Jawz x2*/ { 0, 0, 1, 2, 1, 0, 0, 0 }  // Jawz x2
};

static UINT8 K_KartItemOddsBattle[NUMKARTRESULTS][2] =
{
				//P-Odds	 0  1
			   /*Sneaker*/ { 2, 1 }, // Sneaker
		/*Rocket Sneaker*/ { 0, 0 }, // Rocket Sneaker
		 /*Invincibility*/ { 4, 1 }, // Invincibility
				/*Banana*/ { 0, 0 }, // Banana
		/*Eggman Monitor*/ { 1, 0 }, // Eggman Monitor
			  /*Orbinaut*/ { 8, 0 }, // Orbinaut
				  /*Jawz*/ { 8, 1 }, // Jawz
				  /*Mine*/ { 6, 1 }, // Mine
			 /*Land Mine*/ { 2, 0 }, // Land Mine
			   /*Ballhog*/ { 2, 1 }, // Ballhog
   /*Self-Propelled Bomb*/ { 0, 0 }, // Self-Propelled Bomb
				  /*Grow*/ { 2, 1 }, // Grow
				/*Shrink*/ { 0, 0 }, // Shrink
	  /*Lightning Shield*/ { 4, 0 }, // Lightning Shield
		 /*Bubble Shield*/ { 1, 0 }, // Bubble Shield
		  /*Flame Shield*/ { 1, 0 }, // Flame Shield
			   /*Hyudoro*/ { 2, 0 }, // Hyudoro
		   /*Pogo Spring*/ { 3, 0 }, // Pogo Spring
			/*Super Ring*/ { 0, 0 }, // Super Ring
		  /*Kitchen Sink*/ { 0, 0 }, // Kitchen Sink
		   /*Drop Target*/ { 2, 0 }, // Drop Target
			/*Sneaker x2*/ { 0, 0 }, // Sneaker x2
			/*Sneaker x3*/ { 0, 1 }, // Sneaker x3
			 /*Banana x3*/ { 0, 0 }, // Banana x3
			/*Banana x10*/ { 1, 1 }, // Banana x10
		   /*Orbinaut x3*/ { 2, 0 }, // Orbinaut x3
		   /*Orbinaut x4*/ { 1, 1 }, // Orbinaut x4
			   /*Jawz x2*/ { 5, 1 }  // Jawz x2
};

#define DISTVAR (2048) // Magic number distance for use with item roulette tiers
#define SPBSTARTDIST (6*DISTVAR) // Distance when SPB can start appearing
#define SPBFORCEDIST (12*DISTVAR) // Distance when SPB is forced onto the next person who rolls an item
#define ENDDIST (12*DISTVAR) // Distance when the game stops giving you bananas

// Array of states to pick the starting point of the animation, based on the actual time left for invincibility.
static INT32 K_SparkleTrailStartStates[KART_NUMINVSPARKLESANIM][2] = {
	{S_KARTINVULN12, S_KARTINVULNB12},
	{S_KARTINVULN11, S_KARTINVULNB11},
	{S_KARTINVULN10, S_KARTINVULNB10},
	{S_KARTINVULN9, S_KARTINVULNB9},
	{S_KARTINVULN8, S_KARTINVULNB8},
	{S_KARTINVULN7, S_KARTINVULNB7},
	{S_KARTINVULN6, S_KARTINVULNB6},
	{S_KARTINVULN5, S_KARTINVULNB5},
	{S_KARTINVULN4, S_KARTINVULNB4},
	{S_KARTINVULN3, S_KARTINVULNB3},
	{S_KARTINVULN2, S_KARTINVULNB2},
	{S_KARTINVULN1, S_KARTINVULNB1}
};

INT32 K_GetShieldFromItem(INT32 item)
{
	switch (item)
	{
		case KITEM_LIGHTNINGSHIELD: return KSHIELD_LIGHTNING;
		case KITEM_BUBBLESHIELD: return KSHIELD_BUBBLE;
		case KITEM_FLAMESHIELD: return KSHIELD_FLAME;
		default: return KSHIELD_NONE;
	}
}

SINT8 K_ItemResultToType(SINT8 getitem)
{
	if (getitem <= 0 || getitem >= NUMKARTRESULTS) // Sad (Fallback)
	{
		if (getitem != 0)
		{
			CONS_Printf("ERROR: K_GetItemResultToItemType - Item roulette gave bad item (%d) :(\n", getitem);
		}

		return KITEM_SAD;
	}

	if (getitem >= NUMKARTITEMS)
	{
		switch (getitem)
		{
			case KRITEM_DUALSNEAKER:
			case KRITEM_TRIPLESNEAKER:
				return KITEM_SNEAKER;

			case KRITEM_TRIPLEBANANA:
			case KRITEM_TENFOLDBANANA:
				return KITEM_BANANA;

			case KRITEM_TRIPLEORBINAUT:
			case KRITEM_QUADORBINAUT:
				return KITEM_ORBINAUT;

			case KRITEM_DUALJAWZ:
				return KITEM_JAWZ;

			default:
				I_Error("Bad item cooldown redirect for result %d\n", getitem);
				break;
		}
	}

	return getitem;
}

UINT8 K_ItemResultToAmount(SINT8 getitem)
{
	switch (getitem)
	{
		case KRITEM_DUALSNEAKER:
		case KRITEM_DUALJAWZ:
			return 2;

		case KRITEM_TRIPLESNEAKER:
		case KRITEM_TRIPLEBANANA:
		case KRITEM_TRIPLEORBINAUT:
			return 3;

		case KRITEM_QUADORBINAUT:
			return 4;

		case KITEM_BALLHOG: // Not a special result, but has a special amount
			return 5;

		case KRITEM_TENFOLDBANANA:
			return 10;

		default:
			return 1;
	}
}

tic_t K_GetItemCooldown(SINT8 itemResult)
{
	SINT8 itemType = K_ItemResultToType(itemResult);

	if (itemType < 1 || itemType >= NUMKARTITEMS)
	{
		return 0;
	}

	return itemCooldowns[itemType - 1];
}

void K_SetItemCooldown(SINT8 itemResult, tic_t time)
{
	SINT8 itemType = K_ItemResultToType(itemResult);

	if (itemType < 1 || itemType >= NUMKARTITEMS)
	{
		return;
	}

	itemCooldowns[itemType - 1] = max(itemCooldowns[itemType - 1], time);
}

void K_RunItemCooldowns(void)
{
	size_t i;

	for (i = 0; i < NUMKARTITEMS-1; i++)
	{
		if (itemCooldowns[i] > 0)
		{
			itemCooldowns[i]--;
		}
	}
}


/**	\brief	Item Roulette for Kart

	\param	player		player
	\param	getitem		what item we're looking for

	\return	void
*/
static void K_KartGetItemResult(player_t *player, SINT8 getitem)
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

/**	\brief	Item Roulette for Kart

	\param	player	player object passed from P_KartPlayerThink

	\return	void
*/

INT32 K_KartGetItemOdds(
	UINT8 pos, SINT8 item,
	UINT32 ourDist,
	fixed_t mashed,
	boolean bot, boolean rival)
{
	INT32 newodds;
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

	INT32 shieldtype = KSHIELD_NONE;

	I_Assert(item > KITEM_NONE); // too many off by one scenarioes.
	I_Assert(KartItemCVars[NUMKARTRESULTS-2] != NULL); // Make sure this exists

	if (!KartItemCVars[item-1]->value && !modeattacking)
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
		newodds = K_KartItemOddsBattle[item-1][pos];
	}
	else
	{
		I_Assert(pos < 8); // Ditto
		newodds = K_KartItemOddsRace[item-1][pos];
	}

	// Base multiplication to ALL item odds to simulate fractional precision
	newodds *= 4;

	shieldtype = K_GetShieldFromItem(item);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (!(gametyperules & GTR_BUMPERS) || players[i].bumpers)
			pingame++;

		if (players[i].exiting)
			pexiting++;

		if (shieldtype != KSHIELD_NONE && shieldtype == K_GetShieldFromItem(players[i].itemtype))
		{
			// Don't allow more than one of each shield type at a time
			return 0;
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
			notNearEnd = true;
			break;

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
			powerItem = true;
			break;

		case KRITEM_TRIPLEBANANA:
		case KRITEM_TENFOLDBANANA:
			powerItem = true;
			notNearEnd = true;
			break;

		case KITEM_INVINCIBILITY:
		case KITEM_MINE:
		case KITEM_GROW:
		case KITEM_BUBBLESHIELD:
		case KITEM_FLAMESHIELD:
			cooldownOnStart = true;
			powerItem = true;
			break;

		case KITEM_SPB:
			cooldownOnStart = true;
			notNearEnd = true;

			if (firstDist < ENDDIST*2 // No SPB when 1st is almost done
				|| isFirst == true) // No SPB for 1st ever
			{
				newodds = 0;
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

				newodds = FixedMul(maxOdds * 4, multiplier);
			}
			break;

		case KITEM_SHRINK:
			cooldownOnStart = true;
			powerItem = true;
			notNearEnd = true;

			if (pingame-1 <= pexiting)
				newodds = 0;
			break;

		case KITEM_LIGHTNINGSHIELD:
			cooldownOnStart = true;
			powerItem = true;

			if (spbplace != -1)
				newodds = 0;
			break;

		default:
			break;
	}

	if (newodds == 0)
	{
		// Nothing else we want to do with odds matters at this point :p
		return newodds;
	}

	
	if ((cooldownOnStart == true) && (leveltime < (30*TICRATE)+starttime))
	{
		// This item should not appear at the beginning of a race. (Usually really powerful crowd-breaking items)
		newodds = 0;
	}
	else if ((notNearEnd == true) && (ourDist < ENDDIST))
	{
		// This item should not appear at the end of a race. (Usually trap items that lose their effectiveness)
		newodds = 0;
	}
	else if (powerItem == true)
	{
		// This item is a "power item". This activates "frantic item" toggle related functionality.
		fixed_t fracOdds = newodds * FRACUNIT;

		if (franticitems == true)
		{
			// First, power items multiply their odds by 2 if frantic items are on; easy-peasy.
			fracOdds *= 2;
		}

		if (rival == true)
		{
			// The Rival bot gets frantic-like items, also :p
			fracOdds *= 2;
		}

		fracOdds = FixedMul(fracOdds, FRACUNIT + K_ItemOddsScale(pingame));

		if (mashed > 0)
		{
			// Lastly, it *divides* it based on your mashed value, so that power items are less likely when you mash.
			fracOdds = FixedDiv(fracOdds, FRACUNIT + mashed);
		}

		newodds = fracOdds / FRACUNIT;
	}

	return newodds;
}

//{ SRB2kart Roulette Code - Distance Based, yes waypoints

UINT8 K_FindUseodds(player_t *player, fixed_t mashed, UINT32 pdis, UINT8 bestbumper)
{
	UINT8 i;
	UINT8 useodds = 0;
	UINT8 disttable[14];
	UINT8 distlen = 0;
	boolean oddsvalid[8];

	// Unused now, oops :V
	(void)bestbumper;

	for (i = 0; i < 8; i++)
	{
		UINT8 j;
		boolean available = false;

		if (gametype == GT_BATTLE && i > 1)
		{
			oddsvalid[i] = false;
			break;
		}

		for (j = 1; j < NUMKARTRESULTS; j++)
		{
			if (K_KartGetItemOdds(
					i, j,
					player->distancetofinish,
					mashed,
					player->bot, (player->bot && player->botvars.rival)
				) > 0)
			{
				available = true;
				break;
			}
		}

		oddsvalid[i] = available;
	}

#define SETUPDISTTABLE(odds, num) \
	if (oddsvalid[odds]) \
		for (i = num; i; --i) \
			disttable[distlen++] = odds;

	if (gametype == GT_BATTLE) // Battle Mode
	{
		if (player->roulettetype == 1 && oddsvalid[1] == true)
		{
			// 1 is the extreme odds of player-controlled "Karma" items
			useodds = 1;
		}
		else
		{
			useodds = 0;

			if (oddsvalid[0] == false && oddsvalid[1] == true)
			{
				// try to use karma odds as a fallback
				useodds = 1;
			}
		}
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

		if (pdis == 0)
			useodds = disttable[0];
		else if (pdis > DISTVAR * ((12 * distlen) / 14))
			useodds = disttable[distlen-1];
		else
		{
			for (i = 1; i < 13; i++)
			{
				if (pdis <= DISTVAR * ((i * distlen) / 14))
				{
					useodds = disttable[((i * distlen) / 14)];
					break;
				}
			}
		}
	}

#undef SETUPDISTTABLE

	return useodds;
}

INT32 K_GetRollingRouletteItem(player_t *player)
{
	static UINT8 translation[NUMKARTITEMS-1];
	static UINT16 roulette_size;

	static INT16 odds_cached = -1;

	// Race odds have more columns than Battle
	const UINT8 EMPTYODDS[sizeof K_KartItemOddsRace[0]] = {0};

	if (odds_cached != gametype)
	{
		UINT8 *odds_row;
		size_t odds_row_size;

		UINT8 i;

		roulette_size = 0;

		if (gametype == GT_BATTLE)
		{
			odds_row = K_KartItemOddsBattle[0];
			odds_row_size = sizeof K_KartItemOddsBattle[0];
		}
		else
		{
			odds_row = K_KartItemOddsRace[0];
			odds_row_size = sizeof K_KartItemOddsRace[0];
		}

		for (i = 1; i < NUMKARTITEMS; ++i)
		{
			if (memcmp(odds_row, EMPTYODDS, odds_row_size))
			{
				translation[roulette_size] = i;
				roulette_size++;
			}

			odds_row += odds_row_size;
		}

		roulette_size *= 3;
		odds_cached = gametype;
	}

	return translation[(player->itemroulette % roulette_size) / 3];
}

boolean K_ForcedSPB(player_t *player)
{
	player_t *first = NULL;
	player_t *second = NULL;
	UINT32 secondToFirst = UINT32_MAX;
	UINT8 pingame = 0;
	UINT8 i;

	if (!cv_selfpropelledbomb.value)
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

static void K_KartItemRoulette(player_t *player, ticcmd_t *cmd)
{
	INT32 i;
	UINT8 pingame = 0;
	UINT8 roulettestop;
	UINT32 pdis = 0;
	UINT8 useodds = 0;
	INT32 spawnchance[NUMKARTRESULTS];
	INT32 totalspawnchance = 0;
	UINT8 bestbumper = 0;
	fixed_t mashed = 0;

	// This makes the roulette cycle through items - if this is 0, you shouldn't be here.
	if (!player->itemroulette)
		return;
	player->itemroulette++;

	// Gotta check how many players are active at this moment.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		pingame++;

		if (players[i].bumpers > bestbumper)
			bestbumper = players[i].bumpers;
	}

	// This makes the roulette produce the random noises.
	if ((player->itemroulette % 3) == 1 && P_IsDisplayPlayer(player) && !demo.freecam)
	{
#define PLAYROULETTESND S_StartSound(NULL, sfx_itrol1 + ((player->itemroulette / 3) % 8))
		for (i = 0; i <= r_splitscreen; i++)
		{
			if (player == &players[displayplayers[i]] && players[displayplayers[i]].itemroulette)
				PLAYROULETTESND;
		}
#undef PLAYROULETTESND
	}

	roulettestop = TICRATE + (3*(pingame - player->position));

	// If the roulette finishes or the player presses BT_ATTACK, stop the roulette and calculate the item.
	// I'm returning via the exact opposite, however, to forgo having another bracket embed. Same result either way, I think.
	// Finally, if you get past this check, now you can actually start calculating what item you get.
	if ((cmd->buttons & BT_ATTACK) && (player->itemroulette >= roulettestop)
		&& !(player->pflags & (PF_ITEMOUT|PF_EGGMANOUT|PF_USERINGS)))
	{
		// Mashing reduces your chances for the good items
		mashed = FixedDiv((player->itemroulette)*FRACUNIT, ((TICRATE*3)+roulettestop)*FRACUNIT) - FRACUNIT;
	}
	else if (!(player->itemroulette >= (TICRATE*3)))
		return;

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

	pdis = K_ScaleItemDistance(pdis, pingame);

	if (player->bot && player->botvars.rival)
	{
		// Rival has better odds :)
		pdis = (15 * pdis) / 14;
	}

	// SPECIAL CASE No. 1:
	// Fake Eggman items
	if (player->roulettetype == 2)
	{
		player->eggmanexplode = 4*TICRATE;
		//player->karthud[khud_itemblink] = TICRATE;
		//player->karthud[khud_itemblinkmode] = 1;
		player->itemroulette = 0;
		player->roulettetype = 0;
		if (P_IsDisplayPlayer(player) && !demo.freecam)
			S_StartSound(NULL, sfx_itrole);
		return;
	}

	// SPECIAL CASE No. 2:
	// Give a debug item instead if specified
	if (cv_kartdebugitem.value != 0 && !modeattacking)
	{
		K_KartGetItemResult(player, cv_kartdebugitem.value);
		player->itemamount = cv_kartdebugamount.value;
		player->karthud[khud_itemblink] = TICRATE;
		player->karthud[khud_itemblinkmode] = 2;
		player->itemroulette = 0;
		player->roulettetype = 0;
		if (P_IsDisplayPlayer(player) && !demo.freecam)
			S_StartSound(NULL, sfx_dbgsal);
		return;
	}

	// SPECIAL CASE No. 3:
	// Record Attack / alone mashing behavior
	if (modeattacking || pingame == 1)
	{
		if (gametype == GT_RACE)
		{
			if (mashed && (modeattacking || cv_superring.value)) // ANY mashed value? You get rings.
			{
				K_KartGetItemResult(player, KITEM_SUPERRING);
				player->karthud[khud_itemblinkmode] = 1;
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolm);
			}
			else
			{
				if (modeattacking || cv_sneaker.value) // Waited patiently? You get a sneaker!
					K_KartGetItemResult(player, KITEM_SNEAKER);
				else  // Default to sad if nothing's enabled...
					K_KartGetItemResult(player, KITEM_SAD);
				player->karthud[khud_itemblinkmode] = 0;
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolf);
			}
		}
		else if (gametype == GT_BATTLE)
		{
			if (mashed && (modeattacking || bossinfo.boss || cv_banana.value)) // ANY mashed value? You get a banana.
			{
				K_KartGetItemResult(player, KITEM_BANANA);
				player->karthud[khud_itemblinkmode] = 1;
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolm);
			}
			else if (bossinfo.boss)
			{
				K_KartGetItemResult(player, KITEM_ORBINAUT);
				player->karthud[khud_itemblinkmode] = 0;
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolf);
			}
			else
			{
				if (modeattacking || cv_tripleorbinaut.value) // Waited patiently? You get Orbinaut x3!
					K_KartGetItemResult(player, KRITEM_TRIPLEORBINAUT);
				else  // Default to sad if nothing's enabled...
					K_KartGetItemResult(player, KITEM_SAD);
				player->karthud[khud_itemblinkmode] = 0;
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolf);
			}
		}

		player->karthud[khud_itemblink] = TICRATE;
		player->itemroulette = 0;
		player->roulettetype = 0;
		return;
	}

	// SPECIAL CASE No. 4:
	// Being in ring debt occasionally forces Super Ring on you if you mashed
	if (!(gametyperules & GTR_SPHERES) && mashed && player->rings < 0 && cv_superring.value)
	{
		INT32 debtamount = min(20, abs(player->rings));
		if (P_RandomChance(PR_ITEM_ROULETTE, (debtamount*FRACUNIT)/20))
		{
			K_KartGetItemResult(player, KITEM_SUPERRING);
			player->karthud[khud_itemblink] = TICRATE;
			player->karthud[khud_itemblinkmode] = 1;
			player->itemroulette = 0;
			player->roulettetype = 0;
			if (P_IsDisplayPlayer(player))
				S_StartSound(NULL, sfx_itrolm);
			return;
		}
	}

	// SPECIAL CASE No. 5:
	// Force SPB if 2nd is way too far behind
	if (K_ForcedSPB(player) == true)
	{
		K_KartGetItemResult(player, KITEM_SPB);
		player->karthud[khud_itemblink] = TICRATE;
		player->karthud[khud_itemblinkmode] = 2;
		player->itemroulette = 0;
		player->roulettetype = 0;
		if (P_IsDisplayPlayer(player))
			S_StartSound(NULL, sfx_itrolk);
		return;
	}

	// NOW that we're done with all of those specialized cases, we can move onto the REAL item roulette tables.
	// Initializes existing spawnchance values
	for (i = 0; i < NUMKARTRESULTS; i++)
		spawnchance[i] = 0;

	// Split into another function for a debug function below
	useodds = K_FindUseodds(player, mashed, pdis, bestbumper);

	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		spawnchance[i] = (totalspawnchance += K_KartGetItemOdds(
			useodds, i,
			player->distancetofinish,
			mashed,
			player->bot, (player->bot && player->botvars.rival))
		);
	}

	// Award the player whatever power is rolled
	if (totalspawnchance > 0)
	{
		totalspawnchance = P_RandomKey(PR_ITEM_ROULETTE, totalspawnchance);
		for (i = 0; i < NUMKARTRESULTS && spawnchance[i] <= totalspawnchance; i++);

		K_KartGetItemResult(player, i);
	}
	else
	{
		player->itemtype = KITEM_SAD;
		player->itemamount = 1;
	}

	if (P_IsDisplayPlayer(player) && !demo.freecam)
		S_StartSound(NULL, ((player->roulettetype == 1) ? sfx_itrolk : (mashed ? sfx_itrolm : sfx_itrolf)));

	player->karthud[khud_itemblink] = TICRATE;
	player->karthud[khud_itemblinkmode] = ((player->roulettetype == 1) ? 2 : (mashed ? 1 : 0));

	player->itemroulette = 0; // Since we're done, clear the roulette number
	player->roulettetype = 0; // This too
}

//}

//{ SRB2kart p_user.c Stuff

static fixed_t K_PlayerWeight(mobj_t *mobj, mobj_t *against)
{
	fixed_t weight = 5*FRACUNIT;

	if (!mobj->player)
		return weight;

	if (against && !P_MobjWasRemoved(against) && against->player
		&& ((!P_PlayerInPain(against->player) && P_PlayerInPain(mobj->player)) // You're hurt
		|| (against->player->itemtype == KITEM_BUBBLESHIELD && mobj->player->itemtype != KITEM_BUBBLESHIELD))) // They have a Bubble Shield
	{
		weight = 0; // This player does not cause any bump action
	}
	else
	{
		// Applies rubberbanding, to prevent rubberbanding bots
		// from causing super crazy bumps.
		fixed_t spd = K_GetKartSpeed(mobj->player, false, true);

		weight = (mobj->player->kartweight) * FRACUNIT;

		if (mobj->player->speed > spd)
			weight += (mobj->player->speed - spd) / 8;

		if (mobj->player->itemtype == KITEM_BUBBLESHIELD)
			weight += 9*FRACUNIT;
	}

	return weight;
}

fixed_t K_GetMobjWeight(mobj_t *mobj, mobj_t *against)
{
	fixed_t weight = 5*FRACUNIT;

	switch (mobj->type)
	{
		case MT_PLAYER:
			if (!mobj->player)
				break;
			weight = K_PlayerWeight(mobj, against);
			break;
		case MT_KART_LEFTOVER:
			weight = 5*FRACUNIT/2;

			if (mobj->extravalue1 > 0)
			{
				weight = mobj->extravalue1 * (FRACUNIT >> 1);
			}
			break;
		case MT_BUBBLESHIELD:
			weight = K_PlayerWeight(mobj->target, against);
			break;
		case MT_FALLINGROCK:
			if (against->player)
			{
				if (against->player->invincibilitytimer || K_IsBigger(against, mobj) == true)
					weight = 0;
				else
					weight = K_PlayerWeight(against, NULL);
			}
			break;
		case MT_ORBINAUT:
		case MT_ORBINAUT_SHIELD:
			if (against->player)
				weight = K_PlayerWeight(against, NULL);
			break;
		case MT_JAWZ:
		case MT_JAWZ_SHIELD:
			if (against->player)
				weight = K_PlayerWeight(against, NULL) + (3*FRACUNIT);
			else
				weight += 3*FRACUNIT;
			break;
		case MT_DROPTARGET:
		case MT_DROPTARGET_SHIELD:
			if (against->player)
				weight = K_PlayerWeight(against, NULL);
		default:
			break;
	}

	return FixedMul(weight, mobj->scale);
}

static void K_SpawnBumpForObjs(mobj_t *mobj1, mobj_t *mobj2)
{
	mobj_t *fx = P_SpawnMobj(
		mobj1->x/2 + mobj2->x/2,
		mobj1->y/2 + mobj2->y/2,
		mobj1->z/2 + mobj2->z/2,
		MT_BUMP
	);
	fixed_t avgScale = (mobj1->scale + mobj2->scale) / 2;

	if (mobj1->eflags & MFE_VERTICALFLIP)
	{
		fx->eflags |= MFE_VERTICALFLIP;
	}
	else
	{
		fx->eflags &= ~MFE_VERTICALFLIP;
	}

	P_SetScale(fx, (fx->destscale = avgScale));

	if ((mobj1->player && mobj1->player->itemtype == KITEM_BUBBLESHIELD)
	|| (mobj2->player && mobj2->player->itemtype == KITEM_BUBBLESHIELD))
	{
		S_StartSound(mobj1, sfx_s3k44);
	}
	else if (mobj1->type == MT_DROPTARGET || mobj1->type == MT_DROPTARGET_SHIELD) // no need to check the other way around
	{
		// Sound handled in K_DropTargetCollide
		// S_StartSound(mobj2, sfx_s258);
		fx->colorized = true;
		fx->color = mobj1->color;
	}
	else
	{
		S_StartSound(mobj1, sfx_s3k49);
	}
}

static void K_PlayerJustBumped(player_t *player)
{
	mobj_t *playerMobj = NULL;

	if (player == NULL)
	{
		return;
	}

	playerMobj = player->mo;

	if (playerMobj == NULL || P_MobjWasRemoved(playerMobj))
	{
		return;
	}

	if (abs(player->rmomx) < playerMobj->scale && abs(player->rmomy) < playerMobj->scale)
	{
		// Because this is done during collision now, rmomx and rmomy need to be recalculated
		// so that friction doesn't immediately decide to stop the player if they're at a standstill
		player->rmomx = playerMobj->momx - player->cmomx;
		player->rmomy = playerMobj->momy - player->cmomy;
	}

	player->justbumped = bumptime;
	player->spindash = 0;

	if (player->spinouttimer)
	{
		player->wipeoutslow = wipeoutslowtime+1;
		player->spinouttimer = max(wipeoutslowtime+1, player->spinouttimer);
		//player->spinouttype = KSPIN_WIPEOUT; // Enforce type
	}
}

static fixed_t K_GetBounceForce(mobj_t *mobj1, mobj_t *mobj2, fixed_t distx, fixed_t disty)
{
	const fixed_t forceMul = (4 * FRACUNIT) / 10; // Multiply by this value to make it feel like old bumps.

	fixed_t momdifx, momdify;
	fixed_t dot;
	fixed_t force = 0;

	momdifx = mobj1->momx - mobj2->momx;
	momdify = mobj1->momy - mobj2->momy;

	if (distx == 0 && disty == 0)
	{
		// if there's no distance between the 2, they're directly on top of each other, don't run this
		return 0;
	}

	{ // Normalize distance to the sum of the two objects' radii, since in a perfect world that would be the distance at the point of collision...
		fixed_t dist = P_AproxDistance(distx, disty);
		fixed_t nx, ny;

		dist = dist ? dist : 1;

		nx = FixedDiv(distx, dist);
		ny = FixedDiv(disty, dist);

		distx = FixedMul(mobj1->radius + mobj2->radius, nx);
		disty = FixedMul(mobj1->radius + mobj2->radius, ny);

		if (momdifx == 0 && momdify == 0)
		{
			// If there's no momentum difference, they're moving at exactly the same rate. Pretend they moved into each other.
			momdifx = -nx;
			momdify = -ny;
		}
	}

	dot = FixedMul(momdifx, distx) + FixedMul(momdify, disty);

	if (dot >= 0)
	{
		// They're moving away from each other
		return 0;
	}

	// Return the push force!
	force = FixedDiv(dot, FixedMul(distx, distx) + FixedMul(disty, disty));

	return FixedMul(force, forceMul);
}

boolean K_KartBouncing(mobj_t *mobj1, mobj_t *mobj2)
{
	const fixed_t minBump = 25*mapobjectscale;
	mobj_t *goombaBounce = NULL;
	fixed_t distx, disty, dist;
	fixed_t force;
	fixed_t mass1, mass2;

	if ((!mobj1 || P_MobjWasRemoved(mobj1))
	|| (!mobj2 || P_MobjWasRemoved(mobj2)))
	{
		return false;
	}

	// Don't bump when you're being reborn
	if ((mobj1->player && mobj1->player->playerstate != PST_LIVE)
		|| (mobj2->player && mobj2->player->playerstate != PST_LIVE))
		return false;

	if ((mobj1->player && mobj1->player->respawn.state != RESPAWNST_NONE)
		|| (mobj2->player && mobj2->player->respawn.state != RESPAWNST_NONE))
		return false;

	if (mobj1->type != MT_DROPTARGET && mobj1->type != MT_DROPTARGET_SHIELD)
	{ // Don't bump if you're flashing
		INT32 flash;

		flash = K_GetKartFlashing(mobj1->player);
		if (mobj1->player && mobj1->player->flashing > 0 && mobj1->player->flashing < flash)
		{
			if (mobj1->player->flashing < flash-1)
				mobj1->player->flashing++;
			return false;
		}

		flash = K_GetKartFlashing(mobj2->player);
		if (mobj2->player && mobj2->player->flashing > 0 && mobj2->player->flashing < flash)
		{
			if (mobj2->player->flashing < flash-1)
				mobj2->player->flashing++;
			return false;
		}
	}

	// Don't bump if you've recently bumped
	if (mobj1->player && mobj1->player->justbumped)
	{
		mobj1->player->justbumped = bumptime;
		return false;
	}

	if (mobj2->player && mobj2->player->justbumped)
	{
		mobj2->player->justbumped = bumptime;
		return false;
	}

	// Adds the OTHER object's momentum times a bunch, for the best chance of getting the correct direction
	distx = (mobj1->x + mobj2->momx) - (mobj2->x + mobj1->momx);
	disty = (mobj1->y + mobj2->momy) - (mobj2->y + mobj1->momy);

	force = K_GetBounceForce(mobj1, mobj2, distx, disty);

	if (force == 0)
	{
		return false;
	}

	mass1 = K_GetMobjWeight(mobj1, mobj2);
	mass2 = K_GetMobjWeight(mobj2, mobj1);

	if ((P_IsObjectOnGround(mobj1) && mobj2->momz < 0) // Grounded
		|| (mass2 == 0 && mass1 > 0)) // The other party is immovable
	{
		goombaBounce = mobj2;
	}
	else if ((P_IsObjectOnGround(mobj2) && mobj1->momz < 0) // Grounded
		|| (mass1 == 0 && mass2 > 0)) // The other party is immovable
	{
		goombaBounce = mobj1;
	}

	if (goombaBounce != NULL)
	{
		// Perform a Goomba Bounce by reversing your z momentum.
		goombaBounce->momz = -goombaBounce->momz;
	}
	else
	{
		// Trade z momentum values.
		fixed_t newz = mobj1->momz;
		mobj1->momz = mobj2->momz;
		mobj2->momz = newz;
	}

	// Multiply by force
	distx = FixedMul(force, distx);
	disty = FixedMul(force, disty);
	dist = FixedHypot(distx, disty);

	// if the speed difference is less than this let's assume they're going proportionately faster from each other
	if (dist < minBump)
	{
		fixed_t normalisedx = FixedDiv(distx, dist);
		fixed_t normalisedy = FixedDiv(disty, dist);

		distx = FixedMul(minBump, normalisedx);
		disty = FixedMul(minBump, normalisedy);
	}

	if (mass2 > 0)
	{
		mobj1->momx = mobj1->momx - FixedMul(FixedDiv(2*mass2, mass1 + mass2), distx);
		mobj1->momy = mobj1->momy - FixedMul(FixedDiv(2*mass2, mass1 + mass2), disty);
	}

	if (mass1 > 0)
	{
		mobj2->momx = mobj2->momx - FixedMul(FixedDiv(2*mass1, mass1 + mass2), -distx);
		mobj2->momy = mobj2->momy - FixedMul(FixedDiv(2*mass1, mass1 + mass2), -disty);
	}

	K_SpawnBumpForObjs(mobj1, mobj2);

	K_PlayerJustBumped(mobj1->player);
	K_PlayerJustBumped(mobj2->player);

	return true;
}

// K_KartBouncing, but simplified to act like P_BouncePlayerMove
boolean K_KartSolidBounce(mobj_t *bounceMobj, mobj_t *solidMobj)
{
	const fixed_t minBump = 25*mapobjectscale;
	fixed_t distx, disty, dist;
	fixed_t force;

	if ((!bounceMobj || P_MobjWasRemoved(bounceMobj))
	|| (!solidMobj || P_MobjWasRemoved(solidMobj)))
	{
		return false;
	}

	// Don't bump when you're being reborn
	if (bounceMobj->player && bounceMobj->player->playerstate != PST_LIVE)
		return false;

	if (bounceMobj->player && bounceMobj->player->respawn.state != RESPAWNST_NONE)
		return false;

	// Don't bump if you've recently bumped
	if (bounceMobj->player && bounceMobj->player->justbumped)
	{
		bounceMobj->player->justbumped = bumptime;
		return false;
	}

	// Adds the OTHER object's momentum times a bunch, for the best chance of getting the correct direction
	{
		distx = (bounceMobj->x + solidMobj->momx) - (solidMobj->x + bounceMobj->momx);
		disty = (bounceMobj->y + solidMobj->momy) - (solidMobj->y + bounceMobj->momy);
	}

	force = K_GetBounceForce(bounceMobj, solidMobj, distx, disty);

	if (force == 0)
	{
		return false;
	}

	// Multiply by force
	distx = FixedMul(force, distx);
	disty = FixedMul(force, disty);
	dist = FixedHypot(distx, disty);

	{
		// Normalize to the desired push value.
		fixed_t normalisedx = FixedDiv(distx, dist);
		fixed_t normalisedy = FixedDiv(disty, dist);
		fixed_t bounceSpeed;

		bounceSpeed = FixedHypot(bounceMobj->momx, bounceMobj->momy);
		bounceSpeed = FixedMul(bounceSpeed, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
		bounceSpeed += minBump;

		distx = FixedMul(bounceSpeed, normalisedx);
		disty = FixedMul(bounceSpeed, normalisedy);
	}

	bounceMobj->momx = bounceMobj->momx - distx;
	bounceMobj->momy = bounceMobj->momy - disty;
	bounceMobj->momz = -bounceMobj->momz;

	K_SpawnBumpForObjs(bounceMobj, solidMobj);
	K_PlayerJustBumped(bounceMobj->player);

	return true;
}

/**	\brief	Checks that the player is on an offroad subsector for realsies. Also accounts for line riding to prevent cheese.

	\param	mo	player mobj object

	\return	boolean
*/
static UINT8 K_CheckOffroadCollide(mobj_t *mo)
{
	// Check for sectors in touching_sectorlist
	UINT8 i;			// special type iter
	msecnode_t *node;	// touching_sectorlist iter
	sector_t *s;		// main sector shortcut
	sector_t *s2;		// FOF sector shortcut
	ffloor_t *rover;	// FOF

	fixed_t flr;
	fixed_t cel;	// floor & ceiling for height checks to make sure we're touching the offroad sector.

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	// If tiregrease is active, don't
	if (mo->player && mo->player->tiregrease)
		return 0;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
			break;	// shouldn't happen.

		s = node->m_sector;
		// 1: Check for the main sector, make sure we're on the floor of that sector and see if we can apply offroad.
		// Make arbitrary Z checks because we want to check for 1 sector in particular, we don't want to affect the player if the offroad sector is way below them and they're lineriding a normal sector above.

		flr = P_MobjFloorZ(mo, s, s, mo->x, mo->y, NULL, false, true);
		cel = P_MobjCeilingZ(mo, s, s, mo->x, mo->y, NULL, true, true);	// get Z coords of both floors and ceilings for this sector (this accounts for slopes properly.)
		// NOTE: we don't use P_GetZAt with our x/y directly because the mobj won't have the same height because of its hitbox on the slope. Complex garbage but tldr it doesn't work.

		if ( ((s->flags & SF_FLIPSPECIAL_FLOOR) && mo->z == flr)	// floor check
			|| ((mo->eflags & MFE_VERTICALFLIP && (s->flags & SF_FLIPSPECIAL_CEILING) && (mo->z + mo->height) == cel)) )	// ceiling check.

			for (i = 2; i < 5; i++)	// check for sector special

				if (GETSECSPECIAL(s->special, 1) == i)
					return i-1;	// return offroad type

		// 2: If we're here, we haven't found anything. So let's try looking for FOFs in the sectors using the same logic.
		for (rover = s->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))	// This FOF doesn't exist anymore.
				continue;

			s2 = &sectors[rover->secnum];	// makes things easier for us

			flr = P_GetFOFBottomZ(mo, s, rover, mo->x, mo->y, NULL);
			cel = P_GetFOFTopZ(mo, s, rover, mo->x, mo->y, NULL);	// Z coords for fof top/bottom.

			// we will do essentially the same checks as above instead of bothering with top/bottom height of the FOF.
			// Reminder that an FOF's floor is its bottom, silly!
			if ( ((s2->flags & SF_FLIPSPECIAL_FLOOR) && mo->z == cel)	// "floor" check
				|| ((s2->flags & SF_FLIPSPECIAL_CEILING) && (mo->z + mo->height) == flr) )	// "ceiling" check.

				for (i = 2; i < 5; i++)	// check for sector special

					if (GETSECSPECIAL(s2->special, 1) == i)
						return i-1;	// return offroad type

		}
	}

	return 0;	// couldn't find any offroad
}

/**	\brief	Updates the Player's offroad value once per frame

	\param	player	player object passed from K_KartPlayerThink

	\return	void
*/
static void K_UpdateOffroad(player_t *player)
{
	terrain_t *terrain = player->mo->terrain;
	fixed_t offroadstrength = 0;

	if (terrain != NULL && terrain->offroad > 0)
	{
		offroadstrength = (terrain->offroad << FRACBITS);
	}
	else
	{
		offroadstrength = (K_CheckOffroadCollide(player->mo) << FRACBITS);
	}

	// If you are in offroad, a timer starts.
	if (offroadstrength)
	{
		if (player->offroad < offroadstrength)
			player->offroad += offroadstrength / TICRATE;

		if (player->offroad > offroadstrength)
			player->offroad = offroadstrength;
	}
	else
		player->offroad = 0;
}

static void K_DrawDraftCombiring(player_t *player, player_t *victim, fixed_t curdist, fixed_t maxdist, boolean transparent)
{
#define CHAOTIXBANDLEN 15
#define CHAOTIXBANDCOLORS 9
	static const UINT8 colors[CHAOTIXBANDCOLORS] = {
		SKINCOLOR_SAPPHIRE,
		SKINCOLOR_PLATINUM,
		SKINCOLOR_TEA,
		SKINCOLOR_GARDEN,
		SKINCOLOR_BANANA,
		SKINCOLOR_GOLD,
		SKINCOLOR_ORANGE,
		SKINCOLOR_SCARLET,
		SKINCOLOR_TAFFY
	};
	fixed_t minimumdist = FixedMul(RING_DIST>>1, player->mo->scale);
	UINT8 n = CHAOTIXBANDLEN;
	UINT8 offset = ((leveltime / 3) % 3);
	fixed_t stepx, stepy, stepz;
	fixed_t curx, cury, curz;
	UINT8 c;

	if (maxdist == 0)
	{
		c = leveltime % CHAOTIXBANDCOLORS;
	}
	else
	{
		c = FixedMul(CHAOTIXBANDCOLORS<<FRACBITS, FixedDiv(curdist-minimumdist, maxdist-minimumdist)) >> FRACBITS;
	}

	stepx = (victim->mo->x - player->mo->x) / CHAOTIXBANDLEN;
	stepy = (victim->mo->y - player->mo->y) / CHAOTIXBANDLEN;
	stepz = ((victim->mo->z + (victim->mo->height / 2)) - (player->mo->z + (player->mo->height / 2))) / CHAOTIXBANDLEN;

	curx = player->mo->x + stepx;
	cury = player->mo->y + stepy;
	curz = player->mo->z + stepz;

	while (n)
	{
		if (offset == 0)
		{
			mobj_t *band = P_SpawnMobj(curx + (P_RandomRange(PR_DECORATION, -12, 12)*mapobjectscale),
				cury + (P_RandomRange(PR_DECORATION, -12, 12)*mapobjectscale),
				curz + (P_RandomRange(PR_DECORATION, 24, 48)*mapobjectscale),
				MT_SIGNSPARKLE);

			if (maxdist == 0)
			{
				P_SetMobjState(band, S_KSPARK1 + (leveltime % 8));
				P_SetScale(band, (band->destscale = player->mo->scale));
			}
			else
			{
				P_SetMobjState(band, S_SIGNSPARK1 + (leveltime % 11));
				P_SetScale(band, (band->destscale = (3*player->mo->scale)/2));
			}

			band->color = colors[c];
			band->colorized = true;

			band->fuse = 2;

			if (transparent)
				band->renderflags |= RF_GHOSTLY;

			band->renderflags |= RF_DONTDRAW & ~(K_GetPlayerDontDrawFlag(player) | K_GetPlayerDontDrawFlag(victim));
		}

		curx += stepx;
		cury += stepy;
		curz += stepz;

		offset = abs(offset-1) % 3;
		n--;
	}
#undef CHAOTIXBANDLEN
}

/**	\brief	Updates the player's drafting values once per frame

	\param	player	player object passed from K_KartPlayerThink

	\return	void
*/
static void K_UpdateDraft(player_t *player)
{
	fixed_t topspd = K_GetKartSpeed(player, false, false);
	fixed_t draftdistance;
	fixed_t minDist;
	UINT8 leniency;
	UINT8 i;

	if (player->itemtype == KITEM_LIGHTNINGSHIELD)
	{
		// Lightning Shield gets infinite draft distance as its (other) passive effect.
		draftdistance = 0;
	}
	else
	{
		// Distance you have to be to draft. If you're still accelerating, then this distance is lessened.
		// This distance biases toward low weight! (min weight gets 4096 units, max weight gets 3072 units)
		// This distance is also scaled based on game speed.
		draftdistance = (3072 + (128 * (9 - player->kartweight))) * player->mo->scale;
		if (player->speed < topspd)
			draftdistance = FixedMul(draftdistance, FixedDiv(player->speed, topspd));
		draftdistance = FixedMul(draftdistance, K_GetKartGameSpeedScalar(gamespeed));
	}

	// On the contrary, the leniency period biases toward high weight.
	// (See also: the leniency variable in K_SpawnDraftDust)
	leniency = (3*TICRATE)/4 + ((player->kartweight-1) * (TICRATE/4));

	minDist = 640 * player->mo->scale;

	if (gametype == GT_BATTLE)
	{
		// TODO: gametyperules
		minDist /= 4;
		draftdistance *= 2;
		leniency *= 4;
	}

	// Not enough speed to draft.
	if (player->speed >= 20*player->mo->scale)
	{
//#define EASYDRAFTTEST
		// Let's hunt for players to draft off of!
		for (i = 0; i < MAXPLAYERS; i++)
		{
			fixed_t dist, olddraft;
#ifndef EASYDRAFTTEST
			angle_t yourangle, theirangle, diff;
#endif

			if (!playeringame[i] || players[i].spectator || !players[i].mo)
				continue;

#ifndef EASYDRAFTTEST
			// Don't draft on yourself :V
			if (&players[i] == player)
				continue;
#endif

			// Not enough speed to draft off of.
			if (players[i].speed < 20*players[i].mo->scale)
				continue;

			// No tethering off of the guy who got the starting bonus :P
			if (players[i].startboost > 0)
				continue;

#ifndef EASYDRAFTTEST
			yourangle = K_MomentumAngle(player->mo);
			theirangle = K_MomentumAngle(players[i].mo);

			diff = R_PointToAngle2(player->mo->x, player->mo->y, players[i].mo->x, players[i].mo->y) - yourangle;
			if (diff > ANGLE_180)
				diff = InvAngle(diff);

			// Not in front of this player.
			if (diff > ANG10)
				continue;

			diff = yourangle - theirangle;
			if (diff > ANGLE_180)
				diff = InvAngle(diff);

			// Not moving in the same direction.
			if (diff > ANGLE_90)
				continue;
#endif

			dist = P_AproxDistance(P_AproxDistance(players[i].mo->x - player->mo->x, players[i].mo->y - player->mo->y), players[i].mo->z - player->mo->z);

#ifndef EASYDRAFTTEST
			// TOO close to draft.
			if (dist < minDist)
				continue;

			// Not close enough to draft.
			if (dist > draftdistance && draftdistance > 0)
				continue;
#endif

			olddraft = player->draftpower;

			player->draftleeway = leniency;
			player->lastdraft = i;

			// Draft power is used later in K_GetKartBoostPower, ranging from 0 for normal speed and FRACUNIT for max draft speed.
			// How much this increments every tic biases toward acceleration! (min speed gets 1.5% per tic, max speed gets 0.5% per tic)
			if (player->draftpower < FRACUNIT)
			{
				fixed_t add = (FRACUNIT/200) + ((9 - player->kartspeed) * ((3*FRACUNIT)/1600));;
				player->draftpower += add;

				if (player->bot && player->botvars.rival)
				{
					// Double speed for the rival!
					player->draftpower += add;
				}

				if (gametype == GT_BATTLE)
				{
					// TODO: gametyperules
					// Double speed in Battle
					player->draftpower += add;
				}
			}

			if (player->draftpower > FRACUNIT)
				player->draftpower = FRACUNIT;

			// Play draft finish noise
			if (olddraft < FRACUNIT && player->draftpower >= FRACUNIT)
				S_StartSound(player->mo, sfx_cdfm62);

			// Spawn in the visual!
			K_DrawDraftCombiring(player, &players[i], dist, draftdistance, false);

			return; // Finished doing our draft.
		}
	}

	// No one to draft off of? Then you can knock that off.
	if (player->draftleeway > 0) // Prevent small disruptions from stopping your draft.
	{
		if (P_IsObjectOnGround(player->mo) == true)
		{
			// Allow maintaining tether in air setpieces.
			player->draftleeway--;
		}

		if (player->lastdraft >= 0
			&& player->lastdraft < MAXPLAYERS
			&& playeringame[player->lastdraft]
			&& !players[player->lastdraft].spectator
			&& players[player->lastdraft].mo)
		{
			player_t *victim = &players[player->lastdraft];
			fixed_t dist = P_AproxDistance(P_AproxDistance(victim->mo->x - player->mo->x, victim->mo->y - player->mo->y), victim->mo->z - player->mo->z);
			K_DrawDraftCombiring(player, victim, dist, draftdistance, true);
		}
	}
	else // Remove draft speed boost.
	{
		player->draftpower = 0;
		player->lastdraft = -1;
	}
}

void K_KartPainEnergyFling(player_t *player)
{
	static const UINT8 numfling = 5;
	INT32 i;
	mobj_t *mo;
	angle_t fa;
	fixed_t ns;
	fixed_t z;

	// Better safe than sorry.
	if (!player)
		return;

	// P_PlayerRingBurst: "There's no ring spilling in kart, so I'm hijacking this for the same thing as TD"
	// :oh:

	for (i = 0; i < numfling; i++)
	{
		INT32 objType = mobjinfo[MT_FLINGENERGY].reactiontime;
		fixed_t momxy, momz; // base horizonal/vertical thrusts

		z = player->mo->z;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - mobjinfo[objType].height;

		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, objType);

		mo->fuse = 8*TICRATE;
		P_SetTarget(&mo->target, player->mo);

		mo->destscale = player->mo->scale;
		P_SetScale(mo, player->mo->scale);

		// Angle offset by player angle, then slightly offset by amount of fling
		fa = ((i*FINEANGLES/16) + (player->mo->angle>>ANGLETOFINESHIFT) - ((numfling-1)*FINEANGLES/32)) & FINEMASK;

		if (i > 15)
		{
			momxy = 3*FRACUNIT;
			momz = 4*FRACUNIT;
		}
		else
		{
			momxy = 28*FRACUNIT;
			momz = 3*FRACUNIT;
		}

		ns = FixedMul(momxy, mo->scale);
		mo->momx = FixedMul(FINECOSINE(fa),ns);

		ns = momz;
		P_SetObjectMomZ(mo, ns, false);

		if (i & 1)
			P_SetObjectMomZ(mo, ns, true);

		if (player->mo->eflags & MFE_VERTICALFLIP)
			mo->momz *= -1;
	}
}

// Adds gravity flipping to an object relative to its master and shifts the z coordinate accordingly.
void K_FlipFromObject(mobj_t *mo, mobj_t *master)
{
	mo->eflags = (mo->eflags & ~MFE_VERTICALFLIP)|(master->eflags & MFE_VERTICALFLIP);
	mo->flags2 = (mo->flags2 & ~MF2_OBJECTFLIP)|(master->flags2 & MF2_OBJECTFLIP);

	if (mo->eflags & MFE_VERTICALFLIP)
		mo->z += master->height - FixedMul(master->scale, mo->height);
}

void K_MatchGenericExtraFlags(mobj_t *mo, mobj_t *master)
{
	// flipping
	// handle z shifting from there too. This is here since there's no reason not to flip us if needed when we do this anyway;
	K_FlipFromObject(mo, master);

	// visibility (usually for hyudoro)
	mo->renderflags = (mo->renderflags & ~RF_DONTDRAW) | (master->renderflags & RF_DONTDRAW);
}

// same as above, but does not adjust Z height when flipping
void K_GenericExtraFlagsNoZAdjust(mobj_t *mo, mobj_t *master)
{
	// flipping
	mo->eflags = (mo->eflags & ~MFE_VERTICALFLIP)|(master->eflags & MFE_VERTICALFLIP);
	mo->flags2 = (mo->flags2 & ~MF2_OBJECTFLIP)|(master->flags2 & MF2_OBJECTFLIP);

	// visibility (usually for hyudoro)
	mo->renderflags = (mo->renderflags & ~RF_DONTDRAW) | (master->renderflags & RF_DONTDRAW);
}


void K_SpawnDashDustRelease(player_t *player)
{
	fixed_t newx;
	fixed_t newy;
	mobj_t *dust;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (!P_IsObjectOnGround(player->mo))
		return;

	if (!player->speed && !player->startboost && !player->spindash)
		return;

	travelangle = player->mo->angle;

	if (player->drift || (player->pflags & PF_DRIFTEND))
		travelangle -= (ANGLE_45/5)*player->drift;

	for (i = 0; i < 2; i++)
	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_90, FixedMul(48*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_90, FixedMul(48*FRACUNIT, player->mo->scale));
		dust = P_SpawnMobj(newx, newy, player->mo->z, MT_FASTDUST);

		P_SetTarget(&dust->target, player->mo);
		P_InitAngle(dust, travelangle - ((i&1) ? -1 : 1) * ANGLE_45);
		dust->destscale = player->mo->scale;
		P_SetScale(dust, player->mo->scale);

		dust->momx = 3*player->mo->momx/5;
		dust->momy = 3*player->mo->momy/5;
		dust->momz = 3*P_GetMobjZMovement(player->mo)/5;

		K_MatchGenericExtraFlags(dust, player->mo);
	}
}

static fixed_t K_GetBrakeFXScale(player_t *player, fixed_t maxScale)
{
	fixed_t s = FixedDiv(player->speed,
			K_GetKartSpeed(player, false, false));

	s = max(s, FRACUNIT);
	s = min(s, maxScale);

	return s;
}

static void K_SpawnBrakeDriftSparks(player_t *player) // Be sure to update the mobj thinker case too!
{
	mobj_t *sparks;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	// Position & etc are handled in its thinker, and its spawned invisible.
	// This avoids needing to dupe code if we don't need it.
	sparks = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BRAKEDRIFT);
	P_SetTarget(&sparks->target, player->mo);
	P_SetScale(sparks, (sparks->destscale = FixedMul(K_GetBrakeFXScale(player, 3*FRACUNIT), player->mo->scale)));
	K_MatchGenericExtraFlags(sparks, player->mo);
	sparks->renderflags |= RF_DONTDRAW;
}

static void
spawn_brake_dust
(		mobj_t * master,
		angle_t aoff,
		fixed_t rad,
		fixed_t scale)
{
	const angle_t a = master->angle + aoff;

	mobj_t *spark = P_SpawnMobjFromMobj(master,
			P_ReturnThrustX(NULL, a, rad),
			P_ReturnThrustY(NULL, a, rad), 0,
			MT_BRAKEDUST);

	spark->momx = master->momx;
	spark->momy = master->momy;
	spark->momz = P_GetMobjZMovement(master);
	spark->angle = a - ANGLE_180;
	spark->pitch = master->pitch;
	spark->roll = master->roll;

	P_Thrust(spark, a, 16 * spark->scale);

	P_SetScale(spark, (spark->destscale =
				FixedMul(scale, spark->scale)));
}

static void K_SpawnBrakeVisuals(player_t *player)
{
	const fixed_t scale =
		K_GetBrakeFXScale(player, 2*FRACUNIT);

	if (leveltime & 1)
	{
		angle_t aoff;
		fixed_t radf;

		UINT8 wheel = 3;

		if (player->drift)
		{
			/* brake-drifting: dust flies from outer wheel */
			wheel ^= 1 << (player->drift < 0);

			aoff = 7 * ANG10;
			radf = 32 * FRACUNIT;
		}
		else
		{
			aoff = ANG30;
			radf = 24 * FRACUNIT;
		}

		if (wheel & 1)
		{
			spawn_brake_dust(player->mo,
					aoff, radf, scale);
		}

		if (wheel & 2)
		{
			spawn_brake_dust(player->mo,
					InvAngle(aoff), radf, scale);
		}
	}

	if (leveltime % 4 == 0)
		S_StartSound(player->mo, sfx_s3k67);

	/* vertical shaking, scales with speed */
	player->mo->spriteyoffset = P_RandomFlip(2 * scale);
}

void K_SpawnDriftBoostClip(player_t *player)
{
	mobj_t *clip;
	fixed_t scale = 115*FRACUNIT/100;
	fixed_t momz = P_GetMobjZMovement(player->mo);
	fixed_t z;

	if (( player->mo->eflags & MFE_VERTICALFLIP ))
		z = player->mo->z;
	else
		z = player->mo->z + player->mo->height;

	clip = P_SpawnMobj(player->mo->x, player->mo->y, z, MT_DRIFTCLIP);

	P_SetTarget(&clip->target, player->mo);
	P_SetScale(clip, ( clip->destscale = FixedMul(scale, player->mo->scale) ));
	K_MatchGenericExtraFlags(clip, player->mo);

	clip->fuse = 105;
	clip->momz = 7 * P_MobjFlip(clip) * clip->scale;

	if (momz > 0)
		clip->momz += momz;

	P_InstaThrust(clip, player->mo->angle +
			P_RandomFlip(P_RandomRange(PR_DECORATION, FRACUNIT/2, FRACUNIT)),
			FixedMul(scale, player->speed));
}

void K_SpawnDriftBoostClipSpark(mobj_t *clip)
{
	mobj_t *spark;

	spark = P_SpawnMobj(clip->x, clip->y, clip->z, MT_DRIFTCLIPSPARK);

	P_SetTarget(&spark->target, clip);
	P_SetScale(spark, ( spark->destscale = clip->scale ));
	K_MatchGenericExtraFlags(spark, clip);

	spark->momx = clip->momx/2;
	spark->momy = clip->momx/2;
}

void K_SpawnNormalSpeedLines(player_t *player)
{
	mobj_t *fast = P_SpawnMobj(player->mo->x + (P_RandomRange(PR_DECORATION,-36,36) * player->mo->scale),
		player->mo->y + (P_RandomRange(PR_DECORATION,-36,36) * player->mo->scale),
		player->mo->z + (player->mo->height/2) + (P_RandomRange(PR_DECORATION,-20,20) * player->mo->scale),
		MT_FASTLINE);

	P_SetTarget(&fast->target, player->mo);
	P_InitAngle(fast, K_MomentumAngle(player->mo));
	fast->momx = 3*player->mo->momx/4;
	fast->momy = 3*player->mo->momy/4;
	fast->momz = 3*P_GetMobjZMovement(player->mo)/4;

	K_MatchGenericExtraFlags(fast, player->mo);

	if (player->tripwireLeniency)
	{
		fast->destscale = fast->destscale * 2;
		P_SetScale(fast, 3*fast->scale/2);
	}

	if (player->eggmanexplode)
	{
		// Make it red when you have the eggman speed boost
		fast->color = SKINCOLOR_RED;
		fast->colorized = true;
	}
	else if (player->invincibilitytimer)
	{
		const tic_t defaultTime = itemtime+(2*TICRATE);
		if (player->invincibilitytimer > defaultTime)
		{
			fast->color = player->mo->color;
		}
		else
		{
			fast->color = SKINCOLOR_INVINCFLASH;
		}
		fast->colorized = true;
	}
	else if (player->tripwireLeniency)
	{
		// Make it pink+blue+big when you can go through tripwire
		fast->color = (leveltime & 1) ? SKINCOLOR_LILAC : SKINCOLOR_JAWZ;
		fast->colorized = true;
		fast->renderflags |= RF_ADD;
	}
}

void K_SpawnInvincibilitySpeedLines(mobj_t *mo)
{
	mobj_t *fast = P_SpawnMobjFromMobj(mo,
		P_RandomRange(PR_DECORATION, -48, 48) * FRACUNIT,
		P_RandomRange(PR_DECORATION, -48, 48) * FRACUNIT,
		P_RandomRange(PR_DECORATION, 0, 64) * FRACUNIT,
		MT_FASTLINE);
	P_SetMobjState(fast, S_KARTINVLINES1);

	P_SetTarget(&fast->target, mo);
	P_InitAngle(fast, K_MomentumAngle(mo));

	fast->momx = 3*mo->momx/4;
	fast->momy = 3*mo->momy/4;
	fast->momz = 3*P_GetMobjZMovement(mo)/4;

	K_MatchGenericExtraFlags(fast, mo);

	fast->color = mo->color;
	fast->colorized = true;

	if (mo->player->invincibilitytimer < 10*TICRATE)
		fast->destscale = 6*((mo->player->invincibilitytimer/TICRATE)*FRACUNIT)/8;
}

static void K_SpawnGrowShrinkParticles(mobj_t *mo, INT32 timer)
{
	const boolean shrink = (timer < 0);
	const INT32 maxTime = (10*TICRATE);
	const INT32 noTime = (2*TICRATE);
	INT32 spawnFreq = 1;

	mobj_t *particle = NULL;
	fixed_t particleScale = FRACUNIT;
	fixed_t particleSpeed = 0;

	spawnFreq = abs(timer);

	if (spawnFreq < noTime)
	{
		return;
	}

	spawnFreq -= noTime;

	if (spawnFreq > maxTime)
	{
		spawnFreq = maxTime;
	}

	spawnFreq = (maxTime - spawnFreq) / TICRATE / 4;
	if (spawnFreq == 0)
	{
		spawnFreq++;
	}

	if (leveltime % spawnFreq != 0)
	{
		return;
	}

	particle = P_SpawnMobjFromMobj(
		mo,
		P_RandomRange(PR_DECORATION, -32, 32) * FRACUNIT,
		P_RandomRange(PR_DECORATION, -32, 32) * FRACUNIT,
		(P_RandomRange(PR_DECORATION, 0, 24) + (shrink ? 48 : 0)) * FRACUNIT,
		MT_GROW_PARTICLE
	);

	P_SetTarget(&particle->target, mo);

	particle->momx = mo->momx;
	particle->momy = mo->momy;
	particle->momz = P_GetMobjZMovement(mo);

	K_MatchGenericExtraFlags(particle, mo);

	particleScale = FixedMul((shrink ? SHRINK_PHYSICS_SCALE : GROW_PHYSICS_SCALE), mapobjectscale);
	particleSpeed = mo->scale * 4 * P_MobjFlip(mo); // NOT particleScale

	particle->destscale = particleScale;
	P_SetScale(particle, particle->destscale);

	if (shrink == true)
	{
		particle->color = SKINCOLOR_KETCHUP;
		particle->momz -= particleSpeed;
		particle->renderflags |= RF_VERTICALFLIP;
	}
	else
	{
		particle->color = SKINCOLOR_SAPPHIRE;
		particle->momz += particleSpeed;
	}
}

void K_SpawnBumpEffect(mobj_t *mo)
{
	mobj_t *fx = P_SpawnMobj(mo->x, mo->y, mo->z, MT_BUMP);
	if (mo->eflags & MFE_VERTICALFLIP)
		fx->eflags |= MFE_VERTICALFLIP;
	else
		fx->eflags &= ~MFE_VERTICALFLIP;
	fx->scale = mo->scale;

	S_StartSound(mo, sfx_s3k49);
}

static SINT8 K_GlanceAtPlayers(player_t *glancePlayer)
{
	const fixed_t maxdistance = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const angle_t blindSpotSize = ANG10; // ANG5
	UINT8 i;
	SINT8 glanceDir = 0;
	SINT8 lastValidGlance = 0;

	// See if there's any players coming up behind us.
	// If so, your character will glance at 'em.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *p;
		angle_t back;
		angle_t diff;
		fixed_t distance;
		SINT8 dir = -1;

		if (!playeringame[i])
		{
			// Invalid player
			continue;
		}

		p = &players[i];

		if (p == glancePlayer)
		{
			// FOOL! Don't glance at yerself!
			continue;
		}

		if (!p->mo || P_MobjWasRemoved(p->mo))
		{
			// Invalid mobj
			continue;
		}

		if (p->spectator || p->hyudorotimer > 0)
		{
			// Not playing / invisible
			continue;
		}

		distance = R_PointToDist2(glancePlayer->mo->x, glancePlayer->mo->y, p->mo->x, p->mo->y);

		if (distance > maxdistance)
		{
			continue;
		}

		back = glancePlayer->mo->angle + ANGLE_180;
		diff = R_PointToAngle2(glancePlayer->mo->x, glancePlayer->mo->y, p->mo->x, p->mo->y) - back;

		if (diff > ANGLE_180)
		{
			diff = InvAngle(diff);
			dir = -dir;
		}

		if (diff > ANGLE_90)
		{
			// Not behind the player
			continue;
		}

		if (diff < blindSpotSize)
		{
			// Small blindspot directly behind your back, gives the impression of smoothly turning.
			continue;
		}

		if (P_CheckSight(glancePlayer->mo, p->mo) == true)
		{
			// Not blocked by a wall, we can glance at 'em!
			// Adds, so that if there's more targets on one of your sides, it'll glance on that side.
			glanceDir += dir;

			// That poses a limitation if there's an equal number of targets on both sides...
			// In that case, we'll pick the last chosen glance direction.
			lastValidGlance = dir;
		}
	}

	if (glanceDir > 0)
	{
		return 1;
	}
	else if (glanceDir < 0)
	{
		return -1;
	}

	return lastValidGlance;
}

/**	\brief Handles the state changing for moving players, moved here to eliminate duplicate code

	\param	player	player data

	\return	void
*/
void K_KartMoveAnimation(player_t *player)
{
	const INT16 minturn = KART_FULLTURN/8;

	const fixed_t fastspeed = (K_GetKartSpeed(player, false, true) * 17) / 20; // 85%
	const fixed_t speedthreshold = player->mo->scale / 8;

	const boolean onground = P_IsObjectOnGround(player->mo);

	UINT16 buttons = K_GetKartButtons(player);
	const boolean spinningwheels = (((buttons & BT_ACCELERATE) == BT_ACCELERATE) || (onground && player->speed > 0));
	const boolean lookback = ((buttons & BT_LOOKBACK) == BT_LOOKBACK);

	SINT8 turndir = 0;
	SINT8 destGlanceDir = 0;
	SINT8 drift = player->drift;

	if (!lookback)
	{
		player->pflags &= ~PF_GAINAX;

		// Uses turning over steering -- it's important to show player feedback immediately.
		if (player->cmd.turning < -minturn)
		{
			turndir = -1;
		}
		else if (player->cmd.turning > minturn)
		{
			turndir = 1;
		}
	}
	else if (drift == 0)
	{
		// Prioritize looking back frames over turning
		turndir = 0;
	}

	// Sliptides: drift -> lookback frames
	if (abs(player->aizdriftturn) >= ANGLE_90)
	{
		destGlanceDir = -(2*intsign(player->aizdriftturn));
		player->glanceDir = destGlanceDir;
		drift = turndir = 0;
		player->pflags &= ~PF_GAINAX;
	}
	else if (player->aizdriftturn)
	{
		drift = intsign(player->aizdriftturn);
		turndir = 0;
	}
	else if (turndir == 0 && drift == 0)
	{
		// Only try glancing if you're driving straight.
		// This avoids all-players loops when we don't need it.
		destGlanceDir = K_GlanceAtPlayers(player);

		if (lookback == true)
		{
			statenum_t gainaxstate = S_GAINAX_TINY;
			if (destGlanceDir == 0)
			{
				if (player->glanceDir != 0)
				{
					// Keep to the side you were already on.
					if (player->glanceDir < 0)
					{
						destGlanceDir = -1;
					}
					else
					{
						destGlanceDir = 1;
					}
				}
				else
				{
					// Look to your right by default
					destGlanceDir = -1;
				}
			}
			else
			{
				// Looking back AND glancing? Amplify the look!
				destGlanceDir *= 2;
				if (player->itemamount && player->itemtype)
					gainaxstate = S_GAINAX_HUGE;
				else
					gainaxstate = S_GAINAX_MID1;
			}

			if (destGlanceDir && !(player->pflags & PF_GAINAX))
			{
				mobj_t *gainax = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_GAINAX);
				gainax->movedir = (destGlanceDir < 0) ? (ANGLE_270-ANG10) : (ANGLE_90+ANG10);
				P_SetTarget(&gainax->target, player->mo);
				P_SetMobjState(gainax, gainaxstate);
				gainax->flags2 |= MF2_AMBUSH;
				player->pflags |= PF_GAINAX;
			}
		}
		else if (K_GetForwardMove(player) < 0 && destGlanceDir == 0)
		{
			// Reversing -- like looking back, but doesn't stack on the other glances.
			if (player->glanceDir != 0)
			{
				// Keep to the side you were already on.
				if (player->glanceDir < 0)
				{
					destGlanceDir = -1;
				}
				else
				{
					destGlanceDir = 1;
				}
			}
			else
			{
				// Look to your right by default
				destGlanceDir = -1;
			}
		}
	}
	else
	{
		// Not glancing
		destGlanceDir = 0;
		player->glanceDir = 0;
	}

#define SetState(sn) \
	if (player->mo->state != &states[sn]) \
		P_SetPlayerMobjState(player->mo, sn)

	if (onground == false)
	{
		// Only use certain frames in the air, to make it look like your tires are spinning fruitlessly!

		if (drift > 0)
		{
			// Neutral drift
			SetState(S_KART_DRIFT_L);
		}
		else if (drift < 0)
		{
			// Neutral drift
			SetState(S_KART_DRIFT_R);
		}
		else
		{
			if (turndir == -1)
			{
				SetState(S_KART_FAST_R);
			}
			else if (turndir == 1)
			{
				SetState(S_KART_FAST_L);
			}
			else
			{
				switch (player->glanceDir)
				{
					case -2:
						SetState(S_KART_FAST_LOOK_R);
						break;
					case 2:
						SetState(S_KART_FAST_LOOK_L);
						break;
					case -1:
						SetState(S_KART_FAST_GLANCE_R);
						break;
					case 1:
						SetState(S_KART_FAST_GLANCE_L);
						break;
					default:
						SetState(S_KART_FAST);
						break;
				}
			}
		}

		if (!spinningwheels)
		{
			// TODO: The "tires still in the air" states should have it's own SPR2s.
			// This was a quick hack to get the same functionality with less work,
			// but it's really dunderheaded & isn't customizable at all.
			player->mo->frame = (player->mo->frame & ~FF_FRAMEMASK);
			player->mo->tics++; // Makes it properly use frame 0
		}
	}
	else
	{
		if (drift > 0)
		{
			// Drifting LEFT!

			if (turndir == -1)
			{
				// Right -- outwards drift
				SetState(S_KART_DRIFT_L_OUT);
			}
			else if (turndir == 1)
			{
				// Left -- inwards drift
				SetState(S_KART_DRIFT_L_IN);
			}
			else
			{
				// Neutral drift
				SetState(S_KART_DRIFT_L);
			}
		}
		else if (drift < 0)
		{
			// Drifting RIGHT!

			if (turndir == -1)
			{
				// Right -- inwards drift
				SetState(S_KART_DRIFT_R_IN);
			}
			else if (turndir == 1)
			{
				// Left -- outwards drift
				SetState(S_KART_DRIFT_R_OUT);
			}
			else
			{
				// Neutral drift
				SetState(S_KART_DRIFT_R);
			}
		}
		else
		{
			if (player->speed >= fastspeed && player->speed >= (player->lastspeed - speedthreshold))
			{
				// Going REAL fast!

				if (turndir == -1)
				{
					SetState(S_KART_FAST_R);
				}
				else if (turndir == 1)
				{
					SetState(S_KART_FAST_L);
				}
				else
				{
					switch (player->glanceDir)
					{
						case -2:
							SetState(S_KART_FAST_LOOK_R);
							break;
						case 2:
							SetState(S_KART_FAST_LOOK_L);
							break;
						case -1:
							SetState(S_KART_FAST_GLANCE_R);
							break;
						case 1:
							SetState(S_KART_FAST_GLANCE_L);
							break;
						default:
							SetState(S_KART_FAST);
							break;
					}
				}
			}
			else
			{
				if (spinningwheels)
				{
					// Drivin' slow.

					if (turndir == -1)
					{
						SetState(S_KART_SLOW_R);
					}
					else if (turndir == 1)
					{
						SetState(S_KART_SLOW_L);
					}
					else
					{
						switch (player->glanceDir)
						{
							case -2:
								SetState(S_KART_SLOW_LOOK_R);
								break;
							case 2:
								SetState(S_KART_SLOW_LOOK_L);
								break;
							case -1:
								SetState(S_KART_SLOW_GLANCE_R);
								break;
							case 1:
								SetState(S_KART_SLOW_GLANCE_L);
								break;
							default:
								SetState(S_KART_SLOW);
								break;
						}
					}
				}
				else
				{
					// Completely still.

					if (turndir == -1)
					{
						SetState(S_KART_STILL_R);
					}
					else if (turndir == 1)
					{
						SetState(S_KART_STILL_L);
					}
					else
					{
						switch (player->glanceDir)
						{
							case -2:
								SetState(S_KART_STILL_LOOK_R);
								break;
							case 2:
								SetState(S_KART_STILL_LOOK_L);
								break;
							case -1:
								SetState(S_KART_STILL_GLANCE_R);
								break;
							case 1:
								SetState(S_KART_STILL_GLANCE_L);
								break;
							default:
								SetState(S_KART_STILL);
								break;
						}
					}
				}
			}
		}
	}

#undef SetState

	// Update your glance value to smooth it out.
	if (player->glanceDir > destGlanceDir)
	{
		player->glanceDir--;
	}
	else if (player->glanceDir < destGlanceDir)
	{
		player->glanceDir++;
	}

	if (!player->glanceDir)
		player->pflags &= ~PF_GAINAX;

	// Update lastspeed value -- we use to display slow driving frames instead of fast driving when slowing down.
	player->lastspeed = player->speed;
}

static void K_TauntVoiceTimers(player_t *player)
{
	if (!player)
		return;

	player->karthud[khud_tauntvoices] = 6*TICRATE;
	player->karthud[khud_voices] = 4*TICRATE;
}

static void K_RegularVoiceTimers(player_t *player)
{
	if (!player)
		return;

	player->karthud[khud_voices] = 4*TICRATE;

	if (player->karthud[khud_tauntvoices] < 4*TICRATE)
		player->karthud[khud_tauntvoices] = 4*TICRATE;
}

void K_PlayAttackTaunt(mobj_t *source)
{
	sfxenum_t pick = P_RandomKey(PR_VOICES, 2); // Gotta roll the RNG every time this is called for sync reasons
	boolean tasteful = (!source->player || !source->player->karthud[khud_tauntvoices]);

	if (cv_kartvoices.value && (tasteful || cv_kartvoices.value == 2))
		S_StartSound(source, sfx_kattk1+pick);

	if (!tasteful)
		return;

	K_TauntVoiceTimers(source->player);
}

void K_PlayBoostTaunt(mobj_t *source)
{
	sfxenum_t pick = P_RandomKey(PR_VOICES, 2); // Gotta roll the RNG every time this is called for sync reasons
	boolean tasteful = (!source->player || !source->player->karthud[khud_tauntvoices]);

	if (cv_kartvoices.value && (tasteful || cv_kartvoices.value == 2))
		S_StartSound(source, sfx_kbost1+pick);

	if (!tasteful)
		return;

	K_TauntVoiceTimers(source->player);
}

void K_PlayOvertakeSound(mobj_t *source)
{
	boolean tasteful = (!source->player || !source->player->karthud[khud_voices]);

	if (!gametype == GT_RACE) // Only in race
		return;

	// 4 seconds from before race begins, 10 seconds afterwards
	if (leveltime < starttime+(10*TICRATE))
		return;

	if (cv_kartvoices.value && (tasteful || cv_kartvoices.value == 2))
		S_StartSound(source, sfx_kslow);

	if (!tasteful)
		return;

	K_RegularVoiceTimers(source->player);
}

void K_PlayPainSound(mobj_t *source, mobj_t *other)
{
	sfxenum_t pick = P_RandomKey(PR_VOICES, 2); // Gotta roll the RNG every time this is called for sync reasons

	sfxenum_t sfx_id = ((skin_t *)source->skin)->soundsid[S_sfx[sfx_khurt1 + pick].skinsound];
	boolean alwaysHear = false;

	if (other != NULL && P_MobjWasRemoved(other) == false && other->player != NULL)
	{
		alwaysHear = P_IsDisplayPlayer(other->player);
	}

	if (cv_kartvoices.value)
	{
		S_StartSound(alwaysHear ? NULL : source, sfx_id);
	}

	K_RegularVoiceTimers(source->player);
}

void K_PlayHitEmSound(mobj_t *source, mobj_t *other)
{
	sfxenum_t sfx_id = ((skin_t *)source->skin)->soundsid[S_sfx[sfx_khitem].skinsound];
	boolean alwaysHear = false;

	if (other != NULL && P_MobjWasRemoved(other) == false && other->player != NULL)
	{
		alwaysHear = P_IsDisplayPlayer(other->player);
	}

	if (cv_kartvoices.value)
	{
		S_StartSound(alwaysHear ? NULL : source, sfx_id);
	}

	K_RegularVoiceTimers(source->player);
}

void K_TryHurtSoundExchange(mobj_t *victim, mobj_t *attacker)
{
	if (victim == NULL || P_MobjWasRemoved(victim) == true || victim->player == NULL)
	{
		return;
	}

	// In a perfect world we could move this here, but there's 
	// a few niche situations where we want a pain sound from
	// the victim, but no confirm sound from the attacker.
	// (ex: DMG_STING)

	//K_PlayPainSound(victim, attacker);

	if (attacker == NULL || P_MobjWasRemoved(attacker) == true || attacker->player == NULL)
	{
		return;
	}

	attacker->player->confirmVictim = (victim->player - players);
	attacker->player->confirmVictimDelay = TICRATE/2;

	if (attacker->player->follower != NULL)
	{
		const follower_t *fl = &followers[attacker->player->followerskin];
		attacker->player->follower->movecount = fl->hitconfirmtime; // movecount is used to play the hitconfirm animation for followers.
	}
}

void K_PlayPowerGloatSound(mobj_t *source)
{
	if (cv_kartvoices.value)
	{
		S_StartSound(source, sfx_kgloat);
	}

	K_RegularVoiceTimers(source->player);
}

static void K_HandleDelayedHitByEm(player_t *player)
{
	if (player->confirmVictimDelay == 0)
	{
		return;
	}

	player->confirmVictimDelay--;

	if (player->confirmVictimDelay == 0)
	{
		mobj_t *victim = NULL;

		if (player->confirmVictim < MAXPLAYERS && playeringame[player->confirmVictim])
		{
			player_t *victimPlayer = &players[player->confirmVictim];

			if (victimPlayer != NULL && victimPlayer->spectator == false)
			{
				victim = victimPlayer->mo;
			}
		}

		K_PlayHitEmSound(player->mo, victim);
	}
}

void K_MomentumToFacing(player_t *player)
{
	angle_t dangle = player->mo->angle - K_MomentumAngle(player->mo);

	if (dangle > ANGLE_180)
		dangle = InvAngle(dangle);

	// If you aren't on the ground or are moving in too different of a direction don't do this
	if (player->mo->eflags & MFE_JUSTHITFLOOR)
		; // Just hit floor ALWAYS redirects
	else if (!P_IsObjectOnGround(player->mo) || dangle > ANGLE_90)
		return;

	P_Thrust(player->mo, player->mo->angle, player->speed - FixedMul(player->speed, player->mo->friction));
	player->mo->momx = FixedMul(player->mo->momx - player->cmomx, player->mo->friction) + player->cmomx;
	player->mo->momy = FixedMul(player->mo->momy - player->cmomy, player->mo->friction) + player->cmomy;
}

boolean K_ApplyOffroad(player_t *player)
{
	if (player->invincibilitytimer || player->hyudorotimer || player->sneakertimer)
		return false;
	return true;
}

boolean K_SlopeResistance(player_t *player)
{
	if (player->invincibilitytimer || player->sneakertimer || player->tiregrease || player->flamedash)
		return true;
	return false;
}

tripwirepass_t K_TripwirePassConditions(player_t *player)
{
	if (
			player->invincibilitytimer ||
			player->sneakertimer
		)
		return TRIPWIRE_BLASTER;

	if (
			player->flamedash ||
			player->speed > 2 * K_GetKartSpeed(player, false, false)
	)
		return TRIPWIRE_BOOST;

	if (
			player->growshrinktimer > 0 ||
			player->hyudorotimer
		)
		return TRIPWIRE_IGNORE;

	return TRIPWIRE_NONE;
}

boolean K_TripwirePass(player_t *player)
{
	return (player->tripwirePass != TRIPWIRE_NONE);
}

boolean K_MovingHorizontally(mobj_t *mobj)
{
	return (P_AproxDistance(mobj->momx, mobj->momy) / 5 > abs(mobj->momz));
}

boolean K_WaterRun(mobj_t *mobj)
{
	switch (mobj->type)
	{
		case MT_JAWZ:
		{
			if (mobj->tracer != NULL && P_MobjWasRemoved(mobj->tracer) == false)
			{
				fixed_t jawzFeet = P_GetMobjFeet(mobj);
				fixed_t chaseFeet = P_GetMobjFeet(mobj->tracer);
				fixed_t footDiff = (chaseFeet - jawzFeet) * P_MobjFlip(mobj);

				// Water run if the player we're chasing is above/equal to us.
				// Start water skipping if they're underneath the water.
				return (footDiff > -mobj->tracer->height);
			}

			return false;
		}

		case MT_PLAYER:
		{
			if (mobj->player == NULL)
			{
				return false;
			}

			if (mobj->player->invincibilitytimer
				|| mobj->player->sneakertimer
				|| mobj->player->tiregrease
				|| mobj->player->flamedash
				|| mobj->player->speed > 2 * K_GetKartSpeed(mobj->player, false, false))
			{
				return true;
			}

			return false;
		}

		default:
		{
			return false;
		}
	}
}

boolean K_WaterSkip(mobj_t *mobj)
{
	if (mobj->waterskip >= 2)
	{
		// Already finished waterskipping.
		return false;
	}

	switch (mobj->type)
	{
		case MT_PLAYER:
		case MT_ORBINAUT:
		case MT_JAWZ:
		case MT_BALLHOG:
		{
			// Allow
			break;
		}

		default:
		{
			// Don't allow
			return false;
		}
	}

	if (mobj->waterskip > 0)
	{
		// Already waterskipping.
		// Simply make sure you haven't slowed down drastically.
		return (P_AproxDistance(mobj->momx, mobj->momy) > 20 * mapobjectscale);
	}
	else
	{
		// Need to be moving horizontally and not vertically
		// to be able to start a water skip.
		return K_MovingHorizontally(mobj);
	}
}

void K_SpawnWaterRunParticles(mobj_t *mobj)
{
	fixed_t runSpeed = 14 * mobj->scale;
	fixed_t curSpeed = INT32_MAX;
	fixed_t topSpeed = INT32_MAX;
	fixed_t trailScale = FRACUNIT;

	if (mobj->momz != 0)
	{
		// Only while touching ground.
		return;
	}

	if (mobj->watertop == INT32_MAX || mobj->waterbottom == INT32_MIN)
	{
		// Invalid water plane.
		return;
	}

	if (mobj->player != NULL)
	{
		if (mobj->player->spectator)
		{
			// Not as spectator.
			return;
		}

		if (mobj->player->carry == CR_SLIDING)
		{
			// Not in water slides.
			return;
		}

		topSpeed = K_GetKartSpeed(mobj->player, false, false);
		runSpeed = FixedMul(runSpeed, mobj->movefactor);
	}
	else
	{
		topSpeed = FixedMul(mobj->scale, K_GetKartSpeedFromStat(5));
	}

	curSpeed = P_AproxDistance(mobj->momx, mobj->momy);

	if (curSpeed <= runSpeed)
	{
		// Not fast enough.
		return;
	}

	// Near the water plane.
	if ((!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height >= mobj->watertop && mobj->z <= mobj->watertop)
		|| (mobj->eflags & MFE_VERTICALFLIP && mobj->z + mobj->height >= mobj->waterbottom && mobj->z <= mobj->waterbottom))
	{
		if (topSpeed > runSpeed)
		{
			trailScale = FixedMul(FixedDiv(curSpeed - runSpeed, topSpeed - runSpeed), mapobjectscale);
		}
		else
		{
			trailScale = mapobjectscale; // Scaling is based off difference between runspeed and top speed
		}

		if (trailScale > 0)
		{
			const angle_t forwardangle = K_MomentumAngle(mobj);
			const fixed_t playerVisualRadius = mobj->radius + (8 * mobj->scale);
			const size_t numFrames = S_WATERTRAIL8 - S_WATERTRAIL1;
			const statenum_t curOverlayFrame = S_WATERTRAIL1 + (leveltime % numFrames);
			const statenum_t curUnderlayFrame = S_WATERTRAILUNDERLAY1 + (leveltime % numFrames);
			fixed_t x1, x2, y1, y2;
			mobj_t *water;

			x1 = mobj->x + mobj->momx + P_ReturnThrustX(mobj, forwardangle + ANGLE_90, playerVisualRadius);
			y1 = mobj->y + mobj->momy + P_ReturnThrustY(mobj, forwardangle + ANGLE_90, playerVisualRadius);
			x1 = x1 + P_ReturnThrustX(mobj, forwardangle, playerVisualRadius);
			y1 = y1 + P_ReturnThrustY(mobj, forwardangle, playerVisualRadius);

			x2 = mobj->x + mobj->momx + P_ReturnThrustX(mobj, forwardangle - ANGLE_90, playerVisualRadius);
			y2 = mobj->y + mobj->momy + P_ReturnThrustY(mobj, forwardangle - ANGLE_90, playerVisualRadius);
			x2 = x2 + P_ReturnThrustX(mobj, forwardangle, playerVisualRadius);
			y2 = y2 + P_ReturnThrustY(mobj, forwardangle, playerVisualRadius);

			// Left
			// underlay
			water = P_SpawnMobj(x1, y1,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAILUNDERLAY].height, mobj->scale) : mobj->watertop), MT_WATERTRAILUNDERLAY);
			P_InitAngle(water, forwardangle - ANGLE_180 - ANGLE_22h);
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curUnderlayFrame);

			// overlay
			water = P_SpawnMobj(x1, y1,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAIL].height, mobj->scale) : mobj->watertop), MT_WATERTRAIL);
			P_InitAngle(water, forwardangle - ANGLE_180 - ANGLE_22h);
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curOverlayFrame);

			// Right
			// Underlay
			water = P_SpawnMobj(x2, y2,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAILUNDERLAY].height, mobj->scale) : mobj->watertop), MT_WATERTRAILUNDERLAY);
			P_InitAngle(water, forwardangle - ANGLE_180 + ANGLE_22h);
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curUnderlayFrame);

			// Overlay
			water = P_SpawnMobj(x2, y2,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAIL].height, mobj->scale) : mobj->watertop), MT_WATERTRAIL);
			P_InitAngle(water, forwardangle - ANGLE_180 + ANGLE_22h);
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curOverlayFrame);

			if (!S_SoundPlaying(mobj, sfx_s3kdbs))
			{
				const INT32 volume = (min(trailScale, FRACUNIT) * 255) / FRACUNIT;
				S_StartSoundAtVolume(mobj, sfx_s3kdbs, volume);
			}
		}

		// Little water sound while touching water - just a nicety.
		if ((mobj->eflags & MFE_TOUCHWATER) && !(mobj->eflags & MFE_UNDERWATER))
		{
			if (P_RandomChance(PR_BUBBLE, FRACUNIT/2) && leveltime % TICRATE == 0)
			{
				S_StartSound(mobj, sfx_floush);
			}
		}
	}
}

static fixed_t K_FlameShieldDashVar(INT32 val)
{
	// 1 second = 75% + 50% top speed
	return (3*FRACUNIT/4) + (((val * FRACUNIT) / TICRATE) / 2);
}

INT16 K_GetSpindashChargeTime(player_t *player)
{
	// more charge time for higher speed
	// Tails = 2s, Knuckles = 2.6s, Metal = 3.2s
	return (player->kartspeed + 8) * (TICRATE/5);
}

fixed_t K_GetSpindashChargeSpeed(player_t *player)
{
	// more speed for higher weight & speed
	// Tails = +18.75%, Fang = +46.88%, Mighty = +46.88%, Metal = +56.25%
	// (can be higher than this value when overcharged)
	const fixed_t val = ((player->kartspeed + player->kartweight) + 2) * (FRACUNIT/32);

	// TODO: gametyperules
	return (gametype == GT_BATTLE) ? (4 * val) : val;
}

// sets boostpower, speedboost, accelboost, and handleboost to whatever we need it to be
static void K_GetKartBoostPower(player_t *player)
{
	// Light weights have stronger boost stacking -- aka, better metabolism than heavies XD
	const fixed_t maxmetabolismincrease = FRACUNIT/2;
	const fixed_t metabolism = FRACUNIT - ((9-player->kartweight) * maxmetabolismincrease / 8);

	// v2 almost broke sliptiding when it fixed turning bugs!
	// This value is fine-tuned to feel like v1 again without reverting any of those changes.
	const fixed_t sliptidehandling = FRACUNIT/2;

	fixed_t boostpower = FRACUNIT;
	fixed_t speedboost = 0, accelboost = 0, handleboost = 0;
	UINT8 numboosts = 0;

	if (player->spinouttimer && player->wipeoutslow == 1) // Slow down after you've been bumped
	{
		player->boostpower = player->speedboost = player->accelboost = 0;
		return;
	}

	// Offroad is separate, it's difficult to factor it in with a variable value anyway.
	if (K_ApplyOffroad(player) && player->offroad >= 0)
		boostpower = FixedDiv(boostpower, FixedMul(player->offroad, K_GetKartGameSpeedScalar(gamespeed)) + FRACUNIT);

	if (player->bananadrag > TICRATE)
		boostpower = (4*boostpower)/5;

	// Note: Handling will ONLY stack when sliptiding!
	// When you're not, it just uses the best instead of adding together, like the old behavior.
#define ADDBOOST(s,a,h) { \
	numboosts++; \
	speedboost += FixedDiv(s, FRACUNIT + (metabolism * (numboosts-1))); \
	accelboost += FixedDiv(a, FRACUNIT + (metabolism * (numboosts-1))); \
	if (player->aizdriftstrat) \
		handleboost += FixedDiv(h, FRACUNIT + (metabolism * (numboosts-1))); \
	else \
		handleboost = max(h, handleboost); \
}

	if (player->sneakertimer) // Sneaker
	{
		UINT8 i;
		for (i = 0; i < player->numsneakers; i++)
		{
			ADDBOOST(FRACUNIT/2, 8*FRACUNIT, sliptidehandling); // + 50% top speed, + 800% acceleration, +50% handling
		}
	}

	if (player->invincibilitytimer) // Invincibility
	{
		ADDBOOST(3*FRACUNIT/8, 3*FRACUNIT, sliptidehandling/2); // + 37.5% top speed, + 300% acceleration, +25% handling
	}

	if (player->growshrinktimer > 0) // Grow
	{
		ADDBOOST(0, 0, sliptidehandling/2); // + 0% top speed, + 0% acceleration, +25% handling
	}

	if (player->flamedash) // Flame Shield dash
	{
		fixed_t dash = K_FlameShieldDashVar(player->flamedash);
		ADDBOOST(
			dash, // + infinite top speed
			3*FRACUNIT, // + 300% acceleration
			FixedMul(FixedDiv(dash, FRACUNIT/2), sliptidehandling/2) // + infinite handling
		);
	}

	if (player->spindashboost) // Spindash boost
	{
		const fixed_t MAXCHARGESPEED = K_GetSpindashChargeSpeed(player);
		const fixed_t exponent = FixedMul(player->spindashspeed, player->spindashspeed);

		// character & charge dependent
		ADDBOOST(
			FixedMul(MAXCHARGESPEED, exponent), // + 0 to K_GetSpindashChargeSpeed()% top speed
			(40 * exponent), // + 0% to 4000% acceleration
			0 // + 0% handling
		);
	}

	if (player->startboost) // Startup Boost
	{
		ADDBOOST(FRACUNIT, 4*FRACUNIT, sliptidehandling); // + 100% top speed, + 400% acceleration, +50% handling
	}

	if (player->driftboost) // Drift Boost
	{
		// Rebuff Eggman's stat block corner
		const INT32 heavyAccel = ((9 - player->kartspeed) * 2) + (player->kartweight - 1);
		const fixed_t heavyAccelBonus = FRACUNIT + ((heavyAccel * maxmetabolismincrease * 2) / 24);

		fixed_t driftSpeed = FRACUNIT/4; // 25% base

		if (player->strongdriftboost > 0)
		{
			// Purple/Rainbow drift boost
			driftSpeed = FixedMul(driftSpeed, 4*FRACUNIT/3); // 25% -> 33%
		}

		// Bottom-left bonus
		driftSpeed = FixedMul(driftSpeed, heavyAccelBonus);

		ADDBOOST(driftSpeed, 4*FRACUNIT, 0); // + variable top speed, + 400% acceleration, +0% handling
	}

	if (player->trickboost)	// Trick pannel up-boost
	{
		ADDBOOST(player->trickboostpower, 5*FRACUNIT, 0);	// <trickboostpower>% speed, 500% accel, 0% handling
	}

	if (player->gateBoost) // SPB Juicebox boost
	{
		ADDBOOST(3*FRACUNIT/4, 4*FRACUNIT, sliptidehandling/2); // + 75% top speed, + 400% acceleration, +25% handling
	}

	if (player->ringboost) // Ring Boost
	{
		ADDBOOST(FRACUNIT/4, 4*FRACUNIT, 0); // + 20% top speed, + 400% acceleration, +0% handling
	}

	if (player->eggmanexplode) // Ready-to-explode
	{
		ADDBOOST(3*FRACUNIT/20, FRACUNIT, 0); // + 15% top speed, + 100% acceleration, +0% handling
	}

	if (player->draftpower > 0) // Drafting
	{
		// 30% - 44%, each point of speed adds 1.75%
		fixed_t draftspeed = ((3*FRACUNIT)/10) + ((player->kartspeed-1) * ((7*FRACUNIT)/400));

		if (gametype == GT_BATTLE)
		{
			// TODO: gametyperules
			draftspeed *= 2;
		}

		if (player->itemtype == KITEM_LIGHTNINGSHIELD)
		{
			// infinite tether
			draftspeed *= 2;
		}

		speedboost += FixedMul(draftspeed, player->draftpower); // (Drafting suffers no boost stack penalty.)
		numboosts++;
	}

	player->boostpower = boostpower;

	// value smoothing
	if (speedboost > player->speedboost)
	{
		player->speedboost = speedboost;
	}
	else
	{
		player->speedboost += (speedboost - player->speedboost) / (TICRATE/2);
	}

	player->accelboost = accelboost;
	player->handleboost = handleboost;

	player->numboosts = numboosts;
}

fixed_t K_GrowShrinkSpeedMul(player_t *player)
{
	fixed_t scaleDiff = player->mo->scale - mapobjectscale;
	fixed_t playerScale = FixedDiv(player->mo->scale, mapobjectscale);
	fixed_t speedMul = FRACUNIT;

	if (scaleDiff > 0)
	{
		// Grown
		// Change x2 speed into x1.5
		speedMul = FixedDiv(FixedMul(playerScale, GROW_PHYSICS_SCALE), GROW_SCALE);
	}
	else if (scaleDiff < 0)
	{
		// Shrunk
		// Change x0.5 speed into x0.75
		speedMul = FixedDiv(FixedMul(playerScale, SHRINK_PHYSICS_SCALE), SHRINK_SCALE);
	}

	return speedMul;
}

// Returns kart speed from a stat. Boost power and scale are NOT taken into account, no player or object is necessary.
fixed_t K_GetKartSpeedFromStat(UINT8 kartspeed)
{
	const fixed_t xspd = (3*FRACUNIT)/64;
	fixed_t g_cc = K_GetKartGameSpeedScalar(gamespeed) + xspd;
	fixed_t k_speed = 148;
	fixed_t finalspeed;

	k_speed += kartspeed*4; // 152 - 184

	finalspeed = FixedMul(k_speed<<14, g_cc);
	return finalspeed;
}

fixed_t K_GetKartSpeed(player_t *player, boolean doboostpower, boolean dorubberband)
{
	const boolean mobjValid = (player->mo != NULL && P_MobjWasRemoved(player->mo) == false);
	fixed_t finalspeed = K_GetKartSpeedFromStat(player->kartspeed);

	if (gametyperules & GTR_BUMPERS && player->bumpers <= 0)
		finalspeed = 3 * finalspeed / 2;

	if (player->spheres > 0)
	{
		fixed_t sphereAdd = (FRACUNIT/40); // 100% at max
		finalspeed = FixedMul(finalspeed, FRACUNIT + (sphereAdd * player->spheres));
	}

	if (K_PlayerUsesBotMovement(player))
	{
		// Increase bot speed by 1-10% depending on difficulty
		fixed_t add = (player->botvars.difficulty * (FRACUNIT/10)) / DIFFICULTBOT;
		finalspeed = FixedMul(finalspeed, FRACUNIT + add);

		if (player->bot && player->botvars.rival)
		{
			// +10% top speed for the rival
			finalspeed = FixedMul(finalspeed, 11*FRACUNIT/10);
		}
	}

	finalspeed = FixedMul(finalspeed, mapobjectscale);

	if (doboostpower == true)
	{
		if (mobjValid == true)
		{
			// Scale with the player.
			finalspeed = FixedMul(finalspeed, K_GrowShrinkSpeedMul(player));
		}

		finalspeed = FixedMul(finalspeed, player->boostpower + player->speedboost);
	}

	if (dorubberband == true && K_PlayerUsesBotMovement(player) == true)
	{
		finalspeed = FixedMul(finalspeed, player->botvars.rubberband);
	}

	return finalspeed;
}

fixed_t K_GetKartAccel(player_t *player)
{
	fixed_t k_accel = 121;

	k_accel += 17 * (9 - player->kartspeed); // 121 - 257

	// karma bomb gets 2x acceleration
	if (gametype == GT_BATTLE && player->bumpers <= 0)
		k_accel *= 2;

	return FixedMul(k_accel, (FRACUNIT + player->accelboost) / 4);
}

UINT16 K_GetKartFlashing(player_t *player)
{
	UINT16 tics = flashingtics;

	if (gametype == GT_BATTLE)
	{
		// TODO: gametyperules
		return 1;
	}

	if (player == NULL)
	{
		return tics;
	}

	tics += (tics/8) * (player->kartspeed);
	return tics;
}

boolean K_PlayerShrinkCheat(player_t *player)
{
	return (
		(player->pflags & PF_SHRINKACTIVE)
		&& (player->bot == false)
		&& (modeattacking == false) // Anyone want to make another record attack category?
	);
}

void K_UpdateShrinkCheat(player_t *player)
{
	const boolean mobjValid = (player->mo != NULL && P_MobjWasRemoved(player->mo) == false);

	if (player->pflags & PF_SHRINKME)
	{
		player->pflags |= PF_SHRINKACTIVE;
	}
	else
	{
		player->pflags &= ~PF_SHRINKACTIVE;
	}

	if (mobjValid == true && K_PlayerShrinkCheat(player) == true)
	{
		player->mo->destscale = FixedMul(mapobjectscale, SHRINK_SCALE);
	}
}

boolean K_KartKickstart(player_t *player)
{
	return ((player->pflags & PF_KICKSTARTACCEL)
		&& (!K_PlayerUsesBotMovement(player))
		&& (player->kickstartaccel >= ACCEL_KICKSTART));
}

UINT16 K_GetKartButtons(player_t *player)
{
	return (player->cmd.buttons |
		(K_KartKickstart(player) ? BT_ACCELERATE : 0));
}

SINT8 K_GetForwardMove(player_t *player)
{
	SINT8 forwardmove = player->cmd.forwardmove;

	if ((player->pflags & PF_STASIS) || (player->carry == CR_SLIDING))
	{
		return 0;
	}

	if (player->sneakertimer || player->spindashboost)
	{
		return MAXPLMOVE;
	}

	if (player->spinouttimer || K_PlayerEBrake(player))
	{
		return 0;
	}

	if (K_KartKickstart(player)) // unlike the brute forward of sneakers, allow for backwards easing here
	{
		forwardmove += MAXPLMOVE;
		if (forwardmove > MAXPLMOVE)
			forwardmove = MAXPLMOVE;
	}

	return forwardmove;
}

fixed_t K_GetNewSpeed(player_t *player)
{
	const fixed_t accelmax = 4000;
	const fixed_t p_speed = K_GetKartSpeed(player, true, true);
	fixed_t p_accel = K_GetKartAccel(player);

	fixed_t newspeed, oldspeed, finalspeed;

	if (K_PlayerUsesBotMovement(player) == true && player->botvars.rubberband > 0)
	{
		// Acceleration is tied to top speed...
		// so if we want JUST a top speed boost, we have to do this...
		p_accel = FixedDiv(p_accel, player->botvars.rubberband);
	}

	oldspeed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);
	// Don't calculate the acceleration as ever being above top speed
	if (oldspeed > p_speed)
		oldspeed = p_speed;
	newspeed = FixedDiv(FixedDiv(FixedMul(oldspeed, accelmax - p_accel) + FixedMul(p_speed, p_accel), accelmax), ORIG_FRICTION);

	finalspeed = newspeed - oldspeed;

	return finalspeed;
}

fixed_t K_3dKartMovement(player_t *player)
{
	fixed_t finalspeed = K_GetNewSpeed(player);

	fixed_t movemul = FRACUNIT;
	SINT8 forwardmove = K_GetForwardMove(player);

	movemul = abs(forwardmove * FRACUNIT) / 50;

	// forwardmove is:
	//  50 while accelerating,
	//   0 with no gas, and
	// -25 when only braking.
	if (forwardmove >= 0)
	{
		finalspeed = FixedMul(finalspeed, movemul);
	}
	else
	{
		finalspeed = FixedMul(-mapobjectscale/2, movemul);
	}

	return finalspeed;
}

angle_t K_MomentumAngle(mobj_t *mo)
{
	if (FixedHypot(mo->momx, mo->momy) >= mo->scale)
	{
		return R_PointToAngle2(0, 0, mo->momx, mo->momy);
	}
	else
	{
		return mo->angle; // default to facing angle, rather than 0
	}
}

void K_AddHitLag(mobj_t *mo, INT32 tics, boolean fromDamage)
{
	if (mo == NULL || P_MobjWasRemoved(mo) || (mo->flags & MF_NOHITLAGFORME && mo->type != MT_PLAYER))
	{
		return;
	}

	mo->hitlag += tics;
	mo->hitlag = min(mo->hitlag, MAXHITLAGTICS);

	if (mo->player != NULL)
	{
		// Reset each time. We want to explicitly set this for bananas afterwards,
		// so make sure an old value doesn't possibly linger.
		mo->player->flipDI = false;
	}

	if (fromDamage == true)
	{
		// Dunno if this should flat-out &~ the flag out too.
		// Decided it probably just just keep it since it's "adding" hitlag.
		mo->eflags |= MFE_DAMAGEHITLAG;
	}
}

void K_SetHitLagForObjects(mobj_t *mo1, mobj_t *mo2, INT32 tics, boolean fromDamage)
{
	INT32 finalTics = tics;

	if (tics <= 0)
	{
		return;
	}

	if ((mo1 && !P_MobjWasRemoved(mo1)) == true && (mo2 && !P_MobjWasRemoved(mo2)) == true)
	{
		const fixed_t speedTicFactor = (mapobjectscale * 8);
		const INT32 angleTicFactor = ANGLE_22h;

		const fixed_t mo1speed = FixedHypot(FixedHypot(mo1->momx, mo1->momy), mo1->momz);
		const fixed_t mo2speed = FixedHypot(FixedHypot(mo2->momx, mo2->momy), mo2->momz);
		const fixed_t speedDiff = abs(mo2speed - mo1speed);

		const fixed_t scaleDiff = abs(mo2->scale - mo1->scale);

		angle_t mo1angle = K_MomentumAngle(mo1);
		angle_t mo2angle = K_MomentumAngle(mo2);
		INT32 angleDiff = 0;

		if (mo1speed > 0 && mo2speed > 0)
		{
			// If either object is completely not moving, their speed doesn't matter.
			angleDiff = AngleDelta(mo1angle, mo2angle);
		}

		// Add extra "damage" based on what was happening to the objects on impact.
		finalTics += (FixedMul(speedDiff, FRACUNIT + scaleDiff) / speedTicFactor) + (angleDiff / angleTicFactor);

		// This shouldn't happen anymore, but just in case something funky happens.
		if (finalTics < tics)
		{
			finalTics = tics;
		}
	}

	K_AddHitLag(mo1, finalTics, fromDamage);
	K_AddHitLag(mo2, finalTics, false); // mo2 is the inflictor, so don't use the damage property.
}

void K_AwardPlayerRings(player_t *player, INT32 rings, boolean overload)
{
	UINT16 superring;

	if (!overload)
	{
		INT32 totalrings =
			RINGTOTAL(player) + (player->superring / 3);

		/* capped at 20 rings */
		if ((totalrings + rings) > 20)
			rings = (20 - totalrings);
	}

	superring = player->superring + (rings * 3);

	/* check if not overflow */
	if (superring > player->superring)
		player->superring = superring;
}

void K_DoInstashield(player_t *player)
{
	mobj_t *layera;
	mobj_t *layerb;

	if (player->instashield > 0)
		return;

	player->instashield = 15; // length of instashield animation
	S_StartSound(player->mo, sfx_cdpcm9);

	layera = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INSTASHIELDA);
	layera->old_x = player->mo->old_x;
	layera->old_y = player->mo->old_y;
	layera->old_z = player->mo->old_z;
	P_SetTarget(&layera->target, player->mo);

	layerb = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INSTASHIELDB);
	layerb->old_x = player->mo->old_x;
	layerb->old_y = player->mo->old_y;
	layerb->old_z = player->mo->old_z;
	P_SetTarget(&layerb->target, player->mo);
}

void K_DoPowerClash(player_t *t1, player_t *t2) {
	mobj_t *clash;

	// short-circuit instashield for vfx visibility
	t1->instashield = 1;
	t2->instashield = 1;

	S_StartSound(t1->mo, sfx_parry);
	K_AddHitLag(t1->mo, 6, false);
	K_AddHitLag(t2->mo, 6, false);

	clash = P_SpawnMobj((t1->mo->x/2) + (t2->mo->x/2), (t1->mo->y/2) + (t2->mo->y/2), (t1->mo->z/2) + (t2->mo->z/2), MT_POWERCLASH);

	// needs to handle mixed scale collisions (t1 grow t2 invinc)...
	clash->z = clash->z + (t1->mo->height/4) + (t2->mo->height/4);
	clash->angle = R_PointToAngle2(clash->x, clash->y, t1->mo->x, t1->mo->y) + ANGLE_90;

	// Shrink over time (accidental behavior that looked good)
	clash->destscale = (t1->mo->scale/2) + (t2->mo->scale/2);
	P_SetScale(clash, 3*clash->destscale/2);
}

void K_BattleAwardHit(player_t *player, player_t *victim, mobj_t *inflictor, UINT8 bumpersRemoved)
{
	UINT8 points = 1;
	boolean trapItem = false;

	if (player == NULL || victim == NULL)
	{
		// Invalid player or victim
		return;
	}

	if (player == victim)
	{
		// You cannot give yourself points
		return;
	}

	if ((inflictor && !P_MobjWasRemoved(inflictor)) && (inflictor->type == MT_BANANA && inflictor->health > 1))
	{
		trapItem = true;
	}

	// Only apply score bonuses to non-bananas
	if (trapItem == false)
	{
		if (K_IsPlayerWanted(victim))
		{
			// +3 points for hitting a wanted player
			points = 3;
		}
		else if (gametyperules & GTR_BUMPERS)
		{
			if ((victim->bumpers > 0) && (victim->bumpers <= bumpersRemoved))
			{
				// +2 points for finishing off a player
				points = 2;
			}
		}
	}

	if (gametyperules & GTR_POINTLIMIT)
	{
		P_AddPlayerScore(player, points);
		K_SpawnBattlePoints(player, victim, points);
	}
}

void K_SpinPlayer(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 type)
{
	(void)inflictor;
	(void)source;

	K_DirectorFollowAttack(player, inflictor, source);

	player->spinouttype = type;

	if (( player->spinouttype & KSPIN_THRUST ))
	{
		// At spinout, player speed is increased to 1/4 their regular speed, moving them forward
		fixed_t spd = K_GetKartSpeed(player, true, true) / 4;

		if (player->speed < spd)
			P_InstaThrust(player->mo, player->mo->angle, FixedMul(spd, player->mo->scale));

		S_StartSound(player->mo, sfx_slip);
	}

	player->spinouttimer = (3*TICRATE/2)+2;
	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
}

void K_RemoveGrowShrink(player_t *player)
{
	if (player->mo && !P_MobjWasRemoved(player->mo))
	{
		if (player->growshrinktimer > 0) // Play Shrink noise
			S_StartSound(player->mo, sfx_kc59);
		else if (player->growshrinktimer < 0) // Play Grow noise
			S_StartSound(player->mo, sfx_kc5a);

		K_KartResetPlayerColor(player);

		player->mo->scalespeed = mapobjectscale/TICRATE;
		player->mo->destscale = mapobjectscale;

		if (K_PlayerShrinkCheat(player) == true)
		{
			player->mo->destscale = FixedMul(player->mo->destscale, SHRINK_SCALE);
		}
	}

	player->growshrinktimer = 0;

	P_RestoreMusic(player);
}

boolean K_IsBigger(mobj_t *compare, mobj_t *other)
{
	if ((compare == NULL || P_MobjWasRemoved(compare) == true)
		|| (other == NULL || P_MobjWasRemoved(other) == true))
	{
		return false;
	}

	return (compare->scale > other->scale + (mapobjectscale / 4));
}

static fixed_t K_TumbleZ(mobj_t *mo, fixed_t input)
{
	// Scales base tumble gravity to FRACUNIT
	const fixed_t baseGravity = FixedMul(DEFAULT_GRAVITY, TUMBLEGRAVITY);

	// Adapt momz w/ gravity
	fixed_t gravityAdjust = FixedDiv(P_GetMobjGravity(mo), baseGravity);

	if (mo->eflags & MFE_UNDERWATER)
	{
		// Reverse doubled falling speed.
		gravityAdjust /= 2;
	}

	return FixedMul(input, -gravityAdjust);
}

void K_TumblePlayer(player_t *player, mobj_t *inflictor, mobj_t *source)
{
	(void)source;

	K_DirectorFollowAttack(player, inflictor, source);

	player->tumbleBounces = 1;

	if (player->tripwireState == TRIPSTATE_PASSED)
	{
		player->tumbleHeight = 50;
	}
	else
	{
		player->mo->momx = 2 * player->mo->momx / 3;
		player->mo->momy = 2 * player->mo->momy / 3;

		player->tumbleHeight = 30;
	}

	player->pflags &= ~PF_TUMBLESOUND;

	if (inflictor && !P_MobjWasRemoved(inflictor))
	{
		const fixed_t addHeight = FixedHypot(FixedHypot(inflictor->momx, inflictor->momy) / 2, FixedHypot(player->mo->momx, player->mo->momy) / 2);
		player->tumbleHeight += (addHeight / player->mo->scale);
	}

	S_StartSound(player->mo, sfx_s3k9b);

	player->mo->momz = K_TumbleZ(player->mo, player->tumbleHeight * FRACUNIT);

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

	if (P_IsDisplayPlayer(player))
		P_StartQuake(64<<FRACBITS, 10);
}

angle_t K_StumbleSlope(angle_t angle, angle_t pitch, angle_t roll)
{
	fixed_t pitchMul = -FINESINE(angle >> ANGLETOFINESHIFT);
	fixed_t rollMul = FINECOSINE(angle >> ANGLETOFINESHIFT);

	angle_t slope = FixedMul(pitch, pitchMul) + FixedMul(roll, rollMul);

	if (slope > ANGLE_180)
	{
		slope = InvAngle(slope);
	}

	return slope;
}

void K_StumblePlayer(player_t *player)
{
	P_ResetPlayer(player);

#if 0
	// Single, medium bounce
	player->tumbleBounces = TUMBLEBOUNCES;
	player->tumbleHeight = 30;
#else
	// Two small bounces
	player->tumbleBounces = TUMBLEBOUNCES-1;
	player->tumbleHeight = 20;
#endif

	player->pflags &= ~PF_TUMBLESOUND;
	S_StartSound(player->mo, sfx_s3k9b);

	// and then modulate momz like that...
	player->mo->momz = K_TumbleZ(player->mo, player->tumbleHeight * FRACUNIT);

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

	if (P_IsDisplayPlayer(player))
		P_StartQuake(64<<FRACBITS, 10);

	// Reset slope.
	player->mo->pitch = player->mo->roll = 0;
}

boolean K_CheckStumble(player_t *player, angle_t oldPitch, angle_t oldRoll, boolean fromAir)
{
	angle_t steepVal = ANGLE_MAX;
	angle_t oldSlope, newSlope;
	angle_t slopeDelta;

	// If you don't land upright on a slope, then you tumble,
	// kinda like Kirby Air Ride

	if (player->tumbleBounces)
	{
		// Already tumbling.
		return false;
	}

	if ((player->mo->pitch == oldPitch)
		&& (player->mo->roll == oldRoll))
	{
		// No change.
		return false;
	}

	if (fromAir == true)
	{
		steepVal = STUMBLE_STEEP_VAL_AIR;
	}
	else
	{
		steepVal = STUMBLE_STEEP_VAL;
	}

	oldSlope = K_StumbleSlope(player->mo->angle, oldPitch, oldRoll);

	if (oldSlope <= steepVal)
	{
		// Transferring from flat ground to a steep slope
		// is a free action. (The other way around isn't, though.)
		return false;
	}

	newSlope = K_StumbleSlope(player->mo->angle, player->mo->pitch, player->mo->roll);
	slopeDelta = AngleDelta(oldSlope, newSlope);

	if (slopeDelta <= steepVal)
	{
		// Needs to be VERY steep before we'll punish this.
		return false;
	}

	// Oh jeez, you landed on your side.
	// You get to tumble.

	K_StumblePlayer(player);
	return true;
}

void K_InitStumbleIndicator(player_t *player)
{
	mobj_t *new = NULL;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (player->stumbleIndicator != NULL && P_MobjWasRemoved(player->stumbleIndicator) == false)
	{
		P_RemoveMobj(player->stumbleIndicator);
	}

	new = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_SMOOTHLANDING);

	P_SetTarget(&player->stumbleIndicator, new);
	P_SetTarget(&new->target, player->mo);
}

void K_UpdateStumbleIndicator(player_t *player)
{
	const angle_t fudge = ANG15;
	mobj_t *mobj = NULL;

	boolean air = false;
	angle_t steepVal = STUMBLE_STEEP_VAL;
	angle_t slopeSteep = 0;
	angle_t steepRange = ANGLE_90;

	INT32 delta = 0;
	INT32 trans = 0;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (player->stumbleIndicator == NULL || P_MobjWasRemoved(player->stumbleIndicator) == true)
	{
		K_InitStumbleIndicator(player);
		return;
	}

	mobj = player->stumbleIndicator;

	P_MoveOrigin(mobj, player->mo->x, player->mo->y, player->mo->z + (player->mo->height / 2));

	air = !P_IsObjectOnGround(player->mo);
	steepVal = (air ? STUMBLE_STEEP_VAL_AIR : STUMBLE_STEEP_VAL) - fudge;
	slopeSteep = max(AngleDelta(player->mo->pitch, 0), AngleDelta(player->mo->roll, 0));

	delta = 0;

	if (slopeSteep > steepVal)
	{
		angle_t testAngles[2];
		INT32 testDeltas[2];
		UINT8 i;

		testAngles[0] = R_PointToAngle2(0, 0, player->mo->pitch, player->mo->roll);
		testAngles[1] = R_PointToAngle2(0, 0, -player->mo->pitch, -player->mo->roll);

		for (i = 0; i < 2; i++)
		{
			testDeltas[i] = AngleDeltaSigned(player->mo->angle, testAngles[i]);
		}

		if (abs(testDeltas[1]) < abs(testDeltas[0]))
		{
			delta = testDeltas[1];
		}
		else
		{
			delta = testDeltas[0];
		}
	}

	if (delta < 0)
	{
		mobj->renderflags |= RF_HORIZONTALFLIP;
	}
	else
	{
		mobj->renderflags &= ~RF_HORIZONTALFLIP;
	}

	steepRange = ANGLE_90 - steepVal;
	delta = max(0, abs(delta) - ((signed)steepVal));
	trans = ((FixedDiv(AngleFixed(delta), AngleFixed(steepRange)) * (NUMTRANSMAPS - 2)) + (FRACUNIT/2)) / FRACUNIT;

	if (trans < 0)
	{
		trans = 0;
	}

	if (trans > (NUMTRANSMAPS - 2))
	{
		trans = (NUMTRANSMAPS - 2);
	}

	// invert
	trans = (NUMTRANSMAPS - 2) - trans;

	mobj->renderflags |= RF_DONTDRAW;

	if (trans < (NUMTRANSMAPS - 2))
	{
		mobj->renderflags &= ~(RF_TRANSMASK | K_GetPlayerDontDrawFlag(player));

		if (trans != 0)
		{
			mobj->renderflags |= (trans << RF_TRANSSHIFT);
		}
	}
}

static boolean K_LastTumbleBounceCondition(player_t *player)
{
	return (player->tumbleBounces > TUMBLEBOUNCES && player->tumbleHeight < 60);
}

static void K_HandleTumbleBounce(player_t *player)
{
	player->tumbleBounces++;
	player->tumbleHeight = (player->tumbleHeight * ((player->tumbleHeight > 100) ? 3 : 4)) / 5;
	player->pflags &= ~PF_TUMBLESOUND;

	if (player->tumbleHeight < 10)
	{
		// 10 minimum bounce height
		player->tumbleHeight = 10;
	}

	if (K_LastTumbleBounceCondition(player))
	{
		// Leave tumble state when below 40 height, and have bounced off the ground enough

		if (player->pflags & PF_TUMBLELASTBOUNCE)
		{
			// End tumble state
			player->tumbleBounces = 0;
			player->pflags &= ~PF_TUMBLELASTBOUNCE; // Reset for next time
			return;
		}
		else
		{
			// One last bounce at the minimum height, to reset the animation
			player->tumbleHeight = 10;
			player->pflags |= PF_TUMBLELASTBOUNCE;
			player->mo->rollangle = 0;	// p_user.c will stop rotating the player automatically
			player->mo->pitch = player->mo->roll = 0; // Prevent Kodachrome Void infinite
		}
	}

	// A bit of damage hitlag.
	// This gives a window for DI!!
	K_AddHitLag(player->mo, 3, true);

	if (P_IsDisplayPlayer(player) && player->tumbleHeight >= 40)
		P_StartQuake((player->tumbleHeight*3/2)<<FRACBITS, 6);	// funny earthquakes for the FEEL

	S_StartSound(player->mo, (player->tumbleHeight < 40) ? sfx_s3k5d : sfx_s3k5f);	// s3k5d is bounce < 50, s3k5f otherwise!

	player->mo->momx = player->mo->momx / 2;
	player->mo->momy = player->mo->momy / 2;

	// and then modulate momz like that...
	player->mo->momz = K_TumbleZ(player->mo, player->tumbleHeight * FRACUNIT);
}

// Play a falling sound when you start falling while tumbling and you're nowhere near done bouncing
static void K_HandleTumbleSound(player_t *player)
{
	fixed_t momz;
	momz = player->mo->momz * P_MobjFlip(player->mo);

	if (!K_LastTumbleBounceCondition(player) &&
			!(player->pflags & PF_TUMBLESOUND) && momz < -10*player->mo->scale)
	{
		S_StartSound(player->mo, sfx_s3k51);
		player->pflags |= PF_TUMBLESOUND;
	}
}

void K_TumbleInterrupt(player_t *player)
{
	// If player was tumbling, set variables so that they don't tumble like crazy after they're done respawning
	if (player->tumbleBounces > 0)
	{
		player->tumbleBounces = 0; // MAXBOUNCES-1;
		player->pflags &= ~PF_TUMBLELASTBOUNCE;
		//players->tumbleHeight = 20;

		players->mo->rollangle = 0;
		player->spinouttype = KSPIN_WIPEOUT;
		player->spinouttimer = player->wipeoutslow = TICRATE+2;
	}
}

void K_ApplyTripWire(player_t *player, tripwirestate_t state)
{
	if (state == TRIPSTATE_PASSED)
		S_StartSound(player->mo, sfx_ssa015);
	else if (state == TRIPSTATE_BLOCKED)
		S_StartSound(player->mo, sfx_kc40);

	player->tripwireState = state;
	K_AddHitLag(player->mo, 10, false);

	if (state == TRIPSTATE_PASSED && player->spinouttimer &&
			player->speed > 2 * K_GetKartSpeed(player, false, true))
	{
		K_TumblePlayer(player, NULL, NULL);
	}
}

INT32 K_ExplodePlayer(player_t *player, mobj_t *inflictor, mobj_t *source) // A bit of a hack, we just throw the player up higher here and extend their spinout timer
{
	INT32 ringburst = 10;
	fixed_t spbMultiplier = FRACUNIT;

	(void)source;

	K_DirectorFollowAttack(player, inflictor, source);

	if (inflictor != NULL && P_MobjWasRemoved(inflictor) == false)
	{
		if (inflictor->type == MT_SPBEXPLOSION && inflictor->movefactor)
		{
			spbMultiplier = inflictor->movefactor;

			if (spbMultiplier <= 0)
			{
				// Convert into stumble.
				K_StumblePlayer(player);
				return 0;
			}
			else if (spbMultiplier < FRACUNIT)
			{
				spbMultiplier = FRACUNIT;
			}
		}
	}

	player->mo->momz = 18 * mapobjectscale * P_MobjFlip(player->mo); // please stop forgetting mobjflip checks!!!!
	player->mo->momx = player->mo->momy = 0;

	player->spinouttype = KSPIN_EXPLOSION;
	player->spinouttimer = (3*TICRATE/2)+2;

	if (spbMultiplier != FRACUNIT)
	{
		player->mo->momz = FixedMul(player->mo->momz, spbMultiplier);
		player->spinouttimer = FixedMul(player->spinouttimer, spbMultiplier + ((spbMultiplier - FRACUNIT) / 2));

		ringburst = FixedMul(ringburst * FRACUNIT, spbMultiplier) / FRACUNIT;
		if (ringburst > 20)
		{
			ringburst = 20;
		}
	}

	if (player->mo->eflags & MFE_UNDERWATER)
		player->mo->momz = (117 * player->mo->momz) / 200;

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

	if (P_IsDisplayPlayer(player))
		P_StartQuake(64<<FRACBITS, 5);

	return ringburst;
}

// This kind of wipeout happens with no rings -- doesn't remove a bumper, has no invulnerability, and is much shorter.
void K_DebtStingPlayer(player_t *player, mobj_t *source)
{
	INT32 length = TICRATE;

	if (source->player)
	{
		length += (4 * (source->player->kartweight - player->kartweight));
	}

	player->spinouttype = KSPIN_STUNG;
	player->spinouttimer = length;
	player->wipeoutslow = min(length-1, wipeoutslowtime+1);

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
}

void K_HandleBumperChanges(player_t *player, UINT8 prevBumpers)
{
	if (!(gametyperules & GTR_BUMPERS))
	{
		// Bumpers aren't being used
		return;
	}

	// TODO: replace all console text print-outs with a real visual

	if (player->bumpers > 0 && prevBumpers == 0)
	{
		K_DoInvincibility(player, 8 * TICRATE);

		if (netgame)
		{
			CONS_Printf(M_GetText("%s is back in the game!\n"), player_names[player-players]);
		}
	}
	else if (player->bumpers == 0 && prevBumpers > 0)
	{
		mobj_t *karmahitbox = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_KARMAHITBOX);
		P_SetTarget(&karmahitbox->target, player->mo);

		karmahitbox->destscale = player->mo->destscale;
		P_SetScale(karmahitbox, player->mo->scale);

		player->karmadelay = comebacktime;

		if (bossinfo.boss)
		{
			P_DoTimeOver(player);
		}
		else if (netgame)
		{
			CONS_Printf(M_GetText("%s lost all of their bumpers!\n"), player_names[player-players]);
		}
	}

	K_CalculateBattleWanted();
	K_CheckBumpers();
}

void K_DestroyBumpers(player_t *player, UINT8 amount)
{
	UINT8 oldBumpers = player->bumpers;

	if (!(gametyperules & GTR_BUMPERS))
	{
		return;
	}

	amount = min(amount, player->bumpers);

	if (amount == 0)
	{
		return;
	}

	player->bumpers -= amount;
	K_HandleBumperChanges(player, oldBumpers);
}

void K_TakeBumpersFromPlayer(player_t *player, player_t *victim, UINT8 amount)
{
	UINT8 oldPlayerBumpers = player->bumpers;
	UINT8 oldVictimBumpers = victim->bumpers;

	UINT8 tookBumpers = 0;

	if (!(gametyperules & GTR_BUMPERS))
	{
		return;
	}

	amount = min(amount, victim->bumpers);

	if (amount == 0)
	{
		return;
	}

	while ((tookBumpers < amount) && (victim->bumpers > 0))
	{
		UINT8 newbumper = player->bumpers;

		angle_t newangle, diff;
		fixed_t newx, newy;

		mobj_t *newmo;

		if (newbumper <= 1)
		{
			diff = 0;
		}
		else
		{
			diff = FixedAngle(360*FRACUNIT/newbumper);
		}

		newangle = player->mo->angle;
		newx = player->mo->x + P_ReturnThrustX(player->mo, newangle + ANGLE_180, 64*FRACUNIT);
		newy = player->mo->y + P_ReturnThrustY(player->mo, newangle + ANGLE_180, 64*FRACUNIT);

		newmo = P_SpawnMobj(newx, newy, player->mo->z, MT_BATTLEBUMPER);
		newmo->threshold = newbumper;

		P_SetTarget(&newmo->tracer, victim->mo);
		P_SetTarget(&newmo->target, player->mo);

		P_InitAngle(newmo, (diff * (newbumper-1)));
		newmo->color = victim->skincolor;

		if (newbumper+1 < 2)
		{
			P_SetMobjState(newmo, S_BATTLEBUMPER3);
		}
		else if (newbumper+1 < 3)
		{
			P_SetMobjState(newmo, S_BATTLEBUMPER2);
		}
		else
		{
			P_SetMobjState(newmo, S_BATTLEBUMPER1);
		}

		player->bumpers++;
		victim->bumpers--;
		tookBumpers++;
	}

	if (tookBumpers == 0)
	{
		// No change occured.
		return;
	}

	// Play steal sound
	S_StartSound(player->mo, sfx_3db06);

	K_HandleBumperChanges(player, oldPlayerBumpers);
	K_HandleBumperChanges(victim, oldVictimBumpers);
}

#define MINEQUAKEDIST 4096

// Does the proximity screen flash and quake for explosions
void K_MineFlashScreen(mobj_t *source)
{
	INT32 pnum;
	player_t *p;

	// check for potential display players near the source so we can have a sick earthquake / flashpal.
	for (pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		p = &players[pnum];

		if (!playeringame[pnum] || !P_IsDisplayPlayer(p))
			continue;

		if (R_PointToDist2(p->mo->x, p->mo->y, source->x, source->y) < mapobjectscale*MINEQUAKEDIST)
		{
			P_StartQuake(55<<FRACBITS, 12);
			if (!bombflashtimer && P_CheckSight(p->mo, source))
			{
				bombflashtimer = TICRATE*2;
				P_FlashPal(p, PAL_WHITE, 1);
			}
			break;	// we can break right now because quakes are global to all split players somehow.
		}
	}
}

// Spawns the purely visual explosion
void K_SpawnMineExplosion(mobj_t *source, UINT8 color)
{
	INT32 i, radius, height;
	mobj_t *smoldering = P_SpawnMobj(source->x, source->y, source->z, MT_SMOLDERING);
	mobj_t *dust;
	mobj_t *truc;
	INT32 speed, speed2;

	K_MineFlashScreen(source);

	K_MatchGenericExtraFlags(smoldering, source);
	smoldering->tics = TICRATE*3;
	radius = source->radius>>FRACBITS;
	height = source->height>>FRACBITS;

	S_StartSound(smoldering, sfx_s3k4e);

	if (!color)
		color = SKINCOLOR_KETCHUP;

	for (i = 0; i < 32; i++)
	{
		dust = P_SpawnMobj(source->x, source->y, source->z, MT_SMOKE);
		P_SetMobjState(dust, S_OPAQUESMOKE1);
		P_InitAngle(dust, (ANGLE_180/16) * i);
		P_SetScale(dust, source->scale);
		dust->destscale = source->scale*10;
		dust->scalespeed = source->scale/12;
		P_InstaThrust(dust, dust->angle, FixedMul(20*FRACUNIT, source->scale));

		truc = P_SpawnMobj(source->x + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->y + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->z + P_RandomRange(PR_EXPLOSION, 0, height)*FRACUNIT, MT_BOOMEXPLODE);
		K_MatchGenericExtraFlags(truc, source);
		P_SetScale(truc, source->scale);
		truc->destscale = source->scale*6;
		truc->scalespeed = source->scale/12;
		speed = FixedMul(10*FRACUNIT, source->scale)>>FRACBITS;
		truc->momx = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		truc->momy = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		speed = FixedMul(20*FRACUNIT, source->scale)>>FRACBITS;
		truc->momz = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT*P_MobjFlip(truc);
		if (truc->eflags & MFE_UNDERWATER)
			truc->momz = (117 * truc->momz) / 200;
		truc->color = color;
	}

	for (i = 0; i < 16; i++)
	{
		dust = P_SpawnMobj(source->x + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->y + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->z + P_RandomRange(PR_EXPLOSION, 0, height)*FRACUNIT, MT_SMOKE);
		P_SetMobjState(dust, S_OPAQUESMOKE1);
		P_SetScale(dust, source->scale);
		dust->destscale = source->scale*10;
		dust->scalespeed = source->scale/12;
		dust->tics = 30;
		dust->momz = P_RandomRange(PR_EXPLOSION, FixedMul(3*FRACUNIT, source->scale)>>FRACBITS, FixedMul(7*FRACUNIT, source->scale)>>FRACBITS)*FRACUNIT;

		truc = P_SpawnMobj(source->x + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->y + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->z + P_RandomRange(PR_EXPLOSION, 0, height)*FRACUNIT, MT_BOOMPARTICLE);
		K_MatchGenericExtraFlags(truc, source);
		P_SetScale(truc, source->scale);
		truc->destscale = source->scale*5;
		truc->scalespeed = source->scale/12;
		speed = FixedMul(20*FRACUNIT, source->scale)>>FRACBITS;
		truc->momx = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		truc->momy = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		speed = FixedMul(15*FRACUNIT, source->scale)>>FRACBITS;
		speed2 = FixedMul(45*FRACUNIT, source->scale)>>FRACBITS;
		truc->momz = P_RandomRange(PR_EXPLOSION, speed, speed2)*FRACUNIT*P_MobjFlip(truc);
		if (P_RandomChance(PR_EXPLOSION, FRACUNIT/2))
			truc->momz = -truc->momz;
		if (truc->eflags & MFE_UNDERWATER)
			truc->momz = (117 * truc->momz) / 200;
		truc->tics = TICRATE*2;
		truc->color = color;
	}
}

#undef MINEQUAKEDIST

fixed_t K_ItemScaleForPlayer(player_t *player)
{
	switch (player->itemscale)
	{
		case ITEMSCALE_GROW:
			return FixedMul(GROW_SCALE, mapobjectscale);

		case ITEMSCALE_SHRINK:
			return FixedMul(SHRINK_SCALE, mapobjectscale);

		default:
			return mapobjectscale;
	}
}

static mobj_t *K_SpawnKartMissile(mobj_t *source, mobjtype_t type, angle_t an, INT32 flags2, fixed_t speed, SINT8 dir)
{
	mobj_t *th;
	fixed_t x, y, z;
	fixed_t topspeed = K_GetKartSpeed(source->player, false, false);
	fixed_t finalspeed = speed;
	fixed_t finalscale = mapobjectscale;
	mobj_t *throwmo;

	if (source->player != NULL)
	{
		if (source->player->itemscale == ITEMSCALE_SHRINK)
		{
			// Nerf the base item speed a bit.
			speed = finalspeed = FixedMul(speed, SHRINK_PHYSICS_SCALE);
		}

		if (source->player->speed > topspeed)
		{
			angle_t delta = AngleDelta(source->angle, an);

			finalspeed = max(speed, FixedMul(
				speed,
				FixedMul(
					FixedDiv(source->player->speed, topspeed), // Multiply speed to be proportional to your own, boosted maxspeed.
					FixedDiv(AngleFixed(ANGLE_180 - delta), 180 * FRACUNIT) // multiply speed based on angle diff... i.e: don't do this for firing backward :V
				)
			));
		}

		finalscale = K_ItemScaleForPlayer(source->player);
	}

	if (type == MT_BUBBLESHIELDTRAP)
	{
		finalscale = source->scale;
	}

	if (dir == -1 && (type == MT_ORBINAUT || type == MT_BALLHOG))
	{
		// Backwards nerfs
		finalspeed /= 8;
	}

	x = source->x + source->momx + FixedMul(finalspeed, FINECOSINE(an>>ANGLETOFINESHIFT));
	y = source->y + source->momy + FixedMul(finalspeed, FINESINE(an>>ANGLETOFINESHIFT));
	z = source->z; // spawn on the ground please

	th = P_SpawnMobj(x, y, z, type); // this will never return null because collision isn't processed here

	K_FlipFromObject(th, source);

	th->flags2 |= flags2;
	th->threshold = 10;

	if (th->info->seesound)
		S_StartSound(source, th->info->seesound);

	P_SetTarget(&th->target, source);

	P_SetScale(th, finalscale);
	th->destscale = finalscale;

	if (P_IsObjectOnGround(source))
	{
		// floorz and ceilingz aren't properly set to account for FOFs and Polyobjects on spawn
		// This should set it for FOFs
		P_SetOrigin(th, th->x, th->y, th->z); // however, THIS can fuck up your day. just absolutely ruin you.
		if (P_MobjWasRemoved(th))
			return NULL;

		// spawn on the ground if the player is on the ground
		if (P_MobjFlip(source) < 0)
		{
			th->z = th->ceilingz - th->height;
			th->eflags |= MFE_VERTICALFLIP;
		}
		else
			th->z = th->floorz;
	}

	P_InitAngle(th, an);

	th->momx = FixedMul(finalspeed, FINECOSINE(an>>ANGLETOFINESHIFT));
	th->momy = FixedMul(finalspeed, FINESINE(an>>ANGLETOFINESHIFT));
	th->momz = source->momz;

	if (source->player != NULL)
	{
		th->cusval = source->player->itemscale;
	}

	switch (type)
	{
		case MT_ORBINAUT:
			Obj_OrbinautThrown(th, finalspeed, dir);
			break;
		case MT_JAWZ:
			Obj_JawzThrown(th, finalspeed, dir);
			break;
		case MT_SPB:
			th->movefactor = finalspeed;
			break;
		case MT_BUBBLESHIELDTRAP:
			P_SetScale(th, ((5*th->destscale)>>2)*4);
			th->destscale = (5*th->destscale)>>2;
			S_StartSound(th, sfx_s3kbfl);
			S_StartSound(th, sfx_cdfm35);
			break;
		case MT_BALLHOG:
			// Contra spread shot scale up
			th->destscale = th->destscale << 1;
			th->scalespeed = abs(th->destscale - th->scale) / (2*TICRATE);
			break;
		default:
			break;
	}

	if (type != MT_BUBBLESHIELDTRAP)
	{
		x = x + P_ReturnThrustX(source, an, source->radius + th->radius);
		y = y + P_ReturnThrustY(source, an, source->radius + th->radius);
		throwmo = P_SpawnMobj(x, y, z, MT_FIREDITEM);
		throwmo->movecount = 1;
		throwmo->movedir = source->angle - an;
		P_SetTarget(&throwmo->target, source);
	}

	return th;
}

UINT16 K_DriftSparkColor(player_t *player, INT32 charge)
{
	const INT32 dsone = K_GetKartDriftSparkValueForStage(player, 1);
	const INT32 dstwo = K_GetKartDriftSparkValueForStage(player, 2);
	const INT32 dsthree = K_GetKartDriftSparkValueForStage(player, 3);
	const INT32 dsfour = K_GetKartDriftSparkValueForStage(player, 4);

	UINT16 color = SKINCOLOR_NONE;

	if (charge < 0)
	{
		// Stage 0: Grey
		color = SKINCOLOR_SILVER;
	}
	else if (charge >= dsfour)
	{
		// Stage 4: Rainbow
		if (charge <= dsfour+(32*3))
		{
			// transition
			color = SKINCOLOR_SILVER;
		}
		else
		{
			color = K_RainbowColor(leveltime);
		}
	}
	else if (charge >= dsthree)
	{
		// Stage 3: Blue
		if (charge <= dsthree+(16*3))
		{
			// transition 1
			color = SKINCOLOR_TAFFY;
		}
		else if (charge <= dsthree+(32*3))
		{
			// transition 2
			color = SKINCOLOR_NOVA;
		}
		else
		{
			color = SKINCOLOR_SAPPHIRE;
		}
	}
	else if (charge >= dstwo)
	{
		// Stage 2: Red
		if (charge <= dstwo+(32*3))
		{
			// transition
			color = SKINCOLOR_TANGERINE;
		}
		else
		{
			color = SKINCOLOR_KETCHUP;
		}
	}
	else if (charge >= dsone)
	{
		// Stage 1: Yellow
		if (charge <= dsone+(32*3))
		{
			// transition
			color = SKINCOLOR_TAN;
		}
		else
		{
			color = SKINCOLOR_GOLD;
		}
	}

	return color;
}

static void K_SpawnDriftElectricity(player_t *player)
{
	UINT8 i;
	UINT16 color = K_DriftSparkColor(player, player->driftcharge);
	mobj_t *mo = player->mo;
	fixed_t vr = FixedDiv(mo->radius/3, mo->scale); // P_SpawnMobjFromMobj will rescale
	fixed_t horizontalradius = FixedDiv(5*mo->radius/3, mo->scale);
	angle_t verticalangle = K_MomentumAngle(mo) + ANGLE_180; // points away from the momentum angle

	for (i = 0; i < 2; i++)
	{
		// i == 0 is right, i == 1 is left
		mobj_t *spark;
		angle_t horizonatalangle = verticalangle + (i ? ANGLE_90 : ANGLE_270);
		angle_t sparkangle = verticalangle + ANGLE_180;
		fixed_t verticalradius = vr; // local version of the above so we can modify it
		fixed_t scalefactor = 0; // positive values enlarge sparks, negative values shrink them
		fixed_t x, y;

		if (player->drift == 0)
			; // idk what you're doing spawning drift sparks when you're not drifting but you do you
		else
		{
			scalefactor = -(2*i - 1) * min(max(player->steering, -1), 1) * FRACUNIT;
			if ((player->drift > 0) == !(i)) // inwards spark should be closer to the player
				verticalradius = 0;
		}

		x = P_ReturnThrustX(mo, verticalangle, verticalradius)
			+ P_ReturnThrustX(mo, horizonatalangle, horizontalradius);
		y = P_ReturnThrustY(mo, verticalangle, verticalradius)
			+ P_ReturnThrustY(mo, horizonatalangle, horizontalradius);
		spark = P_SpawnMobjFromMobj(mo, x, y, 0, MT_DRIFTELECTRICITY);
		P_InitAngle(spark, sparkangle);
		spark->momx = mo->momx;
		spark->momy = mo->momy;
		spark->momz = mo->momz;
		spark->color = color;
		K_GenericExtraFlagsNoZAdjust(spark, mo);

		spark->spritexscale += scalefactor/3;
		spark->spriteyscale += scalefactor/8;
	}
}

void K_SpawnDriftElectricSparks(player_t *player, int color, boolean shockwave)
{
	SINT8 hdir, vdir, i;
	int shockscale = shockwave ? 2 : 1;

	mobj_t *mo = player->mo;
	angle_t momangle = K_MomentumAngle(mo) + ANGLE_180;
	fixed_t radius = 2 * FixedDiv(mo->radius, mo->scale); // P_SpawnMobjFromMobj will rescale
	fixed_t x = P_ReturnThrustX(mo, momangle, radius);
	fixed_t y = P_ReturnThrustY(mo, momangle, radius);
	fixed_t z = FixedDiv(mo->height, 2 * mo->scale); // P_SpawnMobjFromMobj will rescale

	fixed_t sparkspeed = mobjinfo[MT_DRIFTELECTRICSPARK].speed;
	fixed_t sparkradius = 2 * shockscale * mobjinfo[MT_DRIFTELECTRICSPARK].radius;

	for (hdir = -1; hdir <= 1; hdir += 2)
	{
		for (vdir = -1; vdir <= 1; vdir += 2)
		{
			fixed_t hspeed = FixedMul(hdir * sparkspeed, mo->scale); // P_InstaThrust treats speed as absolute
			fixed_t vspeed = vdir * sparkspeed; // P_SetObjectMomZ scales speed with object scale
			angle_t sparkangle = mo->angle + ANGLE_45;

			for (i = 0; i < 4; i++)
			{
				fixed_t xoff = P_ReturnThrustX(mo, sparkangle, sparkradius);
				fixed_t yoff = P_ReturnThrustY(mo, sparkangle, sparkradius);
				mobj_t *spark = P_SpawnMobjFromMobj(mo, x + xoff, y + yoff, z, MT_DRIFTELECTRICSPARK);

				P_InitAngle(spark, sparkangle);
				spark->color = color;
				P_InstaThrust(spark, mo->angle + ANGLE_90, hspeed);
				P_SetObjectMomZ(spark, vspeed, false);
				spark->momx += mo->momx; // copy player speed
				spark->momy += mo->momy;
				spark->momz += P_GetMobjZMovement(mo);
				spark->destscale = shockscale * spark->scale;
				P_SetScale(spark, shockscale * spark->scale);

				if (shockwave)
					spark->frame |= FF_ADD;

				sparkangle += ANGLE_90;
			}
		}
	}
	S_StartSound(mo, sfx_s3k45);
}

static void K_SpawnDriftSparks(player_t *player)
{
	const INT32 dsone = K_GetKartDriftSparkValueForStage(player, 1);
	const INT32 dstwo = K_GetKartDriftSparkValueForStage(player, 2);
	const INT32 dsthree = K_GetKartDriftSparkValueForStage(player, 3);
	const INT32 dsfour = K_GetKartDriftSparkValueForStage(player, 4);

	fixed_t newx;
	fixed_t newy;
	mobj_t *spark;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (leveltime % 2 == 1)
		return;

	if (!player->drift
		|| (player->driftcharge < dsone && !(player->driftcharge < 0)))
		return;

	travelangle = player->mo->angle-(ANGLE_45/5)*player->drift;

	for (i = 0; i < 2; i++)
	{
		SINT8 size = 1;
		UINT8 trail = 0;

		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(32*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(32*FRACUNIT, player->mo->scale));
		spark = P_SpawnMobj(newx, newy, player->mo->z, MT_DRIFTSPARK);

		P_SetTarget(&spark->target, player->mo);
		P_InitAngle(spark, travelangle-(ANGLE_45/5)*player->drift);
		spark->destscale = player->mo->scale;
		P_SetScale(spark, player->mo->scale);

		spark->momx = player->mo->momx/2;
		spark->momy = player->mo->momy/2;
		spark->momz = P_GetMobjZMovement(player->mo)/2;

		spark->color = K_DriftSparkColor(player, player->driftcharge);

		if (player->driftcharge < 0)
		{
			// Stage 0: Yellow
			size = 0;
		}
		else if (player->driftcharge >= dsfour)
		{
			// Stage 4: Rainbow
			size = 2;
			trail = 2;

			if (player->driftcharge <= (dsfour)+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*3/2));
				S_StartSound(player->mo, sfx_cock);
			}
			else
			{
				spark->colorized = true;
			}
		}
		else if (player->driftcharge >= dsthree)
		{
			// Stage 3: Purple
			size = 2;
			trail = 1;

			if (player->driftcharge <= dsthree+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*3/2));
			}
		}
		else if (player->driftcharge >= dstwo)
		{
			// Stage 2: Blue
			size = 2;
			trail = 1;

			if (player->driftcharge <= dstwo+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*3/2));
			}
		}
		else
		{
			// Stage 1: Red
			size = 1;

			if (player->driftcharge <= dsone+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*2));
			}
		}

		if ((player->drift > 0 && player->steering > 0) // Inward drifts
			|| (player->drift < 0 && player->steering < 0))
		{
			if ((player->drift < 0 && (i & 1))
				|| (player->drift > 0 && !(i & 1)))
			{
				size++;
			}
			else if ((player->drift < 0 && !(i & 1))
				|| (player->drift > 0 && (i & 1)))
			{
				size--;
			}
		}
		else if ((player->drift > 0 && player->steering < 0) // Outward drifts
			|| (player->drift < 0 && player->steering > 0))
		{
			if ((player->drift < 0 && (i & 1))
				|| (player->drift > 0 && !(i & 1)))
			{
				size--;
			}
			else if ((player->drift < 0 && !(i & 1))
				|| (player->drift > 0 && (i & 1)))
			{
				size++;
			}
		}

		if (size == 2)
			P_SetMobjState(spark, S_DRIFTSPARK_A1);
		else if (size < 1)
			P_SetMobjState(spark, S_DRIFTSPARK_C1);
		else if (size > 2)
			P_SetMobjState(spark, S_DRIFTSPARK_D1);

		if (trail > 0)
			spark->tics += trail;

		K_MatchGenericExtraFlags(spark, player->mo);
	}

	if (player->driftcharge >= dsthree)
	{
		K_SpawnDriftElectricity(player);
	}
}

static void K_SpawnAIZDust(player_t *player)
{
	fixed_t newx;
	fixed_t newy;
	mobj_t *spark;
	angle_t travelangle;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (leveltime % 2 == 1)
		return;

	if (!P_IsObjectOnGround(player->mo))
		return;

	if (player->speed <= K_GetKartSpeed(player, false, true))
		return;

	travelangle = K_MomentumAngle(player->mo);
	//S_StartSound(player->mo, sfx_s3k47);

	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle - (player->aizdriftstrat*ANGLE_45), FixedMul(24*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle - (player->aizdriftstrat*ANGLE_45), FixedMul(24*FRACUNIT, player->mo->scale));
		spark = P_SpawnMobj(newx, newy, player->mo->z, MT_AIZDRIFTSTRAT);

		P_InitAngle(spark, travelangle+(player->aizdriftstrat*ANGLE_90));
		P_SetScale(spark, (spark->destscale = (3*player->mo->scale)>>2));

		spark->momx = (6*player->mo->momx)/5;
		spark->momy = (6*player->mo->momy)/5;
		spark->momz = P_GetMobjZMovement(player->mo);

		K_MatchGenericExtraFlags(spark, player->mo);
	}
}

void K_SpawnBoostTrail(player_t *player)
{
	fixed_t newx, newy, newz;
	fixed_t ground;
	mobj_t *flame;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (!P_IsObjectOnGround(player->mo)
		|| player->hyudorotimer != 0
		|| ((gametyperules & GTR_BUMPERS) && player->bumpers <= 0 && player->karmadelay))
		return;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		ground = player->mo->ceilingz;
	else
		ground = player->mo->floorz;

	if (player->drift != 0)
		travelangle = player->mo->angle;
	else
		travelangle = K_MomentumAngle(player->mo);

	for (i = 0; i < 2; i++)
	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(24*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(24*FRACUNIT, player->mo->scale));
		newz = P_GetZAt(player->mo->standingslope, newx, newy, ground);

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			newz -= FixedMul(mobjinfo[MT_SNEAKERTRAIL].height, player->mo->scale);
		}

		flame = P_SpawnMobj(newx, newy, ground, MT_SNEAKERTRAIL);

		P_SetTarget(&flame->target, player->mo);
		P_InitAngle(flame, travelangle);
		flame->fuse = TICRATE*2;
		flame->destscale = player->mo->scale;
		P_SetScale(flame, player->mo->scale);
		// not K_MatchGenericExtraFlags so that a stolen sneaker can be seen
		K_FlipFromObject(flame, player->mo);

		flame->momx = 8;
		P_XYMovement(flame);
		if (P_MobjWasRemoved(flame))
			continue;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (flame->z + flame->height < flame->ceilingz)
				P_RemoveMobj(flame);
		}
		else if (flame->z > flame->floorz)
			P_RemoveMobj(flame);
	}
}

void K_SpawnSparkleTrail(mobj_t *mo)
{
	const INT32 rad = (mo->radius*3)/FRACUNIT;
	mobj_t *sparkle;
	UINT8 invanimnum; // Current sparkle animation number
	INT32 invtime;// Invincibility time left, in seconds
	UINT8 index = 0;
	fixed_t newx, newy, newz;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (leveltime & 2)
		index = 1;

	invtime = mo->player->invincibilitytimer/TICRATE+1;

	//CONS_Printf("%d\n", index);

	newx = mo->x + (P_RandomRange(PR_DECORATION, -rad, rad)*FRACUNIT);
	newy = mo->y + (P_RandomRange(PR_DECORATION, -rad, rad)*FRACUNIT);
	newz = mo->z + (P_RandomRange(PR_DECORATION, 0, mo->height>>FRACBITS)*FRACUNIT);

	sparkle = P_SpawnMobj(newx, newy, newz, MT_SPARKLETRAIL);

	P_InitAngle(sparkle, R_PointToAngle2(mo->x, mo->y, sparkle->x, sparkle->y));

	sparkle->movefactor = R_PointToDist2(mo->x, mo->y, sparkle->x, sparkle->y);	// Save the distance we spawned away from the player.
	//CONS_Printf("movefactor: %d\n", sparkle->movefactor/FRACUNIT);

	sparkle->extravalue1 = (sparkle->z - mo->z);			// Keep track of our Z position relative to the player's, I suppose.
	sparkle->extravalue2 = P_RandomRange(PR_DECORATION, 0, 1) ? 1 : -1;	// Rotation direction?
	sparkle->cvmem = P_RandomRange(PR_DECORATION, -25, 25)*mo->scale;		// Vertical "angle"

	K_FlipFromObject(sparkle, mo);
	P_SetTarget(&sparkle->target, mo);

	sparkle->destscale = mo->destscale;
	P_SetScale(sparkle, mo->scale);

	invanimnum = (invtime >= 11) ? 11 : invtime;
	//CONS_Printf("%d\n", invanimnum);

	P_SetMobjState(sparkle, K_SparkleTrailStartStates[invanimnum][index]);

	if (mo->player->invincibilitytimer > itemtime+(2*TICRATE))
	{
		sparkle->color = mo->color;
		sparkle->colorized = true;
	}
}

void K_SpawnWipeoutTrail(mobj_t *mo)
{
	mobj_t *dust;
	angle_t aoff;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (mo->player)
		aoff = (mo->player->drawangle + ANGLE_180);
	else
		aoff = (mo->angle + ANGLE_180);

	if ((leveltime / 2) & 1)
		aoff -= ANGLE_45;
	else
		aoff += ANGLE_45;

	dust = P_SpawnMobj(mo->x + FixedMul(24*mo->scale, FINECOSINE(aoff>>ANGLETOFINESHIFT)) + (P_RandomRange(PR_DECORATION,-8,8) << FRACBITS),
		mo->y + FixedMul(24*mo->scale, FINESINE(aoff>>ANGLETOFINESHIFT)) + (P_RandomRange(PR_DECORATION,-8,8) << FRACBITS),
		mo->z, MT_WIPEOUTTRAIL);

	P_SetTarget(&dust->target, mo);
	P_InitAngle(dust, K_MomentumAngle(mo));
	dust->destscale = mo->scale;
	P_SetScale(dust, mo->scale);
	K_FlipFromObject(dust, mo);
}

void K_SpawnDraftDust(mobj_t *mo)
{
	UINT8 i;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	for (i = 0; i < 2; i++)
	{
		angle_t ang, aoff;
		SINT8 sign = 1;
		UINT8 foff = 0;
		mobj_t *dust;
		boolean drifting = false;

		if (mo->player)
		{
			UINT8 leniency = (3*TICRATE)/4 + ((mo->player->kartweight-1) * (TICRATE/4));

			if (gametype == GT_BATTLE)
				leniency *= 4;

			ang = mo->player->drawangle;

			if (mo->player->drift != 0)
			{
				drifting = true;
				ang += (mo->player->drift * ((ANGLE_270 + ANGLE_22h) / 5)); // -112.5 doesn't work. I fucking HATE SRB2 angles
				if (mo->player->drift < 0)
					sign = 1;
				else
					sign = -1;
			}

			foff = 5 - ((mo->player->draftleeway * 5) / leniency);

			// this shouldn't happen
			if (foff > 4)
				foff = 4;
		}
		else
			ang = mo->angle;

		if (!drifting)
		{
			if (i & 1)
				sign = -1;
			else
				sign = 1;
		}

		aoff = (ang + ANGLE_180) + (ANGLE_45 * sign);

		dust = P_SpawnMobj(mo->x + FixedMul(24*mo->scale, FINECOSINE(aoff>>ANGLETOFINESHIFT)),
			mo->y + FixedMul(24*mo->scale, FINESINE(aoff>>ANGLETOFINESHIFT)),
			mo->z, MT_DRAFTDUST);

		P_SetMobjState(dust, S_DRAFTDUST1 + foff);

		P_SetTarget(&dust->target, mo);
		P_InitAngle(dust, ang - (ANGLE_90 * sign)); // point completely perpendicular from the player
		dust->destscale = mo->scale;
		P_SetScale(dust, mo->scale);
		K_FlipFromObject(dust, mo);

		if (leveltime & 1)
			dust->tics++; // "randomize" animation

		dust->momx = (4*mo->momx)/5;
		dust->momy = (4*mo->momy)/5;
		dust->momz = (4*P_GetMobjZMovement(mo))/5;

		P_Thrust(dust, dust->angle, 4*mo->scale);

		if (drifting) // only 1 trail while drifting
			break;
	}
}

//	K_DriftDustHandling
//	Parameters:
//		spawner: The map object that is spawning the drift dust
//	Description: Spawns the drift dust for objects, players use rmomx/y, other objects use regular momx/y.
//		Also plays the drift sound.
//		Other objects should be angled towards where they're trying to go so they don't randomly spawn dust
//		Do note that most of the function won't run in odd intervals of frames
void K_DriftDustHandling(mobj_t *spawner)
{
	angle_t anglediff;
	const INT16 spawnrange = spawner->radius >> FRACBITS;

	if (!P_IsObjectOnGround(spawner) || leveltime % 2 != 0)
		return;

	if (spawner->player)
	{
		if (spawner->player->pflags & PF_FAULT)
		{
			anglediff = abs((signed)(spawner->angle - spawner->player->drawangle));
			if (leveltime % 6 == 0)
				S_StartSound(spawner, sfx_screec); // repeated here because it doesn't always happen to be within the range when this is the case
		}
		else
		{
			angle_t playerangle = spawner->angle;

			if (spawner->player->speed < 5*spawner->scale)
				return;

			if (K_GetForwardMove(spawner->player) < 0)
				playerangle += ANGLE_180;

			anglediff = abs((signed)(playerangle - R_PointToAngle2(0, 0, spawner->player->rmomx, spawner->player->rmomy)));
		}
	}
	else
	{
		if (P_AproxDistance(spawner->momx, spawner->momy) < 5*spawner->scale)
			return;

		anglediff = abs((signed)(spawner->angle - K_MomentumAngle(spawner)));
	}

	if (anglediff > ANGLE_180)
		anglediff = InvAngle(anglediff);

	if (anglediff > ANG10*4) // Trying to turn further than 40 degrees
	{
		fixed_t spawnx = P_RandomRange(PR_DECORATION, -spawnrange, spawnrange) << FRACBITS;
		fixed_t spawny = P_RandomRange(PR_DECORATION, -spawnrange, spawnrange) << FRACBITS;
		INT32 speedrange = 2;
		mobj_t *dust = P_SpawnMobj(spawner->x + spawnx, spawner->y + spawny, spawner->z, MT_DRIFTDUST);
		dust->momx = FixedMul(spawner->momx + (P_RandomRange(PR_DECORATION, -speedrange, speedrange) * spawner->scale), 3*FRACUNIT/4);
		dust->momy = FixedMul(spawner->momy + (P_RandomRange(PR_DECORATION, -speedrange, speedrange) * spawner->scale), 3*FRACUNIT/4);
		dust->momz = P_MobjFlip(spawner) * (P_RandomRange(PR_DECORATION, 1, 4) * (spawner->scale));
		P_SetScale(dust, spawner->scale/2);
		dust->destscale = spawner->scale * 3;
		dust->scalespeed = spawner->scale/12;

		if (leveltime % 6 == 0)
			S_StartSound(spawner, sfx_screec);

		K_MatchGenericExtraFlags(dust, spawner);

		// Sparkle-y warning for when you're about to change drift sparks!
		if (spawner->player && spawner->player->drift)
		{
			INT32 driftval = K_GetKartDriftSparkValue(spawner->player);
			INT32 warntime = driftval/3;
			INT32 dc = spawner->player->driftcharge;
			UINT8 c = SKINCOLOR_NONE;
			boolean rainbow = false;

			if (dc >= 0)
			{
				dc += warntime;
			}

			c = K_DriftSparkColor(spawner->player, dc);

			if (dc > (4*driftval)+(32*3))
			{
				rainbow = true;
			}

			if (c != SKINCOLOR_NONE)
			{
				P_SetMobjState(dust, S_DRIFTWARNSPARK1);
				dust->color = c;
				dust->colorized = rainbow;
			}
		}
	}
}

void K_Squish(mobj_t *mo)
{
	const fixed_t maxstretch = 4*FRACUNIT;
	const fixed_t factor = 5 * mo->height / 4;
	const fixed_t threshold = factor / 6;

	fixed_t old3dspeed = abs(mo->lastmomz);
	fixed_t new3dspeed = abs(mo->momz);

	fixed_t delta = abs(old3dspeed - new3dspeed);
	fixed_t grav = mo->height/3;
	fixed_t add = abs(grav - new3dspeed);

	if (R_ThingIsFloorSprite(mo))
		return;

	if (delta < 2 * add && new3dspeed > grav)
		delta += add;

	if (delta > threshold)
	{
		mo->spritexscale =
			FRACUNIT + FixedDiv(delta, factor);

		if (mo->spritexscale > maxstretch)
			mo->spritexscale = maxstretch;

		if (new3dspeed > old3dspeed || new3dspeed > grav)
		{
			mo->spritexscale =
				FixedDiv(FRACUNIT, mo->spritexscale);
		}
	}
	else
	{
		mo->spritexscale -=
			(mo->spritexscale - FRACUNIT)
			/ (mo->spritexscale < FRACUNIT ? 8 : 3);
	}

	mo->spriteyscale =
		FixedDiv(FRACUNIT, mo->spritexscale);
}

static mobj_t *K_FindLastTrailMobj(player_t *player)
{
	mobj_t *trail;

	if (!player || !(trail = player->mo) || !player->mo->hnext || !player->mo->hnext->health)
		return NULL;

	while (trail->hnext && !P_MobjWasRemoved(trail->hnext) && trail->hnext->health)
	{
		trail = trail->hnext;
	}

	return trail;
}

mobj_t *K_ThrowKartItem(player_t *player, boolean missile, mobjtype_t mapthing, INT32 defaultDir, INT32 altthrow, angle_t angleOffset)
{
	mobj_t *mo;
	INT32 dir;
	fixed_t PROJSPEED;
	angle_t newangle;
	fixed_t newx, newy, newz;
	mobj_t *throwmo;

	if (!player)
		return NULL;

	// Figure out projectile speed by game speed
	if (missile)
	{
		// Use info->speed for missiles
		PROJSPEED = FixedMul(mobjinfo[mapthing].speed, K_GetKartGameSpeedScalar(gamespeed));
	}
	else
	{
		// Use pre-determined speed for tossing
		PROJSPEED = FixedMul(82 * FRACUNIT, K_GetKartGameSpeedScalar(gamespeed));
	}

	// Scale to map scale
	// Intentionally NOT player scale, that doesn't work.
	PROJSPEED = FixedMul(PROJSPEED, mapobjectscale);

	if (altthrow)
	{
		if (altthrow == 2) // Kitchen sink throwing
		{
#if 0
			if (player->throwdir == 1)
				dir = 3;
			else if (player->throwdir == -1)
				dir = 1;
			else
				dir = 2;
#else
			if (player->throwdir == 1)
				dir = 2;
			else
				dir = 1;
#endif
		}
		else
		{
			if (player->throwdir == 1)
				dir = 2;
			else if (player->throwdir == -1)
				dir = -1;
			else
				dir = 1;
		}
	}
	else
	{
		if (player->throwdir != 0)
			dir = player->throwdir;
		else
			dir = defaultDir;
	}

	if (missile) // Shootables
	{
		if (dir < 0 && mapthing != MT_SPB)
		{
			// Shoot backward
			mo = K_SpawnKartMissile(player->mo, mapthing, (player->mo->angle + ANGLE_180) + angleOffset, 0, PROJSPEED, dir);
		}
		else
		{
			// Shoot forward
			mo = K_SpawnKartMissile(player->mo, mapthing, player->mo->angle + angleOffset, 0, PROJSPEED, dir);
		}

		if (mapthing == MT_DROPTARGET && mo)
		{
			mo->reactiontime = TICRATE/2;
			P_SetMobjState(mo, mo->info->painstate);
		}
	}
	else
	{
		fixed_t finalscale = K_ItemScaleForPlayer(player);

		player->bananadrag = 0; // RESET timer, for multiple bananas

		if (dir > 0)
		{
			// Shoot forward
			mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, mapthing);

			// These are really weird so let's make it a very specific case to make SURE it works...
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				mo->z -= player->mo->height;
				mo->eflags |= MFE_VERTICALFLIP;
				mo->flags2 |= (player->mo->flags2 & MF2_OBJECTFLIP);
			}

			mo->threshold = 10;
			P_SetTarget(&mo->target, player->mo);

			S_StartSound(player->mo, mo->info->seesound);

			if (mo)
			{
				angle_t fa = player->mo->angle>>ANGLETOFINESHIFT;
				fixed_t HEIGHT = ((20 + (dir*10)) * FRACUNIT) + (FixedDiv(player->mo->momz, mapobjectscale)*P_MobjFlip(player->mo)); // Also intentionally not player scale

				P_SetObjectMomZ(mo, HEIGHT, false);
				mo->momx = player->mo->momx + FixedMul(FINECOSINE(fa), PROJSPEED*dir);
				mo->momy = player->mo->momy + FixedMul(FINESINE(fa), PROJSPEED*dir);

				mo->extravalue2 = dir;

				if (mo->eflags & MFE_UNDERWATER)
					mo->momz = (117 * mo->momz) / 200;

				P_SetScale(mo, finalscale);
				mo->destscale = finalscale;
			}

			// this is the small graphic effect that plops in you when you throw an item:
			throwmo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_FIREDITEM);
			P_SetTarget(&throwmo->target, player->mo);
			// Ditto:
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				throwmo->z -= player->mo->height;
				throwmo->eflags |= MFE_VERTICALFLIP;
				mo->flags2 |= (player->mo->flags2 & MF2_OBJECTFLIP);
			}

			throwmo->movecount = 0; // above player

			P_SetScale(throwmo, finalscale);
			throwmo->destscale = finalscale;
		}
		else
		{
			mobj_t *lasttrail = K_FindLastTrailMobj(player);

			if (mapthing == MT_BUBBLESHIELDTRAP) // Drop directly on top of you.
			{
				newangle = player->mo->angle;
				newx = player->mo->x + player->mo->momx;
				newy = player->mo->y + player->mo->momy;
				newz = player->mo->z;
			}
			else if (lasttrail)
			{
				newangle = lasttrail->angle;
				newx = lasttrail->x;
				newy = lasttrail->y;
				newz = lasttrail->z;
			}
			else
			{
				// Drop it directly behind you.
				fixed_t dropradius = FixedHypot(player->mo->radius, player->mo->radius) + FixedHypot(mobjinfo[mapthing].radius, mobjinfo[mapthing].radius);

				newangle = player->mo->angle;

				newx = player->mo->x + P_ReturnThrustX(player->mo, newangle + ANGLE_180, dropradius);
				newy = player->mo->y + P_ReturnThrustY(player->mo, newangle + ANGLE_180, dropradius);
				newz = player->mo->z;
			}

			mo = P_SpawnMobj(newx, newy, newz, mapthing); // this will never return null because collision isn't processed here
			K_FlipFromObject(mo, player->mo);

			mo->threshold = 10;
			P_SetTarget(&mo->target, player->mo);

			P_SetScale(mo, finalscale);
			mo->destscale = finalscale;

			if (P_IsObjectOnGround(player->mo))
			{
				// floorz and ceilingz aren't properly set to account for FOFs and Polyobjects on spawn
				// This should set it for FOFs
				P_SetOrigin(mo, mo->x, mo->y, mo->z); // however, THIS can fuck up your day. just absolutely ruin you.
				if (P_MobjWasRemoved(mo))
					return NULL;

				if (P_MobjFlip(mo) > 0)
				{
					if (mo->floorz > mo->target->z - mo->height)
					{
						mo->z = mo->floorz;
					}
				}
				else
				{
					if (mo->ceilingz < mo->target->z + mo->target->height + mo->height)
					{
						mo->z = mo->ceilingz - mo->height;
					}
				}
			}

			if (player->mo->eflags & MFE_VERTICALFLIP)
				mo->eflags |= MFE_VERTICALFLIP;

			if (mapthing == MT_SSMINE)
				mo->extravalue1 = 49; // Pads the start-up length from 21 frames to a full 2 seconds
			else if (mapthing == MT_BUBBLESHIELDTRAP)
			{
				P_SetScale(mo, ((5*mo->destscale)>>2)*4);
				mo->destscale = (5*mo->destscale)>>2;
				S_StartSound(mo, sfx_s3kbfl);
			}
		}
	}

	return mo;
}

void K_PuntMine(mobj_t *origMine, mobj_t *punter)
{
	angle_t fa = K_MomentumAngle(punter);
	fixed_t z = (punter->momz * P_MobjFlip(punter)) + (30 * FRACUNIT);
	fixed_t spd;
	mobj_t *mine;

	if (!origMine || P_MobjWasRemoved(origMine))
		return;

	if (punter->hitlag > 0)
		return;

	// This guarantees you hit a mine being dragged
	if (origMine->type == MT_SSMINE_SHIELD) // Create a new mine, and clean up the old one
	{
		mobj_t *mineOwner = origMine->target;

		mine = P_SpawnMobj(origMine->x, origMine->y, origMine->z, MT_SSMINE);

		P_SetTarget(&mine->target, mineOwner);

		mine->angle = origMine->angle;
		mine->flags2 = origMine->flags2;
		mine->floorz = origMine->floorz;
		mine->ceilingz = origMine->ceilingz;

		P_SetScale(mine, origMine->scale);
		mine->destscale = origMine->destscale;
		mine->scalespeed = origMine->scalespeed;

		// Copy interp data
		mine->old_angle = origMine->old_angle;
		mine->old_x = origMine->old_x;
		mine->old_y = origMine->old_y;
		mine->old_z = origMine->old_z;

		// Since we aren't using P_KillMobj, we need to clean up the hnext reference
		P_SetTarget(&mineOwner->hnext, NULL);
		K_UnsetItemOut(mineOwner->player);

		if (mineOwner->player->itemamount)
		{
			mineOwner->player->itemamount--;
		}

		if (!mineOwner->player->itemamount)
		{
			mineOwner->player->itemtype = KITEM_NONE;
		}

		P_RemoveMobj(origMine);
	}
	else
	{
		mine = origMine;
	}

	if (!mine || P_MobjWasRemoved(mine))
		return;

	if (mine->threshold > 0 || mine->hitlag > 0)
		return;

	spd = FixedMul(82 * punter->scale, K_GetKartGameSpeedScalar(gamespeed)); // Avg Speed is 41 in Normal

	mine->flags |= MF_NOCLIPTHING;

	P_SetMobjState(mine, S_SSMINE_AIR1);
	mine->threshold = 10;
	mine->reactiontime = mine->info->reactiontime;

	mine->momx = punter->momx + FixedMul(FINECOSINE(fa >> ANGLETOFINESHIFT), spd);
	mine->momy = punter->momy + FixedMul(FINESINE(fa >> ANGLETOFINESHIFT), spd);
	P_SetObjectMomZ(mine, z, false);

	//K_SetHitLagForObjects(punter, mine, 5);

	mine->flags &= ~MF_NOCLIPTHING;
}

#define THUNDERRADIUS 320

// Rough size of the outer-rim sprites, after scaling.
// (The hitbox is already pretty strict due to only 1 active frame,
// we don't need to have it disjointedly small too...)
#define THUNDERSPRITE 80

static void K_DoLightningShield(player_t *player)
{
	mobj_t *mo;
	int i = 0;
	fixed_t sx;
	fixed_t sy;
	angle_t an;

	S_StartSound(player->mo, sfx_zio3);
	K_LightningShieldAttack(player->mo, (THUNDERRADIUS + THUNDERSPRITE) * FRACUNIT);

	// spawn vertical bolt
	mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_THOK);
	P_SetTarget(&mo->target, player->mo);
	P_SetMobjState(mo, S_LZIO11);
	mo->color = SKINCOLOR_TEAL;
	mo->scale = player->mo->scale*3 + (player->mo->scale/2);

	mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_THOK);
	P_SetTarget(&mo->target, player->mo);
	P_SetMobjState(mo, S_LZIO21);
	mo->color = SKINCOLOR_CYAN;
	mo->scale = player->mo->scale*3 + (player->mo->scale/2);

	// spawn horizontal bolts;
	for (i=0; i<7; i++)
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_THOK);
		P_InitAngle(mo, P_RandomRange(PR_DECORATION, 0, 359)*ANG1);
		mo->fuse = P_RandomRange(PR_DECORATION, 20, 50);
		P_SetTarget(&mo->target, player->mo);
		P_SetMobjState(mo, S_KLIT1);
	}

	// spawn the radius thing:
	an = ANGLE_22h;
	for (i=0; i<15; i++)
	{
		sx = player->mo->x + FixedMul((player->mo->scale*THUNDERRADIUS), FINECOSINE((an*i)>>ANGLETOFINESHIFT));
		sy = player->mo->y + FixedMul((player->mo->scale*THUNDERRADIUS), FINESINE((an*i)>>ANGLETOFINESHIFT));
		mo = P_SpawnMobj(sx, sy, player->mo->z, MT_THOK);
		P_InitAngle(mo, an*i);
		mo->extravalue1 = THUNDERRADIUS;	// Used to know whether we should teleport by radius or something.
		mo->scale = player->mo->scale*3;
		P_SetTarget(&mo->target, player->mo);
		P_SetMobjState(mo, S_KSPARK1);
	}
}

#undef THUNDERRADIUS
#undef THUNDERSPRITE

static void K_FlameDashLeftoverSmoke(mobj_t *src)
{
	UINT8 i;

	for (i = 0; i < 2; i++)
	{
		mobj_t *smoke = P_SpawnMobj(src->x, src->y, src->z+(8<<FRACBITS), MT_BOOSTSMOKE);

		P_SetScale(smoke, src->scale);
		smoke->destscale = 3*src->scale/2;
		smoke->scalespeed = src->scale/12;

		smoke->momx = 3*src->momx/4;
		smoke->momy = 3*src->momy/4;
		smoke->momz = 3*P_GetMobjZMovement(src)/4;

		P_Thrust(smoke, src->angle + FixedAngle(P_RandomRange(PR_DECORATION, 135, 225)<<FRACBITS), P_RandomRange(PR_DECORATION, 0, 8) * src->scale);
		smoke->momz += P_RandomRange(PR_DECORATION, 0, 4) * src->scale;
	}
}

void K_DoSneaker(player_t *player, INT32 type)
{
	const fixed_t intendedboost = FRACUNIT/2;

	if (player->floorboost == 0 || player->floorboost == 3)
	{
		const sfxenum_t normalsfx = sfx_cdfm01;
		const sfxenum_t smallsfx = sfx_cdfm40;
		sfxenum_t sfx = normalsfx;

		if (player->numsneakers)
		{
			// Use a less annoying sound when stacking sneakers.
			sfx = smallsfx;
		}

		S_StopSoundByID(player->mo, normalsfx);
		S_StopSoundByID(player->mo, smallsfx);
		S_StartSound(player->mo, sfx);

		K_SpawnDashDustRelease(player);
		if (intendedboost > player->speedboost)
			player->karthud[khud_destboostcam] = FixedMul(FRACUNIT, FixedDiv((intendedboost - player->speedboost), intendedboost));

		player->numsneakers++;
	}

	if (player->sneakertimer == 0)
	{
		if (type == 2)
		{
			if (player->mo->hnext)
			{
				mobj_t *cur = player->mo->hnext;
				while (cur && !P_MobjWasRemoved(cur))
				{
					if (!cur->tracer)
					{
						mobj_t *overlay = P_SpawnMobj(cur->x, cur->y, cur->z, MT_BOOSTFLAME);
						P_SetTarget(&overlay->target, cur);
						P_SetTarget(&cur->tracer, overlay);
						P_SetScale(overlay, (overlay->destscale = 3*cur->scale/4));
						K_FlipFromObject(overlay, cur);
					}
					cur = cur->hnext;
				}
			}
		}
		else
		{
			mobj_t *overlay = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BOOSTFLAME);
			P_SetTarget(&overlay->target, player->mo);
			P_SetScale(overlay, (overlay->destscale = player->mo->scale));
			K_FlipFromObject(overlay, player->mo);
		}
	}

	player->sneakertimer = sneakertime;

	// set angle for spun out players:
	player->boostangle = player->mo->angle;
}

static void K_DoShrink(player_t *user)
{
	S_StartSound(user->mo, sfx_kc46); // Sound the BANG!

	Obj_CreateShrinkPohbees(user);

#if 0
	{
		mobj_t *mobj, *next;

		// kill everything in the kitem list while we're at it:
		for (mobj = kitemcap; mobj; mobj = next)
		{
			next = mobj->itnext;

			if (mobj->type == MT_SPB)
			{
				continue;
			}

			// check if the item is being held by a player behind us before removing it.
			// check if the item is a "shield" first, bc i'm p sure thrown items keep the player that threw em as target anyway

			if (mobj->type == MT_BANANA_SHIELD || mobj->type == MT_JAWZ_SHIELD ||
			mobj->type == MT_SSMINE_SHIELD || mobj->type == MT_EGGMANITEM_SHIELD ||
			mobj->type == MT_SINK_SHIELD || mobj->type == MT_ORBINAUT_SHIELD ||
			mobj->type == MT_DROPTARGET_SHIELD)
			{
				if (mobj->target && mobj->target->player)
				{
					if (mobj->target->player->position > user->position)
						continue; // this guy's behind us, don't take his stuff away!
				}
			}

			mobj->destscale = 0;
			mobj->flags &= ~(MF_SOLID|MF_SHOOTABLE|MF_SPECIAL);
			mobj->flags |= MF_NOCLIPTHING; // Just for safety
		}
	}
#endif
}

void K_DoPogoSpring(mobj_t *mo, fixed_t vertispeed, UINT8 sound)
{
	fixed_t thrust = 0;

	if (mo->player && mo->player->spectator)
		return;

	if (mo->eflags & MFE_SPRUNG)
		return;

	mo->standingslope = NULL;
	mo->terrain = NULL;

	mo->eflags |= MFE_SPRUNG;

	if (vertispeed == 0)
	{
		thrust = P_AproxDistance(mo->momx, mo->momy) * P_MobjFlip(mo);
		thrust = FixedMul(thrust, FINESINE(ANGLE_22h >> ANGLETOFINESHIFT));
	}
	else
	{
		thrust = vertispeed * P_MobjFlip(mo);
	}

	if (mo->player)
	{
		if (mo->player->sneakertimer)
		{
			thrust = FixedMul(thrust, 5*FRACUNIT/4);
		}
		else if (mo->player->invincibilitytimer)
		{
			thrust = FixedMul(thrust, 9*FRACUNIT/8);
		}

		mo->player->tricktime = 0; // Reset post-hitlag timer
		// Setup the boost for potential upwards trick, at worse, make it your regular max speed. (boost = curr speed*1.25)
		mo->player->trickboostpower = max(FixedDiv(mo->player->speed, K_GetKartSpeed(mo->player, false, false)) - FRACUNIT, 0)*125/100;
		//CONS_Printf("Got boost: %d%\n", mo->player->trickboostpower*100 / FRACUNIT);
	}

	mo->momz = FixedMul(thrust, mapobjectscale);

	if (mo->eflags & MFE_UNDERWATER)
	{
		mo->momz = FixedDiv(mo->momz, FixedSqrt(3*FRACUNIT));
	}

	if (sound)
	{
		S_StartSound(mo, (sound == 1 ? sfx_kc2f : sfx_kpogos));
	}
}

static void K_ThrowLandMine(player_t *player)
{
	mobj_t *landMine;
	mobj_t *throwmo;

	landMine = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_LANDMINE);
	K_FlipFromObject(landMine, player->mo);
	landMine->threshold = 10;

	if (landMine->info->seesound)
		S_StartSound(player->mo, landMine->info->seesound);

	P_SetTarget(&landMine->target, player->mo);

	P_SetScale(landMine, player->mo->scale);
	landMine->destscale = player->mo->destscale;

	P_InitAngle(landMine, player->mo->angle);

	landMine->momz = (30 * mapobjectscale * P_MobjFlip(player->mo)) + player->mo->momz;
	landMine->color = player->skincolor;

	throwmo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_FIREDITEM);
	P_SetTarget(&throwmo->target, player->mo);
	// Ditto:
	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		throwmo->z -= player->mo->height;
		throwmo->eflags |= MFE_VERTICALFLIP;
	}

	throwmo->movecount = 0; // above player
}

void K_DoInvincibility(player_t *player, tic_t time)
{
	if (!player->invincibilitytimer)
	{
		mobj_t *overlay = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INVULNFLASH);
		P_SetTarget(&overlay->target, player->mo);
		overlay->destscale = player->mo->scale;
		P_SetScale(overlay, player->mo->scale);
	}

	player->invincibilitytimer += time;

	if (P_IsLocalPlayer(player) == true)
	{
		S_ChangeMusicSpecial("kinvnc");
	}
	else //used to be "if (P_IsDisplayPlayer(player) == false)"
	{
		S_StartSound(player->mo, sfx_alarmi);
	}

	P_RestoreMusic(player);
}

void K_KillBananaChain(mobj_t *banana, mobj_t *inflictor, mobj_t *source)
{
	mobj_t *cachenext;

killnext:
	if (P_MobjWasRemoved(banana))
		return;

	cachenext = banana->hnext;

	if (banana->health)
	{
		if (banana->eflags & MFE_VERTICALFLIP)
			banana->z -= banana->height;
		else
			banana->z += banana->height;

		S_StartSound(banana, banana->info->deathsound);
		P_KillMobj(banana, inflictor, source, DMG_NORMAL);

		P_SetObjectMomZ(banana, 8*FRACUNIT, false);
		if (inflictor)
			P_InstaThrust(banana, R_PointToAngle2(inflictor->x, inflictor->y, banana->x, banana->y)+ANGLE_90, 16*FRACUNIT);
	}

	if ((banana = cachenext))
		goto killnext;
}

// Just for firing/dropping items.
void K_UpdateHnextList(player_t *player, boolean clean)
{
	mobj_t *work = player->mo, *nextwork;

	if (!work)
		return;

	nextwork = work->hnext;

	while ((work = nextwork) && !(work == NULL || P_MobjWasRemoved(work)))
	{
		nextwork = work->hnext;

		if (!clean && (!work->movedir || work->movedir <= (UINT16)player->itemamount))
		{
			continue;
		}

		P_RemoveMobj(work);
	}

	if (player->mo->hnext == NULL || P_MobjWasRemoved(player->mo->hnext))
	{
		// Like below, try to clean up the pointer if it's NULL.
		// Maybe this was a cause of the shrink/eggbox fails?
		P_SetTarget(&player->mo->hnext, NULL);
	}
}

// For getting hit!
void K_DropHnextList(player_t *player, boolean keepshields)
{
	mobj_t *work = player->mo, *nextwork, *dropwork;
	INT32 flip;
	mobjtype_t type;
	boolean orbit, ponground, dropall = true;
	INT32 shield = K_GetShieldFromItem(player->itemtype);

	if (work == NULL || P_MobjWasRemoved(work))
	{
		return;
	}

	flip = P_MobjFlip(player->mo);
	ponground = P_IsObjectOnGround(player->mo);

	if (shield != KSHIELD_NONE && !keepshields)
	{
		if (shield == KSHIELD_LIGHTNING)
		{
			K_DoLightningShield(player);
		}

		player->curshield = KSHIELD_NONE;
		player->itemtype = KITEM_NONE;
		player->itemamount = 0;
		K_UnsetItemOut(player);
	}

	nextwork = work->hnext;

	while ((work = nextwork) && !(work == NULL || P_MobjWasRemoved(work)))
	{
		nextwork = work->hnext;

		if (!work->health)
			continue; // taking care of itself

		switch (work->type)
		{
			// Kart orbit items
			case MT_ORBINAUT_SHIELD:
				orbit = true;
				type = MT_ORBINAUT;
				break;
			case MT_JAWZ_SHIELD:
				orbit = true;
				type = MT_JAWZ;
				break;
			// Kart trailing items
			case MT_BANANA_SHIELD:
				orbit = false;
				type = MT_BANANA;
				break;
			case MT_SSMINE_SHIELD:
				orbit = false;
				dropall = false;
				type = MT_SSMINE;
				break;
			case MT_DROPTARGET_SHIELD:
				orbit = false;
				dropall = false;
				type = MT_DROPTARGET;
				break;
			case MT_EGGMANITEM_SHIELD:
				orbit = false;
				type = MT_EGGMANITEM;
				break;
			// intentionally do nothing
			case MT_ROCKETSNEAKER:
			case MT_SINK_SHIELD:
				return;
			default:
				continue;
		}

		dropwork = P_SpawnMobj(work->x, work->y, work->z, type);

		P_SetTarget(&dropwork->target, player->mo);
		P_AddKartItem(dropwork); // needs to be called here so shrink can bust items off players in front of the user.

		dropwork->angle = work->angle;

		P_SetScale(dropwork, work->scale);
		dropwork->destscale = K_ItemScaleForPlayer(player); //work->destscale;
		dropwork->scalespeed = work->scalespeed;
		dropwork->spritexscale = work->spritexscale;
		dropwork->spriteyscale = work->spriteyscale;

		dropwork->flags |= MF_NOCLIPTHING;
		dropwork->flags2 = work->flags2;
		dropwork->eflags = work->eflags;

		dropwork->renderflags = work->renderflags;
		dropwork->color = work->color;
		dropwork->colorized = work->colorized;
		dropwork->whiteshadow = work->whiteshadow;

		dropwork->floorz = work->floorz;
		dropwork->ceilingz = work->ceilingz;

		dropwork->health = work->health; // will never be set to 0 as long as above guard exists
		dropwork->hitlag = work->hitlag;

		if (orbit == true)
		{
			// Projectile item; set fuse
			dropwork->fuse = RR_PROJECTILE_FUSE;
		}

		// Copy interp data
		dropwork->old_angle = work->old_angle;
		dropwork->old_x = work->old_x;
		dropwork->old_y = work->old_y;
		dropwork->old_z = work->old_z;

		if (ponground)
		{
			// floorz and ceilingz aren't properly set to account for FOFs and Polyobjects on spawn
			// This should set it for FOFs
			//P_SetOrigin(dropwork, dropwork->x, dropwork->y, dropwork->z); -- handled better by above floorz/ceilingz passing

			if (flip == 1)
			{
				if (dropwork->floorz > dropwork->target->z - dropwork->height)
				{
					dropwork->z = dropwork->floorz;
				}
			}
			else
			{
				if (dropwork->ceilingz < dropwork->target->z + dropwork->target->height + dropwork->height)
				{
					dropwork->z = dropwork->ceilingz - dropwork->height;
				}
			}
		}

		if (orbit) // splay out
		{
			if (dropwork->destscale > work->destscale)
			{
				fixed_t radius = FixedMul(work->info->radius, dropwork->destscale);
				radius = FixedHypot(player->mo->radius, player->mo->radius) + FixedHypot(radius, radius); // mobj's distance from its Target, or Radius.
				dropwork->flags |= MF_NOCLIPTHING;
				work->momx = FixedMul(FINECOSINE(work->angle>>ANGLETOFINESHIFT), radius);
				work->momy = FixedMul(FINESINE(work->angle>>ANGLETOFINESHIFT), radius);
				P_MoveOrigin(dropwork, player->mo->x + work->momx, player->mo->y + work->momy, player->mo->z);
				dropwork->flags &= ~MF_NOCLIPTHING;
			}

			dropwork->flags2 |= MF2_AMBUSH;

			dropwork->z += flip;

			dropwork->momx = player->mo->momx>>1;
			dropwork->momy = player->mo->momy>>1;
			dropwork->momz = 3*flip*mapobjectscale;

			if (dropwork->eflags & MFE_UNDERWATER)
				dropwork->momz = (117 * dropwork->momz) / 200;

			P_Thrust(dropwork, work->angle - ANGLE_90, 6*mapobjectscale);

			dropwork->movecount = 2;
			dropwork->movedir = work->angle - ANGLE_90;

			P_SetMobjState(dropwork, dropwork->info->deathstate);

			dropwork->tics = -1;

			if (type == MT_JAWZ)
			{
				dropwork->z += 20*flip*dropwork->scale;
			}
			else
			{
				dropwork->angle -= ANGLE_90;
			}
		}
		else // plop on the ground
		{
			dropwork->threshold = 10;
			dropwork->flags &= ~MF_NOCLIPTHING;

			if (type == MT_DROPTARGET && dropwork->hitlag)
			{
				dropwork->momx = work->momx;
				dropwork->momy = work->momy;
				dropwork->momz = work->momz;
				dropwork->reactiontime = work->reactiontime;
				P_SetMobjState(dropwork, mobjinfo[type].painstate);
			}
		}

		P_RemoveMobj(work);
	}

	// we need this here too because this is done in afterthink - pointers are cleaned up at the START of each tic...
	P_SetTarget(&player->mo->hnext, NULL);

	player->bananadrag = 0;

	if (player->pflags & PF_EGGMANOUT)
	{
		player->pflags &= ~PF_EGGMANOUT;
	}
	else if ((player->pflags & PF_ITEMOUT)
		&& (dropall || (--player->itemamount <= 0)))
	{
		player->itemamount = 0;
		K_UnsetItemOut(player);
		player->itemtype = KITEM_NONE;
	}
}

mobj_t *K_CreatePaperItem(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 type, UINT8 amount)
{
	mobj_t *drop = P_SpawnMobj(x, y, z, MT_FLOATINGITEM);
	P_SetScale(drop, drop->scale>>4);
	drop->destscale = (3*drop->destscale)/2;

	P_InitAngle(drop, angle);
	P_Thrust(drop,
		FixedAngle(P_RandomFixed(PR_ITEM_ROULETTE) * 180) + angle,
		16*mapobjectscale);

	drop->momz = flip * 3 * mapobjectscale;
	if (drop->eflags & MFE_UNDERWATER)
		drop->momz = (117 * drop->momz) / 200;

	if (type == 0)
	{
		UINT8 useodds = 0;
		INT32 spawnchance[NUMKARTRESULTS];
		INT32 totalspawnchance = 0;
		INT32 i;

		memset(spawnchance, 0, sizeof (spawnchance));

		useodds = amount;

		for (i = 1; i < NUMKARTRESULTS; i++)
		{
			spawnchance[i] = (totalspawnchance += K_KartGetItemOdds(
				useodds, i,
				UINT32_MAX,
				0,
				false, false)
			);
		}

		if (totalspawnchance > 0)
		{
			UINT8 newType;
			UINT8 newAmount;

			totalspawnchance = P_RandomKey(PR_ITEM_ROULETTE, totalspawnchance);
			for (i = 0; i < NUMKARTRESULTS && spawnchance[i] <= totalspawnchance; i++);

			// TODO: this is bad!
			// K_KartGetItemResult requires a player
			// but item roulette will need rewritten to change this

			newType = K_ItemResultToType(i);
			newAmount = K_ItemResultToAmount(i);

			if (newAmount > 1)
			{
				UINT8 j;

				for (j = 0; j < newAmount-1; j++)
				{
					K_CreatePaperItem(
						x, y, z,
						angle, flip,
						newType, 1
					);
				}
			}

			drop->threshold = newType;
			drop->movecount = 1;
		}
		else
		{
			drop->threshold = 1;
			drop->movecount = 1;
		}
	}
	else
	{
		drop->threshold = type;
		drop->movecount = amount;
	}

	drop->flags |= MF_NOCLIPTHING;

	return drop;
}

// For getting EXTRA hit!
void K_DropItems(player_t *player)
{
	K_DropHnextList(player, true);

	if (player->mo && !P_MobjWasRemoved(player->mo) && player->itemamount > 0)
	{
		mobj_t *drop = K_CreatePaperItem(
			player->mo->x, player->mo->y, player->mo->z + player->mo->height/2,
			player->mo->angle + ANGLE_90, P_MobjFlip(player->mo),
			player->itemtype, player->itemamount
		);

		K_FlipFromObject(drop, player->mo);
	}

	K_StripItems(player);
}

void K_DropRocketSneaker(player_t *player)
{
	mobj_t *shoe = player->mo;
	fixed_t flingangle;
	boolean leftshoe = true; //left shoe is first

	if (!(player->mo && !P_MobjWasRemoved(player->mo) && player->mo->hnext && !P_MobjWasRemoved(player->mo->hnext)))
		return;

	while ((shoe = shoe->hnext) && !P_MobjWasRemoved(shoe))
	{
		if (shoe->type != MT_ROCKETSNEAKER)
			return; //woah, not a rocketsneaker, bail! safeguard in case this gets used when you're holding non-rocketsneakers

		shoe->renderflags &= ~RF_DONTDRAW;
		shoe->flags &= ~MF_NOGRAVITY;
		shoe->angle += ANGLE_45;

		if (shoe->eflags & MFE_VERTICALFLIP)
			shoe->z -= shoe->height;
		else
			shoe->z += shoe->height;

		//left shoe goes off tot eh left, right shoe off to the right
		if (leftshoe)
			flingangle = -(ANG60);
		else
			flingangle = ANG60;

		S_StartSound(shoe, shoe->info->deathsound);
		P_SetObjectMomZ(shoe, 8*FRACUNIT, false);
		P_InstaThrust(shoe, R_PointToAngle2(shoe->target->x, shoe->target->y, shoe->x, shoe->y)+flingangle, 16*FRACUNIT);
		shoe->momx += shoe->target->momx;
		shoe->momy += shoe->target->momy;
		shoe->momz += shoe->target->momz;
		shoe->extravalue2 = 1;

		leftshoe = false;
	}
	P_SetTarget(&player->mo->hnext, NULL);
	player->rocketsneakertimer = 0;
}

void K_DropKitchenSink(player_t *player)
{
	if (!(player->mo && !P_MobjWasRemoved(player->mo) && player->mo->hnext && !P_MobjWasRemoved(player->mo->hnext)))
		return;

	if (player->mo->hnext->type != MT_SINK_SHIELD)
		return; //so we can just call this function regardless of what is being held

	P_KillMobj(player->mo->hnext, NULL, NULL, DMG_NORMAL);

	P_SetTarget(&player->mo->hnext, NULL);
}

// When an item in the hnext chain dies.
void K_RepairOrbitChain(mobj_t *orbit)
{
	mobj_t *cachenext = orbit->hnext;

	// First, repair the chain
	if (orbit->hnext && orbit->hnext->health && !P_MobjWasRemoved(orbit->hnext))
	{
		P_SetTarget(&orbit->hnext->hprev, orbit->hprev);
		P_SetTarget(&orbit->hnext, NULL);
	}

	if (orbit->hprev && orbit->hprev->health && !P_MobjWasRemoved(orbit->hprev))
	{
		P_SetTarget(&orbit->hprev->hnext, cachenext);
		P_SetTarget(&orbit->hprev, NULL);
	}

	// Then recount to make sure item amount is correct
	if (orbit->target && orbit->target->player)
	{
		INT32 num = 0;

		mobj_t *cur = orbit->target->hnext;
		mobj_t *prev = NULL;

		while (cur && !P_MobjWasRemoved(cur))
		{
			prev = cur;
			cur = cur->hnext;
			if (++num > orbit->target->player->itemamount)
				P_RemoveMobj(prev);
			else
				prev->movedir = num;
		}

		if (orbit->target->player->itemamount != num)
			orbit->target->player->itemamount = num;
	}
}

// Simplified version of a code bit in P_MobjFloorZ
static fixed_t K_BananaSlopeZ(pslope_t *slope, fixed_t x, fixed_t y, fixed_t z, fixed_t radius, boolean ceiling)
{
	fixed_t testx, testy;

	if (slope == NULL)
	{
		testx = x;
		testy = y;
	}
	else
	{
		if (slope->d.x < 0)
			testx = radius;
		else
			testx = -radius;

		if (slope->d.y < 0)
			testy = radius;
		else
			testy = -radius;

		if ((slope->zdelta > 0) ^ !!(ceiling))
		{
			testx = -testx;
			testy = -testy;
		}

		testx += x;
		testy += y;
	}

	return P_GetZAt(slope, testx, testy, z);
}

void K_CalculateBananaSlope(mobj_t *mobj, fixed_t x, fixed_t y, fixed_t z, fixed_t radius, fixed_t height, boolean flip, boolean player)
{
	fixed_t newz;
	sector_t *sec;
	pslope_t *slope = NULL;

	sec = R_PointInSubsector(x, y)->sector;

	if (flip)
	{
		slope = sec->c_slope;
		newz = K_BananaSlopeZ(slope, x, y, sec->ceilingheight, radius, true);
	}
	else
	{
		slope = sec->f_slope;
		newz = K_BananaSlopeZ(slope, x, y, sec->floorheight, radius, true);
	}

	// Check FOFs for a better suited slope
	if (sec->ffloors)
	{
		ffloor_t *rover;

		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			fixed_t top, bottom;
			fixed_t d1, d2;

			if (!(rover->flags & FF_EXISTS))
				continue;

			if ((!(((rover->flags & FF_BLOCKPLAYER && player)
				|| (rover->flags & FF_BLOCKOTHERS && !player))
				|| (rover->flags & FF_QUICKSAND))
				|| (rover->flags & FF_SWIMMABLE)))
				continue;

			top = K_BananaSlopeZ(*rover->t_slope, x, y, *rover->topheight, radius, false);
			bottom = K_BananaSlopeZ(*rover->b_slope, x, y, *rover->bottomheight, radius, true);

			if (flip)
			{
				if (rover->flags & FF_QUICKSAND)
				{
					if (z < top && (z + height) > bottom)
					{
						if (newz > (z + height))
						{
							newz = (z + height);
							slope = NULL;
						}
					}
					continue;
				}

				d1 = (z + height) - (top + ((bottom - top)/2));
				d2 = z - (top + ((bottom - top)/2));

				if (bottom < newz && abs(d1) < abs(d2))
				{
					newz = bottom;
					slope = *rover->b_slope;
				}
			}
			else
			{
				if (rover->flags & FF_QUICKSAND)
				{
					if (z < top && (z + height) > bottom)
					{
						if (newz < z)
						{
							newz = z;
							slope = NULL;
						}
					}
					continue;
				}

				d1 = z - (bottom + ((top - bottom)/2));
				d2 = (z + height) - (bottom + ((top - bottom)/2));

				if (top > newz && abs(d1) < abs(d2))
				{
					newz = top;
					slope = *rover->t_slope;
				}
			}
		}
	}

	//mobj->standingslope = slope;
	P_SetPitchRollFromSlope(mobj, slope);
}

// Move the hnext chain!
static void K_MoveHeldObjects(player_t *player)
{
	fixed_t finalscale = INT32_MAX;

	if (!player->mo)
		return;

	if (!player->mo->hnext)
	{
		player->bananadrag = 0;

		if (player->pflags & PF_EGGMANOUT)
		{
			player->pflags &= ~PF_EGGMANOUT;
		}
		else if (player->pflags & PF_ITEMOUT)
		{
			player->itemamount = 0;
			K_UnsetItemOut(player);
			player->itemtype = KITEM_NONE;
		}
		return;
	}

	if (P_MobjWasRemoved(player->mo->hnext))
	{
		// we need this here too because this is done in afterthink - pointers are cleaned up at the START of each tic...
		P_SetTarget(&player->mo->hnext, NULL);
		player->bananadrag = 0;

		if (player->pflags & PF_EGGMANOUT)
		{
			player->pflags &= ~PF_EGGMANOUT;
		}
		else if (player->pflags & PF_ITEMOUT)
		{
			player->itemamount = 0;
			K_UnsetItemOut(player);
			player->itemtype = KITEM_NONE;
		}

		return;
	}

	finalscale = K_ItemScaleForPlayer(player);

	switch (player->mo->hnext->type)
	{
		case MT_ORBINAUT_SHIELD: // Kart orbit items
		case MT_JAWZ_SHIELD:
			{
				Obj_OrbinautJawzMoveHeld(player);
				break;
			}
		case MT_BANANA_SHIELD: // Kart trailing items
		case MT_SSMINE_SHIELD:
		case MT_DROPTARGET_SHIELD:
		case MT_EGGMANITEM_SHIELD:
		case MT_SINK_SHIELD:
			{
				mobj_t *cur = player->mo->hnext;
				mobj_t *curnext;
				mobj_t *targ = player->mo;

				if (P_IsObjectOnGround(player->mo) && player->speed > 0)
					player->bananadrag++;

				while (cur && !P_MobjWasRemoved(cur))
				{
					const fixed_t radius = FixedHypot(targ->radius, targ->radius) + FixedHypot(cur->radius, cur->radius);
					angle_t ang;
					fixed_t targx, targy, targz;
					fixed_t speed, dist;

					curnext = cur->hnext;

					if (cur->type == MT_EGGMANITEM_SHIELD)
					{
						// Decided that this should use their "canon" color.
						cur->color = SKINCOLOR_BLACK;
					}
					else if (cur->type == MT_DROPTARGET_SHIELD)
					{
						cur->renderflags = (cur->renderflags|RF_FULLBRIGHT) ^ RF_FULLDARK; // the difference between semi and fullbright
					}

					cur->flags &= ~MF_NOCLIPTHING;

					if ((player->mo->eflags & MFE_VERTICALFLIP) != (cur->eflags & MFE_VERTICALFLIP))
						K_FlipFromObject(cur, player->mo);

					if (!cur->health)
					{
						cur = curnext;
						continue;
					}

					if (cur->extravalue1 < radius)
						cur->extravalue1 += FixedMul(P_AproxDistance(cur->extravalue1, radius), FRACUNIT/12);
					if (cur->extravalue1 > radius)
						cur->extravalue1 = radius;

					if (cur != player->mo->hnext)
					{
						targ = cur->hprev;
						dist = cur->extravalue1/4;
					}
					else
						dist = cur->extravalue1/2;

					if (!targ || P_MobjWasRemoved(targ))
					{
						cur = curnext;
						continue;
					}

					// Shrink your items if the player shrunk too.
					P_SetScale(cur, (cur->destscale = FixedMul(FixedDiv(cur->extravalue1, radius), finalscale)));

					ang = targ->angle;
					targx = targ->x + P_ReturnThrustX(cur, ang + ANGLE_180, dist);
					targy = targ->y + P_ReturnThrustY(cur, ang + ANGLE_180, dist);
					targz = targ->z;

					speed = FixedMul(R_PointToDist2(cur->x, cur->y, targx, targy), 3*FRACUNIT/4);
					if (P_IsObjectOnGround(targ))
						targz = cur->floorz;

					cur->angle = R_PointToAngle2(cur->x, cur->y, targx, targy);

					/*
					if (P_IsObjectOnGround(player->mo) && player->speed > 0 && player->bananadrag > TICRATE
						&& P_RandomChance(PR_UNDEFINED, min(FRACUNIT/2, FixedDiv(player->speed, K_GetKartSpeed(player, false, false))/2)))
					{
						if (leveltime & 1)
							targz += 8*(2*FRACUNIT)/7;
						else
							targz -= 8*(2*FRACUNIT)/7;
					}
					*/

					if (speed > dist)
						P_InstaThrust(cur, cur->angle, speed-dist);

					P_SetObjectMomZ(cur, FixedMul(targz - cur->z, 7*FRACUNIT/8) - gravity, false);

					if (R_PointToDist2(cur->x, cur->y, targx, targy) > 768*FRACUNIT)
					{
						P_MoveOrigin(cur, targx, targy, cur->z);
						if (P_MobjWasRemoved(cur))
						{
							cur = curnext;
							continue;
						}
					}

					if (P_IsObjectOnGround(cur))
					{
						K_CalculateBananaSlope(cur, cur->x, cur->y, cur->z,
							cur->radius, cur->height, (cur->eflags & MFE_VERTICALFLIP), false);
					}

					cur = curnext;
				}
			}
			break;
		case MT_ROCKETSNEAKER: // Special rocket sneaker stuff
			{
				mobj_t *cur = player->mo->hnext;
				INT32 num = 0;

				while (cur && !P_MobjWasRemoved(cur))
				{
					const fixed_t radius = FixedHypot(player->mo->radius, player->mo->radius) + FixedHypot(cur->radius, cur->radius);
					boolean vibrate = ((leveltime & 1) && !cur->tracer);
					angle_t angoffset;
					fixed_t targx, targy, targz;

					cur->flags &= ~MF_NOCLIPTHING;

					if (player->rocketsneakertimer <= TICRATE && (leveltime & 1))
						cur->renderflags |= RF_DONTDRAW;
					else
						cur->renderflags &= ~RF_DONTDRAW;

					if (num & 1)
						P_SetMobjStateNF(cur, (vibrate ? S_ROCKETSNEAKER_LVIBRATE : S_ROCKETSNEAKER_L));
					else
						P_SetMobjStateNF(cur, (vibrate ? S_ROCKETSNEAKER_RVIBRATE : S_ROCKETSNEAKER_R));

					if (!player->rocketsneakertimer || cur->extravalue2 || !cur->health)
					{
						num = (num+1) % 2;
						cur = cur->hnext;
						continue;
					}

					if (cur->extravalue1 < radius)
						cur->extravalue1 += FixedMul(P_AproxDistance(cur->extravalue1, radius), FRACUNIT/12);
					if (cur->extravalue1 > radius)
						cur->extravalue1 = radius;

					// Shrink your items if the player shrunk too.
					P_SetScale(cur, (cur->destscale = FixedMul(FixedDiv(cur->extravalue1, radius), player->mo->scale)));

#if 1
					{
						angle_t input = player->drawangle - cur->angle;
						boolean invert = (input > ANGLE_180);
						if (invert)
							input = InvAngle(input);

						input = FixedAngle(AngleFixed(input)/4);
						if (invert)
							input = InvAngle(input);

						cur->angle = cur->angle + input;
					}
#else
					cur->angle = player->drawangle;
#endif

					angoffset = ANGLE_90 + (ANGLE_180 * num);

					targx = player->mo->x + P_ReturnThrustX(cur, cur->angle + angoffset, cur->extravalue1);
					targy = player->mo->y + P_ReturnThrustY(cur, cur->angle + angoffset, cur->extravalue1);

					{ // bobbing, copy pasted from my kimokawaiii entry
						fixed_t sine = FixedMul(player->mo->scale, 8 * FINESINE((((M_TAU_FIXED * (4*TICRATE)) * leveltime) >> ANGLETOFINESHIFT) & FINEMASK));
						targz = (player->mo->z + (player->mo->height/2)) + sine;
						if (player->mo->eflags & MFE_VERTICALFLIP)
							targz += (player->mo->height/2 - 32*player->mo->scale)*6;
					}

					if (cur->tracer && !P_MobjWasRemoved(cur->tracer))
					{
						fixed_t diffx, diffy, diffz;

						diffx = targx - cur->x;
						diffy = targy - cur->y;
						diffz = targz - cur->z;

						P_MoveOrigin(cur->tracer, cur->tracer->x + diffx + P_ReturnThrustX(cur, cur->angle + angoffset, 6*cur->scale),
							cur->tracer->y + diffy + P_ReturnThrustY(cur, cur->angle + angoffset, 6*cur->scale), cur->tracer->z + diffz);
						P_SetScale(cur->tracer, (cur->tracer->destscale = 3*cur->scale/4));
					}

					P_MoveOrigin(cur, targx, targy, targz);
					K_FlipFromObject(cur, player->mo);	// Update graviflip in real time thanks.

					cur->roll = player->mo->roll;
					cur->pitch = player->mo->pitch;

					num = (num+1) % 2;
					cur = cur->hnext;
				}
			}
			break;
		default:
			break;
	}
}

player_t *K_FindJawzTarget(mobj_t *actor, player_t *source, angle_t range)
{
	fixed_t best = INT32_MAX;
	player_t *wtarg = NULL;
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		angle_t thisang = ANGLE_MAX;
		fixed_t thisdist = INT32_MAX;
		fixed_t thisScore = INT32_MAX;
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];

		// Don't target yourself, stupid.
		if (player == source)
		{
			continue;
		}

		if (player->spectator)
		{
			// Spectators
			continue;
		}

		if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			// Invalid mobj
			continue;
		}

		if (player->mo->health <= 0)
		{
			// dead
			continue;
		}

		if (G_GametypeHasTeams() && source != NULL && source->ctfteam == player->ctfteam)
		{
			// Don't home in on teammates.
			continue;
		}

		if (player->hyudorotimer > 0)
		{
			// Invisible player
			continue;
		}

		thisdist = P_AproxDistance(player->mo->x - (actor->x + actor->momx), player->mo->y - (actor->y + actor->momy));

		if (gametyperules & GTR_CIRCUIT)
		{
			if (player->position >= source->position)
			{
				// Don't pay attention to people who aren't above your position
				continue;
			}
		}
		else
		{
			if (player->bumpers <= 0)
			{
				// Don't pay attention to dead players
				continue;
			}

			// Z pos too high/low
			if (abs(player->mo->z - (actor->z + actor->momz)) > FixedMul(RING_DIST/8, mapobjectscale))
			{
				continue;
			}

			// Distance too far away
			if (thisdist > FixedMul(RING_DIST*2, mapobjectscale))
			{
				continue;
			}
		}

		// Find the angle, see who's got the best.
		thisang = AngleDelta(actor->angle, R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y));

		// Don't go for people who are behind you
		if (thisang > range)
		{
			continue;
		}

		thisScore = (AngleFixed(thisang) * 8) + (thisdist / 32);

		//CONS_Printf("got score %f from player # %d\n", FixedToFloat(thisScore), i);

		if (thisScore < best)
		{
			wtarg = player;
			best = thisScore;
		}
	}

	return wtarg;
}

// Engine Sounds.
static void K_UpdateEngineSounds(player_t *player)
{
	const INT32 numsnds = 13;

	const fixed_t closedist = 160*FRACUNIT;
	const fixed_t fardist = 1536*FRACUNIT;

	const UINT8 dampenval = 48; // 255 * 48 = close enough to FRACUNIT/6

	const UINT16 buttons = K_GetKartButtons(player);

	INT32 class, s, w; // engine class number

	UINT8 volume = 255;
	fixed_t volumedampen = FRACUNIT;

	INT32 targetsnd = 0;
	INT32 i;

	s = (player->kartspeed - 1) / 3;
	w = (player->kartweight - 1) / 3;

#define LOCKSTAT(stat) \
	if (stat < 0) { stat = 0; } \
	if (stat > 2) { stat = 2; }
	LOCKSTAT(s);
	LOCKSTAT(w);
#undef LOCKSTAT

	class = s + (3*w);

	if (leveltime < 8 || player->spectator)
	{
		// Silence the engines, and reset sound number while we're at it.
		player->karthud[khud_enginesnd] = 0;
		return;
	}

#if 0
	if ((leveltime % 8) != ((player-players) % 8)) // Per-player offset, to make engines sound distinct!
#else
	if (leveltime % 8)
#endif
	{
		// .25 seconds of wait time between each engine sound playback
		return;
	}

	if (player->respawn.state == RESPAWNST_DROP) // Dropdashing
	{
		// Dropdashing
		targetsnd = ((buttons & BT_ACCELERATE) ? 12 : 0);
	}
	else if (K_PlayerEBrake(player) == true)
	{
		// Spindashing
		targetsnd = ((buttons & BT_DRIFT) ? 12 : 0);
	}
	else
	{
		// Average out the value of forwardmove and the speed that you're moving at.
		targetsnd = (((6 * K_GetForwardMove(player)) / 25) + ((player->speed / mapobjectscale) / 5)) / 2;
	}

	if (targetsnd < 0) { targetsnd = 0; }
	if (targetsnd > 12) { targetsnd = 12; }

	if (player->karthud[khud_enginesnd] < targetsnd) { player->karthud[khud_enginesnd]++; }
	if (player->karthud[khud_enginesnd] > targetsnd) { player->karthud[khud_enginesnd]--; }

	if (player->karthud[khud_enginesnd] < 0) { player->karthud[khud_enginesnd] = 0; }
	if (player->karthud[khud_enginesnd] > 12) { player->karthud[khud_enginesnd] = 12; }

	// This code calculates how many players (and thus, how many engine sounds) are within ear shot,
	// and rebalances the volume of your engine sound based on how far away they are.

	// This results in multiple things:
	// - When on your own, you will hear your own engine sound extremely clearly.
	// - When you were alone but someone is gaining on you, yours will go quiet, and you can hear theirs more clearly.
	// - When around tons of people, engine sounds will try to rebalance to not be as obnoxious.

	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 thisvol = 0;
		fixed_t dist;

		if (!playeringame[i] || !players[i].mo)
		{
			// This player doesn't exist.
			continue;
		}

		if (players[i].spectator)
		{
			// This player isn't playing an engine sound.
			continue;
		}

		if (P_IsDisplayPlayer(&players[i]))
		{
			// Don't dampen yourself!
			continue;
		}

		dist = P_AproxDistance(
			P_AproxDistance(
				player->mo->x - players[i].mo->x,
				player->mo->y - players[i].mo->y),
				player->mo->z - players[i].mo->z) / 2;

		dist = FixedDiv(dist, mapobjectscale);

		if (dist > fardist)
		{
			// ENEMY OUT OF RANGE !
			continue;
		}
		else if (dist < closedist)
		{
			// engine sounds' approx. range
			thisvol = 255;
		}
		else
		{
			thisvol = (15 * ((closedist - dist) / FRACUNIT)) / ((fardist - closedist) >> (FRACBITS+4));
		}

		volumedampen += (thisvol * dampenval);
	}

	if (volumedampen > FRACUNIT)
	{
		volume = FixedDiv(volume * FRACUNIT, volumedampen) / FRACUNIT;
	}

	if (volume <= 0)
	{
		// Don't need to play the sound at all.
		return;
	}

	S_StartSoundAtVolume(player->mo, (sfx_krta00 + player->karthud[khud_enginesnd]) + (class * numsnds), volume);
}

static void K_UpdateInvincibilitySounds(player_t *player)
{
	INT32 sfxnum = sfx_None;

	if (player->mo->health > 0 && !P_IsLocalPlayer(player)) // used to be !P_IsDisplayPlayer(player)
	{
		if (player->invincibilitytimer > 0) // Prioritize invincibility
			sfxnum = sfx_alarmi;
		else if (player->growshrinktimer > 0)
			sfxnum = sfx_alarmg;
	}

	if (sfxnum != sfx_None && !S_SoundPlaying(player->mo, sfxnum))
		S_StartSound(player->mo, sfxnum);

#define STOPTHIS(this) \
	if (sfxnum != this && S_SoundPlaying(player->mo, this)) \
		S_StopSoundByID(player->mo, this);
	STOPTHIS(sfx_alarmi);
	STOPTHIS(sfx_alarmg);
#undef STOPTHIS
}

void K_KartPlayerHUDUpdate(player_t *player)
{
	if (player->karthud[khud_lapanimation])
		player->karthud[khud_lapanimation]--;

	if (player->karthud[khud_yougotem])
		player->karthud[khud_yougotem]--;

	if (player->karthud[khud_voices])
		player->karthud[khud_voices]--;

	if (player->karthud[khud_tauntvoices])
		player->karthud[khud_tauntvoices]--;

	if (player->karthud[khud_trickcool])
		player->karthud[khud_trickcool]--;

	if (!(player->pflags & PF_FAULT))
		player->karthud[khud_fault] = 0;
	else if (player->karthud[khud_fault] > 0 && player->karthud[khud_fault] < 2*TICRATE)
		player->karthud[khud_fault]++;

	if (player->karthud[khud_itemblink] && player->karthud[khud_itemblink]-- <= 0)
	{
		player->karthud[khud_itemblinkmode] = 0;
		player->karthud[khud_itemblink] = 0;
	}

	if (gametype == GT_RACE)
	{
		// 0 is the fast spin animation, set at 30 tics of ring boost or higher!
		if (player->ringboost >= 30)
			player->karthud[khud_ringdelay] = 0;
		else
			player->karthud[khud_ringdelay] = ((RINGANIM_DELAYMAX+1) * (30 - player->ringboost)) / 30;

		if (player->karthud[khud_ringframe] == 0 && player->karthud[khud_ringdelay] > RINGANIM_DELAYMAX)
		{
			player->karthud[khud_ringframe] = 0;
			player->karthud[khud_ringtics] = 0;
		}
		else if ((player->karthud[khud_ringtics]--) <= 0)
		{
			if (player->karthud[khud_ringdelay] == 0) // fast spin animation
			{
				player->karthud[khud_ringframe] = ((player->karthud[khud_ringframe]+2) % RINGANIM_NUMFRAMES);
				player->karthud[khud_ringtics] = 0;
			}
			else
			{
				player->karthud[khud_ringframe] = ((player->karthud[khud_ringframe]+1) % RINGANIM_NUMFRAMES);
				player->karthud[khud_ringtics] = min(RINGANIM_DELAYMAX, player->karthud[khud_ringdelay])-1;
			}
		}

		if (player->pflags & PF_RINGLOCK)
		{
			UINT8 normalanim = (leveltime % 14);
			UINT8 debtanim = 14 + (leveltime % 2);

			if (player->karthud[khud_ringspblock] >= 14) // debt animation
			{
				if ((player->rings > 0) // Get out of 0 ring animation
					&& (normalanim == 3 || normalanim == 10)) // on these transition frames.
					player->karthud[khud_ringspblock] = normalanim;
				else
					player->karthud[khud_ringspblock] = debtanim;
			}
			else // normal animation
			{
				if ((player->rings <= 0) // Go into 0 ring animation
					&& (player->karthud[khud_ringspblock] == 1 || player->karthud[khud_ringspblock] == 8)) // on these transition frames.
					player->karthud[khud_ringspblock] = debtanim;
				else
					player->karthud[khud_ringspblock] = normalanim;
			}
		}
		else
			player->karthud[khud_ringspblock] = (leveltime % 14); // reset to normal anim next time
	}

	if ((gametyperules & GTR_BUMPERS) && (player->exiting || player->karmadelay))
	{
		if (player->exiting)
		{
			if (player->exiting < 6*TICRATE)
				player->karthud[khud_cardanimation] += ((164-player->karthud[khud_cardanimation])/8)+1;
			else if (player->exiting == 6*TICRATE)
				player->karthud[khud_cardanimation] = 0;
			else if (player->karthud[khud_cardanimation] < 2*TICRATE)
				player->karthud[khud_cardanimation]++;
		}
		else
		{
			if (player->karmadelay < 6*TICRATE)
				player->karthud[khud_cardanimation] -= ((164-player->karthud[khud_cardanimation])/8)+1;
			else if (player->karmadelay < 9*TICRATE)
				player->karthud[khud_cardanimation] += ((164-player->karthud[khud_cardanimation])/8)+1;
		}

		if (player->karthud[khud_cardanimation] > 164)
			player->karthud[khud_cardanimation] = 164;
		if (player->karthud[khud_cardanimation] < 0)
			player->karthud[khud_cardanimation] = 0;
	}
	else if (gametype == GT_RACE && player->exiting)
	{
		if (player->karthud[khud_cardanimation] < 2*TICRATE)
			player->karthud[khud_cardanimation]++;
	}
	else
		player->karthud[khud_cardanimation] = 0;
}

#undef RINGANIM_DELAYMAX

// SRB2Kart: blockmap iterate for attraction shield users
static mobj_t *attractmo;
static fixed_t attractdist;
static fixed_t attractzdist;

static inline BlockItReturn_t PIT_AttractingRings(mobj_t *thing)
{
	if (attractmo == NULL || P_MobjWasRemoved(attractmo) || attractmo->player == NULL)
	{
		return BMIT_ABORT;
	}

	if (thing == NULL || P_MobjWasRemoved(thing))
	{
		return BMIT_CONTINUE; // invalid
	}

	if (thing == attractmo)
	{
		return BMIT_CONTINUE; // invalid
	}

	if (!(thing->type == MT_RING || thing->type == MT_FLINGRING))
	{
		return BMIT_CONTINUE; // not a ring
	}

	if (thing->health <= 0)
	{
		return BMIT_CONTINUE; // dead
	}

	if (thing->extravalue1)
	{
		return BMIT_CONTINUE; // in special ring animation
	}

	if (thing->tracer != NULL && P_MobjWasRemoved(thing->tracer) == false)
	{
		return BMIT_CONTINUE; // already attracted
	}

	// see if it went over / under
	if (attractmo->z - attractzdist > thing->z + thing->height)
	{
		return BMIT_CONTINUE; // overhead
	}

	if (attractmo->z + attractmo->height + attractzdist < thing->z)
	{
		return BMIT_CONTINUE; // underneath
	}

	if (P_AproxDistance(attractmo->x - thing->x, attractmo->y - thing->y) > attractdist + thing->radius)
	{
		return BMIT_CONTINUE; // Too far away
	}

	if (RINGTOTAL(attractmo->player) >= 20 || (attractmo->player->pflags & PF_RINGLOCK))
	{
		// Already reached max -- just joustle rings around.

		// Regular ring -> fling ring
		if (thing->info->reactiontime && thing->type != (mobjtype_t)thing->info->reactiontime)
		{
			thing->type = thing->info->reactiontime;
			thing->info = &mobjinfo[thing->type];
			thing->flags = thing->info->flags;

			P_InstaThrust(thing, P_RandomRange(PR_ITEM_RINGS, 0, 7) * ANGLE_45, 2 * thing->scale);
			P_SetObjectMomZ(thing, 8<<FRACBITS, false);
			thing->fuse = 120*TICRATE;

			thing->cusval = 0; // Reset attraction flag
		}
	}
	else
	{
		// set target
		P_SetTarget(&thing->tracer, attractmo);
	}

	return BMIT_CONTINUE; // find other rings
}

/** Looks for rings near a player in the blockmap.
  *
  * \param pmo Player object looking for rings to attract
  * \sa A_AttractChase
  */
static void K_LookForRings(mobj_t *pmo)
{
	INT32 bx, by, xl, xh, yl, yh;

	attractmo = pmo;
	attractdist = (400 * pmo->scale);
	attractzdist = attractdist >> 2;

	// Use blockmap to check for nearby rings
	yh = (unsigned)(pmo->y + (attractdist + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(pmo->y - (attractdist + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(pmo->x + (attractdist + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(pmo->x - (attractdist + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_AttractingRings);
}

static void K_UpdateTripwire(player_t *player)
{
	fixed_t speedThreshold = (3*K_GetKartSpeed(player, false, true))/4;
	boolean goodSpeed = (player->speed >= speedThreshold);
	boolean boostExists = (player->tripwireLeniency > 0); // can't be checked later because of subtractions...
	tripwirepass_t triplevel = K_TripwirePassConditions(player);

	if (triplevel != TRIPWIRE_NONE)
	{
		if (!boostExists)
		{
			mobj_t *front = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_TRIPWIREBOOST);
			mobj_t *back = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_TRIPWIREBOOST);

			P_SetTarget(&front->target, player->mo);
			P_SetTarget(&back->target, player->mo);

			front->dispoffset = 1;
			front->old_angle = back->old_angle = K_MomentumAngle(player->mo);
			back->dispoffset = -1;
			back->extravalue1 = 1;
			P_SetMobjState(back, S_TRIPWIREBOOST_BOTTOM);
		}

		player->tripwirePass = triplevel;
		player->tripwireLeniency = max(player->tripwireLeniency, TRIPWIRETIME);
	}
	else
	{
		if (boostExists)
		{
			player->tripwireLeniency--;
			if (goodSpeed == false && player->tripwireLeniency > 0)
			{
				// Decrease at double speed when your speed is bad.
				player->tripwireLeniency--;
			}
		}

		if (player->tripwireLeniency <= 0)
		{
			player->tripwirePass = TRIPWIRE_NONE;
		}
	}
}

/**	\brief	Decreases various kart timers and powers per frame. Called in P_PlayerThink in p_user.c

	\param	player	player object passed from P_PlayerThink
	\param	cmd		control input from player

	\return	void
*/
void K_KartPlayerThink(player_t *player, ticcmd_t *cmd)
{
	const boolean onground = P_IsObjectOnGround(player->mo);

	K_UpdateOffroad(player);
	K_UpdateDraft(player);
	K_UpdateEngineSounds(player); // Thanks, VAda!

	// update boost angle if not spun out
	if (!player->spinouttimer && !player->wipeoutslow)
		player->boostangle = player->mo->angle;

	K_GetKartBoostPower(player);

	// Special effect objects!
	if (player->mo && !player->spectator)
	{
		if (player->dashpadcooldown) // Twinkle Circuit afterimages
		{
			mobj_t *ghost;
			ghost = P_SpawnGhostMobj(player->mo);
			ghost->fuse = player->dashpadcooldown+1;
			ghost->momx = player->mo->momx / (player->dashpadcooldown+1);
			ghost->momy = player->mo->momy / (player->dashpadcooldown+1);
			ghost->momz = player->mo->momz / (player->dashpadcooldown+1);
			player->dashpadcooldown--;
		}

		if (player->speed > 0)
		{
			// Speed lines
			if (player->sneakertimer || player->ringboost
				|| player->driftboost || player->startboost
				|| player->eggmanexplode || player->trickboost
				|| player->gateBoost)
			{
#if 0
				if (player->invincibilitytimer)
					K_SpawnInvincibilitySpeedLines(player->mo);
				else
#endif
					K_SpawnNormalSpeedLines(player);
			}

			if (player->numboosts > 0) // Boosting after images
			{
				mobj_t *ghost;
				ghost = P_SpawnGhostMobj(player->mo);
				ghost->extravalue1 = player->numboosts+1;
				ghost->extravalue2 = (leveltime % ghost->extravalue1);
				ghost->fuse = ghost->extravalue1;
				ghost->renderflags |= RF_FULLBRIGHT;
				ghost->colorized = true;
				//ghost->color = player->skincolor;
				//ghost->momx = (3*player->mo->momx)/4;
				//ghost->momy = (3*player->mo->momy)/4;
				//ghost->momz = (3*player->mo->momz)/4;
				if (leveltime & 1)
					ghost->renderflags |= RF_DONTDRAW;
			}

			if (P_IsObjectOnGround(player->mo))
			{
				// Draft dust
				if (player->draftpower >= FRACUNIT)
				{
					K_SpawnDraftDust(player->mo);
					/*if (leveltime % 23 == 0 || !S_SoundPlaying(player->mo, sfx_s265))
						S_StartSound(player->mo, sfx_s265);*/
				}
			}
		}

		if (player->growshrinktimer != 0)
		{
			K_SpawnGrowShrinkParticles(player->mo, player->growshrinktimer);
		}

		if (gametype == GT_RACE && player->rings <= 0) // spawn ring debt indicator
		{
			mobj_t *debtflag = P_SpawnMobj(player->mo->x + player->mo->momx, player->mo->y + player->mo->momy,
				player->mo->z + P_GetMobjZMovement(player->mo) + player->mo->height + (24*player->mo->scale), MT_THOK);

			P_SetMobjState(debtflag, S_RINGDEBT);
			P_SetScale(debtflag, (debtflag->destscale = player->mo->scale));

			K_MatchGenericExtraFlags(debtflag, player->mo);
			debtflag->frame += (leveltime % 4);

			if ((leveltime/12) & 1)
				debtflag->frame += 4;

			debtflag->color = player->skincolor;
			debtflag->fuse = 2;

			debtflag->renderflags = K_GetPlayerDontDrawFlag(player);
		}

		if (player->springstars && (leveltime & 1))
		{
			fixed_t randx = P_RandomRange(PR_DECORATION, -40, 40) * player->mo->scale;
			fixed_t randy = P_RandomRange(PR_DECORATION, -40, 40) * player->mo->scale;
			fixed_t randz = P_RandomRange(PR_DECORATION, 0, player->mo->height >> FRACBITS) << FRACBITS;
			mobj_t *star = P_SpawnMobj(
				player->mo->x + randx,
				player->mo->y + randy,
				player->mo->z + randz,
				MT_KARMAFIREWORK);

			star->color = player->springcolor;
			star->flags |= MF_NOGRAVITY;
			star->momx = player->mo->momx / 2;
			star->momy = player->mo->momy / 2;
			star->momz = P_GetMobjZMovement(player->mo) / 2;
			star->fuse = 12;
			star->scale = player->mo->scale;
			star->destscale = star->scale / 2;

			player->springstars--;
		}
	}

	if (player->itemtype == KITEM_NONE)
		player->pflags &= ~PF_HOLDREADY;

	// DKR style camera for boosting
	if (player->karthud[khud_boostcam] != 0 || player->karthud[khud_destboostcam] != 0)
	{
		if (player->karthud[khud_boostcam] < player->karthud[khud_destboostcam]
			&& player->karthud[khud_destboostcam] != 0)
		{
			player->karthud[khud_boostcam] += FRACUNIT/(TICRATE/4);
			if (player->karthud[khud_boostcam] >= player->karthud[khud_destboostcam])
				player->karthud[khud_destboostcam] = 0;
		}
		else
		{
			player->karthud[khud_boostcam] -= FRACUNIT/TICRATE;
			if (player->karthud[khud_boostcam] < player->karthud[khud_destboostcam])
				player->karthud[khud_boostcam] = player->karthud[khud_destboostcam] = 0;
		}
		//CONS_Printf("cam: %d, dest: %d\n", player->karthud[khud_boostcam], player->karthud[khud_destboostcam]);
	}

	player->karthud[khud_timeovercam] = 0;

	// Make ABSOLUTELY SURE that your flashing tics don't get set WHILE you're still in hit animations.
	if (player->spinouttimer != 0 || player->wipeoutslow != 0)
	{
		if (( player->spinouttype & KSPIN_IFRAMES ) == 0)
			player->flashing = 0;
		else
			player->flashing = K_GetKartFlashing(player);
	}

	if (player->spinouttimer)
	{
		if ((P_IsObjectOnGround(player->mo)
			|| ( player->spinouttype & KSPIN_AIRTIMER ))
			&& (!player->sneakertimer))
		{
			player->spinouttimer--;
			if (player->wipeoutslow > 1)
				player->wipeoutslow--;
		}
	}
	else
	{
		if (player->wipeoutslow >= 1)
			player->mo->friction = ORIG_FRICTION;
		player->wipeoutslow = 0;
	}

	if (player->rings > 20)
		player->rings = 20;
	else if (player->rings < -20)
		player->rings = -20;

	if (player->spheres > 40)
		player->spheres = 40;
	// where's the < 0 check? see below the following block!

	if ((gametyperules & GTR_BUMPERS) && (player->bumpers <= 0))
	{
		// Deplete 1 every tic when removed from the game.
		player->spheres--;
		player->spheredigestion = 0;
	}
	else
	{
		tic_t spheredigestion = TICRATE; // Base rate of 1 every second when playing.
		tic_t digestionpower = ((10 - player->kartspeed) + (10 - player->kartweight))-1; // 1 to 17

		// currently 0-34
		digestionpower = ((player->spheres*digestionpower)/20);

		if (digestionpower >= spheredigestion)
		{
			spheredigestion = 1;
		}
		else
		{
			spheredigestion -= digestionpower;
		}

		if ((player->spheres > 0) && (player->spheredigestion > 0))
		{
			// If you got a massive boost in spheres, catch up digestion as necessary.
			if (spheredigestion < player->spheredigestion)
			{
				player->spheredigestion = (spheredigestion + player->spheredigestion)/2;
			}

			player->spheredigestion--;

			if (player->spheredigestion == 0)
			{
				player->spheres--;
				player->spheredigestion = spheredigestion;
			}
		}
		else
		{
			player->spheredigestion = spheredigestion;
		}
	}

	// where's the > 40 check? see above the previous block!
	if (player->spheres < 0)
		player->spheres = 0;

	if (comeback == false || !(gametyperules & GTR_KARMA) || (player->pflags & PF_ELIMINATED))
	{
		player->karmadelay = comebacktime;
	}
	else if (player->karmadelay > 0 && !P_PlayerInPain(player))
	{
		player->karmadelay--;
		if (P_IsDisplayPlayer(player) && player->bumpers <= 0 && player->karmadelay <= 0)
			comebackshowninfo = true; // client has already seen the message
	}

	if (player->shrinkLaserDelay)
		player->shrinkLaserDelay--;

	if (player->ringdelay)
		player->ringdelay--;

	if (P_PlayerInPain(player))
		player->ringboost = 0;
	else if (player->ringboost)
		player->ringboost--;

	if (player->sneakertimer)
	{
		player->sneakertimer--;

		if (player->sneakertimer <= 0)
		{
			player->numsneakers = 0;
		}
	}

	if (player->trickboost)
		player->trickboost--;

	if (player->flamedash)
		player->flamedash--;

	if (player->sneakertimer && player->wipeoutslow > 0 && player->wipeoutslow < wipeoutslowtime+1)
		player->wipeoutslow = wipeoutslowtime+1;

	if (player->floorboost > 0)
		player->floorboost--;

	if (player->driftboost)
		player->driftboost--;

	if (player->strongdriftboost)
		player->strongdriftboost--;

	if (player->gateBoost)
		player->gateBoost--;

	if (player->startboost > 0 && onground == true)
	{
		player->startboost--;
	}

	if (player->spindashboost)
	{
		player->spindashboost--;

		if (player->spindashboost <= 0)
		{
			player->spindashspeed = player->spindashboost = 0;
		}
	}

	if (player->invincibilitytimer)
		player->invincibilitytimer--;

	if ((player->respawn.state == RESPAWNST_NONE) && player->growshrinktimer != 0)
	{
		if (player->growshrinktimer > 0)
			player->growshrinktimer--;
		if (player->growshrinktimer < 0)
			player->growshrinktimer++;

		// Back to normal
		if (player->growshrinktimer == 0)
			K_RemoveGrowShrink(player);
	}

	if (player->superring)
	{
		if (player->superring % 3 == 0)
		{
			mobj_t *ring = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_RING);
			ring->extravalue1 = 1; // Ring collect animation timer
			P_InitAngle(ring, player->mo->angle); // animation angle
			P_SetTarget(&ring->target, player->mo); // toucher for thinker
			player->pickuprings++;
			if (player->superring <= 3)
				ring->cvmem = 1; // play caching when collected
		}
		player->superring--;
	}

	if (player->stealingtimer == 0
		&& player->rocketsneakertimer)
		player->rocketsneakertimer--;

	if (player->hyudorotimer)
		player->hyudorotimer--;

	if (player->sadtimer)
		player->sadtimer--;

	if (player->stealingtimer > 0)
		player->stealingtimer--;
	else if (player->stealingtimer < 0)
		player->stealingtimer++;

	if (player->justbumped > 0)
		player->justbumped--;

	if (player->tiregrease)
		player->tiregrease--;

	if (player->tumbleBounces > 0)
	{
		K_HandleTumbleSound(player);
		if (P_IsObjectOnGround(player->mo) && player->mo->momz * P_MobjFlip(player->mo) <= 0)
			K_HandleTumbleBounce(player);
	}

	K_UpdateTripwire(player);

	K_KartPlayerHUDUpdate(player);

	if (battleovertime.enabled && !(player->pflags & PF_ELIMINATED) && player->bumpers <= 0 && player->karmadelay <= 0)
	{
		if (player->overtimekarma)
			player->overtimekarma--;
		else
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_TIMEOVER);
	}

	if ((battleovertime.enabled >= 10*TICRATE) && !(player->pflags & PF_ELIMINATED) && !player->exiting)
	{
		fixed_t distanceToBarrier = 0;

		if (battleovertime.radius > 0)
		{
			distanceToBarrier = R_PointToDist2(player->mo->x, player->mo->y, battleovertime.x, battleovertime.y) - (player->mo->radius * 2);
		}

		if (distanceToBarrier > battleovertime.radius)
		{
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_TIMEOVER);
		}
	}

	if (player->instashield)
		player->instashield--;

	if (player->justDI)
	{
		player->justDI--;

		// return turning if player is fully actionable, no matter when!
		if (!P_PlayerInPain(player))
			player->justDI = 0;
	}

	if (player->eggmanexplode)
	{
		if (player->spectator || (gametype == GT_BATTLE && !player->bumpers))
			player->eggmanexplode = 0;
		else
		{
			player->eggmanexplode--;
			if (player->eggmanexplode <= 0)
			{
				mobj_t *eggsexplode;

				K_KartResetPlayerColor(player);

				//player->flashing = 0;
				eggsexplode = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SPBEXPLOSION);
				if (player->eggmanblame >= 0
				&& player->eggmanblame < MAXPLAYERS
				&& playeringame[player->eggmanblame]
				&& !players[player->eggmanblame].spectator
				&& players[player->eggmanblame].mo)
					P_SetTarget(&eggsexplode->target, players[player->eggmanblame].mo);
			}
		}
	}

	if (player->itemtype == KITEM_BUBBLESHIELD)
	{
		if (player->bubblecool)
			player->bubblecool--;
	}
	else
	{
		player->bubbleblowup = 0;
		player->bubblecool = 0;
	}

	if (player->itemtype != KITEM_FLAMESHIELD)
	{
		if (player->flamedash)
			K_FlameDashLeftoverSmoke(player->mo);
	}

	if (P_IsObjectOnGround(player->mo) && player->trickpanel != 0)
	{
		if (P_MobjFlip(player->mo) * player->mo->momz <= 0)
		{
			player->trickpanel = 0;
		}
	}

	if (cmd->buttons & BT_DRIFT)
	{
		// Only allow drifting while NOT trying to do an spindash input.
		if ((K_GetKartButtons(player) & BT_EBRAKEMASK) != BT_EBRAKEMASK)
		{
			player->pflags |= PF_DRIFTINPUT;
		}
		// else, keep the previous value, because it might be brake-drifting.
	}
	else
	{
		player->pflags &= ~PF_DRIFTINPUT;
	}

	// Roulette Code
	K_KartItemRoulette(player, cmd);

	// Handle invincibility sfx
	K_UpdateInvincibilitySounds(player); // Also thanks, VAda!

	if (player->tripwireState != TRIPSTATE_NONE)
	{
		if (player->tripwireState == TRIPSTATE_PASSED)
			S_StartSound(player->mo, sfx_cdfm63);
		else if (player->tripwireState == TRIPSTATE_BLOCKED)
			S_StartSound(player->mo, sfx_kc4c);

		player->tripwireState = TRIPSTATE_NONE;
	}

	K_KartEbrakeVisuals(player);

	if (K_GetKartButtons(player) & BT_BRAKE &&
			P_IsObjectOnGround(player->mo) &&
			K_GetKartSpeed(player, false, false) / 2 <= player->speed)
	{
		K_SpawnBrakeVisuals(player);
	}
	else
	{
		player->mo->spriteyoffset = 0;
	}

	K_HandleDelayedHitByEm(player);
}

void K_KartResetPlayerColor(player_t *player)
{
	boolean fullbright = false;

	if (!player->mo || P_MobjWasRemoved(player->mo)) // Can't do anything
		return;

	if (player->mo->health <= 0 || player->playerstate == PST_DEAD || (player->respawn.state == RESPAWNST_MOVE)) // Override everything
	{
		player->mo->colorized = (player->dye != 0);
		player->mo->color = player->dye ? player->dye : player->skincolor;
		goto finalise;
	}

	if (player->eggmanexplode) // You're gonna diiiiie
	{
		const INT32 flashtime = 4<<(player->eggmanexplode/TICRATE);
		if (player->eggmanexplode % (flashtime/2) != 0)
		{
			;
		}
		else if (player->eggmanexplode % flashtime == 0)
		{
			player->mo->colorized = true;
			player->mo->color = SKINCOLOR_BLACK;
			fullbright = true;
			goto finalise;
		}
		else
		{
			player->mo->colorized = true;
			player->mo->color = SKINCOLOR_CRIMSON;
			fullbright = true;
			goto finalise;
		}
	}

	if (player->invincibilitytimer) // You're gonna kiiiiill
	{
		const tic_t defaultTime = itemtime+(2*TICRATE);
		tic_t flicker = 2;
		boolean skip = false;

		fullbright = true;

		if (player->invincibilitytimer > defaultTime)
		{
			player->mo->color = K_RainbowColor(leveltime / 2);
			player->mo->colorized = true;
			skip = true;
		}
		else
		{
			flicker += (defaultTime - player->invincibilitytimer) / TICRATE / 2;
		}

		if (leveltime % flicker == 0)
		{
			player->mo->color = SKINCOLOR_INVINCFLASH;
			player->mo->colorized = true;
			skip = true;
		}

		if (skip)
		{
			goto finalise;
		}
	}

	if (player->growshrinktimer) // Ditto, for grow/shrink
	{
		if (player->growshrinktimer % 5 == 0)
		{
			player->mo->colorized = true;
			player->mo->color = (player->growshrinktimer < 0 ? SKINCOLOR_CREAMSICLE : SKINCOLOR_PERIWINKLE);
			fullbright = true;
			goto finalise;
		}
	}

	if (player->ringboost && (leveltime & 1)) // ring boosting
	{
		player->mo->colorized = true;
		fullbright = true;
		goto finalise;
	}
	else
	{
		player->mo->colorized = (player->dye != 0);
		player->mo->color = player->dye ? player->dye : player->skincolor;
	}

finalise:

	if (player->curshield)
	{
		fullbright = true;
	}

	if (fullbright == true)
	{
		player->mo->frame |= FF_FULLBRIGHT;
	}
	else
	{
		if (!(player->mo->state->frame & FF_FULLBRIGHT))
			player->mo->frame &= ~FF_FULLBRIGHT;
	}
}

void K_KartPlayerAfterThink(player_t *player)
{
	K_KartResetPlayerColor(player);

	K_UpdateStumbleIndicator(player);

	// Move held objects (Bananas, Orbinaut, etc)
	K_MoveHeldObjects(player);

	// Jawz reticule (seeking)
	if (player->itemtype == KITEM_JAWZ && (player->pflags & PF_ITEMOUT))
	{
		INT32 lastTargID = player->lastjawztarget;
		player_t *lastTarg = NULL;
		player_t *targ = NULL;
		mobj_t *ret = NULL;

		if ((lastTargID >= 0 && lastTargID <= MAXPLAYERS)
			&& playeringame[lastTargID] == true)
		{
			if (players[lastTargID].spectator == false)
			{
				lastTarg = &players[lastTargID];
			}
		}

		if (player->throwdir == -1)
		{
			// Backwards Jawz targets yourself.
			targ = player;
			player->jawztargetdelay = 0;
		}
		else
		{
			// Find a new target.
			targ = K_FindJawzTarget(player->mo, player, ANGLE_45);
		}

		if (targ != NULL && targ->mo != NULL && P_MobjWasRemoved(targ->mo) == false)
		{
			if (targ - players == lastTargID)
			{
				// Increment delay.
				if (player->jawztargetdelay < 10)
				{
					player->jawztargetdelay++;
				}
			}
			else
			{
				if (player->jawztargetdelay > 0)
				{
					// Wait a bit before swapping...
					player->jawztargetdelay--;
					targ = lastTarg;
				}
				else
				{
					// Allow a swap.
					if (P_IsDisplayPlayer(player) || P_IsDisplayPlayer(targ))
					{
						S_StartSound(NULL, sfx_s3k89);
					}
					else
					{
						S_StartSound(targ->mo, sfx_s3k89);
					}

					player->lastjawztarget = targ - players;
					player->jawztargetdelay = 5;
				}
			}
		}

		if (targ == NULL || targ->mo == NULL || P_MobjWasRemoved(targ->mo) == true)
		{
			player->lastjawztarget = -1;
			player->jawztargetdelay = 0;
			return;
		}

		ret = P_SpawnMobj(targ->mo->x, targ->mo->y, targ->mo->z, MT_PLAYERRETICULE);
		ret->old_x = targ->mo->old_x;
		ret->old_y = targ->mo->old_y;
		ret->old_z = targ->mo->old_z;
		P_SetTarget(&ret->target, targ->mo);
		ret->frame |= ((leveltime % 10) / 2);
		ret->tics = 1;
		ret->color = player->skincolor;
	}
	else
	{
		player->lastjawztarget = -1;
		player->jawztargetdelay = 0;
	}

	if (player->itemtype == KITEM_LIGHTNINGSHIELD)
	{
		K_LookForRings(player->mo);
	}
}

/*--------------------------------------------------
	static waypoint_t *K_GetPlayerNextWaypoint(player_t *player)

		Gets the next waypoint of a player, by finding their closest waypoint, then checking which of itself and next or
		previous waypoints are infront of the player.

	Input Arguments:-
		player - The player the next waypoint is being found for

	Return:-
		The waypoint that is the player's next waypoint
--------------------------------------------------*/
static waypoint_t *K_GetPlayerNextWaypoint(player_t *player)
{
	waypoint_t *bestwaypoint = NULL;

	if ((player != NULL) && (player->mo != NULL) && (P_MobjWasRemoved(player->mo) == false))
	{
		waypoint_t *waypoint     = K_GetBestWaypointForMobj(player->mo);
		boolean    updaterespawn = false;

		bestwaypoint = waypoint;

		// check the waypoint's location in relation to the player
		// If it's generally in front, it's fine, otherwise, use the best next/previous waypoint.
		// EXCEPTION: If our best waypoint is the finishline AND we're facing towards it, don't do this.
		// Otherwise it breaks the distance calculations.
		if (waypoint != NULL)
		{
			boolean finishlinehack  = false;
			angle_t playerangle     = player->mo->angle;
			angle_t momangle        = K_MomentumAngle(player->mo);
			angle_t angletowaypoint =
				R_PointToAngle2(player->mo->x, player->mo->y, waypoint->mobj->x, waypoint->mobj->y);
			angle_t angledelta      = ANGLE_MAX;
			angle_t momdelta        = ANGLE_MAX;

			angledelta = playerangle - angletowaypoint;
			if (angledelta > ANGLE_180)
			{
				angledelta = InvAngle(angledelta);
			}

			momdelta = momangle - angletowaypoint;
			if (momdelta > ANGLE_180)
			{
				momdelta = InvAngle(momdelta);
			}

			if (bestwaypoint == K_GetFinishLineWaypoint())
			{
				waypoint_t *nextwaypoint = waypoint->nextwaypoints[0];
				angle_t angletonextwaypoint =
					R_PointToAngle2(waypoint->mobj->x, waypoint->mobj->y, nextwaypoint->mobj->x, nextwaypoint->mobj->y);

				// facing towards the finishline
				if (AngleDelta(angletonextwaypoint, angletowaypoint) <= ANGLE_90)
				{
					finishlinehack = true;
				}
			}

			// We're using a lot of angle calculations here, because only using facing angle or only using momentum angle both have downsides.
			// nextwaypoints will be picked if you're facing OR moving forward.
			// prevwaypoints will be picked if you're facing AND moving backward.
			if ((angledelta > ANGLE_45 || momdelta > ANGLE_45)
			&& (finishlinehack == false))
			{
				angle_t nextbestdelta = angledelta;
				angle_t nextbestmomdelta = momdelta;
				size_t i = 0U;

				if (K_PlayerUsesBotMovement(player))
				{
					// Try to force bots to use a next waypoint
					nextbestdelta = ANGLE_MAX;
					nextbestmomdelta = ANGLE_MAX;
				}

				if ((waypoint->nextwaypoints != NULL) && (waypoint->numnextwaypoints > 0U))
				{
					for (i = 0U; i < waypoint->numnextwaypoints; i++)
					{
						if (!K_GetWaypointIsEnabled(waypoint->nextwaypoints[i]))
						{
							continue;
						}

						if (K_PlayerUsesBotMovement(player) == true
						&& K_GetWaypointIsShortcut(waypoint->nextwaypoints[i]) == true
						&& K_BotCanTakeCut(player) == false)
						{
							// Bots that aren't able to take a shortcut will ignore shortcut waypoints.
							// (However, if they're already on a shortcut, then we want them to keep going.)

							if (player->nextwaypoint == NULL
							|| K_GetWaypointIsShortcut(player->nextwaypoint) == false)
							{
								continue;
							}
						}

						angletowaypoint = R_PointToAngle2(
							player->mo->x, player->mo->y,
							waypoint->nextwaypoints[i]->mobj->x, waypoint->nextwaypoints[i]->mobj->y);

						angledelta = playerangle - angletowaypoint;
						if (angledelta > ANGLE_180)
						{
							angledelta = InvAngle(angledelta);
						}

						momdelta = momangle - angletowaypoint;
						if (momdelta > ANGLE_180)
						{
							momdelta = InvAngle(momdelta);
						}

						if (angledelta < nextbestdelta || momdelta < nextbestmomdelta)
						{
							if (P_TraceBlockingLines(player->mo, waypoint->nextwaypoints[i]->mobj) == false)
							{
								// Save sight checks when all of the other checks pass, so we only do it if we have to
								continue;
							}

							bestwaypoint = waypoint->nextwaypoints[i];

							if (angledelta < nextbestdelta)
							{
								nextbestdelta = angledelta;
							}
							if (momdelta < nextbestmomdelta)
							{
								nextbestmomdelta = momdelta;
							}

							// Remove wrong way flag if we're using nextwaypoints
							player->pflags &= ~PF_WRONGWAY;
							updaterespawn = true;
						}
					}
				}

				if ((waypoint->prevwaypoints != NULL) && (waypoint->numprevwaypoints > 0U)
				&& !(K_PlayerUsesBotMovement(player))) // Bots do not need prev waypoints
				{
					for (i = 0U; i < waypoint->numprevwaypoints; i++)
					{
						if (!K_GetWaypointIsEnabled(waypoint->prevwaypoints[i]))
						{
							continue;
						}

						angletowaypoint = R_PointToAngle2(
							player->mo->x, player->mo->y,
							waypoint->prevwaypoints[i]->mobj->x, waypoint->prevwaypoints[i]->mobj->y);

						angledelta = playerangle - angletowaypoint;
						if (angledelta > ANGLE_180)
						{
							angledelta = InvAngle(angledelta);
						}

						momdelta = momangle - angletowaypoint;
						if (momdelta > ANGLE_180)
						{
							momdelta = InvAngle(momdelta);
						}

						if (angledelta < nextbestdelta && momdelta < nextbestmomdelta)
						{
							if (P_TraceBlockingLines(player->mo, waypoint->prevwaypoints[i]->mobj) == false)
							{
								// Save sight checks when all of the other checks pass, so we only do it if we have to
								continue;
							}

							bestwaypoint = waypoint->prevwaypoints[i];

							nextbestdelta = angledelta;
							nextbestmomdelta = momdelta;

							// Set wrong way flag if we're using prevwaypoints
							player->pflags |= PF_WRONGWAY;
							updaterespawn = false;
						}
					}
				}
			}
		}

		if (!P_IsObjectOnGround(player->mo))
		{
			updaterespawn = false;
		}

		// Respawn point should only be updated when we're going to a nextwaypoint
		if ((updaterespawn) &&
		(player->respawn.state == RESPAWNST_NONE) &&
		(bestwaypoint != NULL) &&
		(bestwaypoint != player->nextwaypoint) &&
		(K_GetWaypointIsSpawnpoint(bestwaypoint)) &&
		(K_GetWaypointIsEnabled(bestwaypoint) == true))
		{
			player->respawn.wp = bestwaypoint;
		}
	}

	return bestwaypoint;
}

#if 0
static boolean K_PlayerCloserToNextWaypoints(waypoint_t *const waypoint, player_t *const player)
{
	boolean nextiscloser = true;

	if ((waypoint != NULL) && (player != NULL) && (player->mo != NULL))
	{
		size_t     i                   = 0U;
		waypoint_t *currentwpcheck     = NULL;
		angle_t    angletoplayer       = ANGLE_MAX;
		angle_t    currentanglecheck   = ANGLE_MAX;
		angle_t    bestangle           = ANGLE_MAX;

		angletoplayer = R_PointToAngle2(waypoint->mobj->x, waypoint->mobj->y,
			player->mo->x, player->mo->y);

		for (i = 0U; i < waypoint->numnextwaypoints; i++)
		{
			currentwpcheck = waypoint->nextwaypoints[i];
			currentanglecheck = R_PointToAngle2(
				waypoint->mobj->x, waypoint->mobj->y, currentwpcheck->mobj->x, currentwpcheck->mobj->y);

			// Get delta angle
			currentanglecheck = currentanglecheck - angletoplayer;

			if (currentanglecheck > ANGLE_180)
			{
				currentanglecheck = InvAngle(currentanglecheck);
			}

			if (currentanglecheck < bestangle)
			{
				bestangle = currentanglecheck;
			}
		}

		for (i = 0U; i < waypoint->numprevwaypoints; i++)
		{
			currentwpcheck = waypoint->prevwaypoints[i];
			currentanglecheck = R_PointToAngle2(
				waypoint->mobj->x, waypoint->mobj->y, currentwpcheck->mobj->x, currentwpcheck->mobj->y);

			// Get delta angle
			currentanglecheck = currentanglecheck - angletoplayer;

			if (currentanglecheck > ANGLE_180)
			{
				currentanglecheck = InvAngle(currentanglecheck);
			}

			if (currentanglecheck < bestangle)
			{
				bestangle = currentanglecheck;
				nextiscloser = false;
				break;
			}
		}
	}

	return nextiscloser;
}
#endif

/*--------------------------------------------------
	void K_UpdateDistanceFromFinishLine(player_t *const player)

		Updates the distance a player has to the finish line.

	Input Arguments:-
		player - The player the distance is being updated for

	Return:-
		None
--------------------------------------------------*/
void K_UpdateDistanceFromFinishLine(player_t *const player)
{
	if ((player != NULL) && (player->mo != NULL))
	{
		waypoint_t *finishline   = K_GetFinishLineWaypoint();
		waypoint_t *nextwaypoint = NULL;

		if (player->spectator)
		{
			// Don't update waypoints while spectating
			nextwaypoint = finishline;
		}
		else
		{
			nextwaypoint = K_GetPlayerNextWaypoint(player);
		}

		if (nextwaypoint != NULL)
		{
			// If nextwaypoint is NULL, it means we don't want to update the waypoint until we touch another one.
			// player->nextwaypoint will keep its previous value in this case.
			player->nextwaypoint = nextwaypoint;
		}

		// nextwaypoint is now the waypoint that is in front of us
		if (player->exiting || player->spectator)
		{
			// Player has finished, we don't need to calculate this
			player->distancetofinish = 0U;
		}
		else if ((player->nextwaypoint != NULL) && (finishline != NULL))
		{
			const boolean useshortcuts = false;
			const boolean huntbackwards = false;
			boolean pathfindsuccess = false;
			path_t pathtofinish = {0};

			pathfindsuccess =
				K_PathfindToWaypoint(player->nextwaypoint, finishline, &pathtofinish, useshortcuts, huntbackwards);

			// Update the player's distance to the finish line if a path was found.
			// Using shortcuts won't find a path, so distance won't be updated until the player gets back on track
			if (pathfindsuccess == true)
			{
				// Add euclidean distance to the next waypoint to the distancetofinish
				UINT32 adddist;
				fixed_t disttowaypoint =
					P_AproxDistance(
						(player->mo->x >> FRACBITS) - (player->nextwaypoint->mobj->x >> FRACBITS),
						(player->mo->y >> FRACBITS) - (player->nextwaypoint->mobj->y >> FRACBITS));
				disttowaypoint = P_AproxDistance(disttowaypoint, (player->mo->z >> FRACBITS) - (player->nextwaypoint->mobj->z >> FRACBITS));

				adddist = (UINT32)disttowaypoint;

				player->distancetofinish = pathtofinish.totaldist + adddist;
				Z_Free(pathtofinish.array);

				// distancetofinish is currently a flat distance to the finish line, but in order to be fully
				// correct we need to add to it the length of the entire circuit multiplied by the number of laps
				// left after this one. This will give us the total distance to the finish line, and allow item
				// distance calculation to work easily
				if ((mapheaderinfo[gamemap - 1]->levelflags & LF_SECTIONRACE) == 0U)
				{
					const UINT8 numfulllapsleft = ((UINT8)numlaps - player->laps);

					player->distancetofinish += numfulllapsleft * K_GetCircuitLength();

#if 0
					// An additional HACK, to fix looking backwards towards the finish line
					// If the player's next waypoint is the finishline and the angle distance from player to
					// connectin waypoints implies they're closer to a next waypoint, add a full track distance
					if (player->nextwaypoint == finishline)
					{
						if (K_PlayerCloserToNextWaypoints(player->nextwaypoint, player) == true)
						{
							player->distancetofinish += K_GetCircuitLength();
						}
					}
#endif
				}
			}
		}
	}
}

INT32 K_GetKartRingPower(player_t *player, boolean boosted)
{
	INT32 ringPower = ((9 - player->kartspeed) + (9 - player->kartweight)) / 2;

	if (boosted == true && K_PlayerUsesBotMovement(player))
	{
		// Double for Lv. 9
		ringPower += (player->botvars.difficulty * ringPower) / DIFFICULTBOT;
	}

	return ringPower;
}

// Returns false if this player being placed here causes them to collide with any other player
// Used in g_game.c for match etc. respawning
// This does not check along the z because the z is not correctly set for the spawnee at this point
boolean K_CheckPlayersRespawnColliding(INT32 playernum, fixed_t x, fixed_t y)
{
	INT32 i;
	fixed_t p1radius = players[playernum].mo->radius;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playernum == i || !playeringame[i] || players[i].spectator || !players[i].mo || players[i].mo->health <= 0
			|| players[i].playerstate != PST_LIVE || (players[i].mo->flags & MF_NOCLIP) || (players[i].mo->flags & MF_NOCLIPTHING))
			continue;

		if (abs(x - players[i].mo->x) < (p1radius + players[i].mo->radius)
			&& abs(y - players[i].mo->y) < (p1radius + players[i].mo->radius))
		{
			return false;
		}
	}
	return true;
}

// countersteer is how strong the controls are telling us we are turning
// turndir is the direction the controls are telling us to turn, -1 if turning right and 1 if turning left
static INT16 K_GetKartDriftValue(player_t *player, fixed_t countersteer)
{
	INT16 basedrift, driftadjust;
	fixed_t driftweight = player->kartweight*14; // 12

	if (player->drift == 0 || !P_IsObjectOnGround(player->mo))
	{
		// If they aren't drifting or on the ground, this doesn't apply
		return 0;
	}

	if (player->pflags & PF_DRIFTEND)
	{
		// Drift has ended and we are tweaking their angle back a bit
		return -266*player->drift;
	}

	basedrift = (83 * player->drift) - (((driftweight - 14) * player->drift) / 5); // 415 - 303
	driftadjust = abs((252 - driftweight) * player->drift / 5);

	if (player->tiregrease > 0) // Buff drift-steering while in greasemode
	{
		basedrift += (basedrift / greasetics) * player->tiregrease;
	}

#if 0
	if (player->mo->eflags & MFE_UNDERWATER)
	{
		countersteer = FixedMul(countersteer, 3*FRACUNIT/2);
	}
#endif

	return basedrift + (FixedMul(driftadjust * FRACUNIT, countersteer) / FRACUNIT);
}

INT16 K_UpdateSteeringValue(INT16 inputSteering, INT16 destSteering)
{
	// player->steering is the turning value, but with easing applied.
	// Keeps micro-turning from old easing, but isn't controller dependent.

	const INT16 amount = KART_FULLTURN/3;
	INT16 diff = destSteering - inputSteering;
	INT16 outputSteering = inputSteering;

	if (abs(diff) <= amount)
	{
		// Reached the intended value, set instantly.
		outputSteering = destSteering;
	}
	else
	{
		// Go linearly towards the value we wanted.
		if (diff < 0)
		{
			outputSteering -= amount;
		}
		else
		{
			outputSteering += amount;
		}
	}

	return outputSteering;
}

INT16 K_GetKartTurnValue(player_t *player, INT16 turnvalue)
{
	fixed_t turnfixed = turnvalue * FRACUNIT;

	fixed_t currentSpeed = 0;
	fixed_t p_maxspeed = INT32_MAX, p_speed = INT32_MAX;

	fixed_t weightadjust = INT32_MAX;

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) || player->spectator || objectplacing)
	{
		// Invalid object, or incorporeal player. Return the value exactly.
		return turnvalue;
	}

	if (leveltime < introtime)
	{
		// No turning during the intro
		return 0;
	}

	if (player->respawn.state == RESPAWNST_MOVE)
	{
		// No turning during respawn
		return 0;
	}

	if (player->trickpanel != 0 && player->trickpanel < 4)
	{
		// No turning during trick panel unless you did the upwards trick (4)
		return 0;
	}

	if (player->justDI > 0)
	{
		// No turning until you let go after DI-ing.
		return 0;
	}

	currentSpeed = FixedHypot(player->mo->momx, player->mo->momy);

	if ((currentSpeed <= 0) // Not moving
	&& ((K_GetKartButtons(player) & BT_EBRAKEMASK) != BT_EBRAKEMASK) // Not e-braking
	&& (player->respawn.state == RESPAWNST_NONE) // Not respawning
	&& (P_IsObjectOnGround(player->mo) == true)) // On the ground
	{
		return 0;
	}

	p_maxspeed = K_GetKartSpeed(player, false, true);
	p_speed = min(currentSpeed, (p_maxspeed * 2));
	weightadjust = FixedDiv((p_maxspeed * 3) - p_speed, (p_maxspeed * 3) + (player->kartweight * FRACUNIT));

	if (K_PlayerUsesBotMovement(player))
	{
		turnfixed = FixedMul(turnfixed, 5*FRACUNIT/4); // Base increase to turning
	}

	if (player->drift != 0 && P_IsObjectOnGround(player->mo))
	{
		fixed_t countersteer = FixedDiv(turnfixed, KART_FULLTURN*FRACUNIT);

		// If we're drifting we have a completely different turning value

		if (player->pflags & PF_DRIFTEND)
		{
			countersteer = FRACUNIT;
		}

		return K_GetKartDriftValue(player, countersteer);
	}

	if (player->handleboost > 0)
	{
		turnfixed = FixedMul(turnfixed, FRACUNIT + player->handleboost);
	}

	if ((player->mo->eflags & MFE_UNDERWATER) &&
			player->speed > 11 * player->mo->scale)
	{
		turnfixed /= 2;
	}

	// Weight has a small effect on turning
	turnfixed = FixedMul(turnfixed, weightadjust);

	return (turnfixed / FRACUNIT);
}

INT32 K_GetUnderwaterTurnAdjust(player_t *player)
{
	if ((player->mo->eflags & MFE_UNDERWATER) &&
			player->speed > 11 * player->mo->scale)
	{
		INT32 steer = (K_GetKartTurnValue(player,
					player->steering) << TICCMD_REDUCE);
		fixed_t minimum = INT32_MAX;

		if (!player->drift)
			steer = 9 * steer / 5;

		minimum = 2 * K_GetKartSpeed(player, false, true) / 3;
		return FixedMul(steer, 8 * FixedDiv(max(player->speed, minimum), minimum));
	}
	else
		return 0;
}

INT32 K_GetKartDriftSparkValue(player_t *player)
{
	return (26*4 + player->kartspeed*2 + (9 - player->kartweight))*8;
}

INT32 K_GetKartDriftSparkValueForStage(player_t *player, UINT8 stage)
{
	fixed_t mul = FRACUNIT;

	// This code is function is pretty much useless now that the timing changes are linear but bleh.
	switch (stage)
	{
		case 2:
			mul = 2*FRACUNIT; // x2
			break;
		case 3:
			mul = 3*FRACUNIT; // x3
			break;
		case 4:
			mul = 4*FRACUNIT; // x4
			break;
	}

	return (FixedMul(K_GetKartDriftSparkValue(player) * FRACUNIT, mul) / FRACUNIT);
}

/*
Stage 1: yellow sparks
Stage 2: red sparks
Stage 3: blue sparks
Stage 4: big large rainbow sparks
Stage 0: air failsafe
*/
void K_SpawnDriftBoostExplosion(player_t *player, int stage)
{
	mobj_t *overlay = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_DRIFTEXPLODE);

	P_InitAngle(overlay, K_MomentumAngle(player->mo));
	P_SetTarget(&overlay->target, player->mo);
	P_SetScale(overlay, (overlay->destscale = player->mo->scale));
	K_FlipFromObject(overlay, player->mo);

	switch (stage)
	{
		case 1:
			overlay->fuse = 16;
			break;

		case 2:
			
			overlay->fuse = 32;
			S_StartSound(player->mo, sfx_kc5b);
			break;

		case 3:
			overlay->fuse = 48;

			S_StartSound(player->mo, sfx_kc5b);
			break;

		case 4:
			overlay->fuse = 120;

			S_StartSound(player->mo, sfx_kc5b);
			S_StartSound(player->mo, sfx_s3kc4l);
			break;

		case 0:
			overlay->fuse = 16;
			break;
	}

	overlay->extravalue1 = stage;
}

static void K_KartDrift(player_t *player, boolean onground)
{
	const fixed_t minspeed = (10 * player->mo->scale);

	const INT32 dsone = K_GetKartDriftSparkValueForStage(player, 1);
	const INT32 dstwo = K_GetKartDriftSparkValueForStage(player, 2);
	const INT32 dsthree = K_GetKartDriftSparkValueForStage(player, 3);
	const INT32 dsfour = K_GetKartDriftSparkValueForStage(player, 4);

	const UINT16 buttons = K_GetKartButtons(player);

	// Drifting is actually straffing + automatic turning.
	// Holding the Jump button will enable drifting.
	// (This comment is extremely funny)

	// Drift Release (Moved here so you can't "chain" drifts)
	if (player->drift != -5 && player->drift != 5)
	{
		if (player->driftcharge < 0 || player->driftcharge >= dsone)
		{
			angle_t pushdir = K_MomentumAngle(player->mo);

			S_StartSound(player->mo, sfx_s23c);
			//K_SpawnDashDustRelease(player);

			if (player->driftcharge < 0)
			{
				// Stage 0: Grey sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 8);

				if (player->driftboost < 15)
					player->driftboost = 15;
			}
			else if (player->driftcharge >= dsone && player->driftcharge < dstwo)
			{
				// Stage 1: Yellow sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 4);

				if (player->driftboost < 20)
					player->driftboost = 20;

				K_SpawnDriftBoostExplosion(player, 1);
			}
			else if (player->driftcharge < dsthree)
			{
				// Stage 2: Red sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 3);

				if (player->driftboost < 50)
					player->driftboost = 50;

				K_SpawnDriftBoostExplosion(player, 2);
			}
			else if (player->driftcharge < dsfour)
			{
				// Stage 3: Blue sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, ( 5 * player->speed ) / 12);

				if (player->driftboost < 85)
					player->driftboost = 85;
				if (player->strongdriftboost < 85)
					player->strongdriftboost = 85;

				K_SpawnDriftBoostExplosion(player, 3);
				K_SpawnDriftElectricSparks(player, K_DriftSparkColor(player, player->driftcharge), false);
			}
			else if (player->driftcharge >= dsfour)
			{
				// Stage 4: Rainbow sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 2);

				if (player->driftboost < 125)
					player->driftboost = 125;
				if (player->strongdriftboost < 125)
					player->strongdriftboost = 125;

				K_SpawnDriftBoostExplosion(player, 4);
				K_SpawnDriftElectricSparks(player, K_DriftSparkColor(player, player->driftcharge), false);
			}
		}

		// Remove charge
		player->driftcharge = 0;
	}

	// Drifting: left or right?
	if (!(player->pflags & PF_DRIFTINPUT))
	{
		// drift is not being performed so if we're just finishing set driftend and decrement counters
		if (player->drift > 0)
		{
			player->drift--;
			player->pflags |= PF_DRIFTEND;
		}
		else if (player->drift < 0)
		{
			player->drift++;
			player->pflags |= PF_DRIFTEND;
		}
		else
			player->pflags &= ~PF_DRIFTEND;
	}
	else if (player->speed > minspeed
		&& (player->drift == 0 || (player->pflags & PF_DRIFTEND)))
	{
		// Uses turning over steering, since this is very binary.
		// Using steering would cause a lot more "wrong drifts".
		if (player->cmd.turning > 0)
		{
			// Starting left drift
			player->drift = 1;
			player->driftcharge = 0;
			player->pflags &= ~PF_DRIFTEND;
		}
		else if (player->cmd.turning < 0)
		{
			// Starting right drift
			player->drift = -1;
			player->driftcharge = 0;
			player->pflags &= ~PF_DRIFTEND;
		}
	}

	if (P_PlayerInPain(player) || player->speed <= 0)
	{
		// Stop drifting
		player->drift = player->driftcharge = player->aizdriftstrat = 0;
		player->pflags &= ~(PF_BRAKEDRIFT|PF_GETSPARKS);
	}
	else if ((player->pflags & PF_DRIFTINPUT) && player->drift != 0)
	{
		// Incease/decrease the drift value to continue drifting in that direction
		fixed_t driftadditive = 24;
		boolean playsound = false;

		if (onground)
		{
			if (player->drift >= 1) // Drifting to the left
			{
				player->drift++;
				if (player->drift > 5)
					player->drift = 5;

				if (player->steering > 0) // Inward
					driftadditive += abs(player->steering)/100;
				if (player->steering < 0) // Outward
					driftadditive -= abs(player->steering)/75;
			}
			else if (player->drift <= -1) // Drifting to the right
			{
				player->drift--;
				if (player->drift < -5)
					player->drift = -5;

				if (player->steering < 0) // Inward
					driftadditive += abs(player->steering)/100;
				if (player->steering > 0) // Outward
					driftadditive -= abs(player->steering)/75;
			}

			// Disable drift-sparks until you're going fast enough
			if (!(player->pflags & PF_GETSPARKS)
				|| (player->offroad && K_ApplyOffroad(player)))
				driftadditive = 0;

			// Inbetween minspeed and minspeed*2, it'll keep your previous drift-spark state.
			if (player->speed > minspeed*2)
			{
				player->pflags |= PF_GETSPARKS;

				if (player->driftcharge <= -1)
				{
					player->driftcharge = dsone; // Back to red
					playsound = true;
				}
			}
			else if (player->speed <= minspeed)
			{
				player->pflags &= ~PF_GETSPARKS;
				driftadditive = 0;

				if (player->driftcharge >= dsone)
				{
					player->driftcharge = -1; // Set yellow sparks
					playsound = true;
				}
			}
		}
		else
		{
			driftadditive = 0;
		}

		// This spawns the drift sparks
		if ((player->driftcharge + driftadditive >= dsone)
			|| (player->driftcharge < 0))
		{
			K_SpawnDriftSparks(player);
		}

		if ((player->driftcharge < dsone && player->driftcharge+driftadditive >= dsone)
			|| (player->driftcharge < dstwo && player->driftcharge+driftadditive >= dstwo)
			|| (player->driftcharge < dsthree && player->driftcharge+driftadditive >= dsthree))
		{
			playsound = true;
		}

		// Sound whenever you get a different tier of sparks
		if (playsound && P_IsDisplayPlayer(player))
		{
			if (player->driftcharge == -1)
				S_StartSoundAtVolume(player->mo, sfx_sploss, 192); // Yellow spark sound
			else
				S_StartSoundAtVolume(player->mo, sfx_s3ka2, 192);
		}

		player->driftcharge += driftadditive;
		player->pflags &= ~PF_DRIFTEND;
	}

	if ((player->handleboost == 0)
	|| (!player->steering)
	|| (!player->aizdriftstrat)
	|| (player->steering > 0) != (player->aizdriftstrat > 0))
	{
		if (!player->drift)
			player->aizdriftstrat = 0;
		else
			player->aizdriftstrat = ((player->drift > 0) ? 1 : -1);
	}
	else if (player->aizdriftstrat && !player->drift)
	{
		K_SpawnAIZDust(player);

		if (abs(player->aizdrifttilt) < ANGLE_22h)
		{
			player->aizdrifttilt =
				(abs(player->aizdrifttilt) + ANGLE_11hh / 4) *
				player->aizdriftstrat;
		}

		if (abs(player->aizdriftturn) < ANGLE_112h)
		{
			player->aizdriftturn =
				(abs(player->aizdriftturn) + ANGLE_11hh) *
				player->aizdriftstrat;
		}
	}

	if (!K_Sliptiding(player))
	{
		player->aizdrifttilt -= player->aizdrifttilt / 4;
		player->aizdriftturn -= player->aizdriftturn / 4;

		if (abs(player->aizdrifttilt) < ANGLE_11hh / 4)
			player->aizdrifttilt = 0;
		if (abs(player->aizdriftturn) < ANGLE_11hh)
			player->aizdriftturn = 0;
	}

	if (player->drift
		&& ((buttons & BT_BRAKE)
		|| !(buttons & BT_ACCELERATE))
		&& P_IsObjectOnGround(player->mo))
	{
		if (!(player->pflags & PF_BRAKEDRIFT))
			K_SpawnBrakeDriftSparks(player);
		player->pflags |= PF_BRAKEDRIFT;
	}
	else
		player->pflags &= ~PF_BRAKEDRIFT;
}
//
// K_KartUpdatePosition
//
void K_KartUpdatePosition(player_t *player)
{
	fixed_t position = 1;
	fixed_t oldposition = player->position;
	fixed_t i;

	if (player->spectator || !player->mo)
	{
		// Ensure these are reset for spectators
		player->position = 0;
		player->positiondelay = 0;
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || !players[i].mo)
			continue;

		if (gametyperules & GTR_CIRCUIT)
		{
			if (player->exiting) // End of match standings
			{
				// Only time matters
				if (players[i].realtime < player->realtime)
					position++;
			}
			else
			{
				// I'm a lap behind this player OR
				// My distance to the finish line is higher, so I'm behind
				if ((players[i].laps > player->laps)
					|| (players[i].distancetofinish < player->distancetofinish))
				{
					position++;
				}
			}
		}
		else
		{
			if (player->exiting) // End of match standings
			{
				// Only score matters
				if (players[i].roundscore > player->roundscore)
					position++;
			}
			else
			{
				UINT8 myEmeralds = K_NumEmeralds(player);
				UINT8 yourEmeralds = K_NumEmeralds(&players[i]);

				if (yourEmeralds > myEmeralds)
				{
					// Emeralds matter above all
					position++;
				}
				else if (yourEmeralds == myEmeralds)
				{
					// Bumpers are a tie breaker
					if (players[i].bumpers > player->bumpers)
					{
						position++;
					}
					else if (players[i].bumpers == player->bumpers)
					{
						// Score is the second tier tie breaker
						if (players[i].roundscore > player->roundscore)
						{
							position++;
						}
					}
				}
			}
		}
	}

	if (leveltime < starttime || oldposition == 0)
		oldposition = position;

	if (oldposition != position) // Changed places?
		player->positiondelay = 10; // Position number growth

	player->position = position;
}

//
// K_StripItems
//
void K_StripItems(player_t *player)
{
	K_DropRocketSneaker(player);
	K_DropKitchenSink(player);
	player->itemtype = KITEM_NONE;
	player->itemamount = 0;
	player->pflags &= ~(PF_ITEMOUT|PF_EGGMANOUT);

	if (!player->itemroulette || player->roulettetype != 2)
	{
		player->itemroulette = 0;
		player->roulettetype = 0;
	}

	player->hyudorotimer = 0;
	player->stealingtimer = 0;

	player->curshield = KSHIELD_NONE;
	player->bananadrag = 0;
	player->ballhogcharge = 0;
	player->sadtimer = 0;

	K_UpdateHnextList(player, true);
}

void K_StripOther(player_t *player)
{
	player->itemroulette = 0;
	player->roulettetype = 0;

	player->invincibilitytimer = 0;
	if (player->growshrinktimer)
	{
		K_RemoveGrowShrink(player);
	}

	if (player->eggmanexplode)
	{
		player->eggmanexplode = 0;
		player->eggmanblame = -1;

		K_KartResetPlayerColor(player);
	}
}

static INT32 K_FlameShieldMax(player_t *player)
{
	UINT32 disttofinish = 0;
	UINT32 distv = DISTVAR;
	UINT8 numplayers = 0;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
			numplayers++;
		if (players[i].position == 1)
			disttofinish = players[i].distancetofinish;
	}

	if (numplayers <= 1 || gametype == GT_BATTLE)
	{
		return 16; // max when alone, for testing
		// and when in battle, for chaos
	}
	else if (player->position == 1)
	{
		return 0; // minimum for first
	}

	disttofinish = player->distancetofinish - disttofinish;
	distv = FixedMul(distv, mapobjectscale);
	return min(16, 1 + (disttofinish / distv));
}

boolean K_PlayerEBrake(player_t *player)
{
	if (player->fastfall != 0)
	{
		return true;
	}

	return (K_GetKartButtons(player) & BT_EBRAKEMASK) == BT_EBRAKEMASK
		&& player->drift == 0
		&& P_PlayerInPain(player) == false
		&& player->justbumped == 0
		&& player->spindashboost == 0
		&& player->nocontrol == 0;
}

SINT8 K_Sliptiding(player_t *player)
{
	return player->drift ? 0 : player->aizdriftstrat;
}

INT32 K_StairJankFlip(INT32 value)
{
	return P_AltFlip(value, 2);
}

// Ebraking visuals for mo
// we use mo->hprev for the hold bubble. If another hprev exists for some reason, remove it.

void K_KartEbrakeVisuals(player_t *p)
{
	mobj_t *wave;
	mobj_t *spdl;
	fixed_t sx, sy;

	if (K_PlayerEBrake(p) == true)
	{
		if (p->ebrakefor % 20 == 0)
		{
			wave = P_SpawnMobj(p->mo->x, p->mo->y, p->mo->z, MT_SOFTLANDING);
			P_SetScale(wave, p->mo->scale);
			wave->momx = p->mo->momx;
			wave->momy = p->mo->momy;
			wave->momz = p->mo->momz;
			wave->standingslope = p->mo->standingslope;
		}

		// sound
		if (!S_SoundPlaying(p->mo, sfx_s3kd9s))
			S_StartSound(p->mo, sfx_s3kd9s);

		// HOLD! bubble.
		if (!p->ebrakefor)
		{
			if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev))
			{
				// for some reason, there's already an hprev. Remove it.
				P_RemoveMobj(p->mo->hprev);
			}

			p->mo->hprev = P_SpawnMobj(p->mo->x, p->mo->y, p->mo->z, MT_HOLDBUBBLE);
			p->mo->hprev->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(p));
		}

		// Update HOLD bubble.
		if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev))
		{
			P_MoveOrigin(p->mo->hprev, p->mo->x, p->mo->y, p->mo->z);
			p->mo->hprev->angle = p->mo->angle;
			p->mo->hprev->fuse = TICRATE/2;		// When we leave spindash for any reason, make sure this bubble goes away soon after.
			K_FlipFromObject(p->mo->hprev, p->mo);
		}

		if (!p->spindash)
		{
			// Spawn downwards fastline
			sx = p->mo->x + P_RandomRange(PR_DECORATION, -48, 48)*p->mo->scale;
			sy = p->mo->y + P_RandomRange(PR_DECORATION, -48, 48)*p->mo->scale;

			spdl = P_SpawnMobj(sx, sy, p->mo->z, MT_DOWNLINE);
			spdl->colorized = true;
			spdl->color = SKINCOLOR_WHITE;
			K_MatchGenericExtraFlags(spdl, p->mo);
			P_SetScale(spdl, p->mo->scale);

			// squish the player a little bit.
			p->mo->spritexscale = FRACUNIT*115/100;
			p->mo->spriteyscale = FRACUNIT*85/100;
		}
		else
		{
			const UINT16 MAXCHARGETIME = K_GetSpindashChargeTime(p);
			const fixed_t MAXSHAKE = FRACUNIT;

			// update HOLD bubble with numbers based on charge.
			if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev))
			{
				UINT8 frame = min(1 + ((p->spindash*3) / MAXCHARGETIME), 4);

				// ?! limit.
				if (p->spindash >= MAXCHARGETIME +TICRATE)
					frame = 5;

				p->mo->hprev->frame = frame|FF_FULLBRIGHT;
			}

			// shake the player as they charge their spindash!

			// "gentle" shaking as we start...
			if (p->spindash < MAXCHARGETIME)
			{
				fixed_t shake = FixedMul(((p->spindash)*FRACUNIT/MAXCHARGETIME), MAXSHAKE);
				SINT8 mult = leveltime & 1 ? 1 : -1;

				p->mo->spritexoffset = shake*mult;
			}
			else	// get VIOLENT on overcharge :)
			{
				fixed_t shake = MAXSHAKE + FixedMul(((p->spindash-MAXCHARGETIME)*FRACUNIT/TICRATE), MAXSHAKE)*3;
				SINT8 mult = leveltime & 1 ? 1 : -1;

				p->mo->spritexoffset = shake*mult;
			}

			// sqish them a little MORE....
			p->mo->spritexscale = FRACUNIT*12/10;
			p->mo->spriteyscale = FRACUNIT*8/10;
		}


		p->ebrakefor++;
	}
	else if (p->ebrakefor)	// cancel effects
	{
		// reset scale
		p->mo->spritexscale = FRACUNIT;
		p->mo->spriteyscale = FRACUNIT;

		// reset shake
		p->mo->spritexoffset = 0;

		// remove the bubble instantly unless it's in the !? state
		if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev) && (p->mo->hprev->frame & FF_FRAMEMASK) != 5)
		{
			P_RemoveMobj(p->mo->hprev);
			p->mo->hprev = NULL;
		}

		p->ebrakefor = 0;
	}
}

static void K_KartSpindashDust(mobj_t *parent)
{
	fixed_t rad = FixedDiv(FixedHypot(parent->radius, parent->radius), parent->scale);
	INT32 i;

	for (i = 0; i < 2; i++)
	{
		fixed_t hmomentum = P_RandomRange(PR_DECORATION, 6, 12) * parent->scale;
		fixed_t vmomentum = P_RandomRange(PR_DECORATION, 2, 6) * parent->scale;

		angle_t ang = parent->player->drawangle + ANGLE_180;
		SINT8 flip = 1;

		mobj_t *dust;

		if (i & 1)
			ang -= ANGLE_45;
		else
			ang += ANGLE_45;

		dust = P_SpawnMobjFromMobj(parent,
			FixedMul(rad, FINECOSINE(ang >> ANGLETOFINESHIFT)),
			FixedMul(rad, FINESINE(ang >> ANGLETOFINESHIFT)),
			0, MT_SPINDASHDUST
		);
		flip = P_MobjFlip(dust);

		dust->momx = FixedMul(hmomentum, FINECOSINE(ang >> ANGLETOFINESHIFT));
		dust->momy = FixedMul(hmomentum, FINESINE(ang >> ANGLETOFINESHIFT));
		dust->momz = vmomentum * flip;
	}
}

static void K_KartSpindashWind(mobj_t *parent)
{
	mobj_t *wind = P_SpawnMobjFromMobj(parent,
		P_RandomRange(PR_DECORATION,-36,36) * FRACUNIT,
		P_RandomRange(PR_DECORATION,-36,36) * FRACUNIT,
		FixedDiv(parent->height / 2, parent->scale) + (P_RandomRange(PR_DECORATION,-20,20) * FRACUNIT),
		MT_SPINDASHWIND
	);

	P_SetTarget(&wind->target, parent);

	if (parent->momx || parent->momy)
		P_InitAngle(wind, R_PointToAngle2(0, 0, parent->momx, parent->momy));
	else
		P_InitAngle(wind, parent->player->drawangle);

	wind->momx = 3 * parent->momx / 4;
	wind->momy = 3 * parent->momy / 4;
	wind->momz = 3 * P_GetMobjZMovement(parent) / 4;

	K_MatchGenericExtraFlags(wind, parent);
}

// Time after which you get a thrust for releasing spindash
#define SPINDASHTHRUSTTIME 20

static void K_KartSpindash(player_t *player)
{
	const boolean onGround = P_IsObjectOnGround(player->mo);
	const INT16 MAXCHARGETIME = K_GetSpindashChargeTime(player);
	UINT16 buttons = K_GetKartButtons(player);
	boolean spawnWind = (leveltime % 2 == 0);

	if (player->mo->hitlag > 0 || P_PlayerInPain(player))
	{
		player->spindash = 0;
	}

	if (player->spindash > 0 && (buttons & (BT_DRIFT|BT_BRAKE|BT_ACCELERATE)) != (BT_DRIFT|BT_BRAKE|BT_ACCELERATE))
	{
		player->spindashspeed = (player->spindash * FRACUNIT) / MAXCHARGETIME;
		player->spindashboost = TICRATE;

		// if spindash was charged enough, give a small thrust.
		if (player->spindash >= SPINDASHTHRUSTTIME)
		{
			fixed_t thrust = FixedMul(player->mo->scale, player->spindash*FRACUNIT/5);

			// TODO: gametyperules
			if (gametype == GT_BATTLE)
				thrust *= 2;

			// Give a bit of a boost depending on charge.
			P_InstaThrust(player->mo, player->mo->angle, thrust);
		}

		if (!player->tiregrease)
		{
			UINT8 i;
			for (i = 0; i < 2; i++)
			{
				mobj_t *grease;
				grease = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_TIREGREASE);
				P_SetTarget(&grease->target, player->mo);
				P_InitAngle(grease, K_MomentumAngle(player->mo));
				grease->extravalue1 = i;
			}
		}

		player->tiregrease = 2*TICRATE;

		player->spindash = 0;
		S_StartSound(player->mo, sfx_s23c);
	}


	if ((player->spindashboost > 0) && (spawnWind == true))
	{
		K_KartSpindashWind(player->mo);
	}

	if (player->spindashboost > (TICRATE/2))
	{
		K_KartSpindashDust(player->mo);
	}

	if (K_PlayerEBrake(player) == false)
	{
		player->spindash = 0;
		return;
	}

	// Handle fast falling behaviors first.
	if (onGround == false)
	{
		// Update fastfall.
		player->fastfall = player->mo->momz;
		player->spindash = 0;
		return;
	}
	else if (player->fastfall != 0)
	{
		// Still handling fast-fall bounce.
		return;
	}

	if (player->speed == 0 && player->steering != 0 && leveltime % 8 == 0)
	{
		// Rubber burn turn sfx
		S_StartSound(player->mo, sfx_ruburn);
	}

	if (player->speed < 6*player->mo->scale)
	{
		if ((buttons & (BT_DRIFT|BT_BRAKE)) == (BT_DRIFT|BT_BRAKE))
		{
			UINT8 ringdropframes = 2 + (player->kartspeed + player->kartweight);
			INT16 chargetime = MAXCHARGETIME - ++player->spindash;
			boolean spawnOldEffect = true;

			if (player->spindash >= SPINDASHTHRUSTTIME)
			{
				K_KartSpindashDust(player->mo);
				spawnOldEffect = false;
			}

			if (chargetime <= (MAXCHARGETIME / 4) && spawnWind == true)
			{
				K_KartSpindashWind(player->mo);
			}

			if (player->flashing > 0 && (player->spindash % ringdropframes == 0) && player->hyudorotimer == 0)
			{
				// Every frame that you're invisible from flashing, spill a ring.
				// Intentionally a lop-sided trade-off, so the game doesn't become
				// Funky Kong's Ring Racers.

				P_PlayerRingBurst(player, 1);
			}

			if (chargetime > 0)
			{
				UINT16 soundcharge = 0;
				UINT8 add = 0;

				while ((soundcharge += ++add) < chargetime);

				if (soundcharge == chargetime)
				{
					if (spawnOldEffect == true)
						K_SpawnDashDustRelease(player);
					S_StartSound(player->mo, sfx_s3kab);
				}
			}
			else if (chargetime < -TICRATE)
			{
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_NORMAL);
			}
		}
	}
	else
	{
		if (leveltime % 4 == 0)
			S_StartSound(player->mo, sfx_kc2b);
	}
}

#undef SPINDASHTHRUSTTIME

boolean K_FastFallBounce(player_t *player)
{
	// Handle fastfall bounce.
	if (player->fastfall != 0)
	{
		const fixed_t maxBounce = player->mo->scale * 10;
		const fixed_t minBounce = player->mo->scale;
		fixed_t bounce = 2 * abs(player->fastfall) / 3;

		if (bounce > maxBounce)
		{
			bounce = maxBounce;
		}
		else
		{
			// Lose speed on bad bounce.
			player->mo->momx /= 2;
			player->mo->momy /= 2;

			if (bounce < minBounce)
			{
				bounce = minBounce;
			}
		}

		S_StartSound(player->mo, sfx_ffbonc);
		player->mo->momz = bounce * P_MobjFlip(player->mo);

		player->fastfall = 0;
		return true;
	}

	return false;
}

static void K_AirFailsafe(player_t *player)
{
	const fixed_t maxSpeed = 6*player->mo->scale;
	const fixed_t thrustSpeed = 6*player->mo->scale; // 10*player->mo->scale

	if (player->speed > maxSpeed // Above the max speed that you're allowed to use this technique.
		|| player->respawn.state != RESPAWNST_NONE) // Respawning, you don't need this AND drop dash :V
	{
		player->pflags &= ~PF_AIRFAILSAFE;
		return;
	}

	if ((K_GetKartButtons(player) & BT_ACCELERATE) || K_GetForwardMove(player) != 0)
	{
		// Queue up later
		player->pflags |= PF_AIRFAILSAFE;
		return;
	}

	if (player->pflags & PF_AIRFAILSAFE)
	{
		// Push the player forward
		P_Thrust(player->mo, K_MomentumAngle(player->mo), thrustSpeed);

		S_StartSound(player->mo, sfx_s23c);
		K_SpawnDriftBoostExplosion(player, 0);

		player->pflags &= ~PF_AIRFAILSAFE;
	}
}

//
// K_AdjustPlayerFriction
//
void K_AdjustPlayerFriction(player_t *player)
{
	fixed_t prevfriction = player->mo->friction;

	if (P_IsObjectOnGround(player->mo) == false)
	{
		return;
	}

	// Reduce friction after hitting a spring
	if (player->tiregrease)
	{
		player->mo->friction += ((FRACUNIT - prevfriction) / greasetics) * player->tiregrease;
	}

	/*
	if (K_PlayerEBrake(player) == true)
	{
		player->mo->friction -= 1024;
	}
	else if (player->speed > 0 && K_GetForwardMove(player) < 0)
	{
		player->mo->friction -= 512;
	}
	*/

	// Water gets ice physics too
	if ((player->mo->eflags & MFE_TOUCHWATER) &&
			!player->offroad)
	{
		player->mo->friction += 614;
	}
	else if (player->mo->eflags & MFE_UNDERWATER)
	{
		player->mo->friction += 312;
	}

	// Wipeout slowdown
	if (player->speed > 0 && player->spinouttimer && player->wipeoutslow)
	{
		if (player->offroad)
			player->mo->friction -= 4912;
		if (player->wipeoutslow == 1)
			player->mo->friction -= 9824;
	}

	// Cap between intended values
	if (player->mo->friction > FRACUNIT)
		player->mo->friction = FRACUNIT;
	if (player->mo->friction < 0)
		player->mo->friction = 0;

	// Friction was changed, so we must recalculate movefactor
	if (player->mo->friction != prevfriction)
	{
		player->mo->movefactor = FixedDiv(ORIG_FRICTION, player->mo->friction);

		if (player->mo->movefactor < FRACUNIT)
			player->mo->movefactor = 19*player->mo->movefactor - 18*FRACUNIT;
		else
			player->mo->movefactor = FRACUNIT;
	}
}

//
// K_trickPanelTimingVisual
// Spawns the timing visual for trick panels depending on the given player's momz.
// If the player has tricked and is not in hitlag, this will send the half circles flying out.
// if you tumble, they'll fall off instead.
//

#define RADIUSSCALING 6
#define MINRADIUS 12

static void K_trickPanelTimingVisual(player_t *player, fixed_t momz)
{

	fixed_t pos, tx, ty, tz;
	mobj_t *flame;

	angle_t hang = R_PointToAnglePlayer(player, player->mo->x, player->mo->y) + ANG1*90;			// horizontal angle
	angle_t vang = -FixedAngle(momz)*12 + (ANG1*45);												// vertical angle dependant on momz, we want it to line up at 45 degrees at the perfect frame to trick at
	fixed_t dist = FixedMul(max(MINRADIUS<<FRACBITS, abs(momz)*RADIUSSCALING), player->mo->scale);	// distance.

	UINT8 i;

	// Do you like trig? cool, me neither.
	for (i=0; i < 2; i++)
	{
		pos = FixedMul(dist, FINESINE(vang>>ANGLETOFINESHIFT));
		tx = player->mo->x + FixedMul(pos, FINECOSINE(hang>>ANGLETOFINESHIFT));
		ty = player->mo->y + FixedMul(pos, FINESINE(hang>>ANGLETOFINESHIFT));
		tz = player->mo->z + player->mo->height/2 + FixedMul(dist, FINECOSINE(vang>>ANGLETOFINESHIFT));

		// All coordinates set, spawn our fire, now.
		flame = P_SpawnMobj(tx, ty, tz, MT_THOK);

		P_SetScale(flame, player->mo->scale);

		// Visuals
		flame->sprite = SPR_TRCK;
		flame->frame = i|FF_FULLBRIGHT;

		if (player->trickpanel <= 1 && !player->tumbleBounces)
		{
			flame->tics = 2;
			flame->momx = player->mo->momx;
			flame->momy = player->mo->momy;
			flame->momz = player->mo->momz;
		}
		else
		{
			flame->tics = TICRATE;

			if (player->trickpanel > 1)	// we tricked
			{
				// Send the thing outwards via ghetto maths which involves redoing the whole 3d sphere again, witht the "vertical" angle shifted by 90 degrees.
				// There's probably a simplier way to do this the way I want to but this works.
				pos = FixedMul(48*player->mo->scale, FINESINE((vang +ANG1*90)>>ANGLETOFINESHIFT));
				tx = player->mo->x + FixedMul(pos, FINECOSINE(hang>>ANGLETOFINESHIFT));
				ty = player->mo->y + FixedMul(pos, FINESINE(hang>>ANGLETOFINESHIFT));
				tz = player->mo->z + player->mo->height/2 + FixedMul(48*player->mo->scale, FINECOSINE((vang +ANG1*90)>>ANGLETOFINESHIFT));

				flame->momx = tx -player->mo->x;
				flame->momy = ty -player->mo->y;
				flame->momz = tz -(player->mo->z+player->mo->height/2);
			}
			else	// we failed the trick, drop the half circles, it'll be funny I promise.
			{
				flame->flags &= ~MF_NOGRAVITY;
				P_SetObjectMomZ(flame, 4<<FRACBITS, false);
				P_InstaThrust(flame, R_PointToAngle2(player->mo->x, player->mo->y, flame->x, flame->y), 8*mapobjectscale);
				flame->momx += player->mo->momx;
				flame->momy += player->mo->momy;
				flame->momz += player->mo->momz;
			}
		}

		// make sure this is only drawn for our local player
		flame->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(player));

		vang += FixedAngle(180<<FRACBITS);	// Avoid overflow warnings...

	}
}

#undef RADIUSSCALING
#undef MINRADIUS

void K_SetItemOut(player_t *player)
{
	player->pflags |= PF_ITEMOUT;

	if (player->mo->scale >= FixedMul(GROW_PHYSICS_SCALE, mapobjectscale))
	{
		player->itemscale = ITEMSCALE_GROW;
	}
	else if (player->mo->scale <= FixedMul(SHRINK_PHYSICS_SCALE, mapobjectscale))
	{
		player->itemscale = ITEMSCALE_SHRINK;
	}
	else
	{
		player->itemscale = ITEMSCALE_NORMAL;
	}
}

void K_UnsetItemOut(player_t *player)
{
	player->pflags &= ~PF_ITEMOUT;
	player->itemscale = ITEMSCALE_NORMAL;
	player->bananadrag = 0;
}

//
// K_MoveKartPlayer
//
void K_MoveKartPlayer(player_t *player, boolean onground)
{
	ticcmd_t *cmd = &player->cmd;
	boolean ATTACK_IS_DOWN = ((cmd->buttons & BT_ATTACK) && !(player->oldcmd.buttons & BT_ATTACK));
	boolean HOLDING_ITEM = (player->pflags & (PF_ITEMOUT|PF_EGGMANOUT));
	boolean NO_HYUDORO = (player->stealingtimer == 0);

	if (!player->exiting)
	{
		if (player->oldposition < player->position) // But first, if you lost a place,
		{
			player->oldposition = player->position; // then the other player taunts.
			K_RegularVoiceTimers(player); // and you can't for a bit
		}
		else if (player->oldposition > player->position) // Otherwise,
		{
			K_PlayOvertakeSound(player->mo); // Say "YOU'RE TOO SLOW!"
			player->oldposition = player->position; // Restore the old position,
		}
	}

	if (player->positiondelay)
		player->positiondelay--;

	// Prevent ring misfire
	if (!(cmd->buttons & BT_ATTACK))
	{
		if (player->itemtype == KITEM_NONE
			&& NO_HYUDORO && !(HOLDING_ITEM
			|| player->itemamount
			|| player->itemroulette
			|| player->rocketsneakertimer
			|| player->eggmanexplode))
			player->pflags |= PF_USERINGS;
		else
			player->pflags &= ~PF_USERINGS;
	}

	if (player && player->mo && player->mo->health > 0 && !player->spectator && !P_PlayerInPain(player) && !mapreset && leveltime > introtime)
	{
		// First, the really specific, finicky items that function without the item being directly in your item slot.
		{
			// Ring boosting
			if (player->pflags & PF_USERINGS)
			{
				if ((cmd->buttons & BT_ATTACK) && !player->ringdelay && player->rings > 0)
				{
					mobj_t *ring = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_RING);
					P_SetMobjState(ring, S_FASTRING1);
					ring->extravalue1 = 1; // Ring use animation timer
					ring->extravalue2 = 1; // Ring use animation flag
					ring->shadowscale = 0;
					P_SetTarget(&ring->target, player->mo); // user
					player->rings--;
					player->ringdelay = 3;
				}
			}
			// Other items
			else
			{
				// Eggman Monitor exploding
				if (player->eggmanexplode)
				{
					if (ATTACK_IS_DOWN && player->eggmanexplode <= 3*TICRATE && player->eggmanexplode > 1)
						player->eggmanexplode = 1;
				}
				// Eggman Monitor throwing
				else if (player->pflags & PF_EGGMANOUT)
				{
					if (ATTACK_IS_DOWN)
					{
						K_ThrowKartItem(player, false, MT_EGGMANITEM, -1, 0, 0);
						K_PlayAttackTaunt(player->mo);
						player->pflags &= ~PF_EGGMANOUT;
						K_UpdateHnextList(player, true);
					}
				}
				// Rocket Sneaker usage
				else if (player->rocketsneakertimer > 1)
				{
					if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO)
					{
						K_DoSneaker(player, 2);
						K_PlayBoostTaunt(player->mo);
						if (player->rocketsneakertimer <= 3*TICRATE)
							player->rocketsneakertimer = 1;
						else
							player->rocketsneakertimer -= 3*TICRATE;
					}
				}
				else if (player->itemamount == 0)
				{
					K_UnsetItemOut(player);
				}
				else
				{
					switch (player->itemtype)
					{
						case KITEM_SNEAKER:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO)
							{
								K_DoSneaker(player, 1);
								K_PlayBoostTaunt(player->mo);
								player->itemamount--;
							}
							break;
						case KITEM_ROCKETSNEAKER:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO
								&& player->rocketsneakertimer == 0)
							{
								INT32 moloop;
								mobj_t *mo = NULL;
								mobj_t *prev = player->mo;

								K_PlayBoostTaunt(player->mo);
								S_StartSound(player->mo, sfx_s3k3a);

								//K_DoSneaker(player, 2);

								player->rocketsneakertimer = (itemtime*3);
								player->itemamount--;
								K_UpdateHnextList(player, true);

								for (moloop = 0; moloop < 2; moloop++)
								{
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_ROCKETSNEAKER);
									K_MatchGenericExtraFlags(mo, player->mo);
									mo->flags |= MF_NOCLIPTHING;
									P_InitAngle(mo, player->mo->angle);
									mo->threshold = 10;
									mo->movecount = moloop%2;
									mo->movedir = mo->lastlook = moloop+1;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
							}
							break;
						case KITEM_INVINCIBILITY:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO) // Doesn't hold your item slot hostage normally, so you're free to waste it if you have multiple
							{
								K_DoInvincibility(player, 10 * TICRATE);
								K_PlayPowerGloatSound(player->mo);
								player->itemamount--;
							}
							break;
						case KITEM_BANANA:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								INT32 moloop;
								mobj_t *mo;
								mobj_t *prev = player->mo;

								//K_PlayAttackTaunt(player->mo);
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);

								for (moloop = 0; moloop < player->itemamount; moloop++)
								{
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BANANA_SHIELD);
									if (!mo)
									{
										player->itemamount = moloop;
										break;
									}
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = player->itemamount;
									mo->movedir = moloop+1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
							}
							else if (ATTACK_IS_DOWN && (player->pflags & PF_ITEMOUT)) // Banana x3 thrown
							{
								K_ThrowKartItem(player, false, MT_BANANA, -1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemamount--;
								K_UpdateHnextList(player, false);
							}
							break;
						case KITEM_EGGMAN:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								player->itemamount--;
								player->pflags |= PF_EGGMANOUT;
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_EGGMANITEM_SHIELD);
								if (mo)
								{
									K_FlipFromObject(mo, player->mo);
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
							}
							break;
						case KITEM_ORBINAUT:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								angle_t newangle;
								INT32 moloop;
								mobj_t *mo = NULL;
								mobj_t *prev = player->mo;

								//K_PlayAttackTaunt(player->mo);
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s3k3a);

								for (moloop = 0; moloop < player->itemamount; moloop++)
								{
									newangle = (player->mo->angle + ANGLE_157h) + FixedAngle(((360 / player->itemamount) * moloop) << FRACBITS) + ANGLE_90;
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_ORBINAUT_SHIELD);
									if (!mo)
									{
										player->itemamount = moloop;
										break;
									}
									mo->flags |= MF_NOCLIPTHING;
									P_InitAngle(mo, newangle);
									mo->threshold = 10;
									mo->movecount = player->itemamount;
									mo->movedir = mo->lastlook = moloop+1;
									mo->color = player->skincolor;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
							}
							else if (ATTACK_IS_DOWN && (player->pflags & PF_ITEMOUT)) // Orbinaut x3 thrown
							{
								K_ThrowKartItem(player, true, MT_ORBINAUT, 1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemamount--;
								K_UpdateHnextList(player, false);
							}
							break;
						case KITEM_JAWZ:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								angle_t newangle;
								INT32 moloop;
								mobj_t *mo = NULL;
								mobj_t *prev = player->mo;

								//K_PlayAttackTaunt(player->mo);
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s3k3a);

								for (moloop = 0; moloop < player->itemamount; moloop++)
								{
									newangle = (player->mo->angle + ANGLE_157h) + FixedAngle(((360 / player->itemamount) * moloop) << FRACBITS) + ANGLE_90;
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_JAWZ_SHIELD);
									if (!mo)
									{
										player->itemamount = moloop;
										break;
									}
									mo->flags |= MF_NOCLIPTHING;
									P_InitAngle(mo, newangle);
									mo->threshold = 10;
									mo->movecount = player->itemamount;
									mo->movedir = mo->lastlook = moloop+1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
							}
							else if (ATTACK_IS_DOWN && HOLDING_ITEM && (player->pflags & PF_ITEMOUT)) // Jawz thrown
							{
								K_ThrowKartItem(player, true, MT_JAWZ, 1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemamount--;
								K_UpdateHnextList(player, false);
							}
							break;
						case KITEM_MINE:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SSMINE_SHIELD);
								if (mo)
								{
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
							}
							else if (ATTACK_IS_DOWN && (player->pflags & PF_ITEMOUT))
							{
								K_ThrowKartItem(player, false, MT_SSMINE, 1, 1, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemamount--;
								player->pflags &= ~PF_ITEMOUT;
								K_UpdateHnextList(player, true);
							}
							break;
						case KITEM_LANDMINE:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								K_ThrowLandMine(player);
								K_PlayAttackTaunt(player->mo);
							}
							break;
						case KITEM_DROPTARGET:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_DROPTARGET_SHIELD);
								if (mo)
								{
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
							}
							else if (ATTACK_IS_DOWN && (player->pflags & PF_ITEMOUT))
							{
								K_ThrowKartItem(player, (player->throwdir > 0), MT_DROPTARGET, -1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemamount--;
								player->pflags &= ~PF_ITEMOUT;
								K_UpdateHnextList(player, true);
							}
							break;
						case KITEM_BALLHOG:
							if (!HOLDING_ITEM && NO_HYUDORO)
							{
								INT32 ballhogmax = ((player->itemamount-1) * BALLHOGINCREMENT) + 1;

								if ((cmd->buttons & BT_ATTACK) && (player->pflags & PF_HOLDREADY)
									&& (player->ballhogcharge < ballhogmax))
								{
									player->ballhogcharge++;
								}
								else
								{
									if (cmd->buttons & BT_ATTACK)
									{
										player->pflags &= ~PF_HOLDREADY;
									}
									else
									{
										player->pflags |= PF_HOLDREADY;
									}

									if (player->ballhogcharge > 0)
									{
										INT32 numhogs = min((player->ballhogcharge / BALLHOGINCREMENT) + 1, player->itemamount);

										if (numhogs <= 1)
										{
											player->itemamount--;
											K_ThrowKartItem(player, true, MT_BALLHOG, 1, 0, 0);
										}
										else
										{
											angle_t cone = 0x01800000 * (numhogs-1);
											angle_t offsetAmt = (cone * 2) / (numhogs-1);
											angle_t angleOffset = cone;
											INT32 i;

											player->itemamount -= numhogs;

											for (i = 0; i < numhogs; i++)
											{
												K_ThrowKartItem(player, true, MT_BALLHOG, 1, 0, angleOffset);
												angleOffset -= offsetAmt;
											}
										}

										player->ballhogcharge = 0;
										K_PlayAttackTaunt(player->mo);
										player->pflags &= ~PF_HOLDREADY;
									}
								}
							}
							break;
						case KITEM_SPB:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								K_ThrowKartItem(player, true, MT_SPB, 1, 0, 0);
								K_PlayAttackTaunt(player->mo);
							}
							break;
						case KITEM_GROW:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								if (player->growshrinktimer < 0)
								{
									// If you're shrunk, then "grow" will just make you normal again.
									K_RemoveGrowShrink(player);
								}
								else
								{
									K_PlayPowerGloatSound(player->mo);

									player->mo->scalespeed = mapobjectscale/TICRATE;
									player->mo->destscale = FixedMul(mapobjectscale, GROW_SCALE);

									if (K_PlayerShrinkCheat(player) == true)
									{
										player->mo->destscale = FixedMul(player->mo->destscale, SHRINK_SCALE);
									}

									// TODO: gametyperules
									player->growshrinktimer = max(player->growshrinktimer, (gametype == GT_BATTLE ? 8 : 12) * TICRATE);

									if (player->invincibilitytimer > 0)
									{
										; // invincibility has priority in P_RestoreMusic, no point in starting here
									}
									else if (P_IsLocalPlayer(player) == true)
									{
										S_ChangeMusicSpecial("kgrow");
									}
									else //used to be "if (P_IsDisplayPlayer(player) == false)"
									{
										S_StartSound(player->mo, sfx_alarmg);
									}

									P_RestoreMusic(player);
									S_StartSound(player->mo, sfx_kc5a);
								}

								player->itemamount--;
							}
							break;
						case KITEM_SHRINK:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								K_DoShrink(player);
								player->itemamount--;
								K_PlayPowerGloatSound(player->mo);
							}
							break;
						case KITEM_LIGHTNINGSHIELD:
							if (player->curshield != KSHIELD_LIGHTNING)
							{
								mobj_t *shield = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_LIGHTNINGSHIELD);
								P_SetScale(shield, (shield->destscale = (5*shield->destscale)>>2));
								P_SetTarget(&shield->target, player->mo);
								S_StartSound(player->mo, sfx_s3k41);
								player->curshield = KSHIELD_LIGHTNING;
							}

							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								K_DoLightningShield(player);
								if (player->itemamount > 0)
								{
									// Why is this a conditional?
									// Lightning shield: the only item that allows you to
									// activate a mine while you're out of its radius,
									// the SAME tic it sets your itemamount to 0
									// ...:dumbestass:
									player->itemamount--;
									K_PlayAttackTaunt(player->mo);
								}
							}
							break;
						case KITEM_BUBBLESHIELD:
							if (player->curshield != KSHIELD_BUBBLE)
							{
								mobj_t *shield = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BUBBLESHIELD);
								P_SetScale(shield, (shield->destscale = (5*shield->destscale)>>2));
								P_SetTarget(&shield->target, player->mo);
								S_StartSound(player->mo, sfx_s3k3f);
								player->curshield = KSHIELD_BUBBLE;
							}

							if (!HOLDING_ITEM && NO_HYUDORO)
							{
								if ((cmd->buttons & BT_ATTACK) && (player->pflags & PF_HOLDREADY))
								{
									if (player->bubbleblowup == 0)
										S_StartSound(player->mo, sfx_s3k75);

									player->bubbleblowup++;
									player->bubblecool = player->bubbleblowup*4;

									if (player->bubbleblowup > bubbletime*2)
									{
										K_ThrowKartItem(player, (player->throwdir > 0), MT_BUBBLESHIELDTRAP, -1, 0, 0);
										K_PlayAttackTaunt(player->mo);
										player->bubbleblowup = 0;
										player->bubblecool = 0;
										player->pflags &= ~PF_HOLDREADY;
										player->itemamount--;
									}
								}
								else
								{
									if (player->bubbleblowup > bubbletime)
										player->bubbleblowup = bubbletime;

									if (player->bubbleblowup)
										player->bubbleblowup--;

									if (player->bubblecool)
										player->pflags &= ~PF_HOLDREADY;
									else
										player->pflags |= PF_HOLDREADY;
								}
							}
							break;
						case KITEM_FLAMESHIELD:
							if (player->curshield != KSHIELD_FLAME)
							{
								mobj_t *shield = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_FLAMESHIELD);
								P_SetScale(shield, (shield->destscale = (5*shield->destscale)>>2));
								P_SetTarget(&shield->target, player->mo);
								S_StartSound(player->mo, sfx_s3k3e);
								player->curshield = KSHIELD_FLAME;
							}

							if (!HOLDING_ITEM && NO_HYUDORO)
							{
								INT32 destlen = K_FlameShieldMax(player);
								INT32 flamemax = 0;

								if (player->flamelength < destlen)
									player->flamelength++; // Can always go up!

								flamemax = player->flamelength * flameseg;
								if (flamemax > 0)
									flamemax += TICRATE; // leniency period

								if ((cmd->buttons & BT_ATTACK) && (player->pflags & PF_HOLDREADY))
								{
									// TODO: gametyperules
									const INT32 incr = gametype == GT_BATTLE ? 4 : 2;

									if (player->flamedash == 0)
									{
										S_StartSound(player->mo, sfx_s3k43);
										K_PlayBoostTaunt(player->mo);
									}

									player->flamedash += incr;
									player->flamemeter += incr;

									if (!onground)
									{
										P_Thrust(
											player->mo, K_MomentumAngle(player->mo),
											FixedMul(player->mo->scale, K_GetKartGameSpeedScalar(gamespeed))
										);
									}

									if (player->flamemeter > flamemax)
									{
										P_Thrust(
											player->mo, player->mo->angle,
											FixedMul((50*player->mo->scale), K_GetKartGameSpeedScalar(gamespeed))
										);

										player->flamemeter = 0;
										player->flamelength = 0;
										player->pflags &= ~PF_HOLDREADY;
										player->itemamount--;
									}
								}
								else
								{
									player->pflags |= PF_HOLDREADY;

									// TODO: gametyperules
									if (gametype != GT_BATTLE || leveltime % 6 == 0)
									{
										if (player->flamemeter > 0)
											player->flamemeter--;
									}

									if (player->flamelength > destlen)
									{
										player->flamelength--; // Can ONLY go down if you're not using it

										flamemax = player->flamelength * flameseg;
										if (flamemax > 0)
											flamemax += TICRATE; // leniency period
									}

									if (player->flamemeter > flamemax)
										player->flamemeter = flamemax;
								}
							}
							break;
						case KITEM_HYUDORO:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								//K_DoHyudoroSteal(player); // yes. yes they do.
								Obj_HyudoroDeploy(player->mo);
								K_PlayAttackTaunt(player->mo);
							}
							break;
						case KITEM_POGOSPRING:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO && player->trickpanel == 0)
							{
								K_PlayBoostTaunt(player->mo);
								//K_DoPogoSpring(player->mo, 32<<FRACBITS, 2);
								P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_POGOSPRING);
								player->itemamount--;
							}
							break;
						case KITEM_SUPERRING:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								K_AwardPlayerRings(player, 10, true);
								player->itemamount--;
							}
							break;
						case KITEM_KITCHENSINK:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SINK_SHIELD);
								if (mo)
								{
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
							}
							else if (ATTACK_IS_DOWN && HOLDING_ITEM && (player->pflags & PF_ITEMOUT)) // Sink thrown
							{
								K_ThrowKartItem(player, false, MT_SINK, 1, 2, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemamount--;
								player->pflags &= ~PF_ITEMOUT;
								K_UpdateHnextList(player, true);
							}
							break;
						case KITEM_SAD:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO
								&& !player->sadtimer)
							{
								player->sadtimer = stealtime;
								player->itemamount--;
							}
							break;
						default:
							break;
					}
				}
			}
		}

		// No more!
		if (!player->itemamount)
		{
			player->pflags &= ~PF_ITEMOUT;
			player->itemtype = KITEM_NONE;
		}

		if (K_GetShieldFromItem(player->itemtype) == KSHIELD_NONE)
		{
			player->curshield = KSHIELD_NONE; // RESET shield type
			player->bubbleblowup = 0;
			player->bubblecool = 0;
			player->flamelength = 0;
			player->flamemeter = 0;
		}

		if (spbplace == -1 || player->position != spbplace)
			player->pflags &= ~PF_RINGLOCK; // reset ring lock

		if (player->itemtype == KITEM_SPB
			|| player->itemtype == KITEM_SHRINK)
		{
			K_SetItemCooldown(player->itemtype, 20*TICRATE);
		}

		if (player->hyudorotimer > 0)
		{
			INT32 hyu = hyudorotime;

			if (gametype == GT_RACE)
				hyu *= 2; // double in race

			if (leveltime & 1)
			{
				player->mo->renderflags |= RF_DONTDRAW;
			}
			else
			{
				if (player->hyudorotimer >= (TICRATE/2) && player->hyudorotimer <= hyu-(TICRATE/2))
					player->mo->renderflags &= ~K_GetPlayerDontDrawFlag(player);
				else
					player->mo->renderflags &= ~RF_DONTDRAW;
			}

			player->flashing = player->hyudorotimer; // We'll do this for now, let's people know about the invisible people through subtle hints
		}
		else if (player->hyudorotimer == 0)
		{
			player->mo->renderflags &= ~RF_DONTDRAW;
		}

		if (gametype == GT_BATTLE && player->bumpers <= 0) // dead in match? you da bomb
		{
			K_DropItems(player); //K_StripItems(player);
			K_StripOther(player);
			player->mo->renderflags |= RF_GHOSTLY;
			player->flashing = player->karmadelay;
		}
		else if (gametype == GT_RACE || player->bumpers > 0)
		{
			player->mo->renderflags &= ~(RF_TRANSMASK|RF_BRIGHTMASK);
		}

		if (player->trickpanel == 1)
		{
			const angle_t lr = ANGLE_45;
			fixed_t momz = FixedDiv(player->mo->momz, mapobjectscale);	// bring momz back to scale...
			fixed_t speedmult = max(0, FRACUNIT - abs(momz)/TRICKMOMZRAMP);				// TRICKMOMZRAMP momz is minimum speed (Should be 20)
			fixed_t basespeed = K_GetKartSpeed(player, false, false);	// at WORSE, keep your normal speed when tricking.
			fixed_t speed = FixedMul(speedmult, P_AproxDistance(player->mo->momx, player->mo->momy));

			K_trickPanelTimingVisual(player, momz);

			// streaks:
			if (momz*P_MobjFlip(player->mo) > 0)	// only spawn those while you're going upwards relative to your current gravity
			{
				// these are all admittedly arbitrary numbers...
				INT32 n;
				INT32 maxlines = max(1, (momz/FRACUNIT)/16);
				INT32 frequency = max(1, 5-(momz/FRACUNIT)/4);
				fixed_t sx, sy, sz;
				mobj_t *spdl;

				if (!(leveltime % frequency))
				{
					for (n=0; n < maxlines; n++)
					{
						sx = player->mo->x + P_RandomRange(PR_DECORATION, -24, 24)*player->mo->scale;
						sy = player->mo->y + P_RandomRange(PR_DECORATION, -24, 24)*player->mo->scale;
						sz = player->mo->z + P_RandomRange(PR_DECORATION, 0, 48)*player->mo->scale;

						spdl = P_SpawnMobj(sx, sy, sz, MT_FASTLINE);
						P_SetTarget(&spdl->target, player->mo);
						P_InitAngle(spdl, R_PointToAngle2(spdl->x, spdl->y, player->mo->x, player->mo->y));
						spdl->rollangle = -ANG1*90*P_MobjFlip(player->mo);		// angle them downwards relative to the player's gravity...
						spdl->spriteyscale = player->trickboostpower+FRACUNIT;
						spdl->momx = player->mo->momx;
						spdl->momy = player->mo->momy;
					}

				}

			}

			// We'll never need to go above that.
			if (player->tricktime <= TRICKDELAY)
				player->tricktime++;

			// debug shit
			//CONS_Printf("%d\n", player->mo->momz / mapobjectscale);
			if (momz < -10*FRACUNIT)	// :youfuckedup:
			{
				// tumble if you let your chance pass!!
				player->tumbleBounces = 1;
				player->pflags &= ~PF_TUMBLESOUND;
				player->tumbleHeight = 30;	// Base tumble bounce height
				player->trickpanel = 0;
				K_trickPanelTimingVisual(player, momz);	// fail trick visual
				P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
				if (player->pflags & (PF_ITEMOUT|PF_EGGMANOUT))
					K_DropHnextList(player, true);
			}

			else if (!(player->pflags & PF_TRICKDELAY))	// don't allow tricking at the same frame you tumble obv
			{
				INT16 aimingcompare = abs(cmd->throwdir) - abs(cmd->turning);

				// Uses cmd->turning over steering intentionally.
#define TRICKTHRESHOLD (KART_FULLTURN/4)
				if (aimingcompare < -TRICKTHRESHOLD) // side trick
				{
					if (cmd->turning > 0)
					{
						P_InstaThrust(player->mo, player->mo->angle + lr, max(basespeed, speed*5/2));
						player->trickpanel = 2;
					}
					else
					{
						P_InstaThrust(player->mo, player->mo->angle - lr, max(basespeed, speed*5/2));
						player->trickpanel = 3;
					}
				}
				else if (aimingcompare > TRICKTHRESHOLD) // forward/back trick
				{
					if (cmd->throwdir > 0) // back trick
					{
						if (player->mo->momz * P_MobjFlip(player->mo) > 0)
						{
							player->mo->momz = 0;
						}

						P_InstaThrust(player->mo, player->mo->angle, max(basespeed, speed*3));
						player->trickpanel = 2;
					}
					else if (cmd->throwdir < 0)
					{
						boolean relative = true;

						player->mo->momx /= 3;
						player->mo->momy /= 3;

						if (player->mo->momz * P_MobjFlip(player->mo) <= 0)
						{
							relative = false;
						}

						// Calculate speed boost decay:
						// Base speed boost duration is 35 tics.
						// At most, lose 3/4th of your boost.
						player->trickboostdecay = min(TICRATE*3/4, abs(momz/FRACUNIT));
						//CONS_Printf("decay: %d\n", player->trickboostdecay);

						P_SetObjectMomZ(player->mo, 48*FRACUNIT, relative);
						player->trickpanel = 4;
					}
				}
#undef TRICKTHRESHOLD

				// Finalise everything.
				if (player->trickpanel != 1) // just changed from 1?
				{
					player->mo->hitlag = TRICKLAG;
					player->mo->eflags &= ~MFE_DAMAGEHITLAG;

					K_trickPanelTimingVisual(player, momz);

					if (abs(momz) < FRACUNIT*99)	// Let's use that as baseline for PERFECT trick.
					{
						player->karthud[khud_trickcool] = TICRATE;
					}
				}
			}
		}
		else if (player->trickpanel == 4 && P_IsObjectOnGround(player->mo))	// Upwards trick landed!
		{
			//CONS_Printf("apply boost\n");
			S_StartSound(player->mo, sfx_s23c);
			K_SpawnDashDustRelease(player);
			player->trickboost = TICRATE - player->trickboostdecay;

			player->trickpanel = player->trickboostdecay = 0;
		}

		// Wait until we let go off the control stick to remove the delay
		// buttons must be neutral after the initial trick delay. This prevents weirdness where slight nudges after blast off would send you flying.
		if ((player->pflags & PF_TRICKDELAY) && !player->throwdir && !cmd->turning && (player->tricktime >= TRICKDELAY))
		{
			player->pflags &= ~PF_TRICKDELAY;
		}
	}

	K_KartDrift(player, onground);
	K_KartSpindash(player);

	if (onground == false)
	{
		K_AirFailsafe(player);
	}
	else
	{
		player->pflags &= ~PF_AIRFAILSAFE;
	}

	// Play the starting countdown sounds
	if (player == &players[g_localplayers[0]]) // Don't play louder in splitscreen
	{
		if ((leveltime == starttime-(3*TICRATE)) || (leveltime == starttime-(2*TICRATE)) || (leveltime == starttime-TICRATE))
			S_StartSound(NULL, sfx_s3ka7);

		if (leveltime == starttime-(3*TICRATE))
			S_FadeOutStopMusic(3500);
		else if (leveltime == starttime)
			S_StartSound(NULL, sfx_s3kad);
	}
}

void K_CheckSpectateStatus(void)
{
	UINT8 respawnlist[MAXPLAYERS];
	UINT8 i, j, numingame = 0, numjoiners = 0;

	// Maintain spectate wait timer
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator && (players[i].pflags & PF_WANTSTOJOIN))
			players[i].spectatewait++;
		else
			players[i].spectatewait = 0;
	}

	// No one's allowed to join
	if (!cv_allowteamchange.value)
		return;

	// Get the number of players in game, and the players to be de-spectated.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (!players[i].spectator)
		{
			numingame++;
			if (cv_maxplayers.value && numingame >= cv_maxplayers.value) // DON'T allow if you've hit the in-game player cap
				return;
			if (gamestate != GS_LEVEL) // Allow if you're not in a level
				continue;
			if (players[i].exiting) // DON'T allow if anyone's exiting
				return;
			if (numingame < 2 || leveltime < starttime || mapreset) // Allow if the match hasn't started yet
				continue;
			if (leveltime > (starttime + 20*TICRATE)) // DON'T allow if the match is 20 seconds in
				return;
			if (gametype == GT_RACE && players[i].laps >= 2) // DON'T allow if the race is at 2 laps
				return;
			continue;
		}
		else if (!(players[i].pflags & PF_WANTSTOJOIN))
			continue;

		respawnlist[numjoiners++] = i;
	}

	// literally zero point in going any further if nobody is joining
	if (!numjoiners)
		return;

	// Organize by spectate wait timer
	if (cv_maxplayers.value)
	{
		UINT8 oldrespawnlist[MAXPLAYERS];
		memcpy(oldrespawnlist, respawnlist, numjoiners);
		for (i = 0; i < numjoiners; i++)
		{
			UINT8 pos = 0;
			INT32 ispecwait = players[oldrespawnlist[i]].spectatewait;

			for (j = 0; j < numjoiners; j++)
			{
				INT32 jspecwait = players[oldrespawnlist[j]].spectatewait;
				if (j == i)
					continue;
				if (jspecwait > ispecwait)
					pos++;
				else if (jspecwait == ispecwait && j < i)
					pos++;
			}

			respawnlist[pos] = oldrespawnlist[i];
		}
	}

	// Finally, we can de-spectate everyone!
	for (i = 0; i < numjoiners; i++)
	{
		if (cv_maxplayers.value && numingame+i >= cv_maxplayers.value) // Hit the in-game player cap while adding people?
			break;
		//CONS_Printf("player %s is joining on tic %d\n", player_names[respawnlist[i]], leveltime);
		P_SpectatorJoinGame(&players[respawnlist[i]]);
	}

	// Reset the match if you're in an empty server
	if (!mapreset && gamestate == GS_LEVEL && leveltime >= starttime && (numingame < 2 && numingame+i >= 2)) // use previous i value
	{
		S_ChangeMusicInternal("chalng", false); // COME ON
		mapreset = 3*TICRATE; // Even though only the server uses this for game logic, set for everyone for HUD
	}
}

UINT8 K_GetInvincibilityItemFrame(void)
{
	return ((leveltime % (7*3)) / 3);
}

UINT8 K_GetOrbinautItemFrame(UINT8 count)
{
	return min(count - 1, 3);
}

boolean K_IsSPBInGame(void)
{
	UINT8 i;
	thinker_t *think;

	// is there an SPB chasing anyone?
	if (spbplace != -1)
		return true;

	// do any players have an SPB in their item slot?
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (players[i].itemtype == KITEM_SPB)
			return true;
	}

	// spbplace is still -1 until a fired SPB finds a target, so look for an in-map SPB just in case
	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		if (((mobj_t *)think)->type == MT_SPB)
			return true;
	}

	return false;
}

void K_HandleDirectionalInfluence(player_t *player)
{
	fixed_t strength = FRACUNIT >> 1; // 1.0 == 45 degrees

	ticcmd_t *cmd = NULL;
	angle_t sideAngle = ANGLE_MAX;

	INT16 inputX, inputY;
	INT16 inputLen;

	fixed_t diX, diY;
	fixed_t diLen;
	fixed_t diMul;

	fixed_t dot, invDot;

	fixed_t finalX, finalY;
	fixed_t finalLen;
	fixed_t speed;

	if (player->playerstate != PST_LIVE || player->spectator)
	{
		// ded
		return;
	}

	// DI attempted!!
	player->justDI = MAXHITLAGTICS;

	cmd = &player->cmd;

	inputX = cmd->throwdir;
	inputY = -cmd->turning;

	if (player->flipDI == true)
	{
		// Bananas flip the DI direction.
		// Otherwise, DIing bananas is a little brain-dead easy :p
		inputX = -inputX;
		inputY = -inputY;
	}

	if (inputX == 0 && inputY == 0)
	{
		// No DI input, no need to do anything else.
		return;
	}

	inputLen = FixedHypot(inputX, inputY);
	if (inputLen > KART_FULLTURN)
	{
		inputLen = KART_FULLTURN;
	}

	if (player->tumbleBounces > 0)
	{
		// Very strong DI for tumble.
		strength *= 3;
	}

	sideAngle = player->mo->angle - ANGLE_90;

	diX = FixedMul(inputX, FINECOSINE(player->mo->angle >> ANGLETOFINESHIFT)) + FixedMul(inputY, FINECOSINE(sideAngle >> ANGLETOFINESHIFT));
	diY = FixedMul(inputX, FINESINE(player->mo->angle >> ANGLETOFINESHIFT)) + FixedMul(inputY, FINESINE(sideAngle >> ANGLETOFINESHIFT));
	diLen = FixedHypot(diX, diY);

	// Normalize
	diMul = (KART_FULLTURN * FRACUNIT) / inputLen;
	if (diLen > 0)
	{
		diX = FixedMul(diMul, FixedDiv(diX, diLen));
		diY = FixedMul(diMul, FixedDiv(diY, diLen));
	}

	// Now that we got the DI direction, we can
	// actually preform the velocity redirection.

	speed = FixedHypot(player->mo->momx, player->mo->momy);
	finalX = FixedDiv(player->mo->momx, speed);
	finalY = FixedDiv(player->mo->momy, speed);

	dot = FixedMul(diX, finalX) + FixedMul(diY, finalY);
	invDot = FRACUNIT - abs(dot);

	finalX += FixedMul(FixedMul(diX, invDot), strength);
	finalY += FixedMul(FixedMul(diY, invDot), strength);
	finalLen = FixedHypot(finalX, finalY);

	if (finalLen > 0)
	{
		finalX = FixedDiv(finalX, finalLen);
		finalY = FixedDiv(finalY, finalLen);
	}

	player->mo->momx = FixedMul(speed, finalX);
	player->mo->momy = FixedMul(speed, finalY);
}

//}
