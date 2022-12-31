// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Kart Krew
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
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
	INT32 K_KartGetItemOdds(const player_t *player, itemroulette_t *const roulette, UINT8 pos, kartitems_t item);

		Gets the frequency an item should show up in
		an item bracket, and adjusted for special
		factors (such as Frantic Items).

	Input Arguments:-
		player - The player we intend to give the item to later.
			Can be NULL for generic use.
		roulette - The roulette data that we intend to
			insert this item into.
		pos - The item bracket we are in.
		item - The item to give.

	Return:-
		The number of items we want to insert
		into the roulette.
--------------------------------------------------*/

INT32 K_KartGetItemOdds(const player_t *player, itemroulette_t *const roulette, UINT8 pos, kartitems_t item);


/*--------------------------------------------------
	void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette);

		Fills out the item roulette struct when it is
		initially created. This function needs to be
		HUD-safe for the item debugger, so the player
		cannot be modified at this stage.

	Input Arguments:-
		player - The player this roulette data is for.
			Can be NULL for generic use.
		roulette - The roulette data struct to fill out.

	Return:-
		N/A
--------------------------------------------------*/

void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette);


/*--------------------------------------------------
	void K_StartItemRoulette(player_t *const player);

		Starts the item roulette sequence for a player.
		This stage can only be used by gameplay, thus
		this handles gameplay modifications as well.

	Input Arguments:-
		player - The player to start the item roulette for.

	Return:-
		N/A
--------------------------------------------------*/

void K_StartItemRoulette(player_t *const player);


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
	fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta);

		Gets the Y offset, for use in the roulette HUD.
		A separate function since it is used both by the
		HUD itself, as well as when confirming an item.

	Input Arguments:-
		roulette - The roulette we are drawing for.
		renderDelta - Fractional tic delta, when used for HUD.

	Return:-
		The Y offset when drawing the item.
--------------------------------------------------*/

fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta);


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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_ROULETTE_H__
