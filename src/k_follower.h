// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'".
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_follower.h
/// \brief Code relating to the follower system

#ifndef __K_FOLLOWER__
#define __K_FOLLOWER__

#include "doomdef.h"
#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

// The important collorary to "Hornmod is universally hated in dev" is
// the simple phrase "this is why" -- 1024 is obscene yet will fill up.
// By the way, this comment will grow stronger and stronger every time
// somebody comes here to double it, so I encourage you to leave a new
// (dated) comment every time you do so. ~toast 280623
#define MAXFOLLOWERS 1024

#define FOLLOWERCOLOR_MATCH UINT16_MAX
#define FOLLOWERCOLOR_OPPOSITE (UINT16_MAX-1)

extern CV_PossibleValue_t Followercolor_cons_t[]; // follower colours table, not a duplicate because of the "Match" option.

typedef enum
{
	FOLLOWERMODE_FLOAT,		// Default behavior, floats in the position you set it to.
	FOLLOWERMODE_GROUND,	// Snaps to the ground & rotates with slopes.
	FOLLOWERMODE__MAX
} followermode_t;

typedef enum
{
	FOLLOWERSTATE_RESET, // Set to this to reset the state entirely.
	FOLLOWERSTATE_IDLE,
	FOLLOWERSTATE_FOLLOW,
	FOLLOWERSTATE_HURT,
	FOLLOWERSTATE_WIN,
	FOLLOWERSTATE_LOSE,
	FOLLOWERSTATE_HITCONFIRM, // Uses movecount as a timer for how long to play this state.
	FOLLOWERSTATE_RING, // Uses cvmem as a timer for how long to play this state.
	FOLLOWERSTATE__MAX
} followerstate_t;

//
// We'll define these here because they're really just a mobj that'll follow some rules behind a player
//
struct follower_t
{
	char name[SKINNAMESIZE+1];	// Skin Name. This is what to refer to when asking the commands anything..
	char icon[8+1];			// Lump names are only 8 characters. (+1 for \0)

	UINT8 category;			// Category

	skincolornum_t defaultcolor;	// default color for menus.
	followermode_t mode;			// Follower behavior modifier.

	fixed_t scale;			// Scale relative to the player's.
	fixed_t bubblescale;	// Bubble scale relative to the player scale. If not set, no bubble will spawn (default)

	// some position shenanigans:
	angle_t atangle;		// angle the object will be at around the player. The object itself will always face the same direction as the player.
	fixed_t dist;			// distance relative to the player. (In a circle)
	fixed_t height;			// height of the follower, this is mostly important for Z flipping.
	fixed_t zoffs;			// Z offset relative to the player's height. Cannot be negative.

	// movement options

	fixed_t horzlag;		// Lag for X/Y displacement. Default is 3. Must be > 0 because we divide by this number.
	fixed_t vertlag;		// Z displacement lag. Default is 6. Must be > 0 because we divide by this number.
	fixed_t anglelag;		// Angle rotation lag. Default is 8. Must be > 0 because we divide by this number.

	fixed_t bobamp;			// Bob amplitude. Default is 4.
	tic_t bobspeed;			// Arbitrary modifier for bobbing speed. Default is TICRATE*2 (70)

	// from there on out, everything is STATES to allow customization
	// these are only set once when the action is performed and are then free to animate however they want.

	statenum_t idlestate;		// state when the player is at a standstill
	statenum_t followstate;		// state when the player is moving
	statenum_t hurtstate;		// state when the player is being hurt
	statenum_t winstate;		// state when the player has won
	statenum_t losestate;		// state when the player has lost
	statenum_t hitconfirmstate;	// state for hit confirm
	tic_t hitconfirmtime;		// time to keep the above playing for
	statenum_t ringstate;		// state for giving an auto-ring
	tic_t ringtime;				// time to keep the above playing for

	sfxenum_t hornsound;		// Press (B) to announce you are pressing (B)
};

extern INT32 numfollowers;
extern follower_t followers[MAXFOLLOWERS];

#define MAXFOLLOWERCATEGORIES 64

