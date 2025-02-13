// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  am_map.h
/// \brief Code for the 'automap', former Doom feature used for DEVMODE testing

#ifndef __AMMAP_H__
#define __AMMAP_H__

#include "d_event.h"

#ifdef __cplusplus
extern "C" {
#endif

struct fpoint_t
{
	INT32 x, y;
};

struct fline_t
{
	fpoint_t a, b;
};

extern boolean am_recalc; // true if screen size changes
extern boolean automapactive; // In AutoMap mode?

// Called by main loop.
boolean AM_Responder(event_t *ev);

// Called by main loop.
void AM_Ticker(void);

// Called by main loop, instead of view drawer if automap is active.
void AM_Drawer(void);

// Enables the automap.
void AM_Start(void);

// Called to force the automap to quit if the level is completed while it is up.
void AM_Stop(void);

struct minigen_t
{
	INT32 w, h;
	UINT8 *buf;
};

// Minimap generation
minigen_t *AM_MinimapGenerate(INT32 mul);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
