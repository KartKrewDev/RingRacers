// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
// \brief Power Level system

#ifndef K_PWRLV_H
#define K_PWRLV_H

#include "doomtype.h"
#include "doomdef.h"
#include "d_player.h"
#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	PWRLV_DISABLED = -1,
	PWRLV_RACE = 0,
	PWRLV_BATTLE = 1,
	PWRLV_NUMTYPES = 2,
} pwrlv_type_t;

#define PWRLVRECORD_START 1000
#define PWRLVRECORD_MEDIAN 5000
#define PWRLVRECORD_MIN 1
#define PWRLVRECORD_MAX 9999

extern int8_t speedscramble;
extern int8_t encorescramble;

extern consvar_t cv_debugencorevote;

extern uint16_t clientpowerlevels[MAXPLAYERS][PWRLV_NUMTYPES];
extern int16_t clientPowerAdd[MAXPLAYERS];
extern uint8_t spectateGriefed;

int8_t K_UsingPowerLevels(void);
void K_ClearClientPowerLevels(void);
int16_t K_PowerLevelPlacementScore(player_t *player);
int16_t K_CalculatePowerLevelAvg(void);
void K_UpdatePowerLevels(player_t *player, uint8_t gradingpoint, dboolean forfeit);
void K_UpdatePowerLevelsFinalize(player_t *player, dboolean onForfeit);
int16_t K_FinalPowerIncrement(player_t *player, int16_t yourPower, int16_t increment);
void K_CashInPowerLevels(void);
void K_SetPowerLevelScrambles(int8_t powertype);
void K_PlayerForfeit(uint8_t playernum, dboolean nopointloss);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
