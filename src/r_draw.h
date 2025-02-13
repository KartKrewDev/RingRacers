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

#define BASEDRAWFUNC 0

typedef void (coldrawfunc_t)(drawcolumndata_t*);
typedef void (spandrawfunc_t)(drawspandata_t*);

enum
{
	COLDRAWFUNC_BASE = BASEDRAWFUNC,
	COLDRAWFUNC_FUZZY,
	COLDRAWFUNC_TRANS,
	COLDRAWFUNC_SHADOWED,
	COLDRAWFUNC_TRANSTRANS,
	COLDRAWFUNC_TWOSMULTIPATCH,
	COLDRAWFUNC_TWOSMULTIPATCHTRANS,
	COLDRAWFUNC_FOG,
	COLDRAWFUNC_DROPSHADOW,

	COLDRAWFUNC_MAX
};

extern int colfunctype;
extern coldrawfunc_t *colfunc;

extern coldrawfunc_t *colfuncs[COLDRAWFUNC_MAX];
extern coldrawfunc_t *colfuncs_bm[COLDRAWFUNC_MAX];

enum
{
	SPANDRAWFUNC_BASE = BASEDRAWFUNC,
	SPANDRAWFUNC_TRANS,
	SPANDRAWFUNC_TILTED,
	SPANDRAWFUNC_TILTEDTRANS,

	SPANDRAWFUNC_SPLAT,
	SPANDRAWFUNC_TRANSSPLAT,
	SPANDRAWFUNC_TILTEDSPLAT,
	SPANDRAWFUNC_TILTEDTRANSSPLAT,

	SPANDRAWFUNC_SPRITE,
	SPANDRAWFUNC_TRANSSPRITE,
	SPANDRAWFUNC_TILTEDSPRITE,
	SPANDRAWFUNC_TILTEDTRANSSPRITE,

	SPANDRAWFUNC_WATER,
	SPANDRAWFUNC_TILTEDWATER,

	SPANDRAWFUNC_FOG,
	SPANDRAWFUNC_TILTEDFOG,

	SPANDRAWFUNC_MAX
};

extern spandrawfunc_t *spanfunc;

extern spandrawfunc_t *spanfuncs[SPANDRAWFUNC_MAX];
extern spandrawfunc_t *spanfuncs_bm[SPANDRAWFUNC_MAX];
extern spandrawfunc_t *spanfuncs_npo2[SPANDRAWFUNC_MAX];
extern spandrawfunc_t *spanfuncs_bm_npo2[SPANDRAWFUNC_MAX];
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

void R_DrawColumn(drawcolumndata_t* dc);
void R_DrawTranslucentColumn(drawcolumndata_t* dc);
void R_DrawDropShadowColumn(drawcolumndata_t* dc);
void R_DrawTranslatedColumn(drawcolumndata_t* dc);
void R_DrawTranslatedTranslucentColumn(drawcolumndata_t* dc);
void R_Draw2sMultiPatchColumn(drawcolumndata_t* dc);
void R_Draw2sMultiPatchTranslucentColumn(drawcolumndata_t* dc);
void R_DrawFogColumn(drawcolumndata_t* dc);
void R_DrawColumnShadowed(drawcolumndata_t* dc);

void R_DrawColumn_Brightmap(drawcolumndata_t* dc);
void R_DrawTranslucentColumn_Brightmap(drawcolumndata_t* dc);
void R_DrawTranslatedColumn_Brightmap(drawcolumndata_t* dc);
void R_DrawTranslatedTranslucentColumn_Brightmap(drawcolumndata_t* dc);
void R_Draw2sMultiPatchColumn_Brightmap(drawcolumndata_t* dc);
void R_Draw2sMultiPatchTranslucentColumn_Brightmap(drawcolumndata_t* dc);
void R_DrawColumnShadowed_Brightmap(drawcolumndata_t* dc);

void R_DrawSpan(drawspandata_t* ds);
void R_DrawTranslucentSpan(drawspandata_t* ds);
void R_DrawSplat(drawspandata_t* ds);
void R_DrawTranslucentSplat(drawspandata_t* ds);
void R_DrawFloorSprite(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan(drawspandata_t* ds);
void R_DrawFogSpan(drawspandata_t* ds);

void R_DrawSpan_Tilted(drawspandata_t* ds);
void R_DrawTranslucentSpan_Tilted(drawspandata_t* ds);
void R_DrawSplat_Tilted(drawspandata_t* ds);
void R_DrawTranslucentSplat_Tilted(drawspandata_t* ds);
void R_DrawFloorSprite_Tilted(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_Tilted(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_Tilted(drawspandata_t* ds);
void R_DrawFogSpan_Tilted(drawspandata_t* ds);

void R_DrawSpan_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSpan_NPO2(drawspandata_t* ds);
void R_DrawSplat_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSplat_NPO2(drawspandata_t* ds);
void R_DrawFloorSprite_NPO2(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_NPO2(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_NPO2(drawspandata_t* ds);

void R_DrawSpan_Tilted_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSpan_Tilted_NPO2(drawspandata_t* ds);
void R_DrawSplat_Tilted_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSplat_Tilted_NPO2(drawspandata_t* ds);
void R_DrawFloorSprite_Tilted_NPO2(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_Tilted_NPO2(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_Tilted_NPO2(drawspandata_t* ds);

void R_DrawSpan_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentSpan_Brightmap(drawspandata_t* ds);
void R_DrawSplat_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentSplat_Brightmap(drawspandata_t* ds);
void R_DrawFloorSprite_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_Brightmap(drawspandata_t* ds);

void R_DrawSpan_Tilted_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentSpan_Tilted_Brightmap(drawspandata_t* ds);
void R_DrawSplat_Tilted_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentSplat_Tilted_Brightmap(drawspandata_t* ds);
void R_DrawFloorSprite_Tilted_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_Tilted_Brightmap(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_Tilted_Brightmap(drawspandata_t* ds);

void R_DrawSpan_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSpan_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawSplat_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSplat_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawFloorSprite_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_Brightmap_NPO2(drawspandata_t* ds);

void R_DrawSpan_Tilted_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSpan_Tilted_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawSplat_Tilted_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentSplat_Tilted_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawFloorSprite_Tilted_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentFloorSprite_Tilted_Brightmap_NPO2(drawspandata_t* ds);
void R_DrawTranslucentWaterSpan_Tilted_Brightmap_NPO2(drawspandata_t* ds);

// Debugging - highlight surfaces in flat colors
void R_DrawColumn_Flat(drawcolumndata_t* dc);
void R_DrawSpan_Flat(drawspandata_t* ds);
void R_DrawTiltedSpan_Flat(drawspandata_t* ds);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

// =========================================================================
#endif  // __R_DRAW__
