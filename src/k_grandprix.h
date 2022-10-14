// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_grandprix.h
/// \brief Grand Prix mode game logic & bot behaviors

#ifndef __K_GRANDPRIX__
#define __K_GRANDPRIX__

#include "doomdef.h"
#include "doomstat.h"

#define GPEVENT_NONE 0
#define GPEVENT_BONUS 1
#define GPEVENT_SPECIAL 2

extern struct grandprixinfo
{
	boolean gp;				///< If true, then we are in a Grand Prix.
	UINT8 roundnum;			///< Round number. If 0, this is a single session from the warp command.
	cupheader_t *cup;		///< Which cup are we playing?
	UINT8 gamespeed;		///< Copy of gamespeed, just to make sure you can't cheat it with cvars
	boolean encore;			///< Ditto, but for encore mode
	boolean masterbots;		///< If true, all bots should be max difficulty (Master Mode)
	boolean initalize;		///< If true, we need to initialize a new session.
	boolean wonround;		///< If false, then we retry the map instead of going to the next.
	UINT8 eventmode;		///< See GPEVENT_ constants
} grandprixinfo;


/*--------------------------------------------------
	UINT8 K_BotStartingDifficulty(SINT8 value);

		Determines the starting difficulty of the bots
		for a specific game speed.

	Input Arguments:-
		value - Game speed value to use.

	Return:-
		Bot difficulty level.
--------------------------------------------------*/

UINT8 K_BotStartingDifficulty(SINT8 value);


/*--------------------------------------------------
	INT16 K_CalculateGPRankPoints(UINT8 position, UINT8 numplayers);

		Calculates the number of points that a player would
		recieve if they won the round.

	Input Arguments:-
		position - Finishing position.
		numplayers - Number of players in the game.

	Return:-
		Number of points to give.
--------------------------------------------------*/

INT16 K_CalculateGPRankPoints(UINT8 position, UINT8 numplayers);


/*--------------------------------------------------
	UINT8 K_BotDefaultSkin(void);

		Returns the skin number of the skin the game
		uses as a fallback option.
--------------------------------------------------*/

UINT8 K_BotDefaultSkin(void);


/*--------------------------------------------------
	void K_InitGrandPrixBots(void);

		Spawns bots specifically tailored for Grand Prix mode.
--------------------------------------------------*/

void K_InitGrandPrixBots(void);


/*--------------------------------------------------
	void K_UpdateGrandPrixBots(void);

		Updates bot settings based on the the results of the race.
--------------------------------------------------*/

void K_UpdateGrandPrixBots(void);


/*--------------------------------------------------
	void K_IncreaseBotDifficulty(player_t *bot);

		Increases the difficulty of this bot when they finish the race.

	Input Arguments:-
		bot - Player to do this for.

	Return:-
		None
--------------------------------------------------*/

void K_IncreaseBotDifficulty(player_t *bot);


/*--------------------------------------------------
	void K_RetireBots(void);

		Replaces PF_NOCONTEST bots, by refreshing their difficulty
		and changing their skin.
--------------------------------------------------*/

void K_RetireBots(void);


/*--------------------------------------------------
	void K_FakeBotResults(player_t *bot);

		Fakes the results of the race, when all human players have
		already finished and only bots were remaining.

	Input Arguments:-
		bot - Player to do this for.

	Return:-
		None
--------------------------------------------------*/

void K_FakeBotResults(player_t *bot);


/*--------------------------------------------------
	void K_PlayerLoseLife(player_t *player);

		Removes a life from a human player.

	Input Arguments:-
		player - Player to do this for.

	Return:-
		None
--------------------------------------------------*/

void K_PlayerLoseLife(player_t *player);


/*--------------------------------------------------
	boolean K_CanChangeRules(void);

		Returns whenver or not the server is allowed
		to change the game rules.

	Input Arguments:-
		None

	Return:-
		true if can change important gameplay rules, otherwise false.
--------------------------------------------------*/

boolean K_CanChangeRules(void);

#endif
