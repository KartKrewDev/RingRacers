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
/// \file  m_aatree.h
/// \brief AA trees code

#ifndef __M_AATREE__
#define __M_AATREE__

#include "doomtype.h"

#ifdef __cplusplus
extern "C" {
#endif

// Flags for AA trees.
#define AATREE_ZUSER	1		// Treat values as z_zone-allocated blocks and set their user fields

typedef void (*aatree_iter_t)(INT32 key, void *value);

aatree_t *M_AATreeAlloc(UINT32 flags);
void M_AATreeFree(aatree_t *aatree);
void M_AATreeSet(aatree_t *aatree, INT32 key, void* value);
void *M_AATreeGet(aatree_t *aatree, INT32 key);
void M_AATreeIterate(aatree_t *aatree, aatree_iter_t callback);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
