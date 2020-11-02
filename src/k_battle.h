#ifndef __K_BATTLE__
#define __K_BATTLE__

#include "doomtype.h"
#include "d_player.h"

extern struct battleovertime
{
	UINT16 enabled; ///< Has this been initalized yet?
	fixed_t radius; ///< Radius of kill field
	fixed_t x, y, z; ///< Position to center on
} battleovertime;

extern boolean battlecapsules;
extern INT32 nummapboxes, numgotboxes; // keep track of spawned battle mode items
extern UINT8 maptargets, numtargets;

INT32 K_StartingBumperCount(void);
boolean K_IsPlayerWanted(player_t *player);
void K_CalculateBattleWanted(void);
void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount);
void K_CheckBumpers(void);
mobj_t *K_SpawnChaosEmerald(mobj_t *parent, angle_t angle, SINT8 flip, UINT32 emeraldType);
void K_RunPaperItemSpawners(void);
void K_RunBattleOvertime(void);
void K_SetupMovingCapsule(mapthing_t *mt, mobj_t *mobj);
void K_SpawnBattleCapsules(void);

#endif
