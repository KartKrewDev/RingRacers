// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_zvote.h
/// \brief Player callable mid-game vote

#ifndef __K_ZVOTE__
#define __K_ZVOTE__

#include "doomdef.h"
#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZVOTE_GUI_CONFIRM (TICRATE)
#define ZVOTE_GUI_SUCCESS (3 * TICRATE)
#define ZVOTE_GUI_SLIDE (TICRATE / 2)

typedef enum
{
	MVT_KICK,		// Kick another player in the server
	MVT_MUTE,       // Mute another player in the server (Voice Chat)
	MVT_RTV,		// Exit level early
	MVT_RUNITBACK,	// Restart level fresh
	MVT__MAX,		// Total number of vote types
} midVoteType_e;

extern const char *g_midVoteTypeNames[MVT__MAX];

struct midVoteGUI_t
{
	tic_t slide;				// Slide in when Z is first pressed.
	tic_t confirm;				// How long this player has held Z.
	boolean unpress;			// Z button needs unpressed to continue accepting input.
};

struct midVote_t
{
	boolean active;				// If true, a vote is currently running.

	player_t *caller;			// The player that called for this vote.
	player_t *victim;			// If non-NULL, then this vote targets a player (kicks), don't let them vote on it.
	boolean votes[MAXPLAYERS];	// Votes recieved from each player.

	midVoteType_e type;			// Type of vote that was called, see midVoteType_e.
	INT32 variable;				// Extra variable, unique purpose for each vote type.

	tic_t time;					// Time until the vote times out and fails.
	tic_t delay;				// Delay before another vote is allowed to be called.

	tic_t end;					// Ended animation, wait a second before activating callback.
	UINT8 endVotes;				// How many votes it got when the vote went through.
	UINT8 endRequired;			// How many votes were required when the vote went through.

	midVoteGUI_t gui[MAXSPLITSCREENPLAYERS]; // GUI / inputs struct
};

extern midVote_t g_midVote;

/*--------------------------------------------------
	boolean K_MidVoteTypeUsesVictim(midVoteType_e voteType)

		Specifies whenever or not a vote type is intended
		to specify a "victim", or a player that would be
		negatively affected by the vote.

	Input Arguments:-
		voteType - The vote type to check.

	Return:-
		true if it uses a victim, otherwise false.
--------------------------------------------------*/

boolean K_MidVoteTypeUsesVictim(midVoteType_e voteType);


/*--------------------------------------------------
	void K_SendCallMidVote(midVoteType_e voteType, INT32 voteVariable)

		Prepares and sends net packet for calling a midvote.

	Input Arguments:-
		voteType - The type of vote a local player is trying to call.
		variable - Extra arguments for the vote type.

	Return:-
		N/A
--------------------------------------------------*/

void K_SendCallMidVote(midVoteType_e voteType, INT32 voteVariable);


/*--------------------------------------------------
	void K_RegisterMidVoteCVars(void);

		Registers the console variables related to
		the Z-voting systems.
--------------------------------------------------*/

void K_RegisterMidVoteCVars(void);


/*--------------------------------------------------
	void K_ResetMidVote(void);

		Resets Z-voting variables to their default state.
--------------------------------------------------*/

void K_ResetMidVote(void);


/*--------------------------------------------------
	boolean K_AnyMidVotesAllowed(void);

		Determines if the server has enabled any types
		of Z-votes. If this is false, then any menu options
		for Z-voting should be disabled.

	Input Arguments:-
		N/A

	Return:-
		true if any vote types are enabled, otherwise false.
--------------------------------------------------*/

boolean K_AnyMidVotesAllowed(void);


/*--------------------------------------------------
	midVoteType_e K_GetNextCallableMidVote(midVoteType_e seed, boolean backwards)

		Gets the next enabled Z-vote type in the list.

	Input Arguments:-
		seed - position in the list to start with
		backwards - if true, traverses list in reverse order

	Return:-
		next Z-vote id if any vote types are enabled, otherwise MVT__MAX.
--------------------------------------------------*/

midVoteType_e K_GetNextAllowedMidVote(midVoteType_e seed, boolean backwards);


/*--------------------------------------------------
	boolean K_PlayerIDAllowedInMidVote(const UINT8 id);

		Determines if this player ID is allowed to
		vote or not.

	Input Arguments:-
		id - Player index to check.

	Return:-
		true if the player index can vote, otherwise false.
--------------------------------------------------*/

