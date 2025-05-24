// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bans.h
/// \brief ban system definitions

#ifndef __BANS_H__
#define __BANS_H__

#include "doomdef.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BANFILE "srvbans.json"

#define MAXBANUSERNAME 64
#define MAXBANREASON 256

struct banrecord_t
{
    uint8_t public_key[PUBKEYLENGTH];
    mysockaddr_t *address;
    UINT8 mask;

    time_t expires;

    char username[MAXBANUSERNAME+1];
    char reason[MAXBANREASON+1];

    UINT32 hash; // Not persisted! Used for early outs during key comparisons
    boolean deleted; // Not persisted! Deleted records are ignored and not written back to file.
    boolean matchesquery; // Not persisted! Used when filtering listbans/unban searches.
};

banrecord_t *SV_GetBanByKey(uint8_t *key);
banrecord_t *SV_GetBanByAddress(UINT8 node);

void SV_LoadBans(void);
void SV_SaveBans(void);
void SV_BanPlayer(int pnum, time_t duration, char *reason);
boolean SV_BanIP(const char *address, UINT8 mask, uint8_t *public_key, time_t expires, const char *username, const char *reason);
void SV_Ban(mysockaddr_t address, UINT8 mask, uint8_t *public_key, time_t expires, const char *username, const char *reason);

void Command_Listbans(void);
void Command_Unban(void);
void Command_BanIP(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
