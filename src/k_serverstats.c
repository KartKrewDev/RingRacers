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

	if (P_SaveBufferAlloc(&save, headerlen + sizeof(UINT32) + (numtracked * sizeof(serverplayer_t))) == false)
	{
		I_Error("No more free memory for saving server stats\n");
		return;
	}

	// Add header.
	WRITESTRINGN(save.p, SERVERSTATSHEADER, headerlen);

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
void SV_RetrieveStats(int player)
{
	if (!server)
		return;

	UINT32 j;

	for(j = 0; j < numtracked; j++)
	{
		if (memcmp(trackedList[j].public_key, players[player].public_key, PUBKEYLENGTH) == 0)
		{
			memcpy(clientpowerlevels[player], trackedList[j].powerlevels, sizeof(trackedList[j].powerlevels));
			return;
		}
	}

	uint8_t allZero[PUBKEYLENGTH];
	memset(allZero, 0, PUBKEYLENGTH);

	for(j = 0; j < PWRLV_NUMTYPES; j++)
	{
		if (memcmp(players[player].public_key, allZero, PUBKEYLENGTH) == 0)
			clientpowerlevels[player][j] = 0;
		else
			clientpowerlevels[player][j] = PWRLVRECORD_START;
	}

}

// Write player stats to trackedList, then save to disk
void SV_UpdateStats(void)
{	
	UINT32 i, j;
	uint8_t allZero[PUBKEYLENGTH];
	memset(allZero, 0, PUBKEYLENGTH);

	if (!server)
		return;

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (memcmp(players[i].public_key, allZero, PUBKEYLENGTH) == 0)
		{
			continue;
		}

		boolean match = false;
		for(j = 0; j < numtracked; j++)
		{
			if (memcmp(trackedList[j].public_key, players[i].public_key, PUBKEYLENGTH) == 0)
			{
				memcpy(trackedList[j].powerlevels, clientpowerlevels[i], sizeof(trackedList[j].powerlevels));
				match = true;
				break;
			}
		}

		if (match)
			continue;

		memcpy(trackedList[numtracked].public_key, players[i].public_key, PUBKEYLENGTH);
		memcpy(trackedList[numtracked].powerlevels, clientpowerlevels[i], sizeof(trackedList[numtracked].powerlevels));
		
		numtracked++;
	}

	SV_SaveStats();
}