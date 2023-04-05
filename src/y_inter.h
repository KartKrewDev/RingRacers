// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  y_inter.h
/// \brief Tally screens, or "Intermissions" as they were formally called in Doom

#ifndef __Y_INTER_H__
#define __Y_INTER_H__

#ifdef __cplusplus
extern "C" {
#endif

void Y_IntermissionDrawer(void);
void Y_Ticker(void);

void Y_StartIntermission(void);
void Y_EndIntermission(void);

void Y_DetermineIntermissionType(void);

typedef enum
{
	int_none,
	int_time,				// Always time
	int_score,				// Always score
	int_scoreortimeattack,	// Score unless 1P
} intertype_t;

extern intertype_t intertype;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __Y_INTER_H__
