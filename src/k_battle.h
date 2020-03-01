#ifndef __K_BATTLE__
#define __K_BATTLE__

#include "doomtype.h"
#include "d_player.h"

extern struct battleovertime
{
	UINT16 enabled; ///< Has this been initalized yet?
	fixed_t radius, minradius; ///< Radius of kill field
	fixed_t x, y, z; ///< Position to center on
} battleovertime;

extern boolean battlecapsules;
extern UINT8 maptargets, numtargets;

boolean K_IsPlayerWanted(player_t *player);
void K_CalculateBattleWanted(void);
void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount);
void K_CheckBumpers(void);
void K_RunBattleOvertime(void);
void K_SpawnBattleCapsules(void);

#endif
