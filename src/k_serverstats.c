// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
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
#include "time.h"
#include "d_netcmd.h" // isplayeradmin

static serverplayer_t *trackedList;
static size_t numtracked = 0;
static size_t numallocated = 0;
static boolean initialized = false;

UINT16 guestpwr[PWRLV_NUMTYPES]; // All-zero power level to reference for guests

static void SV_InitializeStats(void)
{
	if (!initialized)
	{
		numallocated = 8;
		trackedList = Z_Calloc(
			sizeof(serverplayer_t) * numallocated,
			PU_STATIC,
			&trackedList
		);

		if (trackedList == NULL)
		{
			I_Error("Not enough memory for server stats\n");
		}

		initialized = true;
	}
}

static void SV_ExpandStats(size_t needed)
{
	I_Assert(trackedList != NULL);

	while (numallocated < needed)
	{
		numallocated *= 2;
		trackedList = Z_Realloc(
			trackedList,
			sizeof(serverplayer_t) * numallocated,
			PU_STATIC,
			&trackedList
		);

		if (trackedList == NULL)
		{
			I_Error("Not enough memory for server stats\n");
		}
	}

}

// Read stats file to trackedList for ingame use
void SV_LoadStats(void)
{
	const size_t headerlen = strlen(SERVERSTATSHEADER);
	savebuffer_t save = {0};
	unsigned int i, j;

	if (!server)
		return;

	if (P_SaveBufferFromFile(&save, va(pandf, srb2home, SERVERSTATSFILE)) == false)
	{
		return;
	}

	SV_InitializeStats();

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
	if (version > SERVERSTATSVER)
	{
		P_SaveBufferFree(&save);
		I_Error("Existing %s is from the future! (expected %d, got %d)", SERVERSTATSFILE, SERVERSTATSVER, version);
	}
	else if (version < SERVERSTATSVER)
	{
		// We're converting - let's create a backup.
		FIL_WriteFile(va("%s" PATHSEP "%s.bak", srb2home, SERVERSTATSFILE), save.buffer, save.size);
	}

	numtracked = READUINT32(save.p);

	SV_ExpandStats(numtracked);

	for(i = 0; i < numtracked; i++)
	{
		READMEM(save.p, trackedList[i].public_key, PUBKEYLENGTH);
		READMEM(save.p, &trackedList[i].lastseen, sizeof(trackedList[i].lastseen));
		for(j = 0; j < PWRLV_NUMTYPES; j++)
		{
			trackedList[i].powerlevels[j] = READUINT16(save.p);
		}

		// Migration 1 -> 2: Add finishedrounds
		if (version < 2)
			trackedList[i].finishedrounds = 0;
		else
			trackedList[i].finishedrounds = READUINT32(save.p);

		trackedList[i].hash = quickncasehash((char*)trackedList[i].public_key, PUBKEYLENGTH);
	}
}

// Save trackedList to disc
void SV_SaveStats(void)
{
	size_t length = 0;
	const size_t headerlen = strlen(SERVERSTATSHEADER);
	savebuffer_t save = {0};
	unsigned int i, j;

	if (!server)
		return;

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

	for(i = 0; i < numtracked; i++)
	{
		WRITEMEM(save.p, trackedList[i].public_key, PUBKEYLENGTH);
		WRITEMEM(save.p, &trackedList[i].lastseen, sizeof(trackedList[i].lastseen));
		for(j = 0; j < PWRLV_NUMTYPES; j++)
		{
			WRITEUINT16(save.p, trackedList[i].powerlevels[j]);
		}
		WRITEUINT32(save.p, trackedList[i].finishedrounds);
	}

	length = save.p - save.buffer;

	if (!FIL_WriteFile(va(pandf, srb2home, SERVERSTATSFILE), save.buffer, length))
	{
		P_SaveBufferFree(&save);
		I_Error("Couldn't save server stats. Are you out of disk space / playing in a protected folder?");
	}
	P_SaveBufferFree(&save);
}

