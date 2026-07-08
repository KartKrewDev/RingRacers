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
/// \file
/// \brief SRB2 graphics stuff for SDL

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_video.h>
#include <stdlib.h>
#include <errno.h>
#include <memory>

#include <signal.h>

#include <imgui.h>

#include "../rhi/rhi.hpp"
#include "../rhi/gl2/gl2_rhi.hpp"
#include "rhi_gl2_platform.hpp"

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED
#include <SDL3/SDL.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#pragma warning(default : 4214 4244)
#endif

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#if (defined (__unix__) || (!defined(__APPLE__) && defined (UNIXCOMMON)))
#define USE_XPM_ICON
#include "IMG_xpm.h"
#endif

#include "SDL_icon.xpm"

#include "../doomdef.h"

#include "../doomstat.h"
#include "../i_system.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../k_menu.h"
#include "../d_main.h"
#include "../s_sound.h"
#include "../i_sound.h"  	// midi pause/unpause
#include "../i_joy.h"
#include "../st_stuff.h"
#include "../hu_stuff.h"
#include "../g_game.h"
#include "../i_video.h"
#include "../console.h"
#include "../command.h"
#include "../r_main.h"
#include "../lua_hook.h"
#include "sdlmain.h"
#include "../i_system.h"
#ifdef HWRENDER
#include "../hardware/hw_main.h"
#include "../hardware/hw_drv.h"
// For dynamic referencing of HW rendering functions
#include "hwsym_sdl.h"
#include "ogl_sdl.h"
#endif

#ifdef HAVE_DISCORDRPC
#include "../discord.h"
#endif

// maximum number of windowed modes (see windowedModes[][])
#define MAXWINMODES (19)

using namespace srb2;

/**	\brief
*/
static INT32 numVidModes = -1;

/**	\brief
*/
static char vidModeName[33][32]; // allow 33 different modes

rendermode_t rendermode = render_soft;
rendermode_t chosenrendermode = render_none; // set by command line arguments

UINT8 graphics_started = 0; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in VID_PrepareModeList
boolean allow_fullscreen = false;
static bool disable_fullscreen = false;
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value
static bool disable_mouse = false;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && havefocus)
#define MOUSE_MENU false //(!disable_mouse && cv_usemouse.value && menuactive && !USE_FULLSCREEN)
#define MOUSEBUTTONS_MAX MOUSEBUTTONS

// first entry in the modelist which is not bigger than MAXVIDWIDTHxMAXVIDHEIGHT
static      INT32          firstEntry = 0;

// Total mouse motion X/Y offsets
static      INT32        mousemovex = 0, mousemovey = 0;

// SDL vars
static      SDL_Surface *vidSurface = NULL;
static      SDL_Surface *bufSurface = NULL;
static      SDL_Surface *icoSurface = NULL;
static      SDL_Color    localPalette[256];
Uint16      realwidth = BASEVIDWIDTH;
Uint16      realheight = BASEVIDHEIGHT;
static       bool    mousegrabok = true;
static       bool    exposevideo = false;
static       bool    borderlesswindow = false;

// SDL vars
SDL_Window   *window;
static bool      havefocus = true;
static const char *fallback_resolution_name = "Fallback";

static std::unique_ptr<rhi::Rhi> g_rhi;
static uint32_t g_rhi_generation = 0;

// windowed video modes from which to choose from.
static INT32 windowedModes[MAXWINMODES][2] =
{
	{1920,1200}, // 1.60,6.00
	{1920,1080}, // 1.66
	{1680,1050}, // 1.60,5.25
	{1600,1200}, // 1.33
	{1600, 900}, // 1.66
	{1366, 768}, // 1.66
	{1440, 900}, // 1.60,4.50
	{1280,1024}, // 1.33?
	{1280, 960}, // 1.33,4.00
	{1280, 800}, // 1.60,4.00
	{1280, 720}, // 1.66
	{1152, 864}, // 1.33,3.60
	{1024,1024}, // SPECIAL, for snapshot taker
	{1024, 768}, // 1.33,3.20
	{ 800, 600}, // 1.33,2.50
	{ 640, 480}, // 1.33,2.00
	{ 640, 400}, // 1.60,2.00
	{ 320, 240}, // 1.33,1.00
	{ 320, 200}, // 1.60,1.00
};

static void Impl_VideoSetupBuffer(void);
static bool Impl_CreateWindow(bool fullscreen);
//static void Impl_SetWindowName(const char *title);
static void Impl_SetWindowIcon(void);

static void ValidateDisplay(void)
{
	// Validate display ID, otherwise use main display
	if (cv_display.value == 0)
	{
		CV_SetValue(&cv_display, static_cast<INT32>(SDL_GetPrimaryDisplay()));
	}
}

static void CenterWindow(void)
{
	SDL_SetWindowPosition(window,
		SDL_WINDOWPOS_CENTERED_DISPLAY(cv_display.value),
		SDL_WINDOWPOS_CENTERED_DISPLAY(cv_display.value)
	);
}

static void SDLSetMode(int width, int height, bool fullscreen, bool reposition)
{
	static bool wasfullscreen = false;

	realwidth = vid.width;
	realheight = vid.height;

	if (window)
	{
		if (fullscreen)
		{
			if (reposition)
			{
				ValidateDisplay();
				if (SDL_GetDisplayForWindow(window) != cv_display.value)
				{
					CenterWindow();
				}
			}
			wasfullscreen = true;
			SDL_SetWindowFullscreenMode(window, NULL);
			SDL_SetWindowFullscreen(window, true);
		}
		else // windowed mode
		{
			if (wasfullscreen)
			{
				wasfullscreen = false;
				SDL_SetWindowFullscreen(window, false);
			}
			// Reposition window only in windowed mode
			SDL_SetWindowSize(window, width, height);
			if (reposition)
			{
				ValidateDisplay();
				CenterWindow();
			}
		}
	}
	else
	{
		if (!Impl_CreateWindow(fullscreen))
		{
			return;
		}
		wasfullscreen = fullscreen;
		SDL_SetWindowSize(window, width, height);
		if (fullscreen)
		{
			SDL_SetWindowFullscreenMode(window, NULL);
			SDL_SetWindowFullscreen(window, true);
		}
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		OglSdlSurface(vid.width, vid.height);
	}
	else
#endif
	{
		SDL_GL_SetSwapInterval(cv_vidwait.value ? 1 : 0);
	}

	SDL_GetWindowSize(window, &width, &height);
	vid.realwidth = static_cast<uint32_t>(width);
	vid.realheight = static_cast<uint32_t>(height);

	if (graphics_started)
	{
		I_UpdateNoVsync();
	}
}

