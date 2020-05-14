#ifndef __K_GRANDPRIX__
#define __K_GRANDPRIX__

#include "doomdef.h"
#include "doomstat.h"

extern struct grandprixinfo
{
	UINT8 roundnum; ///< Round number -- if 0, then we're not in a Grand Prix.
	cupheader_t *cup; ///< Which cup are we playing?
	UINT8 gamespeed; ///< Copy of gamespeed, just to make sure you can't cheat it with cvars
	boolean encore; ///< Ditto, but for encore mode
	boolean masterbots; ///< If true, all bots should be max difficulty (Master Mode)
	boolean initalize; ///< If true, we need to initialize a new cup.
	boolean wonround; ///< If false, then we retry the map instead of going to the next.
} grandprixinfo;

UINT8 K_BotStartingDifficulty(SINT8 value);
INT16 K_CalculateGPRankPoints(UINT8 position, UINT8 numplayers);
void K_InitGrandPrixBots(void);
void K_UpdateGrandPrixBots(void);
void K_IncreaseBotDifficulty(player_t *bot);
void K_FakeBotResults(player_t *bot);
void K_PlayerLoseLife(player_t *player);

#endif
