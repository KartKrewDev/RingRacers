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
	boolean initbots; ///< If true, we need to initialize the bots that are competing.
} grandprixinfo;

void K_InitGrandPrixBots(void);
void K_FakeBotResults(player_t *bot);

#endif

