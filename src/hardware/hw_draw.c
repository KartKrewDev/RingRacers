// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_draw.c
/// \brief miscellaneous drawing (mainly 2d)

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_main.h"
#include "hw_glob.h"
#include "hw_drv.h"

#include "../m_misc.h" //FIL_WriteFile()
#include "../r_draw.h" //viewborderlump
#include "../r_main.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../st_stuff.h"
#include "../p_local.h" // stplyr
#include "../g_game.h" // players
#include "../k_hud.h"
#include "../r_plane.h" // R_FlatDimensionsFromLumpSize

#include <fcntl.h>
#include "../i_video.h"  // for rendermode != render_glide

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	UINT8 id_field_length ; // 1
	UINT8 color_map_type  ; // 2
	UINT8 image_type      ; // 3
	UINT8 dummy[5]        ; // 4,  8
	INT16 x_origin        ; // 9, 10
	INT16 y_origin        ; //11, 12
	INT16 width           ; //13, 14
	INT16 height          ; //15, 16
	UINT8 image_pix_size  ; //17
	UINT8 image_descriptor; //18
} ATTRPACK TGAHeader; // sizeof is 18
#if defined(_MSC_VER)
#pragma pack()
#endif

static UINT8 softwaretranstogl[11]    = {  0, 25, 51, 76,102,127,153,178,204,229,255};
static UINT8 softwaretranstogl_hi[11] = {  0, 51,102,153,204,255,255,255,255,255,255};
static UINT8 softwaretranstogl_lo[11] = {  0, 12, 24, 36, 48, 60, 71, 83, 95,111,127};

