// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_bbox.h
/// \brief bounding boxes

#ifndef __M_BBOX__
#define __M_BBOX__

#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/**	\brief	Bounding box coordinate storage
*/

enum
{
	BOXTOP, /// top side of bbox
	BOXBOTTOM, /// bottom side of bbox
	BOXLEFT, /// left side of bbox
	BOXRIGHT /// right side of bbox
}; /// bbox coordinates

// Bounding box functions.
void M_ClearBox(fixed_t *box);

void M_AddToBox(fixed_t *box, fixed_t x, fixed_t y);
boolean M_PointInBox(fixed_t *box, fixed_t x, fixed_t y);
boolean M_CircleTouchBox(fixed_t *box, fixed_t circlex, fixed_t circley, fixed_t circleradius);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
