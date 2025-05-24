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
/// \file  d_event.h
/// \brief Event handling

#ifndef __D_EVENT__
#define __D_EVENT__

#include "doomtype.h"
#include "g_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Input event types.
typedef enum
{
	ev_keydown,
	ev_keyup,
	ev_console,
	ev_mouse,
	ev_gamepad_axis,
	ev_gamepad_device_added,
	ev_gamepad_device_removed,
} evtype_t;

// Event structure.
struct event_t
{
	evtype_t type;
	INT32 data1; // keys / mouse/joystick buttons
	INT32 data2; // mouse/joystick x move; key repeat
	INT32 data3; // mouse/joystick y move
	INT32 device; // which device ID it belongs to (controller ID)
};

//
// GLOBAL VARIABLES
//
#define MAXEVENTS 128

extern event_t events[MAXEVENTS];
extern INT32 eventhead, eventtail;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