boolean K_PlayerIDAllowedInMidVote(const UINT8 id);


/*--------------------------------------------------
	UINT8 K_RequiredMidVotes(void);

		Calculates the number of votes needed for thr
		vote to go through (aka, the "quorum"), as
		per the server's settings. Usually at least
		2 players are required for the vote to go through.

	Input Arguments:-
		N/A

	Return:-
		Number of player votes needed before we should
		call K_MidVoteSuccess.
--------------------------------------------------*/

UINT8 K_RequiredMidVotes(void);


/*--------------------------------------------------
	boolean K_PlayerIDMidVoted(const UINT8 id);

		Determines if this player ID has voted for
		the current issue or not. Is mostly safety
		checks for g_midVote.votes[id], to force
		the player who called the vote to vote for it,
		the victim being affected to vote against,
		and invalid players not having a vote.

	Input Arguments:-
		id - Player index to check.

	Return:-
		true if the player index voted yes, otherwise false.
--------------------------------------------------*/

boolean K_PlayerIDMidVoted(const UINT8 id);


/*--------------------------------------------------
	UINT8 K_CountMidVotes(void);

		Counts the total number of votes in favor of
		the current issue.

	Input Arguments:-
		N/A

	Return:-
		Number of votes that agree.
--------------------------------------------------*/

UINT8 K_CountMidVotes(void);


/*--------------------------------------------------
	boolean K_MinimalCheckNewMidVote(midVoteType_e type)

		Returns if the variables given are a valid state for
		pause menu Z-vote flow.

	Input Arguments:-
		type - The type of vote they're trying to call.
--------------------------------------------------*/

boolean K_MinimalCheckNewMidVote(midVoteType_e type);

/*--------------------------------------------------
	boolean K_AllowNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim);

		Returns if the variables given are a valid state for
		K_InitNewMidVote. Creates console alerts if it's not.

	Input Arguments:-
		caller - The player that is trying to call for a vote.
		type - The type of vote they're trying to call.
		variable - Extra arguments for the vote type.
		victim - If this is a vote that negatively affects a
			player, the player being affected would go here.

	Return:-
		true if we can start a new vote, otherwise false.
--------------------------------------------------*/

boolean K_AllowNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim);


/*--------------------------------------------------
	void K_InitNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim);

		Tries to start a new Z-vote, or mid-game vote. This will
		handle everything needed to be initialized. Also calls
		K_AllowNewMidVote to prevent invalid states from happening.

	Input Arguments:-
		caller - The player that is trying to call for a vote.
		type - The type of vote they're trying to call.
		variable - Extra arguments for the vote type.
		victim - If this is a vote that negatively affects a
			player, the player being affected would go here.

	Return:-
		N/A
--------------------------------------------------*/

void K_InitNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim);


/*--------------------------------------------------
	void K_MidVoteFinalize(fixed_t delayMul);

		Ran when a vote is totally done, and we need to
		reset the struct and set the delay timer.
--------------------------------------------------*/

void K_MidVoteFinalize(fixed_t delayMul);


/*--------------------------------------------------
	void K_MidVoteSuccess(void);

		Ran whenever a vote meets the quorum, activates
		the effect that the current vote is intended to
		have.
--------------------------------------------------*/

void K_MidVoteSuccess(void);


/*--------------------------------------------------
	void K_MidVoteFailure(void);

		Ran when a vote times out without meeting the quorum.
		Doesn't do anything but set a very long delay before
		accepting another vote.
--------------------------------------------------*/

void K_MidVoteFailure(void);


/*--------------------------------------------------
	void K_TickMidVote(void);

		Run every game tick when in a server to process
		the vote in progress, if it exists.
--------------------------------------------------*/

void K_TickMidVote(void);


/*--------------------------------------------------
	void K_UpdateMidVotePatches(void);

		Caches the patches needed for drawing the
		HUD elements for Z-voting.
--------------------------------------------------*/

void K_UpdateMidVotePatches(void);


/*--------------------------------------------------
	const char *K_GetMidVoteLabel(midVoteType_e i)

	Input Arguments:-
		i - id in the list to retrieve label for

	Return:-
		label associated with that id, or a sensible default (not NULL)
--------------------------------------------------*/

const char *K_GetMidVoteLabel(midVoteType_e i);


/*--------------------------------------------------
	void K_DrawMidVote(void);

		Handles drawing the HUD elements for Z-voting.
--------------------------------------------------*/

void K_DrawMidVote(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_ZVOTE__