//
// -----------------+
// HWR_DrawPatch    : Draw a 'tile' graphic
// Notes            : x,y : positions relative to the original Doom resolution
//                  : textes(console+score) + menus + status bar
// -----------------+
void HWR_DrawPatch(patch_t *gpatch, INT32 x, INT32 y, INT32 option)
{
	FOutVector v[4];
	FBITFIELD flags;
	GLPatch_t *hwrPatch;

//  3--2
//  | /|
//  |/ |
//  0--1
	float sdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	float sdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;
	float pdupx = FIXED_TO_FLOAT(vid.fdupx)*2.0f;
	float pdupy = FIXED_TO_FLOAT(vid.fdupy)*2.0f;

	// make patch ready in hardware cache
	HWR_GetPatch(gpatch);
	hwrPatch = ((GLPatch_t *)gpatch->hardware);

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		pdupx = pdupy = 2.0f;
		break;
	case V_SMALLSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fsmalldupy);
		break;
	case V_MEDSCALEPATCH:
		pdupx = 2.0f * FIXED_TO_FLOAT(vid.fmeddupx);
		pdupy = 2.0f * FIXED_TO_FLOAT(vid.fmeddupy);
		break;
	}

	if (option & V_NOSCALESTART)
		sdupx = sdupy = 2.0f;

	v[0].x = v[3].x = (x*sdupx-(gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[2].x = v[1].x = (x*sdupx+(gpatch->width-gpatch->leftoffset)*pdupx)/vid.width - 1;
	v[0].y = v[1].y = 1-(y*sdupy-(gpatch->topoffset)*pdupy)/vid.height;
	v[2].y = v[3].y = 1-(y*sdupy+(gpatch->height-gpatch->topoffset)*pdupy)/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = hwrPatch->max_s;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = hwrPatch->max_t;

	flags = PF_Translucent|PF_NoDepthTest;

	/*if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;*/

	// clip it since it is used for bunny scroll in doom I
	HWD.pfnDrawPolygon(NULL, v, 4, flags);
}

void HWR_DrawStretchyFixedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap)
{
	FOutVector v[4];
	FBITFIELD flags;
	float cx = FIXED_TO_FLOAT(x);
	float cy = FIXED_TO_FLOAT(y);
	UINT32 alphalevel, blendmode;
	GLPatch_t *hwrPatch;

//  3--2
//  | /|
//  |/ |
//  0--1
	float dupx, dupy, fscalew, fscaleh, fwidth, fheight;

	const cliprect_t *clip = V_GetClipRect();

	float s_min, s_max;
	float t_min, t_max;

	// make patch ready in hardware cache
	if (!colormap)
		HWR_GetPatch(gpatch);
	else
		HWR_GetMappedPatch(gpatch, colormap);

	hwrPatch = ((GLPatch_t *)gpatch->hardware);

	dupx = (float)vid.dupx;
	dupy = (float)vid.dupy;

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		dupx = dupy = 1.0f;
		break;
	case V_SMALLSCALEPATCH:
		dupx = (float)vid.smalldupx;
		dupy = (float)vid.smalldupy;
		break;
	case V_MEDSCALEPATCH:
		dupx = (float)vid.meddupx;
		dupy = (float)vid.meddupy;
		break;
	}

	dupx = dupy = (dupx < dupy ? dupx : dupy);
	fscalew = fscaleh = FIXED_TO_FLOAT(pscale);
	if (vscale != pscale)
		fscaleh = FIXED_TO_FLOAT(vscale);

	// See my comments in v_video.c's V_DrawFixedPatch
	// -- Monster Iestyn 29/10/18
	{
		float offsetx = 0.0f, offsety = 0.0f;

		// left offset
		if (option & V_FLIP)
			offsetx = (float)(gpatch->width - gpatch->leftoffset) * fscalew;
		else
			offsetx = (float)(gpatch->leftoffset) * fscalew;

		// top offset
		if (option & V_VFLIP)
			offsety = (float)(gpatch->height - gpatch->topoffset) * fscaleh;
		else
			offsety = (float)(gpatch->topoffset) * fscaleh;

		cx -= offsetx;
		cy -= offsety;
	}

	if (!(option & V_NOSCALESTART))
	{
		cx = cx * dupx;
		cy = cy * dupy;

		if (!(option & V_SCALEPATCHMASK))
		{
			INT32 intx, inty;
			intx = (INT32)cx;
			inty = (INT32)cy;
			V_AdjustXYWithSnap(&intx, &inty, option, dupx, dupy);
			cx = (float)intx;
			cy = (float)inty;
		}
	}

	if (pscale != FRACUNIT || (r_splitscreen && option & V_SPLITSCREEN))
	{
		fwidth = (float)(gpatch->width) * fscalew * dupx;
		fheight = (float)(gpatch->height) * fscaleh * dupy;
	}
	else
	{
		fwidth = (float)(gpatch->width) * dupx;
		fheight = (float)(gpatch->height) * dupy;
	}

	s_min = t_min = 0.0f;
	s_max = hwrPatch->max_s;
	t_max = hwrPatch->max_t;

	if (clip)
	{
		float cx1 = cx;
		float cy1 = cy;
		float cx2 = cx + fwidth;
		float cy2 = cy + fheight;

		if (cx1 < clip->left)
		{
			s_min = (clip->left - cx1) / fwidth * hwrPatch->max_s;
			cx1 = clip->left;
		}

		if (cy1 < clip->top)
		{
			t_min = (clip->top - cy1) / fheight * hwrPatch->max_t;
			cy1 = clip->top;
		}

		if (cx2 > clip->right)
		{
			s_max = s_min + (clip->right - cx1) / fwidth * hwrPatch->max_s;
			cx2 = clip->right;
		}

		if (cy2 > clip->bottom)
		{
			t_max = t_min + (clip->bottom - cy1) / fheight * hwrPatch->max_t;
			cy2 = clip->bottom;
		}

		cx = cx1;
		cy = cy1;
		fwidth = fmaxf(0.0f, cx2 - cx1);
		fheight = fmaxf(0.0f, cy2 - cy1);
	}

	// positions of the cx, cy, are between 0 and vid.width/vid.height now, we need them to be between -1 and 1
	cx = -1 + (cx / (vid.width/2));
	cy = 1 - (cy / (vid.height/2));

	// fwidth and fheight are similar
	fwidth /= vid.width / 2;
	fheight /= vid.height / 2;

	// set the polygon vertices to the right positions
	v[0].x = v[3].x = cx;
	v[2].x = v[1].x = cx + fwidth;

	v[0].y = v[1].y = cy;
	v[2].y = v[3].y = cy - fheight;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	if (option & V_FLIP)
	{
		v[0].s = v[3].s = s_max;
		v[2].s = v[1].s = s_min;
	}
	else
	{
		v[0].s = v[3].s = s_min;
		v[2].s = v[1].s = s_max;
	}

	if (option & V_VFLIP)
	{
		v[0].t = v[1].t = t_max;
		v[2].t = v[3].t = t_min;
	}
	else
	{
		v[0].t = v[1].t = t_min;
		v[2].t = v[3].t = t_max;
	}

	flags = PF_NoDepthTest;

	/*if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;*/

	// clip it since it is used for bunny scroll in doom I
	if ((blendmode = ((option & V_BLENDMASK) >> V_BLENDSHIFT)))
		blendmode++; // realign to constants
	if ((alphalevel = ((option & V_ALPHAMASK) >> V_ALPHASHIFT)) || blendmode)
	{
		FSurfaceInfo Surf;
		Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;

		flags |= HWR_GetBlendModeFlag(blendmode);

		if (alphalevel == 10)
			Surf.PolyColor.s.alpha = softwaretranstogl_lo[V_GetHUDTranslucency(option)];
		else if (alphalevel == 11)
			Surf.PolyColor.s.alpha = softwaretranstogl[V_GetHUDTranslucency(option)];
		else if (alphalevel == 12)
			Surf.PolyColor.s.alpha = softwaretranstogl_hi[V_GetHUDTranslucency(option)];
		else if (alphalevel < 10)
			Surf.PolyColor.s.alpha = softwaretranstogl[10-alphalevel];
		else // alphalevel > 12
			return;

		HWD.pfnDrawPolygon(&Surf, v, 4, flags|PF_Modulated);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags|PF_Translucent);
}

