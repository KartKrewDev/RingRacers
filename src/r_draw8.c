// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw8.c
/// \brief 8bpp span/column drawer functions
/// \note  no includes because this is included as part of r_draw.c

// ==========================================================================
// COLUMNS
// ==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.
//

/**	\brief The R_DrawColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawColumn_8(drawcolumndata_t* dc)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl * vid.width + dc->x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const UINT8 *brightmap = dc->brightmap;
		register const lighttable_t *colormap = dc->colormap;
		register const lighttable_t *fullbright = dc->fullbright;
		register INT32 heightmask = dc->texheight-1;
		if (dc->texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				if (brightmap != NULL && brightmap[frac>>FRACBITS] == BRIGHTPIXEL)
				{
					*dest = fullbright[source[frac>>FRACBITS]];
				}
				else
				{
					*dest = colormap[source[frac>>FRACBITS]];
				}
				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
				{
					*dest = fullbright[source[(frac>>FRACBITS) & heightmask]];
				}
				else
				{
					*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				}

				dest += vid.width;
				frac += fracstep;

				if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
				{
					*dest = fullbright[source[(frac>>FRACBITS) & heightmask]];
				}
				else
				{
					*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				}

				dest += vid.width;
				frac += fracstep;
			}

			if (count & 1)
			{
				if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
				{
					*dest = fullbright[source[(frac>>FRACBITS) & heightmask]];
				}
				else
				{
					*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				}
			}
		}
	}
}

void R_Draw2sMultiPatchColumn_8(drawcolumndata_t* dc)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl * vid.width + dc->x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const UINT8 *brightmap = dc->brightmap;
		register const lighttable_t *colormap = dc->colormap;
		register const lighttable_t *fullbright = dc->fullbright;
		register INT32 heightmask = dc->texheight-1;
		register UINT8 val;
		if (dc->texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				val = source[frac>>FRACBITS];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[frac>>FRACBITS] == BRIGHTPIXEL)
					{
						*dest = fullbright[val];
					}
					else
					{
						*dest = colormap[val];
					}
				}

				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
					{
						*dest = fullbright[val];
					}
					else
					{
						*dest = colormap[val];
					}
				}

				dest += vid.width;
				frac += fracstep;

				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
					{
						*dest = fullbright[val];
					}
					else
					{
						*dest = colormap[val];
					}
				}

				dest += vid.width;
				frac += fracstep;
			}

			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
					{
						*dest = fullbright[val];
					}
					else
					{
						*dest = colormap[val];
					}
				}
			}
		}
	}
}

void R_Draw2sMultiPatchTranslucentColumn_8(drawcolumndata_t* dc)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc->yh - dc->yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl * vid.width + dc->x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const UINT8 *brightmap = dc->brightmap;
		register const UINT8 *transmap = dc->transmap;
		register const lighttable_t *colormap = dc->colormap;
		register const lighttable_t *fullbright = dc->fullbright;
		register INT32 heightmask = dc->texheight-1;
		register UINT8 val;
		if (dc->texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				val = source[frac>>FRACBITS];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[frac>>FRACBITS] == BRIGHTPIXEL)
					{
						*dest = *(transmap + (fullbright[val]<<8) + (*dest));
					}
					else
					{
						*dest = *(transmap + (colormap[val]<<8) + (*dest));
					}
				}

				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
					{
						*dest = *(transmap + (fullbright[val]<<8) + (*dest));
					}
					else
					{
						*dest = *(transmap + (colormap[val]<<8) + (*dest));
					}
				}

				dest += vid.width;
				frac += fracstep;

				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
					{
						*dest = *(transmap + (fullbright[val]<<8) + (*dest));
					}
					else
					{
						*dest = *(transmap + (colormap[val]<<8) + (*dest));
					}
				}

				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[(frac>>FRACBITS) & heightmask] == BRIGHTPIXEL)
					{
						*dest = *(transmap + (fullbright[val]<<8) + (*dest));
					}
					else
					{
						*dest = *(transmap + (colormap[val]<<8) + (*dest));
					}
				}
			}
		}
	}
}

/**	\brief The R_DrawShadeColumn_8 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawShadeColumn_8(drawcolumndata_t* dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	// check out coords for src*
	if ((dc->yl < 0) || (dc->x >= vid.width))
		return;

	count = dc->yh - dc->yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawShadeColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl * vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Here we do an additional index re-mapping.
	do
	{
		*dest = colormaps[(dc->source[frac>>FRACBITS] <<8) + (*dest)];
		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

/**	\brief The R_DrawTranslucentColumn_8 function
	I've made an asm routine for the transparency, because it slows down
	a lot in 640x480 with big sprites (bfg on all screen, or transparent
	walls on fullscreen)
*/
void R_DrawTranslucentColumn_8(drawcolumndata_t* dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc->yh - dc->yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl * vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc->source;
		register const UINT8 *brightmap = dc->brightmap;
		register const UINT8 *transmap = dc->transmap;
		register const lighttable_t *colormap = dc->colormap;
		register const lighttable_t *fullbright = dc->fullbright;
		register INT32 heightmask = dc->texheight - 1;
		if (dc->texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				// using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				if (brightmap != NULL && brightmap[frac>>FRACBITS] == BRIGHTPIXEL)
				{
					*dest = *(transmap + (fullbright[source[frac>>FRACBITS]]<<8) + (*dest));
				}
				else
				{
					*dest = *(transmap + (colormap[source[frac>>FRACBITS]]<<8) + (*dest));
				}
				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				if (brightmap != NULL && brightmap[(frac>>FRACBITS)&heightmask] == BRIGHTPIXEL)
				{
					*dest = *(transmap + (fullbright[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				}
				else
				{
					*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				}
				dest += vid.width;
				frac += fracstep;

				if (brightmap != NULL && brightmap[(frac>>FRACBITS)&heightmask] == BRIGHTPIXEL)
				{
					*dest = *(transmap + (fullbright[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				}
				else
				{
					*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				}
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				if (brightmap != NULL && brightmap[(frac>>FRACBITS)&heightmask] == BRIGHTPIXEL)
				{
					*dest = *(transmap + (fullbright[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				}
				else
				{
					*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				}
			}
		}
	}
}

// Hack: A cut-down copy of R_DrawTranslucentColumn_8 that does not read texture
// data since something about calculating the texture reading address for drop shadows is broken.
// dc_texturemid and dc_iscale get wrong values for drop shadows, however those are not strictly
// needed for the current design of the shadows, so this function bypasses the issue
// by not using those variables at all.
void R_DrawDropShadowColumn_8(drawcolumndata_t* dc)
{
	register INT32 count;
	register UINT8 *dest;

	count = dc->yh - dc->yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	dest = &topleft[dc->yl*vid.width + dc->x];

	{
		register const UINT8 *transmap_offset = dc->transmap + (dc->shadowcolor << 8);
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
}

/**	\brief The R_DrawTranslatedTranslucentColumn_8 function
	Spiffy function. Not only does it colormap a sprite, but does translucency as well.
	Uber-kudos to Cyan Helkaraxe
*/
void R_DrawTranslatedTranslucentColumn_8(drawcolumndata_t* dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc->yh - dc->yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl * vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl - centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register INT32 heightmask = dc->texheight - 1;
		if (dc->texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix

				if (dc->brightmap != NULL && dc->brightmap[frac>>FRACBITS] == BRIGHTPIXEL)
				{
					*dest = *(dc->transmap + (dc->fullbright[dc->translation[dc->source[frac>>FRACBITS]]]<<8) + (*dest));
				}
				else
				{
					*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[frac>>FRACBITS]]]<<8) + (*dest));
				}

				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				if (dc->brightmap != NULL && dc->brightmap[(frac>>FRACBITS)&heightmask] == BRIGHTPIXEL)
				{
					*dest = *(dc->transmap + (dc->fullbright[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				}
				else
				{
					*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				}

				dest += vid.width;
				frac += fracstep;

				if (dc->brightmap != NULL && dc->brightmap[(frac>>FRACBITS)&heightmask] == BRIGHTPIXEL)
				{
					*dest = *(dc->transmap + (dc->fullbright[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				}
				else
				{
					*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				}

				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				if (dc->brightmap != NULL && dc->brightmap[(frac>>FRACBITS)&heightmask] == BRIGHTPIXEL)
				{
					*dest = *(dc->transmap + (dc->fullbright[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				}
				else
				{
					*dest = *(dc->transmap + (dc->colormap[dc->translation[dc->source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				}
			}
		}
	}
}

/**	\brief The R_DrawTranslatedColumn_8 function
	Draw columns up to 128 high but remap the green ramp to other colors

  \warning STILL NOT IN ASM, TO DO..
*/
void R_DrawTranslatedColumn_8(drawcolumndata_t* dc)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc->yh - dc->yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// FIXME. As above.
	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc->yl*vid.width + dc->x];

	// Looks familiar.
	fracstep = dc->iscale;
	//frac = dc_texturemid + (dc_yl-centery)*fracstep;
	frac = (dc->texturemid + FixedMul((dc->yl << FRACBITS) - centeryfrac, fracstep))*(!dc->hires);

	// Here we do an additional index re-mapping.
	do
	{
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo.
		if (dc->brightmap != NULL && dc->brightmap[frac>>FRACBITS] == BRIGHTPIXEL)
		{
			*dest = dc->fullbright[dc->translation[dc->source[frac>>FRACBITS]]];
		}
		else
		{
			*dest = dc->colormap[dc->translation[dc->source[frac>>FRACBITS]]];
		}

		dest += vid.width;

		frac += fracstep;
	} while (count--);
}

// ==========================================================================
// SPANS
// ==========================================================================

#define SPANSIZE 16
#define INVSPAN 0.0625f

// <Callum> 4194303 = (2048x2048)-1 (2048x2048 is maximum flat size)
#define MAXFLATBYTES 4194303

/**	\brief The R_DrawSpan_8 function
	Draws the actual span.
*/
void R_DrawSpan_8 (drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	brightmap = ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	if (dest+8 > deststop)
		return;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!

		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				dest[i] = fullbright[source[bit]];
			}
			else
			{
				dest[i] = colormap[source[bit]];
			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
		{
			*dest = fullbright[source[bit]];
		}
		else
		{
			*dest = colormap[source[bit]];
		}

		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

// R_CalcTiltedLighting
// Exactly what it says on the tin. I wish I wasn't too lazy to explain things properly.
void R_CalcTiltedLighting(INT32 *lightbuffer, INT32 x1, INT32 x2, fixed_t start, fixed_t end)
{
	// ZDoom uses a different lighting setup to us, and I couldn't figure out how to adapt their version
	// of this function. Here's my own.
	INT32 left = x1, right = x2;
	fixed_t step = (end-start)/(x2 - x1 + 1);
	INT32 i;

	// I wanna do some optimizing by checking for out-of-range segments on either side to fill in all at once,
	// but I'm too bad at coding to not crash the game trying to do that. I guess this is fast enough for now...

	for (i = left; i <= right; i++) {
		lightbuffer[i] = (start += step) >> FRACBITS;
		if (lightbuffer[i] < 0)
			lightbuffer[i] = 0;
		else if (lightbuffer[i] >= MAXLIGHTSCALE)
			lightbuffer[i] = MAXLIGHTSCALE-1;
	}
}

/**	\brief The R_DrawTiltedSpan_8 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_8(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;
	INT32 tiltlighting[MAXVIDWIDTH];

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds->szp.x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(tiltlighting, ds->x1, ds->x2, FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds->sup.z + ds->sup.y*(centery-ds->y) + ds->sup.x*(ds->x1-centerx);
	vz = ds->svp.z + ds->svp.y*(centery-ds->y) + ds->svp.x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];

	source = ds->source;
	brightmap = ds->brightmap;
	//colormap = ds_colormap;
	fullbright = ds->fullbright;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);
		bit = ((v >> nflatyshift) & nflatmask) | (u >> nflatxshift);
		if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
		{
			*dest = fullbright[source[bit]];
		}
		else
		{
			colormap = planezlight[tiltlighting[ds_x1]] + (ds_colormap - colormaps);
			*dest = colormap[source[bit]];
		}
		dest++;
		ds_x1++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp.x * SPANSIZE;
	uzstep = ds->sup.x * SPANSIZE;
	vzstep = ds->svp.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = fullbright[source[bit]];
			}
			else
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				*dest = colormap[source[bit]];
			}
			dest++;
			ds->x1++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = fullbright[source[bit]];
			}
			else
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				*dest = colormap[source[bit]];
			}
			ds->x1++;
		}
		else
		{
			double left = width;
			iz += ds->szp.x * left;
			uz += ds->sup.x * left;
			vz += ds->svp.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = fullbright[source[bit]];
				}
				else
				{
					colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
					*dest = colormap[source[bit]];
				}
				dest++;
				ds->x1++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedTranslucentSpan_8 function
	Like DrawTiltedSpan, but translucent
*/
void R_DrawTiltedTranslucentSpan_8(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;
	INT32 tiltlighting[MAXVIDWIDTH];

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds->szp.x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(tiltlighting, ds->x1, ds->x2, FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds->sup.z + ds->sup.y*(centery-ds->y) + ds->sup.x*(ds->x1-centerx);
	vz = ds->svp.z + ds->svp.y*(centery-ds->y) + ds->svp.x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];

	source = ds->source;
	brightmap = ds->brightmap;
	//colormap = ds_colormap;
	fullbright = ds->fullbright;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		bit = ((v >> nflatyshift) & nflatmask) | (u >> nflatxshift);
		if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
		{
			*dest = *(ds_transmap + (fullbright[source[bit]] << 8) + *dest);
		}
		else
		{
			colormap = planezlight[tiltlighting[ds_x1]] + (ds_colormap - colormaps);
			*dest = *(ds_transmap + (colormap[source[bit]] << 8) + *dest);
		}
		dest++;
		ds_x1++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp.x * SPANSIZE;
	uzstep = ds->sup.x * SPANSIZE;
	vzstep = ds->svp.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dest);
			}
			else
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dest);
			}
			dest++;
			ds->x1++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dest);
			}
			else
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dest);
			}
			ds->x1++;
		}
		else
		{
			double left = width;
			iz += ds->szp.x * left;
			uz += ds->sup.x * left;
			vz += ds->svp.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dest);
				}
				else
				{
					colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
					*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dest);
				}
				dest++;
				ds->x1++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedTranslucentWaterSpan_8 function
	Like DrawTiltedTranslucentSpan, but for water
*/
void R_DrawTiltedTranslucentWaterSpan_8(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;
	UINT8 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;
	INT32 tiltlighting[MAXVIDWIDTH];

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds->szp.x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(tiltlighting, ds->x1, ds->x2, FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds->sup.z + ds->sup.y*(centery-ds->y) + ds->sup.x*(ds->x1-centerx);
	vz = ds->svp.z + ds->svp.y*(centery-ds->y) + ds->svp.x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	dsrc = screens[1] + (ds->y+ds->bgofs)*vid.width + ds->x1;
	source = ds->source;
	brightmap = ds->brightmap;
	//colormap = ds_colormap;
	fullbright = ds->fullbright;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		bit = ((v >> nflatyshift) & nflatmask) | (u >> nflatxshift);
		if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
		{
			*dest = *(ds_transmap + (fullbright[source[bit]] << 8) + *dsrc);
		}
		else
		{
			colormap = planezlight[tiltlighting[ds_x1]] + (ds_colormap - colormaps);
			*dest = *(ds_transmap + (colormap[source[bit]] << 8) + *dsrc);
		}
		dest++;
		ds_x1++;
		dsrc++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp.x * SPANSIZE;
	uzstep = ds->sup.x * SPANSIZE;
	vzstep = ds->svp.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dsrc);
			}
			else
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dsrc);
			}
			dest++;
			ds->x1++;
			dsrc++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dsrc);
			}
			else
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dsrc);
			}
			ds->x1++;
		}
		else
		{
			double left = width;
			iz += ds->szp.x * left;
			uz += ds->sup.x * left;
			vz += ds->svp.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dsrc);
				}
				else
				{
					colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
					*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dsrc);
				}
				dest++;
				ds->x1++;
				dsrc++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_8(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;

	UINT8 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;
	INT32 tiltlighting[MAXVIDWIDTH];

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds->szp.x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(tiltlighting, ds->x1, ds->x2, FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	uz = ds->sup.z + ds->sup.y*(centery-ds->y) + ds->sup.x*(ds->x1-centerx);
	vz = ds->svp.z + ds->svp.y*(centery-ds->y) + ds->svp.x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];

	source = ds->source;
	brightmap = ds->brightmap;
	//colormap = ds_colormap;
	fullbright = ds->fullbright;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		bit = ((v >> nflatyshift) & nflatmask) | (u >> nflatxshift);
		val = source[bit];
		if (val != TRANSPARENTPIXEL)
		{
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = fullbright[val];
			}
			else
			{
				colormap = planezlight[tiltlighting[ds_x1]] + (ds_colormap - colormaps);
				*dest = colormap[val];
			}
		}

		dest++;
		ds_x1++;
		iz += ds_szp->x;
		uz += ds_sup->x;
		vz += ds_svp->x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp.x * SPANSIZE;
	uzstep = ds->sup.x * SPANSIZE;
	vzstep = ds->svp.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			val = source[bit];
			if (val != TRANSPARENTPIXEL)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = fullbright[val];
				}
				else
				{
					colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
					*dest = colormap[val];
				}
			}
			dest++;
			ds->x1++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			val = source[bit];
			if (val != TRANSPARENTPIXEL)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = fullbright[val];
				}
				else
				{
					colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
					*dest = colormap[val];
				}
				ds->x1++;
			}
		}
		else
		{
			double left = width;
			iz += ds->szp.x * left;
			uz += ds->sup.x * left;
			vz += ds->svp.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
				val = source[bit];
				if (val != TRANSPARENTPIXEL)
				{
					if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
					{
						*dest = fullbright[val];
					}
					else
					{
						colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
						*dest = colormap[val];
					}
				}
				dest++;
				ds->x1++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawSplat_8 function
	Just like R_DrawSpan_8, but skips transparent pixels.
*/
void R_DrawSplat_8 (drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	brightmap = ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
			bit &= MAXFLATBYTES;
			val = source[bit];
			if (val != TRANSPARENTPIXEL)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					dest[i] = fullbright[val];
				}
				else
				{
					dest[i] = colormap[val];
				}
			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[bit];
		if (val != TRANSPARENTPIXEL)
		{
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = fullbright[val];
			}
			else
			{
				*dest = colormap[val];
			}
		}
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentSplat_8 function
	Just like R_DrawSplat_8, but is translucent!
