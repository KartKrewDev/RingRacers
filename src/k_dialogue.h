// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sonic Team Junior
// Copyright (C) by Kart Krew
// Copyright (C) by Sally "TehRealSalt" Cochenour
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_dialogue.h
/// \brief Basic text prompts

#ifndef __K_DIALOGUE__
#define __K_DIALOGUE__

#include "doomtype.h"
#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void K_DismissDialogue(void);
void K_DrawDialogue(void);
void K_TickDialogue(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__K_DIALOGUE__
