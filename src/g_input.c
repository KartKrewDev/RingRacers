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
/// \file  g_input.c
/// \brief handle mouse/keyboard/joystick inputs,
///        maps inputs to game controls (forward, spin, jump...)

#include "doomdef.h"
#include "doomstat.h"
#include "g_input.h"
#include "keys.h"
#include "k_menu.h"
#include "hu_stuff.h" // need HUFONT start & end
#include "d_net.h"
#include "console.h"
#include "i_joy.h" // JOYAXISRANGE
#include "r_draw.h" // GTC_ macros for assigning gamepad indicator colors
#include "v_video.h" // V_GetColor for assigning gamepad indictaor colors
#include "r_skins.h" // skins[].prefcolor for assigning gamepad indicator colors
#include "z_zone.h"

// current state of the keys
// FRACUNIT for fully pressed, 0 for not pressed
INT32 gamekeydown[MAXDEVICES][NUMINPUTS];

// two key codes (or virtual key) per game control
INT32 gamecontrol[MAXSPLITSCREENPLAYERS][num_gamecontrols][MAXINPUTMAPPING];
UINT8 gamecontrolflags[MAXSPLITSCREENPLAYERS];
UINT8 showgamepadprompts[MAXSPLITSCREENPLAYERS];
INT32 gamecontroldefault[num_gamecontrols][MAXINPUTMAPPING]; // default control storage
INT32 menucontrolreserved[num_gamecontrols][MAXINPUTMAPPING];

// lists of GC codes for selective operation
/*
const INT32 gcl_accelerate[num_gcl_accelerate] = { gc_a };

const INT32 gcl_brake[num_gcl_brake] = { gc_b };

const INT32 gcl_drift[num_gcl_drift] = { gc_c };

const INT32 gcl_spindash[num_gcl_spindash] = {
	gc_a, gc_b, gc_c, gc_abc
};

const INT32 gcl_movement[num_gcl_movement] = {
	gc_a, gc_b, gc_c, gc_abc, gc_left, gc_right
};

const INT32 gcl_item[num_gcl_item] = {
	gc_fire, gc_aimforward, gc_aimbackward
};

const INT32 gcl_full[num_gcl_full] = {
	gc_a, gc_drift, gc_b, gc_spindash, gc_turnleft, gc_turnright,
	gc_fire, gc_aimforward, gc_aimbackward,
	gc_lookback
};
*/

static INT32 g_gamekeydown_device0[NUMINPUTS];

static INT32 g_available_gamepad_devices;
static INT32 g_gamepad_device_ids[MAXGAMEPADS];
static INT32* g_gamepad_gamekeydown[MAXGAMEPADS];
static boolean g_device0_responding;
static boolean g_gamepad_responding[MAXGAMEPADS];
static INT32 g_player_devices[MAXSPLITSCREENPLAYERS] = {-1, -1, -1, -1};

void G_RegisterAvailableGamepad(INT32 device_id)
{
	I_Assert(device_id >= 1);

	if (g_available_gamepad_devices == MAXGAMEPADS)
	{
		// too many!
		return;
	}

	g_gamepad_device_ids[g_available_gamepad_devices] = device_id;

	g_gamepad_gamekeydown[g_available_gamepad_devices] = Z_CallocAlign(NUMINPUTS * sizeof(INT32), PU_STATIC, NULL, 4);

	g_gamepad_responding[g_available_gamepad_devices] = false;

	g_available_gamepad_devices += 1;
}

void G_UnregisterAvailableGamepad(INT32 device_id)
{
	int i = 0;

	I_Assert(device_id >= 1);

	if (g_available_gamepad_devices <= 0)
	{
		return;
	}

	for (i = 0; i < g_available_gamepad_devices; i++)
	{
		if (g_gamepad_device_ids[i] == device_id)
		{
			int32_t *old_gamekeydown = g_gamepad_gamekeydown[i];
			g_gamepad_device_ids[i] = g_gamepad_device_ids[g_available_gamepad_devices - 1];
			g_gamepad_gamekeydown[i] = g_gamepad_gamekeydown[g_available_gamepad_devices - 1];
			g_gamepad_responding[i] = g_gamepad_responding[g_available_gamepad_devices - 1];
			Z_Free(old_gamekeydown);
			g_available_gamepad_devices -= 1;
			return;
		}
	}
}

INT32 G_GetNumAvailableGamepads(void)
{
	return g_available_gamepad_devices;
}

INT32 G_GetAvailableGamepadDevice(INT32 available_index)
{
	if (available_index < 0 || available_index >= G_GetNumAvailableGamepads())
	{
		return -1;
	}

	return g_gamepad_device_ids[available_index];
}

INT32 G_GetPlayerForDevice(INT32 device_id)
{
	INT32 i;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (device_id == g_player_devices[i])
		{
			return i;
		}
	}

	return -1;
}

INT32 G_GetDeviceForPlayer(INT32 player)
{
	int i;

	if (G_GetPlayerForDevice(KEYBOARD_MOUSE_DEVICE) == player)
	{
		return KEYBOARD_MOUSE_DEVICE;
	}

	for (i = 0; i < G_GetNumAvailableGamepads() + 1; i++)
	{
		INT32 device = G_GetAvailableGamepadDevice(i);
		if (G_GetPlayerForDevice(device) == player)
		{
			return device;
		}
	}

	return -1;
}

