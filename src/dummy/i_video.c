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
#include "../command.h"
#include "../i_video.h"

rendermode_t rendermode = render_none;
rendermode_t chosenrendermode = render_none;

dboolean highcolor = false;

dboolean allow_fullscreen = false;

consvar_t cv_vidwait = CVAR_INIT ("vid_wait", "On", CV_SAVE, CV_OnOff, NULL);

void I_StartupGraphics(void){}
void I_ShutdownGraphics(void){}

void VID_StartupOpenGL(void){}

void I_SetPalette(RGBA_t *palette)
{
	(void)palette;
}

int32_t VID_NumModes(void)
{
	return 0;
}

int32_t VID_GetModeForSize(int32_t w, int32_t h)
{
	(void)w;
	(void)h;
	return 0;
}

void VID_PrepareModeList(void){}

int32_t VID_SetMode(int32_t modenum)
{
	(void)modenum;
	return 0;
}

dboolean VID_CheckRenderer(void)
{
	return false;
}

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
}

const char *VID_GetModeName(int32_t modenum)
{
	(void)modenum;
	return NULL;
}

void I_UpdateNoBlit(void){}

void I_FinishUpdate(void){}

void I_UpdateNoVsync(void) {}

void I_WaitVBL(int32_t count)
{
	(void)count;
}

void I_ReadScreen(uint8_t *scr)
{
	(void)scr;
}

void I_BeginRead(void){}

void I_EndRead(void){}

