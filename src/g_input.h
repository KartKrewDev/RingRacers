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

#ifndef G_INPUT_H
#define G_INPUT_H

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

// current state of the keys: JOYAXISRANGE or 0 when dboolean.
// Or anything inbetween for analog values
#define MAXDEVICES (MAXGAMEPADS + 1) // Gamepads + keyboard & mouse
#define KEYBOARD_MOUSE_DEVICE (0)
#define UNASSIGNED_DEVICE (-1)
#define NO_BINDS_REACHABLE (-1)
extern int32_t gamekeydown[MAXDEVICES][NUMINPUTS];

// several key codes (or virtual key) per game control
extern int32_t gamecontrol[MAXSPLITSCREENPLAYERS][num_gamecontrols][MAXINPUTMAPPING];
extern uint8_t gamecontrolflags[MAXSPLITSCREENPLAYERS];
extern uint8_t showgamepadprompts[MAXSPLITSCREENPLAYERS];
extern int32_t gamecontroldefault[num_gamecontrols][MAXINPUTMAPPING]; // default control storage
extern int32_t menucontrolreserved[num_gamecontrols][MAXINPUTMAPPING];

/*
#define num_gcl_accelerate 1
#define num_gcl_brake 1
#define num_gcl_drift 1
#define num_gcl_spindash 4
#define num_gcl_movement 6
#define num_gcl_item 3
#define num_gcl_full 10

extern const int32_t gcl_accelerate[num_gcl_accelerate];
extern const int32_t gcl_brake[num_gcl_brake];
extern const int32_t gcl_drift[num_gcl_drift];
extern const int32_t gcl_spindash[num_gcl_spindash];
extern const int32_t gcl_movement[num_gcl_movement];
extern const int32_t gcl_item[num_gcl_item];
extern const int32_t gcl_full[num_gcl_full];
*/

// peace to my little coder fingers!
// check a gamecontrol being active or not

/*
*/

/// Register a device index (from ev_gamepad_device_added) as an Available Gamepad
void G_RegisterAvailableGamepad(int32_t device_id);
/// Unregister a device index (from ev_gamepad_device_removed) as an Available Gamepad
void G_UnregisterAvailableGamepad(int32_t device_id);
/// Get the number of Available Gamepads registered.
int32_t G_GetNumAvailableGamepads(void);
/// Get the device ID for a given Available Gamepad Index, or -1. 0 <= available_index < G_GetNumAvailableGamepads()
int32_t G_GetAvailableGamepadDevice(int32_t available_index);

int32_t G_GetPlayerForDevice(int32_t deviceID);
/// Get gamepad device for given player, or -1.
int32_t G_GetDeviceForPlayer(int32_t player);

/// Set the given player index's assigned device. If the device is in use by another player, that player is unassigned.
void G_SetDeviceForPlayer(int32_t player, int32_t device);

void G_SetPlayerGamepadIndicatorToPlayerColor(int32_t player);

extern consvar_t cv_rumble[MAXSPLITSCREENPLAYERS];

void G_PlayerDeviceRumble(int32_t player, uint16_t low_strength, uint16_t high_strength);
void G_PlayerDeviceRumbleTriggers(int32_t player, uint16_t left_strength, uint16_t right_strength);
void G_ResetPlayerDeviceRumble(int32_t player);
void G_ResetAllDeviceRumbles(void);

/// Get the gamekeydown array (NUMINPUTS values) for the given device, or NULL if the device id is invalid.
int32_t* G_GetDeviceGameKeyDownArray(int32_t device);
void G_ResetAllDeviceGameKeyDown(void);

dboolean G_IsDeviceResponding(int32_t device);
void G_SetDeviceResponding(int32_t device, dboolean responding);
void G_ResetAllDeviceResponding(void);

void HandleGamepadDeviceEvents(event_t *ev);

// remaps the input event to a game control.
void G_MapEventsToControls(event_t *ev);

// returns the name of a key
const char *G_KeynumToString(int32_t keynum);
const char *G_KeynumToShortString(int32_t keynum);
int32_t G_KeyStringtoNum(const char *keystr);

dboolean G_KeyBindIsNecessary(int32_t gc);
dboolean G_KeyIsAvailable(int32_t key, int32_t deviceID);

// detach any keys associated to the given game control
void G_ClearControlKeys(int32_t (*setupcontrols)[MAXINPUTMAPPING], int32_t control);
void G_ClearAllControlKeys(void);
void Command_Setcontrol_f(void);
void Command_Setcontrol2_f(void);
void Command_Setcontrol3_f(void);
void Command_Setcontrol4_f(void);
void G_DefineDefaultControls(void);
int32_t G_GetControlScheme(int32_t (*fromcontrols)[MAXINPUTMAPPING], const int32_t *gclist, int32_t gclen);
void G_ApplyControlScheme(uint8_t splitplayer, int32_t (*fromcontrols)[MAXINPUTMAPPING]);
void G_SaveKeySetting(FILE *f, int32_t (*fromcontrolsa)[MAXINPUTMAPPING], int32_t (*fromcontrolsb)[MAXINPUTMAPPING], int32_t (*fromcontrolsc)[MAXINPUTMAPPING], int32_t (*fromcontrolsd)[MAXINPUTMAPPING]);
int32_t G_CheckDoubleUsage(int32_t keynum, int32_t playernum, dboolean modify);

int32_t G_FindPlayerBindForGameControl(int32_t player, gamecontrols_e control);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