void G_SetDeviceForPlayer(INT32 player, INT32 device)
{
	int i;

	I_Assert(player >= 0 && player < MAXSPLITSCREENPLAYERS);
	I_Assert(device >= -1);

	g_player_devices[player] = device;

	if (device == -1)
	{
		return;
	}

	if (device != KEYBOARD_MOUSE_DEVICE)
	{
		I_SetGamepadPlayerIndex(device, player);
	}

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (i == player)
		{
			continue;
		}

		if (g_player_devices[i] == device)
		{
			g_player_devices[i] = -1;
			if (device > 0)
			{
				I_SetGamepadPlayerIndex(device, -1);
				I_GamepadRumble(device, 0, 0);
				I_GamepadRumbleTriggers(device, 0, 0);
			}
		}
	}
}

void G_SetPlayerGamepadIndicatorToPlayerColor(INT32 player)
{
	INT32 device;
	UINT16 skincolor;
	byteColor_t byte_color;

	I_Assert(player >= 0 && player < MAXSPLITSCREENPLAYERS);

	device = G_GetDeviceForPlayer(player);

	if (device <= 0)
	{
		return;
	}

	skincolor = M_GetCvPlayerColor(player);

	byte_color = V_GetColor(skincolors[skincolor].ramp[8]).s;

	I_SetGamepadIndicatorColor(device, byte_color.red, byte_color.green, byte_color.blue);
}

INT32* G_GetDeviceGameKeyDownArray(INT32 device)
{
	int i;

	I_Assert(device >= 0);

	if (device == KEYBOARD_MOUSE_DEVICE)
	{
		return g_gamekeydown_device0;
	}

	for (i = 0; i < g_available_gamepad_devices; i++)
	{
		if (g_gamepad_device_ids[i] == device)
		{
			return g_gamepad_gamekeydown[i];
		}
	}

	return NULL;
}

void G_ResetAllDeviceGameKeyDown(void)
{
	int i;

	memset(gamekeydown, 0, sizeof(gamekeydown));
	memset(g_gamekeydown_device0, 0, sizeof(g_gamekeydown_device0));

	for (i = 0; i < g_available_gamepad_devices; i++)
	{
		memset(g_gamepad_gamekeydown[i], 0, sizeof(INT32) * NUMINPUTS);
	}
}

boolean G_IsDeviceResponding(INT32 device)
{
	int i;

	I_Assert(device >= 0);

	if (device == KEYBOARD_MOUSE_DEVICE)
	{
		return g_device0_responding;
	}

	for (i = 0; i < g_available_gamepad_devices; i++)
	{
		INT32 device_id = G_GetAvailableGamepadDevice(i);
		if (device_id == device)
		{
			return g_gamepad_responding[i];
		}
	}

	return false;
}

void G_SetDeviceResponding(INT32 device, boolean responding)
{
	int i;

	I_Assert(device >= 0);

	if (device == KEYBOARD_MOUSE_DEVICE)
	{
		g_device0_responding = responding;
		return;
	}

	for (i = 0; i < g_available_gamepad_devices; i++)
	{
		INT32 device_id = G_GetAvailableGamepadDevice(i);
		if (device_id == device)
		{
			g_gamepad_responding[i] = responding;
			return;
		}
	}
}

void G_ResetAllDeviceResponding(void)
{
	int i;
	int num_gamepads;

	g_device0_responding = false;

	num_gamepads = G_GetNumAvailableGamepads();

	for (i = 0; i < num_gamepads; i++)
	{
		g_gamepad_responding[i] = false;
	}
}

void G_PlayerDeviceRumble(INT32 player, UINT16 low_strength, UINT16 high_strength)
{
	INT32 device_id;

	if (cv_rumble[player].value == 0)
	{
		return;
	}

	device_id = G_GetDeviceForPlayer(player);

	if (device_id < 1)
	{
		return;
	}

	I_GamepadRumble(device_id, low_strength, high_strength);
}

void G_PlayerDeviceRumbleTriggers(INT32 player, UINT16 left_strength, UINT16 right_strength)
{
	INT32 device_id;

	if (cv_rumble[player].value == 0)
	{
		return;
	}

	device_id = G_GetDeviceForPlayer(player);

	if (device_id < 1)
	{
		return;
	}

	I_GamepadRumbleTriggers(device_id, left_strength, right_strength);
}

void G_ResetPlayerDeviceRumble(INT32 player)
{
	INT32 device_id;

	device_id = G_GetDeviceForPlayer(player);

	if (device_id < 1)
	{
		return;
	}

	I_GamepadRumble(device_id, 0, 0);
	I_GamepadRumbleTriggers(device_id, 0, 0);
}

void G_ResetAllDeviceRumbles(void)
{
	int i;
	int devices;

	devices = G_GetNumAvailableGamepads();

	for (i = 0; i < devices; i++)
	{
		INT32 device_id = G_GetAvailableGamepadDevice(i);

		I_GamepadRumble(device_id, 0, 0);
		I_GamepadRumbleTriggers(device_id, 0, 0);
	}
}

static boolean AutomaticControllerReassignmentIsAllowed(INT32 device)
{
	boolean device_is_gamepad = device > 0;
	boolean device_is_unassigned = G_GetPlayerForDevice(device) == -1;
	boolean gamestate_is_in_active_play = (gamestate == GS_LEVEL || gamestate == GS_VOTING);

	return device_is_gamepad && device_is_unassigned && gamestate_is_in_active_play;
}

static INT32 AssignDeviceToFirstUnassignedPlayer(INT32 device)
{
	int i;

	for (i = 0; i < splitscreen + 1; i++)
	{
		if (G_GetDeviceForPlayer(i) == -1)
		{
			G_SetDeviceForPlayer(i, device);
			return i;
		}
	}

	return -1;
}

static void update_vkb_axis(INT32 axis)
{
	if (axis > JOYAXISRANGE/2)
		M_SwitchVirtualKeyboard(true);
}