// New player, grab their stats from trackedList or initialize new ones if they're new
serverplayer_t *SV_GetStatsByKey(uint8_t *key)
{
	UINT32 j, hash;

	SV_InitializeStats();

	hash = quickncasehash((char*)key, PUBKEYLENGTH);

	// Existing record?
	for(j = 0; j < numtracked; j++)
	{
		if (hash != trackedList[j].hash) // Not crypto magic, just an early out with a faster comparison
			continue;
		if (memcmp(trackedList[j].public_key, key, PUBKEYLENGTH) == 0)
			return &trackedList[j];
	}

	// Untracked below this point, make a new record
	SV_ExpandStats(numtracked+1);

	// Default stats
	// (NB: This will make a GUEST record if someone tries to retrieve GUEST stats, because
	// at the very least we should try to provide other codepaths the right  _data type_,
	// but it will not be written back.)
	trackedList[numtracked].lastseen = time(NULL);
	memcpy(&trackedList[numtracked].public_key, key, PUBKEYLENGTH);
	for(j = 0; j < PWRLV_NUMTYPES; j++)
	{
		trackedList[numtracked].powerlevels[j] = PR_IsKeyGuest(key) ? 0 : PWRLVRECORD_START;
	}
	trackedList[numtracked].finishedrounds = 0;
	trackedList[numtracked].hash = quickncasehash((char*)key, PUBKEYLENGTH);

	numtracked++;

	return &trackedList[numtracked - 1];
}

serverplayer_t *SV_GetStatsByPlayerIndex(UINT8 p)
{
	return SV_GetStatsByKey(players[p].public_key);
}

serverplayer_t *SV_GetStats(player_t *player)
{
	return SV_GetStatsByKey(player->public_key);
}

// Write clientpowerlevels and timestamps back to matching trackedList entries, then save trackedList to disk
// (NB: Stats changes can be made directly to trackedList through other paths, but will only write to disk here)
void SV_UpdateStats(void)
{
	UINT32 i, j, hash;

	if (!server)
		return;

	SV_InitializeStats();

	for(i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (PR_IsKeyGuest(players[i].public_key))
			continue;

		hash = quickncasehash((char*)players[i].public_key, PUBKEYLENGTH);

		for(j = 0; j < numtracked; j++)
		{
			if (hash != trackedList[j].hash) // Not crypto magic, just an early out with a faster comparison
				continue;
			if (memcmp(&trackedList[j].public_key, players[i].public_key, PUBKEYLENGTH) == 0)
			{
				trackedList[j].lastseen = time(NULL);
				memcpy(&trackedList[j].powerlevels, clientpowerlevels[i], sizeof(trackedList[j].powerlevels));
				break;
			}
		}

		// SV_RetrievePWR should always be called for a key before SV_UpdateStats runs,
		// so this shouldn't be reachable.
	}

	SV_UpdateTempMutes();
	SV_SaveStats();
}

void SV_BumpMatchStats(void)
{
	int i;

	if (!server)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator)
			continue;
		if (PR_IsKeyGuest(players[i].public_key))
			continue;

		serverplayer_t *stat = SV_GetStatsByPlayerIndex(i);

		// It should never be advantageous to idle, only count rounds where the player accomplishes something.
		// If you NO CONTESTed, assume no participation...
		boolean participated = !(players[i].pflags & PF_NOCONTEST);

		if (gametyperules & GTR_CIRCUIT)
		{
			// ...unless you completed at least one lap...
			if (players[i].laps > 1)
				participated = true;
		}
		else if (gametyperules & GTR_POINTLIMIT)
		{
			// ...or scored at least 2 points.
			if (players[i].roundscore > 1)
				participated = true;
		}

		if (participated)
			stat->finishedrounds++;
	}

	SV_UpdateTempMutes();
}

static void SV_UpdateTempMute(player_t *player, boolean mute)
{
	UINT8 buf[2];

	if (mute == !!(player->pflags2 & PF2_SERVERTEMPMUTE))
		return;

	buf[0] = player - players;
	buf[1] = (UINT8)(mute);
	SendNetXCmd(XD_SERVERTEMPMUTEPLAYER, &buf, 2);
}

void SV_UpdateTempMutes(void)
{
	int i;

	if (!server)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		player_t *player = &players[i];

		if (PR_IsKeyGuest(player->public_key))
		{
			if (cv_gamestochat.value)
				SV_UpdateTempMute(player, false);
			continue;
		}

		serverplayer_t *stat = SV_GetStatsByPlayerIndex(i);

		if (i == serverplayer || IsPlayerAdmin(i))
			SV_UpdateTempMute(player, false);
		else if (stat->finishedrounds >= (UINT32)cv_gamestochat.value)
			SV_UpdateTempMute(player, false);
		else if (stat->finishedrounds < (UINT32)cv_gamestochat.value)
			SV_UpdateTempMute(player, true);
	}
}
