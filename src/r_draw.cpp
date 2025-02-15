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
/// \file  r_draw.cpp
/// \brief span / column drawer functions, for 8bpp and 16bpp
///        All drawing to the view buffer is accomplished in this file.
///        The other refresh files only know about ccordinates,
///        not the architecture of the frame buffer.
///        The frame buffer is a linear one, and we need only the base address.

#include <algorithm>

#include "doomdef.h"
#include "doomstat.h"
#include "r_local.h"
#include "st_stuff.h" // need ST_HEIGHT
#include "i_video.h"
#include "v_video.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"
#include "console.h" // Until buffering gets finished
#include "k_color.h" // SRB2kart
#include "i_threads.h"
#include "libdivide.h" // used by NPO2 tilted span functions

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include <tracy/tracy/Tracy.hpp>

// --------------------------------------------
// assembly or c drawer routines for 8bpp/16bpp
// --------------------------------------------
coldrawfunc_t *colfunc;

coldrawfunc_t *colfuncs[COLDRAWFUNC_MAX];
coldrawfunc_t *colfuncs_bm[COLDRAWFUNC_MAX];

int colfunctype;

spandrawfunc_t *spanfunc;

spandrawfunc_t *spanfuncs[SPANDRAWFUNC_MAX];
spandrawfunc_t *spanfuncs_bm[SPANDRAWFUNC_MAX];
spandrawfunc_t *spanfuncs_npo2[SPANDRAWFUNC_MAX];
spandrawfunc_t *spanfuncs_bm_npo2[SPANDRAWFUNC_MAX];
spandrawfunc_t *spanfuncs_flat[SPANDRAWFUNC_MAX];

drawcolumndata_t g_dc;
drawspandata_t g_ds;


// ==========================================================================
//                     COMMON DATA FOR 8bpp AND 16bpp
// ==========================================================================

/**	\brief view info
*/
INT32 viewwidth, scaledviewwidth, viewheight, viewwindowx, viewwindowy;