static INT32 Impl_SDL_Scancode_To_Keycode(SDL_Scancode code)
{
	if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z)
	{
		// get lowercase ASCII
		return code - SDL_SCANCODE_A + 'a';
	}
	if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9)
	{
		return code - SDL_SCANCODE_1 + '1';
	}
	else if (code == SDL_SCANCODE_0)
	{
		return '0';
	}
	if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10)
	{
		return KEY_F1 + (code - SDL_SCANCODE_F1);
	}
	switch (code)
	{
		// F11 and F12 are separated from the rest of the function keys
		case SDL_SCANCODE_F11: return KEY_F11;
		case SDL_SCANCODE_F12: return KEY_F12;

		case SDL_SCANCODE_KP_0: return KEY_KEYPAD0;
		case SDL_SCANCODE_KP_1: return KEY_KEYPAD1;
		case SDL_SCANCODE_KP_2: return KEY_KEYPAD2;
		case SDL_SCANCODE_KP_3: return KEY_KEYPAD3;
		case SDL_SCANCODE_KP_4: return KEY_KEYPAD4;
		case SDL_SCANCODE_KP_5: return KEY_KEYPAD5;
		case SDL_SCANCODE_KP_6: return KEY_KEYPAD6;
		case SDL_SCANCODE_KP_7: return KEY_KEYPAD7;
		case SDL_SCANCODE_KP_8: return KEY_KEYPAD8;
		case SDL_SCANCODE_KP_9: return KEY_KEYPAD9;

		case SDL_SCANCODE_RETURN:         return KEY_ENTER;
		case SDL_SCANCODE_ESCAPE:         return KEY_ESCAPE;
		case SDL_SCANCODE_BACKSPACE:      return KEY_BACKSPACE;
		case SDL_SCANCODE_TAB:            return KEY_TAB;
		case SDL_SCANCODE_SPACE:          return KEY_SPACE;
		case SDL_SCANCODE_MINUS:          return KEY_MINUS;
		case SDL_SCANCODE_EQUALS:         return KEY_EQUALS;
		case SDL_SCANCODE_LEFTBRACKET:    return '[';
		case SDL_SCANCODE_RIGHTBRACKET:   return ']';
		case SDL_SCANCODE_BACKSLASH:      return '\\';
		case SDL_SCANCODE_NONUSHASH:      return '#';
		case SDL_SCANCODE_SEMICOLON:      return ';';
		case SDL_SCANCODE_APOSTROPHE:     return '\'';
		case SDL_SCANCODE_GRAVE:          return '`';
		case SDL_SCANCODE_COMMA:          return ',';
		case SDL_SCANCODE_PERIOD:         return '.';
		case SDL_SCANCODE_SLASH:          return '/';
		case SDL_SCANCODE_CAPSLOCK:       return KEY_CAPSLOCK;
		case SDL_SCANCODE_PRINTSCREEN:    return 0; // undefined?
		case SDL_SCANCODE_SCROLLLOCK:     return KEY_SCROLLLOCK;
		case SDL_SCANCODE_PAUSE:          return KEY_PAUSE;
		case SDL_SCANCODE_INSERT:         return KEY_INS;
		case SDL_SCANCODE_HOME:           return KEY_HOME;
		case SDL_SCANCODE_PAGEUP:         return KEY_PGUP;
		case SDL_SCANCODE_DELETE:         return KEY_DEL;
		case SDL_SCANCODE_END:            return KEY_END;
		case SDL_SCANCODE_PAGEDOWN:       return KEY_PGDN;
		case SDL_SCANCODE_RIGHT:          return KEY_RIGHTARROW;
		case SDL_SCANCODE_LEFT:           return KEY_LEFTARROW;
		case SDL_SCANCODE_DOWN:           return KEY_DOWNARROW;
		case SDL_SCANCODE_UP:             return KEY_UPARROW;
		case SDL_SCANCODE_NUMLOCKCLEAR:   return KEY_NUMLOCK;
		case SDL_SCANCODE_KP_DIVIDE:      return KEY_KPADSLASH;
		case SDL_SCANCODE_KP_MULTIPLY:    return '*'; // undefined?
		case SDL_SCANCODE_KP_MINUS:       return KEY_MINUSPAD;
		case SDL_SCANCODE_KP_PLUS:        return KEY_PLUSPAD;
		case SDL_SCANCODE_KP_ENTER:       return KEY_ENTER;
		case SDL_SCANCODE_KP_PERIOD:      return KEY_KPADDEL;
		case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

		case SDL_SCANCODE_LSHIFT: return KEY_LSHIFT;
		case SDL_SCANCODE_RSHIFT: return KEY_RSHIFT;
		case SDL_SCANCODE_LCTRL:  return KEY_LCTRL;
		case SDL_SCANCODE_RCTRL:  return KEY_RCTRL;
		case SDL_SCANCODE_LALT:   return KEY_LALT;
		case SDL_SCANCODE_RALT:   return KEY_RALT;
		case SDL_SCANCODE_LGUI:   return KEY_LEFTWIN;
		case SDL_SCANCODE_RGUI:   return KEY_RIGHTWIN;
		default:                  break;
	}
	return 0;
}

extern "C" consvar_t cv_alwaysgrabmouse;

static boolean IgnoreMouse(void)
{
	if (cv_alwaysgrabmouse.value)
		return false;
	if (menuactive)
		return false; // return !M_MouseNeeded();
	if (paused || con_destlines || chat_on)
		return true;
	if (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION &&
			gamestate != GS_CONTINUING && gamestate != GS_CUTSCENE)
		return true;
	return false;
}

void I_UpdateMouseGrab(void)
{
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}

static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
	if (!infoSurface)
		return;

	if (!SurfaceText)
		SurfaceText = M_GetText("Unknown Surface");

	CONS_Printf("\x82" "%s\n", SurfaceText);
	CONS_Printf(M_GetText(" %ix%i, %s\n"), infoSurface->w, infoSurface->h, SDL_GetPixelFormatName(infoSurface->format));
}

static void VID_Command_Info_f (void)
{
	SurfaceInfo(bufSurface, M_GetText("Current Engine Mode"));
	SurfaceInfo(vidSurface, M_GetText("Current Video Mode"));
}

static void VID_Command_ModeList_f(void)
{
	// List windowed modes
	INT32 i = 0;
	for (i = 0; i < MAXWINMODES; i++)
	{
		CONS_Printf("%2d: %dx%d\n", i, windowedModes[i][0], windowedModes[i][1]);
	}

}

