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
/// \file  r_data.h
/// \brief Refresh module, data I/O, caching, retrieval of graphics by name

#ifndef R_DATA_H
#define R_DATA_H

#include "r_defs.h"
#include "r_state.h"
#include "p_setup.h" // levelflats

#ifdef __cplusplus
extern "C" {
#endif

// Store lists of lumps for F_START/F_END etc.
struct lumplist_t
{
	uint16_t wadfile;
	uint16_t firstlump;
	size_t numlumps;
};

uint32_t ASTBlendPixel(RGBA_t background, RGBA_t foreground, int style, uint8_t alpha);
uint32_t ASTBlendTexturePixel(RGBA_t background, RGBA_t foreground, int style, uint8_t alpha);
uint8_t ASTBlendPaletteIndexes(uint8_t background, uint8_t foreground, int style, uint8_t alpha);

extern int32_t ASTTextureBlendingThreshold[2];

extern int16_t color8to16[256]; // remap color index to highcolor
extern int16_t *hicolormaps; // remap high colors to high colors..

extern CV_PossibleValue_t Color_cons_t[];

// I/O, setting up the stuff.
void R_InitTextureData(void);
void R_PrecacheLevel(void);

extern size_t flatmemory, spritememory, texturememory;

// Extra Colormap lumps (C_START/C_END) are not used anywhere
// Uncomment to enable
//#define EXTRACOLORMAPLUMPS

// Uncomment to make extra_colormaps order Newest -> Oldest
//#define COLORMAPREVERSELIST

void R_InitColormaps(void);
void R_ReInitColormaps(uint16_t num, void *newencoremap, size_t encoremapsize);
void R_ClearColormaps(void);
extracolormap_t *R_CreateDefaultColormap(dboolean lighttable);
extracolormap_t *R_GetDefaultColormap(void);
extracolormap_t *R_CopyColormap(extracolormap_t *extra_colormap, dboolean lighttable);
void R_AddColormapToList(extracolormap_t *extra_colormap);

#ifdef EXTRACOLORMAPLUMPS
dboolean R_CheckDefaultColormapByValues(dboolean checkrgba, dboolean checkfadergba, dboolean checkparams,
	int32_t rgba, int32_t fadergba, uint8_t fadestart, uint8_t fadeend, uint8_t flags, lumpnum_t lump);
extracolormap_t *R_GetColormapFromListByValues(int32_t rgba, int32_t fadergba, uint8_t fadestart, uint8_t fadeend, uint8_t flags, lumpnum_t lump);
#else
dboolean R_CheckDefaultColormapByValues(dboolean checkrgba, dboolean checkfadergba, dboolean checkparams,
	int32_t rgba, int32_t fadergba, uint8_t fadestart, uint8_t fadeend, uint8_t flags);
extracolormap_t *R_GetColormapFromListByValues(int32_t rgba, int32_t fadergba, uint8_t fadestart, uint8_t fadeend, uint8_t flags);
#endif
dboolean R_CheckDefaultColormap(extracolormap_t *extra_colormap, dboolean checkrgba, dboolean checkfadergba, dboolean checkparams);
dboolean R_CheckEqualColormaps(extracolormap_t *exc_a, extracolormap_t *exc_b, dboolean checkrgba, dboolean checkfadergba, dboolean checkparams);
extracolormap_t *R_GetColormapFromList(extracolormap_t *extra_colormap);

typedef int textmapcolormapflags_t;
#define TMCF_RELATIVE     (1)
#define TMCF_SUBLIGHTR    (1<<1)
#define TMCF_SUBLIGHTG    (1<<2)
#define TMCF_SUBLIGHTB    (1<<3)
#define TMCF_SUBLIGHTA    (1<<4)
#define TMCF_SUBFADER     (1<<5)
#define TMCF_SUBFADEG     (1<<6)
#define TMCF_SUBFADEB     (1<<7)
#define TMCF_SUBFADEA     (1<<8)
#define TMCF_SUBFADESTART (1<<9)
#define TMCF_SUBFADEEND   (1<<10)
#define TMCF_IGNOREFLAGS  (1<<11)
#define TMCF_FROMBLACK    (1<<12)
#define TMCF_OVERRIDE     (1<<13)

lighttable_t *R_CreateLightTable(extracolormap_t *extra_colormap);
extracolormap_t * R_CreateColormapFromLinedef(char *p1, char *p2, char *p3);
extracolormap_t* R_CreateColormap(int32_t rgba, int32_t fadergba, uint8_t fadestart, uint8_t fadeend, uint8_t flags);
extracolormap_t *R_AddColormaps(extracolormap_t *exc_augend, extracolormap_t *exc_addend,
	dboolean subR, dboolean subG, dboolean subB, dboolean subA,
	dboolean subFadeR, dboolean subFadeG, dboolean subFadeB, dboolean subFadeA,
	dboolean subFadeStart, dboolean subFadeEnd, dboolean ignoreFlags,
	dboolean lighttable);
#ifdef EXTRACOLORMAPLUMPS
extracolormap_t *R_ColormapForName(char *name);
const char *R_NameForColormap(extracolormap_t *extra_colormap);
#endif

#define R_GetRgbaR(rgba) (rgba & 0xFF)
#define R_GetRgbaG(rgba) ((rgba >> 8) & 0xFF)
#define R_GetRgbaB(rgba) ((rgba >> 16) & 0xFF)
#define R_GetRgbaA(rgba) ((rgba >> 24) & 0xFF)
#define R_GetRgbaRGB(rgba) (rgba & 0xFFFFFF)
#define R_PutRgbaR(r) (r)
#define R_PutRgbaG(g) (g << 8)
#define R_PutRgbaB(b) (b << 16)
#define R_PutRgbaA(a) (a << 24)
#define R_PutRgbaRGB(r, g, b) (R_PutRgbaR(r) + R_PutRgbaG(g) + R_PutRgbaB(b))
#define R_PutRgbaRGBA(r, g, b, a) (R_PutRgbaRGB(r, g, b) + R_PutRgbaA(a))

uint8_t NearestPaletteColor(uint8_t r, uint8_t g, uint8_t b, RGBA_t *palette);
#define NearestColor(r, g, b) NearestPaletteColor(r, g, b, NULL)

#ifdef __cplusplus
} // extern "C"
#endif

#endif
