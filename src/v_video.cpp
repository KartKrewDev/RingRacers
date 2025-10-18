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
/// \file  v_video.c
/// \brief Gamma correction LUT stuff
///        Functions to draw patches (by post) directly to screen.
///        Functions to blit a block to the screen.

#include <cmath>
#include <optional>

#include <tracy/tracy/Tracy.hpp>

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h" // stplyr
#include "g_game.h" // players
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "f_finale.h"
#include "r_draw.h"
#include "console.h"
#include "r_fps.h"
#include "k_dialogue.h" // K_GetDialogueSlide

#include "i_video.h" // rendermode
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "doomstat.h"
#include "hwr2/blendmode.hpp"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

// SRB2Kart
#include "k_hud.h"
#include "k_boss.h"
#include "i_time.h"
#include "v_draw.hpp"

using namespace srb2;

// Each screen is [vid.width*vid.height];
UINT8 *screens[5];
// screens[0] = main display window
// screens[1] = back screen, alternative blitting
// screens[2] = screenshot buffer, gif movie buffer
// screens[3] = fade screen start
// screens[4] = fade screen end, postimage tempoarary buffer

#define huecoloursteps 4

extern "C" CV_PossibleValue_t hue_cons_t[];
CV_PossibleValue_t hue_cons_t[] = {{0, "MIN"}, {(huecoloursteps*6)-1, "MAX"}, {0, NULL}};

extern "C" CV_PossibleValue_t constextsize_cons_t[];
CV_PossibleValue_t constextsize_cons_t[] = {
	{V_NOSCALEPATCH, "Small"}, {V_SMALLSCALEPATCH, "Medium"}, {V_MEDSCALEPATCH, "Large"}, {0, "Huge"},
	{0, NULL}};

// local copy of the palette for V_GetColor()
RGBA_t *pLocalPalette = NULL;
RGBA_t *pMasterPalette = NULL;
RGBA_t *pGammaCorrectedPalette = NULL;

hwr2::Twodee srb2::g_2d;

static size_t currentPaletteSize;

static UINT8 softwaretranstohwr[11]    = {  0, 25, 51, 76,102,127,153,178,204,229,255};

/*
The following was an extremely helpful resource when developing my Colour Cube LUT.
http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter24.html
Please check it out if you're trying to maintain this.
toast 18/04/17
*/

float Cubepal[2][2][2][3];
boolean Cubeapply = false;

// returns whether to apply cube, selectively avoiding expensive operations
static boolean InitCube(void)
{
	boolean apply = false;
	UINT8 q;
	float working[2][2][2][3] = // the initial positions of the corners of the colour cube!
	{
		{
			{
				{0.0, 0.0, 0.0}, // black corner
				{0.0, 0.0, 1.0}  // blue corner
			},
			{
				{0.0, 1.0, 0.0}, // green corner
				{0.0, 1.0, 1.0}  // cyan corner
			}
		},
		{
			{
				{1.0, 0.0, 0.0}, // red corner
				{1.0, 0.0, 1.0}  // magenta corner
			},
			{
				{1.0, 1.0, 0.0}, // yellow corner
				{1.0, 1.0, 1.0}  // white corner
			}
		}
	};
	float desatur[3]; // grey
	float globalgammamul, globalgammaoffs;
	boolean doinggamma;

	if (con_startup_loadprogress < LOADED_CONFIG)
		return false;

#define diffcons(cv) (cv.value != atoi(cv.defaultvalue))

	doinggamma = diffcons(cv_globalgamma);

#define gammascale 8
	globalgammamul = (cv_globalgamma.value ? ((255 - (gammascale*abs(cv_globalgamma.value)))/255.0) : 1.0);
	globalgammaoffs = ((cv_globalgamma.value > 0) ? ((gammascale*cv_globalgamma.value)/255.0) : 0.0);
	desatur[0] = desatur[1] = desatur[2] = globalgammaoffs + (0.33*globalgammamul);

	if (doinggamma
		|| diffcons(cv_rhue)
		|| diffcons(cv_yhue)
		|| diffcons(cv_ghue)
		|| diffcons(cv_chue)
		|| diffcons(cv_bhue)
		|| diffcons(cv_mhue)
		|| diffcons(cv_rgamma)
		|| diffcons(cv_ygamma)
		|| diffcons(cv_ggamma)
		|| diffcons(cv_cgamma)
		|| diffcons(cv_bgamma)
		|| diffcons(cv_mgamma)) // set the gamma'd/hued positions (saturation is done later)
	{
		float mod, tempgammamul, tempgammaoffs;

		apply = true;

		working[0][0][0][0] = working[0][0][0][1] = working[0][0][0][2] = globalgammaoffs;
		working[1][1][1][0] = working[1][1][1][1] = working[1][1][1][2] = globalgammaoffs+globalgammamul;

#define dohue(hue, gamma, loc) \
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		mod = ((hue % huecoloursteps)*(tempgammamul)/huecoloursteps);\
		switch (hue/huecoloursteps)\
		{\
			case 0:\
			default:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs+mod;\
				loc[2] = tempgammaoffs;\
				break;\
			case 1:\
				loc[0] = tempgammaoffs+tempgammamul-mod;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs;\
				break;\
			case 2:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul;\
				loc[2] = tempgammaoffs+mod;\
				break;\
			case 3:\
				loc[0] = tempgammaoffs;\
				loc[1] = tempgammaoffs+tempgammamul-mod;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 4:\
				loc[0] = tempgammaoffs+mod;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul;\
				break;\
			case 5:\
				loc[0] = tempgammaoffs+tempgammamul;\
				loc[1] = tempgammaoffs;\
				loc[2] = tempgammaoffs+tempgammamul-mod;\
				break;\
		}
		dohue(cv_rhue.value, cv_rgamma.value, working[1][0][0]);
		dohue(cv_yhue.value, cv_ygamma.value, working[1][1][0]);
		dohue(cv_ghue.value, cv_ggamma.value, working[0][1][0]);
		dohue(cv_chue.value, cv_cgamma.value, working[0][1][1]);
		dohue(cv_bhue.value, cv_bgamma.value, working[0][0][1]);
		dohue(cv_mhue.value, cv_mgamma.value, working[1][0][1]);
#undef dohue
	}

#define dosaturation(a, e) a = ((1 - work)*e + work*a)
#define docvsat(cv_sat, hue, gamma, r, g, b) \
	if diffcons(cv_sat)\
	{\
		float work, mod, tempgammamul, tempgammaoffs;\
		apply = true;\
		work = (cv_sat.value/10.0);\
		mod = ((hue % huecoloursteps)*(1.0)/huecoloursteps);\
		if (hue & huecoloursteps)\
			mod = 2-mod;\
		else\
			mod += 1;\
		tempgammamul = (gamma ? ((255 - (gammascale*abs(gamma)))/255.0)*globalgammamul : globalgammamul);\
		tempgammaoffs = ((gamma > 0) ? ((gammascale*gamma)/255.0) + globalgammaoffs : globalgammaoffs);\
		for (q = 0; q < 3; q++)\
			dosaturation(working[r][g][b][q], (tempgammaoffs+(desatur[q]*mod*tempgammamul)));\
	}

	docvsat(cv_rsaturation, cv_rhue.value, cv_rgamma.value, 1, 0, 0);
	docvsat(cv_ysaturation, cv_yhue.value, cv_ygamma.value, 1, 1, 0);
	docvsat(cv_gsaturation, cv_ghue.value, cv_ggamma.value, 0, 1, 0);
	docvsat(cv_csaturation, cv_chue.value, cv_cgamma.value, 0, 1, 1);
	docvsat(cv_bsaturation, cv_bhue.value, cv_bgamma.value, 0, 0, 1);
	docvsat(cv_msaturation, cv_mhue.value, cv_mgamma.value, 1, 0, 1);

#undef gammascale

	if diffcons(cv_globalsaturation)
	{
		float work = (cv_globalsaturation.value/10.0);

		apply = true;

		for (q = 0; q < 3; q++)
		{
			dosaturation(working[1][0][0][q], desatur[q]);
			dosaturation(working[0][1][0][q], desatur[q]);
			dosaturation(working[0][0][1][q], desatur[q]);

			dosaturation(working[1][1][0][q], 2*desatur[q]);
			dosaturation(working[0][1][1][q], 2*desatur[q]);
			dosaturation(working[1][0][1][q], 2*desatur[q]);
		}
	}

#undef dosaturation

#undef diffcons

	if (!apply)
		return false;

#define dowork(i, j, k, l) \
	if (working[i][j][k][l] > 1.0)\
		working[i][j][k][l] = 1.0;\
	else if (working[i][j][k][l] < 0.0)\
		working[i][j][k][l] = 0.0;\
	Cubepal[i][j][k][l] = working[i][j][k][l]
	for (q = 0; q < 3; q++)
	{
		dowork(0, 0, 0, q);
		dowork(1, 0, 0, q);
		dowork(0, 1, 0, q);
		dowork(1, 1, 0, q);
		dowork(0, 0, 1, q);
		dowork(1, 0, 1, q);
		dowork(0, 1, 1, q);
		dowork(1, 1, 1, q);
	}
#undef dowork

	return true;
}

UINT32 V_GammaCorrect(UINT32 input, double power)
{
	RGBA_t result;
	double linear;

	result.rgba = input;

	linear = ((double)result.s.red)/255.0f;
	linear = pow(linear, power)*255.0f;
	result.s.red = (UINT8)(linear);
	linear = ((double)result.s.green)/255.0f;
	linear = pow(linear, power)*255.0f;
	result.s.green = (UINT8)(linear);
	linear = ((double)result.s.blue)/255.0f;
	linear = pow(linear, power)*255.0f;
	result.s.blue = (UINT8)(linear);

	return result.rgba;
}

// keep a copy of the palette so that we can get the RGB value for a color index at any time.
static void LoadPalette(const char *lumpname)
{
	lumpnum_t lumpnum = W_GetNumForName(lumpname);
	size_t i, palsize;
	UINT8 *pal;

	currentPaletteSize = W_LumpLength(lumpnum);
	palsize = currentPaletteSize / 3;

	Cubeapply = InitCube();

	if (pLocalPalette != pMasterPalette)
		Z_Free(pLocalPalette);
	Z_Free(pMasterPalette);
	Z_Free(pGammaCorrectedPalette);

	pMasterPalette = static_cast<RGBA_t*>(Z_Malloc(sizeof (*pMasterPalette)*palsize, PU_STATIC, NULL));
	if (Cubeapply)
		pLocalPalette = static_cast<RGBA_t*>(Z_Malloc(sizeof (*pLocalPalette)*palsize, PU_STATIC, NULL));
	else
		pLocalPalette = pMasterPalette;
	pGammaCorrectedPalette = static_cast<RGBA_t*>(Z_Malloc(sizeof (*pGammaCorrectedPalette)*palsize, PU_STATIC, NULL));

	pal = static_cast<UINT8*>(W_CacheLumpNum(lumpnum, PU_CACHE));
	for (i = 0; i < palsize; i++)
	{
		pMasterPalette[i].s.red = *pal++;
		pMasterPalette[i].s.green = *pal++;
		pMasterPalette[i].s.blue = *pal++;
		pMasterPalette[i].s.alpha = 0xFF;

		pGammaCorrectedPalette[i].rgba = V_GammaDecode(pMasterPalette[i].rgba);

		if (!Cubeapply)
			continue;

		V_CubeApply(&pGammaCorrectedPalette[i]);
		pLocalPalette[i].rgba = V_GammaEncode(pGammaCorrectedPalette[i].rgba);
	}
}

void V_CubeApply(RGBA_t *input)
{
	float working[4][3];
	float linear;
	UINT8 q;

	if (!Cubeapply)
		return;

	linear = ((*input).s.red/255.0);
#define dolerp(e1, e2) ((1 - linear)*e1 + linear*e2)
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(Cubepal[0][0][0][q], Cubepal[1][0][0][q]);
		working[1][q] = dolerp(Cubepal[0][1][0][q], Cubepal[1][1][0][q]);
		working[2][q] = dolerp(Cubepal[0][0][1][q], Cubepal[1][0][1][q]);
		working[3][q] = dolerp(Cubepal[0][1][1][q], Cubepal[1][1][1][q]);
	}
	linear = ((*input).s.green/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = dolerp(working[0][q], working[1][q]);
		working[1][q] = dolerp(working[2][q], working[3][q]);
	}
	linear = ((*input).s.blue/255.0);
	for (q = 0; q < 3; q++)
	{
		working[0][q] = 255*dolerp(working[0][q], working[1][q]);
		if (working[0][q] > 255.0)
			working[0][q] = 255.0;
		else if (working[0][q]  < 0.0)
			working[0][q] = 0.0;
	}
#undef dolerp

	(*input).s.red = (UINT8)(working[0][0]);
	(*input).s.green = (UINT8)(working[0][1]);
	(*input).s.blue = (UINT8)(working[0][2]);
}