void HWR_DrawCroppedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, INT32 option, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h)
{
	FOutVector v[4];
	FBITFIELD flags;
	float cx = FIXED_TO_FLOAT(x);
	float cy = FIXED_TO_FLOAT(y);
	UINT32 alphalevel, blendmode;
	GLPatch_t *hwrPatch;

//  3--2
//  | /|
//  |/ |
//  0--1
	float dupx, dupy, fscale, fwidth, fheight;

	// make patch ready in hardware cache
	HWR_GetPatch(gpatch);
	hwrPatch = ((GLPatch_t *)gpatch->hardware);

	dupx = (float)vid.dupx;
	dupy = (float)vid.dupy;

	switch (option & V_SCALEPATCHMASK)
	{
	case V_NOSCALEPATCH:
		dupx = dupy = 1.0f;
		break;
	case V_SMALLSCALEPATCH:
		dupx = (float)vid.smalldupx;
		dupy = (float)vid.smalldupy;
		break;
	case V_MEDSCALEPATCH:
		dupx = (float)vid.meddupx;
		dupy = (float)vid.meddupy;
		break;
	}

	dupx = dupy = (dupx < dupy ? dupx : dupy);
	fscale = FIXED_TO_FLOAT(pscale);

	// fuck it, no GL support for croppedpatch V_SPLITSCREEN right now. it's not like it's accessible to Lua or anything, and we only use it for menus...

	cy -= (float)(gpatch->topoffset) * fscale;
	cx -= (float)(gpatch->leftoffset) * fscale;

	if (!(option & V_NOSCALESTART))
	{
		cx = cx * dupx;
		cy = cy * dupy;

		if (!(option & V_SCALEPATCHMASK))
		{
			// if it's meant to cover the whole screen, black out the rest
			// no the patch is cropped do not do this ever

			// centre screen
			if (fabsf((float)vid.width - (float)BASEVIDWIDTH * dupx) > 1.0E-36f)
			{
				if (option & V_SNAPTORIGHT)
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dupx));
				else if (!(option & V_SNAPTOLEFT))
					cx += ((float)vid.width - ((float)BASEVIDWIDTH * dupx))/2;
			}
			if (fabsf((float)vid.height - (float)BASEVIDHEIGHT * dupy) > 1.0E-36f)
			{
				if (option & V_SNAPTOBOTTOM)
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dupy));
				else if (!(option & V_SNAPTOTOP))
					cy += ((float)vid.height - ((float)BASEVIDHEIGHT * dupy))/2;
			}
		}
	}

	fwidth = w;
	fheight = h;

	if (sx + w > gpatch->width)
		fwidth = gpatch->width - sx;

	if (sy + h > gpatch->height)
		fheight = gpatch->height - sy;

	if (pscale != FRACUNIT)
	{
		fwidth *=  fscale * dupx;
		fheight *=  fscale * dupy;
	}
	else
	{
		fwidth *= dupx;
		fheight *= dupy;
	}

	// positions of the cx, cy, are between 0 and vid.width/vid.height now, we need them to be between -1 and 1
	cx = -1 + (cx / (vid.width/2));
	cy = 1 - (cy / (vid.height/2));

	// fwidth and fheight are similar
	fwidth /= vid.width / 2;
	fheight /= vid.height / 2;

	// set the polygon vertices to the right positions
	v[0].x = v[3].x = cx;
	v[2].x = v[1].x = cx + fwidth;

	v[0].y = v[1].y = cy;
	v[2].y = v[3].y = cy - fheight;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = ((sx)/(float)(gpatch->width))*hwrPatch->max_s;
	if (sx + w > gpatch->width)
		v[2].s = v[1].s = hwrPatch->max_s;
	else
		v[2].s = v[1].s = ((sx+w)/(float)(gpatch->width))*hwrPatch->max_s;

	v[0].t = v[1].t = ((sy)/(float)(gpatch->height))*hwrPatch->max_t;
	if (sy + h > gpatch->height)
		v[2].t = v[3].t = hwrPatch->max_t;
	else
		v[2].t = v[3].t = ((sy+h)/(float)(gpatch->height))*hwrPatch->max_t;

	flags = PF_NoDepthTest;

	/*if (option & V_WRAPX)
		flags |= PF_ForceWrapX;
	if (option & V_WRAPY)
		flags |= PF_ForceWrapY;*/

	// clip it since it is used for bunny scroll in doom I
	if ((blendmode = ((option & V_BLENDMASK) >> V_BLENDSHIFT)))
		blendmode++; // realign to constants
	if ((alphalevel = ((option & V_ALPHAMASK) >> V_ALPHASHIFT)) || blendmode)
	{
		FSurfaceInfo Surf;
		Surf.PolyColor.s.red = Surf.PolyColor.s.green = Surf.PolyColor.s.blue = 0xff;

		flags |= HWR_GetBlendModeFlag(blendmode);

		if (alphalevel == 10)
			Surf.PolyColor.s.alpha = softwaretranstogl_lo[V_GetHUDTranslucency(option)];
		else if (alphalevel == 11)
			Surf.PolyColor.s.alpha = softwaretranstogl[V_GetHUDTranslucency(option)];
		else if (alphalevel == 12)
			Surf.PolyColor.s.alpha = softwaretranstogl_hi[V_GetHUDTranslucency(option)];
		else if (alphalevel < 10)
			Surf.PolyColor.s.alpha = softwaretranstogl[10-alphalevel];
		else // alphalevel > 12
			return;

		HWD.pfnDrawPolygon(&Surf, v, 4, flags|PF_Modulated);
	}
	else
		HWD.pfnDrawPolygon(NULL, v, 4, flags|PF_Translucent);
}

