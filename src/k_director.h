// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_director.h
/// \brief SRB2kart automatic spectator camera.

#ifndef K_DIRECTOR_H
#define K_DIRECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

void K_InitDirector(void);
void K_UpdateDirector(void);
void K_DrawDirectorDebugger(void);
void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source);
void K_ToggleDirector(UINT8 viewnum, boolean active);
boolean K_DirectorIsEnabled(UINT8 viewnum);
boolean K_DirectorIsAvailable(UINT8 viewnum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // K_DIRECTOR_H