/**	\brief pointer to the start of each line of the screen,
*/
UINT8 *ylookup[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view1 (splitscreen)
*/
UINT8 *ylookup1[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view2 (splitscreen)
*/
UINT8 *ylookup2[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view3 (splitscreen)
*/
UINT8 *ylookup3[MAXVIDHEIGHT*4];

/**	\brief pointer to the start of each line of the screen, for view4 (splitscreen)
*/
UINT8 *ylookup4[MAXVIDHEIGHT*4];

/**	\brief  x byte offset for columns inside the viewwindow,
	so the first column starts at (SCRWIDTH - VIEWWIDTH)/2
*/
INT32 columnofs[MAXVIDWIDTH*4];

UINT8 *topleft;

UINT8 r8_flatcolor;

// =========================================================================
//                      COLUMN DRAWING CODE STUFF
// =========================================================================

// -----------------------
// translucency stuff here
// -----------------------
#define NUMTRANSTABLES 11 // how many translucency tables are used

UINT8 *transtables; // translucency tables
UINT8 *blendtables[NUMBLENDMAPS];

/**	\brief R_DrawTransColumn uses this
*/
UINT8 *dc_transmap; // one of the translucency tables

// ----------------------
// translation stuff here
// ----------------------


/**	\brief R_DrawTranslatedColumn uses this
*/
UINT8 *dc_translation;

struct r_lightlist_t *dc_lightlist = NULL;
INT32 dc_numlights = 0, dc_maxlights;

// =========================================================================
//                      SPAN DRAWING CODE STUFF
// =========================================================================

// Vectors for Software's tilted slope drawers
floatv3_t *ds_su, *ds_sv, *ds_sz;
float focallengthf[MAXSPLITSCREENPLAYERS];
float zeroheight;

// =========================================================================
//                   TRANSLATION COLORMAP CODE
// =========================================================================

#define DEFAULT_TT_CACHE_INDEX MAXSKINS
#define BOSS_TT_CACHE_INDEX (MAXSKINS + 1)
#define METALSONIC_TT_CACHE_INDEX (MAXSKINS + 2)
#define ALLWHITE_TT_CACHE_INDEX (MAXSKINS + 3)
#define RAINBOW_TT_CACHE_INDEX (MAXSKINS + 4)
#define BLINK_TT_CACHE_INDEX (MAXSKINS + 5)
#define DASHMODE_TT_CACHE_INDEX (MAXSKINS + 6)
#define HITLAG_TT_CACHE_INDEX (MAXSKINS + 7)
#define INTERMISSION_TT_CACHE_INDEX (MAXSKINS + 8)
#define TT_CACHE_SIZE (MAXSKINS + 9)

#define SKIN_RAMP_LENGTH 16
#define DEFAULT_STARTTRANSCOLOR 96
#define NUM_PALETTE_ENTRIES 256

static UINT8 **translationtablecache[TT_CACHE_SIZE] = {NULL};
UINT8 skincolor_modified[MAXSKINCOLORS];

static INT32 SkinToCacheIndex(INT32 skinnum)
{
	switch (skinnum)
	{
		case TC_DEFAULT:    return DEFAULT_TT_CACHE_INDEX;
		case TC_BOSS:       return BOSS_TT_CACHE_INDEX;
		case TC_METALSONIC: return METALSONIC_TT_CACHE_INDEX;
		case TC_ALLWHITE:   return ALLWHITE_TT_CACHE_INDEX;
		case TC_RAINBOW:    return RAINBOW_TT_CACHE_INDEX;
		case TC_BLINK:      return BLINK_TT_CACHE_INDEX;
		case TC_DASHMODE:   return DASHMODE_TT_CACHE_INDEX;
		case TC_HITLAG:     return HITLAG_TT_CACHE_INDEX;
		case TC_INTERMISSION: return INTERMISSION_TT_CACHE_INDEX;
		     default:       break;
	}

	return skinnum;
}

static INT32 CacheIndexToSkin(INT32 ttc)
{
	switch (ttc)
	{
		case DEFAULT_TT_CACHE_INDEX:    return TC_DEFAULT;
		case BOSS_TT_CACHE_INDEX:       return TC_BOSS;
		case METALSONIC_TT_CACHE_INDEX: return TC_METALSONIC;
		case ALLWHITE_TT_CACHE_INDEX:   return TC_ALLWHITE;
		case RAINBOW_TT_CACHE_INDEX:    return TC_RAINBOW;
		case BLINK_TT_CACHE_INDEX:      return TC_BLINK;
		case DASHMODE_TT_CACHE_INDEX:   return TC_DASHMODE;
		case HITLAG_TT_CACHE_INDEX:     return TC_HITLAG;
		case INTERMISSION_TT_CACHE_INDEX: return TC_INTERMISSION;
		     default:                   break;
	}

	return ttc;
}

CV_PossibleValue_t Color_cons_t[MAXSKINCOLORS+1];

#define TRANSTAB_AMTMUL10 (255.0f / 10.0f)

struct GenerateBlendTables_State
{
	RGBA_t *masterPalette;
	RGBA_t *gammaCorrectedPalette;
};

static void R_GenerateBlendTables_Core(struct GenerateBlendTables_State *state);
static void R_GenerateTranslucencyTable(UINT8 *table, RGBA_t* sourcepal, int style, UINT8 blendamt);

static void R_AllocateBlendTables(void)
{
	INT32 i;

	for (i = 0; i < NUMBLENDMAPS; i++)
	{
		if (i == blendtab_modulate)
			continue;
		blendtables[i] = static_cast<UINT8 *>(Z_MallocAlign((NUMTRANSTABLES + 1) * 0x10000, PU_STATIC, NULL, 16));
	}

	// Modulation blending only requires a single table
	blendtables[blendtab_modulate] = static_cast<UINT8 *>(Z_MallocAlign(0x10000, PU_STATIC, NULL, 16));
}

#ifdef HAVE_THREADS
static void R_GenerateBlendTables_Thread(void *userdata)
{
	struct GenerateBlendTables_State *state = static_cast<struct GenerateBlendTables_State *>(userdata);

	R_GenerateBlendTables_Core(state);

	free(state->masterPalette);
	free(state->gammaCorrectedPalette);
	free(state);
}
#endif

/** \brief Initializes the translucency tables used by the Software renderer.
*/
void R_InitTranslucencyTables(void)
{
	// Load here the transparency lookup tables 'TINTTAB'
	// NOTE: the TINTTAB resource MUST BE aligned on 64k for the asm
	// optimised code (in other words, transtables pointer low word is 0)
	transtables = static_cast<UINT8 *>(Z_MallocAlign(NUMTRANSTABLES*0x10000, PU_STATIC, NULL, 16));

	W_ReadLump(W_GetNumForName("TRANS10"), transtables);
	W_ReadLump(W_GetNumForName("TRANS20"), transtables+0x10000);
	W_ReadLump(W_GetNumForName("TRANS30"), transtables+0x20000);
	W_ReadLump(W_GetNumForName("TRANS40"), transtables+0x30000);
	W_ReadLump(W_GetNumForName("TRANS50"), transtables+0x40000);
	W_ReadLump(W_GetNumForName("TRANS60"), transtables+0x50000);
	W_ReadLump(W_GetNumForName("TRANS70"), transtables+0x60000);
	W_ReadLump(W_GetNumForName("TRANS80"), transtables+0x70000);
	W_ReadLump(W_GetNumForName("TRANS90"), transtables+0x80000);

	R_AllocateBlendTables();
	R_GenerateBlendTables();
}

void R_GenerateBlendTables(void)
{
#ifdef HAVE_THREADS
	// Allocate copies for the worker thread since the originals can be freed in the main thread.
	struct GenerateBlendTables_State *state = static_cast<struct GenerateBlendTables_State *>(malloc(sizeof *state));
	size_t palsize = 256 * sizeof(RGBA_t);

	state->masterPalette = static_cast<RGBA_t *>(memcpy(malloc(palsize), pMasterPalette, palsize));
	state->gammaCorrectedPalette = static_cast<RGBA_t *>(memcpy(malloc(palsize), pGammaCorrectedPalette, palsize));

	I_spawn_thread("blend-tables",
			R_GenerateBlendTables_Thread, state);
#else
	struct GenerateBlendTables_State state = {pMasterPalette, pGammaCorrectedPalette};

	R_GenerateBlendTables_Core(&state);
#endif
}

static void R_GenerateBlendTables_Core(struct GenerateBlendTables_State *state)
{
	INT32 i;

	for (i = 0; i <= 9; i++)
	{
		const size_t offs = (0x10000 * i);
		const UINT8 alpha = (TRANSTAB_AMTMUL10 * ((float)(10-i)));

		R_GenerateTranslucencyTable(blendtables[blendtab_add] + offs, state->gammaCorrectedPalette, AST_ADD, alpha);
		R_GenerateTranslucencyTable(blendtables[blendtab_subtract] + offs, state->masterPalette, AST_SUBTRACT, alpha); // intentionally uses pMasterPalette
		R_GenerateTranslucencyTable(blendtables[blendtab_reversesubtract] + offs, state->gammaCorrectedPalette, AST_REVERSESUBTRACT, alpha);
	}

	R_GenerateTranslucencyTable(blendtables[blendtab_modulate], state->gammaCorrectedPalette, AST_MODULATE, 0);
}

void R_GenerateTranslucencyTable(UINT8 *table, RGBA_t* sourcepal, int style, UINT8 blendamt)
{
	INT16 bg, fg;
	RGBA_t backrgba, frontrgba, result;

	if (table == NULL)
		I_Error("R_GenerateTranslucencyTable: input table was NULL!");

	for (bg = 0; bg <= 0xFF; bg++)
	{
		backrgba = sourcepal[bg];
		for (fg = 0; fg <= 0xFF; fg++)
		{
			frontrgba = sourcepal[fg];

			result.rgba = ASTBlendPixel(backrgba, frontrgba, style, blendamt);
			table[((fg * 0x100) + bg)] = NearestPaletteColor(result.s.red, result.s.green, result.s.blue, sourcepal);
		}
	}
}

#define ClipTransLevel(trans) std::clamp<INT32>(trans, 0, NUMTRANSMAPS-2)

UINT8 *R_GetTranslucencyTable(INT32 alphalevel)
{
	return transtables + (ClipTransLevel(alphalevel-1) << FF_TRANSSHIFT);
}

UINT8 *R_GetBlendTable(int style, INT32 alphalevel)
{
	size_t offs = (ClipTransLevel(alphalevel) << FF_TRANSSHIFT);

	// Lactozilla: Returns the equivalent to AST_TRANSLUCENT
	// if no alpha style matches any of the blend tables.
	switch (style)
	{
		case AST_ADD:
			return blendtables[blendtab_add] + offs;
		case AST_SUBTRACT:
			return blendtables[blendtab_subtract] + offs;
		case AST_REVERSESUBTRACT:
			return blendtables[blendtab_reversesubtract] + offs;
		case AST_MODULATE:
			return blendtables[blendtab_modulate];
		default:
			break;
	}

	// Return a normal translucency table
	if (--alphalevel < 0)
		return NULL;
	return transtables + (ClipTransLevel(alphalevel) << FF_TRANSSHIFT);
}

/**	\brief	Retrieves a translation colormap from the cache.

	\param	skinnum	number of skin, TC_DEFAULT or TC_BOSS
	\param	color	translation color
	\param	flags	set GTC_CACHE to use the cache

	\return	Colormap. If not cached, caller should Z_Free.
*/
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags)
{
	UINT8* ret;
	INT32 skintableindex = SkinToCacheIndex(skinnum); // Adjust if we want the default colormap
	INT32 i;

	if (flags & GTC_CACHE)
	{
		// Allocate table for skin if necessary
		if (!translationtablecache[skintableindex])
			translationtablecache[skintableindex] = static_cast<UINT8 **>(Z_Calloc(MAXSKINCOLORS * sizeof(UINT8**), PU_STATIC, NULL));

		// Get colormap
		ret = translationtablecache[skintableindex][color];

		// Rebuild the cache if necessary
		if (skincolor_modified[color])
		{
			for (i = 0; i < (INT32)(sizeof(translationtablecache) / sizeof(translationtablecache[0])); i++)
				if (translationtablecache[i] && translationtablecache[i][color])
					K_GenerateKartColormap(translationtablecache[i][color], CacheIndexToSkin(i), color);
			skincolor_modified[color] = false;
		}
	}
	else ret = NULL;

	// Generate the colormap if necessary
	if (!ret)
	{
		ret = static_cast<UINT8 *>(Z_MallocAlign(NUM_PALETTE_ENTRIES, (flags & GTC_CACHE) ? PU_LEVEL : PU_STATIC, NULL, 8));
		K_GenerateKartColormap(ret, skinnum, color); //R_GenerateTranslationColormap(ret, skinnum, color); // SRB2kart

		// Cache the colormap if desired
		if (flags & GTC_CACHE)
			translationtablecache[skintableindex][color] = ret;
	}

	return ret;
}

/**	\brief	Flushes cache of translation colormaps.

	Flushes cache of translation colormaps, but doesn't actually free the
	colormaps themselves. These are freed when PU_LEVEL blocks are purged,
	at or before which point, this function should be called.

	\return	void
*/
void R_FlushTranslationColormapCache(void)
{
	INT32 i;

	for (i = 0; i < (INT32)(sizeof(translationtablecache) / sizeof(translationtablecache[0])); i++)
		if (translationtablecache[i])
			memset(translationtablecache[i], 0, MAXSKINCOLORS * sizeof(UINT8**));
}

UINT16 R_GetColorByName(const char *name)
{
	UINT16 color = (UINT16)atoi(name);
	if (color > 0 && color < numskincolors)
		return color;
	for (color = 1; color < numskincolors; color++)
		if (!stricmp(skincolors[color].name, name))
			return color;
	return SKINCOLOR_NONE;
}

UINT16 R_GetSuperColorByName(const char *name)
{
	UINT16 i, color = SKINCOLOR_NONE;
	char *realname = static_cast<char *>(Z_Malloc(MAXCOLORNAME+1, PU_STATIC, NULL));
	snprintf(realname, MAXCOLORNAME+1, "Super %s 1", name);
	for (i = 1; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name, realname)) {
			color = i;
			break;
		}
	Z_Free(realname);
	return color;
}

// ==========================================================================
//               COMMON DRAWER FOR 8 AND 16 BIT COLOR MODES
// ==========================================================================

// in a perfect world, all routines would be compatible for either mode,
// and optimised enough
//
// in reality, the few routines that can work for either mode, are
// put here

/**	\brief	The R_InitViewBuffer function

	Creates lookup tables for getting the framebuffer address
	of a pixel to draw.

	\param	width	witdh of buffer
	\param	height	hieght of buffer

	\return	void


*/

void R_InitViewBuffer(INT32 width, INT32 height)
{
	INT32 i, bytesperpixel = vid.bpp;

	if (width > MAXVIDWIDTH)
		width = MAXVIDWIDTH;
	if (height > MAXVIDHEIGHT)
		height = MAXVIDHEIGHT;
	if (bytesperpixel < 1 || bytesperpixel > 4)
		I_Error("R_InitViewBuffer: wrong bytesperpixel value %d\n", bytesperpixel);

	viewwindowx = 0;
	viewwindowy = 0;

	// Column offset for those columns of the view window, but relative to the entire screen
	for (i = 0; i < width; i++)
		columnofs[i] = (viewwindowx + i) * bytesperpixel;

	// Precalculate all row offsets.
	for (i = 0; i < height; i++)
	{
		ylookup[i] = ylookup1[i] = screens[0] + i*vid.width*bytesperpixel;
		if (r_splitscreen == 1)
			ylookup2[i] = screens[0] + (i+viewheight)*vid.width*bytesperpixel;
		else
			ylookup2[i] = screens[0] + i*vid.width*bytesperpixel + (viewwidth*bytesperpixel);
		ylookup3[i] = screens[0] + (i+viewheight)*vid.width*bytesperpixel;
		ylookup4[i] = screens[0] + (i+viewheight)*vid.width*bytesperpixel + (viewwidth*bytesperpixel);
	}
}

/**	\brief viewborder patches lump numbers
*/
lumpnum_t viewborderlump[8];

/**	\brief Store the lumpnumber of the viewborder patches
*/

void R_InitViewBorder(void)
{
	viewborderlump[BRDR_T] = W_GetNumForName("brdr_t");
	viewborderlump[BRDR_B] = W_GetNumForName("brdr_b");
	viewborderlump[BRDR_L] = W_GetNumForName("brdr_l");
	viewborderlump[BRDR_R] = W_GetNumForName("brdr_r");
	viewborderlump[BRDR_TL] = W_GetNumForName("brdr_tl");
	viewborderlump[BRDR_BL] = W_GetNumForName("brdr_bl");
	viewborderlump[BRDR_TR] = W_GetNumForName("brdr_tr");
	viewborderlump[BRDR_BR] = W_GetNumForName("brdr_br");
}

#if 0
/**	\brief R_FillBackScreen

	Fills the back screen with a pattern for variable screen sizes
	Also draws a beveled edge.
*/
void R_FillBackScreen(void)
{
	UINT8 *src, *dest;
	patch_t *patch;
	INT32 x, y, step, boff;

	// quickfix, don't cache lumps in both modes
	if (rendermode == render_opengl)
		return;

	// draw pattern around the status bar too (when hires),
	// so return only when in full-screen without status bar.
	if (scaledviewwidth == vid.width && viewheight == vid.height)
		return;

	src = scr_borderpatch;
	dest = screens[1];

	for (y = 0; y < vid.height; y++)
	{
		for (x = 0; x < vid.width/128; x++)
		{
			M_Memcpy (dest, src+((y&127)<<7), 128);
			dest += 128;
		}

		if (vid.width&127)
		{
			M_Memcpy(dest, src+((y&127)<<7), vid.width&127);
			dest += (vid.width&127);
		}
	}

	// don't draw the borders when viewwidth is full vid.width.
	if (scaledviewwidth == vid.width)
		return;

	step = 8;
	boff = 8;

	patch = W_CacheLumpNum(viewborderlump[BRDR_T], PU_CACHE);
	for (x = 0; x < scaledviewwidth; x += step)
		V_DrawPatch(viewwindowx + x, viewwindowy - boff, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_B], PU_CACHE);
	for (x = 0; x < scaledviewwidth; x += step)
		V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_L], PU_CACHE);
	for (y = 0; y < viewheight; y += step)
		V_DrawPatch(viewwindowx - boff, viewwindowy + y, 1, patch);

	patch = W_CacheLumpNum(viewborderlump[BRDR_R],PU_CACHE);
	for (y = 0; y < viewheight; y += step)
		V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, 1,
			patch);

	// Draw beveled corners.
	V_DrawPatch(viewwindowx - boff, viewwindowy - boff, 1,
		W_CacheLumpNum(viewborderlump[BRDR_TL], PU_CACHE));
	V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - boff, 1,
		W_CacheLumpNum(viewborderlump[BRDR_TR], PU_CACHE));
	V_DrawPatch(viewwindowx - boff, viewwindowy + viewheight, 1,
		W_CacheLumpNum(viewborderlump[BRDR_BL], PU_CACHE));
	V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, 1,
		W_CacheLumpNum(viewborderlump[BRDR_BR], PU_CACHE));
}
#endif