void HWR_DrawPic(INT32 x, INT32 y, lumpnum_t lumpnum)
{
	FOutVector      v[4];
	const patch_t    *patch;

	// make pic ready in hardware cache
	patch = HWR_GetPic(lumpnum);

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = 2.0f * (float)x/vid.width - 1;
	v[2].x = v[1].x = 2.0f * (float)(x + patch->width*FIXED_TO_FLOAT(vid.fdupx))/vid.width - 1;
	v[0].y = v[1].y = 1.0f - 2.0f * (float)y/vid.height;
	v[2].y = v[3].y = 1.0f - 2.0f * (float)(y + patch->height*FIXED_TO_FLOAT(vid.fdupy))/vid.height;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0;
	v[2].s = v[1].s = ((GLPatch_t *)patch->hardware)->max_s;
	v[0].t = v[1].t = 0;
	v[2].t = v[3].t = ((GLPatch_t *)patch->hardware)->max_t;


	//Hurdler: Boris, the same comment as above... but maybe for pics
	// it not a problem since they don't have any transparent pixel
	// if I'm right !?
	// But then, the question is: why not 0 instead of PF_Masked ?
	// or maybe PF_Environment ??? (like what I said above)
	// BP: PF_Environment don't change anything ! and 0 is undifined
	HWD.pfnDrawPolygon(NULL, v, 4, PF_Translucent | PF_NoDepthTest);
}

// ==========================================================================
//                                                            V_VIDEO.C STUFF
// ==========================================================================


// --------------------------------------------------------------------------
// Fills a box of pixels using a flat texture as a pattern
// --------------------------------------------------------------------------
void HWR_DrawFlatFill (INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum)
{
	const size_t len = W_LumpLength(flatlumpnum);

	size_t sflatsize;
	double dflatsize;
	INT32 flatflag;

	FOutVector v[4];

	sflatsize = R_FlatDimensionsFromLumpSize(len);
	dflatsize = (double)sflatsize;
	flatflag = sflatsize - 1;

//  3--2
//  | /|
//  |/ |
//  0--1

	v[0].x = v[3].x = (x - 160.0f)/160.0f;
	v[2].x = v[1].x = ((x+w) - 160.0f)/160.0f;
	v[0].y = v[1].y = -(y - 100.0f)/100.0f;
	v[2].y = v[3].y = -((y+h) - 100.0f)/100.0f;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	// flat is 64x64 lod and texture offsets are [0.0, 1.0]
	v[0].s = v[3].s = (float)((x & flatflag)/dflatsize);
	v[2].s = v[1].s = (float)(v[0].s + w/dflatsize);
	v[0].t = v[1].t = (float)((y & flatflag)/dflatsize);
	v[2].t = v[3].t = (float)(v[0].t + h/dflatsize);

	HWR_GetRawFlat(flatlumpnum, false);

	//Hurdler: Boris, the same comment as above... but maybe for pics
	// it not a problem since they don't have any transparent pixel
	// if I'm right !?
	// BTW, I see we put 0 for PFs, and If I'm right, that
	// means we take the previous PFs as default
	// how can we be sure they are ok?
	HWD.pfnDrawPolygon(NULL, v, 4, PF_NoDepthTest); //PF_Translucent);
}


