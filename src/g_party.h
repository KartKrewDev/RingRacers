// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef G_PARTY_H
#define G_PARTY_H

#include "doomdef.h" // MAXPLAYERS

#ifdef __cplusplus
extern "C" {
#endif

//
//        Functions
//

// Frees all party resources.
void G_ObliterateParties(void);

// Wipes all party data for this player slot.
void G_DestroyParty(uint8_t player);

// Adds player to their local party.
void G_BuildLocalSplitscreenParty(uint8_t player);

// Join guest's entire local party to the host. All checks are
// performed, so this is a no-op if the parties are already
// joined, or if either party is too big for the other, etc.
//
// Resets viewports for all players involved.
void G_JoinParty(uint8_t host, uint8_t guest);

// Removes guest from an online party and restores their
// initial local party.
void G_LeaveParty(uint8_t guest);

// Size of the player's initial local party.
uint8_t G_LocalSplitscreenPartySize(uint8_t player);

// Ultimate size of this player's party. Includes any joined
// parties, else the same as G_LocalSplitscreenPartySize.
uint8_t G_PartySize(uint8_t player);

// True if this player is a member of the consoleplayer's
// party.
dboolean G_IsPartyLocal(uint8_t player);

// Returns the player slot present at a certain position
// within this player's party. Do not call this function with
// an index beyond G_PartySize() - 1.
uint8_t G_PartyMember(uint8_t player, uint8_t index);

// C array access to the same data as G_PartyMember.
const uint8_t *G_PartyArray(uint8_t player);

// Suitable index to G_PartyMember and G_PartyArray.
uint8_t G_PartyPosition(uint8_t player);

//
uint8_t G_LocalSplitscreenPartyPosition(uint8_t player);

//
uint8_t G_LocalSplitscreenPartyMember(uint8_t player, uint8_t index);

//
//        Globals
//

// Whether this player has been invited to join anyone's party
// and who invited them. -1 if no invitation.
extern int32_t splitscreen_invitations[MAXPLAYERS];

#ifdef __cplusplus
} // extern "C"
#endif

#endif // G_PARTY_H