//
// Remaps the inputs to game controls.
//
// A game control can be triggered by one or more keys/buttons.
//
// Each key/mousebutton/joybutton triggers ONLY ONE game control.
//
void G_MapEventsToControls(event_t *ev)
{
	INT32 i;
	INT32 *DeviceGameKeyDownArray;

	if (ev->device >= 0)
	{
		switch (ev->type)
		{
			case ev_keydown:
			//case ev_keyup:
			//case ev_mouse:
			//case ev_gamepad_axis:
				G_SetDeviceResponding(ev->device, true);
				break;

			default:
				break;
		}
	}
	else
	{
		return;
	}

	DeviceGameKeyDownArray = G_GetDeviceGameKeyDownArray(ev->device);

	if (!DeviceGameKeyDownArray)
		return;

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 < NUMINPUTS)
			{
				M_MenuTypingInput(ev->data1);

				if (ev->data2) // OS repeat? We handle that ourselves
				{
					break;
				}

				DeviceGameKeyDownArray[ev->data1] = JOYAXISRANGE;

				if (AutomaticControllerReassignmentIsAllowed(ev->device))
				{
					INT32 assigned = AssignDeviceToFirstUnassignedPlayer(ev->device);
					if (assigned >= 0)
					{
						CONS_Alert(CONS_NOTICE, "Player %d device was reassigned\n", assigned + 1);
					}
				}
			}
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad downkey input %d\n", ev->data1);
			}
#endif
			break;

		case ev_keyup:
			if (ev->data1 < NUMINPUTS)
			{
				DeviceGameKeyDownArray[ev->data1] = 0;
			}
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad upkey input %d\n", ev->data1);
			}
#endif
			break;

		case ev_mouse: // buttons are virtual keys
			// X axis
			if (ev->data2 < 0)
			{
				// Left
				DeviceGameKeyDownArray[KEY_MOUSEMOVE + 2] = abs(ev->data2);
				DeviceGameKeyDownArray[KEY_MOUSEMOVE + 3] = 0;
			}
			else
			{
				// Right
				DeviceGameKeyDownArray[KEY_MOUSEMOVE + 2] = 0;
				DeviceGameKeyDownArray[KEY_MOUSEMOVE + 3] = abs(ev->data2);
			}

			// Y axis
			if (ev->data3 < 0)
			{
				// Up
				DeviceGameKeyDownArray[KEY_MOUSEMOVE] = abs(ev->data3);
				DeviceGameKeyDownArray[KEY_MOUSEMOVE + 1] = 0;
			}
			else
			{
				// Down
				DeviceGameKeyDownArray[KEY_MOUSEMOVE] = 0;
				DeviceGameKeyDownArray[KEY_MOUSEMOVE + 1] = abs(ev->data3);
			}
			break;

		case ev_gamepad_axis: // buttons are virtual keys
			if (ev->data1 >= JOYAXISSETS)
			{
#ifdef PARANOIA
				CONS_Debug(DBG_GAMELOGIC, "Bad joystick axis event %d\n", ev->data1);
#endif
				break;
			}

			i = ev->data1;

			if (i >= JOYANALOGS)
			{
				// The trigger axes are handled specially.
				i -= JOYANALOGS;

				if (AutomaticControllerReassignmentIsAllowed(ev->device)
					&& (abs(ev->data2) > JOYAXISRANGE/2 || abs(ev->data3) > JOYAXISRANGE/2))
				{
					INT32 assigned = AssignDeviceToFirstUnassignedPlayer(ev->device);
					if (assigned >= 0)
					{
						CONS_Alert(CONS_NOTICE, "Player %d device was reassigned\n", assigned + 1);
					}
				}

				if (ev->data2 != INT32_MAX)
				{
					DeviceGameKeyDownArray[KEY_AXIS1 + (JOYANALOGS * 4) + (i * 2)] = max(0, ev->data2);
					update_vkb_axis(max(0, ev->data2));
				}

				if (ev->data3 != INT32_MAX)
				{
					DeviceGameKeyDownArray[KEY_AXIS1 + (JOYANALOGS * 4) + (i * 2) + 1] = max(0, ev->data3);
					update_vkb_axis(max(0, ev->data3));
				}
			}
			else
			{
				// We used to only allow this assignment for triggers, but it caused some confusion in vote screen.
				// In case of misebhaving devices, break glass.
				if (AutomaticControllerReassignmentIsAllowed(ev->device)
					&& (abs(ev->data2) > JOYAXISRANGE/2 || abs(ev->data3) > JOYAXISRANGE/2))
				{
					INT32 assigned = AssignDeviceToFirstUnassignedPlayer(ev->device);
					if (assigned >= 0)
					{
						CONS_Alert(CONS_NOTICE, "Player %d device was reassigned\n", assigned + 1);
					}
				}

				// Actual analog sticks
				if (ev->data2 != INT32_MAX)
				{
					if (ev->data2 < 0)
					{
						// Left
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4)] = abs(ev->data2);
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4) + 1] = 0;
					}
					else
					{
						// Right
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4)] = 0;
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4) + 1] = abs(ev->data2);
					}
					update_vkb_axis(abs(ev->data2));
				}

				if (ev->data3 != INT32_MAX)
				{
					if (ev->data3 < 0)
					{
						// Up
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4) + 2] = abs(ev->data3);
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4) + 3] = 0;
					}
					else
					{
						// Down
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4) + 2] = 0;
						DeviceGameKeyDownArray[KEY_AXIS1 + (i * 4) + 3] = abs(ev->data3);
					}
					update_vkb_axis(abs(ev->data3));
				}
			}
			break;

		default:
			break;
	}
}

typedef struct
{
	INT32 keynum;
	const char *name;
} keyname_t;

