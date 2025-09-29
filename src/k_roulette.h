// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_roulette.h
/// \brief Item roulette code.

#ifndef __K_ROULETTE_H__
#define __K_ROULETTE_H__

#include "doomtype.h"
#include "d_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROULETTE_SPACING (36 << FRACBITS)
#define ROULETTE_SPACING_SPLITSCREEN (16 << FRACBITS)

#define SLOT_SPACING (40 << FRACBITS)
#define SLOT_SPACING_SPLITSCREEN (22 << FRACBITS)

/*--------------------------------------------------
	boolean K_ItemEnabled(kartitems_t item);

		Determines whenever or not an item should
		be enabled. Accounts for situations where
		rules should not be able to be changed.

	Input Arguments:-
		item - The item to check.

	Return:-
		true if the item is enabled, otherwise false.
--------------------------------------------------*/

boolean K_ItemEnabled(kartitems_t item);


/*--------------------------------------------------
	boolean K_ItemSingularity(kartitems_t item);

		Determines whenever or not this item should
		be using special cases to prevent more than
		one existing at a time.

	Input Arguments:-
		item - The item to check.

	Return:-
		true to use the special rules, otherwise false.
--------------------------------------------------*/

boolean K_ItemSingularity(kartitems_t item);


/*--------------------------------------------------
	botItemPriority_e K_GetBotItemPriority(kartitems_t result)

		Returns an item's priority value, which
		bots use to determine what kind of item they
		want when the roulette is started.

	Input Arguments:-
		result - The item result type to check.

	Return:-
		The item's priority type.
--------------------------------------------------*/

botItemPriority_e K_GetBotItemPriority(kartitems_t result);

/*--------------------------------------------------
	fixed_t K_ItemOddsScale(UINT8 playerCount)

		A multiplier for odds and distances to scale
		them with the player count.

	Input Arguments:-
		playerCount - Number of players in the game.

	Return:-
		Fixed point number, to multiply odds or
		distances by.
--------------------------------------------------*/

fixed_t K_ItemOddsScale(UINT8 playerCount);

/*--------------------------------------------------
	UINT32 K_ScaleItemDistance(UINT32 distance, UINT8 numPlayers)

		Adjust item distance for lobby-size scaling
		as well as Frantic Items.

	Input Arguments:-
		distance - Original distance.
		numPlayers - Number of players in the game.

	Return:-
		New distance after scaling.
--------------------------------------------------*/

UINT32 K_ScaleItemDistance(INT32 distance, UINT8 numPlayers);

/*--------------------------------------------------
	UINT32 K_UndoMapScaling(UINT32 distance)

		Takes a raw map distance and adjusts it to
		be in x1 scale.

	Input Arguments:-
		distance - Original distance.

	Return:-
		Distance unscaled by mapobjectscale.
--------------------------------------------------*/

UINT32 K_UndoMapScaling(UINT32 distance);

/*--------------------------------------------------
	void K_PushToRouletteItemList(itemroulette_t *const roulette, INT32 item)

		Pushes a new item to the end of the item
		roulette's item list. Also accepts slot machine
		values instead of items.

	Input Arguments:-
		roulette - The item roulette data to modify.
		item - The item / slot machine index to push to the list.

	Return:-
		N/A
--------------------------------------------------*/

void K_InitRoulette(itemroulette_t *const roulette);
/*--------------------------------------------------
	static void K_InitRoulette(itemroulette_t *const roulette)

		Initializes the data for a new item roulette.

	Input Arguments:-
		roulette - The item roulette data to initialize.

	Return:-
		N/A
--------------------------------------------------*/

void K_PushToRouletteItemList(itemroulette_t *const roulette, INT32 item);

/*--------------------------------------------------
	void K_AddItemToReel(const player_t *player, itemroulette_t *const roulette, kartitems_t item)

		Adds an item to a player's item reel. Unlike
		pushing directly with K_PushToRouletteItemList,
		this function handles special behaviors (like
		padding with extra Super Rings).

	Input Arguments:-
		player - The player to add to the item roulette.
			This is valid to be NULL.
		roulette - The player's item roulette data.
		item - The item to push to the list.

	Return:-
		N/A
--------------------------------------------------*/

void K_AddItemToReel(const player_t *player, itemroulette_t *const roulette, kartitems_t item);

/*--------------------------------------------------
	void K_CalculateRouletteSpeed(itemroulette_t *const roulette)

		Determines the speed for the item roulette,
		adjusted for progress in the race and front
		running.

	Input Arguments:-
		roulette - The item roulette data to modify.

	Return:-
		N/A
--------------------------------------------------*/

void K_CalculateRouletteSpeed(itemroulette_t *const roulette);

/*--------------------------------------------------
	INT32 K_KartGetBattleOdds(const player_t *player, itemroulette_t *const roulette, UINT8 pos, kartitems_t item);

		Gets legacy item priority.
		Currently used only for Battle monitors/spawners.

	Input Arguments:-
		player - The player we intend to give the item to later.
			Can be NULL for generic use.
		pos - The item bracket we are in.
		item - The item to give.

	Return:-
		The number of items we want to insert
		into the roulette.
--------------------------------------------------*/

INT32 K_KartGetBattleOdds(const player_t *player, UINT8 pos, kartitems_t item);