static void VID_Command_Mode_f (void)
{
	INT32 modenum;

	if (COM_Argc()!= 2)
	{
		CONS_Printf(M_GetText("vid_mode <modenum> : set video mode, current video mode %i\n"), vid.modenum);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes())
		CONS_Printf(M_GetText("Video mode not present\n"));
	else
		setmodeneeded = modenum+1; // request vid mode change
}

static inline void SDLJoyRemap(event_t *event)
{
	(void)event;
}

static INT32 SDLJoyAxis(const Sint16 axis, UINT8 pid)
{
	// -32768 to 32767
	INT32 raxis = axis / 32;

	if (Joystick[pid].bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (raxis < -(JOYAXISRANGE/2))
			raxis = -1;
		else if (raxis > (JOYAXISRANGE/2))
			raxis = 1;
		else
			raxis = 0;
	}
	else
	{
		raxis = (abs(JoyInfo[pid].scale) > 1) ? ((raxis / JoyInfo[pid].scale) * JoyInfo[pid].scale) : raxis;

#ifdef SDL_JDEADZONE
		if (-SDL_JDEADZONE <= raxis && raxis <= SDL_JDEADZONE)
			raxis = 0;
#endif
	}

	return raxis;
}

static void Impl_HandleWindowEvent(SDL_WindowEvent evt)
{
#define FOCUSUNION static_cast<unsigned int>(mousefocus | (kbfocus << 1))
	static bool firsttimeonmouse = true;
	static bool mousefocus = true;
	static bool kbfocus = true;

	const unsigned int oldfocus = FOCUSUNION;

	switch (evt.type)
	{
		case SDL_EVENT_WINDOW_MOUSE_ENTER:
			mousefocus = true;
			break;
		case SDL_EVENT_WINDOW_MOUSE_LEAVE:
			mousefocus = false;
			break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			kbfocus = true;
			mousefocus = true;
			SDL_HideCursor();
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			kbfocus = false;
			mousefocus = false;
			SDL_ShowCursor();
			break;
		case SDL_EVENT_WINDOW_MAXIMIZED:
			break;
		case SDL_EVENT_WINDOW_MOVED:
			window_x = evt.data1;
			window_y = evt.data2;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			vid.realwidth = evt.data1;
			vid.realheight = evt.data2;
			break;
		case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
			CV_SetValue(&cv_display, evt.data1);
			break;
		default:
			break;
	}

	if (FOCUSUNION == oldfocus) // No state change
	{
		return;
	}

	if (mousefocus && kbfocus)
	{
		// Tell game we got focus back, resume music if necessary
		window_notinfocus = false;

		S_SetMusicVolume();
		g_voice_disabled = cv_voice_selfdeafen.value;

		if (!firsttimeonmouse)
		{
			if (cv_usemouse.value) I_StartupMouse();
		}
	}
	else if (!mousefocus && !kbfocus)
	{
		// Tell game we lost focus, pause music
		window_notinfocus = true;
		if (!(cv_bgaudio.value & 1))
			I_SetMusicVolume(0);
		if (!(cv_bgaudio.value & 2))
			S_StopSounds();
		if (!(cv_bgaudio.value & 4))
			g_voice_disabled = true;

		G_ResetAllDeviceGameKeyDown();
		G_ResetAllDeviceResponding();
	}
#undef FOCUSUNION
}

static void Impl_HandleKeyboardEvent(SDL_KeyboardEvent evt, Uint32 type)
{
	event_t event;

	event.device = 0;

	if (type == SDL_EVENT_KEY_UP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_EVENT_KEY_DOWN)
	{
		event.type = ev_keydown;
	}
	else
	{
		return;
	}
	event.data1 = Impl_SDL_Scancode_To_Keycode(evt.scancode);
	event.data2 = evt.repeat;
	if (event.data1) D_PostEvent(&event);
}

static void Impl_HandleMouseMotionEvent(SDL_MouseMotionEvent evt)
{
}

static void Impl_HandleMouseButtonEvent(SDL_MouseButtonEvent evt, Uint32 type)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	// Ignore the event if the mouse is not actually focused on the window.
	// This can happen if you used the mouse to restore keyboard focus;
	// this apparently makes a mouse button down event but not a mouse button up event,
	// resulting in whatever key was pressed down getting "stuck" if we don't ignore it.
	// -- Monster Iestyn (28/05/18)
	if (SDL_GetMouseFocus() != window || IgnoreMouse())
		return;

	/// \todo inputEvent.button.which
	if (USE_MOUSEINPUT)
	{
		event.device = 0;

		if (type == SDL_EVENT_MOUSE_BUTTON_UP)
		{
			event.type = ev_keyup;
		}
		else if (type == SDL_EVENT_MOUSE_BUTTON_DOWN)
		{
			event.type = ev_keydown;
		}
		else return;
		if (evt.button == SDL_BUTTON_MIDDLE)
			event.data1 = KEY_MOUSE1+2;
		else if (evt.button == SDL_BUTTON_RIGHT)
			event.data1 = KEY_MOUSE1+1;
		else if (evt.button == SDL_BUTTON_LEFT)
			event.data1 = KEY_MOUSE1;
		else if (evt.button == SDL_BUTTON_X1)
			event.data1 = KEY_MOUSE1+3;
		else if (evt.button == SDL_BUTTON_X2)
			event.data1 = KEY_MOUSE1+4;
		if (event.type == ev_keyup || event.type == ev_keydown)
		{
			D_PostEvent(&event);
		}
	}
}

static void Impl_HandleMouseWheelEvent(SDL_MouseWheelEvent evt)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	event.device = 0;

	if (evt.y > 0)
	{
		event.data1 = KEY_MOUSEWHEELUP;
		event.type = ev_keydown;
	}
	if (evt.y < 0)
	{
		event.data1 = KEY_MOUSEWHEELDOWN;
		event.type = ev_keydown;
	}
	if (evt.y == 0)
	{
		event.data1 = 0;
		event.type = ev_keyup;
	}
	if (event.type == ev_keyup || event.type == ev_keydown)
	{
		D_PostEvent(&event);
	}
}

static void Impl_HandleControllerAxisEvent(SDL_GamepadAxisEvent evt)
{
	event_t event;
	INT32 value;

	event.type = ev_gamepad_axis;

	event.device = 1 + evt.which;
	if (event.device == INT32_MAX)
	{
		return;
	}

	event.data1 = event.data2 = event.data3 = INT32_MAX;

	//axis
	if (evt.axis > 2 * JOYAXISSETS)
	{
		return;
	}

	//vaule[sic]
	value = SDLJoyAxis(evt.value, evt.which);

	if (evt.axis & 1)
	{
		event.data3 = value;
	}
	else
	{
		event.data2 = value;
	}

	event.data1 = evt.axis / 2;

	D_PostEvent(&event);
}

