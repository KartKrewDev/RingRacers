// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
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

void Y_VoteDrawer(void);
void Y_VoteTicker(void);
void Y_StartVote(void);
void Y_EndVote(void);
void Y_SetupVoteFinish(SINT8 pick, SINT8 level);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_VOTE_H__