const char *R_GetPalname(UINT16 num)
{
	static char palname[9];
	char newpal[9] = "PLAYPAL";

	if (num > 0 && num <= 10000)
		snprintf(newpal, 8, "PAL%04u", num-1);

	strlcpy(palname, newpal, 9);
	return palname;
}

const char *GetPalette(void)
{
	const char *user = cv_palette.string;

	if (user && user[0])
	{
		if (W_CheckNumForName(user) == LUMPERROR)
		{
			CONS_Alert(CONS_WARNING,
					"cv_palette %s lump does not exist\n", user);
		}
		else
		{
			return cv_palette.string;
		}
	}

	if (gamestate == GS_LEVEL)
		return R_GetPalname((encoremode ? mapheaderinfo[gamemap-1]->encorepal : mapheaderinfo[gamemap-1]->palette));

	return "PLAYPAL";
}

void V_ReloadPalette(void)
{
	LoadPalette(GetPalette());
}

// -------------+
// V_SetPalette : Set the current palette to use for palettized graphics
//              :
// -------------+
void V_SetPalette(INT32 palettenum)
{
	if (!pLocalPalette)
		V_ReloadPalette();

	if (palettenum == 0)
	{
		palettenum = cv_palettenum.value;

		if (palettenum * 256U > currentPaletteSize - 256)
		{
			CONS_Alert(CONS_WARNING,
					"cv_palettenum %d out of range\n",
					palettenum);
			palettenum = 0;
		}
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_SetPalette(&pLocalPalette[palettenum*256]);
#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	else
#endif
#endif
	if (rendermode != render_none)
		I_SetPalette(&pLocalPalette[palettenum*256]);
}

void V_SetPaletteLump(const char *pal)
{
	LoadPalette(pal);
	V_SetPalette(0);
}

extern "C" {

void CV_palette_OnChange(void);
void CV_palette_OnChange(void)
{
	if (con_startup_loadprogress < LOADED_CONFIG)
		return;
	// recalculate Color Cube
	V_ReloadPalette();
	V_SetPalette(0);
}

}; // extern "C"

#if defined (__GNUC__) && defined (__i386__) && !defined (NOASM) && !defined (__APPLE__) && !defined (NORUSEASM)
void VID_BlitLinearScreen_ASM(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes);
#define HAVE_VIDCOPY
#endif

extern "C" {

void CV_constextsize_OnChange(void);
void CV_constextsize_OnChange(void)
{
	if (!con_startup)
		con_recalc = true;
}

}; // extern "C"


// --------------------------------------------------------------------------
// Copy a rectangular area from one bitmap to another (8bpp)
// --------------------------------------------------------------------------
void VID_BlitLinearScreen(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes)
{
#ifdef HAVE_VIDCOPY
    VID_BlitLinearScreen_ASM(srcptr,destptr,width,height,srcrowbytes,destrowbytes);
#else
	if ((srcrowbytes == destrowbytes) && (srcrowbytes == (size_t)width))
		M_Memcpy(destptr, srcptr, srcrowbytes * height);
	else
	{
		while (height--)
		{
			M_Memcpy(destptr, srcptr, width);

			destptr += destrowbytes;
			srcptr += srcrowbytes;
		}
	}
#endif
}

void V_AdjustXYWithSnap(INT32 *x, INT32 *y, UINT32 options, INT32 dupx, INT32 dupy)
{
	// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx
	INT32 screenwidth = vid.width;
	INT32 screenheight = vid.height;
	INT32 basewidth = BASEVIDWIDTH * dupx;
	INT32 baseheight = BASEVIDHEIGHT * dupy;
	SINT8 player = R_GetViewNumber();

	if (r_splitscreen > 0)
	{
		if (options & V_SPLITSCREEN)
		{
			screenheight /= 2;
			baseheight /= 2;

			if (r_splitscreen > 1)
			{
				screenwidth /= 2;
				basewidth /= 2;
			}
		}
	}
	else if ((options & (V_SLIDEIN|V_SNAPTOBOTTOM)) == (V_SLIDEIN|V_SNAPTOBOTTOM))
	{
		INT32 slide = K_GetDialogueSlide(51 * FRACUNIT);
		if (slide)
		{
			*y -= FixedMul(slide, dupy);
		}
	}

	if (vid.width != (BASEVIDWIDTH * dupx))
	{
		if (options & V_SNAPTORIGHT)
			*x += (screenwidth - basewidth);
		else if (!(options & V_SNAPTOLEFT))
			*x += (screenwidth - basewidth) / 2;
	}

	if (vid.height != (BASEVIDHEIGHT * dupy))
	{
		if (options & V_SNAPTOBOTTOM)
			*y += (screenheight - baseheight);
		else if (!(options & V_SNAPTOTOP))
			*y += (screenheight - baseheight) / 2;
	}

	if (options & V_SPLITSCREEN)
	{
		if (r_splitscreen == 1)
		{
			if (player == 1)
				*y += screenheight;
		}
		else if (r_splitscreen > 1)
		{
			if (player == 1 || player == 3)
				*x += screenwidth;

			if (player == 2 || player == 3)
				*y += screenheight;
		}
	}

	if ((options & V_SLIDEIN))
	{
		if (st_fadein < FRACUNIT)
		{
			if ((options & (V_SNAPTORIGHT|V_SNAPTOLEFT|V_SPLITSCREEN)) != 0)
			{
				boolean slidefromright = false;

				const fixed_t offsetAmount = (screenwidth * FRACUNIT/2);
				INT32 offset = (offsetAmount - FixedMul(offsetAmount, st_fadein)) / FRACUNIT;

				if (r_splitscreen > 1)
				{
					if (player & 1)
						slidefromright = true;
				}

				if (options & V_SNAPTORIGHT)
					slidefromright = true;
				else if (options & V_SNAPTOLEFT)
					slidefromright = false;

				if (slidefromright == true)
				{
					offset = -offset;
				}

				*x -= offset;
			}
			else
			{
				const fixed_t offsetAmount = (screenheight * FRACUNIT/2);
				INT32 offset = (offsetAmount - FixedMul(offsetAmount, st_fadein)) / FRACUNIT;

				if (options & V_SNAPTOBOTTOM)
				{
					offset = -offset;
				}

				*y -= offset;
			}
		}
	}
}

static cliprect_t cliprect = {0};

const cliprect_t *V_GetClipRect(void)
{
	if (cliprect.enabled == false)
	{
		return NULL;
	}

	return &cliprect;
}

void V_SetClipRect(fixed_t x, fixed_t y, fixed_t w, fixed_t h, INT32 flags)
{
	// Adjust position.
	if (!(flags & V_NOSCALESTART))
	{
		fixed_t dupx = vid.dupx;
		fixed_t dupy = vid.dupy;

		if (flags & V_SCALEPATCHMASK)
		{
			switch ((flags & V_SCALEPATCHMASK) >> V_SCALEPATCHSHIFT)
			{
				case 1: // V_NOSCALEPATCH
					dupx = dupy = 1;
					break;
				case 2: // V_SMALLSCALEPATCH
					dupx = vid.smalldupx;
					dupy = vid.smalldupy;
					break;
				case 3: // V_MEDSCALEPATCH
					dupx = vid.meddupx;
					dupy = vid.meddupy;
					break;
				default:
					break;
			}
		}

		dupx = dupy = (dupx < dupy ? dupx : dupy);

		x = FixedMul(x, dupx);
		y = FixedMul(y, dupy);
		w = FixedMul(w, dupx);
		h = FixedMul(h, dupy);

		if (!(flags & V_SCALEPATCHMASK))
		{
			V_AdjustXYWithSnap(&x, &y, flags, dupx, dupy);
		}
	}

	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (x > vid.width)
	{
		x = vid.width;
		w = 0;
	}

	if (y > vid.height)
	{
		y = vid.height;
		h = 0;
	}

	cliprect.left = x;
	cliprect.top = y;
	cliprect.right = x + w;
	cliprect.bottom = y + h;
	cliprect.flags = flags;
	cliprect.enabled = true;

	/*
	V_DrawFill(cliprect.l, cliprect.t, cliprect.r - cliprect.l, cliprect.b - cliprect.t, V_NOSCALESTART);
	CONS_Printf("[(%d, %d), (%d, %d)]\n", cliprect.l, cliprect.t, cliprect.r, cliprect.b);
	*/
}

void V_ClearClipRect(void)
{
	cliprect.enabled = false;
}

void V_SaveClipRect(cliprect_t *copy)
{
	*copy = cliprect;
}

void V_RestoreClipRect(const cliprect_t *copy)
{
	cliprect = *copy;
}

static UINT8 hudplusalpha[11]  = { 10,  8,  6,  4,  2,  0,  0,  0,  0,  0,  0};
static UINT8 hudminusalpha[11] = { 10,  9,  9,  8,  8,  7,  7,  6,  6,  5,  5};

static const UINT8 *v_colormap = NULL;
static const UINT8 *v_translevel = NULL;

static inline UINT8 standardpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	(void)dest; return source[ofs>>FRACBITS];
}
static inline UINT8 mappedpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	(void)dest; return *(v_colormap + source[ofs>>FRACBITS]);
}
static inline UINT8 translucentpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	return *(v_translevel + ((source[ofs>>FRACBITS]<<8)&0xff00) + (*dest&0xff));
}
static inline UINT8 transmappedpdraw(const UINT8 *dest, const UINT8 *source, fixed_t ofs)
{
	return *(v_translevel + (((*(v_colormap + source[ofs>>FRACBITS]))<<8)&0xff00) + (*dest&0xff));
}

UINT32 V_GetHUDTranslucency(INT32 scrn)
{
	if (scrn & V_SLIDEIN)
	{
		return 10;
	}

	if (scrn & V_SPLITSCREEN)
	{
		return FixedMul(10, st_fadein);
	}

	return st_translucency;
}

static UINT32 V_GetAlphaLevel(INT32 scrn)
{
	switch (scrn & V_ALPHAMASK)
	{
	case V_HUDTRANSHALF:
		return hudminusalpha[V_GetHUDTranslucency(scrn)];

	case V_HUDTRANS:
		return 10 - V_GetHUDTranslucency(scrn);

	case V_HUDTRANSDOUBLE:
		return hudplusalpha[V_GetHUDTranslucency(scrn)];

	default:
		return (scrn & V_ALPHAMASK) >> V_ALPHASHIFT;
	}
}

// Draws a patch scaled to arbitrary size.
void V_DrawStretchyFixedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap)
{
	UINT32 alphalevel, blendmode;

	fixed_t vdup;
	INT32 dupx, dupy;
	fixed_t pwidth; // patch width

	const cliprect_t *clip = V_GetClipRect();

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawStretchyFixedPatch(patch, x, y, pscale, vscale, scrn, colormap);
		return;
	}
#endif

	if ((blendmode = ((scrn & V_BLENDMASK) >> V_BLENDSHIFT)))
		blendmode++; // realign to constants
	if ((alphalevel = V_GetAlphaLevel(scrn)) >= 10)
		return;

	dupx = vid.dupx;
	dupy = vid.dupy;
	if (scrn & V_SCALEPATCHMASK) switch ((scrn & V_SCALEPATCHMASK) >> V_SCALEPATCHSHIFT)
	{
		case 1: // V_NOSCALEPATCH
			dupx = dupy = 1;
			break;
		case 2: // V_SMALLSCALEPATCH
			dupx = vid.smalldupx;
			dupy = vid.smalldupy;
			break;
		case 3: // V_MEDSCALEPATCH
			dupx = vid.meddupx;
			dupy = vid.meddupy;
			break;
		default:
			break;
	}

	// only use one dup, to avoid stretching (har har)
	dupx = dupy = (dupx < dupy ? dupx : dupy);
	vdup = FixedMul(dupx<<FRACBITS, pscale);
	if (vscale != pscale)
		vdup = FixedMul(dupx<<FRACBITS, vscale);

	{
		fixed_t offsetx = 0, offsety = 0;

		// left offset
		if (scrn & V_FLIP)
			offsetx = FixedMul((patch->width - patch->leftoffset)<<FRACBITS, pscale);
		else
			offsetx = FixedMul(patch->leftoffset<<FRACBITS, pscale);

		// top offset
		if (scrn & V_VFLIP)
			offsety = FixedMul((patch->height - patch->topoffset)<<FRACBITS, vscale);
		else
			offsety = FixedMul(patch->topoffset<<FRACBITS, vscale);

		// Subtract the offsets from x/y positions
		x -= offsetx;
		y -= offsety;
	}

	if (scrn & V_NOSCALESTART)
	{
		x >>= FRACBITS;
		y >>= FRACBITS;
	}
	else
	{
		x = FixedMul(x,dupx<<FRACBITS);
		y = FixedMul(y,dupy<<FRACBITS);
		x >>= FRACBITS;
		y >>= FRACBITS;

		// Center it if necessary
		if (!(scrn & V_SCALEPATCHMASK))
		{
			V_AdjustXYWithSnap(&x, &y, scrn, dupx, dupy);
		}
	}

	if (pscale != FRACUNIT) // scale width properly
	{
		pwidth = patch->width<<FRACBITS;
		pwidth = FixedMul(pwidth, pscale);
		pwidth = FixedMul(pwidth, dupx<<FRACBITS);
		pwidth >>= FRACBITS;
	}
	else
		pwidth = patch->width * dupx;

	float fdupy = FIXED_TO_FLOAT(vdup);

	float fx = x;
	float fy = y;
	float fx2 = fx + pwidth;
	float fy2 = fy + std::round(static_cast<float>(patch->height) * fdupy);
	float falpha = 1.f;
	float umin = 0.f;
	float umax = 1.f;
	float vmin = 0.f;
	float vmax = 1.f;

	// flip UVs
	if (scrn & V_FLIP)
	{
		umin = 1.f - umin;
		umax = 1.f - umax;
	}
	if (scrn & V_VFLIP)
	{
		vmin = 1.f - vmin;
		vmax = 1.f - vmax;
	}

	if (alphalevel > 0 && alphalevel <= 10)
	{
		falpha = (10 - alphalevel) / 10.f;
	}
	hwr2::BlendMode blend = hwr2::BlendMode::kAlphaTransparent;
	switch (blendmode)
	{
	case AST_MODULATE:
		blend = hwr2::BlendMode::kModulate;
		break;
	case AST_ADD:
		blend = hwr2::BlendMode::kAdditive;
		break;

	// Note: SRB2 has these blend modes flipped compared to GL and Vulkan.
	// SRB2's Subtract is Dst - Src. OpenGL is Src - Dst. And vice versa for reverse.
	// Twodee will use the GL definitions.
	case AST_SUBTRACT:
		blend = hwr2::BlendMode::kReverseSubtractive;
		break;
	case AST_REVERSESUBTRACT:
		blend = hwr2::BlendMode::kSubtractive;
		break;
	default:
		blend = hwr2::BlendMode::kAlphaTransparent;
		break;
	}

	auto builder = g_2d.begin_quad();
	builder
		.patch(patch)
		.rect(fx, fy, fx2 - fx, fy2 - fy)
		.flip((scrn & V_FLIP) > 0)
		.vflip((scrn & V_VFLIP) > 0)
		.color(1, 1, 1, falpha)
		.blend(blend)
		.colormap(colormap);

	if (clip && clip->enabled)
	{
		builder.clip(clip->left, clip->top, clip->right, clip->bottom);
	}
	builder.done();
}

