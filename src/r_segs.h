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
/// \file  r_segs.h
/// \brief Refresh module, drawing LineSegs from BSP

#ifndef __R_SEGS__
#define __R_SEGS__

#ifdef __cplusplus
extern "C" {
#endif

transnum_t R_GetLinedefTransTable(fixed_t alpha);
void R_RenderMaskedSegRange(drawseg_t *ds, INT32 x1, INT32 x2);
void R_RenderThickSideRange(drawseg_t *ds, INT32 x1, INT32 x2, ffloor_t *pffloor);
void R_StoreWallRange(INT32 start, INT32 stop);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
