// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_vote.h
/// \brief Voting screen

#ifndef __K_VOTE_H__
#define __K_VOTE_H__

#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VOTE_NUM_LEVELS (4)
#define VOTE_NOT_PICKED (-1)

#define VOTE_MOD_ENCORE (0x01)

boolean Y_PlayerIDCanVote(const UINT8 playerId);
void Y_SetPlayersVote(const UINT8 playerId, SINT8 vote);

void Y_VoteDrawer(void);
void Y_VoteTicker(void);
void Y_StartVote(void);
void Y_EndVote(void);
void Y_SetupVoteFinish(SINT8 pick, SINT8 level, SINT8 anger);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_VOTE_H__
