// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_serverstats.c
/// \brief implements methods for serverside stat tracking.

#include "doomtype.h"
#include "d_main.h" // pandf
#include "byteptr.h" // READ/WRITE macros
#include "p_saveg.h" // savebuffer_t
#include "m_misc.h" //FIL_WriteFile()
#include "k_serverstats.h"
#include "z_zone.h"

static serverplayer_t trackedList[MAXTRACKEDSERVERPLAYERS];
static UINT32 numtracked = 0;

UINT16 guestpwr[PWRLV_NUMTYPES]; // All-zero power level to reference for guests

// Read stats file to trackedList for ingame use
void SV_LoadStats(void)
{
	const size_t headerlen = strlen(SERVERSTATSHEADER);
	savebuffer_t save = {0};

	if (!server)
		return;

	if (P_SaveBufferFromFile(&save, va(pandf, srb2home, SERVERSTATSFILE)) == false)
	{
		return;
	}

	if (strncmp(SERVERSTATSHEADER, (const char *)save.buffer, headerlen))
	{
		const char *gdfolder = "the Ring Racers folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		P_SaveBufferFree(&save);
		I_Error("Not a valid server stats file.\nDelete %s (maybe in %s) and try again.", SERVERSTATSFILE, gdfolder);
	}

	save.p += headerlen;
	UINT8 version = READUINT8(save.p);
	
	numtracked = READUINT32(save.p);
	if (numtracked > MAXTRACKEDSERVERPLAYERS)
		numtracked = MAXTRACKEDSERVERPLAYERS;

	READMEM(save.p, trackedList, (numtracked * sizeof(serverplayer_t)));
}

// Save trackedList to disc
void SV_SaveStats(void)
{
	size_t length = 0;
	const size_t headerlen = strlen(SERVERSTATSHEADER);
	UINT8 i;
	savebuffer_t save = {0};

	if (!server)
		return;

	/*
	if (profilesList[PROFILE_GUEST] == NULL)
	{
		// Profiles have not been loaded yet, don't overwrite with garbage.
		return;
	}
	*/

	// header + version + numtracked + payload
	if (P_SaveBufferAlloc(&save, headerlen + sizeof(UINT32) + sizeof(UINT8) + (numtracked * sizeof(serverplayer_t))) == false)
	{
		I_Error("No more free memory for saving server stats\n");
		return;
	}

	// Add header.
	WRITESTRINGN(save.p, SERVERSTATSHEADER, headerlen);

	WRITEUINT8(save.p, SERVERSTATSVER);

	WRITEUINT32(save.p, numtracked);

	WRITEMEM(save.p, trackedList, (numtracked * sizeof(serverplayer_t)));

	for (i = 0; i < numtracked; i++)
	{

	}

	length = save.p - save.buffer;

	if (!FIL_WriteFile(va(pandf, srb2home, SERVERSTATSFILE), save.buffer, length))
	{
		P_SaveBufferFree(&save);
		I_Error("Couldn't save server stats. Are you out of Disk space / playing in a protected folder?");
	}
	P_SaveBufferFree(&save);
}

// New player, grab their stats from trackedList or initialize new ones if they're new
UINT16 *SV_RetrievePWR(uint8_t *key)
{
	UINT32 j;

	// Existing record?
	for(j = 0; j < numtracked; j++)
	{
		if (memcmp(trackedList[j].public_key, key, PUBKEYLENGTH) == 0)
		{
			return trackedList[j].powerlevels;
		}
	}

	// Untracked, make a new record
	memcpy(trackedList[numtracked].public_key, key, PUBKEYLENGTH);
	for(j = 0; j < PWRLV_NUMTYPES; j++)
	{
		trackedList[numtracked].powerlevels[j] = PR_IsKeyGuest(key) ? 0 : PWRLVRECORD_START;
	}
	
	numtracked++;

	return trackedList[numtracked - 1].powerlevels;
}

// Write player stats to trackedList, then save to disk
void SV_UpdateStats(void)
{	
	UINT32 i, j;

	if (!server)
		return;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (PR_IsKeyGuest(players[i].public_key))
		{
			continue;
		}

		for(j = 0; j < numtracked; j++)
		{
			if (memcmp(trackedList[j].public_key, players[i].public_key, PUBKEYLENGTH) == 0)
			{
				memcpy(trackedList[j].powerlevels, clientpowerlevels[i], sizeof(trackedList[j].powerlevels));
				break;
			}
		}

		// SV_RetrievePWR should always be called for a key before SV_UpdateStats runs,
		// so this shouldn't be reachable.
	}

	SV_SaveStats();
}