struct followercategory_t
{
	char name[SKINNAMESIZE+1];		// Name. This is used for the menus. We'll just follow the same rules as skins for this.
	char icon[8+1];			// Lump names are only 8 characters. (+1 for \0)
	UINT8 numincategory;
};

extern INT32 numfollowercategories;
extern followercategory_t followercategories[MAXFOLLOWERCATEGORIES];

extern boolean horngoner;

/*--------------------------------------------------
	INT32 K_FollowerAvailable(const char *name)

		Check if a follower with the specified name
		exists or not.

	Input Arguments:-
		name - The skin name of the follower to check for.

	Return:-
		The follower numerical ID of the follower,
		or -1 if it doesn't exist.
--------------------------------------------------*/

INT32 K_FollowerAvailable(const char *name);


/*--------------------------------------------------
	boolean K_FollowerUsable(INT32 followernum);

		Check if a follower is usable or not.

	Input Arguments:-
		skinnum - The follower's skin ID

	Return:-
		true if it was a valid follower,
		otherwise false.
--------------------------------------------------*/

boolean K_FollowerUsable(INT32 skinnum);


/*--------------------------------------------------
	boolean K_SetFollowerByName(INT32 playernum, const char *skinname)

		Updates a player's follower type via a named value.
		Calls "K_SetFollowerByNum" internally.

	Input Arguments:-
		playernum - The player ID to update
		skinname - The follower's skin name

	Return:-
		true if it was a valid name for a follower,
		otherwise false.
--------------------------------------------------*/

boolean K_SetFollowerByName(INT32 playernum, const char *skinname);


/*--------------------------------------------------
	void K_SetFollowerByNum(INT32 playernum, INT32 skinnum)

		Updates a player's follower type via a numerical ID.

	Input Arguments:-
		playernum - The player ID to update.
		skinnum - The follower's skin ID

	Return:-
		None
--------------------------------------------------*/

void K_SetFollowerByNum(INT32 playernum, INT32 skinnum);


/*--------------------------------------------------
	UINT16 K_GetEffectiveFollowerColor(UINT16 followercolor, follower_t *follower, UINT16 playercolor, skin_t *playerskin)

		Updates a player's follower pointer, and does
		its positioning and animations.

	Input Arguments:-
		followercolor - The raw color setting for the follower
		follower - Follower struct to retrieve default color from. Can be NULL
		playercolor - The player's associated colour, for reference
		playerskin - Skin struct to retrieve default color from. Can be NULL

	Return:-
		The resultant skincolor enum for the follower
--------------------------------------------------*/

UINT16 K_GetEffectiveFollowerColor(UINT16 followercolor, follower_t *follower, UINT16 playercolor, skin_t *playerskin);


/*--------------------------------------------------
	void K_HandleFollower(player_t *player)

		Updates a player's follower pointer, and does
		its positioning and animations.

	Input Arguments:-
		player - The player who we want to update the follower of.

	Return:-
		None
--------------------------------------------------*/

void K_HandleFollower(player_t *player);

/*--------------------------------------------------
	void K_RemoveFollower(player_t *player)

		Removes Follower object

	Input Arguments:-
		player - The player who we want to remove the follower of.

	Return:-
		None
--------------------------------------------------*/

void K_RemoveFollower(player_t *player);

/*--------------------------------------------------
	void K_FollowerHornTaunt(player_t *taunter, player_t *victim, boolean mysticmelodyspecial)

		Plays horn and spawns object (MOSTLY non-netsynced)

	Input Arguments:-
		taunter - Source player with a follower
		victim - Player that hears and sees the honk
		mysticmelodyspecial - Special Mystic Melody behaviour

	Return:-
		None
--------------------------------------------------*/

void K_FollowerHornTaunt(player_t *taunter, player_t *victim, boolean mysticmelodyspecial);

/*--------------------------------------------------
	INT32 K_GetEffectiveFollowerSkin(const player_t *player)

		Returns the player's follower, set by profile or as
		a fallback.

	Input Arguments:-
		player - The player.

	Return:-
		The resultant skin id for the follower, or -1 for None
--------------------------------------------------*/

INT32 K_GetEffectiveFollowerSkin(const player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_FOLLOWER__
