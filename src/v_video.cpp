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
/// \file  v_video.c
/// \brief Gamma correction LUT stuff
///        Functions to draw patches (by post) directly to screen.
///        Functions to blit a block to the screen.

#include <cmath>

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

#include "i_video.h" // rendermode
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "doomstat.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

// SRB2Kart
#include "k_hud.h"
#include "k_boss.h"
#include "i_time.h"

// Each screen is [vid.width*vid.height];
UINT8 *screens[5];
// screens[0] = main display window
// screens[1] = back screen, alternative blitting
// screens[2] = screenshot buffer, gif movie buffer
// screens[3] = fade screen start
// screens[4] = fade screen end, postimage tempoarary buffer

consvar_t cv_ticrate = CVAR_INIT ("showfps", "No", CV_SAVE, CV_YesNo, NULL);

static void CV_palette_OnChange(void);

static CV_PossibleValue_t gamma_cons_t[] = {{-15, "MIN"}, {5, "MAX"}, {0, NULL}};
consvar_t cv_globalgamma = CVAR_INIT ("gamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);

static CV_PossibleValue_t saturation_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_globalsaturation = CVAR_INIT ("saturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);

#define huecoloursteps 4

static CV_PossibleValue_t hue_cons_t[] = {{0, "MIN"}, {(huecoloursteps*6)-1, "MAX"}, {0, NULL}};
consvar_t cv_rhue = CVAR_INIT ("rhue",  "0", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_yhue = CVAR_INIT ("yhue",  "4", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_ghue = CVAR_INIT ("ghue",  "8", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_chue = CVAR_INIT ("chue", "12", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_bhue = CVAR_INIT ("bhue", "16", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);
consvar_t cv_mhue = CVAR_INIT ("mhue", "20", CV_SAVE|CV_CALL, hue_cons_t, CV_palette_OnChange);

consvar_t cv_rgamma = CVAR_INIT ("rgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_ygamma = CVAR_INIT ("ygamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_ggamma = CVAR_INIT ("ggamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_cgamma = CVAR_INIT ("cgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_bgamma = CVAR_INIT ("bgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);
consvar_t cv_mgamma = CVAR_INIT ("mgamma", "0", CV_SAVE|CV_CALL, gamma_cons_t, CV_palette_OnChange);

consvar_t cv_rsaturation = CVAR_INIT ("rsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_ysaturation = CVAR_INIT ("ysaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_gsaturation = CVAR_INIT ("gsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_csaturation = CVAR_INIT ("csaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_bsaturation = CVAR_INIT ("bsaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);
consvar_t cv_msaturation = CVAR_INIT ("msaturation", "10", CV_SAVE|CV_CALL, saturation_cons_t, CV_palette_OnChange);

static CV_PossibleValue_t constextsize_cons_t[] = {
	{V_NOSCALEPATCH, "Small"}, {V_SMALLSCALEPATCH, "Medium"}, {V_MEDSCALEPATCH, "Large"}, {0, "Huge"},
	{0, NULL}};
static void CV_constextsize_OnChange(void);
consvar_t cv_constextsize = CVAR_INIT ("con_textsize", "Medium", CV_SAVE|CV_CALL, constextsize_cons_t, CV_constextsize_OnChange);

consvar_t cv_palette = CVAR_INIT ("palette", "", CV_CHEAT|CV_CALL|CV_NOINIT, NULL, CV_palette_OnChange);
consvar_t cv_palettenum = CVAR_INIT ("palettenum", "0", CV_CHEAT|CV_CALL|CV_NOINIT, CV_Unsigned, CV_palette_OnChange);

// local copy of the palette for V_GetColor()
RGBA_t *pLocalPalette = NULL;
RGBA_t *pMasterPalette = NULL;
RGBA_t *pGammaCorrectedPalette = NULL;

static size_t currentPaletteSize;

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

static void CV_palette_OnChange(void)
{
	if (con_startup_loadprogress < LOADED_CONFIG)
		return;
	// recalculate Color Cube
	V_ReloadPalette();
	V_SetPalette(0);
}

#if defined (__GNUC__) && defined (__i386__) && !defined (NOASM) && !defined (__APPLE__) && !defined (NORUSEASM)
void VID_BlitLinearScreen_ASM(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes);
#define HAVE_VIDCOPY
#endif

static void CV_constextsize_OnChange(void)
{
	if (!con_startup)
		con_recalc = true;
}


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
	SINT8 player = -1;
	UINT8 i;

	if (options & V_SPLITSCREEN)
	{
		if (r_splitscreen > 0)
		{
			screenheight /= 2;
			baseheight /= 2;
		}

		if (r_splitscreen > 1)
		{
			screenwidth /= 2;
			basewidth /= 2;
		}
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (stplyr == &players[displayplayers[i]])
		{
			player = i;
			break;
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
		const tic_t length = TICRATE/4;
		tic_t timer = lt_exitticker;
		if (K_CheckBossIntro() == true || G_IsTitleCardAvailable() == false)
		{
			if (leveltime <= 16)
				timer = 0;
			else
				timer = leveltime-16;
		}

		if (timer < length)
		{
			boolean slidefromright = false;

			const INT32 offsetAmount = (screenwidth * FRACUNIT/2) / length;
			fixed_t offset = (screenwidth * FRACUNIT/2) - (timer * offsetAmount);

			offset += FixedMul(offsetAmount, renderdeltatics);
			offset /= FRACUNIT;

			if (r_splitscreen > 1)
			{
				if (stplyr == &players[displayplayers[1]] || stplyr == &players[displayplayers[3]])
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
	}
}

static cliprect_t cliprect;

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

// Draws a patch scaled to arbitrary size.
void V_DrawStretchyFixedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap)
{
	UINT8 (*patchdrawfunc)(const UINT8*, const UINT8*, fixed_t);
	UINT32 alphalevel, blendmode;

	fixed_t col, ofs, colfrac, rowfrac, fdup, vdup;
	INT32 dupx, dupy;
	const column_t *column;
	UINT8 *desttop, *dest, *deststart, *destend;
	const UINT8 *source, *deststop;
	fixed_t pwidth; // patch width
	fixed_t offx = 0; // x offset

	const cliprect_t *clip = V_GetClipRect();

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	//if (rendermode != render_soft && !con_startup)		// Why?
	if (rendermode == render_opengl)
	{
		HWR_DrawStretchyFixedPatch(patch, x, y, pscale, vscale, scrn, colormap);
		return;
	}
#endif

	patchdrawfunc = standardpdraw;

	if ((blendmode = ((scrn & V_BLENDMASK) >> V_BLENDSHIFT)))
		blendmode++; // realign to constants
	if ((alphalevel = ((scrn & V_ALPHAMASK) >> V_ALPHASHIFT)))
	{
		if (alphalevel == 10) // V_HUDTRANSHALF
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 11) // V_HUDTRANS
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 12) // V_HUDTRANSDOUBLE
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10) // Still inelegible to render?
			return;
	}
	if ((v_translevel = R_GetBlendTable(blendmode, alphalevel)))
		patchdrawfunc = translucentpdraw;

	v_colormap = NULL;
	if (colormap)
	{
		v_colormap = colormap;
		patchdrawfunc = (v_translevel) ? transmappedpdraw : mappedpdraw;
	}

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
	fdup = vdup = FixedMul(dupx<<FRACBITS, pscale);
	if (vscale != pscale)
		vdup = FixedMul(dupx<<FRACBITS, vscale);
	colfrac = FixedDiv(FRACUNIT, fdup);
	rowfrac = FixedDiv(FRACUNIT, vdup);

	{
		fixed_t offsetx = 0, offsety = 0;

		// left offset
		if (scrn & V_FLIP)
			offsetx = FixedMul((patch->width - patch->leftoffset)<<FRACBITS, pscale) + 1;
		else
			offsetx = FixedMul(patch->leftoffset<<FRACBITS, pscale);

		// top offset
		if (scrn & V_VFLIP)
			offsety = FixedMul((patch->height - patch->topoffset)<<FRACBITS, vscale) + 1;
		else
			offsety = FixedMul(patch->topoffset<<FRACBITS, vscale);

		// Subtract the offsets from x/y positions
		x -= offsetx;
		y -= offsety;
	}

	desttop = screens[scrn&V_SCREENMASK];

	if (!desttop)
		return;

	deststop = desttop + vid.rowbytes * vid.height;

	if (scrn & V_NOSCALESTART)
	{
		x >>= FRACBITS;
		y >>= FRACBITS;
		desttop += (y*vid.width) + x;
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

		desttop += (y*vid.width) + x;
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

	deststart = desttop;
	destend = desttop + pwidth;

	for (col = 0; (col>>FRACBITS) < patch->width; col += colfrac, ++offx, desttop++)
	{
		INT32 topdelta, prevdelta = -1;

		if (scrn & V_FLIP) // offx is measured from right edge instead of left
		{
			if (x+pwidth-offx < (clip ? clip->left : 0)) // don't draw off the left of the screen (WRAP PREVENTION)
				break;
			if (x+pwidth-offx >= (clip ? clip->right : vid.width)) // don't draw off the right of the screen (WRAP PREVENTION)
				continue;
		}
		else
		{
			if (x+offx < (clip ? clip->left : 0)) // don't draw off the left of the screen (WRAP PREVENTION)
				continue;
			if (x+offx >= (clip ? clip->right : vid.width)) // don't draw off the right of the screen (WRAP PREVENTION)
				break;
		}

		column = (const column_t *)((const UINT8 *)(patch->columns) + (patch->columnofs[col>>FRACBITS]));

		while (column->topdelta != 0xff)
		{
			fixed_t offy = 0;

			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			source = (const UINT8 *)(column) + 3;

			dest = desttop;
			if (scrn & V_FLIP)
				dest = deststart + (destend - dest);
			topdelta = FixedInt(FixedMul(topdelta << FRACBITS, vdup));
			dest += topdelta * vid.width;

			if (scrn & V_VFLIP)
			{
				for (ofs = (column->length << FRACBITS)-1; dest < deststop && ofs >= 0; ofs -= rowfrac, ++offy)
				{
					if (clip != NULL)
					{
						const INT32 cy = y + topdelta - offy;

						if (cy < clip->top) // don't draw off the top of the clip rect
						{
							dest += vid.width;
							continue;
						}

						if (cy >= clip->bottom) // don't draw off the bottom of the clip rect
						{
							dest += vid.width;
							continue;
						}
					}

					if (dest >= screens[scrn&V_SCREENMASK]) // don't draw off the top of the screen (CRASH PREVENTION)
						*dest = patchdrawfunc(dest, source, ofs);

					dest += vid.width;
				}
			}
			else
			{
				for (ofs = 0; dest < deststop && ofs < (column->length << FRACBITS); ofs += rowfrac, ++offy)
				{
					if (clip != NULL)
					{
						const INT32 cy = y + topdelta + offy;

						if (cy < clip->top) // don't draw off the top of the clip rect
						{
							dest += vid.width;
							continue;
						}

						if (cy >= clip->bottom) // don't draw off the bottom of the clip rect
						{
							dest += vid.width;
							continue;
						}
					}

					if (dest >= screens[scrn&V_SCREENMASK]) // don't draw off the top of the screen (CRASH PREVENTION)
						*dest = patchdrawfunc(dest, source, ofs);

					dest += vid.width;
				}
			}

			column = (const column_t *)((const UINT8 *)column + column->length + 4);
		}
	}
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

#ifdef RANGECHECK
	if (x < 0 || x + width > vid.width || y < 0 || y + height > vid.height || (unsigned)scrn > 4)
		I_Error("Bad V_DrawBlock");
#endif

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
	UINT8 *dest;
	const UINT8 *deststop;

	if (rendermode == render_none)
		return;

#ifdef HWRENDER
	//if (rendermode != render_soft && !con_startup)		// Not this again
	if (rendermode == render_opengl)
	{
		HWR_DrawFill(x, y, w, h, c);
		return;
	}
#endif

	if (!(c & V_NOSCALESTART))
	{
		INT32 dupx = vid.dupx, dupy = vid.dupy;

		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{ // Clear the entire screen, from dest to deststop. Yes, this really works.
			memset(screens[0], (c&255), vid.width * vid.height * vid.bpp);
			return;
		}

		x *= dupx;
		y *= dupy;
		w *= dupx;
		h *= dupy;

		// Center it if necessary
		V_AdjustXYWithSnap(&x, &y, c, dupx, dupy);
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

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;

	for (;(--h >= 0) && dest < deststop; dest += vid.width)
		memset(dest, c, w * vid.bpp);
}

#ifdef HWRENDER
// This is now a function since it's otherwise repeated 2 times and honestly looks retarded:
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
		case 5:		hwcolor = 0xff69b400;	break; 	// Raspberry
		case 6:		hwcolor = 0xff000000;	break; 	// Red
		case 7:		hwcolor = 0xffd68300;	break;	// Creamsicle
		case 8:		hwcolor = 0xff800000;	break; 	// Orange
		case 9:		hwcolor = 0xdaa52000;	break; 	// Gold
		case 10:	hwcolor = 0x80800000;	break; 	// Yellow
		case 11:	hwcolor = 0x00ff0000;	break; 	// Emerald
		case 12:	hwcolor = 0x00800000;	break; 	// Green
		case 13:	hwcolor = 0x4080ff00;	break; 	// Cyan
		case 14:	hwcolor = 0x4682b400;	break; 	// Steel
		case 15:	hwcolor = 0x1e90ff00;	break;	// Periwinkle
		case 16:	hwcolor = 0x0000ff00;	break; 	// Blue
		case 17:	hwcolor = 0xff00ff00;	break; 	// Purple
		case 18:	hwcolor = 0xee82ee00;	break; 	// Lavender
		// Default green
		default:	hwcolor = 0x00800000;	break;
	}
	return hwcolor;
}
#endif

// THANK YOU MPC!!!
// and thanks toaster for cleaning it up.

void V_DrawFillConsoleMap(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c)
{
	UINT8 *dest;
	const UINT8 *deststop;
	INT32 u;
	UINT8 *fadetable;
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

	if ((alphalevel = ((c & V_ALPHAMASK) >> V_ALPHASHIFT)))
	{
		if (alphalevel == 10) // V_HUDTRANSHALF
			alphalevel = hudminusalpha[st_translucency];
		else if (alphalevel == 11) // V_HUDTRANS
			alphalevel = 10 - st_translucency;
		else if (alphalevel == 12) // V_HUDTRANSDOUBLE
			alphalevel = hudplusalpha[st_translucency];

		if (alphalevel >= 10) // Still inelegible to render?
			return;
	}

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

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;

	// Jimita (12-04-2018)
	if (alphalevel)
	{
		fadetable = R_GetTranslucencyTable(alphalevel) + (c*256);
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
		{
			u = 0;
			while (u < w)
			{
				dest[u] = fadetable[consolebgmap[dest[u]]];
				u++;
			}
		}
	}
	else
	{
		for (;(--h >= 0) && dest < deststop; dest += vid.width)
		{
			u = 0;
			while (u < w)
			{
				dest[u] = consolebgmap[dest[u]];
				u++;
			}
		}
	}
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
	UINT8 *dest;
	const UINT8 *deststop;
	INT32 w, h, wait = 0;

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
		wait = w - (vid.width - x);
		w = vid.width - x;
	}
	if (y + w > vid.height)
		h = vid.height - y;

	if (h > w)
		h = w;

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;

	for (;(--h >= 0) && dest < deststop; dest += vid.width)
	{
		memset(dest, c, w * vid.bpp);
		if (wait)
			wait--;
		else
			w--;
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
	UINT8 *dest;
	const UINT8 *deststop;
	INT32 u;
	UINT8 *fadetable;

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
		// adjustxy
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

	dest = screens[0] + y*vid.width + x;
	deststop = screens[0] + vid.rowbytes * vid.height;

	c &= 255;

	fadetable = ((color & 0xFF00) // Color is not palette index?
		? ((UINT8 *)colormaps + strength*256) // Do COLORMAP fade.
		: ((UINT8 *)R_GetTranslucencyTable((9-strength)+1) + color*256)); // Else, do TRANSMAP** fade.
	for (;(--h >= 0) && dest < deststop; dest += vid.width)
	{
		u = 0;
		while (u < w)
		{
			dest[u] = fadetable[dest[u]];
			u++;
		}
	}
}

//
// Fills a box of pixels using a flat texture as a pattern, scaled to screen size.
//
void V_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatnum)
{
	INT32 u, v, dupx, dupy;
	fixed_t dx, dy, xfrac, yfrac;
	const UINT8 *src, *deststop;
	UINT8 *flat, *dest;
	size_t size, lflatsize, flatshift;

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
			flatshift = 11;
			break;
		case 1048576: // 1024x1024 lump
			lflatsize = 1024;
			flatshift = 10;
			break;
		case 262144:// 512x512 lump
			lflatsize = 512;
			flatshift = 9;
			break;
		case 65536: // 256x256 lump
			lflatsize = 256;
			flatshift = 8;
			break;
		case 16384: // 128x128 lump
			lflatsize = 128;
			flatshift = 7;
			break;
		case 1024: // 32x32 lump
			lflatsize = 32;
			flatshift = 5;
			break;
		case 256: // 16x16 lump
			lflatsize = 16;
			flatshift = 4;
			break;
		case 64: // 8x8 lump
			lflatsize = 8;
			flatshift = 3;
			break;
		default: // 64x64 lump
			lflatsize = 64;
			flatshift = 6;
			break;
	}

	flat = static_cast<UINT8*>(W_CacheLumpNum(flatnum, PU_CACHE));

	dupx = dupy = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	dest = screens[0] + y*dupy*vid.width + x*dupx;
	deststop = screens[0] + vid.rowbytes * vid.height;

	// from V_DrawScaledPatch
	if (vid.width != BASEVIDWIDTH * dupx)
	{
		// dupx adjustments pretend that screen width is BASEVIDWIDTH * dupx,
		// so center this imaginary screen
		dest += (vid.width - (BASEVIDWIDTH * dupx)) / 2;
	}
	if (vid.height != BASEVIDHEIGHT * dupy)
	{
		// same thing here
		dest += (vid.height - (BASEVIDHEIGHT * dupy)) * vid.width / 2;
	}

	w *= dupx;
	h *= dupy;

	dx = FixedDiv(FRACUNIT, dupx<<(FRACBITS-2));
	dy = FixedDiv(FRACUNIT, dupy<<(FRACBITS-2));

	yfrac = 0;
	for (v = 0; v < h; v++, dest += vid.width)
	{
		xfrac = 0;
		src = flat + (((yfrac>>FRACBITS) & (lflatsize - 1)) << flatshift);
		for (u = 0; u < w; u++)
		{
			if (&dest[u] > deststop)
				return;
			dest[u] = src[(xfrac>>FRACBITS)&(lflatsize-1)];
			xfrac += dx;
		}
		yfrac += dy;
	}
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

	{
		const UINT8 *fadetable =
			(color > 0xFFF0) // Grab a specific colormap palette?
			? R_GetTranslationColormap(color | 0xFFFF0000, static_cast<skincolornum_t>(strength), GTC_CACHE)
			: ((color & 0xFF00) // Color is not palette index?
			? ((UINT8 *)colormaps + strength*256) // Do COLORMAP fade.
			: ((UINT8 *)R_GetTranslucencyTable((9-strength)+1) + color*256)); // Else, do TRANSMAP** fade.
		const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;
		UINT8 *buf = screens[0];

		// heavily simplified -- we don't need to know x or y
		// position when we're doing a full screen fade
		for (; buf < deststop; ++buf)
			*buf = fadetable[*buf];
	}
}

//
// Fade the screen buffer, using a custom COLORMAP lump.
// Split from V_DrawFadeScreen, because that function has
// WAY too many options piled on top of it as is. :V
//
void V_DrawCustomFadeScreen(const char *lump, UINT8 strength)
{
#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		//HWR_DrawCustomFadeScreen(color, strength);
		return;
	}
#endif

	{
		lumpnum_t lumpnum = LUMPERROR;
		lighttable_t *clm = NULL;

		if (lump != NULL)
			lumpnum = W_GetNumForName(lump);
		else
			return;

		if (lumpnum != LUMPERROR)
		{
			clm = static_cast<lighttable_t*>(Z_MallocAlign(COLORMAP_SIZE, PU_STATIC, NULL, 8));
			W_ReadLump(lumpnum, clm);

			if (clm != NULL)
			{
				const UINT8 *fadetable = ((UINT8 *)clm + strength*256);
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
}

// Simple translucency with one color, over a set number of lines starting from the top.
void V_DrawFadeConsBack(INT32 plines)
{
	UINT8 *deststop, *buf;

#ifdef HWRENDER // not win32 only 19990829 by Kin
	if (rendermode == render_opengl)
	{
		UINT32 hwcolor = V_GetHWConsBackColor();
		HWR_DrawConsoleBack(hwcolor, plines);
		return;
	}
#endif

	// heavily simplified -- we don't need to know x or y position,
	// just the stop position
	deststop = screens[0] + vid.rowbytes * std::min(plines, vid.height);
	for (buf = screens[0]; buf < deststop; ++buf)
		*buf = consolebgmap[*buf];
}


//
// Invert the entire screen, for Encore fades
//
void V_EncoreInvertScreen(void)
{
#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		HWR_EncoreInvertScreen();
		return;
	}
#endif

	{
		const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;
		UINT8 *buf = screens[0];

		for (; buf < deststop; ++buf)
		{
			*buf = NearestColor(
				255 - pLocalPalette[*buf].s.red,
				255 - pLocalPalette[*buf].s.green,
				255 - pLocalPalette[*buf].s.blue
			);
		}
	}
}

// Very similar to F_DrawFadeConsBack, except we draw from the middle(-ish) of the screen to the bottom.
void V_DrawPromptBack(INT32 boxheight, INT32 color)
{
	UINT8 *deststop, *buf;

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

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		UINT32 hwcolor;
		switch (color)
		{
			case 0:		hwcolor = 0xffffff00;	break; 	// White
			case 1:		hwcolor = 0x00000000;	break; 	// Black // Note this is different from V_DrawFadeConsBack
			case 2:		hwcolor = 0xdeb88700;	break;	// Sepia
			case 3:		hwcolor = 0x40201000;	break; 	// Brown
			case 4:		hwcolor = 0xfa807200;	break; 	// Pink
			case 5:		hwcolor = 0xff69b400;	break; 	// Raspberry
			case 6:		hwcolor = 0xff000000;	break; 	// Red
			case 7:		hwcolor = 0xffd68300;	break;	// Creamsicle
			case 8:		hwcolor = 0xff800000;	break; 	// Orange
			case 9:		hwcolor = 0xdaa52000;	break; 	// Gold
			case 10:	hwcolor = 0x80800000;	break; 	// Yellow
			case 11:	hwcolor = 0x00ff0000;	break; 	// Emerald
			case 12:	hwcolor = 0x00800000;	break; 	// Green
			case 13:	hwcolor = 0x4080ff00;	break; 	// Cyan
			case 14:	hwcolor = 0x4682b400;	break; 	// Steel
			case 15:	hwcolor = 0x1e90ff00;	break;	// Periwinkle
			case 16:	hwcolor = 0x0000ff00;	break; 	// Blue
			case 17:	hwcolor = 0xff00ff00;	break; 	// Purple
			case 18:	hwcolor = 0xee82ee00;	break; 	// Lavender
			// Default green
			default:	hwcolor = 0x00800000;	break;
		}
		HWR_DrawTutorialBack(hwcolor, boxheight);
		return;
	}
#endif

	CON_SetupBackColormapEx(color, true);

	// heavily simplified -- we don't need to know x or y position,
	// just the start and stop positions
	buf = deststop = screens[0] + vid.rowbytes * vid.height;
	if (boxheight < 0)
		buf += vid.rowbytes * boxheight;
	else // 4 lines of space plus gaps between and some leeway
		buf -= vid.rowbytes * ((boxheight * 4) + (boxheight/2)*5);
	for (; buf < deststop; ++buf)
		*buf = promptbgmap[*buf];
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

// Writes a single character (draw WHITE if bit 7 set)
//
void V_DrawCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed)
{
	INT32 w, flags;
	const UINT8 *colormap = V_GetStringColormap(c);

	flags = c & ~(V_CHARCOLORMASK | V_PARAMMASK);
	c &= 0x7f;
	if (lowercaseallowed)
		c -= HU_FONTSTART;
	else
		c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE || !fontv[HU_FONT].font[c])
		return;

	w = fontv[HU_FONT].font[c]->width;
	if (x + w > vid.width)
		return;

	if (colormap != NULL)
		V_DrawMappedPatch(x, y, flags, fontv[HU_FONT].font[c], colormap);
	else
		V_DrawScaledPatch(x, y, flags, fontv[HU_FONT].font[c]);
}

// Writes a single character for the chat (half scaled). (draw WHITE if bit 7 set)
// 16/02/19: Scratch the scaling thing, chat doesn't work anymore under 2x res -Lat'
//
void V_DrawChatCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed, UINT8 *colormap)
{
	INT32 w, flags;
	//const UINT8 *colormap = V_GetStringColormap(c);

	flags = c & ~(V_CHARCOLORMASK | V_PARAMMASK);
	c &= 0x7f;
	if (lowercaseallowed)
		c -= HU_FONTSTART;
	else
		c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE || !fontv[HU_FONT].font[c])
		return;

	w = fontv[HU_FONT].font[c]->width / 2;
	if (x + w > vid.width)
		return;

	V_DrawFixedPatch(x*FRACUNIT, y*FRACUNIT, FRACUNIT/2, flags, fontv[HU_FONT].font[c], colormap);
}

// V_TitleCardStringWidth
// Get the string's width using the titlecard font.
INT32 V_TitleCardStringWidth(const char *str)
{
	INT32 xoffs = 0;
	const char *ch = str;
	char c;
	patch_t *pp;

	for (;;ch++)
	{
		if (!*ch)
			break;

		if (*ch == '\n')
		{
			xoffs = 0;
			continue;
		}

		c = *ch;
		c = toupper(c);
		c -= LT_FONTSTART;

		// check if character exists, if not, it's a space.
		if (c < 0 || c >= LT_FONTSIZE || !fontv[GTOL_FONT].font[(INT32)c])
		{
			xoffs += 10;
			continue;
		}

		pp = fontv[GTFN_FONT].font[(INT32)c];

		xoffs += pp->width-5;
	}

	return xoffs;
}

// V_DrawTitleCardScreen.
// see v_video.h's prototype for more information.
//
void V_DrawTitleCardString(INT32 x, INT32 y, const char *str, INT32 flags, boolean bossmode, INT32 timer, INT32 threshold)
{

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

	x -= 2;	// Account for patch width...

	if (flags & V_SNAPTORIGHT)
		x -= V_TitleCardStringWidth(str);


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
			yoffs += 32;

			continue;
		}

		c = *ch;

		c = toupper(c);
		c -= LT_FONTSTART;

		// check if character exists, if not, it's a space.
		if (c < 0 || c >= LT_FONTSIZE || !fontv[GTFN_FONT].font[(INT32)c])
		{
			xoffs += 10;
			continue;
		}

		ol = fontv[GTOL_FONT].font[(INT32)c];
		pp = fontv[GTFN_FONT].font[(INT32)c];

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
				fakeang = std::min(360 + 90, let_time*41) * ANG1;
				scalex = FINESINE(fakeang>>ANGLETOFINESHIFT);
			}
			else if (!bossmode && let_time > threshold)
			{
				// Make letters disappear...
				let_time -= threshold;

				fakeang = std::max(0, (360+90) - let_time*41)*ANG1;
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
			V_DrawStretchyFixedPatch((x + xoffs)*FRACUNIT + offs, (y+yoffs)*FRACUNIT, abs(scalex), FRACUNIT, flags|flipflag, ol, NULL);
			V_DrawStretchyFixedPatch((x + xoffs)*FRACUNIT + offs, (y+yoffs)*FRACUNIT, abs(scalex), FRACUNIT, flags|flipflag, pp, NULL);
		}

		xoffs += pp->width -5;
	}
}


// Precompile a wordwrapped string to any given width.
// This is a muuuch better method than V_WORDWRAP.
char *V_WordWrap(INT32 x, INT32 w, INT32 option, const char *string)
{
	int c;
	size_t chw, i, lastusablespace = 0;
	size_t slen;
	char *newstring = Z_StrDup(string);
	INT32 spacewidth = 4, charwidth = 0;

	slen = strlen(string);

	if (w == 0)
		w = BASEVIDWIDTH;
	w -= x;
	x = 0;

	switch (option & V_SPACINGMASK)
	{
		case V_MONOSPACE:
			spacewidth = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			charwidth = 8;
			break;
		case V_6WIDTHSPACE:
			spacewidth = 6;
		default:
			break;
	}

	for (i = 0; i < slen; ++i)
	{
		c = newstring[i];
		if ((UINT8)c & 0x80) //color parsing! -Inuyasha 2.16.09
			continue;

		if (c == '\n')
		{
			x = 0;
			lastusablespace = 0;
			continue;
		}

		if (!(option & V_ALLOWLOWERCASE))
			c = toupper(c);
		c -= HU_FONTSTART;

		if (c < 0 || c >= HU_FONTSIZE || !fontv[HU_FONT].font[c])
		{
			chw = spacewidth;
			lastusablespace = i;
		}
		else
			chw = (charwidth ? charwidth : fontv[HU_FONT].font[c]->width);

		x += chw;

		if (lastusablespace != 0 && x > w)
		{
			newstring[lastusablespace] = '\n';
			i = lastusablespace;
			lastusablespace = 0;
			x = 0;
		}
	}
	return newstring;
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
	(*cwp) = FixedMul(std::max(1, (*cwp) - 1) << FRACBITS, scale);
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
	(*cwp) = FixedMul(std::max(1, (*cwp) - 2) << FRACBITS, scale);
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
	(*cwp) = FixedMul(std::max(1, (*cwp) - 3) << FRACBITS, scale);
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
	(*cwp) = FixedMul(std::max(1, (*cwp) - 4) << FRACBITS, scale);
	return 0;
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
	fixed_t    chw;
	INT32     hchw;/* half-width for centering */
	fixed_t spacew;
	fixed_t    lfh;

	INT32     dupx;

	fixed_t  right;
	fixed_t    bot;

	fixed_t (*dim_fn)(fixed_t,fixed_t,INT32,INT32,fixed_t *);

	font_t   *font;

	boolean uppercase;
	boolean notcolored;

	boolean   dance;
	boolean nodanceoverride;
	INT32     dancecounter;

	fixed_t cx, cy;

	fixed_t cxoff, cyoff;
	fixed_t cw;

	INT32     spacing;
	fixed_t   left;

	int c;

	uppercase  = !( flags & V_ALLOWLOWERCASE );
	flags	&= ~(V_FLIP);/* These two (V_ALLOWLOWERCASE) share a bit. */

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

	chw        = 0;

	spacing = ( flags & V_SPACINGMASK );

	/*
	Hardcoded until a better system can be implemented
	for determining how fonts space.
	*/
	switch (fontno)
	{
		default:
		case HU_FONT:
			spacew = 4;
			switch (spacing)
			{
				case V_MONOSPACE:
					spacew = 8;
					/* FALLTHRU */
				case V_OLDSPACING:
					chw    = 8;
					break;
				case V_6WIDTHSPACE:
					spacew = 6;
			}
			break;
		case TINY_FONT:
			spacew = 2;
			switch (spacing)
			{
				case V_MONOSPACE:
					spacew = 5;
					/* FALLTHRU */
				case V_OLDSPACING:
					chw    = 5;
					break;
				// Out of video flags, so we're reusing this for alternate charwidth instead
				/*case V_6WIDTHSPACE:
				  spacewidth = 3;*/
			}
			break;
		case LT_FONT:
			spacew = 12;
			break;
		case CRED_FONT:
			spacew = 16;
			break;
		case KART_FONT:
			spacew = 12;
			switch (spacing)
			{
				case V_MONOSPACE:
					spacew = 12;
					/* FALLTHRU */
				case V_OLDSPACING:
					chw    = 12;
					break;
				case V_6WIDTHSPACE:
					spacew = 6;
			}
			break;
		case GM_FONT:
			spacew = 6;
			break;
		case FILE_FONT:
			spacew = 0;
			break;
		case LSHI_FONT:
		case LSLOW_FONT:
			spacew = 16;
			break;
	}

	switch (fontno)
	{
		default:
		case HU_FONT:
		case TINY_FONT:
		case KART_FONT:
			lfh = 12;
			break;
		case LT_FONT:
		case CRED_FONT:
		case FILE_FONT:
			lfh    = 12;
			break;
		case GM_FONT:
			lfh    = 32;
			break;
		case LSHI_FONT:
			lfh    = 56;
			break;
		case LSLOW_FONT:
			lfh    = 38;
			break;
	}

	hchw     = chw >> 1;

	chw    <<= FRACBITS;
	spacew <<= FRACBITS;
	lfh    <<= FRACBITS;

#define Mul( id, scale ) ( id = FixedMul (scale, id) )
	Mul    (chw,      scale);
	Mul (spacew,      scale);
	Mul    (lfh,      scale);

	Mul (spacew, spacescale);
	Mul    (lfh,    lfscale);
#undef  Mul

	if (( flags & V_NOSCALESTART ))
	{
		dupx      = vid.dupx;

		hchw     *=     dupx;

		chw      *=     dupx;
		spacew   *=     dupx;
		lfh      *= vid.dupy;

		right     = vid.width;
	}
	else
	{
		dupx      = 1;

		right     = ( vid.width / vid.dupx );
		if (!( flags & V_SNAPTOLEFT ))
		{
			left   = ( right - BASEVIDWIDTH )/ 2;/* left edge of drawable area */
			right -= left;
		}
	}

	right      <<=               FRACBITS;
	bot          = vid.height << FRACBITS;

	switch (fontno)
	{
		default:
			if (chw)
				dim_fn = CenteredCharacterDim;
			else
				dim_fn = VariableCharacterDim;
			break;
		case TINY_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
			{
				/* Reuse this flag for the alternate bunched-up spacing. */
				if (( flags & V_6WIDTHSPACE ))
					dim_fn = BunchedCharacterDim;
				else
					dim_fn = VariableCharacterDim;
			}
			break;
		case GM_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
				dim_fn = GamemodeCharacterDim;
			break;
		case FILE_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
				dim_fn = FileCharacterDim;
			break;
		case LSHI_FONT:
		case LSLOW_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
				dim_fn = LSTitleCharacterDim;
			break;
	}

	cx = x;
	cy = y;
	cyoff = 0;

	for (; ( c = *s ); ++s, ++dancecounter)
	{
		switch (c)
		{
			case '\n':
				cy += lfh;
				if (cy >= bot)
					return;
				cx  =   x;
				break;
			default:
				if (( c & 0x80 ))
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
					}
				}
				else if (c == V_STRINGDANCE)
				{
					dance = true;
				}
				else if (cx < right)
				{
					if (uppercase)
						c = toupper(c);

					if (dance)
						cyoff = V_DanceYOffset(dancecounter) * FRACUNIT;

					c -= font->start;
					if (c >= 0 && c < font->size && font->font[c])
					{
						cw = SHORT (font->font[c]->width) * dupx;
						cxoff = (*dim_fn)(scale, chw, hchw, dupx, &cw);
						V_DrawFixedPatch(cx + cxoff, cy + cyoff, scale,
								flags, font->font[c], colormap);
						cx += cw;
					}
					else
						cx += spacew;
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
	fixed_t    chw;
	INT32     hchw;/* half-width for centering */
	fixed_t spacew;
	fixed_t    lfh;

	INT32     dupx;

	fixed_t (*dim_fn)(fixed_t,fixed_t,INT32,INT32,fixed_t *);

	font_t   *font;

	boolean uppercase;

	fixed_t cx, cy;

	fixed_t cw;

	INT32     spacing;

	int c;

	fixed_t fullwidth = 0;

	uppercase  = !( flags & V_ALLOWLOWERCASE );
	flags	&= ~(V_FLIP);/* These two (V_ALLOWLOWERCASE) share a bit. */

	font       = &fontv[fontno];

	chw        = 0;

	spacing = ( flags & V_SPACINGMASK );

	/*
	Hardcoded until a better system can be implemented
	for determining how fonts space.
	*/
	switch (fontno)
	{
		default:
		case HU_FONT:
			spacew = 4;
			switch (spacing)
			{
				case V_MONOSPACE:
					spacew = 8;
					/* FALLTHRU */
				case V_OLDSPACING:
					chw    = 8;
					break;
				case V_6WIDTHSPACE:
					spacew = 6;
			}
			break;
		case TINY_FONT:
			spacew = 2;
			switch (spacing)
			{
				case V_MONOSPACE:
					spacew = 5;
					/* FALLTHRU */
				case V_OLDSPACING:
					chw    = 5;
					break;
				// Out of video flags, so we're reusing this for alternate charwidth instead
				/*case V_6WIDTHSPACE:
				  spacewidth = 3;*/
			}
			break;
		case LT_FONT:
			spacew = 12;
			break;
		case CRED_FONT:
			spacew = 16;
			break;
		case KART_FONT:
			spacew = 12;
			switch (spacing)
			{
				case V_MONOSPACE:
					spacew = 12;
					/* FALLTHRU */
				case V_OLDSPACING:
					chw    = 12;
					break;
				case V_6WIDTHSPACE:
					spacew = 6;
			}
			break;
		case GM_FONT:
		case FILE_FONT:
			spacew = 0;
			break;
		case LSHI_FONT:
		case LSLOW_FONT:
			spacew = 16;
			break;
	}

	switch (fontno)
	{
		default:
		case HU_FONT:
		case TINY_FONT:
		case KART_FONT:
			lfh = 12;
			break;
		case LT_FONT:
		case CRED_FONT:
		case FILE_FONT:
			lfh    = 12;
			break;
		case GM_FONT:
			lfh    = 32;
			break;
		case LSHI_FONT:
			lfh    = 56;
			break;
		case LSLOW_FONT:
			lfh    = 38;
			break;
	}

	hchw     = chw >> 1;

	chw    <<= FRACBITS;
	spacew <<= FRACBITS;

#define Mul( id, scale ) ( id = FixedMul (scale, id) )
	Mul    (chw,      scale);
	Mul (spacew,      scale);
	Mul    (lfh,      scale);

	Mul (spacew, spacescale);
	Mul    (lfh,    lfscale);
#undef  Mul

	if (( flags & V_NOSCALESTART ))
	{
		dupx      = vid.dupx;

		hchw     *=     dupx;

		chw      *=     dupx;
		spacew   *=     dupx;
		lfh      *= vid.dupy;
	}
	else
	{
		dupx      = 1;
	}

	switch (fontno)
	{
		default:
			if (chw)
				dim_fn = CenteredCharacterDim;
			else
				dim_fn = VariableCharacterDim;
			break;
		case TINY_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
			{
				/* Reuse this flag for the alternate bunched-up spacing. */
				if (( flags & V_6WIDTHSPACE ))
					dim_fn = BunchedCharacterDim;
				else
					dim_fn = VariableCharacterDim;
			}
			break;
		case GM_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
				dim_fn = GamemodeCharacterDim;
			break;
		case FILE_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
				dim_fn = FileCharacterDim;
			break;
		case LSHI_FONT:
		case LSLOW_FONT:
			if (chw)
				dim_fn = FixedCharacterDim;
			else
				dim_fn = LSTitleCharacterDim;
			break;
	}

	cx = cy = 0;

	for (; ( c = *s ); ++s)
	{
		switch (c)
		{
			case '\n':
				cy += lfh;
				cx  =   0;
				break;
			default:
				if (uppercase)
					c = toupper(c);

				c -= font->start;
				if (c >= 0 && c < font->size && font->font[c])
				{
					cw = SHORT (font->font[c]->width) * dupx;
					(*dim_fn)(scale, chw, hchw, dupx, &cw);
					cx += cw;
				}
				else
					cx += spacew;
		}

		fullwidth = std::max(cx, fullwidth);
	}

	return fullwidth;
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

void V_DrawCenteredThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= (V_ThinStringWidth(string, option) / 2) * FRACUNIT;
	V_DrawThinStringAtFixed(x, y, option, string);
}

void V_DrawRightAlignedThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string)
{
	x -= V_ThinStringWidth(string, option) * FRACUNIT;
	V_DrawThinStringAtFixed(x, y, option, string);
}

// Draws a number using the PING font thingy.
// TODO: Merge number drawing functions into one with "font name" selection.

INT32 V_DrawPingNum(INT32 x, INT32 y, INT32 flags, INT32 num, const UINT8 *colormap)
{
	INT32 w = SHORT(fontv[PINGNUM_FONT].font[0]->width);	// this SHOULD always be 5 but I guess custom graphics exist.

	if (flags & V_NOSCALESTART)
		w *= vid.dupx;

	if (num < 0)
		num = -num;

	// draw the number
	do
	{
		x -= (w-1);	// Oni wanted their outline to intersect.
		V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, flags, fontv[PINGNUM_FONT].font[num%10], colormap);
		num /= 10;
	} while (num);

	return x;
}

void V_DrawCenteredKartString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_KartStringWidth(string, option)/2;
	V_DrawKartString(x, y, option, string);
}

