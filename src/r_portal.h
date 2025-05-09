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
/// \file  r_portal.h
/// \brief Software renderer portal struct, functions, linked list extern.

#ifndef __R_PORTAL__
#define __R_PORTAL__

#include "r_data.h"
#include "r_textures.h"
#include "r_plane.h" // visplanes

#ifdef __cplusplus
extern "C" {
#endif

/** Portal structure for the software renderer.
 */
struct portal_t
{
	portal_t *next;

	// Viewport.
	fixed_t viewx;
	fixed_t viewy;
	fixed_t viewz;
	angle_t viewangle;

	UINT8 pass;			/**< Keeps track of the portal's recursion depth. */
	boolean isskybox;
	INT32 clipline;		/**< Optional clipline for line-based portals. */

	// Clipping information.
	INT32 start;		/**< First horizontal pixel coordinate to draw at. */
	INT32 end;			/**< Last horizontal pixel coordinate to draw at. */
	INT16 *ceilingclip; /**< Temporary screen top clipping array. */
	INT16 *floorclip;	/**< Temporary screen bottom clipping array. */
	fixed_t *frontscale;/**< Temporary screen bottom clipping array. */
};

extern portal_t* portal_base;
extern portal_t* portal_cap;
extern UINT8 portalrender;
extern boolean portalskipprecipmobjs;

extern line_t *portalclipline;
extern sector_t *portalcullsector;
extern INT32 portalclipstart, portalclipend;

void Portal_InitList	(void);
void Portal_Remove		(portal_t* portal);
void Portal_Add2Lines	(const INT32 line1, const INT32 line2, const INT32 x1, const INT32 x2);
void Portal_AddSkybox	(const player_t* player, const visplane_t* plane);

void Portal_ClipApply (const portal_t* portal);

void Portal_AddSkyboxPortals (const player_t* player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
