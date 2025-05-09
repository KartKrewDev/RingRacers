// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_time.h
/// \brief Timing for the system layer.

#ifndef __I_TIME_H__
#define __I_TIME_H__

#include "command.h"
#include "doomtype.h"
#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

struct timestate_t {
	tic_t time;
	fixed_t timefrac;
};

extern timestate_t g_time;
extern consvar_t cv_timescale;

/**	\brief  Called by D_SRB2Loop, returns current time in game tics.
*/
tic_t I_GetTime(void);

/**	\brief  Initializes timing system.
*/
void I_InitializeTime(void);

void I_UpdateTime(void);
fixed_t I_GetTimeScale(void);

/** \brief  Block for at minimum the duration specified. This function makes a
            best effort not to oversleep, and will spinloop if sleeping would
			take too long. However, callers should still check the current time
			after this returns.
*/
void I_SleepDuration(precise_t duration);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __I_TIME_H__
