// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
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