static void Impl_HandleControllerButtonEvent(SDL_GamepadButtonEvent evt, Uint32 type)
{
	event_t event;

	event.device = 1 + evt.which;

	if (event.device == INT32_MAX)
	{
		return;
	}

	event.data1 = KEY_JOY1;
	event.data2 = 0;

	if (type == SDL_EVENT_GAMEPAD_BUTTON_UP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
	{
		event.type = ev_keydown;
	}
	else
	{
		return;
	}

	if (evt.button < JOYBUTTONS)
	{
		event.data1 += evt.button;
	}
	else
	{
		return;
	}

	SDLJoyRemap(&event);

	if (event.type != ev_console)
	{
		D_PostEvent(&event);
	}
}

static void Impl_HandleControllerDeviceAddedEvent(SDL_GamepadDeviceEvent event)
{
	// The game is always interested in controller events, even if they aren't internally assigned to a player.
	// Thus, we *always* open SDL controllers as they become available, to begin receiving their events.

	SDL_Gamepad* controller = SDL_OpenGamepad(event.which);
	if (controller == NULL)
	{
		return;
	}

	SDL_Joystick* joystick = SDL_GetGamepadJoystick(controller);
	SDL_JoystickID joystick_instance_id = SDL_GetJoystickID(joystick);

	event_t engine_event {};

	engine_event.type = ev_gamepad_device_added;
	engine_event.device = 1 + joystick_instance_id;

	D_PostEvent(&engine_event);
}

static void Impl_HandleControllerDeviceRemovedEvent(SDL_GamepadDeviceEvent event)
{
	// SDL only posts Device Removed events for controllers that have actually been opened.
	// Thus, we don't need to filter out controllers that may not have opened successfully prior to this event.
	event_t engine_event {};

	engine_event.type = ev_gamepad_device_removed;
	engine_event.device = 1 + event.which;

	D_PostEvent(&engine_event);
}

static ImGuiKey ImGui_ImplSDL3_KeycodeToImGuiKey(int keycode)
{
	switch (keycode)
	{
		case SDLK_TAB: return ImGuiKey_Tab;
		case SDLK_LEFT: return ImGuiKey_LeftArrow;
		case SDLK_RIGHT: return ImGuiKey_RightArrow;
		case SDLK_UP: return ImGuiKey_UpArrow;
		case SDLK_DOWN: return ImGuiKey_DownArrow;
		case SDLK_PAGEUP: return ImGuiKey_PageUp;
		case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
		case SDLK_HOME: return ImGuiKey_Home;
		case SDLK_END: return ImGuiKey_End;
		case SDLK_INSERT: return ImGuiKey_Insert;
		case SDLK_DELETE: return ImGuiKey_Delete;
		case SDLK_BACKSPACE: return ImGuiKey_Backspace;
		case SDLK_SPACE: return ImGuiKey_Space;
		case SDLK_RETURN: return ImGuiKey_Enter;
		case SDLK_ESCAPE: return ImGuiKey_Escape;
		case SDLK_APOSTROPHE: return ImGuiKey_Apostrophe;
		case SDLK_COMMA: return ImGuiKey_Comma;
		case SDLK_MINUS: return ImGuiKey_Minus;
		case SDLK_PERIOD: return ImGuiKey_Period;
		case SDLK_SLASH: return ImGuiKey_Slash;
		case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
		case SDLK_EQUALS: return ImGuiKey_Equal;
		case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
		case SDLK_BACKSLASH: return ImGuiKey_Backslash;
		case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
		case SDLK_GRAVE: return ImGuiKey_GraveAccent;
		case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
		case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
		case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
		case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
		case SDLK_PAUSE: return ImGuiKey_Pause;
		case SDLK_KP_0: return ImGuiKey_Keypad0;
		case SDLK_KP_1: return ImGuiKey_Keypad1;
		case SDLK_KP_2: return ImGuiKey_Keypad2;
		case SDLK_KP_3: return ImGuiKey_Keypad3;
		case SDLK_KP_4: return ImGuiKey_Keypad4;
		case SDLK_KP_5: return ImGuiKey_Keypad5;
		case SDLK_KP_6: return ImGuiKey_Keypad6;
		case SDLK_KP_7: return ImGuiKey_Keypad7;
		case SDLK_KP_8: return ImGuiKey_Keypad8;
		case SDLK_KP_9: return ImGuiKey_Keypad9;
		case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
		case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
		case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
		case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
		case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
		case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
		case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
		case SDLK_LSHIFT: return ImGuiKey_LeftShift;
		case SDLK_LALT: return ImGuiKey_LeftAlt;
		case SDLK_LGUI: return ImGuiKey_LeftSuper;
		case SDLK_RCTRL: return ImGuiKey_RightCtrl;
		case SDLK_RSHIFT: return ImGuiKey_RightShift;
		case SDLK_RALT: return ImGuiKey_RightAlt;
		case SDLK_RGUI: return ImGuiKey_RightSuper;
		case SDLK_APPLICATION: return ImGuiKey_Menu;
		case SDLK_0: return ImGuiKey_0;
		case SDLK_1: return ImGuiKey_1;
		case SDLK_2: return ImGuiKey_2;
		case SDLK_3: return ImGuiKey_3;
		case SDLK_4: return ImGuiKey_4;
		case SDLK_5: return ImGuiKey_5;
		case SDLK_6: return ImGuiKey_6;
		case SDLK_7: return ImGuiKey_7;
		case SDLK_8: return ImGuiKey_8;
		case SDLK_9: return ImGuiKey_9;
		case SDLK_A: return ImGuiKey_A;
		case SDLK_B: return ImGuiKey_B;
		case SDLK_C: return ImGuiKey_C;
		case SDLK_D: return ImGuiKey_D;
		case SDLK_E: return ImGuiKey_E;
		case SDLK_F: return ImGuiKey_F;
		case SDLK_G: return ImGuiKey_G;
		case SDLK_H: return ImGuiKey_H;
		case SDLK_I: return ImGuiKey_I;
		case SDLK_J: return ImGuiKey_J;
		case SDLK_K: return ImGuiKey_K;
		case SDLK_L: return ImGuiKey_L;
		case SDLK_M: return ImGuiKey_M;
		case SDLK_N: return ImGuiKey_N;
		case SDLK_O: return ImGuiKey_O;
		case SDLK_P: return ImGuiKey_P;
		case SDLK_Q: return ImGuiKey_Q;
		case SDLK_R: return ImGuiKey_R;
		case SDLK_S: return ImGuiKey_S;
		case SDLK_T: return ImGuiKey_T;
		case SDLK_U: return ImGuiKey_U;
		case SDLK_V: return ImGuiKey_V;
		case SDLK_W: return ImGuiKey_W;
		case SDLK_X: return ImGuiKey_X;
		case SDLK_Y: return ImGuiKey_Y;
		case SDLK_Z: return ImGuiKey_Z;
		case SDLK_F1: return ImGuiKey_F1;
		case SDLK_F2: return ImGuiKey_F2;
		case SDLK_F3: return ImGuiKey_F3;
		case SDLK_F4: return ImGuiKey_F4;
		case SDLK_F5: return ImGuiKey_F5;
		case SDLK_F6: return ImGuiKey_F6;
		case SDLK_F7: return ImGuiKey_F7;
		case SDLK_F8: return ImGuiKey_F8;
		case SDLK_F9: return ImGuiKey_F9;
		case SDLK_F10: return ImGuiKey_F10;
		case SDLK_F11: return ImGuiKey_F11;
		case SDLK_F12: return ImGuiKey_F12;
	}
	return ImGuiKey_None;
}

static void ImGui_ImplSDL3_UpdateKeyModifiers(SDL_Keymod sdl_key_mods)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddKeyEvent(ImGuiMod_Ctrl, (sdl_key_mods & SDL_KMOD_CTRL) != 0);
	io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & SDL_KMOD_SHIFT) != 0);
	io.AddKeyEvent(ImGuiMod_Alt, (sdl_key_mods & SDL_KMOD_ALT) != 0);
	io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & SDL_KMOD_GUI) != 0);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.
