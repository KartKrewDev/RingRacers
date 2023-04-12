// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Inuyasha" Walsh.
// Copyright (C) 1999-2018 by Sonic Team Junior.
// Copyright (C) 2023 by AJ "Tyron" Martinez
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
#define SERVERSTATSVER 1

struct serverplayer_t
{
	uint8_t public_key[PUBKEYLENGTH];
	time_t lastseen;
	UINT16 powerlevels[PWRLV_NUMTYPES];
};

void SV_SaveStats(void);

void SV_LoadStats(void);

serverplayer_t *SV_RetrieveStats(uint8_t *key);

void SV_UpdateStats(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
