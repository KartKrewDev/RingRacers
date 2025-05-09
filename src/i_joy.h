// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_joy.h
/// \brief share joystick information with game control code

#ifndef __I_JOY_H__
#define __I_JOY_H__

#include "g_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
  \brief	-JOYAXISRANGE to +JOYAXISRANGE for each axis

	(1024-1) so we can do a right shift instead of division
	(doesnt matter anyway, just give enough precision)
	a gamepad will return -1, 0, or 1 in the event data
	an analog type joystick will return a value
	from -JOYAXISRANGE to +JOYAXISRANGE for each axis
*/

#define JOYAXISRANGE 1023

// detect a bug if we increase JOYBUTTONS above DIJOYSTATE's number of buttons
#if (JOYBUTTONS > 64)
"JOYBUTTONS is greater than INT64 bits can hold"
#endif

/**	\brief	The struct JoyType_s

 share some joystick information (maybe 2 for splitscreen), to the game input code,
 actually, we need to know if it is a gamepad or analog controls
*/

struct JoyType_t
{
	/*! if true, we MUST Poll() to get new joystick data,
	that is: we NEED the DIRECTINPUTDEVICE2 ! (watchout NT compatibility) */
	INT32 bJoyNeedPoll;
	/*! this joystick is a gamepad, read: digital axes
	if FALSE, interpret the joystick event data as JOYAXISRANGE (see above) */
	INT32 bGamepadStyle;

};
/**	\brief Joystick info
	for palyer[sic] 1-4's joystick/gamepad
*/

extern JoyType_t Joystick[MAXSPLITSCREENPLAYERS];

void I_SetGamepadPlayerIndex(INT32 device_id, INT32 index);
void I_SetGamepadIndicatorColor(INT32 device_id, UINT8 red, UINT8 green, UINT8 blue);
void I_GetGamepadGuid(INT32 device_id, char *out, int out_len);
void I_GetGamepadName(INT32 device_id, char *out, int out_len);
void I_GamepadRumble(INT32 device_id, UINT16 low_strength, UINT16 high_strength);
void I_GamepadRumbleTriggers(INT32 device_id, UINT16 left_strength, UINT16 right_strength);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __I_JOY_H__
