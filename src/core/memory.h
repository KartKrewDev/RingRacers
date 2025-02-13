// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_CORE_MEMORY_H__
#define __SRB2_CORE_MEMORY_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cpluspplus

/// @brief Allocate a block of memory with a lifespan of the current main-thread frame.
/// This function is NOT thread-safe, but the allocated memory may be used across threads.
/// @return a pointer to a block of memory aligned with libc malloc alignment, or null if allocation fails
void* Z_Frame_Alloc(size_t size);

/// @brief Resets per-frame memory. Not thread safe.
void Z_Frame_Reset(void);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __SRB2_CORE_MEMORY_H__
