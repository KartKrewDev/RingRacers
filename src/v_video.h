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
/// \file  v_video.h
/// \brief Gamma correction LUT

#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomdef.h"
#include "doomtype.h"
#include "r_defs.h"

// SRB2Kart
#include "hu_stuff.h" // fonts

#ifdef __cplusplus

#include "hwr2/twodee.hpp"

namespace srb2
{
extern hwr2::Twodee g_2d;
} // namespace srb2

extern "C" {
#endif

//
// VIDEO
//

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

extern UINT8 *screens[5];

extern consvar_t cv_ticrate, cv_constextsize,
cv_globalgamma, cv_globalsaturation,
cv_rhue, cv_yhue, cv_ghue, cv_chue, cv_bhue, cv_mhue,
cv_rgamma, cv_ygamma, cv_ggamma, cv_cgamma, cv_bgamma, cv_mgamma,
cv_rsaturation, cv_ysaturation, cv_gsaturation, cv_csaturation, cv_bsaturation, cv_msaturation,
cv_palette, cv_palettenum;

extern consvar_t cv_songcredits;

// Allocates buffer screens, call before R_Init.
void V_Init(void);

// Recalculates the viddef (dupx, dupy, etc.) according to the current screen resolution.
void V_Recalc(void);

// Color look-up table
#define CLUTINDEX(r, g, b) (((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)

struct colorlookup_t
{
	boolean init;
	RGBA_t palette[256];
	UINT16 table[0xFFFF];
};

void InitColorLUT(colorlookup_t *lut, RGBA_t *palette, boolean makecolors);
UINT8 GetColorLUT(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b);
UINT8 GetColorLUTDirect(colorlookup_t *lut, UINT8 r, UINT8 g, UINT8 b);

// Loads the correct palette into memory
void V_ReloadPalette(void);

// Set the current RGB palette lookup to use for palettized graphics
void V_SetPalette(INT32 palettenum);

void V_SetPaletteLump(const char *pal);

const char *R_GetPalname(UINT16 num);
const char *GetPalette(void);

extern RGBA_t *pLocalPalette;
extern RGBA_t *pMasterPalette;
extern RGBA_t *pGammaCorrectedPalette;

UINT32 V_GammaCorrect(UINT32 input, double power);
#define V_GammaDecode(input) V_GammaCorrect(input, 2.2f)
#define V_GammaEncode(input) V_GammaCorrect(input, (1/2.2f))

void V_CubeApply(RGBA_t *input);

// Retrieve the ARGB value from a palette color index
#define V_GetColor(color) (pLocalPalette[color&0xFF])
#define V_GetMasterColor(color) (pMasterPalette[color&0xFF])

// Bottom 8 bits are used for parameter (character)
#define V_PARAMMASK          0x000000FF

// Bottom bit is used for screen (patches)
#define V_SCREENMASK          0x0000000F

#define V_STRINGDANCE        0x00000002 // (strings/characters only) funny undertale
#define V_VFLIP              0x00000010 // (patches only) Vertical flip

// flags hacked in scrn (not supported by all functions (see src))
// patch scaling uses bits 9 and 10
#define V_SCALEPATCHSHIFT    8
#define V_SCALEPATCHMASK     0x00000300
#define V_NOSCALEPATCH       0x00000100
#define V_SMALLSCALEPATCH    0x00000200
#define V_MEDSCALEPATCH      0x00000300

// string spacing uses bits 11 and 12
#define V_SPACINGMASK        0x00000C00
#define V_6WIDTHSPACE        0x00000400 // early 2.1 style spacing, variable widths, 6 character space
#define V_OLDSPACING         0x00000800 // Old style spacing, 8 per character 4 per space
#define V_MONOSPACE          0x00000C00 // Don't do width checks on characters, all characters 8 width

// use bits 13-16 for colors
#define V_CHARCOLORSHIFT     12
#define V_CHARCOLORMASK      0x0000F000
// for simplicity's sake, shortcuts to specific colors
#define V_PURPLEMAP          0x00001000
#define V_YELLOWMAP          0x00002000
#define V_GREENMAP           0x00003000
#define V_BLUEMAP            0x00004000
#define V_REDMAP             0x00005000
#define V_GRAYMAP            0x00006000
#define V_ORANGEMAP          0x00007000
#define V_SKYMAP             0x00008000
#define V_LAVENDERMAP        0x00009000
#define V_GOLDMAP            0x0000A000
#define V_AQUAMAP            0x0000B000
#define V_MAGENTAMAP         0x0000C000
#define V_PINKMAP            0x0000D000
#define V_BROWNMAP           0x0000E000
#define V_TANMAP             0x0000F000

// use bits 17-20 for alpha transparency
#define V_ALPHASHIFT         16
#define V_ALPHAMASK          0x000F0000
// define specific translucencies
#define V_10TRANS            0x00010000
#define V_20TRANS            0x00020000
#define V_30TRANS            0x00030000
#define V_40TRANS            0x00040000
#define V_TRANSLUCENT        0x00050000 // TRANS50
#define V_60TRANS            0x00060000
#define V_70TRANS            0x00070000
#define V_80TRANS            0x00080000 // used to be V_8020TRANS
#define V_90TRANS            0x00090000
#define V_HUDTRANSHALF       0x000A0000
#define V_HUDTRANS           0x000B0000 // draw the hud translucent
#define V_HUDTRANSDOUBLE     0x000C0000

// use bits 21-23 for blendmodes
#define V_BLENDSHIFT         20
#define V_BLENDMASK          0x00700000
// preshifted blend flags minus 1 as effects don't distinguish between AST_COPY and AST_TRANSLUCENT
#define V_ADD                ((AST_ADD-1)<<V_BLENDSHIFT) // Additive
#define V_SUBTRACT           ((AST_SUBTRACT-1)<<V_BLENDSHIFT) // Subtractive
#define V_REVERSESUBTRACT    ((AST_REVERSESUBTRACT-1)<<V_BLENDSHIFT) // Reverse subtractive
#define V_MODULATE           ((AST_MODULATE-1)<<V_BLENDSHIFT) // Modulate
#define V_OVERLAY            ((AST_OVERLAY-1)<<V_BLENDSHIFT) // Overlay

#define V_SNAPTOTOP          0x01000000 // for centering
#define V_SNAPTOBOTTOM       0x02000000 // for centering
#define V_SNAPTOLEFT         0x04000000 // for centering
#define V_SNAPTORIGHT        0x08000000 // for centering

#define V_FORCEUPPERCASE     0x10000000 // (strings only) prevents fonts that have lowercase letters from using them
#define V_FLIP               0x10000000 // (patches only) Horizontal flip
#define V_SLIDEIN            0x20000000 // Slide in from the sides on level load, depending on snap flags

#define V_NOSCALESTART       0x40000000 // don't scale x, y, start coords
#define V_SPLITSCREEN        0x80000000 // Add half of screen width or height automatically depending on player number

UINT32 V_GetHUDTranslucency(INT32 scrn);

void V_AdjustXYWithSnap(INT32 *x, INT32 *y, UINT32 options, INT32 dupx, INT32 dupy);

struct cliprect_t
{
	fixed_t left, right, top, bottom;
	INT32 flags;
	boolean enabled;
};

const cliprect_t *V_GetClipRect(void);
void V_SetClipRect(fixed_t x, fixed_t y, fixed_t w, fixed_t h, INT32 flags);
void V_ClearClipRect(void);
void V_SaveClipRect(cliprect_t *copy);
void V_RestoreClipRect(const cliprect_t *copy);

// defines for old functions
#define V_DrawPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s|V_NOSCALESTART|V_NOSCALEPATCH, p, NULL)
#define V_DrawTranslucentMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s, p, c)
#define V_DrawSmallTranslucentMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, c)
#define V_DrawTinyTranslucentMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, c)
#define V_DrawMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s, p, c)
#define V_DrawSmallMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, c)
#define V_DrawTinyMappedPatch(x,y,s,p,c) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, c)
#define V_DrawScaledPatch(x,y,s,p) V_DrawFixedPatch((x)*FRACUNIT, (y)<<FRACBITS, FRACUNIT, s, p, NULL)
#define V_DrawSmallScaledPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, NULL)
#define V_DrawTinyScaledPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, NULL)
#define V_DrawTranslucentPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT, s, p, NULL)
#define V_DrawSmallTranslucentPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, s, p, NULL)
#define V_DrawTinyTranslucentPatch(x,y,s,p) V_DrawFixedPatch((x)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, s, p, NULL)
#define V_DrawSciencePatch(x,y,s,p,sc) V_DrawFixedPatch(x,y,sc,s,p,NULL)
#define V_DrawFixedPatch(x,y,sc,s,p,c) V_DrawStretchyFixedPatch(x,y,sc,sc,s,p,c)
void V_DrawStretchyFixedPatch(fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 scrn, patch_t *patch, const UINT8 *colormap);
void V_DrawCroppedPatch(fixed_t x, fixed_t y, fixed_t pscale, INT32 scrn, patch_t *patch, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);

void V_DrawContinueIcon(INT32 x, INT32 y, INT32 flags, INT32 skinnum, UINT16 skincolor);

// Draw a linear block of pixels into the view buffer.
void V_DrawBlock(INT32 x, INT32 y, INT32 scrn, INT32 width, INT32 height, const UINT8 *src);

// fill a box with a single color
void V_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c);
void V_DrawFillConsoleMap(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c);
// fill a triangle with a single color
void V_DrawDiag(INT32 x, INT32 y, INT32 wh, INT32 c);
// fill a box with a flat as a pattern
void V_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatnum);


