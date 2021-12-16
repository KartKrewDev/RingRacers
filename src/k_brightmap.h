// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2021 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2021 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_brightmap.h
/// \brief Brightmap texture loading.

#ifndef __K_BRIGHTMAP_H__
#define __K_BRIGHTMAP_H__

#include "doomdata.h"
#include "doomdef.h"
#include "doomtype.h"

typedef struct brightmapStorage_s
{
	// Brightmap storage struct.
	// Stores data for brightmap definitions,
	// before putting them into texturebrightmaps.

	char textureName[9]; // The texture's name.
	char brightmapName[9]; // The brightmap's name.
} brightmapStorage_t;

/*--------------------------------------------------
	void K_InitBrightmaps(void);

		Finds all BRIGHT lumps and processes them.
--------------------------------------------------*/

void K_InitBrightmaps(void);

#endif // __K_BRIGHTMAP_H__
