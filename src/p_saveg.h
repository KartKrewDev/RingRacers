// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_saveg.h
/// \brief Savegame I/O, archiving, persistence

#ifndef P_SAVEG_H
#define P_SAVEG_H

#ifdef __cplusplus
extern "C" {
#endif

// 1024 bytes is plenty for a savegame
// ...but we'll be accomodating of a heavily replaced Round Queue.
#define SAVEGAMESIZE (2048)

// For netgames
#define NETSAVEGAMESIZE (768*1024)

// Persistent storage/archiving.
// These are the load / save game routines.

// Local Play
void P_SaveGame(savebuffer_t *save);
dboolean P_LoadGame(savebuffer_t *save);
void P_GetBackupCupData(savebuffer_t *save);

// Online
void P_SaveNetGame(savebuffer_t *save, dboolean resending);
dboolean P_LoadNetGame(savebuffer_t *save, dboolean reloading);

mobj_t *P_FindNewPosition(uint32_t oldposition);

struct savedata_bot_s
{
	dboolean valid;
	uint16_t skin;
	uint8_t difficulty;
	dboolean rival;
	dboolean foe;
	uint32_t score;
};

struct savedata_t
{
	uint32_t score;
	int8_t lives;
	uint16_t totalring;

	uint16_t skin;
	uint16_t skincolor;
	int32_t followerskin;
	uint16_t followercolor;

	struct savedata_bot_s bots[MAXPLAYERS];
};

extern savedata_t savedata;

struct savedata_cup_t
{
	cupheader_t *cup;
	uint8_t difficulty;
	dboolean encore;
};

extern savedata_cup_t cupsavedata;

struct savebuffer_t
{
	uint8_t *buffer;
	uint8_t *p;
	uint8_t *end;
	size_t size;
};

dboolean P_SaveBufferZAlloc(savebuffer_t *save, size_t alloc_size, int32_t tag, void *user);
#define P_SaveBufferAlloc(a,b) P_SaveBufferZAlloc(a, b, PU_STATIC, NULL)
dboolean P_SaveBufferFromExisting(savebuffer_t *save, uint8_t *existing_buffer, size_t existing_size);
dboolean P_SaveBufferFromLump(savebuffer_t *save, lumpnum_t lump);
dboolean P_SaveBufferFromFile(savebuffer_t *save, char const *name);
void P_SaveBufferFree(savebuffer_t *save);
size_t P_SaveBufferRemaining(const savebuffer_t *save);

dboolean TypeIsNetSynced(mobjtype_t type);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
