// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_vote.h
/// \brief Voting screen

#ifndef K_VOTE_H
#define K_VOTE_H

#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VOTE_MOD_ENCORE (0x01)

dboolean Y_PlayerIDCanVote(const uint8_t playerId);
void Y_SetPlayersVote(const uint8_t playerId, int8_t vote);

void Y_VoteDrawer(void);
void Y_VoteTicker(void);
void Y_StartVote(void);
void Y_EndVote(void);
void Y_SetupVoteFinish(int8_t pick, int8_t level, int8_t anger);
uint8_t Y_VoteContext(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // K_VOTE_H
