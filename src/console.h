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
/// \file  console.h
/// \brief Console drawing and input

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "d_event.h"
#include "command.h"
#include "i_threads.h"

#ifdef __cplusplus
extern "C" {
#endif

void CON_Init(void);

boolean CON_Responder(event_t *ev);

#ifdef HAVE_THREADS
extern I_mutex con_mutex;
#endif

// set true when screen size has changed, to adapt console
extern boolean con_recalc;

// console being displayed at game startup
extern boolean con_startup;

// when modifying the below, you must also adjust d_main and console.c
typedef enum
{
	LOADED_ZINIT = 1,
	LOADED_ISTARTUPTIMER,
	LOADED_IWAD,
	LOADED_ISTARTUPGRAPHICS,
	LOADED_HUINIT,
	LOADED_PWAD,
	LOADED_CONFIG,
	LOADED_INITTEXTUREDATA,
	LOADED_INITSPRITES,
	LOADED_INITSKINS,
	LOADED_RINIT,
	LOADED_SINITSFXCHANNELS,
	LOADED_STINIT,
	LOADED_ACSINIT,
	LOADED_DCHECKNETGAME,
	LOADED_ALLDONE = LOADED_DCHECKNETGAME,
} con_loadprogress_t;

extern con_loadprogress_t con_startup_loadprogress;

// top clip value for view render: do not draw part of view hidden by console
extern INT32 con_clipviewtop;

// 0 means console if off, or moving out
extern INT32 con_destlines;

extern INT32 con_clearlines; // lines of top of screen to refresh
extern boolean con_hudupdate; // hud messages have changed, need refresh
extern UINT32 con_scalefactor; // console text scale factor

extern consvar_t cons_hudtime, cons_hudlines, cons_speed, cons_height, cons_backpic, cons_backcolor;

extern UINT8 *yellowmap, *purplemap, *greenmap, *bluemap, *graymap, *redmap, *orangemap,\
 *skymap, *goldmap, *lavendermap, *aquamap, *magentamap, *pinkmap, *brownmap, *tanmap;

// Console bg color (auto updated to match)
extern UINT8 *consolebgmap;
extern UINT8 *promptbgmap;

INT32 CON_ShiftChar(INT32 ch);
void CON_SetupBackColormapEx(INT32 color, boolean prompt);
void CON_SetupBackColormap(void);
void CON_ClearHUD(void); // clear heads up messages

void CON_Ticker(void);
void CON_Drawer(void);
void CONS_Error(const char *msg); // print out error msg, and wait a key

// force console to move out
void CON_ToggleOff(void);

// Is console down?
boolean CON_Ready(void);

void CON_LogMessage(const char *msg);

// Startup loading bar
void CON_SetLoadingProgress(con_loadprogress_t newStep);
void CON_DrawLoadBar(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __CONSOLE_H__
