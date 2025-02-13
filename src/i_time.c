// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2022 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_time.c
/// \brief Timing for the system layer.

#include "i_time.h"

#include <math.h>

#include "command.h"
#include "doomtype.h"
#include "d_netcmd.h"
#include "f_finale.h"
#include "g_demo.h"
#include "m_fixed.h"
#include "i_system.h"

timestate_t g_time;

static precise_t enterprecise, oldenterprecise;
static fixed_t entertic, oldentertics;
static double tictimer;

// A little more than the minimum sleep duration on Windows.
// May be incorrect for other platforms, but we don't currently have a way to
// query the scheduler granularity. SDL will do what's needed to make this as
// low as possible though.
#define MIN_SLEEP_DURATION_MS 2.1

tic_t I_GetTime(void)
{
	return g_time.time;
}

void I_InitializeTime(void)
{
	g_time.time = 0;
	g_time.timefrac = 0;

	enterprecise = 0;
	oldenterprecise = 0;
	tictimer = 0.0;

	{
		extern struct CVarList *cvlist_timer;
		CV_RegisterList(cvlist_timer);
	}

	// I_StartupTimer is preserved for potential subsystems that need to setup
	// timing information for I_GetPreciseTime and sleeping
	I_StartupTimer();
}

fixed_t I_GetTimeScale(void)
{
	if (demo.playback && demo.attract == DEMO_ATTRACT_TITLE && F_AttractDemoExitFade())
	{
		// Slow down at the end of attract demos
		return FRACUNIT/2;
	}

	return cv_timescale.value;
}

void I_UpdateTime(void)
{
	double ticratescaled;
	double elapsedseconds;
	tic_t realtics;

	// get real tics
	ticratescaled = (double)TICRATE * FIXED_TO_FLOAT(I_GetTimeScale());

	enterprecise = I_GetPreciseTime();
	elapsedseconds = (double)(enterprecise - oldenterprecise) / I_GetPrecisePrecision();
	tictimer += elapsedseconds;
	while (tictimer > 1.0/ticratescaled)
	{
		entertic += 1;
		tictimer -= 1.0/ticratescaled;
	}
	realtics = entertic - oldentertics;
	oldentertics = entertic;
	oldenterprecise = enterprecise;

	// Update global time state
	g_time.time += realtics;
	{
		double fractional, integral;
		fractional = modf(tictimer * ticratescaled, &integral);
		g_time.timefrac = FLOAT_TO_FIXED(fractional);
	}
}

void I_SleepDuration(precise_t duration)
{
	UINT64 precision = I_GetPrecisePrecision();
	INT32 sleepvalue = cv_sleep.value;
	UINT64 delaygranularity;
	precise_t cur;
	precise_t dest;

	{
		double gran = round(((double)(precision / 1000) * sleepvalue * MIN_SLEEP_DURATION_MS));
		delaygranularity = (UINT64)gran;
	}

	cur = I_GetPreciseTime();
	dest = cur + duration;

	// the reason this is not dest > cur is because the precise counter may wrap
	// two's complement arithmetic is our friend here, though!
	// e.g. cur 0xFFFFFFFFFFFFFFFE = -2, dest 0x0000000000000001 = 1
	// 0x0000000000000001 - 0xFFFFFFFFFFFFFFFE = 3
	while ((INT64)(dest - cur) > 0)
	{
		// If our cv_sleep value exceeds the remaining sleep duration, use the
		// hard sleep function.
		if (sleepvalue > 0 && (dest - cur) > delaygranularity)
		{
			I_Sleep(sleepvalue);
		}

		// Otherwise, this is a spinloop.

		cur = I_GetPreciseTime();
	}
}