bool ImGui_ImplSDL3_ProcessEvent(const SDL_Event* event)
{
	ImGuiIO& io = ImGui::GetIO();

	switch (event->type)
	{
		case SDL_EVENT_MOUSE_MOTION:
		{
			io.AddMousePosEvent((float)event->motion.x, (float)event->motion.y);
			return true;
		}
		case SDL_EVENT_MOUSE_WHEEL:
		{
			float wheel_x = (event->wheel.x > 0) ? 1.0f : (event->wheel.x < 0) ? -1.0f : 0.0f;
			float wheel_y = (event->wheel.y > 0) ? 1.0f : (event->wheel.y < 0) ? -1.0f : 0.0f;
			io.AddMouseWheelEvent(wheel_x, wheel_y);
			return true;
		}
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			int mouse_button = -1;
			if (event->button.button == SDL_BUTTON_LEFT) { mouse_button = 0; }
			if (event->button.button == SDL_BUTTON_RIGHT) { mouse_button = 1; }
			if (event->button.button == SDL_BUTTON_MIDDLE) { mouse_button = 2; }
			if (event->button.button == SDL_BUTTON_X1) { mouse_button = 3; }
			if (event->button.button == SDL_BUTTON_X2) { mouse_button = 4; }
			if (mouse_button == -1)
				break;
			io.AddMouseButtonEvent(mouse_button, (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN));
			return true;
		}
		case SDL_EVENT_TEXT_INPUT:
		{
			io.AddInputCharactersUTF8(event->text.text);
			return true;
		}
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		{
			ImGui_ImplSDL3_UpdateKeyModifiers((SDL_Keymod)event->key.mod);
			ImGuiKey key = ImGui_ImplSDL3_KeycodeToImGuiKey(event->key.key);
			io.AddKeyEvent(key, (event->type == SDL_EVENT_KEY_DOWN));
			io.SetKeyEventNativeData(key, event->key.key, event->key.scancode, event->key.scancode);
			return true;
		}
		case SDL_EVENT_WINDOW_MOUSE_ENTER:
		{
			return true;
		}
		case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		{
			return true;
		}
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		{
			io.AddFocusEvent(true);
			return true;
		}
		case SDL_EVENT_WINDOW_FOCUS_LOST:
		{
			io.AddFocusEvent(false);
			return true;
		}
	}
	return false;
}

void I_GetEvent(void)
{
	SDL_Event evt;

	// We only want the first motion event,
	// otherwise we'll end up catching the warp back to center.
	//int mouseMotionOnce = 0;

	if (!graphics_started)
	{
		return;
	}

	mousemovex = mousemovey = 0;

	ImGuiIO& io = ImGui::GetIO();

	while (SDL_PollEvent(&evt))
	{
		ImGui_ImplSDL3_ProcessEvent(&evt);
		if (io.WantCaptureMouse || io.WantCaptureKeyboard)
		{
			continue;
		}

		switch (evt.type)
		{
			case SDL_EVENT_WINDOW_MOUSE_ENTER:
			case SDL_EVENT_WINDOW_MOUSE_LEAVE:
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
			case SDL_EVENT_WINDOW_FOCUS_LOST:
			case SDL_EVENT_WINDOW_MAXIMIZED:
			case SDL_EVENT_WINDOW_MOVED:
			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
				Impl_HandleWindowEvent(evt.window);
				break;
			case SDL_EVENT_KEY_UP:
			case SDL_EVENT_KEY_DOWN:
				Impl_HandleKeyboardEvent(evt.key, evt.type);
				break;
			case SDL_EVENT_MOUSE_MOTION:
				//if (!mouseMotionOnce)
				Impl_HandleMouseMotionEvent(evt.motion);
				//mouseMotionOnce = 1;
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				Impl_HandleMouseButtonEvent(evt.button, evt.type);
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				Impl_HandleMouseWheelEvent(evt.wheel);
				break;
			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				Impl_HandleControllerAxisEvent(evt.gaxis);
				break;
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				Impl_HandleControllerButtonEvent(evt.gbutton, evt.type);
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				Impl_HandleControllerDeviceAddedEvent(evt.gdevice);
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				Impl_HandleControllerDeviceRemovedEvent(evt.gdevice);
				break;

			case SDL_EVENT_QUIT:
				LUA_HookBool(true, HOOK(GameQuit));
				I_Quit();
				break;

			default:
				break;
		}
	}

	// Send all relative mouse movement as one single mouse event.
	if (mousemovex || mousemovey)
	{
		event_t event;
		int wwidth, wheight;
		SDL_GetWindowSize(window, &wwidth, &wheight);
		//SDL_memset(&event, 0, sizeof(event_t));
		event.type = ev_mouse;
		event.data1 = 0;
		event.data2 = (INT32)lround(mousemovex * ((float)wwidth / (float)realwidth));
		event.data3 = (INT32)lround(mousemovey * ((float)wheight / (float)realheight));
		D_PostEvent(&event);
	}

	// In order to make wheels act like buttons, we have to set their state to Up.
	// This is because wheel messages don't have an up/down state.
	G_GetDeviceGameKeyDownArray(0)[KEY_MOUSEWHEELDOWN] = G_GetDeviceGameKeyDownArray(0)[KEY_MOUSEWHEELUP] = 0;
}

