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

#include "../doomdef.h"
#include "../doomtype.h"
#include "../i_system.h"

FILE *logstream = NULL;

uint8_t graphics_started = 0;

uint8_t keyboard_started = 0;

uint64_t I_GetFreeMem(uint64_t *total)
{
	*total = 0;
	return 0;
}

void I_Sleep(uint32_t ms){}

precise_t I_GetPreciseTime(void) {
	return 0;
}

uint64_t I_GetPrecisePrecision(void) {
	return 1000000;
}

void I_GetEvent(void){}

void I_OsPolling(void){}

ticcmd_t *I_BaseTiccmd(void)
{
	return NULL;
}

ticcmd_t *I_BaseTiccmd2(void)
{
	return NULL;
}

void I_Quit(void)
{
	exit(0);
}

void I_Error(const char *error, ...)
{
	(void)error;
	exit(-1);
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void){}

void I_InitJoystick2(void){}

int32_t I_NumJoys(void)
{
	return 0;
}

const char *I_GetJoyName(int32_t joyindex)
{
	(void)joyindex;
	return NULL;
}

#ifndef NOMUMBLE
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
	(void)mobj;
	(void)listener;
}
#endif

void I_OutputMsg(const char *error, ...)
{
	(void)error;
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

int32_t I_GetKey(void)
{
	return 0;
}

void I_StartupTimer(void){}

void I_AddExitFunc(void (*func)())
{
	(void)func;
}

void I_RemoveExitFunc(void (*func)())
{
	(void)func;
}

int32_t I_StartupSystem(void)
{
	return -1;
}

void I_ShutdownSystem(void){}

void I_GetDiskFreeSpace(int64_t* freespace)
{
	*freespace = 0;
}

char *I_GetUserName(void)
{
	return NULL;
}

int32_t I_mkdir(const char *dirname, int32_t unixright)
{
	(void)dirname;
	(void)unixright;
	return -1;
}

int32_t I_ChDir(const char *path)
{
	(void)path;
	return -1;
}

char *I_GetCwd(char *buf, size_t size)
{
	if (size > 0)
		buf[0] = '\0';
	return NULL;
}

const char *I_LocateWad(void)
{
	return NULL;
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

void I_UpdateMouseGrab(void){}

char *I_GetEnv(const char *name)
{
	(void)name;
	return NULL;
}

int32_t I_PutEnv(char *variable)
{
	(void)variable;
	return -1;
}

int32_t I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	return NULL;
}

void I_RegisterSysCommands(void) {}

void I_GetCursorPosition(int32_t *x, int32_t *y)
{
	(void)x;
	(void)y;
}

dboolean I_HasOpenURL()
{
	return 0;
}

void I_OpenURL(const char *data)
{
	return;
}

#include "../sdl/dosstr.c"

