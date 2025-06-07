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
/// \file  g_input.h
/// \brief handle mouse/keyboard/joystick inputs,
///        maps inputs to game controls (forward, spin, jump...)

#ifndef __G_INPUT__
#define __G_INPUT__

#include "d_event.h"
#include "keys.h"
#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

// number of total 'button' inputs, include keyboard keys, plus virtual
// keys (mousebuttons and joybuttons becomes keys)
#define NUMKEYS 256

#define MOUSEBUTTONS 8

#define JOYBUTTONS   21 // 21 buttons, to match SDL_GameControllerButton
#define JOYANALOGS   2 // 2 sets of analog stick axes, with positive and negative each
#define JOYTRIGGERS  1 // 1 set of trigger axes, positive only
#define JOYAXISSETS (JOYANALOGS + JOYTRIGGERS)
#define JOYAXES ((4 * JOYANALOGS) + (2 * JOYTRIGGERS))

#define MAXINPUTMAPPING 4

//
// mouse and joystick buttons are handled as 'virtual' keys
//
typedef enum
{
	KEY_JOY1 = NUMKEYS,
	KEY_HAT1 = KEY_JOY1 + 11, // macro for SDL_CONTROLLER_BUTTON_DPAD_UP
	KEY_AXIS1 = KEY_JOY1 + JOYBUTTONS,
	JOYINPUTEND = KEY_AXIS1 + JOYAXES,

	KEY_MOUSE1 = JOYINPUTEND,
	KEY_MOUSEMOVE = KEY_MOUSE1 + MOUSEBUTTONS,
	KEY_MOUSEWHEELUP = KEY_MOUSEMOVE + 4,
	KEY_MOUSEWHEELDOWN = KEY_MOUSEWHEELUP + 1,
	MOUSEINPUTEND = KEY_MOUSEWHEELDOWN + 1,

	NUMINPUTS = MOUSEINPUTEND,
} key_input_e;

// Helper to keep descriptive input setup slightly more readable
typedef enum
{
	nc_a = KEY_JOY1,
	nc_b,
	nc_x,
	nc_y,
	nc_back,
	nc_guide,
	nc_start,
	nc_ls,
	nc_rs,
	nc_lb,
	nc_rb,
	nc_hatup,
	nc_hatdown,
	nc_hatleft,
	nc_hatright,
	nc_touch = KEY_JOY1+20,
	nc_lsleft = KEY_AXIS1+0,
	nc_lsright,
	nc_lsup,
	nc_lsdown,
	nc_lt = KEY_AXIS1+8,
	nc_rt,
} named_controls_e;

typedef enum
{
	gc_null = 0, // a key/button mapped to gc_null has no effect

	// The actual gamepad
	gc_up,
	gc_down,
	gc_left,
	gc_right,
	gc_a,
	gc_b,
	gc_c,
	gc_x,
	gc_y,
	gc_z,
	gc_l,
	gc_r,
	gc_start,

	// special keys
	gc_abc,
	gc_lua1,
	gc_lua2,
	gc_lua3,
	gc_console,
	gc_talk,
	gc_teamtalk,
	gc_rankings,
	gc_screenshot,
	gc_startmovie,
	gc_startlossless,
	gc_voicepushtotalk,

	num_gamecontrols,

	// alias gameplay controls
	gc_accel = gc_a,
	gc_lookback = gc_b,
	gc_spindash = gc_c,
	gc_brake = gc_x,
	gc_bail = gc_y,
	gc_vote = gc_z,
	gc_item = gc_l,
	gc_drift = gc_r,
} gamecontrols_e;

typedef enum
{
	GCF_ANALOGSTICK = 1 << 0,
} gamecontrol_flags_e;

// mouse values are used once
extern consvar_t cv_controlperkey;

// current state of the keys: JOYAXISRANGE or 0 when boolean.
// Or anything inbetween for analog values
#define MAXDEVICES (MAXGAMEPADS + 1) // Gamepads + keyboard & mouse
#define KEYBOARD_MOUSE_DEVICE (0)
#define UNASSIGNED_DEVICE (-1)
#define NO_BINDS_REACHABLE (-1)
extern INT32 gamekeydown[MAXDEVICES][NUMINPUTS];

// several key codes (or virtual key) per game control
extern INT32 gamecontrol[MAXSPLITSCREENPLAYERS][num_gamecontrols][MAXINPUTMAPPING];
extern UINT8 gamecontrolflags[MAXSPLITSCREENPLAYERS];
extern UINT8 showgamepadprompts[MAXSPLITSCREENPLAYERS];
extern INT32 gamecontroldefault[num_gamecontrols][MAXINPUTMAPPING]; // default control storage
extern INT32 menucontrolreserved[num_gamecontrols][MAXINPUTMAPPING];

