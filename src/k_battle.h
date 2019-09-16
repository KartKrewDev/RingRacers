#ifndef __K_BATTLE__
#define __K_BATTLE__

#include "doomtype.h"
#include "d_player.h"

boolean K_IsPlayerWanted(player_t *player);
void K_CalculateBattleWanted(void);
void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount);
void K_CheckBumpers(void);
void K_SpawnBattleCapsules(void);

#endif