// draw wobbly VHS pause stuff
void V_DrawVhsEffect(boolean rewind);

// fade down the screen buffer before drawing the menu over
void V_DrawFadeScreen(UINT16 color, UINT8 strength);
// available to lua over my dead body, which will probably happen in this heat
void V_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 c, UINT16 color, UINT8 strength);

lighttable_t *V_LoadCustomFadeMap(const char *lump);
const UINT8 *V_OffsetIntoFadeMap(const lighttable_t *clm, UINT8 strength);
void V_DrawCustomFadeScreen(const char *lump, UINT8 strength);
void V_DrawFadeConsBack(INT32 plines);

void V_EncoreInvertScreen(void);

void V_DrawPromptBack(INT32 boxheight, INT32 color);

/* Convenience macros for leagacy string function macros. */
#define V__DrawOneScaleString( x,y,scale,option,cm,font,string ) \
	V_DrawStringScaled(x,y,scale,FRACUNIT,FRACUNIT,option,cm,font,string)
#define V__DrawDupxString( x,y,scale,option,cm,font,string )\
	V__DrawOneScaleString ((x)<<FRACBITS,(y)<<FRACBITS,scale,option,cm,font,string)

#define V__OneScaleStringWidth( scale,option,font,string ) \
	V_StringScaledWidth(scale,FRACUNIT,FRACUNIT,option,font,string)
