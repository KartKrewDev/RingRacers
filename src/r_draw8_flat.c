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

void R_DrawColumn_Flat_8 (void)
{
	INT32 count;
	UINT8 color = dc_lightmap[r8_flatcolor];
	register UINT8 *dest;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?

	//dest = ylookup[dc_yl] + columnofs[dc_x];
	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;

	do
	{
		*dest = color;
		dest += vid.width;
	} while (--count);
}

void R_DrawSpan_Flat_8 (void)
{
	UINT8 *dest = ylookup[ds_y] + columnofs[ds_x1];

	memset(dest, ds_colormap[r8_flatcolor], (ds_x2 - ds_x1) + 1);
}

void R_DrawTiltedSpan_Flat_8 (void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz = ds_szp->z + ds_szp->y*(centery-ds_y) + ds_szp->x*(ds_x1-centerx);

	UINT8 *dest = ylookup[ds_y];

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds_szp->x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	while (ds_x1 <= ds_x2)
	{
		dest[ds_x1] = planezlight[tiltlighting[ds_x1]][r8_flatcolor];
		ds_x1++;
	}
}
