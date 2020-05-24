// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bot.h
/// \brief Bot logic & ticcmd generation code

#ifndef __K_BOT__
#define __K_BOT__

#include "k_waypoint.h"
#include "d_player.h"

// Maximum value of botvars.difficulty
#define MAXBOTDIFFICULTY 9

// How many tics in a row do you need to turn in this direction before we'll let you turn.
// Made it as small as possible without making it look like the bots are twitching constantly.
#define BOTTURNCONFIRM 4

// Point for bots to aim for
typedef struct botprediction_s {
	fixed_t x, y;
	fixed_t radius;
	angle_t dir;
} botprediction_t;


// AVAILABLE FOR LUA


/*--------------------------------------------------
	boolean K_PlayerUsesBotMovement(player_t *player);

		Tells if this player is being controlled via bot movement code (is a bot, or is exiting).

	Input Arguments:-
		player - Player to check.

	Return:-
		true if using bot movement code, otherwise false.
--------------------------------------------------*/

boolean K_PlayerUsesBotMovement(player_t *player);


/*--------------------------------------------------
	boolean K_BotCanTakeCut(player_t *player);

		Tells if this bot is able to take shortcuts (currently unaffected by offroad,
		or has certain items ready).

	Input Arguments:-
		player - Player to check.

	Return:-
		true if able to take shortcuts, otherwise false.
--------------------------------------------------*/

boolean K_BotCanTakeCut(player_t *player);


/*--------------------------------------------------
	fixed_t K_BotRubberband(player_t *player);

		Gives a multiplier for a bot's rubberbanding. Meant to be used for top speed,
		acceleration, and handling.

	Input Arguments:-
		player - Player to check.

	Return:-
		A multiplier in fixed point scale, between 0.875 and 2.0.
--------------------------------------------------*/

fixed_t K_BotRubberband(player_t *player);


/*--------------------------------------------------
	fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy);

		Gets the distance of a point away from a line.
		TODO: Could go in another file?

	Input Arguments:-
		v1x - Line's first vertex x position.
		v1y - Line's first vertex y position.
		v2x - Line's second vertex x position.
		v2y - Line's second vertex y position.
		cx - Point's x position.
		cy - Point's y position.

	Return:-
		The distance between the point and the line.
--------------------------------------------------*/

fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy);


// NOT AVAILABLE FOR LUA


/*--------------------------------------------------
	boolean K_AddBot(UINT8 skin, UINT8 difficulty, UINT8 *newplayernum);

		Returns the waypoint actually being used as the finish line.

	Input Arguments:-
		skin - Skin number that the bot will use.
		difficulty - Difficulty level this bot will use.
		newplayernum - Pointer to the last valid player slot number.
			Is a pointer so that this function can be called multiple times to add more than one bot.

	Return:-
		true if a bot packet can be sent, otherwise false.
--------------------------------------------------*/

boolean K_AddBot(UINT8 skin, UINT8 difficulty, UINT8 *newplayernum);


/*--------------------------------------------------
	void K_UpdateMatchRaceBots(void);

		Updates the number of bots in the server and their difficulties for Match Race.
--------------------------------------------------*/

void K_UpdateMatchRaceBots(void);


/*--------------------------------------------------
	UINT8 K_EggboxStealth(fixed_t x, fixed_t y);

		Gets a "stealth" value for a position, to figure out how
		well Eggman boxes blend into random items.

	Input Arguments:-
		x - X coordinate to check.
		y - Y coordinate to check.

	Return:-
		Stealth value for the position.
--------------------------------------------------*/

UINT8 K_EggboxStealth(fixed_t x, fixed_t y);


/*--------------------------------------------------
	fixed_t K_BotReducePrediction(player_t *player);

		Finds walls nearby the specified player, to create a multiplier
		to pull bot predictions back by.

	Input Arguments:-
		player - Player to compare.

	Return:-
		Multiplier in fixed point scale.
--------------------------------------------------*/

fixed_t K_BotReducePrediction(player_t *player);


/*--------------------------------------------------
	INT16 K_BotFindObjects(player_t *player, INT16 turn);

		Generates a sum for objects to steer towards/away from.

	Input Arguments:-
		player - Player to compare.
		turn - Turn value before object steering.

	Return:-
		Turn amount sum to add to final product.
--------------------------------------------------*/

INT16 K_BotFindObjects(player_t *player, INT16 turn);


/*--------------------------------------------------
	void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd);

		Gives a multiplier for a bot's rubberbanding. Meant to be used for top speed,
		acceleration, and handling.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.

	Return:-
		None
--------------------------------------------------*/

void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd);


/*--------------------------------------------------
	void K_BotItemUsage(player_t *player, ticcmd_t *cmd, INT16 turnamt);

		Item usage part of ticcmd generation.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		turnamt - How hard the bot is turning.

	Return:-
		None
--------------------------------------------------*/

void K_BotItemUsage(player_t *player, ticcmd_t *cmd, INT16 turnamt);


#endif