static keyname_t keynames[] =
{
	{KEY_SPACE, "SPACE"},
	{KEY_CAPSLOCK, "CAPS LOCK"},
	{KEY_ENTER, "ENTER"},
	{KEY_TAB, "TAB"},
	{KEY_ESCAPE, "ESCAPE"},
	{KEY_BACKSPACE, "BACKSPACE"},

	{KEY_NUMLOCK, "NUM LOCK"},
	{KEY_SCROLLLOCK, "SCROLL LOCK"},

	// bill gates keys
	{KEY_LEFTWIN, "LWINDOWS"},
	{KEY_RIGHTWIN, "RWINDOWS"},
	{KEY_MENU, "MENU"},

	{KEY_LSHIFT, "LSHIFT"},
	{KEY_RSHIFT, "RSHIFT"},
	{KEY_LSHIFT, "SHIFT"},
	{KEY_LCTRL, "LCTRL"},
	{KEY_RCTRL, "RCTRL"},
	{KEY_LCTRL, "CTRL"},
	{KEY_LALT, "LALT"},
	{KEY_RALT, "RALT"},
	{KEY_LALT, "ALT"},

	// keypad keys
	{KEY_KPADSLASH, "KEYPAD /"},
	{KEY_KEYPAD7, "KEYPAD 7"},
	{KEY_KEYPAD8, "KEYPAD 8"},
	{KEY_KEYPAD9, "KEYPAD 9"},
	{KEY_MINUSPAD, "KEYPAD -"},
	{KEY_KEYPAD4, "KEYPAD 4"},
	{KEY_KEYPAD5, "KEYPAD 5"},
	{KEY_KEYPAD6, "KEYPAD 6"},
	{KEY_PLUSPAD, "KEYPAD +"},
	{KEY_KEYPAD1, "KEYPAD 1"},
	{KEY_KEYPAD2, "KEYPAD 2"},
	{KEY_KEYPAD3, "KEYPAD 3"},
	{KEY_KEYPAD0, "KEYPAD 0"},
	{KEY_KPADDEL, "KEYPAD ."},

	// extended keys (not keypad)
	{KEY_HOME, "HOME"},
	{KEY_UPARROW, "UP ARROW"},
	{KEY_PGUP, "PAGE UP"},
	{KEY_LEFTARROW, "LEFT ARROW"},
	{KEY_RIGHTARROW, "RIGHT ARROW"},
	{KEY_END, "END"},
	{KEY_DOWNARROW, "DOWN ARROW"},
	{KEY_PGDN, "PAGE DOWN"},
	{KEY_INS, "INSERT"},
	{KEY_DEL, "DELETE"},

	// other keys
	{KEY_F1, "F1"},
	{KEY_F2, "F2"},
	{KEY_F3, "F3"},
	{KEY_F4, "F4"},
	{KEY_F5, "F5"},
	{KEY_F6, "F6"},
	{KEY_F7, "F7"},
	{KEY_F8, "F8"},
	{KEY_F9, "F9"},
	{KEY_F10, "F10"},
	{KEY_F11, "F11"},
	{KEY_F12, "F12"},

	// KEY_CONSOLE has an exception in the keyname code
	{'`', "TILDE"},
	{KEY_PAUSE, "PAUSE/BREAK"},

	// virtual keys for mouse buttons and joystick buttons
	{KEY_MOUSE1+0,"MOUSE1"},
	{KEY_MOUSE1+1,"MOUSE2"},
	{KEY_MOUSE1+2,"MOUSE3"},
	{KEY_MOUSE1+3,"MOUSE4"},
	{KEY_MOUSE1+4,"MOUSE5"},
	{KEY_MOUSE1+5,"MOUSE6"},
	{KEY_MOUSE1+6,"MOUSE7"},
	{KEY_MOUSE1+7,"MOUSE8"},
	{KEY_MOUSEMOVE+0,"Mouse Up"},
	{KEY_MOUSEMOVE+1,"Mouse Down"},
	{KEY_MOUSEMOVE+2,"Mouse Left"},
	{KEY_MOUSEMOVE+3,"Mouse Right"},
	{KEY_MOUSEWHEELUP, "Wheel Up"},
	{KEY_MOUSEWHEELDOWN, "Wheel Down"},

	{KEY_JOY1+0, "A BUTTON"},
	{KEY_JOY1+1, "B BUTTON"},
	{KEY_JOY1+2, "X BUTTON"},
	{KEY_JOY1+3, "Y BUTTON"},
	{KEY_JOY1+4, "BACK BUTTON"},
	{KEY_JOY1+5, "GUIDE BUTTON"},
	{KEY_JOY1+6, "START BUTTON"},
	{KEY_JOY1+7, "L-STICK CLICK"},
	{KEY_JOY1+8, "R-STICK CLICK"},
	{KEY_JOY1+9, "L BUMPER"},
	{KEY_JOY1+10, "R BUMPER"},
	{KEY_JOY1+11, "D-PAD UP"},
	{KEY_JOY1+12, "D-PAD DOWN"},
	{KEY_JOY1+13, "D-PAD LEFT"},
	{KEY_JOY1+14, "D-PAD RIGHT"},
	{KEY_JOY1+15, "MISC. BUTTON"},
	{KEY_JOY1+16, "PADDLE1 BUTTON"},
	{KEY_JOY1+17, "PADDLE2 BUTTON"},
	{KEY_JOY1+18, "PADDLE3 BUTTON"},
	{KEY_JOY1+19, "PADDLE4 BUTTON"},
	{KEY_JOY1+20, "TOUCHPAD"},

	{KEY_AXIS1+0, "L-STICK LEFT"},
	{KEY_AXIS1+1, "L-STICK RIGHT"},
	{KEY_AXIS1+2, "L-STICK UP"},
	{KEY_AXIS1+3, "L-STICK DOWN"},
	{KEY_AXIS1+4, "R-STICK LEFT"},
	{KEY_AXIS1+5, "R-STICK RIGHT"},
	{KEY_AXIS1+6, "R-STICK UP"},
	{KEY_AXIS1+7, "R-STICK DOWN"},
	{KEY_AXIS1+8, "L TRIGGER"},
	{KEY_AXIS1+9, "R TRIGGER"},
};