// Draws a patch cropped and scaled to arbitrary size.
void V_DrawCroppedPatch(fixed_t x, fixed_t y, fixed_t pscale, INT32 scrn, patch_t *patch, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h)
{
	cliprect_t oldClip = cliprect;

	V_SetClipRect(x, y, w, h, scrn);

	x -= sx;
	y -= sy;

	V_DrawStretchyFixedPatch(x, y, pscale, pscale, scrn, patch, NULL);

	cliprect = oldClip;
}

//
// V_DrawContinueIcon
// Draw a mini player!  If we can, that is.  Otherwise we draw a star.
//
void V_DrawContinueIcon(INT32 x, INT32 y, INT32 flags, INT32 skinnum, UINT16 skincolor)
{
	(void)skinnum;
	(void)skincolor;
	V_DrawScaledPatch(x - 10, y - 14, flags, static_cast<patch_t*>(W_CachePatchName("CONTINS", PU_PATCH)));
}

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void V_DrawBlock(INT32 x, INT32 y, INT32 scrn, INT32 width, INT32 height, const UINT8 *src)
{
	UINT8 *dest;
	const UINT8 *deststop;

	if (x < 0 || x + width > vid.width || y < 0 || y + height > vid.height || (unsigned)scrn > 4)
		I_Error("Bad V_DrawBlock");

	dest = screens[scrn] + y*vid.width + x;
	deststop = screens[scrn] + vid.rowbytes * vid.height;

	while (height--)
	{
		M_Memcpy(dest, src, width);

		src += width;
		dest += vid.width;
		if (dest > deststop)
			return;
	}
}

//
// Fills a box of pixels with a single color, NOTE: scaled to screen size
//
void V_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	const cliprect_t *clip = V_GetClipRect();

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawFill(x, y, w, h, c);
		return;
	}
#endif

	UINT32 alphalevel;
	if ((alphalevel = V_GetAlphaLevel(c)) >= 10)
		return;

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{
			w = vid.width;
			h = vid.height;
		}
		else
		{
			x *= dupx;
			y *= dupy;
			w *= dupx;
			h *= dupy;

			// Center it if necessary
			V_AdjustXYWithSnap(&x, &y, c, dupx, dupy);
		}
	}

	if (x >= vid.width || y >= vid.height)
		return; // off the screen

	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return; // zero width/height wouldn't draw anything
	if (x + w > vid.width)
		w = vid.width - x;
	if (y + h > vid.height)
		h = vid.height - y;

	c &= 255;

	RGBA_t color = pLocalPalette[c];
	UINT8 r = (color.rgba & 0xFF);
	UINT8 g = (color.rgba & 0xFF00) >> 8;
	UINT8 b = (color.rgba & 0xFF0000) >> 16;

	if (clip && clip->enabled)
	{
		int x2 = std::min(x + w, clip->right);
		int y2 = std::min(y + h, clip->bottom);

		if (x < clip->left)
			x = clip->left;

		if (y < clip->top)
			y = clip->top;

		w = std::max<INT32>(0, x2 - x);
		h = std::max<INT32>(0, y2 - y);
	}

	g_2d.begin_quad()
		.patch(nullptr)
		.color(r / 255.f, g / 255.f, b / 255.f, (10 - alphalevel) / 10.f)
		.rect(x, y, w, h)
		.done();
}

static UINT32 V_GetHWConsBackColor(void)
{
	UINT32 hwcolor;
	switch (cons_backcolor.value)
	{
		case 0:		hwcolor = 0xffffff00;	break; 	// White
		case 1:		hwcolor = 0x80808000;	break; 	// Black
		case 2:		hwcolor = 0xdeb88700;	break;	// Sepia
		case 3:		hwcolor = 0x40201000;	break; 	// Brown
		case 4:		hwcolor = 0xfa807200;	break; 	// Pink
		case 5:		hwcolor = 0xff000000;	break; 	// Red
		case 6:		hwcolor = 0xff800000;	break; 	// Orange
		case 7:		hwcolor = 0xdaa52000;	break; 	// Gold
		case 8:		hwcolor = 0xffdd0000;	break; 	// Yellow
		case 9:		hwcolor = 0xc5e80000;	break; 	// Peridot
		case 10:	hwcolor = 0x00800000;	break; 	// Green
		case 11:	hwcolor = 0x15f2b000;	break; 	// Aquamarine
		case 12:	hwcolor = 0x00ffff00;	break; 	// Cyan
		case 13:	hwcolor = 0x4682b400;	break; 	// Steel
		case 14:	hwcolor = 0x0000ff00;	break; 	// Blue
		case 15:	hwcolor = 0x9844ff00;	break; 	// Purple
		case 16:	hwcolor = 0xff00ff00;	break;	// Magenta
		case 17:	hwcolor = 0xee82ee00;	break; 	// Lavender
		case 18:	hwcolor = 0xf570a500;	break;	// Rose
		// Default green
		default:	hwcolor = 0x00800000;	break;
	}
	return hwcolor;
}

// THANK YOU MPC!!!
// and thanks toaster for cleaning it up.

void V_DrawFillConsoleMap(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	UINT32 alphalevel = 0;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		UINT32 hwcolor = V_GetHWConsBackColor();
		HWR_DrawConsoleFill(x, y, w, h, c, hwcolor);	// we still use the regular color stuff but only for flags. actual draw color is "hwcolor" for this.
		return;
	}
#endif

	if ((alphalevel = V_GetAlphaLevel(c)) >= 10)
		return;

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		x *= dupx;
		y *= dupy;
		w *= dupx;
		h *= dupy;

		// Center it if necessary
		V_AdjustXYWithSnap(&x, &y, c, dupx, dupy);
	}

	if (x >= vid.width || y >= vid.height)
		return; // off the screen
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return; // zero width/height wouldn't draw anything
	if (x + w > vid.width)
		w = vid.width-x;
	if (y + h > vid.height)
		h = vid.height-y;

	c &= 255;

	UINT32 hwcolor = V_GetHWConsBackColor();
	float r = ((hwcolor & 0xFF000000) >> 24) / 255.f;
	float g = ((hwcolor & 0xFF0000) >> 16) / 255.f;
	float b = ((hwcolor & 0xFF00) >> 8) / 255.f;
	float a = 0.5f; // alphalevel is unused in GL??
	g_2d.begin_quad()
		.rect(x, y, w, h)
		.blend(hwr2::BlendMode::kAlphaTransparent)
		.color(r, g, b, a)
		.done();
}

//
// Fills a triangle of pixels with a single color, NOTE: scaled to screen size
//
// ...
// ..  <-- this shape only for now, i'm afraid
// .
//
void V_DrawDiag(INT32 x, INT32 y, INT32 wh, INT32 c)
{
	INT32 w, h;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawDiag(x, y, wh, c);
		return;
	}
#endif

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		x *= dupx;
		y *= dupy;
		wh *= dupx;

		// Center it if necessary
		V_AdjustXYWithSnap(&x, &y, c, dupx, dupy);
	}

	if (x >= vid.width || y >= vid.height)
		return; // off the screen

	if (y < 0)
	{
		wh += y;
		y = 0;
	}

	w = h = wh;

	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (w <= 0 || h <= 0)
		return; // zero width/height wouldn't draw anything
	if (x + w > vid.width)
	{
		w = vid.width - x;
	}
	if (y + w > vid.height)
		h = vid.height - y;

	if (h > w)
		h = w;

	c &= 255;

	{
		auto builder = g_2d.begin_verts();

		const RGBA_t color = pLocalPalette[c];
		const float r = ((color.rgba & 0xFF000000) >> 24) / 255.f;
		const float g = ((color.rgba & 0xFF0000) >> 16) / 255.f;
		const float b = ((color.rgba & 0xFF00) >> 8) / 255.f;
		const float a = 1.f;
		builder.color(r, g, b, a);

		builder
			.vert(x, y)
			.vert(x + wh, y + wh)
			.vert(x, y + wh)
			.done();
	}
}

//
// If color is 0x00 to 0xFF, draw transtable (strength range 0-9).
// Else, use COLORMAP lump (strength range 0-31).
// c is not color, it is for flags only. transparency flags will be ignored.
// IF YOU ARE NOT CAREFUL, THIS CAN AND WILL CRASH!
// I have kept the safety checks for strength out of this function;
// I don't trust Lua users with it, so it doesn't matter.
//
void V_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c, UINT16 color, UINT8 strength)
{
	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		// ughhhhh please can someone else do this? thanks ~toast 25/7/19 in 38 degrees centigrade w/o AC
		HWR_DrawFadeFill(x, y, w, h, c, color, strength); // toast two days later - left above comment in 'cause it's funny
		return;
	}
#endif

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		x *= dupx;
		y *= dupy;
		w *= dupx;
		h *= dupy;

		// Center it if necessary
		V_AdjustXYWithSnap(&x, &y, c, dupx, dupy);
	}

	if (x >= vid.width || y >= vid.height)
		return; // off the screen
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}

	if (w <= 0 || h <= 0)
		return; // zero width/height wouldn't draw anything
	if (x + w > vid.width)
		w = vid.width-x;
	if (y + h > vid.height)
		h = vid.height-y;

	float r;
	float g;
	float b;
	float a;
	hwr2::BlendMode blendmode;

	if (color & 0xFF00)
	{
		// Historical COLORMAP fade
		// In Ring Racers this is a Mega Drive style per-channel fade (though it'd probably be cool in SRB2 too)
		// HWR2 will implement as a rev-subtractive rect because colormaps aren't possible in hardware
		float fstrength = std::clamp(strength / 31.f, 0.f, 1.f);
		r = std::clamp((fstrength - (0.f / 3.f)) * 3.f, 0.f, 1.f);
		g = std::clamp((fstrength - (1.f / 3.f)) * 3.f, 0.f, 1.f);
		b = std::clamp((fstrength - (2.f / 3.f)) * 3.f, 0.f, 1.f);
		a = 1;

		blendmode = hwr2::BlendMode::kReverseSubtractive;
	}
	else
	{
		// Historically TRANSMAP fade
		// This is done by modulative (transparent) blend to the given palette color.
		byteColor_t bc = V_GetColor(color).s;
		r = bc.red / 255.f;
		g = bc.green / 255.f;
		b = bc.blue / 255.f;
		a = softwaretranstohwr[std::clamp(static_cast<int>(strength), 0, 10)] / 255.f;
		blendmode = hwr2::BlendMode::kAlphaTransparent;
	}

	g_2d.begin_quad()
		.blend(blendmode)
		.color(r, g, b, a)
		.rect(x, y, w, h)
		.done();
}

//
// Fills a box of pixels using a flat texture as a pattern, scaled to screen size.
//
void V_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatnum)
{
	INT32 dupx;
	INT32 dupy;
	size_t size;
	size_t lflatsize;

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawFlatFill(x, y, w, h, flatnum);
		return;
	}