#define V__IntegerStringWidth( scale,option,font,string ) \
	(V_StringScaledWidth(scale,FRACUNIT,FRACUNIT,option,font,string) / FRACUNIT)

INT32 V_DanceYOffset(INT32 counter);

// draw a single character
void V_DrawCharacterScaled(fixed_t x, fixed_t y, fixed_t scale, INT32 flags, int font, int c, UINT8 *colormap);
void V_DrawCharacter(INT32 x, INT32 y, INT32 option, boolean lowercase);

void V_DrawChatCharacter(INT32 x, INT32 y, INT32 c, boolean lowercaseallowed, UINT8 *colormap);

UINT8 *V_GetStringColormap(INT32 colorflags);

#define V_DrawLevelTitle( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,LT_FONT,string)

// Find string width from lt_font chars
#define V_LevelNameWidth( string ) \
	V__IntegerStringWidth ( FRACUNIT,0,LT_FONT,string )

INT32 V_LevelNameHeight(const char *string);

// draw a string using a font
void V_DrawStringScaled(
		fixed_t     x,
		fixed_t     y,
		fixed_t           scale,
		fixed_t     space_scale,
		fixed_t  linefeed_scale,
		INT32       flags,
		const UINT8 *colormap,
		int         font,
		const char *text);

fixed_t V_StringScaledWidth(
		fixed_t      scale,
		fixed_t spacescale,
		fixed_t    lfscale,
		INT32      flags,
		int        fontno,
		const char *s);

char * V_ScaledWordWrap(
		fixed_t          w,
		fixed_t      scale,
		fixed_t spacescale,
		fixed_t    lfscale,
		INT32      flags,
		int        fontno,
		const char *s);

// draw a string using the hu_font
#define V_DrawString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,HU_FONT,string)

#define V_StringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,HU_FONT,string )

void V_DrawCenteredString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedString(INT32 x, INT32 y, INT32 option, const char *string);

// draw a string using the hu_font, 0.5x scale
#define V_DrawSmallString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT>>1,option,NULL,HU_FONT,string)

#define V_SmallStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT>>1,option,HU_FONT,string )

void V_DrawCenteredSmallString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedSmallString(INT32 x, INT32 y, INT32 option, const char *string);

// draw a string using the tny_font
#define V_DrawThinString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,TINY_FONT,string)

#define V_ThinStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,TINY_FONT,string )

void V_DrawCenteredThinString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedThinString(INT32 x, INT32 y, INT32 option, const char *string);

#define V_DrawStringAtFixed( x,y,option,string ) \
	V__DrawOneScaleString (x,y,FRACUNIT,option,NULL,HU_FONT,string)
void V_DrawCenteredStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawRightAlignedStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);

#define V_DrawThinStringAtFixed( x,y,option,string ) \
	V__DrawOneScaleString (x,y,FRACUNIT,option,NULL,TINY_FONT,string)