/**	\brief	The R_VideoErase function

	Copy a screen buffer.

	\param	ofs	offest from buffer
	\param	count	bytes to erase

	\return	void


*/
void R_VideoErase(size_t ofs, INT32 count)
{
	// LFB copy.
	// This might not be a good idea if memcpy
	//  is not optimal, e.g. byte by byte on
	//  a 32bit CPU, as GNU GCC/Linux libc did
	//  at one point.
	M_Memcpy(screens[0] + ofs, screens[1] + ofs, count);
}

#if 0
/**	\brief The R_DrawViewBorder

  Draws the border around the view
	for different size windows?
*/
void R_DrawViewBorder(void)
{
	INT32 top, side, ofs;

	if (rendermode == render_none)
		return;
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawViewBorder(0);
		return;
	}
	else
#endif

#ifdef DEBUG
	fprintf(stderr,"RDVB: vidwidth %d vidheight %d scaledviewwidth %d viewheight %d\n",
		vid.width, vid.height, scaledviewwidth, viewheight);
#endif

	if (scaledviewwidth == vid.width)
		return;

	top = (vid.height - viewheight)>>1;
	side = (vid.width - scaledviewwidth)>>1;

	// copy top and one line of left side
	R_VideoErase(0, top*vid.width+side);

	// copy one line of right side and bottom
	ofs = (viewheight+top)*vid.width - side;
	R_VideoErase(ofs, top*vid.width + side);

	// copy sides using wraparound
	ofs = top*vid.width + vid.width-side;
	side <<= 1;

    // simpler using our VID_Blit routine
	VID_BlitLinearScreen(screens[1] + ofs, screens[0] + ofs, side, viewheight - 1,
		vid.width, vid.width);
}
#endif

// ==========================================================================
//                   INCLUDE MAIN DRAWERS CODE HERE
// ==========================================================================

#include "r_draw_column.cpp"
#include "r_draw_span.cpp"
