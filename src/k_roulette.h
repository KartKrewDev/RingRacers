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
boolean K_ItemSingularity(kartitems_t item);

INT32 K_KartGetItemOdds(const player_t *player, itemroulette_t *const roulette, UINT8 pos, kartitems_t item);
void K_FillItemRouletteData(const player_t *player, itemroulette_t *const roulette);

void K_StartItemRoulette(player_t *const player);
void K_StartEggmanRoulette(player_t *const player);

#define ROULETTE_SPACING (36 << FRACBITS)
#define ROULETTE_SPACING_SPLITSCREEN (16 << FRACBITS)
fixed_t K_GetRouletteOffset(itemroulette_t *const roulette, fixed_t renderDelta);

void K_KartItemRoulette(player_t *const player, ticcmd_t *cmd);

#endif // __K_ROULETTE_H__
