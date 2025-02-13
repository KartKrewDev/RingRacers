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
/// \brief SDL specific part of the OpenGL API for SRB2

#ifndef __SDL_OGL_SDL_H__
#define __SDL_OGL_SDL_H__

#include <SDL.h>

#include "../v_video.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *GLUhandle;

boolean OglSdlSurface(INT32 w, INT32 h);

void OglSdlFinishUpdate(boolean vidwait);

extern SDL_GLContext sdlglcontext;
extern SDL_GLContext g_legacy_gl_context;
extern Uint16      realwidth;
extern Uint16      realheight;

#ifdef _CREATE_DLL_
EXPORT void HWRAPI( OglSdlSetPalette ) (RGBA_t *palette);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SDL_OGL_SDL_H__
