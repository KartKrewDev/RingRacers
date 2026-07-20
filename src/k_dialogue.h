// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
// Copyright (C) 2020 by Sonic Team Junior
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_dialogue.h
/// \brief Basic text prompts

#ifndef K_DIALOGUE_H
#define K_DIALOGUE_H

#include "doomtype.h"
#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void K_UnsetDialogue(void);
void K_DrawDialogue(void);
void K_TickDialogue(void);

boolean K_DialogueFreeze(void);
INT32 K_GetDialogueSlide(fixed_t multiplier);
INT32 K_GetDialogueFade(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //K_DIALOGUE_H