static keyname_t shortkeynames[] =
{
	{KEY_SPACE, "SPC"},
	{KEY_CAPSLOCK, "CAPS"},
	{KEY_ENTER, "ENTER"},
	{KEY_TAB, "TAB"},
	{KEY_ESCAPE, "ESC"},
	{KEY_BACKSPACE, "BKSP"},

	{KEY_NUMLOCK, "NLOK"},
	{KEY_SCROLLLOCK, "SLOK"},

	// bill gates keys
	{KEY_LEFTWIN, "LWIN"},
	{KEY_RIGHTWIN, "RWIN"},
	{KEY_MENU, "MENU"},

	{KEY_LSHIFT, "LSFT"},
	{KEY_RSHIFT, "RSFT"},
	{KEY_LSHIFT, "SFT"},
	{KEY_LCTRL, "LCTRL"},
	{KEY_RCTRL, "RCTRL"},
	{KEY_LCTRL, "CTRL"},
	{KEY_LALT, "LALT"},
	{KEY_RALT, "RALT"},
	{KEY_LALT, "ALT"},

	// keypad keys
	{KEY_KPADSLASH, "/"},
	{KEY_KEYPAD7, "7"},
	{KEY_KEYPAD8, "8"},
	{KEY_KEYPAD9, "9"},
	{KEY_MINUSPAD, "-"},
	{KEY_KEYPAD4, "4"},
	{KEY_KEYPAD5, "5"},
	{KEY_KEYPAD6, "6"},
	{KEY_PLUSPAD, "+"},
	{KEY_KEYPAD1, "1"},
	{KEY_KEYPAD2, "2"},
	{KEY_KEYPAD3, "3"},
	{KEY_KEYPAD0, "0"},
	{KEY_KPADDEL, "."},

	// extended keys (not keypad)
	{KEY_HOME, "HOME"},
	{KEY_UPARROW, "UP"},
	{KEY_PGUP, "PGUP"},
	{KEY_LEFTARROW, "LEFT"},
	{KEY_RIGHTARROW, "RIGHT"},
	{KEY_END, "END"},
	{KEY_DOWNARROW, "DOWN"},
	{KEY_PGDN, "PGDN"},
	{KEY_INS, "INS"},
	{KEY_DEL, "DEL"},

	// other keys
	{KEY_F1, "F1"},
	{KEY_F2, "F2"},
	{KEY_F3, "F3"},
	{KEY_F4, "F4"},
	{KEY_F5, "F5"},
	{KEY_F6, "F6"},
	{KEY_F7, "F7"},
	{KEY_F8, "F8"},
	{KEY_F9, "F9"},
	{KEY_F10, "F10"},
	{KEY_F11, "F11"},
	{KEY_F12, "F12"},

	// KEY_CONSOLE has an exception in the keyname code
	{'`', "TILDE"},
	{KEY_PAUSE, "PAUSE"},

	// virtual keys for mouse buttons and joystick buttons
	{KEY_MOUSE1+0,"M1"},
	{KEY_MOUSE1+1,"M2"},
	{KEY_MOUSE1+2,"M3"},
	{KEY_MOUSE1+3,"M4"},
	{KEY_MOUSE1+4,"M5"},
	{KEY_MOUSE1+5,"M6"},
	{KEY_MOUSE1+6,"M7"},
	{KEY_MOUSE1+7,"M8"},
	{KEY_MOUSEMOVE+0,"Mouse Up"},
	{KEY_MOUSEMOVE+1,"Mouse Down"},
	{KEY_MOUSEMOVE+2,"Mouse Left"},
	{KEY_MOUSEMOVE+3,"Mouse Right"},
	{KEY_MOUSEWHEELUP, "Wheel Up"},
	{KEY_MOUSEWHEELDOWN, "Wheel Down"},

	{KEY_JOY1+0, "A"},
	{KEY_JOY1+1, "B"},
	{KEY_JOY1+2, "X"},
	{KEY_JOY1+3, "Y"},
	{KEY_JOY1+4, "BACK"},
	{KEY_JOY1+5, "GUIDE"},
	{KEY_JOY1+6, "START"},
	{KEY_JOY1+7, "LS"},
	{KEY_JOY1+8, "RS"},
	{KEY_JOY1+9, "LB"},
	{KEY_JOY1+10, "RB"},
	{KEY_JOY1+11, "D-UP"},
	{KEY_JOY1+12, "D-DOWN"},
	{KEY_JOY1+13, "D-LEFT"},
	{KEY_JOY1+14, "D-RIGHT"},
	{KEY_JOY1+15, "MISC."},
	{KEY_JOY1+16, "PADDLE1"},
	{KEY_JOY1+17, "PADDLE2"},
	{KEY_JOY1+18, "PADDLE3"},
	{KEY_JOY1+19, "PADDLE4"},
	{KEY_JOY1+20, "TOUCHPAD"},

	{KEY_AXIS1+0, "LS LEFT"},
	{KEY_AXIS1+1, "LS RIGHT"},
	{KEY_AXIS1+2, "LS UP"},
	{KEY_AXIS1+3, "LS DOWN"},
	{KEY_AXIS1+4, "RS LEFT"},
	{KEY_AXIS1+5, "RS RIGHT"},
	{KEY_AXIS1+6, "RS UP"},
	{KEY_AXIS1+7, "RS DOWN"},
	{KEY_AXIS1+8, "LT"},
	{KEY_AXIS1+9, "RT"},
};

