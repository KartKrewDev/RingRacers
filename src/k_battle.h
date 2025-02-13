// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __K_BATTLE__
#define __K_BATTLE__

#include "doomtype.h"
#include "d_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BATTLE_SPAWN_INTERVAL (4*TICRATE)
#define BATTLE_DESPAWN_TIME (15*TICRATE)
#define BATTLE_POWERUP_TIME (30*TICRATE)
#define BATTLE_POWERUP_DROPPED_TIME (15*TICRATE)
#define BATTLE_UFO_TIME (20*TICRATE)

extern struct battleovertime
{
	UINT16 enabled; ///< Has this been initalized yet?
	fixed_t radius; ///< Radius of kill field
	fixed_t initial_radius; ///< Starting radius of kill field
	tic_t start; ///< Leveltime to decrease kill field radius from
	fixed_t x, y, z; ///< Position to center on
} battleovertime;

extern struct battleufo
{
	INT32 previousId;
	tic_t due;
} g_battleufo;

extern boolean battleprisons;
extern INT32 nummapboxes, numgotboxes; // keep track of spawned battle mode items
extern UINT8 maptargets, numtargets;
extern tic_t g_emeraldWin;

INT32 K_StartingBumperCount(void);
boolean K_IsPlayerWanted(player_t *player);
#define K_CalculateBattleWanted() (void)0 // not nulled out so we know where we need to recalculate some other form of battle mode importance
void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount);
void K_CheckBumpers(void);
void K_CheckEmeralds(player_t *player);
UINT16 K_GetChaosEmeraldColor(UINT32 emeraldType);
mobj_t *K_SpawnChaosEmerald(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT32 emeraldType);
mobj_t *K_SpawnSphereBox(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 amount);
void K_DropEmeraldsFromPlayer(player_t *player, UINT32 emeraldType);
UINT8 K_NumEmeralds(player_t *player);
void K_RunPaperItemSpawners(void);
void K_SpawnOvertimeBarrier(void);
void K_RunBattleOvertime(void);
void K_SetupMovingCapsule(mapthing_t *mt, mobj_t *mobj);
void K_SpawnPlayerBattleBumpers(player_t *p);
void K_BattleInit(boolean singleplayercontext);
UINT8 K_Bumpers(player_t *player);
INT32 K_BumpersToHealth(UINT8 bumpers);
boolean K_BattleOvertimeKiller(mobj_t *mobj);
boolean K_EndBattleRound(player_t *victor);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
