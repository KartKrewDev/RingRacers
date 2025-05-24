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
/// \file  d_think.h
/// \brief MapObj data. Map Objects or mobjs are actors, entities,
///        thinker, take-your-pick
///
///        anything that moves, acts, or suffers state changes of more or less violent nature.

#ifndef __D_THINK__
#define __D_THINK__

#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Experimental stuff.
// To compile this as "ANSI C with classes" we will need to handle the various
//  action functions cleanly.
//
typedef void (*actionf_v)();
typedef void (*actionf_p1)(void *);

typedef union
{
	actionf_v acv;
	actionf_p1 acp1;
} actionf_t;

typedef enum
{
	/// The allocation is standard e.g. Z_Malloc
	TAT_MALLOC,

	/// The allocation is in the pool allocator (e.g. Z_LevelPoolCalloc)
	TAT_LEVELPOOL
} thinker_alloc_type_e;

// Historically, "think_t" is yet another function pointer to a routine
// to handle an actor.
typedef actionf_t think_t;

// Doubly linked list of actors.
struct thinker_t
{
	thinker_t *prev;
	thinker_t *next;
	think_t function;

	// killough 11/98: count of how many other objects reference
	// this one using pointers. Used for garbage collection.
	INT32 references;
	INT32 alloctype;
	size_t size;

#ifdef PARANOIA
	INT32 debug_mobjtype;
	tic_t debug_time;
#endif
};

#ifdef __cplusplus
} // extern "C"
#endif

#endif