void V_DrawRightAlignedKartString(INT32 x, INT32 y, INT32 option, const char *string)
{
	x -= V_KartStringWidth(string, option);
	V_DrawKartString(x, y, option, string);
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

	for (i = 0; i < strlen(string); i++)
	{
		c = string[i] - LT_FONTSTART;
		if (c < 0 || c >= LT_FONTSIZE || !fontv[LT_FONT].font[c])
			continue;

		if (fontv[LT_FONT].font[c]->height > w)
			w = fontv[LT_FONT].font[c]->height;
	}

	return w;
}

boolean *heatshifter = NULL;
INT32 lastheight = 0;
INT32 heatindex[MAXSPLITSCREENPLAYERS] = {0, 0, 0, 0};

//
// V_DoPostProcessor
//
// Perform a particular image postprocessing function.
//
#include "p_local.h"
void V_DoPostProcessor(INT32 view, postimg_t type, INT32 param)
{
#if NUMSCREENS < 5
	// do not enable image post processing for ARM, SH and MIPS CPUs
	(void)view;
	(void)type;
	(void)param;
#else
	INT32 yoffset, xoffset;

#ifdef HWRENDER
	if (rendermode != render_soft)
		return;
#endif

	if (view < 0 || view > 3 || view > r_splitscreen)
		return;

	if ((view == 1 && r_splitscreen == 1) || view >= 2)
		yoffset = viewheight;
	else
		yoffset = 0;

	if ((view == 1 || view == 3) && r_splitscreen > 1)
		xoffset = viewwidth;
	else
		xoffset = 0;

	if (type == postimg_water)
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;
		angle_t disStart = (leveltime * 128) & FINEMASK; // in 0 to FINEANGLE
		INT32 newpix;
		INT32 sine;
		//UINT8 *transme = R_GetTranslucencyTable(tr_trans50);

		for (y = yoffset; y < yoffset+viewheight; y++)
		{
			sine = (FINESINE(disStart)*5)>>FRACBITS;
			newpix = abs(sine);

			if (sine < 0)
			{
				M_Memcpy(&tmpscr[(y*vid.width)+xoffset+newpix], &srcscr[(y*vid.width)+xoffset], viewwidth-newpix);

				// Cleanup edge
				while (newpix)
				{
					tmpscr[(y*vid.width)+xoffset+newpix] = srcscr[(y*vid.width)+xoffset];
					newpix--;
				}
			}
			else
			{
				M_Memcpy(&tmpscr[(y*vid.width)+xoffset+0], &srcscr[(y*vid.width)+xoffset+sine], viewwidth-newpix);

				// Cleanup edge
				while (newpix)
				{
					tmpscr[(y*vid.width)+xoffset+viewwidth-newpix] = srcscr[(y*vid.width)+xoffset+(viewwidth-1)];
					newpix--;
				}
			}

/*
Unoptimized version
			for (x = 0; x < vid.width*vid.bpp; x++)
			{
				newpix = (x + sine);

				if (newpix < 0)
					newpix = 0;
				else if (newpix >= vid.width)
					newpix = vid.width-1;

				tmpscr[y*vid.width + x] = srcscr[y*vid.width+newpix]; // *(transme + (srcscr[y*vid.width+x]<<8) + srcscr[y*vid.width+newpix]);
			}*/
			disStart += 22;//the offset into the displacement map, increment each game loop
			disStart &= FINEMASK; //clip it to FINEMASK
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset+xoffset, screens[0]+vid.width*vid.bpp*yoffset+xoffset,
				viewwidth*vid.bpp, viewheight, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_motion) // Motion Blur!
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 x, y;

		// TODO: Add a postimg_param so that we can pick the translucency level...
		UINT8 *transme = R_GetTranslucencyTable(param);

		for (y = yoffset; y < yoffset+viewheight; y++)
		{
			for (x = xoffset; x < xoffset+viewwidth; x++)
			{
				tmpscr[y*vid.width + x]
					=     colormaps[*(transme     + (srcscr   [(y*vid.width)+x ] <<8) + (tmpscr[(y*vid.width)+x]))];
			}
		}
		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset+xoffset, screens[0]+vid.width*vid.bpp*yoffset+xoffset,
				viewwidth*vid.bpp, viewheight, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_flip) // Flip the screen upside-down
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y, y2;

		for (y = yoffset, y2 = yoffset+viewheight - 1; y < yoffset+viewheight; y++, y2--)
			M_Memcpy(&tmpscr[(y2*vid.width)+xoffset], &srcscr[(y*vid.width)+xoffset], viewwidth);

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset+xoffset, screens[0]+vid.width*vid.bpp*yoffset+xoffset,
				viewwidth*vid.bpp, viewheight, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_heat) // Heat wave
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y;

		// Make sure table is built
		if (heatshifter == NULL || lastheight != viewheight)
		{
			if (heatshifter)
				Z_Free(heatshifter);

			heatshifter = static_cast<boolean*>(Z_Calloc(viewheight * sizeof(boolean), PU_STATIC, NULL));

			for (y = 0; y < viewheight; y++)
			{
				if (M_RandomChance(FRACUNIT/8)) // 12.5%
					heatshifter[y] = true;
			}

			heatindex[0] = heatindex[1] = heatindex[2] = heatindex[3] = 0;
			lastheight = viewheight;
		}

		for (y = yoffset; y < yoffset+viewheight; y++)
		{
			if (heatshifter[heatindex[view]++])
			{
				// Shift this row of pixels to the right by 2
				tmpscr[(y*vid.width)+xoffset] = srcscr[(y*vid.width)+xoffset];
				M_Memcpy(&tmpscr[(y*vid.width)+xoffset], &srcscr[(y*vid.width)+xoffset+vid.dupx], viewwidth-vid.dupx);
			}
			else
				M_Memcpy(&tmpscr[(y*vid.width)+xoffset], &srcscr[(y*vid.width)+xoffset], viewwidth);

			heatindex[view] %= viewheight;
		}

		heatindex[view]++;
		heatindex[view] %= vid.height;

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset+xoffset, screens[0]+vid.width*vid.bpp*yoffset+xoffset,
				viewwidth*vid.bpp, viewheight, vid.width*vid.bpp, vid.width);
	}
	else if (type == postimg_mirror) // Flip the screen on the x axis
	{
		UINT8 *tmpscr = screens[4];
		UINT8 *srcscr = screens[0];
		INT32 y, x, x2;

		for (y = yoffset; y < yoffset+viewheight; y++)
		{
			for (x = xoffset, x2 = xoffset+((viewwidth*vid.bpp)-1); x < xoffset+(viewwidth*vid.bpp); x++, x2--)
				tmpscr[y*vid.width + x2] = srcscr[y*vid.width + x];
		}

		VID_BlitLinearScreen(tmpscr+vid.width*vid.bpp*yoffset+xoffset, screens[0]+vid.width*vid.bpp*yoffset+xoffset,
				viewwidth*vid.bpp, viewheight, vid.width*vid.bpp, vid.width);
	}
#endif
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