void V_DrawCenteredThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);
void V_DrawRightAlignedThinStringAtFixed(fixed_t x, fixed_t y, INT32 option, const char *string);

// Draws a titlecard font string.
// timer: when the letters start appearing (leave to 0 to disable)
// threshold: when the letters start disappearing (leave to 0 to disable) (both are INT32 in case you supply negative values...)
// NOTE: This function ignores most conventional string flags (V_RETURN8, V_FORCEUPPERCASE ...)
// NOTE: This font only works with uppercase letters.
void V_DrawTitleCardStringFixed(fixed_t x, fixed_t y, fixed_t scale, const char *str, INT32 flags, boolean bossmode, INT32 timer, INT32 threshold, boolean p4);
#define V_DrawTitleCardString( x,y,str,flags,bossmode,timer,threshold,p4 ) \
	V_DrawTitleCardStringFixed ((x)<<FRACBITS,(y)<<FRACBITS,FRACUNIT,str,flags,bossmode,timer,threshold,p4)
	
// returns thr width of a string drawn using the above function.
INT32 V_TitleCardStringWidth(const char *str, boolean p4);

// offset that can be subtracted to center align.
INT32 V_CenteredTitleCardStringOffset(const char *str, boolean p4);

// Draw tall nums, used for menu, HUD, intermission
void V_DrawTallNum(INT32 x, INT32 y, INT32 flags, INT32 num);
void V_DrawPaddedTallNum(INT32 x, INT32 y, INT32 flags, INT32 num, INT32 digits);

// Draw ping numbers. Used by the scoreboard and that one ping option. :P
// This is a separate function because IMO lua should have access to it as well.
fixed_t V_DrawPingNum(fixed_t x, fixed_t y, INT32 flags, INT32 num, const UINT8 *colormap);

void V_DrawProfileNum(INT32 x, INT32 y, INT32 flags, UINT8 num);

#define V_DrawCreditString( x,y,option,string ) \
	V__DrawOneScaleString (x,y,FRACUNIT,option,NULL,CRED_FONT,string)

#define V_CreditStringWidth( string ) \
	V__IntegerStringWidth ( FRACUNIT,0,CRED_FONT,string )

// SRB2Kart
#define V_DrawTimerString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,TIMER_FONT,string)

#define V_TimerStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,TIMER_FONT,string )

void V_DrawCenteredTimerString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedTimerString(INT32 x, INT32 y, INT32 option, const char *string);

#define V_DrawMenuString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,MENU_FONT,string)

#define V_MenuStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,MENU_FONT,string )

void V_DrawCenteredMenuString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedMenuString(INT32 x, INT32 y, INT32 option, const char *string);

#define V_DrawGamemodeString( x,y,option,cm,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,cm,GM_FONT,string)

#define V_GamemodeStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,GM_FONT,string )

void V_DrawCenteredGamemodeString(INT32 x, INT32 y, INT32 option, const UINT8 *colormap, const char *string);
void V_DrawRightAlignedGamemodeString(INT32 x, INT32 y, INT32 option, const UINT8 *colormap, const char *string);

#define V_DrawFileString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,FILE_FONT,string)

#define V_FileStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,FILE_FONT,string )

void V_DrawCenteredFileString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedFileString(INT32 x, INT32 y, INT32 option, const char *string);

#define V_DrawLSTitleHighString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,LSHI_FONT,string)

#define V_LSTitleHighStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,LSHI_FONT,string )

void V_DrawCenteredLSTitleHighString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedLSTitleHighString(INT32 x, INT32 y, INT32 option, const char *string);

#define V_DrawLSTitleLowString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,LSLOW_FONT,string)

#define V_LSTitleLowStringWidth( string,option ) \
	V__IntegerStringWidth ( FRACUNIT,option,LSLOW_FONT,string )

void V_DrawCenteredLSTitleLowString(INT32 x, INT32 y, INT32 option, const char *string);
void V_DrawRightAlignedLSTitleLowString(INT32 x, INT32 y, INT32 option, const char *string);

void V_DrawPatchFill(patch_t *pat);

void VID_BlitLinearScreen(const UINT8 *srcptr, UINT8 *destptr, INT32 width, INT32 height, size_t srcrowbytes,
	size_t destrowbytes);

/**
 * Display the software framebuffer to the screen. Added in RHI conversion; software is not implicitly displayed by the
 * system.
 */
void VID_DisplaySoftwareScreen(void);

char *V_ParseText(const char *rawText); // Launder srb2::draw::TextElement.parse() through C code!


#ifdef __cplusplus
} // extern "C"
#endif

#endif
