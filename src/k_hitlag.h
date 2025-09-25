// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hitlag.h
/// \brief Race Mode specific code.

#ifndef __K_HITLAG__
#define __K_HITLAG__

#include "doomtype.h"
#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXHITLAGTICS (30)
#define HITLAGDIV (20)	// define this so we arent using a magic number
#define HITLAGJITTERS (FRACUNIT / HITLAGDIV)
#define NUM_HITLAG_STATES (9)
#define NUM_HITLAG_SOUNDS (4)

void K_AddHitLagFromCollision(mobj_t *mo, INT32 tics);

/*--------------------------------------------------
	void K_AddHitLag(mobj_t *mo, INT32 tics, boolean fromDamage);

		Adds hitlag to an object.

	Input Arguments:-
		mo - Object to add hitlag to.
		tics - How much hitlag to add.
		fromDamage - Whenever or not this was a damage interaction.

	Return:-
		N/A
--------------------------------------------------*/

void K_AddHitLag(mobj_t *mo, INT32 tics, boolean fromDamage);


/*--------------------------------------------------
	void K_SetHitLagForObjects(mobj_t *victim, mobj_t *inflictor, mobj_t *source, INT32 tics, boolean fromDamage);

		Sets the hitlag for two objects, victim and inflictor,
		in a touch-related interaction (typically damage).

	Input Arguments:-
		victim - Object getting touched.
		inflictor - Object touching the victim. May be NULL.
		source - Object that inflictor came from. May be NULL or same as inflictor.
		tics - Minimum time for the hitlag to be. Can be increased if it is a damage interaction.
		fromDamage - Whenever or not this was a damage interaction.

	Return:-
		N/A
--------------------------------------------------*/

void K_SetHitLagForObjects(mobj_t *victim, mobj_t *inflictor, mobj_t *source, INT32 tics, boolean fromDamage);
void K_SpawnSingleHitLagSpark(mobj_t *parent, vector3_t *offset, fixed_t scale, UINT8 tics, UINT8 pause, skincolornum_t color);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_HITLAG__
