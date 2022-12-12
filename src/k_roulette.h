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

boolean K_ItemEnabled(SINT8 item);

fixed_t K_ItemOddsScale(UINT8 playerCount);
UINT32 K_ScaleItemDistance(UINT32 distance, UINT8 numPlayers);
UINT32 K_GetItemRouletteDistance(player_t *const player, UINT8 pingame);

INT32 K_KartGetItemOdds(UINT8 pos, SINT8 item, UINT32 ourDist, boolean bot, boolean rival);
UINT8 K_FindUseodds(player_t *const player, UINT32 playerDist);

boolean K_ForcedSPB(player_t *const player);

void K_StartItemRoulette(player_t *const player, itemroulette_t *const roulette);
void K_StartEggmanRoulette(player_t *const player);

#define ROULETTE_SPACING (36 << FRACBITS)
fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta);

void K_KartItemRoulette(player_t *const player, ticcmd_t *cmd);

#endif // __K_ROULETTE_H__
