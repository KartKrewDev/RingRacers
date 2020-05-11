// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_color.h
/// \brief Skincolor & colormapping code

#ifndef __K_COLOR__
#define __K_COLOR__

#include "doomdef.h"
#include "doomtype.h"

#define SKIN_RAMP_LENGTH 16
#define DEFAULT_STARTTRANSCOLOR 96
#define NUM_PALETTE_ENTRIES 256

extern UINT8 colortranslations[MAXTRANSLATIONS][16];
extern const char *KartColor_Names[MAXSKINCOLORS];
extern const UINT8 KartColor_Opposite[MAXSKINCOLORS*2];

/*--------------------------------------------------
	UINT8 K_ColorRelativeLuminance(UINT8 r, UINT8 g, UINT8 b);

		Gives you the brightness value of the provided RGB value, based on how the human eye interprets it.
		See https://en.wikipedia.org/wiki/Relative_luminance for more info.

	Input Arguments:-
		r - Red component
		g - Green component
		b - Blue component

	Return:-
		Brightness value from 0 to 255.
--------------------------------------------------*/

UINT8 K_ColorRelativeLuminance(UINT8 r, UINT8 g, UINT8 b);

/*--------------------------------------------------
	void K_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor);

		Generates a colormap to "colorize" all palette indicies
		to the provided skincolor.

	Input Arguments:-
		dest_colormap - Colormap to populate.
		skincolor - Translation color.

	Return:-
		None
--------------------------------------------------*/

void K_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor);

/*--------------------------------------------------
	void K_GenerateKartColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color);

		Generates a translation colormap for Kart, to replace R_GenerateTranslationColormap in r_draw.c

	Input Arguments:-
		dest_colormap - Colormap to populate.
		skinnum - Number of skin or translation mode (TC_ constants)
		color - Translation color.

	Return:-
		None
--------------------------------------------------*/
void K_GenerateKartColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color);

/*--------------------------------------------------
	UINT8 K_GetKartColorByName(const char *name);

		Finds the corresponding SKINCOLOR_ constant to the string provided.

	Input Arguments:-
		name - The name of the color desired.

	Return:-
		SKINCOLOR_ constant, SKINCOLOR_NONE if invalid
--------------------------------------------------*/
UINT8 K_GetKartColorByName(const char *name);

#endif
