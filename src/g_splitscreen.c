// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_splitscreen.c
/// \brief some splitscreen stuff

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "r_local.h"

INT32 splitscreen_original_party_size[MAXPLAYERS];
INT32 splitscreen_original_party[MAXPLAYERS][MAXSPLITSCREENPLAYERS];

INT32 splitscreen_invitations[MAXPLAYERS];
INT32 splitscreen_party_size[MAXPLAYERS];
INT32 splitscreen_party[MAXPLAYERS][MAXSPLITSCREENPLAYERS];

boolean splitscreen_partied[MAXPLAYERS];

void
G_ResetSplitscreen (INT32 playernum)
{
	INT32 old_displayplayers[MAXSPLITSCREENPLAYERS];

	INT32 i;

	splitscreen_party_size[playernum] =
		splitscreen_original_party_size[playernum];

	memcpy(splitscreen_party[playernum], splitscreen_original_party[playernum],
			sizeof splitscreen_party[playernum]);

	if (playernum == consoleplayer)
	{
		memset(splitscreen_partied, 0, sizeof splitscreen_partied);
		splitscreen_partied[consoleplayer] = true;

		memcpy(old_displayplayers, displayplayers, sizeof old_displayplayers);

		/* easier to just rebuild displayplayers with local players */
		for (i = 0; i <= splitscreen; ++i)
		{
			displayplayers[i] = g_localplayers[i];
			P_ResetCamera(&players[displayplayers[i]], &camera[i]);
		}
		while (i < MAXSPLITSCREENPLAYERS)
		{
			displayplayers[i] = consoleplayer;

			i++;
		}

		r_splitscreen = splitscreen;

		R_ExecuteSetViewSize();
	}
}

void
G_RemovePartyMember (INT32 playernum)
{
	INT32 old_party[MAXSPLITSCREENPLAYERS];
	INT32 new_party[MAXSPLITSCREENPLAYERS];

	INT32 old_party_size;
	INT32 before;
	INT32 after;
	INT32 views;

	INT32 i;
	INT32 n;

	old_party_size = splitscreen_party_size[playernum];

	for (i = 0; i < old_party_size; ++i)
	{
		/* exploit that splitscreen players keep order */
		if (splitscreen_party[playernum][i] == playernum)
		{
			before = i;

			views = splitscreen_original_party_size[playernum];
			after = ( before + views );

			memcpy(old_party, splitscreen_party[playernum], sizeof old_party);
			memcpy(new_party, old_party, before * sizeof *old_party);

			memcpy(&new_party[before], &old_party[after],
					( old_party_size - after ) * sizeof *new_party);

			views = ( old_party_size - views );

			for (i = 0; i < old_party_size; ++i)
			{
				n = old_party[i];
				if (n != playernum && playerconsole[n] == n)
				{
					splitscreen_party_size[n] = views;
					memcpy(splitscreen_party[n], new_party,
							sizeof splitscreen_party[n]);
				}
			}

			/* don't want to remove yourself from your own screen! */
			if (playernum != consoleplayer && splitscreen_partied[playernum])
			{
				splitscreen_partied[playernum] = false;

				for (i = 0; i < views; ++i)
				{
					displayplayers[i] = new_party[i];
					P_ResetCamera(&players[displayplayers[i]], &camera[i]);
				}
				while (i < MAXSPLITSCREENPLAYERS)
				{
					displayplayers[i] = displayplayers[0];

					i++;
				}

				r_splitscreen = ( views - 1 );

				R_ExecuteSetViewSize();
			}

			break;
		}
	}
}

void
G_AddPartyMember (INT32 invitation, INT32 playernum)
{
	INT32 *    party;
	INT32 *add_party;

	INT32 old_party_size;
	INT32 new_party_size;

	INT32 views;

	INT32 i;
	INT32 n;

	views = splitscreen_original_party_size[playernum];

	old_party_size = splitscreen_party_size[invitation];
	new_party_size = ( old_party_size + views );

	party = splitscreen_party[invitation];
	add_party = splitscreen_original_party[playernum];

	for (i = 0; i < old_party_size; ++i)
	{
		n = party[i];
		if (playerconsole[n] == n)
		{
			splitscreen_party_size[n] = new_party_size;
			memcpy(&splitscreen_party[n][old_party_size], add_party,
					views * sizeof *splitscreen_party[n]);
		}
	}

	splitscreen_party_size[playernum] = new_party_size;
	memcpy(splitscreen_party[playernum], party,
			sizeof splitscreen_party[playernum]);

	/* in my party or adding me? */
	if (splitscreen_partied[invitation])
	{
		splitscreen_partied[playernum] = true;

		for (i = old_party_size; i < new_party_size; ++i)
		{
			displayplayers[i] = party[i];
			P_ResetCamera(&players[displayplayers[i]], &camera[i]);
		}

		r_splitscreen += views;

		R_ExecuteSetViewSize();
	}
	else if (playernum == consoleplayer)
	{
		for (i = 0; i < new_party_size; ++i)
		{
			splitscreen_partied[playerconsole[party[i]]] = true;

			displayplayers[i] = party[i];
			P_ResetCamera(&players[displayplayers[i]], &camera[i]);
		}
		while (i < MAXSPLITSCREENPLAYERS)
		{
			displayplayers[i] = displayplayers[0];

			i++;
		}

		r_splitscreen = ( new_party_size - 1 );

		R_ExecuteSetViewSize();
	}
}