#endif
	size = W_LumpLength(flatnum);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			lflatsize = 2048;
			break;
		case 1048576: // 1024x1024 lump
			lflatsize = 1024;
			break;
		case 262144:// 512x512 lump
			lflatsize = 512;
			break;
		case 65536: // 256x256 lump
			lflatsize = 256;
			break;
		case 16384: // 128x128 lump
			lflatsize = 128;
			break;
		case 1024: // 32x32 lump
			lflatsize = 32;
			break;
		case 256: // 16x16 lump
			lflatsize = 16;
			break;
		case 64: // 8x8 lump
			lflatsize = 8;
			break;
		default: // 64x64 lump
			lflatsize = 64;
			break;
	}

	float fsize = lflatsize;

	dupx = dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	g_2d.begin_verts()
		.flat(flatnum)
		.vert(x * dupx, y * dupy, 0, 0)
		.vert(x * dupx + w * dupx, y * dupy, w / fsize, 0)
		.vert(x * dupx + w * dupx, y * dupy + h * dupy, w / fsize, h / fsize)
		.vert(x * dupx, y * dupy, 0, 0)
		.vert(x * dupx + w * dupx, y * dupy + h * dupy, w / fsize, h / fsize)
		.vert(x * dupx, y * dupy + h * dupy, 0, h / fsize)
		.done();
}

//
// V_DrawPatchFill
//
void V_DrawPatchFill(patch_t *pat)
{
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	INT32 x, y, pw = pat->width * dupz, ph = pat->height * dupz;

	for (x = 0; x < vid.width; x += pw)
	{
		for (y = 0; y < vid.height; y += ph)
			V_DrawScaledPatch(x, y, V_NOSCALESTART, pat);
	}
}

void V_DrawVhsEffect(boolean rewind)
{
	static fixed_t upbary = 100, downbary = 150;

	UINT8 *buf = screens[0], *tmp = screens[4];
	UINT16 y;
	UINT32 x, pos = 0;

	UINT8 *normalmapstart = ((UINT8 *)transtables + (8<<FF_TRANSSHIFT|(19<<8)));
#ifdef HQ_VHS
	UINT8 *tmapstart = ((UINT8 *)transtables + (6<<FF_TRANSSHIFT));
#endif
	UINT8 *thismapstart;
	SINT8 offs;

	UINT8 barsize = vid.dupy<<5;
	UINT8 updistort = vid.dupx<<(rewind ? 5 : 3);
	UINT8 downdistort = updistort>>1;

	if (rewind)
		V_DrawVhsEffect(false); // experimentation

	upbary -= FixedMul(vid.dupy * (rewind ? 3 : 1.8f), renderdeltatics);
	downbary += FixedMul(vid.dupy * (rewind ? 2 : 1), renderdeltatics);
	if (upbary < -barsize) upbary = vid.height;
	if (downbary > vid.height) downbary = -barsize;

	for (y = 0; y < vid.height; y+=2)
	{
		thismapstart = normalmapstart;
		offs = 0;

		if (y >= upbary && y < upbary+barsize)
		{
			thismapstart -= (2<<FF_TRANSSHIFT) - (5<<8);
			offs += updistort * 2.0f * std::min(y-upbary, upbary+barsize-y) / barsize;
		}
		if (y >= downbary && y < downbary+barsize)
		{
			thismapstart -= (2<<FF_TRANSSHIFT) - (5<<8);
			offs -= downdistort * 2.0f * std::min(y-downbary, downbary+barsize-y) / barsize;
		}
		offs += M_RandomKey(vid.dupx<<1);

		// lazy way to avoid crashes
		if (y == 0 && offs < 0) offs = 0;
		else if (y >= vid.height-2 && offs > 0) offs = 0;

		for (x = pos+vid.rowbytes*2; pos < x; pos++)
		{
			tmp[pos] = thismapstart[buf[pos+offs]];
#ifdef HQ_VHS
			tmp[pos] = tmapstart[buf[pos]<<8 | tmp[pos]];
#endif
		}
	}

	memcpy(buf, tmp, vid.rowbytes*vid.height);
}

//
// Fade all the screen buffer, so that the menu is more readable,
// especially now that we use the small hufont in the menus...
// If color is 0x00 to 0xFF, draw transtable (strength range 0-9).
// Else, use COLORMAP lump (strength range 0-31).
// IF YOU ARE NOT CAREFUL, THIS CAN AND WILL CRASH!
// I have kept the safety checks out of this function;
// the v.fadeScreen Lua interface handles those.
//
void V_DrawFadeScreen(UINT16 color, UINT8 strength)
{
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_FadeScreenMenuBack(color, strength);
		return;
	}
#endif

	float r;
	float g;
	float b;
	float a;
	hwr2::BlendMode blendmode;

	if (color & 0xFF00)
	{
		// Historical COLORMAP fade
		// In Ring Racers this is a Mega Drive style per-channel fade (though it'd probably be cool in SRB2 too)
		// HWR2 will implement as a rev-subtractive rect because colormaps aren't possible in hardware
		float fstrength = std::clamp(strength / 31.f, 0.f, 1.f);
		r = std::clamp((fstrength - (0.f / 3.f)) * 3.f, 0.f, 1.f);
		g = std::clamp((fstrength - (1.f / 3.f)) * 3.f, 0.f, 1.f);
		b = std::clamp((fstrength - (2.f / 3.f)) * 3.f, 0.f, 1.f);
		a = 1;

		blendmode = hwr2::BlendMode::kReverseSubtractive;
	}
	else
	{
		// Historically TRANSMAP fade
		// This is done by modulative (transparent) blend to the given palette color.
		byteColor_t bc = V_GetColor(color).s;
		r = bc.red / 255.f;
		g = bc.green / 255.f;
		b = bc.blue / 255.f;
		a = softwaretranstohwr[std::clamp(static_cast<int>(strength), 0, 10)] / 255.f;
		blendmode = hwr2::BlendMode::kAlphaTransparent;
	}

	g_2d.begin_quad()
		.blend(blendmode)
		.color(r, g, b, a)
		.rect(0, 0, vid.width, vid.height)
		.done();
}

lighttable_t *V_LoadCustomFadeMap(const char *lump)
{
	lumpnum_t lumpnum = LUMPERROR;
	lighttable_t *clm = NULL;

	if (lump != NULL)
		lumpnum = W_GetNumForName(lump);
	else
		return NULL;

	if (lumpnum != LUMPERROR)
	{
		clm = static_cast<lighttable_t*>(Z_MallocAlign(COLORMAP_SIZE, PU_STATIC, NULL, 8));
		W_ReadLump(lumpnum, clm);
		return clm;
	}

	return NULL;
}

const UINT8 *V_OffsetIntoFadeMap(const lighttable_t *clm, UINT8 strength)
{
	return ((const UINT8 *)clm + strength*256);
}

//
// Fade the screen buffer, using a custom COLORMAP lump.
// Split from V_DrawFadeScreen, because that function has
// WAY too many options piled on top of it as is. :V
//
void V_DrawCustomFadeScreen(const char *lump, UINT8 strength)
{
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawCustomFadeScreen(
			(strcmp(lump, "FADEMAP1") != 0
				? 31
				: 0
			),
			strength
		);
		return;
	}
#endif

	// TODO: fix this for Twodee
	{
		lighttable_t *clm = V_LoadCustomFadeMap(lump);

		if (clm != NULL)
		{
			const UINT8 *fadetable = V_OffsetIntoFadeMap(clm, strength);
			const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;
			UINT8 *buf = screens[0];

			// heavily simplified -- we don't need to know x or y
			// position when we're doing a full screen fade
			for (; buf < deststop; ++buf)
				*buf = fadetable[*buf];

			Z_Free(clm);
			clm = NULL;
		}
	}
}

// Simple translucency with one color, over a set number of lines starting from the top.
void V_DrawFadeConsBack(INT32 plines)
{
	UINT32 hwcolor = V_GetHWConsBackColor();
#ifdef HWRENDER // not win32 only 19990829 by Kin
	if (rendermode == render_opengl)
	{
		HWR_DrawConsoleBack(hwcolor, plines);
		return;
	}
#endif

	float r = ((hwcolor & 0xFF000000) >> 24) / 255.f;
	float g = ((hwcolor & 0xFF0000) >> 16) / 255.f;
	float b = ((hwcolor & 0xFF00) >> 8) / 255.f;
	float a = 0.5f;
	g_2d.begin_quad()
		.rect(0, 0, vid.width, plines)
		.blend(hwr2::BlendMode::kAlphaTransparent)
		.color(r, g, b, a)
		.done();
}


//
// Invert the entire screen, for Encore fades
//
void V_EncoreInvertScreen(void)
{
#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_EncoreInvertScreen();
		return;
	}
#endif

	g_2d.begin_quad()
		.blend(hwr2::BlendMode::kInvertDest)
		.color(1, 1, 1, 1)
		.rect(0, 0, vid.width, vid.height)
		.done();
}

// Very similar to F_DrawFadeConsBack, except we draw from the middle(-ish) of the screen to the bottom.
void V_DrawPromptBack(INT32 boxheight, INT32 color)
{
	if (color >= 256 && color < 512)
	{
		if (boxheight < 0)
			boxheight = -boxheight;
		else // 4 lines of space plus gaps between and some leeway
			boxheight = ((boxheight * 4) + (boxheight/2)*5);
		V_DrawFill((BASEVIDWIDTH-(vid.width/vid.dupx))/2, BASEVIDHEIGHT-boxheight, (vid.width/vid.dupx),boxheight, (color-256)|V_SNAPTOBOTTOM);
		return;
	}

	boxheight *= vid.dupy;

	if (color == INT32_MAX)
		color = cons_backcolor.value;

	UINT32 hwcolor = V_GetHWConsBackColor();

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		HWR_DrawTutorialBack(hwcolor, boxheight);
		return;
	}
#endif

	float r = ((color & 0xFF000000) >> 24) / 255.f;
	float g = ((color & 0xFF0000) >> 16) / 255.f;
	float b = ((color & 0xFF00) >> 8) / 255.f;
	float a = (color == 0 ? 0xC0 : 0x80) / 255.f; // make black darker, like software

	INT32 real_boxheight = (boxheight * 4) + (boxheight / 2) * 5;
	g_2d.begin_quad()
		.rect(0, vid.height - real_boxheight, vid.width, real_boxheight)
		.color(r, g, b, a)
		.done();
}

// Gets string colormap, used for 0x80 color codes
//
UINT8 *V_GetStringColormap(INT32 colorflags)
{
#if 0 // perfect
	switch ((colorflags & V_CHARCOLORMASK) >> V_CHARCOLORSHIFT)
	{
	case  1: // 0x81, purple
		return purplemap;
	case  2: // 0x82, yellow
		return yellowmap;
	case  3: // 0x83, green
		return greenmap;
	case  4: // 0x84, blue
		return bluemap;
	case  5: // 0x85, red
		return redmap;
	case  6: // 0x86, gray
		return graymap;
	case  7: // 0x87, orange
		return orangemap;
	case  8: // 0x88, sky
		return skymap;
	case  9: // 0x89, lavender
		return lavendermap;
	case 10: // 0x8A, gold
		return goldmap;
	case 11: // 0x8B, aqua-green
		return aquamap;
	case 12: // 0x8C, magenta
		return magentamap;
	case 13: // 0x8D, pink
		return pinkmap;
	case 14: // 0x8E, brown
		return brownmap;
	case 15: // 0x8F, tan
		return tanmap;
	default: // reset
		return NULL;
	}
#else // optimised
	colorflags = ((colorflags & V_CHARCOLORMASK) >> V_CHARCOLORSHIFT);
	if (!colorflags || colorflags > 15) // INT32 is signed, but V_CHARCOLORMASK is a very restrictive mask.
		return NULL;
	return (purplemap+((colorflags-1)<<8));
#endif
}

INT32 V_DanceYOffset(INT32 counter)
{
	const INT32 duration = 16;
	const INT32 step = (I_GetTime() + counter) % duration;

	return abs(step - (duration / 2)) - (duration / 4);
}

static boolean V_CharacterValid(font_t *font, int c)
{
	return (c >= 0 && c < font->size && font->font[c] != NULL);
}

// Writes a single character (draw WHITE if bit 7 set)
//
void V_DrawCharacterScaled(
	fixed_t x,
	fixed_t y,
	fixed_t scale,
	INT32 flags,
	int fontno,
	int c,
	UINT8 *colormap)
{
	font_t *font = &fontv[fontno];
	boolean notColored = false;

	const boolean uppercase = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags &= ~(V_FLIP); /* These two (V_FORCEUPPERCASE) share a bit. */

	if (colormap == NULL)
	{
		colormap = V_GetStringColormap(( flags & V_CHARCOLORMASK ));
	}

	notColored = (colormap == NULL);

	flags &= ~(V_CHARCOLORMASK | V_PARAMMASK);

	if (uppercase)
	{
		c = toupper(c);
	}
	else if (V_CharacterValid(font, c - font->start) == false)
	{
		// Try the other case if it doesn't exist
		if (c >= 'A' && c <= 'Z')
		{
			c = tolower(c);
		}
		else if (c >= 'a' && c <= 'z')
		{
			c = toupper(c);
		}
	}

	c -= font->start;
	if (V_CharacterValid(font, c) == false)
	{
		return;
	}

	if (notColored == true)
	{
		if (( c & 0xF0 ) == 0x80)
		{
			colormap = V_GetStringColormap(
				( ( c & 0x7f ) << V_CHARCOLORSHIFT ) & V_CHARCOLORMASK
			);
		}
	}

	V_DrawFixedPatch(
		x, y,
		scale,
		flags,
		font->font[c],
		colormap
	);
}

