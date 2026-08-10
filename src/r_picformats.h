// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_picformats.h
/// \brief Patch generation.

#ifndef R_PICFORMATS_H
#define R_PICFORMATS_H

#include "r_defs.h"
#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	PICFMT_NONE = 0,

	// Doom formats
	PICFMT_PATCH,
	PICFMT_FLAT,
	PICFMT_DOOMPATCH,

	// PNG
	PICFMT_PNG,

	// 16bpp
	PICFMT_PATCH16,
	PICFMT_FLAT16,
	PICFMT_DOOMPATCH16,

	// 32bpp
	PICFMT_PATCH32,
	PICFMT_FLAT32,
	PICFMT_DOOMPATCH32
} pictureformat_t;

typedef enum
{
	PICFLAGS_NONE = 0,
	PICFLAGS_XFLIP = 1,
	PICFLAGS_YFLIP = 1<<1
} pictureflags_t;

enum
{
	PICDEPTH_NONE = 0,
	PICDEPTH_8BPP = 8,
	PICDEPTH_16BPP = 16,
	PICDEPTH_32BPP = 32
};

void *Picture_Convert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	int32_t inwidth, int32_t inheight, int32_t inleftoffset, int32_t intopoffset,
	pictureflags_t flags);

void *Picture_PatchConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	int16_t inwidth, int16_t inheight, int16_t inleftoffset, int16_t intopoffset,
	pictureflags_t flags);
void *Picture_FlatConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	int16_t inwidth, int16_t inheight, int16_t inleftoffset, int16_t intopoffset,
	pictureflags_t flags);
void *Picture_GetPatchPixel(
	patch_t *patch, pictureformat_t informat,
	int32_t x, int32_t y,
	pictureflags_t flags);

void *Picture_TextureToFlat(size_t trickytex);

int32_t Picture_FormatBPP(pictureformat_t format);
dboolean Picture_IsPatchFormat(pictureformat_t format);
dboolean Picture_IsInternalPatchFormat(pictureformat_t format);
dboolean Picture_IsDoomPatchFormat(pictureformat_t format);
dboolean Picture_IsFlatFormat(pictureformat_t format);
dboolean Picture_CheckIfDoomPatch(softwarepatch_t *patch, size_t size);

// Structs
typedef enum
{
	ROTAXIS_X, // Roll (the default)
	ROTAXIS_Y, // Pitch
	ROTAXIS_Z  // Yaw
} rotaxis_t;

struct spriteframepivot_t
{
	int32_t x, y;
	rotaxis_t rotaxis;
};

struct spriteinfo_t
{
	spriteframepivot_t pivot[64 + 1];
#define SPRINFO_DEFAULT_PIVOT (64)
	uint8_t available[BIT_ARRAY_SIZE(64 + 1)]; // 1 extra for default_pivot
	char *bright[64 + 1]; // brightmap lump name
};

// Portable Network Graphics
#define PNG_HEADER_SIZE (8)
dboolean Picture_IsLumpPNG(const uint8_t *d, size_t s);
#define Picture_ThrowPNGError(lumpname, wadfilename) I_Error("W_Wad: Lump \"%s\" in file \"%s\" is a .png - please convert to either Doom or Flat (raw) image format.", lumpname, wadfilename); // Fears Of LJ Sonic

#ifndef NO_PNG_LUMPS
void *Picture_PNGConvert(
	const uint8_t *png, pictureformat_t outformat,
	int32_t *w, int32_t *h,
	int16_t *topoffset, int16_t *leftoffset,
	size_t insize, size_t *outsize,
	pictureflags_t flags);
dboolean Picture_PNGDimensions(uint8_t *png, int32_t *width, int32_t *height, int16_t *topoffset, int16_t *leftoffset, size_t size);
#endif

#define PICTURE_PNG_USELOOKUP

// SpriteInfo
extern spriteinfo_t spriteinfo[NUMSPRITES];
void R_LoadSpriteInfoLumps(uint16_t wadnum, uint16_t numlumps);
void R_ParseSPRTINFOLump(uint16_t wadNum, uint16_t lumpNum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // R_PICFORMATS_H
