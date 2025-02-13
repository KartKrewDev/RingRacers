// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_anigif.h
/// \brief Animated GIF creation movie mode.

#ifndef __M_ANIGIF_H__
#define __M_ANIGIF_H__

#include "doomdef.h"
#include "command.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

#if NUMSCREENS > 2
#define HAVE_ANIGIF
#endif

#ifdef HAVE_ANIGIF
INT32 GIF_open(const char *filename);
void GIF_frame(void);
void GIF_frame_rgb24(INT32 width, INT32 height, const UINT8 *buffer);
INT32 GIF_close(void);
#endif

extern consvar_t cv_gif_optimize, cv_gif_downscale, cv_gif_dynamicdelay, cv_gif_localcolortable;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
