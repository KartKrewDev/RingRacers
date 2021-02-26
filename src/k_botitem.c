// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_botitem.c
/// \brief Bot item usage logic

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
	static boolean K_BotUseItemNearPlayer(player_t *player, ticcmd_t *cmd, fixed_t radius)

		Looks for players around the bot, and presses the item button
		if there is one in range.

	Input Arguments:-
		player - Bot to compare against.
		cmd - The bot's ticcmd.
		radius - The radius to look for players in.

	Return:-
		true if a player was found & we can press the item button, otherwise false.
--------------------------------------------------*/
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

/*--------------------------------------------------
	static boolean K_PlayerNearSpot(player_t *player, fixed_t x, fixed_t y, fixed_t radius)

		Looks for players around a specified x/y coordinate.

	Input Arguments:-
		player - Bot to compare against.
		x - X coordinate to look around.
		y - Y coordinate to look around.
		radius - The radius to look for players in.

	Return:-
		true if a player was found around the coordinate, otherwise false.
--------------------------------------------------*/
static boolean K_PlayerNearSpot(player_t *player, fixed_t x, fixed_t y, fixed_t radius)
{
	UINT8 i;

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

		dist = P_AproxDistance(
			x - target->mo->x,
			y - target->mo->y
		);

		if (dist <= radius)
		{
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static boolean K_PlayerPredictThrow(player_t *player, UINT8 extra)

		Looks for players around the predicted coordinates of their thrown item.

	Input Arguments:-
		player - Bot to compare against.
		extra - Extra throwing distance, for aim forward on mines.

	Return:-
		true if a player was found around the coordinate, otherwise false.
--------------------------------------------------*/
static boolean K_PlayerPredictThrow(player_t *player, UINT8 extra)
{
	const fixed_t dist = (30 + (extra * 10)) * player->mo->scale;
	const UINT32 airtime = FixedDiv(dist + player->mo->momz, gravity);
	const fixed_t throwspeed = FixedMul(82 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const fixed_t estx = player->mo->x + P_ReturnThrustX(NULL, player->mo->angle, (throwspeed + player->speed) * airtime);
	const fixed_t esty = player->mo->y + P_ReturnThrustY(NULL, player->mo->angle, (throwspeed + player->speed) * airtime);
	return K_PlayerNearSpot(player, estx, esty, player->mo->radius * 2);
}

/*--------------------------------------------------
	static boolean K_PlayerInCone(player_t *player, UINT16 cone, boolean flip)

		Looks for players in the .

	Input Arguments:-
		player - Bot to compare against.
		radius - How far away the targets can be.
		cone - Size of cone, in degrees as an integer.
		flip - If true, look behind. Otherwise, check in front of the player.

	Return:-
		true if a player was found in the cone, otherwise false.
--------------------------------------------------*/
static boolean K_PlayerInCone(player_t *player, fixed_t radius, UINT16 cone, boolean flip)
{
	UINT8 i;

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
			|| target->powers[pw_flashing]
			|| !P_CheckSight(player->mo, target->mo))
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
			angle_t a = player->mo->angle - R_PointToAngle2(player->mo->x, player->mo->y, target->mo->x, target->mo->y);
			INT16 ad = 0;

			if (a < ANGLE_180)
			{
				ad = AngleFixed(a)>>FRACBITS;
			}
			else 
			{
				ad = 360-(AngleFixed(a)>>FRACBITS);
			}

			ad = abs(ad);

			if (flip)
			{
				if (ad >= 180-cone)
				{
					return true;
				}
			}
			else
			{
				if (ad <= cone)
				{
					return true;
				}
			}
		}
	}

	return false;
}

/*--------------------------------------------------
	static boolean K_BotGenericPressItem(player_t *player, ticcmd_t *cmd, SINT8 dir)

		Presses the item button & aim buttons for the bot.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		dir - Aiming direction: 1 for forwards, -1 for backwards, 0 for neutral.

	Return:-
		true if we could press, false if not.
--------------------------------------------------*/
static boolean K_BotGenericPressItem(player_t *player, ticcmd_t *cmd, SINT8 dir)
{
	if (player->pflags & PF_ATTACKDOWN)
	{
		return false;
	}

	if (dir == 1)
	{
		cmd->buttons |= BT_FORWARD;
	}
	else if (dir == -1)
	{
		cmd->buttons |= BT_BACKWARD;
	}

	cmd->buttons |= BT_ATTACK;
	player->botvars.itemconfirm = 0;
	return true;
}

