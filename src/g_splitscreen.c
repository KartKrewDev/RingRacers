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
#include "r_local.h"

int splitscreen_original_party_size[MAXPLAYERS];
int splitscreen_original_party[MAXPLAYERS][MAXSPLITSCREENPLAYERS];

int splitscreen_invitations[MAXPLAYERS];
int splitscreen_party_size[MAXPLAYERS];
int splitscreen_party[MAXPLAYERS][MAXSPLITSCREENPLAYERS];

boolean splitscreen_partied[MAXPLAYERS];

void
G_ResetSplitscreen (int playernum)
{
	INT32 old_displayplayers[MAXSPLITSCREENPLAYERS];

	int i;

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
			displayplayers[i] = old_displayplayers[localdisplayplayers[i]];
			localdisplayplayers[i] = i;
		}
		while (i < MAXSPLITSCREENPLAYERS)
		{
			displayplayers[i] = consoleplayer;
			localdisplayplayers[i] = 0;

			i++;
		}

		r_splitscreen = splitscreen;

		R_ExecuteSetViewSize();
	}
}

void
G_RemovePartyMember (int playernum)
{
	int old_party[MAXSPLITSCREENPLAYERS];
	int new_party[MAXSPLITSCREENPLAYERS];

	int old_party_size;
	int before;
	int after;
	int views;

	int i;
	int n;

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

			if (splitscreen_partied[playernum] &&
					localdisplayplayers[0] >= after)
			{
				for (i = 0; i < MAXSPLITSCREENPLAYERS; ++i)
					localdisplayplayers[i] -= views;
			}

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
				}
				while (i < MAXSPLITSCREENPLAYERS)
				{
					displayplayers[i] = consoleplayer;

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
G_AddPartyMember (int invitation, int playernum)
{
	int *    party;
	int *add_party;

	int old_party_size;
	int new_party_size;

	int views;

	int i;
	int n;

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
		}

		r_splitscreen += views;

		R_ExecuteSetViewSize();
	}
	else if (playernum == consoleplayer)
	{
		splitscreen_partied[invitation] = true;

		for (i = 0; i <= splitscreen; ++i)
		{
			localdisplayplayers[i] = ( old_party_size + i );
			displayplayers[i] = party[i];
		}
		while (++i < new_party_size)
		{
			displayplayers[i] = party[i];
			localdisplayplayers[i] = old_party_size;
		}

		r_splitscreen = ( new_party_size - 1 );

		R_ExecuteSetViewSize();
	}
}
