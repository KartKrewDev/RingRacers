// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_random.h
/// \brief RNG for client effects and PRNG for game actions

#ifndef __M_RANDOM__
#define __M_RANDOM__

#include "doomtype.h"
#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUGRANDOM

typedef enum
{
	// Before release, cases of this RNG class should
	// be removed, only kept as the default for Lua.
	PR_UNDEFINED,

	// The rule for RNG classes:
	// Don't mix up gameplay & decorative RNG.

	// Decorative RNG is a lot less important
	// and can be lumped together. If it's used enough,
	// it might be nice to give it's own, though.

	// However each instance of RNG being used for
	// gameplay should be split up as much as possible.

	// Place new ones at the end for demo compatibility.

	PR_EXECUTOR, // Linedef executor
	PR_ACS, // ACS scripts

	PR_DECORATION, // Generic decoration
	PR_TERRAIN, // TERRAIN particles
	PR_BUBBLE, // Decorative air bubbles

	PR_RANDOMANIM, // FF_ANIMATE|FF_RANDOMANIM

	PR_PLAYERSTARTS, // Player starts
	PR_VOICES, // Player voice sounds
	PR_RANDOMSKIN, // Random skin select from Heavy Magician(?)

	PR_RANDOMAUDIENCE, // Audience randomisation

	PR_RULESCRAMBLE, // Rule scrambing events

	PR_MUSICSELECT, // Randomized music selection

	PR_ITEM_ROULETTE, // Item results. Overwritten constantly by K_FillItemRouletteData, not meant for typical use.
	PR_ITEM_RINGS, // Flung ring/bumper/player (on death)
	PR_ITEM_SHRINK, // Shrink gun offsets
	PR_ITEM_BUBBLE, // Item bubbles
	PR_ITEM_DEBRIS, // Item debris
	PR_ITEM_BOOST, // Boost

	PR_EXPLOSION, // Explosion VFX
	PR_SMOLDERING, // Smoldering particles
	PR_SPARKLE, // Endsign and/or Emerald

	PR_MOVINGTARGET, // Randomised moving targets
	PR_TRACKHAZARD, // Randomised track hazards

	PR_BATTLEUFO, // Battle UFO spawning

	PR_BOTS, // Bot spawning

	PR_AUTOROULETTE, // Item box accessibility

	PR_FUZZ, // Stability testing

	PR_FROSTTHROWERS,

	PROLDDEMO, // The number of RNG classes in versions that didn't write down how many RNG classes they had in their replays.

	PR_ITEM_SPAWNER = PROLDDEMO, // Battle mode item spawners
	PR_TEAMS, // Teamplay shuffling

	PR__PLACEHOLDER_DUMMY, // Whoops, screwed up PR_NUISANCE before, guess this lives here forever now

	PRNUMSYNCED,

	PR_NUISANCE, // Margin Boost HUD

	PR_INTERPHUDRANDOM = PRNUMSYNCED, // Interpolation-accomodating HUD randomisation

	PRNUMCLASS
} pr_class_t;

extern char rng_class_names[34][30];
// M_Random functions pull random numbers of various types that aren't network synced.
// P_Random functions pulls random bytes from a PRNG that is network synced.

// RNG functions
UINT32  M_Random(void);
fixed_t M_RandomFixed(void);
UINT8   M_RandomByte(void);
UINT32  M_RandomKey(UINT32 a);
INT32   M_RandomRange(INT32 a, INT32 b);

// PRNG functions
#ifdef DEBUGRANDOM
#define P_Random(c)            P_RandomD(__FILE__, __LINE__, c)
#define P_RandomFixed(c)       P_RandomFixedD(__FILE__, __LINE__, c)
#define P_RandomByte(c)        P_RandomByteD(__FILE__, __LINE__, c)
#define P_RandomKey(c, d)      P_RandomKeyD(__FILE__, __LINE__, c, d)
#define P_RandomRange(c, d, e) P_RandomRangeD(__FILE__, __LINE__, c, d, e)
UINT32  P_RandomD(const char *rfile, INT32 rline, pr_class_t pr_class);
fixed_t P_RandomFixedD(const char *rfile, INT32 rline, pr_class_t pr_class);
UINT8   P_RandomByteD(const char *rfile, INT32 rline, pr_class_t pr_class);
UINT32  P_RandomKeyD(const char *rfile, INT32 rline, pr_class_t pr_class, UINT32 a);
INT32   P_RandomRangeD(const char *rfile, INT32 rline, pr_class_t pr_class, INT32 a, INT32 b);
#else
UINT32  P_Random(pr_class_t pr_class);
fixed_t P_RandomFixed(pr_class_t pr_class);
UINT8   P_RandomByte(pr_class_t pr_class);
UINT32  P_RandomKey(pr_class_t pr_class, UINT32 a);
INT32   P_RandomRange(pr_class_t pr_class, INT32 a, INT32 b);
#endif

// Macros for other functions
#define M_SignedRandom()   ((INT32)M_RandomByte() + INT8_MIN)   // [-128, 127] signed byte, originally a
#define P_SignedRandom(pr) ((INT32)P_RandomByte(pr) + INT8_MIN) // function of its own, moved to a macro

#define M_RandomChance(p)     (M_RandomFixed() < p)   // given fixed point probability, p, between 0 (0%)
#define P_RandomChance(pr, p) (P_RandomFixed(pr) < p) // and FRACUNIT (100%), returns true p% of the time

// Debugging
UINT32 P_RandomPeek(pr_class_t pr_class);

// Working with the seed for PRNG
#ifdef DEBUGRANDOM
#define P_GetRandSeed(pr) P_GetRandSeedD(__FILE__, __LINE__, pr)
#define P_GetInitSeed(pr) P_GetInitSeedD(__FILE__, __LINE__, pr)
#define P_SetRandSeed(pr, s) P_SetRandSeedD(__FILE__, __LINE__, pr, s)
#define P_SetRandSeedNet(pr, i, s) P_SetRandSeedD(__FILE__, __LINE__, pr, i, s)
#define P_ResetInterpHudRandSeed(newframe) P_ResetInterpHudRandSeedD(__FILE__, __LINE__, newframe)
UINT32 P_GetRandSeedD(const char *rfile, INT32 rline, pr_class_t pr_class);
UINT32 P_GetInitSeedD(const char *rfile, INT32 rline, pr_class_t pr_class);
void P_SetRandSeedD(const char *rfile, INT32 rline, pr_class_t pr_class, UINT32 seed);
void P_SetRandSeedNetD(const char *rfile, INT32 rline, pr_class_t pr_class, UINT32 init, UINT32 seed);
void P_ResetInterpHudRandSeedD(const char *rfile, INT32 rline, boolean newframe);
#else
UINT32 P_GetRandSeed(pr_class_t pr_class);
UINT32 P_GetInitSeed(pr_class_t pr_class);
void P_SetRandSeed(pr_class_t pr_class, UINT32 seed);
void P_SetRandSeedNet(pr_class_t pr_class, UINT32 init, UINT32 seed);
void P_ResetInterpHudRandSeed(boolean newframe);
#endif

void P_ClearRandom(UINT32 seed);
UINT32 M_RandomizedSeed(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
