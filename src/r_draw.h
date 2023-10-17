// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw.h
/// \brief Low-level span/column drawer functions

#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------
extern UINT8 *ylookup[MAXVIDHEIGHT*4];
extern UINT8 *ylookup1[MAXVIDHEIGHT*4];
extern UINT8 *ylookup2[MAXVIDHEIGHT*4];
extern UINT8 *ylookup3[MAXVIDHEIGHT*4];
extern UINT8 *ylookup4[MAXVIDHEIGHT*4];
extern INT32 columnofs[MAXVIDWIDTH*4];
extern UINT8 *topleft;
extern UINT8 r8_flatcolor;

// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

// Vectors for Software's tilted slope drawers
extern floatv3_t *ds_su, *ds_sv, *ds_sz;
extern float focallengthf[MAXSPLITSCREENPLAYERS];
extern float zeroheight;

/// \brief Top border
#define BRDR_T 0
/// \brief Bottom border
#define BRDR_B 1
/// \brief Left border
#define BRDR_L 2
/// \brief Right border
#define BRDR_R 3
/// \brief Topleft border
#define BRDR_TL 4
/// \brief Topright border
#define BRDR_TR 5
/// \brief Bottomleft border
#define BRDR_BL 6
/// \brief Bottomright border
#define BRDR_BR 7

extern lumpnum_t viewborderlump[8];



// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

#define USE_COL_SPAN_ASM 0

#define BASEDRAWFUNC 0

enum
{
	COLDRAWFUNC_BASE = BASEDRAWFUNC,
	COLDRAWFUNC_FUZZY,
	COLDRAWFUNC_TRANS,
	COLDRAWFUNC_SHADE,
	COLDRAWFUNC_SHADOWED,
	COLDRAWFUNC_TRANSTRANS,
	COLDRAWFUNC_TWOSMULTIPATCH,
	COLDRAWFUNC_TWOSMULTIPATCHTRANS,
	COLDRAWFUNC_FOG,
	COLDRAWFUNC_DROPSHADOW,

	COLDRAWFUNC_MAX
};

typedef void (coldrawfunc_t)(drawcolumndata_t*);
typedef void (spandrawfunc_t)(drawspandata_t*);

extern coldrawfunc_t *colfunc;
extern coldrawfunc_t *colfuncs[COLDRAWFUNC_MAX];
#ifdef USE_COL_SPAN_ASM
extern coldrawfunc_t *colfuncs_asm[COLDRAWFUNC_MAX];
#endif
extern int colfunctype;

enum
{
	SPANDRAWFUNC_BASE = BASEDRAWFUNC,
	SPANDRAWFUNC_TRANS,
	SPANDRAWFUNC_TILTED,
	SPANDRAWFUNC_TILTEDTRANS,

	SPANDRAWFUNC_SPLAT,
	SPANDRAWFUNC_TRANSSPLAT,
	SPANDRAWFUNC_TILTEDSPLAT,

	SPANDRAWFUNC_SPRITE,
	SPANDRAWFUNC_TRANSSPRITE,
	SPANDRAWFUNC_TILTEDSPRITE,
	SPANDRAWFUNC_TILTEDTRANSSPRITE,

	SPANDRAWFUNC_WATER,
	SPANDRAWFUNC_TILTEDWATER,

	SPANDRAWFUNC_FOG,

	SPANDRAWFUNC_MAX
};

extern spandrawfunc_t *spanfunc;
extern spandrawfunc_t *spanfuncs[SPANDRAWFUNC_MAX];
extern spandrawfunc_t *spanfuncs_npo2[SPANDRAWFUNC_MAX];
#ifdef USE_COL_SPAN_ASM
extern spandrawfunc_t *spanfuncs_asm[SPANDRAWFUNC_MAX];
#endif
extern spandrawfunc_t *spanfuncs_flat[SPANDRAWFUNC_MAX];

// ------------------------------------------------
// r_draw.c COMMON ROUTINES FOR BOTH 8bpp and 16bpp
// ------------------------------------------------

#define GTC_CACHE 1
#define GTC_MENUCACHE GTC_CACHE
//@TODO Add a separate caching mechanism for menu colormaps distinct from in-level GTC_CACHE. For now this is still preferable to memory leaks...

enum
{
	TC_BOSS       = INT8_MIN,
	TC_METALSONIC, // For Metal Sonic battle
	TC_ALLWHITE,   // For Cy-Brak-demon
	TC_RAINBOW,    // For single colour
	TC_BLINK,      // For item blinking, according to kart
	TC_DASHMODE,   // For Metal Sonic's dashmode
	TC_HITLAG,     // Damage hitlag effect
	TC_INTERMISSION, // Intermission / menu background

	TC_DEFAULT
};

// Custom player skin translation
// Initialize color translation tables, for player rendering etc.
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags);
void R_FlushTranslationColormapCache(void);
UINT16 R_GetColorByName(const char *name);
UINT16 R_GetSuperColorByName(const char *name);

extern UINT8 *transtables; // translucency tables, should be (*transtables)[5][256][256]

enum
{
	blendtab_add,
	blendtab_subtract,
	blendtab_reversesubtract,
	blendtab_modulate,
	NUMBLENDMAPS
};

