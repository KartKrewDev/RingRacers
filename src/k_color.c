// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_color.c
/// \brief Skincolor & colormapping code

#include "k_color.h"

#include "doomdef.h"
#include "doomtype.h"
#include "r_draw.h"
#include "r_things.h"
#include "v_video.h"

/*--------------------------------------------------
	UINT8 K_ColorRelativeLuminance(UINT8 r, UINT8 g, UINT8 b)

		See header file for description.
--------------------------------------------------*/
UINT8 K_ColorRelativeLuminance(UINT8 r, UINT8 g, UINT8 b)
{
	// These are the BT.601 coefficents
	// See also: https://en.wikipedia.org/wiki/Rec._601
	UINT32 redweight = 299 * r;
	UINT32 greenweight = 587 * g;
	UINT32 blueweight = 114 * b;
	return min((redweight + greenweight + blueweight) / 1000, UINT8_MAX);
}

/*--------------------------------------------------
	UINT16 K_RainbowColor(tic_t time)

		See header file for description.
--------------------------------------------------*/

UINT16 K_RainbowColor(tic_t time)
{
	return (UINT16)(FIRSTRAINBOWCOLOR + (time % (FIRSTSUPERCOLOR - FIRSTRAINBOWCOLOR)));
}

/*--------------------------------------------------
	void K_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor)

		See header file for description.
--------------------------------------------------*/
void K_RainbowColormap(UINT8 *dest_colormap, UINT8 skincolor)
{
	INT32 i;
	RGBA_t color;
	UINT8 brightness;
	INT32 j;
	UINT8 colorbrightnesses[16];
	UINT16 brightdif;
	INT32 temp;

	// first generate the brightness of all the colours of that skincolour
	for (i = 0; i < 16; i++)
	{
		color = V_GetColor(skincolors[skincolor].ramp[i]);
		colorbrightnesses[i] = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);
	}

	// next, for every colour in the palette, choose the transcolor that has the closest brightness
	for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		if (i == 0 || i == 31) // pure black and pure white don't change
		{
			dest_colormap[i] = (UINT8)i;
			continue;
		}

		color = V_GetColor(i);
		brightness = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);
		brightdif = 256;

		for (j = 0; j < 16; j++)
		{
			temp = abs((INT16)brightness - (INT16)colorbrightnesses[j]);

			if (temp < brightdif)
			{
				brightdif = (UINT16)temp;
				dest_colormap[i] = skincolors[skincolor].ramp[j];
			}
		}
	}
}

/*--------------------------------------------------
	UINT8 K_HitlagColorValue(RGBA_t color)

		See header file for description.
--------------------------------------------------*/
UINT8 K_HitlagColorValue(RGBA_t color)
{
	// Outputs a raw brightness value (makes OGL support easier)
	INT32 output = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);

	// Invert the color
	output = 255 - output;

	// Increase the contrast
	output = ((output-128) * 2) + 128;

	// Make sure to cap it.
	if (output > 255)
	{
		output = 255;
	}
	else if (output < 0)
	{
		output = 0;
	}

	return output;
}

/*--------------------------------------------------
	void K_HitlagColormap(UINT8 *dest_colormap)

		See header file for description.
--------------------------------------------------*/
void K_HitlagColormap(UINT8 *dest_colormap)
{
	RGBA_t color;
	UINT8 v, offset;
	INT32 i;

	// for every colour in the palette, invert, greyscale, and increase the contrast.
	for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		color = V_GetColor(i);
		v = K_HitlagColorValue(color);

		// Convert raw brightness value to an offset from the greyscale palette line
		offset = (255 - v) / 8; 

		dest_colormap[i] = offset; // Starts from 0, add it if greyscale moves.
	}
}

/*--------------------------------------------------
	void K_GenerateKartColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color)

		See header file for description.
--------------------------------------------------*/
void K_GenerateKartColormap(UINT8 *dest_colormap, INT32 skinnum, UINT8 color)
{
	INT32 i;
	INT32 starttranscolor;

	if (skinnum == TC_HITLAG)
	{
		K_HitlagColormap(dest_colormap);
		return;
	}
	else if (skinnum == TC_BOSS
		|| skinnum == TC_ALLWHITE
		|| skinnum == TC_METALSONIC
		|| skinnum == TC_BLINK
		|| color == SKINCOLOR_NONE)
	{
		// Handle a couple of simple special cases
		for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
		{
			if (skinnum == TC_ALLWHITE)
				dest_colormap[i] = 0;
			else if (skinnum == TC_BLINK)
				dest_colormap[i] = skincolors[color].ramp[3];
			else
				dest_colormap[i] = (UINT8)i;
		}

		// White!
		if (skinnum == TC_BOSS)
			dest_colormap[31] = 0;
		else if (skinnum == TC_METALSONIC)
			dest_colormap[143] = 0;

		return;
	}
	else if (skinnum == TC_RAINBOW)
	{
		K_RainbowColormap(dest_colormap, color);
		return;
	}

	starttranscolor = (skinnum != TC_DEFAULT) ? skins[skinnum].starttranscolor : DEFAULT_STARTTRANSCOLOR;

	// Fill in the entries of the palette that are fixed
	for (i = 0; i < starttranscolor; i++)
		dest_colormap[i] = (UINT8)i;

	for (i = (UINT8)(starttranscolor + 16); i < NUM_PALETTE_ENTRIES; i++)
		dest_colormap[i] = (UINT8)i;

	// Build the translated ramp
	for (i = 0; i < SKIN_RAMP_LENGTH; i++)
	{
		// Sryder 2017-10-26: What was here before was most definitely not particularly readable, check above for new color translation table
		dest_colormap[starttranscolor + i] = skincolors[color].ramp[i];
	}
}

//}