static const char *gamecontrolname[num_gamecontrols] =
{
	"null", // a key/button mapped to gc_null has no effect
	"up",
	"down",
	"left",
	"right",
	"a",
	"b",
	"c",
	"x",
	"y",
	"z",
	"l",
	"r",
	"start",
	"abc",
	"luaa",
	"luab",
	"luac",
	"console",
	"talk",
	"teamtalk",
	"rankings",
	"screenshot",
	"startmovie",
	"startlossless",
	"voicepushtotalk"
};

#define NUMKEYNAMES (sizeof (keynames)/sizeof (keyname_t))

// If keybind is necessary to navigate menus, it's on this list.
boolean G_KeyBindIsNecessary(INT32 gc)
{
	switch (gc)
	{
		case gc_a:
		case gc_c:
		case gc_x:
		case gc_up:
		case gc_down:
		case gc_left:
		case gc_right:
		//case gc_start: // Is necessary, but handled special.
			return true;
		default:
			return false;
	}
	return false;
}

// Returns false if a key is deemed unreachable for this device.
boolean G_KeyIsAvailable(INT32 key, INT32 deviceID)
{
	boolean gamepad_key = false;

	// Invalid key number.
	if (key <= 0 || key >= NUMINPUTS)
	{
		return false;
	}

	// Only allow gamepad keys for gamepad devices,
	// and vice versa.
	gamepad_key = (key >= KEY_JOY1 && key < JOYINPUTEND);
	if (deviceID == KEYBOARD_MOUSE_DEVICE)
	{
		if (gamepad_key == true)
		{
			return false;
		}
	}
	else
	{
		if (gamepad_key == false)
		{
			return false;
		}
	}

	return true;
}

//
// Detach any keys associated to the given game control
// - pass the pointer to the gamecontrol table for the player being edited
void G_ClearControlKeys(INT32 (*setupcontrols)[MAXINPUTMAPPING], INT32 control)
{
	INT32 i;
	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		setupcontrols[control][i] = KEY_NULL;
	}
}

void G_ClearAllControlKeys(void)
{
	INT32 i, j;
	for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
	{
		for (i = 0; i < num_gamecontrols; i++)
		{
			G_ClearControlKeys(gamecontrol[j], i);
		}
	}
}

//
// Returns the name of a key (or virtual key for mouse and joy)
// the input value being an keynum
//
const char *G_KeynumToString(INT32 keynum)
{
	static char keynamestr[8];

	UINT32 j;

	// return a string with the ascii char if displayable
	if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
	{
		keynamestr[0] = toupper(keynum); // Uppercase looks better!
		keynamestr[1] = '\0';
		return keynamestr;
	}

	// find a description for special keys
	for (j = 0; j < NUMKEYNAMES; j++)
		if (keynames[j].keynum == keynum)
			return keynames[j].name;

	// create a name for unknown keys
	sprintf(keynamestr, "KEY%d", keynum);
	return keynamestr;
}

const char *G_KeynumToShortString(INT32 keynum)
{
	static char keynamestr[8];

	UINT32 j;

	// return a string with the ascii char if displayable
	if (keynum > ' ' && keynum <= 'z' && keynum != KEY_CONSOLE)
	{
		keynamestr[0] = toupper(keynum); // Uppercase looks better!
		keynamestr[1] = '\0';
		return keynamestr;
	}

	// find a description for special keys
	for (j = 0; j < NUMKEYNAMES; j++)
		if (shortkeynames[j].keynum == keynum)
			return shortkeynames[j].name;

	// create a name for unknown keys
	sprintf(keynamestr, "KEY%d", keynum);
	return keynamestr;
}

INT32 G_KeyStringtoNum(const char *keystr)
{
	UINT32 j;

	if (!keystr[0])
		return 0;

	if (!keystr[1] && keystr[0] > ' ' && keystr[0] <= 'z')
		return keystr[0];

	if (!strncmp(keystr, "KEY", 3) && keystr[3] >= '0' && keystr[3] <= '9')
	{
		/* what if we out of range bruh? */
		j = atoi(&keystr[3]);
		if (j < NUMINPUTS)
			return j;
		return 0;
	}

	for (j = 0; j < NUMKEYNAMES; j++)
		if (!stricmp(keynames[j].name, keystr))
			return keynames[j].keynum;

	return 0;
}

