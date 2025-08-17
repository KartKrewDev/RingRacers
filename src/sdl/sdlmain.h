// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief System specific interface stuff.

#ifndef __sdlmain__
#define __sdlmain__

extern SDL_bool consolevent;
extern SDL_bool framebuffer;

#include "../m_fixed.h"
#include "../doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

// SDL2 stub macro
#ifdef _MSC_VER
#define SDL2STUB() CONS_Printf("SDL2: stubbed: %s:%d\n", __FUNCTION__, __LINE__)
#else
#define SDL2STUB() CONS_Printf("SDL2: stubbed: %s:%d\n", __func__, __LINE__)
#endif

/**	\brief	The JoyInfo_s struct

  info about joystick
*/
typedef struct SDLJoyInfo_s
{
	/// Controller handle
	SDL_GameController *dev;
	/// number of old joystick
	int oldjoy;
	/// number of axies
	int axises;
	/// scale of axises
	INT32 scale;
	/// number of buttons
	int buttons;
	/// number of hats
	int hats;
	/// number of balls
	int balls;

} SDLJoyInfo_t;

/**	\brief SDL info about controllers
*/
extern SDLJoyInfo_t JoyInfo[MAXSPLITSCREENPLAYERS];

/**	\brief joystick axis deadzone
*/
#define SDL_JDEADZONE 153
#undef SDL_JDEADZONE

void I_GetConsoleEvents(void);

// Needed for some WIN32 functions
extern SDL_Window *window;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