// --------------------------------------------------------------------------
// Fade down the screen so that the menu drawn on top of it looks brighter
// --------------------------------------------------------------------------
//  3--2
//  | /|
//  |/ |
//  0--1
void HWR_FadeScreenMenuBack(UINT16 color, UINT8 strength)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y = -1.0f;
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	if (color & 0xFF00) // Do COLORMAP fade.
	{
		Surf.PolyColor.rgba = UINT2RGBA(0x01010160);
		Surf.PolyColor.s.alpha = (strength*8);
	}
	else // Do TRANSMAP** fade.
	{
		Surf.PolyColor.rgba = V_GetColor(color).rgba;
		Surf.PolyColor.s.alpha = softwaretranstogl[strength];
	}
	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// -----------------+
// HWR_DrawFadeFill : draw flat coloured rectangle, with transparency
// -----------------+
void HWR_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT16 actualcolor, UINT8 strength)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh;

//  3--2
//  | /|
//  |/ |
//  0--1

	fx = (float)x;
	fy = (float)y;
	fw = (float)w;
	fh = (float)h;

	if (!(color & V_NOSCALESTART))
	{
		float dupx = (float)vid.dupx, dupy = (float)vid.dupy;
		INT32 intx, inty;

		fx *= dupx;
		fy *= dupy;
		fw *= dupx;
		fh *= dupy;

		intx = (INT32)fx;
		inty = (INT32)fy;
		V_AdjustXYWithSnap(&intx, &inty, color, dupx, dupy);
		fx = (float)intx;
		fy = (float)inty;
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
		fw = (float)vid.width - fx;
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[2].y = v[3].y = fy - fh;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	if (actualcolor & 0xFF00) // Do COLORMAP fade.
	{
		Surf.PolyColor.rgba = UINT2RGBA(0x01010160);
		Surf.PolyColor.s.alpha = (strength*8);
	}
	else // Do TRANSMAP** fade.
	{
		Surf.PolyColor.rgba = V_GetColor(actualcolor).rgba;
		Surf.PolyColor.s.alpha = softwaretranstogl[strength];
	}
	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// Draw the console background with translucency support
void HWR_DrawConsoleBack(UINT32 color, INT32 height)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;

	// setup some neat-o translucency effect
	if (!height) //cool hack 0 height is full height
		height = vid.height;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y =  1.0f-((height<<1)/(float)vid.height);
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	Surf.PolyColor.rgba = UINT2RGBA(color);
	Surf.PolyColor.s.alpha = 0x80;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

void HWR_EncoreInvertScreen(void)
{
	FOutVector v[4];
	FSurfaceInfo Surf;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y = -1.0f;
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	Surf.PolyColor.rgba = 0xFFFFFFFF;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Invert|PF_NoDepthTest);
}

