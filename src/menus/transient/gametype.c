// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/gametype.c
/// \brief Gametype selection

#include "../../k_menu.h"

int16_t menugametype = GT_RACE;

void M_NextMenuGametype(uint32_t forbidden)
{
	const int16_t currentmenugametype = menugametype;
	do
	{
		menugametype++;
		if (menugametype >= numgametypes)
			menugametype = 0;

		if (!(gametypes[menugametype]->rules & forbidden))
			break;
	} while (menugametype != currentmenugametype);
}

void M_PrevMenuGametype(uint32_t forbidden)
{
	const int16_t currentmenugametype = menugametype;
	do
	{
		if (menugametype == 0)
			menugametype = numgametypes;
		menugametype--;

		if (!(gametypes[menugametype]->rules & forbidden))
			break;
	} while (menugametype != currentmenugametype);
}
