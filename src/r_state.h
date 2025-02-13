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
/// \file  r_state.h
/// \brief Refresh/render internal state variables (global)

#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"
#include "doomstat.h" // MAXSPLITSCREENPLAYERS

#ifdef __cplusplus
extern "C" {
#endif

//
// Refresh internal data structures, for rendering.
//

// needed for pre rendering (fracs)
struct sprcache_t
{
	fixed_t width;
	fixed_t offset;
	fixed_t topoffset;
	fixed_t height;
};

extern sprcache_t *spritecachedinfo;

extern lighttable_t *colormaps;
extern UINT8 *encoremap;
#ifdef HASINVERT
extern UINT8 invertmap[256];
#endif

#define LIGHTLEVELS 32
#define COLORMAP_SIZE (256*LIGHTLEVELS)
#define COLORMAP_REMAPOFFSET COLORMAP_SIZE

// Boom colormaps.
extern extracolormap_t *extra_colormaps;

// for global animation
extern INT32 *texturetranslation;

// for brightmaps
extern INT32 *texturebrightmaps;

// Sprites
extern size_t numspritelumps, max_spritelumps;

//
// Lookup tables for map data.
//
#define UDMF_CURRENT_VERSION (2)
extern boolean udmf;
extern INT32 udmf_version;

extern size_t numsprites;
extern spritedef_t *sprites;

extern size_t numvertexes;
extern vertex_t *vertexes;

extern size_t numsegs;
extern seg_t *segs;

extern size_t numsectors;
extern sector_t *sectors;
extern sector_t *spawnsectors;

extern size_t numsubsectors;
extern subsector_t *subsectors;

extern size_t numnodes;
extern node_t *nodes;

extern size_t numlines;
extern line_t *lines;
extern line_t *spawnlines;

extern size_t numsides;
extern side_t *sides;
extern side_t *spawnsides;

//
// POV data.
//
extern fixed_t viewx, viewy, viewz;
extern angle_t viewangle, aimingangle, viewroll;
extern UINT8 viewssnum; // splitscreen view number
extern boolean viewsky, skyVisible;
extern boolean skyVisiblePerPlayer[MAXSPLITSCREENPLAYERS]; // saved values of skyVisible of each splitscreen player
extern sector_t *viewsector;
extern player_t *viewplayer;
extern mobj_t *r_viewmobj;

extern consvar_t cv_allowmlook;
extern consvar_t cv_maxportals;

extern angle_t clipangle[MAXSPLITSCREENPLAYERS];
extern angle_t doubleclipangle[MAXSPLITSCREENPLAYERS];

extern INT32 viewangletox[MAXSPLITSCREENPLAYERS][FINEANGLES/2];
extern angle_t xtoviewangle[MAXSPLITSCREENPLAYERS][MAXVIDWIDTH+1];

extern fixed_t rw_distance;
extern angle_t rw_normalangle;

// angle to line origin
extern angle_t rw_angle1;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
