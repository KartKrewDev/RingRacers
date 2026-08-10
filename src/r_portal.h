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

#ifndef R_PORTAL_H
#define R_PORTAL_H

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

	uint8_t pass;			/**< Keeps track of the portal's recursion depth. */
	dboolean isskybox;
	int32_t clipline;		/**< Optional clipline for line-based portals. */

	// Clipping information.
	int32_t start;		/**< First horizontal pixel coordinate to draw at. */
	int32_t end;			/**< Last horizontal pixel coordinate to draw at. */
	int16_t *ceilingclip; /**< Temporary screen top clipping array. */
	int16_t *floorclip;	/**< Temporary screen bottom clipping array. */
	fixed_t *frontscale;/**< Temporary screen bottom clipping array. */
};

extern portal_t* portal_base;
extern portal_t* portal_cap;
extern uint8_t portalrender;
extern dboolean portalskipprecipmobjs;

extern line_t *portalclipline;
extern sector_t *portalcullsector;
extern int32_t portalclipstart, portalclipend;

void Portal_InitList	(void);
void Portal_Remove		(portal_t* portal);
void Portal_Add2Lines	(const int32_t line1, const int32_t line2, const int32_t x1, const int32_t x2);
void Portal_AddSkybox	(const player_t* player, const visplane_t* plane);

void Portal_ClipApply (const portal_t* portal);

void Portal_AddSkyboxPortals (const player_t* player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
