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

void Y_IntermissionDrawer(void);
void Y_Ticker(void);

void Y_StartIntermission(void);
void Y_EndIntermission(void);

void Y_DetermineIntermissionType(void);

void Y_VoteDrawer(void);
void Y_VoteTicker(void);
void Y_StartVote(void);
void Y_EndVote(void);
void Y_SetupVoteFinish(SINT8 pick, SINT8 level);

typedef enum
{
	int_none,
	int_race,		// Race
	int_battle,		// Battle (score-based)
	int_battletime,	// Battle (time-based)
} intertype_t;

extern intertype_t intertype;
extern intertype_t intermissiontypes[NUMGAMETYPES];
