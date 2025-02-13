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
/// \file  r_draw_column.cpp
/// \brief column drawer functions
/// \note  no includes because this is included as part of r_draw.cpp

// ==========================================================================
// COLUMNS
// ==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.
//

enum DrawColumnType
{
	DC_BASIC			= 0x0000,
	DC_COLORMAP			= 0x0001,
	DC_TRANSMAP			= 0x0002,
	DC_BRIGHTMAP		= 0x0004,
	DC_HOLES			= 0x0008,
	DC_LIGHTLIST		= 0x0010,
};

template<DrawColumnType Type>
static constexpr UINT8 R_GetColumnTranslated(drawcolumndata_t* dc, UINT8 col)
{
	if constexpr (Type & DrawColumnType::DC_COLORMAP)
	{
		return dc->translation[col];
	}
	else
	{
		return col;
	}
}

template<DrawColumnType Type>
static constexpr UINT8 R_GetColumnBrightmapped(drawcolumndata_t* dc, UINT32 bit, UINT8 col)
{
	col = R_GetColumnTranslated<Type>(dc, col);

	if constexpr (Type & DrawColumnType::DC_BRIGHTMAP)
	{
		if (dc->brightmap[bit] == BRIGHTPIXEL)
		{
			return dc->fullbright[col];
		}
	}

	return dc->colormap[col];
}

template<DrawColumnType Type>
static constexpr UINT8 R_GetColumnTranslucent(drawcolumndata_t* dc, UINT8 *dest, UINT32 bit, UINT8 col)
{
	col = R_GetColumnBrightmapped<Type>(dc, bit, col);

	if constexpr (Type & DrawColumnType::DC_TRANSMAP)
	{
		return *(dc->transmap + (col << 8) + (*dest));
	}
	else
	{
		return col;
	}
}

template<DrawColumnType Type>
static constexpr UINT8 R_DrawColumnPixel(drawcolumndata_t* dc, UINT8 *dest, UINT32 bit)
{
	UINT8 col = dc->source[bit];

	if constexpr (Type & DrawColumnType::DC_HOLES)
	{
		if (col == TRANSPARENTPIXEL)
		{
			return *dest;
		}
	}

	return R_GetColumnTranslucent<Type>(dc, dest, bit, col);
}

/**	\brief The R_DrawColumn function
	Experiment to make software go faster. Taken from the Boom source
*/
template<DrawColumnType Type>
static void R_DrawColumnTemplate(drawcolumndata_t *dc)
{
	INT32 count;
	UINT8 *dest;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
	{
		return;
	}

	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
	{
		return;
	}

	if constexpr (Type & DrawColumnType::DC_LIGHTLIST)
	{
		constexpr DrawColumnType NewType = static_cast<DrawColumnType>(Type & ~DC_LIGHTLIST);
		INT32 i, realyh, height, bheight = 0, solid = 0;
		drawcolumndata_t dc_copy = *dc;

		realyh = dc_copy.yh;

		// This runs through the lightlist from top to bottom and cuts up the column accordingly.
		for (i = 0; i < dc_copy.numlights; i++)
		{
			// If the height of the light is above the column, get the colormap
			// anyway because the lighting of the top should be affected.
			solid = dc_copy.lightlist[i].flags & FOF_CUTSOLIDS;
			height = dc_copy.lightlist[i].height >> LIGHTSCALESHIFT;

			if (solid)
			{
				bheight = dc_copy.lightlist[i].botheight >> LIGHTSCALESHIFT;

				if (bheight < height)
				{
					// confounded slopes sometimes allow partial invertedness,
					// even including cases where the top and bottom heights
					// should actually be the same!
					// swap the height values as a workaround for this quirk
					INT32 temp = height;
					height = bheight;
					bheight = temp;
				}
			}

			if (height <= dc_copy.yl)
			{
				dc_copy.colormap = dc_copy.lightlist[i].rcolormap;
				dc_copy.fullbright = colormaps;

				if (encoremap)
				{
					dc_copy.colormap += COLORMAP_REMAPOFFSET;
					dc_copy.fullbright += COLORMAP_REMAPOFFSET;
				}

				if (solid && dc_copy.yl < bheight)
				{
					dc_copy.yl = bheight;
				}

				continue;
			}

			// Found a break in the column!
			dc_copy.yh = height;

			if (dc_copy.yh > realyh)
			{
				dc_copy.yh = realyh;
			}

			R_DrawColumnTemplate<NewType>(&dc_copy);
			if (solid)
			{
				dc_copy.yl = bheight;
			}
			else
			{
				dc_copy.yl = dc_copy.yh + 1;
			}

			dc_copy.colormap = dc_copy.lightlist[i].rcolormap;
			dc_copy.fullbright = colormaps;
			if (encoremap)
			{
				dc_copy.colormap += COLORMAP_REMAPOFFSET;
				dc_copy.fullbright += COLORMAP_REMAPOFFSET;
			}
		}

		dc_copy.yh = realyh;

		if (dc_copy.yl <= realyh)
		{
			R_DrawColumnTemplate<NewType>(&dc_copy);
		}
	}
	else
	{
		fixed_t fracstep;
		fixed_t frac;
		INT32 heightmask;
		INT32 npow2min;
		INT32 npow2max;

		// Framebuffer destination address.
		// Use ylookup LUT to avoid multiply with ScreenWidth.
		// Use columnofs LUT for subwindows?

		//dest = ylookup[dc_yl] + columnofs[dc_x];
		dest = &topleft[dc->yl * vid.width + dc->x];

		count++;

		// Determine scaling, which is the only mapping to be done.
		fracstep = dc->iscale;
		//frac = dc_texturemid + (dc_yl - centery)*fracstep;
		frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep)) * (!dc->hires);

		// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
		// This is as fast as it gets.
		heightmask = dc->sourcelength-1;
		npow2min = -1;
		npow2max = dc->sourcelength;

		if (dc->sourcelength & heightmask)   // not a power of 2 -- killough
		{
			heightmask = dc->texheight << FRACBITS;

			if (frac < 0)
			{
				while ((frac += heightmask) < 0)
				{
					;
				}
			}
			else
			{
				while (frac >= heightmask)
				{
					frac -= heightmask;
				}
			}

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix

				// -1 is the lower clamp bound because column posts have a "safe" byte before the real data
				// and a few bytes after as well
				//*dest = R_DrawColumnPixel<Type>(dc, dest, std::clamp(frac >> FRACBITS, npow2min, npow2max));
				{
					// jartha: faster on my AMD FX-6300 CPU.
					// Faster than ternaries, faster than std::min/std::max. Don't ask me why.
					// I tested by viewing a non-PO2 texture from a consistent distance so it covered the entire screen.
					// The framerate difference was about 50 frames at 640x400.
					INT32 n = frac >> FRACBITS;
					if (n < npow2min)
						n = npow2min;
					if (n > npow2max)
						n = npow2max;
					*dest = R_DrawColumnPixel<Type>(dc, dest, n);
				}

				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
				{
					frac += fracstep - heightmask;
				}
				else
				{
					frac += fracstep;
				}

				while (frac >= heightmask)
				{
					frac -= heightmask;
				}
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = R_DrawColumnPixel<Type>(dc, dest, (frac>>FRACBITS) & heightmask);

				dest += vid.width;
				frac += fracstep;

				*dest = R_DrawColumnPixel<Type>(dc, dest, (frac>>FRACBITS) & heightmask);

				dest += vid.width;
				frac += fracstep;
			}

			if (count & 1)
			{
				*dest = R_DrawColumnPixel<Type>(dc, dest, (frac>>FRACBITS) & heightmask);
			}
		}
	}
}