/*--------------------------------------------------
	static void K_BotItemGenericTap(player_t *player, ticcmd_t *cmd)

		Item usage for generic items that you need to tap.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGenericTap(player_t *player, ticcmd_t *cmd)
{
	if (!(player->pflags & PF_ATTACKDOWN))
	{
		cmd->buttons |= BT_ATTACK;
		player->botvars.itemconfirm = 0;
	}
}

/*--------------------------------------------------
	static boolean K_BotRevealsGenericTrap(player_t *player, INT16 turnamt, boolean mine)

		Decides if a bot is ready to reveal their trap item or not.

	Input Arguments:-
		player - Bot that has the banana.
		turnamt - How hard they currently are turning.
		mine - Set to true to handle Mine-specific behaviors.

	Return:-
		true if we want the bot to reveal their banana, otherwise false.
--------------------------------------------------*/
static boolean K_BotRevealsGenericTrap(player_t *player, INT16 turnamt, boolean mine)
{
	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		// DON'T reveal on turns, we can place bananas on turns whenever we have multiple to spare,
		// or if you missed your intentioned throw/place on a player.
		return false;
	}

	// Check the predicted throws.
	if (K_PlayerPredictThrow(player, 0))
	{
		return true;
	}

	if (mine)
	{
		if (K_PlayerPredictThrow(player, 1))
		{
			return true;
		}
	}

	// Check your behind.
	if (K_PlayerInCone(player, player->mo->radius * 16, 10, true))
	{
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static void K_BotItemGenericTrapShield(player_t *player, ticcmd_t *cmd, INT16 turnamt, boolean mine)

		Item usage for Eggman shields.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.
		mine - Set to true to handle Mine-specific behaviors.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGenericTrapShield(player_t *player, ticcmd_t *cmd, INT16 turnamt, boolean mine)
{
	if (player->kartstuff[k_itemheld])
	{
		return;
	}

	if (K_BotRevealsGenericTrap(player, turnamt, mine) || (player->botvars.itemconfirm++ > 5*TICRATE))
	{
		K_BotGenericPressItem(player, cmd, 0);
	}
}

/*--------------------------------------------------
	static void K_BotItemGenericOrbitShield(player_t *player, ticcmd_t *cmd)

		Item usage for orbitting shields.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemGenericOrbitShield(player_t *player, ticcmd_t *cmd)
{
	if (player->kartstuff[k_itemheld])
	{
		return;
	}

	K_BotGenericPressItem(player, cmd, 0);
}

/*--------------------------------------------------
	static void K_BotItemSneaker(player_t *player, ticcmd_t *cmd)

		Item usage for sneakers.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemSneaker(player_t *player, ticcmd_t *cmd)
{
	if ((player->kartstuff[k_offroad] && K_ApplyOffroad(player)) // Stuck in offroad, use it NOW
		|| K_GetWaypointIsShortcut(player->nextwaypoint) == true // Going toward a shortcut!
		|| player->speed < K_GetKartSpeed(player, false)/2 // Being slowed down too much
		|| player->kartstuff[k_speedboost] > (FRACUNIT/8) // Have another type of boost (tethering)
		|| player->botvars.itemconfirm > 4*TICRATE) // Held onto it for too long
	{
		if (!player->kartstuff[k_sneakertimer] && !(player->pflags & PF_ATTACKDOWN))
		{
			cmd->buttons |= BT_ATTACK;
			player->botvars.itemconfirm = 2*TICRATE;
		}
	}
	else
	{
		player->botvars.itemconfirm++;
	}
}

/*--------------------------------------------------
	static void K_BotItemRocketSneaker(player_t *player, ticcmd_t *cmd)

		Item usage for rocket sneakers.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRocketSneaker(player_t *player, ticcmd_t *cmd)
{
	if (player->botvars.itemconfirm > TICRATE)
	{
		if (!player->kartstuff[k_sneakertimer] && !(player->pflags & PF_ATTACKDOWN))
		{
			cmd->buttons |= BT_ATTACK;
			player->botvars.itemconfirm = 0;
		}
	}
	else
	{
		player->botvars.itemconfirm++;
	}
}


/*--------------------------------------------------
	static void K_BotItemBanana(player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for trap item throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemBanana(player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	SINT8 throwdir = -1;

	player->botvars.itemconfirm++;

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		player->botvars.itemconfirm += player->botvars.difficulty / 2;
		throwdir = -1;
	}
	else
	{
		if (K_PlayerPredictThrow(player, 0))
		{
			player->botvars.itemconfirm += player->botvars.difficulty * 2;
			throwdir = 1;
		}
	}

	if (K_PlayerInCone(player, player->mo->radius * 16, 10, true))
	{
		player->botvars.itemconfirm += player->botvars.difficulty;
		throwdir = -1;
	}

	if (player->botvars.itemconfirm > 2*TICRATE || player->kartstuff[k_bananadrag] >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemMine(player_t *player, ticcmd_t *cmd, INT16 turnamt)

		Item usage for trap item throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.
		turnamt - How hard they currently are turning.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemMine(player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	SINT8 throwdir = 0;

	player->botvars.itemconfirm++;

	if (K_PlayerInCone(player, player->mo->radius * 16, 10, true))
	{
		player->botvars.itemconfirm += player->botvars.difficulty;
		throwdir = -1;
	}

	if (abs(turnamt) >= KART_FULLTURN/2)
	{
		player->botvars.itemconfirm += player->botvars.difficulty / 2;
		throwdir = -1;
	}
	else
	{
		if (K_PlayerPredictThrow(player, 0))
		{
			player->botvars.itemconfirm += player->botvars.difficulty * 2;
			throwdir = 0;
		}

		if (K_PlayerPredictThrow(player, 1))
		{
			player->botvars.itemconfirm += player->botvars.difficulty * 2;
			throwdir = 1;
		}
	}

	

	if (player->botvars.itemconfirm > 2*TICRATE || player->kartstuff[k_bananadrag] >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemEggman(player_t *player, ticcmd_t *cmd)

		Item usage for Eggman item throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemEggman(player_t *player, ticcmd_t *cmd)
{
	const UINT8 stealth = K_EggboxStealth(player->mo->x, player->mo->y);
	SINT8 throwdir = -1;

	player->botvars.itemconfirm++;

	if (K_PlayerPredictThrow(player, 0))
	{
		player->botvars.itemconfirm += player->botvars.difficulty / 2;
		throwdir = 1;
	}

	if (K_PlayerInCone(player, player->mo->radius * 16, 10, true))
	{
		player->botvars.itemconfirm += player->botvars.difficulty;
		throwdir = -1;
	}

	if (stealth > 1 || player->kartstuff[k_itemroulette] > 0)
	{
		player->botvars.itemconfirm += player->botvars.difficulty * 4;
		throwdir = -1;
	}

	if (player->botvars.itemconfirm > 2*TICRATE || player->kartstuff[k_bananadrag] >= TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static boolean K_BotRevealsEggbox(player_t *player)

		Decides if a bot is ready to place their Eggman item or not.

	Input Arguments:-
		player - Bot that has the eggbox.

	Return:-
		true if we want the bot to reveal their eggbox, otherwise false.
--------------------------------------------------*/
static boolean K_BotRevealsEggbox(player_t *player)
{
	const UINT8 stealth = K_EggboxStealth(player->mo->x, player->mo->y);

	// This is a stealthy spot for an eggbox, lets reveal it!
	if (stealth > 1)
	{
		return true;
	}

	// Check the predicted throws.
	if (K_PlayerPredictThrow(player, 0))
	{
		return true;
	}

	// Check your behind.
	if (K_PlayerInCone(player, player->mo->radius * 16, 10, true))
	{
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static void K_BotItemEggmanShield(player_t *player, ticcmd_t *cmd)

		Item usage for Eggman shields.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemEggmanShield(player_t *player, ticcmd_t *cmd)
{
	if (player->kartstuff[k_eggmanheld])
	{
		return;
	}

	if (K_BotRevealsEggbox(player) || (player->botvars.itemconfirm++ > 20*TICRATE))
	{
		K_BotGenericPressItem(player, cmd, 0);
	}
}

/*--------------------------------------------------
	static void K_BotItemEggmanExplosion(player_t *player, ticcmd_t *cmd)

		Item usage for Eggman explosions.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemEggmanExplosion(player_t *player, ticcmd_t *cmd)
{
	if (player->kartstuff[k_position] == 1)
	{
		cmd->forwardmove /= 2;
		cmd->buttons |= BT_BRAKE;
	}

	K_BotUseItemNearPlayer(player, cmd, 128*player->mo->scale);
}

/*--------------------------------------------------
	static void K_BotItemOrbinaut(player_t *player, ticcmd_t *cmd)

		Item usage for Orbinaut throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemOrbinaut(player_t *player, ticcmd_t *cmd)
{
	const fixed_t topspeed = K_GetKartSpeed(player, false);
	fixed_t radius = (player->mo->radius * 32);
	SINT8 throwdir = -1;

	if (player->speed > topspeed)
	{
		radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
	}

	player->botvars.itemconfirm++;

	if (K_PlayerInCone(player, radius, 10, false))
	{
		player->botvars.itemconfirm += player->botvars.difficulty * 2;
		throwdir = 1;
	}
	else if (K_PlayerInCone(player, radius, 10, true))
	{
		player->botvars.itemconfirm += player->botvars.difficulty;
		throwdir = -1;
	}

	if (player->botvars.itemconfirm > 5*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemJawz(player_t *player, ticcmd_t *cmd)

		Item usage for Jawz throwing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemJawz(player_t *player, ticcmd_t *cmd)
{
	const fixed_t topspeed = K_GetKartSpeed(player, false);
	fixed_t radius = (player->mo->radius * 32);
	SINT8 throwdir = 1;

	if (player->speed > topspeed)
	{
		radius = FixedMul(radius, FixedDiv(player->speed, topspeed));
	}

	player->botvars.itemconfirm++;

	if (K_PlayerInCone(player, radius, 10, true))
	{
		player->botvars.itemconfirm += player->botvars.difficulty;
		throwdir = -1;
	}

	if (player->kartstuff[k_lastjawztarget] != -1)
	{
		player->botvars.itemconfirm += player->botvars.difficulty * 2;
		throwdir = 1;
	}

	if (player->botvars.itemconfirm > 5*TICRATE)
	{
		K_BotGenericPressItem(player, cmd, throwdir);
	}
}

/*--------------------------------------------------
	static void K_BotItemThunder(player_t *player, ticcmd_t *cmd)

		Item usage for Thunder Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemThunder(player_t *player, ticcmd_t *cmd)
{
	if (!K_BotUseItemNearPlayer(player, cmd, 192*player->mo->scale))
	{
		if (player->botvars.itemconfirm > 10*TICRATE)
		{
			K_BotGenericPressItem(player, cmd, 0);
		}
		else
		{
			player->botvars.itemconfirm++;
		}
	}
}

/*--------------------------------------------------
	static void K_BotItemBubble(player_t *player, ticcmd_t *cmd)

		Item usage for Bubble Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemBubble(player_t *player, ticcmd_t *cmd)
{
	boolean hold = false;

	if (player->kartstuff[k_bubbleblowup] <= 0)
	{
		UINT8 i;

		player->botvars.itemconfirm++;

		if (player->kartstuff[k_bubblecool] <= 0)
		{
			const fixed_t radius = 192 * player->mo->scale;

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
					hold = true;
					break;
				}
			}
		}
	}
	else if (player->kartstuff[k_bubbleblowup] >= bubbletime)
	{
		if (player->botvars.itemconfirm >= 10*TICRATE)
		{
			hold = true;
		}
	}
	else if (player->kartstuff[k_bubbleblowup] < bubbletime)
	{
		hold = true;
	}

	if (hold && player->kartstuff[k_holdready])
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemFlame(player_t *player, ticcmd_t *cmd)

		Item usage for Flame Shield.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemFlame(player_t *player, ticcmd_t *cmd)
{
	if (player->botvars.itemconfirm > 0)
	{
		player->botvars.itemconfirm--;
	}
	else if (player->kartstuff[k_holdready])
	{
		INT32 flamemax = player->kartstuff[k_flamelength] * flameseg;

		if (player->kartstuff[k_flamemeter] < flamemax || flamemax == 0)
		{
			cmd->buttons |= BT_ATTACK;
		}
		else
		{
			player->botvars.itemconfirm = 3*flamemax/4;
		}
	}
}

/*--------------------------------------------------
	static void K_BotItemRings(player_t *player, ticcmd_t *cmd)

		Item usage for rings.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRings(player_t *player, ticcmd_t *cmd)
{
	INT32 saferingsval = 16 - K_GetKartRingPower(player);

	if (player->speed < K_GetKartSpeed(player, false)/2 // Being slowed down too much
		|| player->kartstuff[k_speedboost] > (FRACUNIT/5)) // Have another type of boost (tethering)
	{
		saferingsval -= 5;
	}

	if (player->rings > saferingsval)
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	static void K_BotItemRouletteMash(player_t *player, ticcmd_t *cmd)

		Item usage for item roulette mashing.

	Input Arguments:-
		player - Bot to do this for.
		cmd - Bot's ticcmd to edit.

	Return:-
		None
--------------------------------------------------*/
static void K_BotItemRouletteMash(player_t *player, ticcmd_t *cmd)
{
	boolean mash = false;

	if (player->pflags & PF_ATTACKDOWN)
	{
		return;
	}

	if (player->rings < 0 && cv_superring.value)
	{
		// Uh oh, we need a loan!
		// It'll be better in the long run for bots to lose an item set for 10 free rings.
		mash = true;
	}

	// TODO: Mash based on how far behind you are, when items are
	// almost garantueed to be in your favor.

	if (mash == true)
	{
		cmd->buttons |= BT_ATTACK;
	}
}

/*--------------------------------------------------
	void K_BotItemUsage(player_t *player, ticcmd_t *cmd, INT16 turnamt)

		See header file for description.
--------------------------------------------------*/
void K_BotItemUsage(player_t *player, ticcmd_t *cmd, INT16 turnamt)
{
	if (player->kartstuff[k_userings] == 1)
	{
		// Use rings!

		if (leveltime > starttime && !player->exiting)
		{
			K_BotItemRings(player, cmd);
		}
	}
	else
	{
		if (player->botvars.itemdelay)
		{
			player->botvars.itemdelay--;
			player->botvars.itemconfirm = 0;
			return;
		}

		if (player->kartstuff[k_itemroulette])
		{
			// Mashing behaviors
			K_BotItemRouletteMash(player, cmd);
			return;
		}

		if (player->kartstuff[k_stealingtimer] == 0 && player->kartstuff[k_stolentimer] == 0)
		{
			if (player->kartstuff[k_eggmanexplode])
			{
				K_BotItemEggmanExplosion(player, cmd);
			}
			else if (player->kartstuff[k_eggmanheld])
			{
				K_BotItemEggman(player, cmd);
			}
			else if (player->kartstuff[k_rocketsneakertimer] > 0)
			{
				K_BotItemRocketSneaker(player, cmd);
			}
			else
			{
				switch (player->kartstuff[k_itemtype])
				{
					default:
						if (player->kartstuff[k_itemtype] != KITEM_NONE)
						{
							K_BotItemGenericTap(player, cmd);
						}

						player->botvars.itemconfirm = 0;
						break;
					case KITEM_INVINCIBILITY:
					case KITEM_SPB:
					case KITEM_GROW:
					case KITEM_SHRINK:
					case KITEM_HYUDORO:
					case KITEM_SUPERRING:
						K_BotItemGenericTap(player, cmd);
						break;
					case KITEM_ROCKETSNEAKER:
						if (player->kartstuff[k_rocketsneakertimer] <= 0)
						{
							K_BotItemGenericTap(player, cmd);
						}
						break;
					case KITEM_SNEAKER:
						K_BotItemSneaker(player, cmd);
						break;
					case KITEM_BANANA:
					case KITEM_LANDMINE:
						if (!player->kartstuff[k_itemheld])
						{
							K_BotItemGenericTrapShield(player, cmd, turnamt, false);
						}
						else
						{
							K_BotItemBanana(player, cmd, turnamt);
						}
						break;
					case KITEM_EGGMAN:
						K_BotItemEggmanShield(player, cmd);
						break;
					case KITEM_ORBINAUT:
						if (!player->kartstuff[k_itemheld])
						{
							K_BotItemGenericOrbitShield(player, cmd);
						}
						else if (player->kartstuff[k_position] != 1) // Hold onto orbiting items when in 1st :)
						/* FALL-THRU */
					case KITEM_BALLHOG:
						{
							K_BotItemOrbinaut(player, cmd);
						}
						break;
					case KITEM_JAWZ:
						if (!player->kartstuff[k_itemheld])
						{
							K_BotItemGenericOrbitShield(player, cmd);
						}
						else if (player->kartstuff[k_position] != 1) // Hold onto orbiting items when in 1st :)
						{
							K_BotItemJawz(player, cmd);
						}
						break;
					case KITEM_MINE:
						if (!player->kartstuff[k_itemheld])
						{
							K_BotItemGenericTrapShield(player, cmd, turnamt, true);
						}
						else
						{
							K_BotItemMine(player, cmd, turnamt);
						}
						break;
					case KITEM_THUNDERSHIELD:
						K_BotItemThunder(player, cmd);
						break;
					case KITEM_BUBBLESHIELD:
						K_BotItemBubble(player, cmd);
						break;
					case KITEM_FLAMESHIELD:
						K_BotItemFlame(player, cmd);
						break;
				}
			}
		}
	}
}