void V_DrawCharacter(INT32 x, INT32 y, INT32 c, boolean lowercase)
{
	// Backwards compatibility
	if (lowercase == false)
	{
		c |= V_FORCEUPPERCASE;
	}

	V_DrawCharacterScaled(
		x << FRACBITS,
		y << FRACBITS,
		FRACUNIT,
		(c & ~V_PARAMMASK),
		HU_FONT,
		(c & V_PARAMMASK),
		NULL
	);
}

void V_DrawChatCharacter(INT32 x, INT32 y, INT32 c, boolean lowercase, UINT8 *colormap)
{
	// Backwards compatibility
	if (lowercase == false)
	{
		c |= V_FORCEUPPERCASE;
	}

	V_DrawCharacterScaled(
		x << FRACBITS,
		y << FRACBITS,
		FRACUNIT >> 1,
		(c & ~V_PARAMMASK),
		HU_FONT,
		(c & V_PARAMMASK),
		colormap
	);
}

template <bool Centered>
static INT32 Internal_TitleCardStringOffset(const char *str, boolean p4)
{
	int bg_font = GTOL_FONT;
	int fg_font = GTFN_FONT;

	if (p4)
	{
		bg_font = GTOL4_FONT;
		fg_font = GTFN4_FONT;
	}

	INT32 xoffs = 0;
	const char *ch = str;
	char c;
	patch_t *pp;

	// Returns true if it reached the end, false if interrupted.
	auto scan = [&](auto keep_going)
	{
		for (;;ch++)
		{
			if (*ch == '\n')
			{
				xoffs = 0;
				return false;
			}

			if (!keep_going(*ch))
			{
				break;
			}

			c = *ch;
			c = toupper(c);
			c -= LT_FONTSTART;

			// check if character exists, if not, it's a space.
			if (c < 0 || c >= LT_FONTSIZE || !fontv[bg_font].font[(INT32)c])
			{
				xoffs += p4 ? 5 : 10;
				continue;
			}

			pp = fontv[fg_font].font[(INT32)c];

			xoffs += pp->width - (p4 ? 3 : 5);
		}

		return true;
	};

	do
	{
		// For the sake of centering, don't count spaces or
		// punctuation at each end of a line.
		// TODO: This should ideally be more sophisticated:
		// - Check patch width directly for monospace or
		//   punctuation that isn't necessarily thin.
		// - Apply to all centered string drawing.
		if constexpr (Centered)
		{
			// Count leading fluff
			if (!scan([](int c) { return c && !isalnum(c); }))
			{
				continue;
			}

			if (!*ch)
			{
				// ALL fluff, so center it normally.
				break;
			}

			// xoffs gets halved later, which centers the
			// string. If we don't want leading fluff to push
			// everything to the right, its full width needs
			// to be subtracted, so it's doubled here to
			// cancel out the division.
			xoffs *= 2;

			INT32 trim = -1;

			bool reached_end = scan(
				[&trim, &xoffs](int c)
				{
					if (isalnum(c))
					{
						trim = -1;
					}
					else if (trim < 0)
					{
						trim = xoffs;
					}

					return c;
				}
			);

			// Discount trailing fluff
			if (reached_end && trim >= 0)
			{
				xoffs = trim;
			}
		}
		else
		{
			scan([](int c) { return c; });
		}
	}
	while (*(ch++));

	if constexpr (Centered)
	{
		return xoffs / 2;
	}
	else
	{
		return xoffs;
	}
}

// V_TitleCardStringWidth
// Get the string's width using the titlecard font.
INT32 V_TitleCardStringWidth(const char *str, boolean p4)
{
	return Internal_TitleCardStringOffset<false>(str, p4);
}

// V_CenteredTitleCardStringOffset
// Subtract this offset from an X coordinate to center the string around that point.
INT32 V_CenteredTitleCardStringOffset(const char *str, boolean p4)
{
	return Internal_TitleCardStringOffset<true>(str, p4);
}

// V_DrawTitleCardStringFixed.
// see v_video.h's prototype for more information.
//
void V_DrawTitleCardStringFixed(fixed_t x, fixed_t y, fixed_t scale, const char *str, INT32 flags, boolean bossmode, INT32 timer, INT32 threshold, boolean p4)
{
	int bg_font = GTOL_FONT;
	int fg_font = GTFN_FONT;

	if (p4)
	{
		bg_font = GTOL4_FONT;
		fg_font = GTFN4_FONT;
	}

	INT32 xoffs = 0;
	INT32 yoffs = 0;
	INT32 i = 0;

	// per-letter variables
	fixed_t scalex;
	fixed_t offs;
	INT32 let_time;
	INT32 flipflag;
	angle_t fakeang;

	const char *ch = str;
	char c;
	patch_t *pp;
	patch_t *ol;

	x -= 2 * scale;	// Account for patch width...

	if (flags & V_SNAPTORIGHT)
	{
		x -= V_TitleCardStringWidth(str, p4) * scale;
	}

	for (;;ch++, i++)
	{

		scalex = FRACUNIT;
		offs = 0;
		let_time = timer - i;
		flipflag = 0;

		if (!*ch)
			break;

		if (*ch == '\n')
		{
			xoffs = x;
			yoffs += (p4 ? 18 : 32) * scale;

			continue;
		}

		c = *ch;

		c = toupper(c);
		c -= LT_FONTSTART;

		// check if character exists, if not, it's a space.
		if (c < 0 || c >= LT_FONTSIZE || !fontv[fg_font].font[(INT32)c])
		{
			xoffs += (p4 ? 5 : 10) * scale;
			continue;
		}

		ol = fontv[bg_font].font[(INT32)c];
		pp = fontv[fg_font].font[(INT32)c];

		if (bossmode)
		{
			if (let_time <= 0)
				return;
			if (threshold > 0)
			{
				if (threshold > 3)
					return;
				fakeang = (threshold*ANGLE_45)/2;
				scalex = FINECOSINE(fakeang>>ANGLETOFINESHIFT);
			}
			offs = ((FRACUNIT-scalex)*pp->width)/2;
		}
		else if (timer)
		{
			// make letters appear
			if (!threshold)
				;
			else if (let_time < threshold)
			{
				if (let_time <= 0)
					return;	// No reason to continue drawing, none of the next letters will be drawn either.

				// otherwise; scalex must start at 0
				// let's have each letter do 4 spins (360*4 + 90 = 1530 "degrees")
				fakeang = std::min<INT32>(360 + 90, let_time*41) * ANG1;
				scalex = FINESINE(fakeang>>ANGLETOFINESHIFT);
			}
			else if (!bossmode && let_time > threshold)
			{
				// Make letters disappear...
				let_time -= threshold;

				fakeang = std::max<INT32>(0, (360+90) - let_time*41)*ANG1;
				scalex = FINESINE(fakeang>>ANGLETOFINESHIFT);
			}

			// Because of how our patches are offset, we need to counter the displacement caused by changing the scale with an offset of our own.
			offs = ((FRACUNIT-scalex)*pp->width)/2;
		}

		// And now, we just need to draw the stuff.
		flipflag = (scalex < 0) ? V_FLIP : 0;

		if (scalex && ol && pp)
		{
			//CONS_Printf("%d\n", (INT32)c);
			V_DrawStretchyFixedPatch((x + xoffs) + offs, (y+yoffs), FixedMul(abs(scalex), scale), scale, flags|flipflag, ol, NULL);
			V_DrawStretchyFixedPatch((x + xoffs) + offs, (y+yoffs), FixedMul(abs(scalex), scale), scale, flags|flipflag, pp, NULL);
		}

		xoffs += (pp->width - (p4 ? 3 : 5)) * scale;
	}
}

static inline fixed_t FixedCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)scale;
	(void)hchw;
	(void)dupx;
	(*cwp) = chw;
	return 0;
}

static inline fixed_t VariableCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)chw;
	(void)hchw;
	(void)dupx;
	(*cwp) = FixedMul ((*cwp) << FRACBITS, scale);
	return 0;
}

static inline fixed_t CenteredCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	INT32 cxoff;
	/*
	For example, center a 4 wide patch to 8 width:
	4/2   = 2
	8/2   = 4
	4 - 2 = 2 (our offset)
	2 + 4 = 6 = 8 - 2 (equal space on either side)
	*/
	cxoff  = hchw -((*cwp) >> 1 );
	(*cwp) = chw;
	return FixedMul (( cxoff * dupx )<< FRACBITS, scale);
}

static inline fixed_t BunchedCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)chw;
	(void)hchw;
	(void)dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 1) << FRACBITS, scale);
	return 0;
}

static inline fixed_t MenuCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)chw;
	(void)hchw;
	(void)dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 2) << FRACBITS, scale);
	return 0;
}

static inline fixed_t GamemodeCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)chw;
	(void)hchw;
	(void)dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 2) << FRACBITS, scale);
	return 0;
}

static inline fixed_t FileCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)chw;
	(void)hchw;
	(void)dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 3) << FRACBITS, scale);
	return 0;
}

static inline fixed_t LSTitleCharacterDim(
		fixed_t  scale,
		fixed_t   chw,
		INT32    hchw,
		INT32    dupx,
		fixed_t *  cwp)
{
	(void)chw;
	(void)hchw;
	(void)dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 4) << FRACBITS, scale);
	return 0;
}

typedef struct
{
	fixed_t    chw;
	fixed_t spacew;
	fixed_t    lfh;
	fixed_t (*dim_fn)(fixed_t,fixed_t,INT32,INT32,fixed_t *);
	UINT8 button_yofs;
	UINT8 right_outline;
} fontspec_t;

static void V_GetFontSpecification(int fontno, INT32 flags, fontspec_t *result)
{
	/*
	Hardcoded until a better system can be implemented
	for determining how fonts space.
	*/

	// All other properties are guaranteed to be set
	result->chw = 0;
	result->button_yofs = 0;

	result->right_outline = 1;

	const INT32 spacing = ( flags & V_SPACINGMASK );

	switch (fontno)
	{
		default:
		case HU_FONT:
		case MENU_FONT:
			result->spacew = 4;
			switch (spacing)
			{
				case V_MONOSPACE:
					result->spacew = 8;
					/* FALLTHRU */
				case V_OLDSPACING:
					result->chw    = 8;
					break;
				case V_6WIDTHSPACE:
					result->spacew = 6;
					break;
			}
			break;
		case TINY_FONT:
		case TINYTIMER_FONT:
			result->spacew = 2;
			switch (spacing)
			{
				case V_MONOSPACE:
					result->spacew = 5;
					/* FALLTHRU */
				case V_OLDSPACING:
					result->chw    = 5;
					break;
				case V_6WIDTHSPACE:
					result->spacew = 3;
					break;
			}
			break;
		case MED_FONT:
			result->chw    = 6;
			result->spacew = 6;
			break;
		case LT_FONT:
			result->spacew = 12;
			break;
		case CRED_FONT:
			result->spacew = 16;
			break;
		case KART_FONT:
			result->spacew = 3;
			switch (spacing)
			{
				case V_MONOSPACE:
					result->spacew = 12;
					/* FALLTHRU */
				case V_OLDSPACING:
					result->chw    = 12;
					break;
				case V_6WIDTHSPACE:
					result->spacew = 6;
			}
			break;
		case GM_FONT:
			result->spacew = 6;
			break;
		case GENESIS_FONT:
			result->spacew = 8;
			result->right_outline = 0;
			break;
		case FILE_FONT:
			result->spacew = 0;
			break;
		case LSHI_FONT:
		case LSLOW_FONT:
			result->spacew = 10;
			break;
		case OPPRF_FONT:
			result->spacew = 5;
			break;
		case PINGF_FONT:
			result->spacew = 3;
			break;
		case ROLNUM_FONT:
			result->spacew = 17;
			break;
		case RO4NUM_FONT:
			result->spacew = 9;
			break;
	}

	switch (fontno)
	{
		default:
		case HU_FONT:
		case MENU_FONT:
		case TINY_FONT:
		case TINYTIMER_FONT:
		case KART_FONT:
		case MED_FONT:
			result->lfh = 12;
			break;
		case LT_FONT:
		case CRED_FONT:
		case FILE_FONT:
			result->lfh    = 12;
			break;
		case GM_FONT:
			result->lfh    = 32;
			break;
		case GENESIS_FONT:
			result->lfh    = 36;
			break;
		case LSHI_FONT:
			result->lfh    = 56;
			break;
		case LSLOW_FONT:
			result->lfh    = 38;
			break;
		case OPPRF_FONT:
		case PINGF_FONT:
			result->lfh = 10;
			break;
		case ROLNUM_FONT:
			result->lfh = 33;
			break;
		case RO4NUM_FONT:
			result->lfh = 15;
			break;
	}

	switch (fontno)
	{
		default:
			if (result->chw)
				result->dim_fn = CenteredCharacterDim;
			else
				result->dim_fn = VariableCharacterDim;
			break;
		case HU_FONT:
			if (result->chw)
				result->dim_fn = CenteredCharacterDim;
			else
				result->dim_fn = BunchedCharacterDim;
			break;
		case MENU_FONT:
			if (result->chw)
				result->dim_fn = CenteredCharacterDim;
			else
				result->dim_fn = MenuCharacterDim;
			break;
		case KART_FONT:
			if (result->chw)
				result->dim_fn = FixedCharacterDim;
			else
				result->dim_fn = BunchedCharacterDim;
			break;
		case TINY_FONT:
		case TINYTIMER_FONT:
			if (result->chw)
				result->dim_fn = FixedCharacterDim;
			else
				result->dim_fn = BunchedCharacterDim;
			break;
		case MED_FONT:
			result->dim_fn = FixedCharacterDim;
			break;
		case GM_FONT:
			if (result->chw)
				result->dim_fn = FixedCharacterDim;
			else
				result->dim_fn = GamemodeCharacterDim;
			break;
		case FILE_FONT:
			if (result->chw)
				result->dim_fn = FixedCharacterDim;
			else
				result->dim_fn = FileCharacterDim;
			break;
		case LSHI_FONT:
		case LSLOW_FONT:
			if (result->chw)
				result->dim_fn = FixedCharacterDim;
			else
				result->dim_fn = LSTitleCharacterDim;
			break;
		case OPPRF_FONT:
		case PINGF_FONT:
		case ROLNUM_FONT:
		case RO4NUM_FONT:
			if (result->chw)
				result->dim_fn = FixedCharacterDim;
			else
				result->dim_fn = BunchedCharacterDim;
			break;
	}

	switch (fontno)
	{
	case HU_FONT:
		result->button_yofs = 2;
		break;
	case MENU_FONT:
		result->button_yofs = 1;
		break;
	}

	switch (fontno)
	{
		case MENU_FONT:
			result->right_outline = 2;
			break;
	}
}

