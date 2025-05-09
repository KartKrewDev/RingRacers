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
/// \file  d_main.h
/// \brief game startup, and main loop code, system specific interface stuff.

#ifndef __D_MAIN__
#define __D_MAIN__

#include "k_profiles.h"	// PR_LoadProfiles()
#include "d_event.h"
#include "w_wad.h"   // for MAX_WADFILES

#ifdef __cplusplus
extern "C" {
#endif

// make sure not to write back the config until it's been correctly loaded
extern tic_t rendergametic;

extern char srb2home[256]; //Alam: My Home
extern boolean usehome; //Alam: which path?
extern const char *pandf; //Alam: how to path?
extern char srb2path[256]; //Alam: SRB2's Home
extern char addonsdir[MAX_WADPATH]; // Where addons are stored

// the infinite loop of D_SRB2Loop() called from win_main for windows version
void D_SRB2Loop(void) FUNCNORETURN;

//
// D_SRB2Main()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls D_AdvanceDemo.
//
void D_SRB2Main(void);

const char *D_GetFancyBranchName(void);

// Called by IO functions when input is detected.
void D_PostEvent(const event_t *ev);
#if defined (PC_DOS) && !defined (DOXYGEN)
void D_PostEvent_end(void);    // delimiter for locking memory
#endif

void D_ProcessEvents(boolean callresponders);

const char *D_Home(void);

void D_TakeMapSnapshots(void);

//
// BASE LEVEL
//
void D_ClearState(void);
void D_StartTitle(void);
void D_SetDeferredStartTitle(boolean deferred);
boolean D_IsDeferredStartTitle(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__D_MAIN__
