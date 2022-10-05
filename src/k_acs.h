// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_acs.h
/// \brief Action Code Script implementation using ACSVM

#ifndef __K_ACS__
#define __K_ACS__

#include "doomtype.h"
#include "doomdef.h"

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

typedef enum
{
	ACS_ST_OPEN			=  1, // OPEN: Runs once when the level starts.
	ACS_ST_RESPAWN		=  2, // RESPAWN: Runs when a player respawns.
	ACS_ST_DEATH		=  3, // DEATH: Runs when a player dies.
	ACS_ST_ENTER		=  4, // ENTER: Runs when a player enters the game; both on start of the level, and when un-spectating.
} acs_scriptType_e;

ACSVM_Environment *ACS_GetEnvironment(void);

void ACS_Init(void);
void ACS_Shutdown(void);
void ACS_LoadLevelScripts(size_t mapID);
void ACS_Tick(void);

bool ACS_CF_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
bool ACS_CF_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);

#endif // __K_ACS__