extern UINT8 *blendtables[NUMBLENDMAPS];

void R_InitTranslucencyTables(void);
void R_GenerateBlendTables(void);

UINT8 *R_GetTranslucencyTable(INT32 alphalevel);
UINT8 *R_GetBlendTable(int style, INT32 alphalevel);

// Color ramp modification should force a recache
extern UINT8 skincolor_modified[];

void R_InitViewBuffer(INT32 width, INT32 height);
void R_InitViewBorder(void);
void R_VideoErase(size_t ofs, INT32 count);

// Rendering function.
#if 0
void R_FillBackScreen(void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder(void);
#endif

#define TRANSPARENTPIXEL 255
#define BRIGHTPIXEL 0

// -----------------
// 8bpp DRAWING CODE
// -----------------

void R_DrawColumn_8(drawcolumndata_t* dc);
void R_DrawShadeColumn_8(drawcolumndata_t* dc);
void R_DrawTranslucentColumn_8(drawcolumndata_t* dc);
void R_DrawDropShadowColumn_8(drawcolumndata_t* dc);
void R_DrawTranslatedColumn_8(drawcolumndata_t* dc);
void R_DrawTranslatedTranslucentColumn_8(drawcolumndata_t* dc);
void R_Draw2sMultiPatchColumn_8(drawcolumndata_t* dc);
void R_Draw2sMultiPatchTranslucentColumn_8(drawcolumndata_t* dc);
void R_DrawFogColumn_8(drawcolumndata_t* dc);
void R_DrawColumnShadowed_8(drawcolumndata_t* dc);

#define PLANELIGHTFLOAT (BASEVIDWIDTH * BASEVIDWIDTH / vid.width / ds->zeroheight / 21.0f * FIXED_TO_FLOAT(fovtan[viewssnum]))

void R_DrawSpan_8(drawspandata_t* ds);
void R_DrawTranslucentSpan_8(drawspandata_t* ds);
void R_DrawTiltedSpan_8(drawspandata_t* ds);
void R_DrawTiltedTranslucentSpan_8(drawspandata_t* ds);

void R_DrawSplat_8(drawspandata_t* ds);
void R_DrawTranslucentSplat_8(drawspandata_t* ds);
void R_DrawTiltedSplat_8(drawspandata_t* ds);

void R_DrawFloorSprite_8(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_8(drawspandata_t* ds);
void R_DrawTiltedFloorSprite_8(drawspandata_t* ds);
void R_DrawTiltedTranslucentFloorSprite_8(drawspandata_t* ds);

void R_CalcTiltedLighting(INT32 *lightbuffer, INT32 x1, INT32 x2, fixed_t start, fixed_t end);

void R_DrawTranslucentWaterSpan_8(drawspandata_t* ds);
void R_DrawTiltedTranslucentWaterSpan_8(drawspandata_t* ds);

void R_DrawFogSpan_8(drawspandata_t* ds);

// Lactozilla: Non-powers-of-two
void R_DrawSpan_NPO2_8(drawspandata_t* ds);
void R_DrawTranslucentSpan_NPO2_8(drawspandata_t* ds);
void R_DrawTiltedSpan_NPO2_8(drawspandata_t* ds);
void R_DrawTiltedTranslucentSpan_NPO2_8(drawspandata_t* ds);

void R_DrawSplat_NPO2_8(drawspandata_t* ds);
void R_DrawTranslucentSplat_NPO2_8(drawspandata_t* ds);
void R_DrawTiltedSplat_NPO2_8(drawspandata_t* ds);

void R_DrawFloorSprite_NPO2_8(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_NPO2_8(drawspandata_t* ds);
void R_DrawTiltedFloorSprite_NPO2_8(drawspandata_t* ds);
void R_DrawTiltedTranslucentFloorSprite_NPO2_8(drawspandata_t* ds);

void R_DrawTranslucentWaterSpan_NPO2_8(drawspandata_t* ds);
void R_DrawTiltedTranslucentWaterSpan_NPO2_8(drawspandata_t* ds);

// Debugging - highlight surfaces in flat colors
void R_DrawColumn_Flat_8(drawcolumndata_t* dc);
void R_DrawSpan_Flat_8(drawspandata_t* ds);
void R_DrawTiltedSpan_Flat_8(drawspandata_t* ds);

#ifdef USEASM
void ASMCALL R_DrawColumn_8_ASM(void);
void ASMCALL R_DrawShadeColumn_8_ASM(void);
void ASMCALL R_DrawTranslucentColumn_8_ASM(void);
void ASMCALL R_Draw2sMultiPatchColumn_8_ASM(void);

void ASMCALL R_DrawColumn_8_MMX(void);

void ASMCALL R_Draw2sMultiPatchColumn_8_MMX(void);
void ASMCALL R_DrawSpan_8_MMX(void);
#endif

// ------------------
// 16bpp DRAWING CODE
// ------------------

#ifdef HIGHCOLOR
void R_DrawColumn_16(void);
void R_DrawWallColumn_16(void);
void R_DrawTranslucentColumn_16(void);
void R_DrawTranslatedColumn_16(void);
void R_DrawSpan_16(void);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

// =========================================================================
#endif  // __R_DRAW__
