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
/// \file  r_textures.h
/// \brief Texture generation.

#ifndef R_TEXTURES_H
#define R_TEXTURES_H

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
	int16_t originx, originy;
	uint16_t wad, lump;
	uint8_t flip; // 1 = flipx, 2 = flipy, 3 = both
	uint8_t alpha; // Translucency value
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
	uint32_t hash;
	uint8_t type; // TEXTURETYPE_
	int16_t width, height;
	dboolean holes;
	uint8_t flip; // 1 = flipx, 2 = flipy, 3 = both
	void *flat; // The texture, as a flat.
	size_t terrainID;

	// All the patches[patchcount] are drawn back to front into the cached texture.
	int16_t patchcount;
	texpatch_t patches[];
};

// all loaded and prepared textures from the start of the game
extern texture_t **textures;

extern int32_t *texturewidth;
extern fixed_t *textureheight; // needed for texture pegging

extern uint32_t **texturecolumnofs; // column offset lookup table for each texture
extern uint8_t **texturecache; // graphics data for each generated full-size texture
extern uint8_t **texturebrightmapcache; // graphics data for brightmap converted for use with a specific texture

// Load TEXTURES definitions, create lookup tables
void R_LoadTextures(void);
void R_LoadTexturesPwad(uint16_t wadnum);
void R_FlushTextureCache(void);

// Texture generation
uint8_t *R_GenerateTexture(size_t texnum);
uint8_t *R_GenerateTextureAsFlat(size_t texnum);
uint8_t *R_GenerateTextureBrightmap(size_t texnum);
int32_t R_GetTextureNum(int32_t texnum);
int32_t R_GetTextureBrightmap(int32_t texnum);
dboolean R_TextureHasBrightmap(int32_t texnum);
dboolean R_TextureCanRemap(int32_t texnum);
void R_CheckTextureCache(int32_t tex);
void R_ClearTextureNumCache(dboolean btell);

// Retrieve texture data.
void *R_GetLevelFlat(drawspandata_t* ds, levelflat_t *levelflat);
uint8_t *R_GetColumn(fixed_t tex, int32_t col);
uint8_t *R_GetBrightmapColumn(fixed_t tex, int32_t col);
void *R_GetFlat(lumpnum_t flatnum);

dboolean R_CheckPowersOfTwo(drawspandata_t* ds);
void R_CheckFlatLength(drawspandata_t* ds, size_t size);

void R_UpdateTextureBrightmap(int32_t tx, int32_t bm);

// Returns the texture number for the texture name.
int32_t R_TextureNumForName(const char *name);
int32_t R_CheckTextureNumForName(const char *name);
lumpnum_t R_GetFlatNumForName(const char *name);

void R_CheckTextureDuplicates(int32_t start, int32_t end);
void R_PrintTextureDuplicates(void);

void R_InsertTextureWarning(const char *header, const char *warning);
void R_PrintTextureWarnings(void);

extern int32_t numtextures;

extern int32_t g_texturenum_dbgline;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