#define DEFINE_COLUMN_FUNC(name, flags) \
	void name(drawcolumndata_t *dc) \
	{ \
		ZoneScoped; \
		constexpr DrawColumnType opt = static_cast<DrawColumnType>(flags); \
		R_DrawColumnTemplate<opt>(dc); \
	}

#define DEFINE_COLUMN_COMBO(name, flags) \
	DEFINE_COLUMN_FUNC(name, flags) \
	DEFINE_COLUMN_FUNC(name ## _Brightmap, flags|DC_BRIGHTMAP)

DEFINE_COLUMN_COMBO(R_DrawColumn, DC_BASIC)
DEFINE_COLUMN_COMBO(R_DrawTranslucentColumn, DC_TRANSMAP)
DEFINE_COLUMN_COMBO(R_DrawTranslatedColumn, DC_COLORMAP)
DEFINE_COLUMN_COMBO(R_DrawColumnShadowed, DC_LIGHTLIST)
DEFINE_COLUMN_COMBO(R_DrawTranslatedTranslucentColumn, DC_COLORMAP|DC_TRANSMAP)
DEFINE_COLUMN_COMBO(R_Draw2sMultiPatchColumn, DC_HOLES)
DEFINE_COLUMN_COMBO(R_Draw2sMultiPatchTranslucentColumn, DC_HOLES|DC_TRANSMAP)

void R_DrawFogColumn(drawcolumndata_t *dc)
{
	ZoneScoped;

	INT32 count;
	UINT8 *dest;

	count = dc->yh - dc->yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Determine scaling, which is the only mapping to be done.
	do
	{
		// Simple. Apply the colormap to what's already on the screen.
		*dest = dc->colormap[*dest];
		dest += vid.width;
	}
	while (count--);
}

void R_DrawDropShadowColumn(drawcolumndata_t *dc)
{
	ZoneScoped;

	// Hack: A cut-down copy of R_DrawTranslucentColumn_8 that does not read texture
	// data since something about calculating the texture reading address for drop shadows is broken.
	// dc_texturemid and dc_iscale get wrong values for drop shadows, however those are not strictly
	// needed for the current design of the shadows, so this function bypasses the issue
	// by not using those variables at all.

	INT32 count;
	UINT8 *dest;

	count = dc->yh - dc->yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	dest = &topleft[dc->yl*vid.width + dc->x];

	const UINT8 *transmap_offset = dc->transmap + (dc->shadowcolor << 8);
	while ((count -= 2) >= 0)
	{
		*dest = *(transmap_offset + (*dest));
		dest += vid.width;
		*dest = *(transmap_offset + (*dest));
		dest += vid.width;
	}

	if (count & 1)
		*dest = *(transmap_offset + (*dest));
}

void R_DrawColumn_Flat(drawcolumndata_t *dc)
{
	ZoneScoped;

	INT32 count;
	UINT8 color = dc->lightmap[dc->r8_flatcolor];
	UINT8 *dest;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	count++;

	do
	{
		*dest = color;
		dest += vid.width;
	}
	while (--count);
}