/*--------------------------------------------------
	void K_FillItemRoulette(player_t *player, itemroulette_t *const roulette, boolean ringbox);

		Entry point for roulette builder.
		Includes Lua hooks.

	Input Arguments:-
		player - The player this roulette data is for.
			Can be NULL for generic use.
		roulette - The roulette data struct to fill out.
		ringbox - Is this roulette fill triggered by a just-respawned Ring Box?

	Return:-
		N/A
--------------------------------------------------*/

void K_FillItemRoulette(player_t *player, itemroulette_t *const roulette, boolean ringbox);


/*--------------------------------------------------
	void K_FillItemRouletteData(player_t *player, itemroulette_t *const roulette, boolean ringbox, boolean dryrun);

		Fills out the item roulette struct when it is
		initially created. This function needs to be
		HUD-safe for the item debugger, so the player
		cannot be modified at this stage.

	Input Arguments:-
		player - The player this roulette data is for.
			Can be NULL for generic use.
		roulette - The roulette data struct to fill out.
		ringbox - Is this roulette fill triggered by a just-respawned Ring Box?
		dryrun - Are we calling this from the distribution debugger? Don't call RNG or write roulette data!

	Return:-
		N/A
--------------------------------------------------*/

void K_FillItemRouletteData(player_t *player, itemroulette_t *const roulette, boolean ringbox, boolean dryrun);


/*--------------------------------------------------
	void K_StartItemRoulette(player_t *const player, boolean ringbox);

		Starts the item roulette sequence for a player.
		This stage can only be used by gameplay, thus
		this handles gameplay modifications as well.

	Input Arguments:-
		player - The player to start the item roulette for.
		ringbox - Is this roulette being started from a just-respawned Ring Box?

	Return:-
		N/A
--------------------------------------------------*/

void K_StartItemRoulette(player_t *const player, boolean ringbox);


/*--------------------------------------------------
	void K_StartEggmanRoulette(player_t *const player);

		Starts the Eggman Mark roulette sequence for
		a player. Looks identical to a regular item
		roulette, but gives you the Eggman explosion
		countdown instead when confirming it.

	Input Arguments:-
		player - The player to start the Eggman roulette for.

	Return:-
		N/A
--------------------------------------------------*/

void K_StartEggmanRoulette(player_t *const player);

/*--------------------------------------------------
	void K_StopRoulette(itemroulette_t *const roulette);

		Resets the roulette back to a default state.
		Stops item roulette, Eggman and Ringbox.

	Input Arguments:-
		roulette - The roulette to stop.

	Return:-
		N/A
--------------------------------------------------*/

void K_StopRoulette(itemroulette_t *const roulette);


/*--------------------------------------------------
	fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge);

		Gets the Y offset, for use in the roulette HUD.
		A separate function since it is used both by the
		HUD itself, as well as when confirming an item.

	Input Arguments:-
		roulette - The roulette we are drawing for.
		renderDelta - Fractional tic delta, when used for HUD.
		fudge - Input latency fudge factor, when used for gameplay.

	Return:-
		The Y offset when drawing the item.
--------------------------------------------------*/

fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge);


/*--------------------------------------------------
	fixed_t K_GetSlotOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge);

		Gets the Y offset, for use in the slot HUD.
		A separate function since it is used both by the
		HUD itself, as well as when confirming an item.

	Input Arguments:-
		roulette - The roulette we are drawing for.
		renderDelta - Fractional tic delta, when used for HUD.
		fudge - Input latency fudge factor, when used for gameplay.

	Return:-
		The Y offset when drawing the item.
--------------------------------------------------*/

fixed_t K_GetSlotOffset(itemroulette_t *const roulette, fixed_t renderDelta, UINT8 fudge);


/*--------------------------------------------------
	void K_KartItemRoulette(player_t *const player, ticcmd_t *cmd);

		Handles ticking a player's item roulette,
		and player input for stopping it.

	Input Arguments:-
		player - The player to run the item roulette for.
		cmd - The player's controls.

	Return:-
		N/A
--------------------------------------------------*/

void K_KartItemRoulette(player_t *const player, ticcmd_t *cmd);

void K_KartGetItemResult(player_t *const player, kartitems_t getitem);

/*--------------------------------------------------
	static UINT32 K_GetItemRouletteDistance(const player_t *player, UINT8 numPlayers)

		Gets a player's distance used for the item
		roulette, including all scaling factors.

	Input Arguments:-
		player - The player to get the distance of.
		numPlayers - Number of players in the game.

	Return:-
		The player's finalized item distance.
--------------------------------------------------*/

UINT32 K_GetItemRouletteDistance(const player_t *player, UINT8 numPlayers);

/*--------------------------------------------------
	boolean K_DenyShieldOdds(kartitems_t item)

		Checks if this type of shield already exists in
		another player's inventory.

	Input Arguments:-
		item - The item type of the shield.

	Return:-
		Whether this item is a shield and may not be awarded
		at this time.
--------------------------------------------------*/

boolean K_DenyShieldOdds(kartitems_t item);

/*--------------------------------------------------
	boolean K_ForcedSPB(const player_t *player, itemroulette_t *const roulette)

		Determines special conditions where we want
		to forcefully give the player an SPB.

	Input Arguments:-
		player - The player the roulette is for.
		roulette - The item roulette data.

	Return:-
		true if we want to give the player a forced SPB,
		otherwise false.
--------------------------------------------------*/

boolean K_ForcedSPB(const player_t *player, itemroulette_t *const roulette);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_ROULETTE_H__