void G_DefineDefaultControls(void)
{
	// These defaults are less bad than they used to be.
	// Keyboard controls
	gamecontroldefault[gc_up           ][0] = KEY_UPARROW;
	gamecontroldefault[gc_down         ][0] = KEY_DOWNARROW;
	gamecontroldefault[gc_left         ][0] = KEY_LEFTARROW;
	gamecontroldefault[gc_right        ][0] = KEY_RIGHTARROW;
	gamecontroldefault[gc_a            ][0] = 'a';
	gamecontroldefault[gc_b            ][0] = KEY_LSHIFT;
	gamecontroldefault[gc_c            ][0] = 'q';
	gamecontroldefault[gc_x            ][0] = 'd';
	gamecontroldefault[gc_y            ][0] = 'v';
	gamecontroldefault[gc_z            ][0] = 'z';
	gamecontroldefault[gc_l            ][0] = KEY_SPACE;
	gamecontroldefault[gc_r            ][0] = 's';
	gamecontroldefault[gc_start        ][0] = KEY_ESCAPE;
	gamecontroldefault[gc_talk         ][0] = 't';
	gamecontroldefault[gc_rankings     ][0] = KEY_TAB;
	gamecontroldefault[gc_screenshot   ][0] = KEY_F8;
	gamecontroldefault[gc_startmovie   ][0] = KEY_F9;
	gamecontroldefault[gc_startlossless][0] = KEY_F10;

	// Gamepad controls
	gamecontroldefault[gc_up   ][1] = KEY_HAT1+0; // D-Pad Up
	gamecontroldefault[gc_down ][1] = KEY_HAT1+1; // D-Pad Down
	gamecontroldefault[gc_left ][1] = KEY_HAT1+2; // D-Pad Left
	gamecontroldefault[gc_right][1] = KEY_HAT1+3; // D-Pad Right
	gamecontroldefault[gc_a    ][1] = KEY_JOY1+0; // A
	gamecontroldefault[gc_b    ][1] = KEY_JOY1+1; // B
	gamecontroldefault[gc_c    ][1] = KEY_JOY1+3; // Y
	gamecontroldefault[gc_x    ][1] = KEY_JOY1+2; // X
	gamecontroldefault[gc_y    ][1] = KEY_JOY1+9; // LB
	gamecontroldefault[gc_z    ][1] = KEY_JOY1+10; // RB
	gamecontroldefault[gc_l    ][1] = KEY_AXIS1+8; // LT
	gamecontroldefault[gc_r    ][1] = KEY_AXIS1+9; // RT
	gamecontroldefault[gc_start][1] = KEY_JOY1+6; // Start

	gamecontroldefault[gc_up   ][2] = KEY_AXIS1+2; // Axis Y-
	gamecontroldefault[gc_down ][2] = KEY_AXIS1+3; // Axis Y+
	gamecontroldefault[gc_left ][2] = KEY_AXIS1+0; // Axis X-
	gamecontroldefault[gc_right][2] = KEY_AXIS1+1; // Axis X+

	#ifdef DEVELOP
		gamecontroldefault[gc_console][0] = '`';
	#endif

	// Menu reserved controls
	menucontrolreserved[gc_up   ][0] = KEY_UPARROW;
	menucontrolreserved[gc_down ][0] = KEY_DOWNARROW;
	menucontrolreserved[gc_left ][0] = KEY_LEFTARROW;
	menucontrolreserved[gc_right][0] = KEY_RIGHTARROW;
	menucontrolreserved[gc_a    ][0] = KEY_ENTER;
	menucontrolreserved[gc_c    ][0] = KEY_BACKSPACE;
	menucontrolreserved[gc_x    ][0] = KEY_ESCAPE;
	menucontrolreserved[gc_start][0] = KEY_ESCAPE; // Handled special
}

static boolean G_ControlUsesAxis(INT32 map[MAXINPUTMAPPING])
{
	for (INT32 i = 0; i < MAXINPUTMAPPING; i++)
	{
		INT32 key = map[i];
		if (key >= KEY_AXIS1 && key < JOYINPUTEND)
		{
			return true;
		}
	}

	return false;
}

void G_ApplyControlScheme(UINT8 splitplayer, INT32 (*fromcontrols)[MAXINPUTMAPPING])
{
	UINT8 flags = 0;

	if (G_ControlUsesAxis(fromcontrols[gc_up]) ||
		G_ControlUsesAxis(fromcontrols[gc_down]) ||
		G_ControlUsesAxis(fromcontrols[gc_left]) ||
		G_ControlUsesAxis(fromcontrols[gc_right]))
	{
		flags |= GCF_ANALOGSTICK;
	}

	memcpy(gamecontrol[splitplayer], fromcontrols, sizeof gamecontrol[splitplayer]);
	gamecontrolflags[splitplayer] = flags;

	if (Playing())
		WeaponPref_Send(splitplayer); // update PF_ANALOGSTICK
}

void G_SaveKeySetting(FILE *f, INT32 (*fromcontrolsa)[MAXINPUTMAPPING], INT32 (*fromcontrolsb)[MAXINPUTMAPPING], INT32 (*fromcontrolsc)[MAXINPUTMAPPING], INT32 (*fromcontrolsd)[MAXINPUTMAPPING])
{
	INT32 i, j;

	// TODO: would be nice to get rid of this code duplication
	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol \"%s\" \"%s\"", gamecontrolname[i], G_KeynumToString(fromcontrolsa[i][0]));

		for (j = 1; j < MAXINPUTMAPPING+1; j++)
		{
			if (j < MAXINPUTMAPPING && fromcontrolsa[i][j])
			{
				fprintf(f, " \"%s\"", G_KeynumToString(fromcontrolsa[i][j]));
			}
			else
			{
				fprintf(f, "\n");
				break;
			}
		}
	}

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol2 \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(fromcontrolsb[i][0]));

		for (j = 1; j < MAXINPUTMAPPING+1; j++)
		{
			if (j < MAXINPUTMAPPING && fromcontrolsb[i][j])
			{
				fprintf(f, " \"%s\"", G_KeynumToString(fromcontrolsb[i][j]));
			}
			else
			{
				fprintf(f, "\n");
				break;
			}
		}
	}

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol3 \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(fromcontrolsc[i][0]));

		for (j = 1; j < MAXINPUTMAPPING+1; j++)
		{
			if (j < MAXINPUTMAPPING && fromcontrolsc[i][j])
			{
				fprintf(f, " \"%s\"", G_KeynumToString(fromcontrolsc[i][j]));
			}
			else
			{
				fprintf(f, "\n");
				break;
			}
		}
	}

	for (i = 1; i < num_gamecontrols; i++)
	{
		fprintf(f, "setcontrol4 \"%s\" \"%s\"", gamecontrolname[i],
			G_KeynumToString(fromcontrolsd[i][0]));

		for (j = 1; j < MAXINPUTMAPPING+1; j++)
		{
			if (j < MAXINPUTMAPPING && fromcontrolsd[i][j])
			{
				fprintf(f, " \"%s\"", G_KeynumToString(fromcontrolsd[i][j]));
			}
			else
			{
				fprintf(f, "\n");
				break;
			}
		}
	}
}

