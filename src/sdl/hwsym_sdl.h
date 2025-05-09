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
/// \file
/// \brief Tool for dynamic referencing of hardware rendering/3D sound functions

#ifndef __SDL_HWSYM_SDL_H__
#define __SDL_HWSYM_SDL_H__

#ifdef __cplusplus
extern "C" {
#endif

void *hwSym(const char *funcName,void *handle);

void *hwOpen(const char *hwfile);

void hwClose(void *handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __SDL_HWSYM_SDL_H__
