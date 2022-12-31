// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_specialstage.h
/// \brief Special Stage game logic

#ifndef __K_SPECIALSTAGE__
#define __K_SPECIALSTAGE__

#include "doomdef.h"
#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct specialStage
{
	boolean active;						///< If true, then we are in a special stage
	boolean encore;						///< Copy of encore, just to make sure you can't cheat it with cvars

	UINT32 beamDist;					///< Where the exit beam is.
	mobj_t *ufo;						///< The Chaos Emerald capsule.
} specialStage;

/*--------------------------------------------------
	void K_ResetSpecialStage(void);

		Resets Special Stage information to a clean slate.
--------------------------------------------------*/

void K_ResetSpecialStage(void);


/*--------------------------------------------------
	void K_InitSpecialStage(void);

		Initializes Special Stage data on map load.
--------------------------------------------------*/

void K_InitSpecialStage(void);


/*--------------------------------------------------
	void K_TickSpecialStage(void);

		Updates Special Stage data each frame.
--------------------------------------------------*/

void K_TickSpecialStage(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