static UINT8 V_GetButtonCodeWidth(UINT8 c, boolean largebutton)
{
	UINT8 x = 14;

	switch (c & 0x0F)
	{
	case sb_up:
	case sb_down:
	case sb_left:
	case sb_right:
		x -= largebutton ? 2 : 4;
		break;

	case sb_l:
	case sb_r:
		x -= largebutton ? 1 : 4;
		break;

	case sb_start:
		x -= largebutton ? 0 : 4;
		break;

	case sb_lua1:
	case sb_lua2:
	case sb_lua3:
		x -= largebutton ? 0 : 4;
		break;

	case sb_a:
	case sb_b:
	case sb_c:
	case sb_x:
	case sb_y:
	case sb_z:
		x -= largebutton ? 0 : 4;
		break;
	}

	return x;
}

static UINT8 V_GetGenericButtonCodeWidth(UINT8 c, boolean largebutton)
{
	UINT8 x = 16;

	switch ((c & 0x0F) | gb_mask)
	{
	case gb_a:
	case gb_b:
	case gb_x:
	case gb_y:
		x -= largebutton ? 0 : 2;
		break;

	case gb_lb:
	case gb_rb:
		x -= largebutton ? 2 : 6;
		break;

	case gb_lt:
	case gb_rt:
		x -= largebutton ? 2 : 6;
		break;

	case gb_start:
		x -= largebutton ? 2 : 6;
		break;

	case gb_back:
		x -= largebutton ? 2 : 6;
		break;

	case gb_ls:
	case gb_rs:
		x -= largebutton ? 1 : 4;
		break;

	case gb_dpad:
		x -= largebutton ? 2 : 5;
		break;
	}

	return x;
}

void V_DrawStringScaled(
		fixed_t    x,
		fixed_t    y,
		fixed_t      scale,
		fixed_t spacescale,
		fixed_t    lfscale,
		INT32      flags,
		const UINT8 *colormap,
		int        fontno,
		const char *s)
{
	INT32     hchw;/* half-width for centering */

	INT32     dupx;
	INT32     dupy;

	fixed_t  right;
	fixed_t    bot;

	font_t   *font;

	boolean uppercase;
	boolean notcolored;
	UINT8 boxed = 0;
	boolean descriptive = false;

	boolean debugalternation = false;
	UINT8 debugcolor1 = 181;
	UINT8 debugcolor2 = 96;

	boolean   dance;
	boolean nodanceoverride;
	INT32     dancecounter;

	INT32 boxedflags = ((flags) & (~V_HUDTRANS)) | (V_40TRANS);

	boolean largebutton = false;

	fixed_t cx, cy;
	fixed_t cxsave;

	const char *ssave;

	fixed_t cxoff, cyoff;
	fixed_t cw;

	fixed_t   left;

	int c;

	uppercase  = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags	&= ~(V_FLIP);/* These two (V_FORCEUPPERCASE) share a bit. */

	dance           = (flags & V_STRINGDANCE) != 0;
	nodanceoverride = !dance;
	dancecounter    = 0;

	/* Some of these flags get overloaded in this function so
	   don't pass them on. */
	flags &= ~(V_PARAMMASK);

	if (colormap == NULL)
	{
		colormap   =  V_GetStringColormap(( flags & V_CHARCOLORMASK ));
	}

	notcolored = !colormap;

	font       = &fontv[fontno];

	fontspec_t fontspec;

	V_GetFontSpecification(fontno, flags, &fontspec);

	hchw     = fontspec.chw >> 1;

	fontspec.chw    <<= FRACBITS;
	fontspec.spacew <<= FRACBITS;
	fontspec.lfh    <<= FRACBITS;

#define Mul( id, scale ) ( id = FixedMul (scale, id) )
	Mul    (fontspec.chw,      scale);
	Mul (fontspec.spacew,      scale);
	Mul    (fontspec.lfh,      scale);

	Mul (fontspec.spacew, spacescale);
	Mul    (fontspec.lfh,    lfscale);
#undef  Mul

	if (( flags & V_NOSCALESTART ))
	{
		dupx      = vid.dupx;
		dupy      = vid.dupy;

		hchw     *=     dupx;

		fontspec.chw      *=     dupx;
		fontspec.spacew   *=     dupx;
		fontspec.lfh      *=     dupy;

		right     = vid.width;
	}
	else
	{
		dupx      = 1;
		dupy      = 1;

		right     = ( vid.width / vid.dupx );
		if (!( flags & V_SNAPTOLEFT ))
		{
			left   = ( right - BASEVIDWIDTH )/ 2;/* left edge of drawable area */
			right -= left;
		}
	}

	right      <<=               FRACBITS;
	bot          = vid.height << FRACBITS;

	ssave = s;
	cx = cxsave = x;
	cy = y;
	cyoff = 0;

	for (; ( c = *s ); ++s, ++dancecounter)
	{
		switch (c)
		{
			case '\n':
				if (boxed)
					continue;
				cy += fontspec.lfh;
				if (cy >= bot)
					return;
				cx  =   x;
				break;
			case '\xEB':
				if (fontno != TINY_FONT && fontno != HU_FONT)
					largebutton = true;
				break;
			case '\xEF':
				descriptive = true;
				break;
			case '\xEE':
			case '\xED':
			case '\xEC':
			{
				UINT8 anim_duration = 16;
				UINT8 anim = 0;

				if (c == '\xEC') // Pressed
					anim = 1;
				else if (c != '\xEE') // Not lifted..?
					anim = ((I_GetTime() % (anim_duration * 2)) < anim_duration) ? 1 : 0;

				// For bullshit text outlining reasons, we cannot draw this background character-by-character.
				// Thinking about doing string manipulation and calling out to V_StringWidth made me drink water.
				// So instead, we just draw this section of the string twice—invisibly the first time, to measure the width.

				if (boxed == 0) // Save our position and start no-op drawing
				{
					cy -= 2*FRACUNIT;

					Draw(FixedToFloat(cx), FixedToFloat(cy)-3).flags(flags).patch(gen_button_keyleft[anim]);

					cx += 3*FRACUNIT;
					ssave = s;
					cxsave = cx;

					boxed = 1;
				}
				else if (boxed == 1) // Draw box from saved pos to current pos and roll back
				{
					cx += (fontspec.right_outline)*FRACUNIT;
					fixed_t working = cxsave - 1*FRACUNIT;

					Draw(FixedToFloat(working)+1, FixedToFloat(cy)-3)
						.width(FixedToFloat(cx - working)-1)
						.flags(flags)
						.stretch(Draw::Stretch::kWidth).patch(gen_button_keycenter[anim]);
					Draw(FixedToFloat(cx), FixedToFloat(cy)-3).flags(flags).patch(gen_button_keyright[anim]);

					s = ssave;
					cx = cxsave;

					// This is a little gross, but this is our way of smuggling text offset to
					// the standard character drawing case. boxed=3 means we're drawing a pressed button.
					boxed = 2 + anim;
				}
				else // Meeting the ending tag the second time, space away and resume standard parsing
				{
					boxed = 0;

					cx += (3)*FRACUNIT;
					cy += 2*FRACUNIT;
				}

				break;
			}
			default:
				if (( c & 0xF0 ) == 0x80)
				{
					if (notcolored)
					{
						colormap = V_GetStringColormap(
								( ( c & 0x7f )<< V_CHARCOLORSHIFT )&
								V_CHARCOLORMASK);
					}
					if (nodanceoverride)
					{
						dance = false;
						cyoff = 0;
					}
				}
				else if (c == V_STRINGDANCE)
				{
					dance = true;
				}
				else if (cx < right)
				{
					if (uppercase)
					{
						c = toupper(c);
					}
					else if (V_CharacterValid(font, c - font->start) == false)
					{
						// Try the other case if it doesn't exist
						if (c >= 'A' && c <= 'Z')
						{
							c = tolower(c);
						}
						else if (c >= 'a' && c <= 'z')
						{
							c = toupper(c);
						}
					}


					if (dance)
					{
						cyoff = V_DanceYOffset(dancecounter) * FRACUNIT;
					}

					if (( c & 0xB0 ) & 0x80) // button prompts
					{
						if (!descriptive)
						{
							using srb2::Draw;

							struct BtConf
							{
								UINT8 x, y;
								Draw::Button type;
							};

							auto bt_inst = [c]() -> std::optional<BtConf>
							{
								switch (c & 0x0F)
								{
								case sb_up: return {{2, 2, Draw::Button::up}};
								case sb_down: return {{2, 2, Draw::Button::down}};
								case sb_right: return {{2, 2, Draw::Button::right}};
								case sb_left: return {{2, 2, Draw::Button::left}};

								case sb_lua1: return {{2, 2, Draw::Button::lua1}};
								case sb_lua2: return {{2, 2, Draw::Button::lua2}};
								case sb_lua3: return {{2, 2, Draw::Button::lua3}};

								case sb_r: return {{2, 2, Draw::Button::r}};
								case sb_l: return {{2, 2, Draw::Button::l}};

								case sb_start: return {{2, 2, Draw::Button::start}};

								case sb_a: return {{2, 2, Draw::Button::a}};
								case sb_b: return {{2, 2, Draw::Button::b}};
								case sb_c: return {{2, 2, Draw::Button::c}};

								case sb_x: return {{2, 2, Draw::Button::x}};
								case sb_y: return {{2, 2, Draw::Button::y}};
								case sb_z: return {{2, 2, Draw::Button::z}};

								default: return {};
								}
							}();

							if (largebutton)
							{
								bt_inst = [c]() -> std::optional<BtConf>
								{
									switch (c & 0x0F)
									{
									case sb_up: return {{2, 4, Draw::Button::up}};
									case sb_down: return {{2, 4, Draw::Button::down}};
									case sb_right: return {{2, 4, Draw::Button::right}};
									case sb_left: return {{2, 4, Draw::Button::left}};

									case sb_lua1: return {{1, 4, Draw::Button::lua1}};
									case sb_lua2: return {{1, 4, Draw::Button::lua2}};
									case sb_lua3: return {{1, 4, Draw::Button::lua3}};

									case sb_r: return {{1, 4, Draw::Button::r}};
									case sb_l: return {{1, 4, Draw::Button::l}};

									case sb_start: return {{1, 4, Draw::Button::start}};

									case sb_a: return {{1, 4, Draw::Button::a}};
									case sb_b: return {{1, 4, Draw::Button::b}};
									case sb_c: return {{1, 4, Draw::Button::c}};

									case sb_x: return {{1, 4, Draw::Button::x}};
									case sb_y: return {{1, 4, Draw::Button::y}};
									case sb_z: return {{1, 4, Draw::Button::z}};

									default: return {};
									}
								}();
							}

							if (bt_inst)
							{
								auto bt_translate_press = [c]() -> std::optional<bool>
								{
									switch (c & 0xB0)
									{
									default:
									case 0x90: return true;
									case 0xA0: return {};
									case 0xB0: return false;
									}
								};

								cw = V_GetButtonCodeWidth(c, largebutton) * dupx;

								cxoff = (*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);

								if (cv_debugfonts.value)
								{
									V_DrawFill(cx/FRACUNIT, cy/FRACUNIT, cw/FRACUNIT, fontspec.lfh/FRACUNIT, debugalternation ? debugcolor1 : debugcolor2);
									debugalternation = !debugalternation;
								}

								Draw bt = Draw(
									FixedToFloat(cx + cxoff) - (bt_inst->x * dupx),
									FixedToFloat(cy + cyoff) - ((bt_inst->y + fontspec.button_yofs) * dupy))
									.flags(flags);

								if (largebutton)
									bt.button(bt_inst->type, bt_translate_press());
								else
									bt.small_button(bt_inst->type, bt_translate_press());

								cx += cw;
							}
							descriptive = false;
							largebutton = false;
							break;
						}
						else
						{
							using srb2::Draw;

							struct BtConf
							{
								UINT8 x, y;
								Draw::GenericButton type;
							};

							auto bt_inst = [c]() -> std::optional<BtConf>
							{
								switch ((c & 0x0F) | gb_mask)
								{
								case gb_a: return {{0, 1, Draw::GenericButton::a}};
								case gb_b: return {{0, 1, Draw::GenericButton::b}};
								case gb_x: return {{0, 1, Draw::GenericButton::x}};
								case gb_y: return {{0, 1, Draw::GenericButton::y}};
								case gb_lb: return {{2, 2, Draw::GenericButton::lb}};
								case gb_rb: return {{2, 2, Draw::GenericButton::rb}};
								case gb_lt: return {{2, 2, Draw::GenericButton::lt}};
								case gb_rt: return {{2, 2, Draw::GenericButton::rt}};
								case gb_start: return {{2, 2, Draw::GenericButton::start}};
								case gb_back: return {{2, 2, Draw::GenericButton::back}};
								case gb_ls: return {{1, 2, Draw::GenericButton::ls}};
								case gb_rs: return {{1, 2, Draw::GenericButton::rs}};
								case gb_dpad: return {{2, 2, Draw::GenericButton::dpad}};
								default: return {};
								}
							}();

							if (largebutton)
							{
								bt_inst = [c]() -> std::optional<BtConf>
								{
									switch ((c & 0x0F) | gb_mask)
									{
									case gb_a: return {{0, 3, Draw::GenericButton::a}};
									case gb_b: return {{0, 3, Draw::GenericButton::b}};
									case gb_x: return {{0, 3, Draw::GenericButton::x}};
									case gb_y: return {{0, 3, Draw::GenericButton::y}};
									case gb_lb: return {{1, 3, Draw::GenericButton::lb}};
									case gb_rb: return {{1, 3, Draw::GenericButton::rb}};
									case gb_lt: return {{1, 4, Draw::GenericButton::lt}};
									case gb_rt: return {{1, 4, Draw::GenericButton::rt}};
									case gb_start: return {{1, 6, Draw::GenericButton::start}};
									case gb_back: return {{1, 6, Draw::GenericButton::back}};
									case gb_ls: return {{1, 5, Draw::GenericButton::ls}};
									case gb_rs: return {{1, 5, Draw::GenericButton::rs}};
									case gb_dpad: return {{1, 4, Draw::GenericButton::dpad}};
									default: return {};
									}
								}();
							}

							if (bt_inst)
							{
								auto bt_translate_press = [c]() -> std::optional<bool>
								{
									switch (c & 0xB0)
									{
									default:
									case 0x90: return true;
									case 0xA0: return {};
									case 0xB0: return false;
									}
								};

								cw = V_GetGenericButtonCodeWidth(c, largebutton) * dupx;

								cxoff = (*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);

								if (cv_debugfonts.value)
								{
									V_DrawFill(cx/FRACUNIT, cy/FRACUNIT, cw/FRACUNIT, fontspec.lfh/FRACUNIT, debugalternation ? debugcolor1 : debugcolor2);
									debugalternation = !debugalternation;
								}

								Draw bt = Draw(
									FixedToFloat(cx + cxoff) - (bt_inst->x * dupx),
									FixedToFloat(cy + cyoff) - ((bt_inst->y + fontspec.button_yofs) * dupy))
									.flags(flags);

								if (largebutton)
									bt.generic_button(bt_inst->type, bt_translate_press());
								else
									bt.generic_small_button(bt_inst->type, bt_translate_press());

								cx += cw;
							}
							descriptive = false;
							largebutton = false;
							break;
						}
						break;
					}

					c -= font->start;
					if (V_CharacterValid(font, c) == true)
					{
						// Remove offsets from patch
						fixed_t patchxofs = SHORT (font->font[c]->leftoffset) * dupx * scale;
						cw = SHORT (font->font[c]->width) * dupx;
						cxoff = (*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);

						if (cv_debugfonts.value)
						{
							V_DrawFill(cx/FRACUNIT, cy/FRACUNIT, cw/FRACUNIT, fontspec.lfh/FRACUNIT, debugalternation ? debugcolor1 : debugcolor2);
							debugalternation = !debugalternation;
						}

						if (boxed != 1)
						{
							V_DrawFixedPatch(cx + cxoff + patchxofs, cy + cyoff + (boxed == 3 ? 2*FRACUNIT : 0), scale,
								boxed ? boxedflags : flags, font->font[c], boxed ? 0 : colormap);
						}

						cx += cw;
					}
					else
						cx += fontspec.spacew;
				}
		}
	}
}