void HWR_DrawCustomFadeScreen(UINT8 color, UINT8 strength)
{
	FOutVector v[4];
	FSurfaceInfo Surf;

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y = -1.0f;
	v[2].y = v[3].y =  1.0f;
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	Surf.PolyColor.rgba = V_GetColor(color).rgba;

	UINT16 workingstrength = (strength*12);
	if (workingstrength > 0xFF)
		Surf.PolyColor.s.alpha = 0xFF;
	else
		Surf.PolyColor.s.alpha = workingstrength;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// Very similar to HWR_DrawConsoleBack, except we draw from the middle(-ish) of the screen to the bottom.
void HWR_DrawTutorialBack(UINT32 color, INT32 boxheight)
{
	FOutVector  v[4];
	FSurfaceInfo Surf;
	INT32 height;
	if (boxheight < 0)
		height = -boxheight;
	else
		height = (boxheight * 4) + (boxheight/2)*5; // 4 lines of space plus gaps between and some leeway

	// setup some neat-o translucency effect

	v[0].x = v[3].x = -1.0f;
	v[2].x = v[1].x =  1.0f;
	v[0].y = v[1].y =  -1.0f;
	v[2].y = v[3].y =  -1.0f+((height<<1)/(float)vid.height);
	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 1.0f;
	v[2].t = v[3].t = 0.0f;

	Surf.PolyColor.rgba = UINT2RGBA(color);
	Surf.PolyColor.s.alpha = (color == 0 ? 0xC0 : 0x80); // make black darker, like software

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}


// ==========================================================================
//                                                             R_DRAW.C STUFF
// ==========================================================================

// ------------------
// HWR_DrawViewBorder
// Fill the space around the view window with a Doom flat texture, draw the
// beveled edges.
// 'clearlines' is useful to clear the heads up messages, when the view
// window is reduced, it doesn't refresh all the view borders.
// ------------------
void HWR_DrawViewBorder(INT32 clearlines)
{
	INT32 x, y;
	INT32 top, side;
	INT32 baseviewwidth, baseviewheight;
	INT32 basewindowx, basewindowy;
	patch_t *patch;

//    if (gl_viewwidth == vid.width)
//        return;

	if (!clearlines)
		clearlines = BASEVIDHEIGHT; // refresh all

	// calc view size based on original game resolution
	baseviewwidth =  FixedInt(FixedDiv(FLOAT_TO_FIXED(gl_viewwidth), vid.fdupx)); //(cv_viewsize.value * BASEVIDWIDTH/10)&~7;
	baseviewheight = FixedInt(FixedDiv(FLOAT_TO_FIXED(gl_viewheight), vid.fdupy));
	top = FixedInt(FixedDiv(FLOAT_TO_FIXED(gl_baseviewwindowy), vid.fdupy));
	side = FixedInt(FixedDiv(FLOAT_TO_FIXED(gl_viewwindowx), vid.fdupx));

	// top
	HWR_DrawFlatFill(0, 0,
		BASEVIDWIDTH, (top < clearlines ? top : clearlines),
		st_borderpatchnum);

	// left
	if (top < clearlines)
		HWR_DrawFlatFill(0, top, side,
			(clearlines-top < baseviewheight ? clearlines-top : baseviewheight),
			st_borderpatchnum);

	// right
	if (top < clearlines)
		HWR_DrawFlatFill(side + baseviewwidth, top, side,
			(clearlines-top < baseviewheight ? clearlines-top : baseviewheight),
			st_borderpatchnum);

	// bottom
	if (top + baseviewheight < clearlines)
		HWR_DrawFlatFill(0, top + baseviewheight,
			BASEVIDWIDTH, BASEVIDHEIGHT, st_borderpatchnum);

	//
	// draw the view borders
	//

	basewindowx = (BASEVIDWIDTH - baseviewwidth)>>1;
	if (baseviewwidth == BASEVIDWIDTH)
		basewindowy = 0;
	else
		basewindowy = top;

	// top edge
	if (clearlines > basewindowy - 8)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_T], PU_PATCH);
		for (x = 0; x < baseviewwidth; x += 8)
			HWR_DrawPatch(patch, basewindowx + x, basewindowy - 8,
				0);
	}

	// bottom edge
	if (clearlines > basewindowy + baseviewheight)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_B], PU_PATCH);
		for (x = 0; x < baseviewwidth; x += 8)
			HWR_DrawPatch(patch, basewindowx + x,
				basewindowy + baseviewheight, 0);
	}

	// left edge
	if (clearlines > basewindowy)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_L], PU_PATCH);
		for (y = 0; y < baseviewheight && basewindowy + y < clearlines;
			y += 8)
		{
			HWR_DrawPatch(patch, basewindowx - 8, basewindowy + y,
				0);
		}
	}

	// right edge
	if (clearlines > basewindowy)
	{
		patch = W_CachePatchNum(viewborderlump[BRDR_R], PU_PATCH);
		for (y = 0; y < baseviewheight && basewindowy+y < clearlines;
			y += 8)
		{
			HWR_DrawPatch(patch, basewindowx + baseviewwidth,
				basewindowy + y, 0);
		}
	}

	// Draw beveled corners.
	if (clearlines > basewindowy - 8)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_TL],
				PU_PATCH),
			basewindowx - 8, basewindowy - 8, 0);

	if (clearlines > basewindowy - 8)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_TR],
				PU_PATCH),
			basewindowx + baseviewwidth, basewindowy - 8, 0);

	if (clearlines > basewindowy+baseviewheight)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_BL],
				PU_PATCH),
			basewindowx - 8, basewindowy + baseviewheight, 0);

	if (clearlines > basewindowy + baseviewheight)
		HWR_DrawPatch(W_CachePatchNum(viewborderlump[BRDR_BR],
				PU_PATCH),
			basewindowx + baseviewwidth,
			basewindowy + baseviewheight, 0);
}



