// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __G_PARTY_H__
#define __G_PARTY_H__

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
void G_DestroyParty(UINT8 player);

// Adds player to their local party.
void G_BuildLocalSplitscreenParty(UINT8 player);

// Join guest's entire local party to the host. All checks are
// performed, so this is a no-op if the parties are already
// joined, or if either party is too big for the other, etc.
//
// Resets viewports for all players involved.
void G_JoinParty(UINT8 host, UINT8 guest);

// Removes guest from an online party and restores their
// initial local party.
void G_LeaveParty(UINT8 guest);

// Size of the player's initial local party.
UINT8 G_LocalSplitscreenPartySize(UINT8 player);

// Ultimate size of this player's party. Includes any joined
// parties, else the same as G_LocalSplitscreenPartySize.
UINT8 G_PartySize(UINT8 player);

// True if this player is a member of the consoleplayer's
// party.
boolean G_IsPartyLocal(UINT8 player);

// Returns the player slot present at a certain position
// within this player's party. Do not call this function with
// an index beyond G_PartySize() - 1.
UINT8 G_PartyMember(UINT8 player, UINT8 index);

// C array access to the same data as G_PartyMember.
const UINT8 *G_PartyArray(UINT8 player);

// Suitable index to G_PartyMember and G_PartyArray.
UINT8 G_PartyPosition(UINT8 player);

//
UINT8 G_LocalSplitscreenPartyPosition(UINT8 player);

//
UINT8 G_LocalSplitscreenPartyMember(UINT8 player, UINT8 index);

//
//        Globals
//

// Whether this player has been invited to join anyone's party
// and who invited them. -1 if no invitation.
extern INT32 splitscreen_invitations[MAXPLAYERS];

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __G_PARTY_H__
