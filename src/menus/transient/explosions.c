// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/explosions.c
/// \brief Explosions used on the character select grid and
///        challenges grid.

#include "../../k_menu.h"
#include "../../m_cond.h" // Condition Sets

struct setup_explosions_s setup_explosions[CSEXPLOSIONS];

void M_SetupReadyExplosions(boolean charsel, UINT16 basex, UINT16 basey, UINT16 color)
{
	UINT8 i, j;
	UINT8 e = 0;
	UINT16 maxx = (charsel ? 9 : gamedata->challengegridwidth);
	UINT16 maxy = (charsel ? 9 : CHALLENGEGRIDHEIGHT);

	while (setup_explosions[e].tics)
	{
		e++;
		if (e == CSEXPLOSIONS)
			return;
	}

	for (i = 0; i < 3; i++)
	{
		UINT8 t = 5 + (i*2);
		UINT8 offset = (i+1);

		for (j = 0; j < 4; j++)
		{
			INT16 x = basex, y = basey;

			switch (j)
			{
				case 0: x += offset; break;
				case 1: x -= offset; break;
				case 2: y += offset; break;
				case 3: y -= offset; break;
			}

			if ((y < 0 || y >= maxy))
				continue;

			if (charsel || !challengegridloops)
			{
				if (x < 0 || x >= maxx)
					continue;
			}

			setup_explosions[e].tics = t;
			setup_explosions[e].color = color;
			setup_explosions[e].x = x;
			setup_explosions[e].y = y;

			while (setup_explosions[e].tics)
			{
				e++;
				if (e == CSEXPLOSIONS)
					return;
			}
		}
	}
}
