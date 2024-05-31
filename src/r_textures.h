// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_textures.h
/// \brief Texture generation.

#ifndef __R_TEXTURES__
#define __R_TEXTURES__

#include "r_defs.h"
#include "r_state.h"
#include "p_setup.h" // levelflats
#include "r_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MISSING_TEXTURE "AASMELLY" // Replacement for invalid textures

// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
struct texpatch_t
{
	// Block origin (always UL), which has already accounted for the internal origin of the patch.
	INT16 originx, originy;
	UINT16 wad, lump;
	UINT8 flip; // 1 = flipx, 2 = flipy, 3 = both
	UINT8 alpha; // Translucency value
	patchalphastyle_t style;
};

// texture type
enum
{
	TEXTURETYPE_UNKNOWN,
	TEXTURETYPE_SINGLEPATCH,
	TEXTURETYPE_COMPOSITE,
#ifdef WALLFLATS
	TEXTURETYPE_FLAT,
#endif
};

// A texture_t describes a rectangular texture,
//  which is composed of one or more texpatch_t structures
//  that arrange graphic patches.
struct texture_t
{
	// Keep name for switch changing, etc.
	char name[8];
	UINT32 hash;
	UINT8 type; // TEXTURETYPE_
	INT16 width, height;
	boolean holes;
	UINT8 flip; // 1 = flipx, 2 = flipy, 3 = both
	void *flat; // The texture, as a flat.
	size_t terrainID;

	// All the patches[patchcount] are drawn back to front into the cached texture.
	INT16 patchcount;
	texpatch_t patches[];
};

// all loaded and prepared textures from the start of the game
extern texture_t **textures;

extern INT32 *texturewidth;
extern fixed_t *textureheight; // needed for texture pegging

extern UINT32 **texturecolumnofs; // column offset lookup table for each texture
extern UINT8 **texturecache; // graphics data for each generated full-size texture
extern UINT8 **texturebrightmapcache; // graphics data for brightmap converted for use with a specific texture

// Load TEXTURES definitions, create lookup tables
void R_LoadTextures(void);
void R_LoadTexturesPwad(UINT16 wadnum);
void R_FlushTextureCache(void);

// Texture generation
UINT8 *R_GenerateTexture(size_t texnum);
UINT8 *R_GenerateTextureAsFlat(size_t texnum);
UINT8 *R_GenerateTextureBrightmap(size_t texnum);
INT32 R_GetTextureNum(INT32 texnum);
INT32 R_GetTextureBrightmap(INT32 texnum);
boolean R_TextureHasBrightmap(INT32 texnum);
boolean R_TextureCanRemap(INT32 texnum);
void R_CheckTextureCache(INT32 tex);
void R_ClearTextureNumCache(boolean btell);

// Retrieve texture data.
void *R_GetLevelFlat(drawspandata_t* ds, levelflat_t *levelflat);
UINT8 *R_GetColumn(fixed_t tex, INT32 col);
UINT8 *R_GetBrightmapColumn(fixed_t tex, INT32 col);
void *R_GetFlat(lumpnum_t flatnum);

boolean R_CheckPowersOfTwo(drawspandata_t* ds);
void R_CheckFlatLength(drawspandata_t* ds, size_t size);

void R_UpdateTextureBrightmap(INT32 tx, INT32 bm);

// Returns the texture number for the texture name.
INT32 R_TextureNumForName(const char *name);
INT32 R_CheckTextureNumForName(const char *name);
lumpnum_t R_GetFlatNumForName(const char *name);

void R_CheckTextureDuplicates(INT32 start, INT32 end);
void R_PrintTextureDuplicates(void);

void R_InsertTextureWarning(const char *header, const char *warning);
void R_PrintTextureWarnings(void);

extern INT32 numtextures;

extern INT32 g_texturenum_dbgline;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
