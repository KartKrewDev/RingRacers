// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C) 2023      by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw8_flat.c
/// \brief 8bpp span/column drawer functions for debugging (draws in flat colors only)
/// \note  no includes because this is included as part of r_draw.c

void R_DrawColumn_Flat_8 (drawcolumndata_t* dc)
{
	INT32 count;
	UINT8 color = dc->lightmap[dc->r8_flatcolor];
	register UINT8 *dest;

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
	dest = &topleft[dc->yl*vid.width + dc->x];

	count++;

	do
	{
		*dest = color;
		dest += vid.width;
	} while (--count);
}

void R_DrawSpan_Flat_8 (drawspandata_t* ds)
{
	UINT8 *dest = ylookup[ds->y] + columnofs[ds->x1];

	memset(dest, ds->colormap[ds->r8_flatcolor], (ds->x2 - ds->x1) + 1);
}

void R_DrawTiltedSpan_Flat_8 (drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);
	INT32 tiltlighting[MAXVIDWIDTH];

	UINT8 *dest = ylookup[ds->y];

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds->szp.x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(tiltlighting, ds->x1, ds->x2, FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	while (ds->x1 <= ds->x2)
	{
		dest[ds->x1] = ds->planezlight[tiltlighting[ds->x1]][ds->r8_flatcolor];
		ds->x1++;
	}
}
