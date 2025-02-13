// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __K_POWERUP__
#define __K_POWERUP__

#include "doomtype.h"
#include "d_player.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BATTLE_POWERUP_VFX_TIME (40)

tic_t K_PowerUpRemaining(const player_t *player, kartitems_t powerup);
UINT32 K_AnyPowerUpRemaining(const player_t *player); // returns POWERUP_BIT mask
void K_GivePowerUp(player_t *player, kartitems_t powerup, tic_t timer);
void K_DropPowerUps(player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_POWERUP__
