// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2022 by "Lat'"
// Copyright (C) 2018-2022 by Kart Krew
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
	FOLLOWERSTATE__MAX
} followerstate_t;

//
// We'll define these here because they're really just a mobj that'll follow some rules behind a player
//
typedef struct follower_s
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
} follower_t;

extern INT32 numfollowers;
extern follower_t followers[MAXSKINS];

#define MAXFOLLOWERCATEGORIES 32

typedef struct followercategory_s
{
	char name[SKINNAMESIZE+1];		// Name. This is used for the menus. We'll just follow the same rules as skins for this.
	char icon[8+1];			// Lump names are only 8 characters. (+1 for \0)
	UINT8 numincategory;
} followercategory_t;

extern INT32 numfollowercategories;
extern followercategory_t followercategories[MAXFOLLOWERCATEGORIES];

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
	UINT16 K_GetEffectiveFollowerColor(UINT16 followercolor, UINT16 playercolor)

		Updates a player's follower pointer, and does
		its positioning and animations.

	Input Arguments:-
		followercolor - The raw color setting for the follower
		playercolor - The player's associated colour, for reference

	Return:-
		The resultant skincolor enum for the follower
--------------------------------------------------*/

UINT16 K_GetEffectiveFollowerColor(UINT16 followercolor, UINT16 playercolor);


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


#endif // __K_FOLLOWER__
