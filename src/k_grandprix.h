// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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
#include "k_rank.h" // gpRank_t

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	GPEVENT_NONE = 0,
	GPEVENT_BONUS,
	GPEVENT_SPECIAL,
} gpEvent_e;

// Please also see P_ArchiveMisc
extern struct grandprixinfo
{
	boolean gp;				///< If true, then we are in a Grand Prix.
	cupheader_t *cup;		///< Which cup are we playing?
	UINT8 gamespeed;		///< Copy of gamespeed, just to make sure you can't cheat it with cvars
	boolean encore;			///< Ditto, but for encore mode
	boolean masterbots;		///< If true, all bots should be max difficulty (Master Mode)
	boolean initalize;		///< If true, we need to initialize a new session.
	boolean wonround;		///< If false, then we retry the map instead of going to the next.
	gpEvent_e eventmode;	///< Special event mode, bots are set to spectate and a special gametype is played
	UINT32 specialDamage;	///< Accumulated Sealed Star difficulty reduction
	gpRank_t rank;			///< Struct containing grading information. (See also: k_rank.h)
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
	INT16 K_CalculateGPRankPoints(player_t* player, UINT8 numplayers);

		Calculates the number of points that a player would
		recieve if they won the round.

	Input Arguments:-
		position - Finishing position.
		numplayers - Number of players in the game.

	Return:-
		Number of points to give.
--------------------------------------------------*/

INT16 K_CalculateGPRankPoints(UINT16 exp, UINT8 position, UINT8 numplayers);


/*--------------------------------------------------
	UINT8 K_GetGPPlayerCount(UINT8 humans)

		Counts the number of total players,
		including humans and bots, to put into
		a GP session.

	Input Arguments:-
		humans - Number of human players.

	Return:-
		Number of both human players and CPU.
--------------------------------------------------*/

UINT8 K_GetGPPlayerCount(UINT8 humans);


/*--------------------------------------------------
	void K_InitGrandPrixBots(void);

		Spawns bots specifically tailored for Grand Prix mode.
--------------------------------------------------*/

void K_InitGrandPrixBots(void);


/*--------------------------------------------------
	void K_LoadGrandPrixSaveGame(void)

		Handles loading savedata_t info for Grand Prix context.
---------------------------------------------------*/

void K_LoadGrandPrixSaveGame(void);


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
	boolean K_CanChangeRules(boolean allowdemos);

		Returns whenver or not the server is allowed
		to change the game rules.

	Input Arguments:-
		allowdemos - permits this behavior during demo playback

	Return:-
		true if can change important gameplay rules, otherwise false.
--------------------------------------------------*/

boolean K_CanChangeRules(boolean allowdemos);


/*--------------------------------------------------
	boolean K_BotDefaultSpectator(void)

		Check whether bots should spectate this round.
--------------------------------------------------*/

boolean K_BotDefaultSpectator(void);

void K_AssignFoes(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
