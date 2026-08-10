// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
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
#include "m_cond.h"
#include "g_demo.h"
#include "k_follower.h"

/*--------------------------------------------------
	uint8_t K_ColorRelativeLuminance(uint8_t r, uint8_t g, uint8_t b)

		See header file for description.
--------------------------------------------------*/
uint8_t K_ColorRelativeLuminance(uint8_t r, uint8_t g, uint8_t b)
{
	double redWeight = ((r * 1.0) / UINT8_MAX);
	double greenWeight = ((g * 1.0) / UINT8_MAX);
	double blueWeight = ((b * 1.0) / UINT8_MAX);
	double brightness = 0.5;

	redWeight = pow(redWeight, 2.2) * 0.2126;
	greenWeight = pow(greenWeight, 2.2) * 0.7152;
	blueWeight = pow(greenWeight, 2.2) * 0.0722;

	brightness = pow(redWeight + greenWeight + blueWeight, 1.0 / 2.2);

	return (uint8_t)(brightness * UINT8_MAX);
}

/*--------------------------------------------------
	uint16_t K_RainbowColor(tic_t time)

		See header file for description.
--------------------------------------------------*/

uint16_t K_RainbowColor(tic_t time)
{
	return (uint16_t)(FIRSTRAINBOWCOLOR + (time % (FIRSTSUPERCOLOR - FIRSTRAINBOWCOLOR)));
}

/*--------------------------------------------------
	void K_RainbowColormap(uint8_t *dest_colormap, skincolornum_t skincolor)

		See header file for description.
--------------------------------------------------*/
void K_RainbowColormap(uint8_t *dest_colormap, skincolornum_t skincolor)
{
	int32_t i;
	RGBA_t color;
	uint8_t brightness;
	int32_t j;
	uint8_t colorbrightnesses[16];
	uint16_t brightdif;
	int32_t temp;

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
			dest_colormap[i] = (uint8_t)i;
			continue;
		}

		color = V_GetColor(i);
		brightness = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);
		brightdif = 256;

		for (j = 0; j < 16; j++)
		{
			temp = abs((int16_t)brightness - (int16_t)colorbrightnesses[j]);

			if (temp < brightdif)
			{
				brightdif = (uint16_t)temp;
				dest_colormap[i] = skincolors[skincolor].ramp[j];
			}
		}
	}
}

/*--------------------------------------------------
	uint8_t K_HitlagColorValue(RGBA_t color)

		See header file for description.
--------------------------------------------------*/
uint8_t K_HitlagColorValue(RGBA_t color)
{
	// Outputs a raw brightness value (makes OGL support easier)
	int32_t output = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);

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
	void K_HitlagColormap(uint8_t *dest_colormap)

		See header file for description.
--------------------------------------------------*/
void K_HitlagColormap(uint8_t *dest_colormap)
{
	RGBA_t color;
	uint8_t v, offset;
	int32_t i;

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
	static void K_IntermissionColormap(uint8_t *dest_colormap)

		Turns warm colors tan, and cool colors steel-blue.
--------------------------------------------------*/
static void K_IntermissionColormap(uint8_t *dest_colormap)
{
	RGBA_t color;
	int32_t i;

	// for every colour in the palette, check its
	for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
	{
		color = V_GetColor(i);

		uint8_t lo = min(min(color.s.red, color.s.green), color.s.blue);
		uint8_t hi = max(max(color.s.red, color.s.green), color.s.blue);

		double hue = 0.0;
		if (lo != hi)
		{
			if (hi == color.s.red)
			{
				hue = (color.s.green - color.s.blue) / (hi - lo);
			}
			else if (hi == color.s.green)
			{
				hue = 2.0 + (color.s.blue - color.s.red) / (hi - lo);
			}
			else
			{
				hue = 4.0 + (color.s.red - color.s.green) / (hi - lo);
			}

			if (hue < 0.0)
			{
				hue += 6.0;
			}
		}

		skincolornum_t skincolor = SKINCOLOR_INTERMISSION1;
		const double blue_start = 3.0;
		const double blue_end = 5.0;
		const double green_buffer = 0.5;
		if (hue > blue_start && hue < blue_end)
		{
			skincolor = SKINCOLOR_INTERMISSION3;
		}
		else if (hue > blue_start - green_buffer && hue < blue_start + green_buffer)
		{
			skincolor = SKINCOLOR_INTERMISSION2;
		}

		int32_t lum = K_ColorRelativeLuminance(color.s.red, color.s.green, color.s.blue);
		int32_t skincolor_index = ((255 - lum) * 15) / 255;

		dest_colormap[i] = skincolors[skincolor].ramp[skincolor_index];
	}
}

/*--------------------------------------------------
	void K_GenerateKartColormap(uint8_t *dest_colormap, int32_t skinnum, skincolornum_t color)

		See header file for description.
--------------------------------------------------*/
void K_GenerateKartColormap(uint8_t *dest_colormap, int32_t skinnum, skincolornum_t color)
{
	int32_t i;
	int32_t starttranscolor;

	if (skinnum == TC_HITLAG)
	{
		K_HitlagColormap(dest_colormap);
		return;
	}
	else if (skinnum == TC_INTERMISSION)
	{
		K_IntermissionColormap(dest_colormap);
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
				dest_colormap[i] = (uint8_t)i;
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

	starttranscolor = (skinnum != TC_DEFAULT) ? skins[skinnum]->starttranscolor : DEFAULT_STARTTRANSCOLOR;

	// Fill in the entries of the palette that are fixed
	for (i = 0; i < starttranscolor; i++)
		dest_colormap[i] = (uint8_t)i;

	for (i = (uint8_t)(starttranscolor + 16); i < NUM_PALETTE_ENTRIES; i++)
		dest_colormap[i] = (uint8_t)i;

	// Build the translated ramp
	for (i = 0; i < SKIN_RAMP_LENGTH; i++)
	{
		// Sryder 2017-10-26: What was here before was most definitely not particularly readable, check above for new color translation table
		dest_colormap[starttranscolor + i] = skincolors[color].ramp[i];
	}
}

/*--------------------------------------------------
	dboolean K_ColorUsable(skincolornum_t color, dboolean follower, dboolean locked)

		See header file for description.
--------------------------------------------------*/
dboolean K_ColorUsable(skincolornum_t color, dboolean follower, dboolean locked)
{
	int32_t i = MAXUNLOCKABLES;

	if (color == FOLLOWERCOLOR_MATCH || color == FOLLOWERCOLOR_OPPOSITE)
	{
		// Special follower colors, always allow if follower.
		return follower;
	}

	if (skincolors[color].accessible == false)
	{
		// Never intended to be used.
		return false;
	}

	if (demo.playback || !locked)
	{
		// Simplifies things elsewhere...
		return true;
	}

	// Determine if this follower is supposed to be unlockable or not
	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		skincolornum_t cid = SKINCOLOR_NONE;

		if (unlockables[i].type != SECRET_COLOR)
		{
			continue;
		}

		cid = M_UnlockableColorNum(&unlockables[i]);

		if (cid != color)
		{
			continue;
		}

		// i is now the unlockable index, we can use this later
		break;
	}

	if (i == MAXUNLOCKABLES)
	{
		// Didn't trip anything, so we can use this color.
		return true;
	}

	// Use the unlockables table directly
	// DEFINITELY not M_CheckNetUnlockByID
	return (dboolean)(gamedata->unlocked[i]);
}

//}