void I_StartupMouse(void)
{
	if (disable_mouse)
		return;

	SDL_HideCursor();
}

//
// I_OsPolling
//
void I_OsPolling(void)
{
	SDL_Keymod mod;

	if (consolevent)
		I_GetConsoleEvents();

	if (SDL_WasInit(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD) == (SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD))
		SDL_UpdateGamepads();

	I_GetEvent();

	mod = SDL_GetModState();
	/* Handle here so that our state is always synched with the system. */
	shiftdown = ctrldown = altdown = 0;
	capslock = false;
	if (mod & SDL_KMOD_LSHIFT) shiftdown |= 1;
	if (mod & SDL_KMOD_RSHIFT) shiftdown |= 2;
	if (mod & SDL_KMOD_LCTRL)   ctrldown |= 1;
	if (mod & SDL_KMOD_RCTRL)   ctrldown |= 2;
	if (mod & SDL_KMOD_LALT)     altdown |= 1;
	if (mod & SDL_KMOD_RALT)     altdown |= 2;
	if (mod & SDL_KMOD_CAPS) capslock = true;
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
	if (rendermode == render_none)
		return;
	if (exposevideo)
	{
#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			OglSdlFinishUpdate(cv_vidwait.value);
		}
#endif
	}
	exposevideo = false;
}

//
// I_FinishUpdate
//
static SDL_Rect src_rect = { 0, 0, 0, 0 };

//
// I_UpdateNoVsync
//
void I_UpdateNoVsync(void)
{
	INT32 real_vidwait = cv_vidwait.value;
	cv_vidwait.value = 0;
	I_FinishUpdate();
	cv_vidwait.value = real_vidwait;
}

//
// I_ReadScreen
//
void I_ReadScreen(UINT8 *scr)
{
	if (rendermode == render_opengl)
		I_Error ("I_ReadScreen: called while in Legacy GL mode");
	else
		VID_BlitLinearScreen(screens[0], scr,
			vid.width*vid.bpp, vid.height,
			vid.rowbytes, vid.rowbytes);
}

//
// I_SetPalette
//
void I_SetPalette(RGBA_t *palette)
{
	size_t i;
	for (i=0; i<256; i++)
	{
		localPalette[i].r = palette[i].s.red;
		localPalette[i].g = palette[i].s.green;
		localPalette[i].b = palette[i].s.blue;
	}
}

// return number of fullscreen + X11 modes
INT32 VID_NumModes(void)
{
	if (USE_FULLSCREEN && numVidModes != -1)
		return numVidModes - firstEntry;
	else
		return MAXWINMODES;
}

const char *VID_GetModeName(INT32 modeNum)
{
#if 0
	if (USE_FULLSCREEN && numVidModes != -1) // fullscreen modes
	{
		modeNum += firstEntry;
		if (modeNum >= numVidModes)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			modeList[modeNum]->w,
			modeList[modeNum]->h);
	}
	else // windowed modes
	{
#endif
	if (modeNum == -1)
	{
		return fallback_resolution_name;
	}
		if (modeNum > MAXWINMODES)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			windowedModes[modeNum][0],
			windowedModes[modeNum][1]);
	//}
	return &vidModeName[modeNum][0];
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	int i;
	for (i = 0; i < MAXWINMODES; i++)
	{
		if (windowedModes[i][0] == w && windowedModes[i][1] == h)
		{
			return i;
		}
	}
	return -1;
#if 0
	INT32 matchMode = -1, i;
	VID_PrepareModeList();
	if (USE_FULLSCREEN && numVidModes != -1)
	{
		for (i=firstEntry; i<numVidModes; i++)
		{
			if (modeList[i]->w == w &&
			    modeList[i]->h == h)
			{
				matchMode = i;
				break;
			}
		}
		if (-1 == matchMode) // use smaller mode
		{
			w -= w%BASEVIDWIDTH;
			h -= h%BASEVIDHEIGHT;
			for (i=firstEntry; i<numVidModes; i++)
			{
				if (modeList[i]->w == w &&
				    modeList[i]->h == h)
				{
					matchMode = i;
					break;
				}
			}
			if (-1 == matchMode) // use smallest mode
				matchMode = numVidModes-1;
		}
		matchMode -= firstEntry;
	}
	else
	{
		for (i=0; i<MAXWINMODES; i++)
		{
			if (windowedModes[i][0] == w &&
			    windowedModes[i][1] == h)
			{
				matchMode = i;
				break;
			}
		}
		if (-1 == matchMode) // use smaller mode
		{
			w -= w%BASEVIDWIDTH;
			h -= h%BASEVIDHEIGHT;
			for (i=0; i<MAXWINMODES; i++)
			{
				if (windowedModes[i][0] == w &&
				    windowedModes[i][1] == h)
				{
					matchMode = i;
					break;
				}
			}
			if (-1 == matchMode) // use smallest mode
				matchMode = MAXWINMODES-1;
		}
	}
	return matchMode;
#endif
}

void VID_PrepareModeList(void)
{
	// Under SDL3, we just use the windowed modes list, and scale in windowed fullscreen.
	allow_fullscreen = true;
#if 0
	INT32 i;

	firstEntry = 0;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		modeList = SDL_ListModes(NULL, SDL_OPENGL|SDL_FULLSCREEN);
	else
#endif
	modeList = SDL_ListModes(NULL, surfaceFlagsF|SDL_HWSURFACE); //Alam: At least hardware surface

	if (disable_fullscreen?0:cv_fullscreen.value) // only fullscreen needs preparation
	{
		if (-1 != numVidModes)
		{
			for (i=0; i<numVidModes; i++)
			{
				if (modeList[i]->w <= MAXVIDWIDTH &&
					modeList[i]->h <= MAXVIDHEIGHT)
				{
					firstEntry = i;
					break;
				}
			}
		}
	}
	allow_fullscreen = true;
#endif
}

static void init_imgui()
{
	if (ImGui::GetCurrentContext() != NULL)
	{
		return;
	}

	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	io.BackendFlags = 0;
	io.BackendRendererName = "SRB2 SDL 2 RHI";
	io.Fonts->AddFontDefault();
	{
		unsigned char* pixels;
		int width;
		int height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	}
	ImGui::StyleColorsDark();
}

