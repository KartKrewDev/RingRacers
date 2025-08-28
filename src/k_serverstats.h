// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_serverstats.h
/// \brief serverside stat tracking definitions

#ifndef __SERVERSTATS_H__
#define __SERVERSTATS_H__

#include "doomdef.h"		// MAXPLAYERNAME
#include "g_input.h"		// Input related stuff
#include "string.h"			// strcpy etc
#include "g_game.h"			// game CVs

#ifdef __cplusplus
extern "C" {
#endif

#define SERVERSTATSFILE "srvstats.dat"
#define SERVERSTATSHEADER "Doctor Robotnik's Ring Racers Server Stats"
#define SERVERSTATSVER 2

struct serverplayer_t
{
	uint8_t public_key[PUBKEYLENGTH];
	UINT32 lastseen;
	UINT16 powerlevels[PWRLV_NUMTYPES];
	UINT32 finishedrounds;

	UINT32 hash; // Not persisted! Used for early outs during key comparisons
};

void SV_SaveStats(void);

void SV_LoadStats(void);

serverplayer_t *SV_GetStatsByKey(uint8_t *key);
serverplayer_t *SV_GetStatsByPlayerIndex(UINT8 p);
serverplayer_t *SV_GetStats(player_t *player);

void SV_UpdateStats(void);

void SV_BumpMatchStats(void);

void SV_UpdateTempMutes(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
