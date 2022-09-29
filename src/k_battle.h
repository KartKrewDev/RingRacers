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
#define K_CalculateBattleWanted() (void)0 // not nulled out so we know where we need to recalculate some other form of battle mode importance
void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount);
void K_CheckBumpers(void);
void K_CheckEmeralds(player_t *player);
mobj_t *K_SpawnChaosEmerald(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT32 emeraldType);
mobj_t *K_SpawnSphereBox(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 amount);
void K_DropEmeraldsFromPlayer(player_t *player, UINT32 emeraldType);
UINT8 K_NumEmeralds(player_t *player);
void K_RunPaperItemSpawners(void);
void K_RunBattleOvertime(void);
void K_SetupMovingCapsule(mapthing_t *mt, mobj_t *mobj);
void K_SpawnPlayerBattleBumpers(player_t *p);
void K_BattleInit(void);

#endif
