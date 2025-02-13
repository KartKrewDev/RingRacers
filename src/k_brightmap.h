// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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

#ifdef __cplusplus
extern "C" {
#endif

struct brightmapStorage_t
{
	// Brightmap storage struct.
	// Stores data for brightmap definitions,
	// before putting them into texturebrightmaps.

	char textureName[9];	// The texture's name.
	UINT32 textureHash;		// The texture name's hash.

	char brightmapName[9];	// The brightmap's name.
	UINT32 brightmapHash;	// The brightmap name's hash.
};

/*--------------------------------------------------
	void K_InitBrightmapsPwad(INT32 wadNum);

		Finds all BRIGHT lumps for one WAD/PK3 and processes them.
--------------------------------------------------*/

void K_InitBrightmapsPwad(INT32 wadNum);

/*--------------------------------------------------
	void K_InitBrightmaps(void);

		Finds all BRIGHT lumps and processes them.
--------------------------------------------------*/

void K_InitBrightmaps(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_BRIGHTMAP_H__