// ==========================================================================
//                                                     AM_MAP.C DRAWING STUFF
// ==========================================================================

// -----------------+
// HWR_drawAMline   : draw a line of the automap (the clipping is already done in automap code)
// Arg              : color is a RGB 888 value
// -----------------+
void HWR_drawAMline(const fline_t *fl, INT32 color)
{
	F2DCoord v1, v2;
	RGBA_t color_rgba;

	color_rgba = V_GetColor(color);

	v1.x = ((float)fl->a.x-(vid.width/2.0f))*(2.0f/vid.width);
	v1.y = ((float)fl->a.y-(vid.height/2.0f))*(2.0f/vid.height);

	v2.x = ((float)fl->b.x-(vid.width/2.0f))*(2.0f/vid.width);
	v2.y = ((float)fl->b.y-(vid.height/2.0f))*(2.0f/vid.height);

	HWD.pfnDraw2DLine(&v1, &v2, color_rgba);
}

// -----------------+
// HWR_DrawDiag     : draw flat coloured rectangle, with no texture
// -----------------+
void HWR_DrawDiag(INT32 x, INT32 y, INT32 wh, INT32 color)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh, fwait = 0;

	if (wh < 0)
		return; // consistency w/ software

//  3--2
//  | /|
//  |/ |
//  0--1

	fx = (float)x;
	fy = (float)y;
	fw = fh = (float)wh;

	if (!(color & V_NOSCALESTART))
	{
		float dupx = (float)vid.dupx, dupy = (float)vid.dupy;
		INT32 intx, inty;

		fx *= dupx;
		fy *= dupy;
		fw *= dupx;
		fh *= dupy;

		intx = (INT32)fx;
		inty = (INT32)fy;
		V_AdjustXYWithSnap(&intx, &inty, color, dupx, dupy);
		fx = (float)intx;
		fy = (float)inty;
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
	{
		fwait = fw - ((float)vid.width - fx);
		fw = (float)vid.width - fx;
	}
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[3].y = fy - fh;
	v[2].y = fy - fwait;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	Surf.PolyColor = V_GetColor(color);

	HWD.pfnDrawPolygon(&Surf, v, 4,
		PF_Modulated|PF_NoTexture|PF_NoDepthTest);
}

// -------------------+
// HWR_DrawConsoleFill     : draw flat coloured transparent rectangle because that's cool, and hw sucks less than sw for that.
// -------------------+
void HWR_DrawConsoleFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT32 actualcolor)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh;

//  3--2
//  | /|
//  |/ |
//  0--1

	fx = (float)x;
	fy = (float)y;
	fw = (float)w;
	fh = (float)h;

	if (!(color & V_NOSCALESTART))
	{
		float dupx = (float)vid.dupx, dupy = (float)vid.dupy;
		INT32 intx, inty;

		fx *= dupx;
		fy *= dupy;
		fw *= dupx;
		fh *= dupy;

		intx = (INT32)fx;
		inty = (INT32)fy;
		V_AdjustXYWithSnap(&intx, &inty, color, dupx, dupy);
		fx = (float)intx;
		fy = (float)inty;
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
		fw = (float)vid.width - fx;
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[2].y = v[3].y = fy - fh;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	Surf.PolyColor.rgba = UINT2RGBA(actualcolor);
	Surf.PolyColor.s.alpha = 0x80;

	HWD.pfnDrawPolygon(&Surf, v, 4, PF_NoTexture|PF_Modulated|PF_Translucent|PF_NoDepthTest);
}

