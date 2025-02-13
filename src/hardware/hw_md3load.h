// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2020 by Spaddlewit Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2.
*/

#ifndef _HW_MD3LOAD_H_
#define _HW_MD3LOAD_H_

#include "hw_model.h"
#include "../doomtype.h"

#ifdef __cplusplus
extern "C" {
#endif

// Load the Model
model_t *MD3_LoadModel(const char *fileName, int ztag, boolean useFloat);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