/*
#define num_gcl_accelerate 1
#define num_gcl_brake 1
#define num_gcl_drift 1
#define num_gcl_spindash 4
#define num_gcl_movement 6
#define num_gcl_item 3
#define num_gcl_full 10

extern const INT32 gcl_accelerate[num_gcl_accelerate];
extern const INT32 gcl_brake[num_gcl_brake];
extern const INT32 gcl_drift[num_gcl_drift];
extern const INT32 gcl_spindash[num_gcl_spindash];
extern const INT32 gcl_movement[num_gcl_movement];
extern const INT32 gcl_item[num_gcl_item];
extern const INT32 gcl_full[num_gcl_full];
*/

// peace to my little coder fingers!
// check a gamecontrol being active or not

/*
*/

/// Register a device index (from ev_gamepad_device_added) as an Available Gamepad
void G_RegisterAvailableGamepad(INT32 device_id);
/// Unregister a device index (from ev_gamepad_device_removed) as an Available Gamepad
void G_UnregisterAvailableGamepad(INT32 device_id);
/// Get the number of Available Gamepads registered.
INT32 G_GetNumAvailableGamepads(void);
/// Get the device ID for a given Available Gamepad Index, or -1. 0 <= available_index < G_GetNumAvailableGamepads()
INT32 G_GetAvailableGamepadDevice(INT32 available_index);

INT32 G_GetPlayerForDevice(INT32 deviceID);
/// Get gamepad device for given player, or -1.
INT32 G_GetDeviceForPlayer(INT32 player);

/// Set the given player index's assigned device. If the device is in use by another player, that player is unassigned.
void G_SetDeviceForPlayer(INT32 player, INT32 device);

void G_SetPlayerGamepadIndicatorToPlayerColor(INT32 player);

extern consvar_t cv_rumble[MAXSPLITSCREENPLAYERS];

void G_PlayerDeviceRumble(INT32 player, UINT16 low_strength, UINT16 high_strength);
void G_PlayerDeviceRumbleTriggers(INT32 player, UINT16 left_strength, UINT16 right_strength);
void G_ResetPlayerDeviceRumble(INT32 player);
void G_ResetAllDeviceRumbles(void);

/// Get the gamekeydown array (NUMINPUTS values) for the given device, or NULL if the device id is invalid.
INT32* G_GetDeviceGameKeyDownArray(INT32 device);
void G_ResetAllDeviceGameKeyDown(void);

boolean G_IsDeviceResponding(INT32 device);
void G_SetDeviceResponding(INT32 device, boolean responding);
void G_ResetAllDeviceResponding(void);

void HandleGamepadDeviceEvents(event_t *ev);

// remaps the input event to a game control.
void G_MapEventsToControls(event_t *ev);

// returns the name of a key
const char *G_KeynumToString(INT32 keynum);
const char *G_KeynumToShortString(INT32 keynum);
INT32 G_KeyStringtoNum(const char *keystr);

boolean G_KeyBindIsNecessary(INT32 gc);
boolean G_KeyIsAvailable(INT32 key, INT32 deviceID);

// detach any keys associated to the given game control
void G_ClearControlKeys(INT32 (*setupcontrols)[MAXINPUTMAPPING], INT32 control);
void G_ClearAllControlKeys(void);
void Command_Setcontrol_f(void);
void Command_Setcontrol2_f(void);
void Command_Setcontrol3_f(void);
void Command_Setcontrol4_f(void);
void G_DefineDefaultControls(void);
INT32 G_GetControlScheme(INT32 (*fromcontrols)[MAXINPUTMAPPING], const INT32 *gclist, INT32 gclen);
void G_ApplyControlScheme(UINT8 splitplayer, INT32 (*fromcontrols)[MAXINPUTMAPPING]);
void G_SaveKeySetting(FILE *f, INT32 (*fromcontrolsa)[MAXINPUTMAPPING], INT32 (*fromcontrolsb)[MAXINPUTMAPPING], INT32 (*fromcontrolsc)[MAXINPUTMAPPING], INT32 (*fromcontrolsd)[MAXINPUTMAPPING]);
INT32 G_CheckDoubleUsage(INT32 keynum, INT32 playernum, boolean modify);

INT32 G_FindPlayerBindForGameControl(INT32 player, gamecontrols_e control);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
