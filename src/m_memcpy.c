// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_memcpy.c
/// \brief (formerly) X86 optimized implementations of M_Memcpy

#include "doomdef.h"
#include "m_misc.h"

void *M_Memcpy(void* dest, const void* src, size_t n)
{
	memcpy(dest, src, n);
	return dest;
}