*/
void R_DrawTranslucentSplat_8 (drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	brightmap = ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
			val = source[bit];
			if (val != TRANSPARENTPIXEL)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					dest[i] = *(ds->transmap + (fullbright[val] << 8) + dest[i]);
				}
				else
				{
					dest[i] = *(ds->transmap + (colormap[val] << 8) + dest[i]);
				}

			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[bit];
		if (val != TRANSPARENTPIXEL)
		{
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = *(ds->transmap + (fullbright[val] << 8) + *dest);
			}
			else
			{
				*dest = *(ds->transmap + (colormap[val] << 8) + *dest);
			}

		}
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFloorSprite_8 function
	Just like R_DrawSplat_8, but for floor sprites.
*/
void R_DrawFloorSprite_8 (drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT16 *source;
	UINT16 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = (UINT16 *)ds->source;
	brightmap = (UINT16 *)ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	translation = ds->translation;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
			val = source[bit];
			if (val & 0xFF00)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					dest[i] = fullbright[translation[val & 0xFF]];
				}
				else
				{
					dest[i] = colormap[translation[val & 0xFF]];
				}
			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[bit];
		if (val & 0xFF00)
		{
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = fullbright[translation[val & 0xFF]];
			}
			else
			{
				*dest = colormap[translation[val & 0xFF]];
			}
		}
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentFloorSplat_8 function
	Just like R_DrawFloorSprite_8, but is translucent!
