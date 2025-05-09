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
/// \file  r_sky.h
/// \brief Sky rendering

#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief SKY, store the number for name.
#define SKYFLATNAME "F_SKY1"

/// \brief The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT 22

extern INT32 skytexture, skytexturemid, skytextureoffset;
extern fixed_t skyscale[MAXSPLITSCREENPLAYERS];

extern INT32 skyflatnum;
extern char levelskytexture[9];
extern char globallevelskytexture[9];

// call after skytexture is set to adapt for old/new skies
void R_SetupSkyDraw(void);

void R_SetSkyScale(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
