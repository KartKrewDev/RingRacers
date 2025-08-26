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

#ifndef __K_PWRLV__
#define __K_PWRLV__

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

extern SINT8 speedscramble;
extern SINT8 encorescramble;

extern consvar_t cv_debugencorevote;

extern UINT16 clientpowerlevels[MAXPLAYERS][PWRLV_NUMTYPES];
extern INT16 clientPowerAdd[MAXPLAYERS];
extern UINT8 spectateGriefed;

SINT8 K_UsingPowerLevels(void);
void K_ClearClientPowerLevels(void);
INT16 K_PowerLevelPlacementScore(player_t *player);
INT16 K_CalculatePowerLevelAvg(void);
void K_UpdatePowerLevels(player_t *player, UINT8 gradingpoint, boolean forfeit);
void K_UpdatePowerLevelsFinalize(player_t *player, boolean onForfeit);
INT16 K_FinalPowerIncrement(player_t *player, INT16 yourPower, INT16 increment);
void K_CashInPowerLevels(void);
void K_SetPowerLevelScrambles(SINT8 powertype);
void K_PlayerForfeit(UINT8 playernum, boolean nopointloss);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