*/
void R_DrawTranslucentFloorSprite_8 (drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT16 *source;
	UINT16 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;
	UINT32 val;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = (UINT16 *)ds->source;
	brightmap = (UINT16 *)ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	translation = ds->translation;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
			val = source[bit];
			if (val & 0xFF00)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					dest[i] = *(ds->transmap + (fullbright[translation[val & 0xFF]] << 8) + dest[i]);
				}
				else
				{
					dest[i] = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + dest[i]);
				}
			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		val = source[bit];
		if (val & 0xFF00)
		{
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				*dest = *(ds->transmap + (fullbright[translation[val & 0xFF]] << 8) + *dest);
			}
			else
			{
				*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
			}
		}
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedFloorSprite_8 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_8(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT16 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *translation;
	UINT8 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);
	uz = ds->sup.z + ds->sup.y*(centery-ds->y) + ds->sup.x*(ds->x1-centerx);
	vz = ds->svp.z + ds->svp.y*(centery-ds->y) + ds->svp.x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = (UINT16 *)ds->source;
	brightmap = (UINT16 *)ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	translation = ds->translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp.x * SPANSIZE;
	uzstep = ds->sup.x * SPANSIZE;
	vzstep = ds->svp.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			val = source[bit];
			if (val & 0xFF00)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = fullbright[translation[val & 0xFF]];
				}
				else
				{
					*dest = colormap[translation[val & 0xFF]];
				}
			}
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			val = source[bit];
			if (val & 0xFF00)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = fullbright[translation[val & 0xFF]];
				}
				else
				{
					*dest = colormap[translation[val & 0xFF]];
				}
			}
		}
		else
		{
			double left = width;
			iz += ds->szp.x * left;
			uz += ds->sup.x * left;
			vz += ds->svp.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
				val = source[bit];
				if (val & 0xFF00)
				{
					if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
					{
						*dest = fullbright[translation[val & 0xFF]];
					}
					else
					{
						*dest = colormap[translation[val & 0xFF]];
					}
				}
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTiltedTranslucentFloorSprite_8 function
	Draws a tilted, translucent, floor sprite.
*/
void R_DrawTiltedTranslucentFloorSprite_8(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT16 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *translation;
	UINT8 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);
	uz = ds->sup.z + ds->sup.y*(centery-ds->y) + ds->sup.x*(ds->x1-centerx);
	vz = ds->svp.z + ds->svp.y*(centery-ds->y) + ds->svp.x*(ds->x1-centerx);

	dest = ylookup[ds->y] + columnofs[ds->x1];
	source = (UINT16 *)ds->source;
	brightmap = (UINT16 *)ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	translation = ds->translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds->szp.x * SPANSIZE;
	uzstep = ds->sup.x * SPANSIZE;
	vzstep = ds->svp.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			val = source[bit];
			if (val & 0xFF00)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = *(ds->transmap + (fullbright[translation[val & 0xFF]] << 8) + *dest);
				}
				else
				{
					*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
				}
			}
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
			val = source[bit];
			if (val & 0xFF00)
			{
				if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
				{
					*dest = *(ds->transmap + (fullbright[translation[val & 0xFF]] << 8) + *dest);
				}
				else
				{
					*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
				}
			}
		}
		else
		{
			double left = width;
			iz += ds->szp.x * left;
			uz += ds->sup.x * left;
			vz += ds->svp.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				bit = ((v >> ds->nflatyshift) & ds->nflatmask) | (u >> ds->nflatxshift);
				val = source[bit];
				if (val & 0xFF00)
				{
					if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
					{
						*dest = *(ds->transmap + (fullbright[translation[val & 0xFF]] << 8) + *dest);
					}
					else
					{
						*dest = *(ds->transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
					}
				}
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTranslucentSpan_8 function
	Draws the actual span with translucency.
*/
void R_DrawTranslucentSpan_8 (drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	source = ds->source;
	brightmap = ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	dest = ylookup[ds->y] + columnofs[ds->x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				dest[i] = *(ds->transmap + (fullbright[source[bit]] << 8) + dest[i]);
			}
			else
			{
				dest[i] = *(ds->transmap + (colormap[source[bit]] << 8) + dest[i]);
			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);
		if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
		{
			*dest = *(ds->transmap + (fullbright[source[bit]] << 8) + *dest);
		}
		else
		{
			*dest = *(ds->transmap + (colormap[source[bit]] << 8) + *dest);
		}
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawTranslucentWaterSpan_8(drawspandata_t* ds)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;
	UINT32 bit;

	UINT8 *source;
	UINT8 *brightmap;
	UINT8 *colormap;
	UINT8 *fullbright;
	UINT8 *dest;
	UINT8 *dsrc;

	size_t count;
	size_t i;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds->xfrac << ds->nflatshiftup; yposition = (ds->yfrac + ds->waterofs) << ds->nflatshiftup;
	xstep = ds->xstep << ds->nflatshiftup; ystep = ds->ystep << ds->nflatshiftup;

	source = ds->source;
	brightmap = ds->brightmap;
	colormap = ds->colormap;
	fullbright = ds->fullbright;
	dest = ylookup[ds->y] + columnofs[ds->x1];
	dsrc = screens[1] + (ds->y+ds->bgofs)*vid.width + ds->x1;
	count = ds->x2 - ds->x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		for (i = 0; i < 8; i++)
		{
			bit = ((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift);
			if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
			{
				dest[i] = fullbright[*(ds->transmap + (source[bit] << 8) + dsrc[i])];
			}
			else
			{
				dest[i] = colormap[*(ds->transmap + (source[bit] << 8) + dsrc[i])];
			}
			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		dsrc += 8;
		count -= 8;
	}
	while (count--)
	{
		bit = ((yposition >> ds->nflatyshift) & ds->nflatmask) | (xposition >> ds->nflatxshift);
		if (brightmap != NULL && brightmap[bit] == BRIGHTPIXEL)
		{
			*dest = fullbright[*(ds->transmap + (source[bit] << 8) + *dsrc)];
		}
		else
		{
			*dest = colormap[*(ds->transmap + (source[bit] << 8) + *dsrc)];
		}
		dest++;
		dsrc++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFogSpan_8 function
	Draws the actual span with fogging.
*/
void R_DrawFogSpan_8(drawspandata_t* ds)
{
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;

	colormap = ds->colormap;
	//dest = ylookup[ds_y] + columnofs[ds_x1];
	dest = &topleft[ds->y *vid.width + ds->x1];

	count = ds->x2 - ds->x1 + 1;

	while (count >= 4)
	{
		dest[0] = colormap[dest[0]];
		dest[1] = colormap[dest[1]];
		dest[2] = colormap[dest[2]];
		dest[3] = colormap[dest[3]];

		dest += 4;
		count -= 4;
	}

	while (count--)
	{
		*dest = colormap[*dest];
		dest++;
	}
}

/**	\brief The R_DrawFogColumn_8 function
	Fog wall.
*/
void R_DrawFogColumn_8(drawcolumndata_t* dc)
{
	INT32 count;
	UINT8 *dest;

	count = dc->yh - dc->yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawFogColumn_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

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
	} while (count--);
}

/**	\brief The R_DrawShadeColumn_8 function
	This is for 3D floors that cast shadows on walls.

	This function just cuts the column up into sections and calls R_DrawColumn_8
*/
void R_DrawColumnShadowed_8(drawcolumndata_t* dc)
{
	INT32 count, realyh, i, height, bheight = 0, solid = 0;

	realyh = dc->yh;

	count = dc->yh - dc->yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc->x >= (unsigned)vid.width || dc->yl < 0 || dc->yh >= vid.height)
		I_Error("R_DrawColumnShadowed_8: %d to %d at %d", dc->yl, dc->yh, dc->x);
#endif

	// This runs through the lightlist from top to bottom and cuts up the column accordingly.
	for (i = 0; i < dc->numlights; i++)
	{
		// If the height of the light is above the column, get the colormap
		// anyway because the lighting of the top should be affected.
		solid = dc->lightlist[i].flags & FOF_CUTSOLIDS;

		height = dc->lightlist[i].height >> LIGHTSCALESHIFT;
		if (solid)
		{
			bheight = dc->lightlist[i].botheight >> LIGHTSCALESHIFT;
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
		if (height <= dc->yl)
		{
			dc->colormap = dc->lightlist[i].rcolormap;
			dc->fullbright = colormaps;
			if (encoremap)
			{
				dc->colormap += COLORMAP_REMAPOFFSET;
				dc->fullbright += COLORMAP_REMAPOFFSET;
			}
			if (solid && dc->yl < bheight)
				dc->yl = bheight;
			continue;
		}
		// Found a break in the column!
		dc->yh = height;

		if (dc->yh > realyh)
			dc->yh = realyh;
		(colfuncs[BASEDRAWFUNC])(dc);		// R_DrawColumn_8 for the appropriate architecture
		if (solid)
			dc->yl = bheight;
		else
			dc->yl = dc->yh + 1;

		dc->colormap = dc->lightlist[i].rcolormap;
		dc->fullbright = colormaps;
		if (encoremap)
		{
			dc->colormap += COLORMAP_REMAPOFFSET;
			dc->fullbright += COLORMAP_REMAPOFFSET;
		}
	}
	dc->yh = realyh;
	if (dc->yl <= realyh)
		(colfuncs[BASEDRAWFUNC])(dc);		// R_DrawWallColumn_8 for the appropriate architecture
}
