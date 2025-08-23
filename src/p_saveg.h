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

#ifndef __P_SAVEG__
#define __P_SAVEG__

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
boolean P_LoadGame(savebuffer_t *save);
void P_GetBackupCupData(savebuffer_t *save);

// Online
void P_SaveNetGame(savebuffer_t *save, boolean resending);
boolean P_LoadNetGame(savebuffer_t *save, boolean reloading);

mobj_t *P_FindNewPosition(UINT32 oldposition);

struct savedata_bot_s
{
	boolean valid;
	UINT16 skin;
	UINT8 difficulty;
	boolean rival;
	boolean foe;
	UINT32 score;
};

struct savedata_t
{
	UINT32 score;
	SINT8 lives;
	UINT16 totalring;

	UINT16 skin;
	UINT16 skincolor;
	INT32 followerskin;
	UINT16 followercolor;

	struct savedata_bot_s bots[MAXPLAYERS];
};

extern savedata_t savedata;

struct savedata_cup_t
{
	cupheader_t *cup;
	UINT8 difficulty;
	boolean encore;
};

extern savedata_cup_t cupsavedata;

struct savebuffer_t
{
	UINT8 *buffer;
	UINT8 *p;
	UINT8 *end;
	size_t size;
};

boolean P_SaveBufferZAlloc(savebuffer_t *save, size_t alloc_size, INT32 tag, void *user);
#define P_SaveBufferAlloc(a,b) P_SaveBufferZAlloc(a, b, PU_STATIC, NULL)
boolean P_SaveBufferFromExisting(savebuffer_t *save, UINT8 *existing_buffer, size_t existing_size);
boolean P_SaveBufferFromLump(savebuffer_t *save, lumpnum_t lump);
boolean P_SaveBufferFromFile(savebuffer_t *save, char const *name);
void P_SaveBufferFree(savebuffer_t *save);
size_t P_SaveBufferRemaining(const savebuffer_t *save);

boolean TypeIsNetSynced(mobjtype_t type);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