INT32 G_CheckDoubleUsage(INT32 keynum, INT32 playernum, boolean modify)
{
	INT32 result = gc_null;

	if (cv_controlperkey.value == 1)
	{
		INT32 i, j;
		for (i = 0; i < num_gamecontrols; i++)
		{
			for (j = 0; j < MAXINPUTMAPPING; j++)
			{
				if (gamecontrol[playernum][i][j] == keynum)
				{
					result = i;
					if (modify)
					{
						gamecontrol[playernum][i][j] = KEY_NULL;
					}
				}

				if (result && !modify)
					return result;
			}
		}
	}

	return result;
}

INT32 G_FindPlayerBindForGameControl(INT32 player, gamecontrols_e control)
{
	INT32 device = showgamepadprompts[player] ? 1 : KEYBOARD_MOUSE_DEVICE;

	INT32 bestbind = -1; // Bind that matches our input device
	INT32 anybind = -1; // Bind that doesn't match, but is at least for this control

	INT32 bindindex = MAXINPUTMAPPING-1;

	// CONS_Printf("Check bind %d for player %d device %d\n", control, player, device);

	// PASS 1: Binds that are directly in our active control mapping.
	while (bindindex >= 0) // Prefer earlier binds
	{
		INT32 possiblecontrol = gamecontrol[player][control][bindindex];

		bindindex--;

		if (possiblecontrol == 0)
			continue;

		// if (device is gamepad) == (bound control is in gamepad range) - e.g. if bind matches device
		if ((device != KEYBOARD_MOUSE_DEVICE) == (possiblecontrol >= KEY_JOY1 && possiblecontrol < JOYINPUTEND))
		{
			// CONS_Printf("PASS1 found %s\n", G_KeynumToShortString(possiblecontrol));
			bestbind = possiblecontrol;
			anybind = possiblecontrol;
		}
		else
		{
			// CONS_Printf("PASS1 considering %s\n", G_KeynumToShortString(possiblecontrol));
			anybind = possiblecontrol;
		}
	}

	// PASS 3: "Safety" binds that are reserved by the menu system.
	if (bestbind == -1)
	{
		bindindex = MAXINPUTMAPPING-1;

		while (bindindex >= 0)
		{
			INT32 possiblecontrol = menucontrolreserved[control][bindindex];

			bindindex--;

			if (possiblecontrol == 0)
				continue;

			if ((device != KEYBOARD_MOUSE_DEVICE) == (possiblecontrol >= KEY_JOY1 && possiblecontrol < JOYINPUTEND))
			{
				// CONS_Printf("PASS2 found %s\n", G_KeynumToShortString(possiblecontrol));
				bestbind = possiblecontrol;
				anybind = possiblecontrol;
			}
			else
			{
				// CONS_Printf("PASS2 considering %s\n", G_KeynumToShortString(possiblecontrol));
				anybind = possiblecontrol;
			}
		}
	}

	return (bestbind != -1) ? bestbind : anybind; // If we couldn't find a device-appropriate bind, try to at least use something
}

static void setcontrol(UINT8 player)
{
	INT32 numctrl;
	const char *namectrl;
	INT32 keynum;
	INT32 inputMap = 0;
	INT32 i;

	namectrl = COM_Argv(1);

	for (numctrl = 0;
		numctrl < num_gamecontrols && stricmp(namectrl, gamecontrolname[numctrl]);
		numctrl++)
	{ ; }

	if (numctrl == num_gamecontrols)
	{
		CONS_Printf(M_GetText("Control '%s' unknown\n"), namectrl);
		return;
	}

	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		keynum = G_KeyStringtoNum(COM_Argv(inputMap + 2));

		if (keynum >= 0)
		{
			(void)G_CheckDoubleUsage(keynum, player, true);

			// if keynum was rejected, try it again with the next key.
			while (keynum == 0)
			{
				inputMap++;
				if (inputMap >= MAXINPUTMAPPING)
				{
					break;
				}

				keynum = G_KeyStringtoNum(COM_Argv(inputMap + 2));

				if (keynum >= 0)
				{
					(void)G_CheckDoubleUsage(keynum, player, true);
				}
			}
		}

		if (keynum >= 0)
		{
			gamecontrol[player][numctrl][i] = keynum;
		}

		inputMap++;
		if (inputMap >= MAXINPUTMAPPING)
		{
			break;
		}
	}
}

void Command_Setcontrol_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na < 3 || na > MAXINPUTMAPPING+2)
	{
		CONS_Printf(M_GetText("setcontrol <controlname> <keyname> [<keyname>] [<keyname>] [<keyname>]: set controls for player 1\n"));
		return;
	}

	setcontrol(0);
}

void Command_Setcontrol2_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na < 3 || na > MAXINPUTMAPPING+2)
	{
		CONS_Printf(M_GetText("setcontrol2 <controlname> <keyname> [<keyname>] [<keyname>] [<keyname>]: set controls for player 2\n"));
		return;
	}

	setcontrol(1);
}

void Command_Setcontrol3_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na < 3 || na > MAXINPUTMAPPING+2)
	{
		CONS_Printf(M_GetText("setcontrol3 <controlname> <keyname> [<keyname>] [<keyname>] [<keyname>]: set controls for player 3\n"));
		return;
	}

	setcontrol(2);
}

void Command_Setcontrol4_f(void)
{
	INT32 na;

	na = (INT32)COM_Argc();

	if (na < 3 || na > MAXINPUTMAPPING+2)
	{
		CONS_Printf(M_GetText("setcontrol4 <controlname> <keyname> [<keyname>] [<keyname>] [<keyname>]: set controls for player 4\n"));
		return;
	}

	setcontrol(3);
}
