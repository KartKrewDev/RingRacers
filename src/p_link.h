// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef p_link_h
#define p_link_h

#include "typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

void P_InitMobjPointers(void);
void P_SaveMobjPointers(void(*callback)(mobj_t*));
void P_LoadMobjPointers(void(*callback)(mobj_t**));

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*p_link_h*/
