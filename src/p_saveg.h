// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_saveg.h
/// \brief Savegame I/O, archiving, persistence

#ifndef __P_SAVEG__
#define __P_SAVEG__

#ifdef __GNUG__
#pragma interface
#endif

// 1024 bytes is plenty for a savegame
#define SAVEGAMESIZE (1024)

// For netgames
#define NETSAVEGAMESIZE (768*1024)

// Persistent storage/archiving.
// These are the load / save game routines.

void P_SaveGame(savebuffer_t *save, INT16 mapnum);
void P_SaveNetGame(savebuffer_t *save, boolean resending);
boolean P_LoadGame(savebuffer_t *save, INT16 mapoverride);
boolean P_LoadNetGame(savebuffer_t *save, boolean reloading);

mobj_t *P_FindNewPosition(UINT32 oldposition);

struct savedata_t
{
	UINT8 skin;
	INT32 score;
	INT32 lives;
	UINT16 emeralds;
	UINT8 numgameovers;
};

extern savedata_t savedata;

struct savebuffer_t
{
	UINT8 *buffer;
	UINT8 *p;
	UINT8 *end;
	size_t size;
};

#endif