static bool Impl_CreateContext(void)
{
	if (!sdlglcontext)
	{
		SDL_GL_ResetAttributes();
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		sdlglcontext = SDL_GL_CreateContext(window);
	}
	if (sdlglcontext == NULL)
	{
		SDL_DestroyWindow(window);
		I_Error("Failed to create a GL context: %s\n", SDL_GetError());
	}
	init_imgui();
	SDL_GL_MakeCurrent(window, sdlglcontext);

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		LoadGL();
	}
#endif

	if (!g_rhi)
	{
		std::unique_ptr<rhi::SdlGl2Platform> platform = std::make_unique<rhi::SdlGl2Platform>();
		platform->window = window;
		g_rhi = std::make_unique<rhi::Gl2Rhi>(std::move(platform), reinterpret_cast<rhi::GlLoadFunc>(SDL_GL_GetProcAddress));
		g_rhi_generation += 1;
	}

	return true;
}

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
}

boolean VID_CheckRenderer(void)
{
	boolean rendererchanged = false;

	if (dedicated)
		return false;

	if (setrenderneeded)
	{
		rendermode = static_cast<rendermode_t>(setrenderneeded);
		rendererchanged = true;

		if (rendererchanged)
		{
			Impl_CreateContext();
#ifdef HWRENDER
			if (rendermode == render_opengl)
			{
				VID_StartupOpenGL();
				if (vid.glstate != VID_GL_LIBRARY_LOADED)
				{
					rendererchanged = false;
				}
			}
#endif
		}

		setrenderneeded = 0;
	}

	SDLSetMode(vid.width, vid.height, static_cast<bool>(USE_FULLSCREEN), (setmodeneeded ? true : false));
	Impl_VideoSetupBuffer();

	if (rendermode == render_soft)
	{
		SCR_SetDrawFuncs();
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && rendererchanged)
	{
		HWR_Switch();
		V_SetPalette(0);
	}
#endif

	M_RefreshAdvancedVideoOptions();

	return rendererchanged;
}

static UINT32 refresh_rate;
static UINT32 VID_GetRefreshRate(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		// Video not init yet.
		return 0;
	}

	SDL_DisplayID display_id = SDL_GetDisplayForWindow(window);
	const SDL_DisplayMode *m = SDL_GetCurrentDisplayMode(display_id);

	if (m == nullptr)
	{
		// Error has occurred.
		return 0;
	}

	return static_cast<UINT32>(m->refresh_rate);
}

INT32 VID_SetMode(INT32 modeNum)
{
	vid.recalc = 1;
	vid.bpp = 1;

	if (modeNum < 0)
		modeNum = 0;
	if (modeNum >= MAXWINMODES)
		modeNum = MAXWINMODES-1;

	vid.width = windowedModes[modeNum][0];
	vid.height = windowedModes[modeNum][1];
	vid.realwidth = vid.width;
	vid.realheight = vid.height;
	vid.modenum = modeNum;

	src_rect.w = vid.width;
	src_rect.h = vid.height;

	refresh_rate = VID_GetRefreshRate();

	VID_CheckRenderer();
	return true;
}

static bool Impl_CreateWindow(bool fullscreen)
{
	uint32_t flags = SDL_WINDOW_RESIZABLE;

	if (rendermode == render_none) // dedicated
		return true; // Monster Iestyn -- not sure if it really matters what we return here tbh

	if (window != NULL)
		return false;

	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;

	if (borderlesswindow)
		flags |= SDL_WINDOW_BORDERLESS;

	// RHI: always create window as OPENGL
	flags |= SDL_WINDOW_OPENGL;

	// Create a window
	window = SDL_CreateWindow("Dr. Robotnik's Ring Racers " VERSIONSTRING,
			realwidth, realheight, flags);

	if (window == NULL)
	{
		CONS_Printf(M_GetText("Couldn't create window: %s\n"), SDL_GetError());
		return false;
	}

	if (!fullscreen)
	{
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	Impl_SetWindowIcon();

	return Impl_CreateContext();
}

static void Impl_SetWindowIcon(void)
{
	if (window && icoSurface)
		SDL_SetWindowIcon(window, icoSurface);
}

static void Impl_VideoSetupBuffer(void)
{
	// Set up game's software render buffer
	vid.rowbytes = vid.width * vid.bpp;
	vid.direct = NULL;
	if (vid.buffer)
		free(vid.buffer);
	vid.buffer = static_cast<uint8_t*>(calloc(vid.rowbytes*vid.height, NUMSCREENS));
	if (!vid.buffer)
	{
		I_Error("%s", M_GetText("Not enough memory for video buffer\n"));
	}
}

extern "C" CVarList* cvlist_graphics_driver;

void I_StartupGraphics(void)
{
	if (dedicated)
	{
		rendermode = render_none;
		return;
	}
	if (graphics_started)
		return;

	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f);
	COM_AddCommand ("vid_info", VID_Command_Info_f);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f);
	CV_RegisterList(cvlist_graphics_driver);
	disable_mouse = static_cast<bool>(M_CheckParm("-nomouse"));
	disable_fullscreen = M_CheckParm("-win") ? true : false;

	keyboard_started = true;

#if !defined(HAVE_TTF)
	// Previously audio was init here for questionable reasons?
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
	{
		CONS_Printf(M_GetText("Couldn't initialize SDL's Video System: %s\n"), SDL_GetError());
		return;
	}
#endif

	// Renderer choices
	// Takes priority over the config.
	if (M_CheckParm("-renderer"))
	{
		INT32 i = 0;
		CV_PossibleValue_t *renderer_list = cv_renderer_t;
		const char *modeparm = M_GetNextParm();
		while (renderer_list[i].strvalue)
		{
			if (!stricmp(modeparm, renderer_list[i].strvalue))
			{
				chosenrendermode = static_cast<rendermode_t>(renderer_list[i].value);
				break;
			}
			i++;
		}
	}

	// Choose Software renderer
	else if (M_CheckParm("-software"))
		chosenrendermode = render_soft;

#ifdef HWRENDER
	// Choose OpenGL renderer
	else if (M_CheckParm("-opengl"))
		chosenrendermode = render_opengl;

	// Don't startup OpenGL
	if (M_CheckParm("-nogl"))
	{
		vid.glstate = VID_GL_LIBRARY_ERROR;
		if (chosenrendermode == render_opengl)
			chosenrendermode = render_none;
	}
#endif

	if (chosenrendermode != render_none)
		rendermode = chosenrendermode;

	borderlesswindow = M_CheckParm("-borderless") ? true : false;

	VID_Command_ModeList_f();

#ifdef HWRENDER
	if (rendermode == render_opengl)
		VID_StartupOpenGL();
#endif

	// Window icon