fixed_t V_StringScaledWidth(
		fixed_t      scale,
		fixed_t spacescale,
		fixed_t    lfscale,
		INT32      flags,
		int        fontno,
		const char *s)
{
	INT32     hchw;/* half-width for centering */

	INT32     dupx;

	font_t   *font;

	boolean uppercase;
	boolean boxed = false;
	boolean descriptive = false;
	boolean largebutton = false;

	fixed_t cx;
	fixed_t right;

	fixed_t cw;

	int c;

	fixed_t fullwidth = 0;

	uppercase  = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags	&= ~(V_FLIP);/* These two (V_FORCEUPPERCASE) share a bit. */

	font       = &fontv[fontno];

	fontspec_t fontspec;

	V_GetFontSpecification(fontno, flags, &fontspec);

	hchw     = fontspec.chw >> 1;

	fontspec.chw    <<= FRACBITS;
	fontspec.spacew <<= FRACBITS;

#define Mul( id, scale ) ( id = FixedMul (scale, id) )
	Mul    (fontspec.chw,      scale);
	Mul (fontspec.spacew,      scale);
	Mul    (fontspec.lfh,      scale);

	Mul (fontspec.spacew, spacescale);
	Mul    (fontspec.lfh,    lfscale);
#undef  Mul

	if (( flags & V_NOSCALESTART ))
	{
		dupx      = vid.dupx;

		hchw     *=     dupx;

		fontspec.chw      *=     dupx;
		fontspec.spacew   *=     dupx;
		fontspec.lfh      *= vid.dupy;
	}
	else
	{
		dupx      = 1;
	}

	cx = 0;
	right = 0;

	for (; ( c = *s ); ++s)
	{
		switch (c)
		{
			case '\n':
				cx  =   0;
				break;
			case '\xEB':
				if (fontno != TINY_FONT && fontno != HU_FONT)
					largebutton = true;
				break;
			case '\xEF':
				descriptive = true;
				break;
			case '\xEE':
			case '\xED':
			case '\xEC':
				if (boxed)
					cx += 3*FRACUNIT;
				else
					cx += 3*FRACUNIT;
				boxed = !boxed;
				break;
			default:
				if (( c & 0xF0 ) == 0x80 || c == V_STRINGDANCE)
					continue;

				if (( c & 0xB0 ) & 0x80)
				{
					if (descriptive)
					{
						cw = V_GetGenericButtonCodeWidth(c, largebutton) * dupx;
						cx += cw * scale;
						right = cx;
					}
					else
					{
						cw = V_GetButtonCodeWidth(c, largebutton) * dupx;
						cx += cw * scale;
						right = cx;
					}

					largebutton = false;
					descriptive = false;
					break;
				}

				if (uppercase)
				{
					c = toupper(c);
				}
				else if (V_CharacterValid(font, c - font->start) == false)
				{
					// Try the other case if it doesn't exist
					if (c >= 'A' && c <= 'Z')
					{
						c = tolower(c);
					}
					else if (c >= 'a' && c <= 'z')
					{
						c = toupper(c);
					}
				}

				c -= font->start;
				if (V_CharacterValid(font, c) == true)
				{
					cw = SHORT (font->font[c]->width) * dupx;

					// How bunched dims work is by incrementing cx slightly less than a full character width.
					// This causes the next character to be drawn overlapping the previous.
					// We need to count the full width to get the rightmost edge of the string though.
					right = cx + (cw * scale);

					(*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);
					cx += cw;
				}
				else
					cx += fontspec.spacew;
				descriptive = false;
		}

		fullwidth = std::max(right, std::max(cx, fullwidth));
	}

	return fullwidth;
}

// Modify a string to wordwrap at any given width.
char * V_ScaledWordWrap(
		fixed_t          w,
		fixed_t      scale,
		fixed_t spacescale,
		fixed_t    lfscale,
		INT32      flags,
		int        fontno,
		const char *s)
{
	INT32     hchw;/* half-width for centering */

	INT32     dupx;

	font_t   *font;

	boolean uppercase;
	boolean largebutton = false;
	boolean descriptive = false;
	boolean boxed = false;

	fixed_t cx;
	fixed_t right;

	fixed_t cw;

	int c;

	uppercase  = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags	&= ~(V_FLIP);/* These two (V_FORCEUPPERCASE) share a bit. */

	font       = &fontv[fontno];

	fontspec_t fontspec;

	V_GetFontSpecification(fontno, flags, &fontspec);

	hchw     = fontspec.chw >> 1;

	fontspec.chw    <<= FRACBITS;
	fontspec.spacew <<= FRACBITS;

#define Mul( id, scale ) ( id = FixedMul (scale, id) )
	Mul    (fontspec.chw,      scale);
	Mul (fontspec.spacew,      scale);
	Mul    (fontspec.lfh,      scale);

	Mul (fontspec.spacew, spacescale);
	Mul    (fontspec.lfh,    lfscale);
#undef  Mul

	if (( flags & V_NOSCALESTART ))
	{
		dupx      = vid.dupx;

		hchw     *=     dupx;

		fontspec.chw      *=     dupx;
		fontspec.spacew   *=     dupx;
		fontspec.lfh      *= vid.dupy;
	}
	else
	{
		dupx      = 1;
	}

	cx = 0;
	right = 0;

	size_t reader = 0, writer = 0, startwriter = 0;
	fixed_t cxatstart = 0;

	size_t len = strlen(s) + 1;
	size_t potentialnewlines = 8;
	size_t sparenewlines = potentialnewlines;

	char *newstring = static_cast<char *>(Z_Malloc(len + sparenewlines, PU_STATIC, NULL));

	for (; ( c = s[reader] ); ++reader, ++writer)
	{
		newstring[writer] = s[reader];

		right = 0;

		switch (c)
		{
			case '\n':
				cx  =   0;
				cxatstart = 0;
				startwriter = 0;
				break;
			case '\xEB':
				if (fontno != TINY_FONT && fontno != HU_FONT)
					largebutton = true;
			case '\xEF':
				descriptive = true;
				break;
			case '\xEE':
			case '\xED':
			case '\xEC':
				if (boxed)
					cx += 3*FRACUNIT;
				else
					cx += 3*FRACUNIT;
				boxed = !boxed;
				break;
			default:
				if (( c & 0xF0 ) == 0x80 || c == V_STRINGDANCE)
					;
				else if (( c & 0xB0 ) & 0x80) // button prompts
				{
					if (descriptive)
						cw = V_GetGenericButtonCodeWidth(c, largebutton) * dupx;
					else
						cw = V_GetButtonCodeWidth(c, largebutton) * dupx;

					cx += cw * scale;
					right = cx;

					descriptive = false;
					boxed = false;
				}
				else
				{
					if (uppercase)
					{
						c = toupper(c);
					}
					else if (V_CharacterValid(font, c - font->start) == false)
					{
						// Try the other case if it doesn't exist
						if (c >= 'A' && c <= 'Z')
						{
							c = tolower(c);
						}
						else if (c >= 'a' && c <= 'z')
						{
							c = toupper(c);
						}
					}

					c -= font->start;
					if (V_CharacterValid(font, c) == true)
					{
						cw = SHORT (font->font[c]->width) * dupx;

						// How bunched dims work is by incrementing cx slightly less than a full character width.
						// This causes the next character to be drawn overlapping the previous.
						// We need to count the full width to get the rightmost edge of the string though.
						right = cx + (cw * scale);

						(*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);
						cx += cw;
					}
					else
					{
						cx += fontspec.spacew;
						cxatstart = cx;
						startwriter = writer;
					}
				}
		}

		// Start trying to wrap if presumed length exceeds the space we have on-screen.
		if (right && right > w)
		{
			if (startwriter != 0)
			{
				newstring[startwriter] = '\n';
				cx -= cxatstart;
				cxatstart = 0;
				startwriter = 0;
			}
			else
			{
				if (sparenewlines == 0)
				{
					sparenewlines = (potentialnewlines *= 2);
					newstring = static_cast<char *>(Z_Realloc(newstring, len + sparenewlines, PU_STATIC, NULL));
				}

				sparenewlines--;
				len++;

				newstring[writer++] = '\n'; // Over-write previous
				cx = cw; // Valid value in the only case right is currently set
				newstring[writer] = s[reader]; // Re-add
			}
		}
	}

	newstring[writer] = '\0';

	return newstring;
}

