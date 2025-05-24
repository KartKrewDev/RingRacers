// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_credits.h
/// \brief Grand Prix podium cutscene

#ifndef __K_CREDITS__
#define __K_CREDITS__

#include "doomtype.h"
#include "d_event.h"

#ifdef __cplusplus
extern "C" {
#endif

void F_LoadCreditsDefinitions(void);

void F_CreditsReset(void);

void F_StartCredits(void);

void F_DeferContinueCredits(void);

boolean F_IsDeferredContinueCredits(void);

void F_ContinueCredits(void);

void F_TickCreditsDemoExit(void);

INT32 F_CreditsDemoExitFade(void);

void F_ConsiderCreditsMusicUpdate(void);

void F_CreditTicker(void);

void F_CreditDrawer(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_CREDITS__