#if USE_XPM_ICON
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif

	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));

	vid.width = BASEVIDWIDTH; // Default size for startup
	vid.height = BASEVIDHEIGHT; // BitsPerPixel is the SDL interface's
	vid.recalc = true; // Set up the console stufff
	vid.direct = NULL; // Maybe direct access?
	vid.bpp = 1; // This is the game engine's Bpp
	vid.WndParent = NULL; //For the window?

	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));

	if (M_CheckParm("-nomousegrab"))
		mousegrabok = false;
	realwidth = (Uint16)vid.width;
	realheight = (Uint16)vid.height;

	VID_Command_Info_f();

	SDL_RaiseWindow(window);

	graphics_started = true;

#ifdef __APPLE__
	// Must pump events once after creating window before window will actually appear.
	SDL_PumpEvents();
#endif
}

void VID_StartupOpenGL(void)
{
#ifdef HWRENDER
	static boolean glstartup = false;
	if (!glstartup)
	{
		CONS_Printf("VID_StartupOpenGL()...\n");
		*(void**)&HWD.pfnInit             = hwSym("Init",NULL);
		*(void**)&HWD.pfnFinishUpdate     = NULL;
		*(void**)&HWD.pfnDraw2DLine       = hwSym("Draw2DLine",NULL);
		*(void**)&HWD.pfnDrawPolygon      = hwSym("DrawPolygon",NULL);
		*(void**)&HWD.pfnDrawIndexedTriangles = hwSym("DrawIndexedTriangles",NULL);
		*(void**)&HWD.pfnRenderSkyDome    = hwSym("RenderSkyDome",NULL);
		*(void**)&HWD.pfnSetBlend         = hwSym("SetBlend",NULL);
		*(void**)&HWD.pfnClearBuffer      = hwSym("ClearBuffer",NULL);
		*(void**)&HWD.pfnSetTexture       = hwSym("SetTexture",NULL);
		*(void**)&HWD.pfnUpdateTexture    = hwSym("UpdateTexture",NULL);
		*(void**)&HWD.pfnDeleteTexture    = hwSym("DeleteTexture",NULL);
		*(void**)&HWD.pfnReadRect         = hwSym("ReadRect",NULL);
		*(void**)&HWD.pfnGClipRect        = hwSym("GClipRect",NULL);
		*(void**)&HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache",NULL);
		*(void**)&HWD.pfnSetSpecialState  = hwSym("SetSpecialState",NULL);
		*(void**)&HWD.pfnSetPalette       = hwSym("SetPalette",NULL);
		*(void**)&HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed",NULL);
		*(void**)&HWD.pfnDrawModel        = hwSym("DrawModel",NULL);
		*(void**)&HWD.pfnCreateModelVBOs  = hwSym("CreateModelVBOs",NULL);
		*(void**)&HWD.pfnSetTransform     = hwSym("SetTransform",NULL);
		*(void**)&HWD.pfnPostImgRedraw    = hwSym("PostImgRedraw",NULL);
		*(void**)&HWD.pfnFlushScreenTextures=hwSym("FlushScreenTextures",NULL);
		*(void**)&HWD.pfnStartScreenWipe  = hwSym("StartScreenWipe",NULL);
		*(void**)&HWD.pfnEndScreenWipe    = hwSym("EndScreenWipe",NULL);
		*(void**)&HWD.pfnDoScreenWipe     = hwSym("DoScreenWipe",NULL);
		*(void**)&HWD.pfnDrawIntermissionBG=hwSym("DrawIntermissionBG",NULL);
		*(void**)&HWD.pfnMakeScreenTexture= hwSym("MakeScreenTexture",NULL);
		*(void**)&HWD.pfnMakeScreenFinalTexture=hwSym("MakeScreenFinalTexture",NULL);
		*(void**)&HWD.pfnDrawScreenFinalTexture=hwSym("DrawScreenFinalTexture",NULL);

		*(void**)&HWD.pfnCompileShaders   = hwSym("CompileShaders",NULL);
		*(void**)&HWD.pfnCleanShaders     = hwSym("CleanShaders",NULL);
		*(void**)&HWD.pfnSetShader        = hwSym("SetShader",NULL);
		*(void**)&HWD.pfnUnSetShader      = hwSym("UnSetShader",NULL);

		*(void**)&HWD.pfnSetShaderInfo    = hwSym("SetShaderInfo",NULL);
		*(void**)&HWD.pfnLoadCustomShader = hwSym("LoadCustomShader",NULL);
		*(void**)&HWD.pfnResetRenderState = hwSym("ResetRenderState",NULL);
		glstartup = true;
	}

	// For RHI-Legacy GL compatibility: Init always fetches GL functions, but only dlsym's libraries once.
	vid.glstate = HWD.pfnInit() ? VID_GL_LIBRARY_LOADED : VID_GL_LIBRARY_ERROR; // let load the OpenGL library

	if (vid.glstate == VID_GL_LIBRARY_ERROR)
	{
		rendermode = render_soft;
		setrenderneeded = 0;
	}
#endif
}

void I_ShutdownGraphics(void)
{
	rendermode = render_none;
	if (icoSurface) SDL_DestroySurface(icoSurface);
	icoSurface = NULL;

	I_OutputMsg("I_ShutdownGraphics(): ");

	// was graphics initialized anyway?
	if (!graphics_started)
	{
		return;
	}
	graphics_started = false;

	g_rhi.reset();
	g_rhi_generation = 0;

#ifdef HWRENDER
	if (GLUhandle)
		hwClose(GLUhandle);
	if (sdlglcontext)
	{
		SDL_GL_DestroyContext(sdlglcontext);
		sdlglcontext = nullptr;
	}
#endif
	window = NULL;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	framebuffer = false;
}

rhi::Rhi* srb2::sys::get_rhi(rhi::Handle<rhi::Rhi> handle)
{
	// TODO actually use handle...
	return g_rhi.get();
}

#endif

UINT32 I_GetRefreshRate(void)
{
	// Moved to VID_GetRefreshRate.
	// Precalculating it like that won't work as
	// well for windowed mode since you can drag
	// the window around, but very slow PCs might have
	// trouble querying mode over and over again.
	return refresh_rate;
}

namespace srb2::cvarhandler
{
void on_set_vid_wait();
}

void srb2::cvarhandler::on_set_vid_wait()
{
	int interval = 0;
	if (cv_vidwait.value > 0)
	{
		interval = 1;
	}

	if (sdlglcontext == nullptr || SDL_GL_GetCurrentContext() != sdlglcontext)
	{
		return;
	}
	SDL_GL_SetSwapInterval(interval);
}