void V_DrawCenteredString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_StringWidth(string, option)/2;
	V_DrawString(x, y, option, string);
}

void V_DrawRightAlignedString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_StringWidth(string, option);
	V_DrawString(x, y, option, string);
}

void V_DrawCenteredSmallString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_SmallStringWidth(string, option)/2;
	V_DrawSmallString(x, y, option, string);
}

void V_DrawRightAlignedSmallString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_SmallStringWidth(string, option);
	V_DrawSmallString(x, y, option, string);
}

void V_DrawCenteredThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option)/2;
	V_DrawThinString(x, y, option, string);
}

void V_DrawRightAlignedThinString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option);
	V_DrawThinString(x, y, option, string);
}

void V_DrawCenteredStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= (V_ThinStringWidth(string, option) / 2) * FRACUNIT;
	V_DrawThinStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_StringWidth(string, option) * FRACUNIT;
	V_DrawStringAtFixed(x, y, option, string);
}

void V_DrawCenteredThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= (V_StringWidth(string, option) / 2) * FRACUNIT;
	V_DrawStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option) * FRACUNIT;
	V_DrawThinStringAtFixed(x, y, option, string);
}

// Draws a number using the PING font thingy.
// TODO: Merge number drawing functions into one with "font name" selection.

fixed_t V_DrawPingNum(fixed_t x, fixed_t y, INT32 flags, INT32 num, const UINT8 *colormap)
{
	// this SHOULD always be 5 but I guess custom graphics exist.
	const fixed_t w = (fontv[PINGNUM_FONT].font[0]->width) * FRACUNIT;

	if (num < 0)
	{
		num = -num;
	}

	do // draw the number
	{
		x -= (w - FRACUNIT); // Oni wanted their outline to intersect.
		V_DrawFixedPatch(x, y, FRACUNIT, flags, fontv[PINGNUM_FONT].font[num % 10], colormap);
		num /= 10;
	} while (num > 0);

	return x;
}

void V_DrawCenteredTimerString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_TimerStringWidth(string, option)/2;
	V_DrawTimerString(x, y, option, string);
}

void V_DrawRightAlignedTimerString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_TimerStringWidth(string, option);
	V_DrawTimerString(x, y, option, string);
}

void V_DrawCenteredMenuString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_MenuStringWidth(string, option)/2;
	V_DrawMenuString(x, y, option, string);
}

void V_DrawRightAlignedMenuString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_MenuStringWidth(string, option);
	V_DrawMenuString(x, y, option, string);
}

void V_DrawCenteredGamemodeString(INT32 x, INT32 y, INT32 option, const UINT8 *colormap, const char *string)
{
	x -= V_GamemodeStringWidth(string, option)/2;
	V_DrawGamemodeString(x, y, option, colormap, string);
}

void V_DrawRightAlignedGamemodeString(INT32 x, INT32 y, INT32 option, const UINT8 *colormap, const char *string)
{
	x -= V_GamemodeStringWidth(string, option);
	V_DrawGamemodeString(x, y, option, colormap, string);
}

void V_DrawCenteredFileString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_FileStringWidth(string, option)/2;
	V_DrawFileString(x, y, option, string);
}

void V_DrawRightAlignedFileString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_FileStringWidth(string, option);
	V_DrawFileString(x, y, option, string);
}

void V_DrawCenteredLSTitleHighString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_LSTitleHighStringWidth(string, option)/2;
	V_DrawLSTitleHighString(x, y, option, string);
}

void V_DrawRightAlignedLSTitleHighString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_LSTitleHighStringWidth(string, option);
	V_DrawLSTitleHighString(x, y, option, string);
}

void V_DrawCenteredLSTitleLowString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_LSTitleLowStringWidth(string, option)/2;
	V_DrawLSTitleLowString(x, y, option, string);
}

void V_DrawRightAlignedLSTitleLowString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_LSTitleLowStringWidth(string, option);
	V_DrawLSTitleLowString(x, y, option, string);
}

// Draws a tallnum.  Replaces two functions in y_inter and st_stuff
void V_DrawTallNum(INT32 x, INT32 y, INT32 flags, INT32 num)
{
	INT32 w = SHORT(fontv[TALLNUM_FONT].font[0]->width);
	boolean neg;

	if (flags & V_NOSCALESTART)
		w *= vid.dupx;

	if ((neg = num < 0))
		num = -num;

	// draw the number
	do
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, fontv[TALLNUM_FONT].font[num % 10]);
		num /= 10;
	} while (num);

	// draw a minus sign if necessary
	//if (neg)
		//V_DrawScaledPatch(x - w, y, flags, tallminus); // Tails
}

// Draws a number with a set number of digits.
// Does not handle negative numbers in a special way, don't try to feed it any.
void V_DrawPaddedTallNum(INT32 x, INT32 y, INT32 flags, INT32 num, INT32 digits)
{
	INT32 w = fontv[TALLNUM_FONT].font[0]->width;

	if (flags & V_NOSCALESTART)
		w *= vid.dupx;

	if (num < 0)
		num = -num;

	// draw the number
	do
	{
		x -= w;
		V_DrawScaledPatch(x, y, flags, fontv[TALLNUM_FONT].font[num % 10]);
		num /= 10;
	} while (--digits);
}

void V_DrawProfileNum(INT32 x, INT32 y, INT32 flags, UINT8 num)
{
	UINT8 digits = 3;
	INT32 w = fontv[PROFNUM_FONT].font[0]->width;

	if (flags & V_NOSCALESTART)
		w *= vid.dupx;

	// draw the number
	do
	{
		x -= (w-1);
		V_DrawScaledPatch(x, y, flags, fontv[PROFNUM_FONT].font[num % 10]);
		num /= 10;
	} while (--digits);
}

// Find max height of the string
//
INT32 V_LevelNameHeight(const char *string)
{
	INT32 c, w = 0;
	size_t i;

	for (i = 0; string[i]; i++)
	{
		c = string[i] - LT_FONTSTART;
		if (c < 0 || c >= LT_FONTSIZE || !fontv[LT_FONT].font[c])
			continue;

		if (fontv[LT_FONT].font[c]->height > w)
			w = fontv[LT_FONT].font[c]->height;
	}

	return w;
}

// Generates a RGB565 color look-up table
void InitColorLUT(colorlookup_t *lut, RGBA_t *palette, boolean makecolors)
{
	size_t palsize = (sizeof(RGBA_t) * 256);

	if (!lut->init || memcmp(lut->palette, palette, palsize))
	{
		INT32 i;

		lut->init = true;
		memcpy(lut->palette, palette, palsize);

		for (i = 0; i < 0xFFFF; i++)
			lut->table[i] = 0xFFFF;

		if (makecolors)
		{
			UINT8 r, g, b;

			for (r = 0; r < 0xFF; r++)
			for (g = 0; g < 0xFF; g++)
			for (b = 0; b < 0xFF; b++)
			{
				i = CLUTINDEX(r, g, b);
				if (lut->table[i] == 0xFFFF)
					lut->table[i] = NearestPaletteColor(r, g, b, palette);
			}
		}
	}
}

UINT8 GetColorLUT(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b)
{
	INT32 i = CLUTINDEX(r, g, b);
	if (lut->table[i] == 0xFFFF)
		lut->table[i] = NearestPaletteColor(r, g, b, lut->palette);
	return lut->table[i];
}

UINT8 GetColorLUTDirect(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b)
{
	INT32 i = CLUTINDEX(r, g, b);
	return lut->table[i];
}

// V_Init
// old software stuff, buffers are allocated at video mode setup
// here we set the screens[x] pointers accordingly
// WARNING: called at runtime (don't init cvar here)
void V_Init(void)
{
	INT32 i;
	UINT8 *base = vid.buffer;
	const INT32 screensize = vid.rowbytes * vid.height;

	for (i = 0; i < NUMSCREENS; i++)
		screens[i] = NULL;

	// start address of NUMSCREENS * width*height vidbuffers
	if (base)
	{
		for (i = 0; i < NUMSCREENS; i++)
			screens[i] = base + i*screensize;
	}

	if (vid.direct)
		screens[0] = vid.direct;

#ifdef DEBUG
	CONS_Debug(DBG_RENDER, "V_Init done:\n");
	for (i = 0; i < NUMSCREENS; i++)
		CONS_Debug(DBG_RENDER, " screens[%d] = %x\n", i, screens[i]);
#endif
}

void V_Recalc(void)
{
	// scale 1,2,3 times in x and y the patches for the menus and overlays...
	// calculated once and for all, used by routines in v_video.c and v_draw.c
	vid.dupx = vid.width / BASEVIDWIDTH;
	vid.dupy = vid.height / BASEVIDHEIGHT;
	vid.dupx = vid.dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	vid.fdupx = FixedDiv(vid.width*FRACUNIT, BASEVIDWIDTH*FRACUNIT);
	vid.fdupy = FixedDiv(vid.height*FRACUNIT, BASEVIDHEIGHT*FRACUNIT);

#ifdef HWRENDER
	//if (rendermode != render_opengl && rendermode != render_none) // This was just placing it incorrectly at non aspect correct resolutions in opengl
	// 13/11/18:
	// The above is no longer necessary, since we want OpenGL to be just like software now
	// -- Monster Iestyn
#endif
		vid.fdupx = vid.fdupy = (vid.fdupx < vid.fdupy ? vid.fdupx : vid.fdupy);

	vid.meddupx = (UINT8)(vid.dupx >> 1) + 1;
	vid.meddupy = (UINT8)(vid.dupy >> 1) + 1;
#ifdef HWRENDER
	vid.fmeddupx = vid.meddupx*FRACUNIT;
	vid.fmeddupy = vid.meddupy*FRACUNIT;
#endif

	vid.smalldupx = (UINT8)(vid.dupx / 3) + 1;
	vid.smalldupy = (UINT8)(vid.dupy / 3) + 1;
#ifdef HWRENDER
	vid.fsmalldupx = vid.smalldupx*FRACUNIT;
	vid.fsmalldupy = vid.smalldupy*FRACUNIT;
#endif
}

void VID_DisplaySoftwareScreen()
{
	ZoneScoped;

	// TODO implement
	// upload framebuffer, bind pipeline, draw
	rhi::Rhi* rhi = srb2::sys::get_rhi(srb2::sys::g_current_rhi);
	hwr2::HardwareState* hw_state = srb2::sys::main_hardware_state();

	// Misnomer; this just uploads the screen to the software indexed screen texture
	hw_state->software_screen_renderer->draw(*rhi);

	const int screens = std::clamp(r_splitscreen + 1, 1, MAXSPLITSCREENPLAYERS);
	hw_state->blit_postimg_screens->set_num_screens(screens);
	hw_state->blit_postimg_screens->set_target(static_cast<uint32_t>(vid.width), static_cast<uint32_t>(vid.height));

	for (int i = 0; i < screens; i++)
	{
		glm::vec2 uv_offset {0.f, 0.f};
		glm::vec2 uv_size {1.f, 1.f};

		if (screens > 2)
		{
			uv_size = glm::vec2(.5f, .5f);
			switch (i)
			{
			case 0:
				uv_offset = glm::vec2(0.f, 0.f);
				break;
			case 1:
				uv_offset = glm::vec2(.5f, 0.f);
				break;
			case 2:
				uv_offset = glm::vec2(0.f, .5f);
				break;
			case 3:
				uv_offset = glm::vec2(.5f, .5f);
				break;
			}
		}
		else if (screens > 1)
		{
			uv_size = glm::vec2(1.f, .5f);
			if (i == 1)
			{
				uv_offset = glm::vec2(0.f, .5f);
			}
		}

		hw_state->blit_postimg_screens->set_screen(
			i,
			{
				hw_state->software_screen_renderer->screen(),
				true,
				uv_offset,
				uv_size,
				{
					postimgtype[i] == postimg_water && !cv_reducevfx.value,
					postimgtype[i] == postimg_heat && !cv_reducevfx.value,
					postimgtype[i] == postimg_flip,
					postimgtype[i] == postimg_mirror
				}
			}
		);
	}

	// Post-process blit to the 'default' framebuffer
	hw_state->blit_postimg_screens->draw(*rhi);
}

char *V_ParseText(const char *rawText)
{
	using srb2::Draw;

	return Z_StrDup(srb2::Draw::TextElement().parse(rawText).string().c_str());
}