// -----------------+
// HWR_DrawFill     : draw flat coloured rectangle, with no texture
// -----------------+
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color)
{
	FOutVector v[4];
	FSurfaceInfo Surf;
	float fx, fy, fw, fh;

//  3--2
//  | /|
//  |/ |
//  0--1

	fx = (float)x;
	fy = (float)y;
	fw = (float)w;
	fh = (float)h;

	if (!(color & V_NOSCALESTART))
	{
		float dupx = (float)vid.dupx, dupy = (float)vid.dupy;
		INT32 intx, inty;

		if (x == 0 && y == 0 && w == BASEVIDWIDTH && h == BASEVIDHEIGHT)
		{
			RGBA_t rgbaColour = V_GetColor(color);
			FRGBAFloat clearColour;
			clearColour.red = (float)rgbaColour.s.red / 255;
			clearColour.green = (float)rgbaColour.s.green / 255;
			clearColour.blue = (float)rgbaColour.s.blue / 255;
			clearColour.alpha = 1;
			HWD.pfnClearBuffer(true, false, &clearColour);
			return;
		}

		fx *= dupx;
		fy *= dupy;
		fw *= dupx;
		fh *= dupy;

		intx = (INT32)fx;
		inty = (INT32)fy;
		V_AdjustXYWithSnap(&intx, &inty, color, dupx, dupy);
		fx = (float)intx;
		fy = (float)inty;
	}

	if (fx >= vid.width || fy >= vid.height)
		return;
	if (fx < 0)
	{
		fw += fx;
		fx = 0;
	}
	if (fy < 0)
	{
		fh += fy;
		fy = 0;
	}

	if (fw <= 0 || fh <= 0)
		return;
	if (fx + fw > vid.width)
		fw = (float)vid.width - fx;
	if (fy + fh > vid.height)
		fh = (float)vid.height - fy;

	fx = -1 + fx / (vid.width / 2);
	fy = 1 - fy / (vid.height / 2);
	fw = fw / (vid.width / 2);
	fh = fh / (vid.height / 2);

	v[0].x = v[3].x = fx;
	v[2].x = v[1].x = fx + fw;
	v[0].y = v[1].y = fy;
	v[2].y = v[3].y = fy - fh;

	v[0].z = v[1].z = v[2].z = v[3].z = 1.0f;

	v[0].s = v[3].s = 0.0f;
	v[2].s = v[1].s = 1.0f;
	v[0].t = v[1].t = 0.0f;
	v[2].t = v[3].t = 1.0f;

	Surf.PolyColor = V_GetColor(color);

	HWD.pfnDrawPolygon(&Surf, v, 4,
		PF_Modulated|PF_NoTexture|PF_NoDepthTest);
}

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "png.h"
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // PNG is only used if write is supported (see ../m_misc.c)
 #endif
#endif

#ifndef USE_PNG
// --------------------------------------------------------------------------
// save screenshots with TGA format
// --------------------------------------------------------------------------
static inline boolean saveTGA(const char *file_name, void *buffer,
	INT32 width, INT32 height)
{
	INT32 fd;
	TGAHeader tga_hdr;
	INT32 i;
	UINT8 *buf8 = buffer;

	fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
	if (fd < 0)
		return false;

	memset(&tga_hdr, 0, sizeof (tga_hdr));
	tga_hdr.width = SHORT(width);
	tga_hdr.height = SHORT(height);
	tga_hdr.image_pix_size = 24;
	tga_hdr.image_type = 2;
	tga_hdr.image_descriptor = 32;

	if ( -1 == write(fd, &tga_hdr, sizeof (TGAHeader)))
	{
		close(fd);
		return false;
	}
	// format to 888 BGR
	for (i = 0; i < width * height * 3; i+=3)
	{
		const UINT8 temp = buf8[i];
		buf8[i] = buf8[i+2];
		buf8[i+2] = temp;
	}
	if ( -1 == write(fd, buffer, width * height * 3))
	{
		close(fd);
		return false;
	}
	close(fd);
	return true;
}
#endif

// --------------------------------------------------------------------------
// screen shot
// --------------------------------------------------------------------------

UINT8 *HWR_GetScreenshot(void)
{
	UINT8 *buf = malloc(vid.width * vid.height * 3 * sizeof (*buf));

	if (!buf)
		return NULL;
	// returns 24bit 888 RGB
	HWD.pfnReadRect(0, 0, vid.width, vid.height, vid.width * 3, (void *)buf);
	return buf;
}

boolean HWR_Screenshot(const char *pathname)
{
	boolean ret;
	UINT8 *buf = malloc(vid.width * vid.height * 3 * sizeof (*buf));

	if (!buf)
	{
		CONS_Debug(DBG_RENDER, "HWR_Screenshot: Failed to allocate memory\n");
		return false;
	}

	// returns 24bit 888 RGB
	HWD.pfnReadRect(0, 0, vid.width, vid.height, vid.width * 3, (void *)buf);

#ifdef USE_PNG
	ret = M_SavePNG(pathname, buf, vid.width, vid.height, NULL);
#else
	ret = saveTGA(pathname, buf, vid.width, vid.height);
#endif
	free(buf);
	return ret;
}

#endif //HWRENDER
