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
/// \file  r_draw_span.cpp
/// \brief span drawer functions
/// \note  no includes because this is included as part of r_draw.cpp

using namespace libdivide;

// ==========================================================================
// SPANS
// ==========================================================================

#define SPANSIZE 16
#define INVSPAN 0.0625f

// <Callum> 4194303 = (2048x2048)-1 (2048x2048 is maximum flat size)
#define MAXFLATBYTES 4194303

#define PLANELIGHTFLOAT (BASEVIDWIDTH * BASEVIDWIDTH / vid.width / ds->zeroheight / 21.0f * FIXED_TO_FLOAT(fovtan[viewssnum]))

enum DrawSpanType
{
	DS_BASIC			= 0x0000,
	DS_COLORMAP			= 0x0001,
	DS_TRANSMAP			= 0x0002,
	DS_BRIGHTMAP		= 0x0004,
	DS_HOLES			= 0x0008,
	DS_RIPPLE			= 0x0010,
	DS_SPRITE			= 0x0020,
};

template<DrawSpanType Type>
static constexpr UINT8 R_GetSpanTranslated(drawspandata_t* ds, UINT8 col)
{
	if constexpr (Type & DrawSpanType::DS_COLORMAP)
	{
		return ds->translation[col];
	}
	else
	{
		return col;
	}
}

template<DrawSpanType Type>
static constexpr UINT8 R_GetSpanBrightmapped(drawspandata_t* ds, UINT8 *colormap, UINT32 bit, UINT8 col)
{
	col = R_GetSpanTranslated<Type>(ds, col);

	if constexpr (Type & DrawSpanType::DS_BRIGHTMAP)
	{
		UINT8 brightCol = 31;

		if constexpr (Type & DrawSpanType::DS_SPRITE)
		{
			UINT16 *spriteSource = reinterpret_cast<UINT16 *>(ds->brightmap);
			UINT16 spriteCol = spriteSource[bit];

			if (spriteCol & 0xFF00)
			{
				brightCol = (spriteCol & 0xFF);
			}
		}
		else
		{
			brightCol = ds->brightmap[bit];
		}

		if (brightCol == BRIGHTPIXEL)
		{
			return ds->fullbright[col];
		}
	}

	return colormap[col];
}

template<DrawSpanType Type>
static constexpr UINT8 R_GetSpanTranslucent(drawspandata_t* ds, UINT8 *dsrc, UINT8 *colormap, UINT32 bit, UINT8 col)
{
	col = R_GetSpanBrightmapped<Type>(ds, colormap, bit, col);

	if constexpr (Type & DrawSpanType::DS_TRANSMAP)
	{
		return *(ds->transmap + (col << 8) + (*dsrc));
	}
	else
	{
		return col;
	}
}

template<DrawSpanType Type>
static constexpr UINT8 R_DrawSpanPixel(drawspandata_t* ds, UINT8 *dsrc, UINT8 *colormap, UINT32 bit)
{
	UINT8 col = 0;

	if constexpr (Type & DrawSpanType::DS_SPRITE)
	{
		UINT16 *spriteSource = reinterpret_cast<UINT16 *>(ds->source);
		UINT16 spriteCol = spriteSource[bit];

		if (spriteCol & 0xFF00)
		{
			col = (spriteCol & 0xFF);
		}
		else
		{
			return *dsrc;
		}
	}
	else
	{
		col = ds->source[bit];
	}

	if constexpr (Type & DrawSpanType::DS_HOLES)
	{
		if (col == TRANSPARENTPIXEL)
		{
			return *dsrc;
		}
	}

	return R_GetSpanTranslucent<Type>(ds, dsrc, colormap, bit, col);
}

/**	\brief The R_DrawSpan_8 function
	Draws the actual span.
*/
template<DrawSpanType Type>
static void R_DrawSpanTemplate(drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	UINT32 bit;

	UINT8 *dest;
	UINT8 *dsrc;

	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);
	size_t i;

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	if constexpr (Type & DS_RIPPLE)
	{
		yposition += ds->waterofs;
	}

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= ds->nflatshiftup; yposition <<= ds->nflatshiftup;
	xstep <<= ds->nflatshiftup; ystep <<= ds->nflatshiftup;

	dest = ylookup[ds->y] + columnofs[ds->x1];
	if constexpr (Type & DS_RIPPLE)
	{
		dsrc = screens[1] + (ds->y + ds->bgofs) * vid.width + ds->x1;
	}
	else
	{
		dsrc = dest;
	}

	if (dest+8 > deststop)
	{
		return;
	}

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!

		for (i = 0; i < 8; i++)
		{
			bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);

			dest[i] = R_DrawSpanPixel<Type>(ds, &dsrc[i], ds->colormap, bit);

			xposition += xstep;
			yposition += ystep;
		}

		dest += 8;
		dsrc += 8;

		count -= 8;
	}

	while (count-- && dest <= deststop)
	{
		bit = (((UINT32)yposition >> ds->nflatyshift) & ds->nflatmask) | ((UINT32)xposition >> ds->nflatxshift);

		*dest = R_DrawSpanPixel<Type>(ds, dsrc, ds->colormap, bit);

		dest++;
		dsrc++;

		xposition += xstep;
		yposition += ystep;
	}
}

// R_CalcTiltedLighting
// Exactly what it says on the tin. I wish I wasn't too lazy to explain things properly.
static void R_CalcTiltedLighting(INT32 *lightbuffer, INT32 x1, INT32 x2, fixed_t start, fixed_t end)
{
	// ZDoom uses a different lighting setup to us, and I couldn't figure out how to adapt their version
	// of this function. Here's my own.
	INT32 left = x1, right = x2;
	fixed_t step = (end-start)/(x2 - x1 + 1);
	INT32 i;

	// I wanna do some optimizing by checking for out-of-range segments on either side to fill in all at once,
	// but I'm too bad at coding to not crash the game trying to do that. I guess this is fast enough for now...

	for (i = left; i <= right; i++)
	{
		lightbuffer[i] = (start += step) >> FRACBITS;

		if (lightbuffer[i] < 0)
		{
			lightbuffer[i] = 0;
		}
		else if (lightbuffer[i] >= MAXLIGHTSCALE)
		{
			lightbuffer[i] = MAXLIGHTSCALE-1;
		}
	}
}

template<DrawSpanType Type>
static void R_DrawTiltedSpanTemplate(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	UINT32 bit;
	INT32 tiltlighting[MAXVIDWIDTH];

	INT32 x1 = ds->x1;
	const INT32 nflatxshift = ds->nflatxshift;
	const INT32 nflatyshift = ds->nflatyshift;
	const INT32 nflatmask = ds->nflatmask;

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	if constexpr (!(Type & DS_SPRITE))
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

	colormap = ds->colormap;

	dest = ylookup[ds->y] + columnofs[ds->x1];
	if constexpr (Type & DS_RIPPLE)
	{
		dsrc = screens[1] + (ds->y + ds->bgofs) * vid.width + ds->x1;
	}
	else
	{
		dsrc = dest;
	}

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		bit = ((v >> nflatyshift) & nflatmask) | (u >> nflatxshift);
		if constexpr (!(Type & DS_SPRITE))
		{
			colormap = planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
		}
		*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, bit);
		dest++;
		ds->x1++;
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

		x1 = ds->x1;

		for (i = 0; i < SPANSIZE; i++)
		{
			bit = (((v + stepv * i) >> nflatyshift) & nflatmask) | ((u + stepu * i) >> nflatxshift);

			if constexpr (!(Type & DS_SPRITE))
			{
				colormap = ds->planezlight[tiltlighting[x1 + i]] + (ds->colormap - colormaps);
			}

			dest[i] = R_DrawSpanPixel<Type>(ds, &dsrc[i], colormap, bit);
		}

		ds->x1 += SPANSIZE;
		dest += SPANSIZE;
		dsrc += SPANSIZE;
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
			bit = ((v >> nflatyshift) & nflatmask) | (u >> nflatxshift);
			if constexpr (!(Type & DS_SPRITE))
			{
				colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
			}
			*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, bit);
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
				if constexpr (!(Type & DS_SPRITE))
				{
					colormap = ds->planezlight[tiltlighting[ds->x1]] + (ds->colormap - colormaps);
				}
				*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, bit);
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

/**	\brief The R_DrawSpan_NPO2 function
	Draws the actual span.
*/
template<DrawSpanType Type>
static void R_DrawNPO2SpanTemplate(drawspandata_t* ds)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;
	fixed_t x, y;
	fixed_t fixedwidth, fixedheight;

	UINT8 *dest;
	UINT8 *dsrc;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds->x2 - ds->x1 + 1);

	xposition = ds->xfrac; yposition = ds->yfrac;
	xstep = ds->xstep; ystep = ds->ystep;

	if constexpr (Type & DS_RIPPLE)
	{
		yposition += ds->waterofs;
	}

	dest = ylookup[ds->y] + columnofs[ds->x1];

	if constexpr (Type & DS_RIPPLE)
	{
		dsrc = screens[1] + (ds->y + ds->bgofs) * vid.width + ds->x1;
	}
	else
	{
		dsrc = dest;
	}

	if (dest+8 > deststop)
		return;

	fixedwidth = ds->flatwidth << FRACBITS;
	fixedheight = ds->flatheight << FRACBITS;

	// Fix xposition and yposition if they are out of bounds.
	if (xposition < 0)
		xposition = fixedwidth - ((UINT32)(fixedwidth - xposition) % fixedwidth);
	else if (xposition >= fixedwidth)
		xposition %= fixedwidth;
	if (yposition < 0)
		yposition = fixedheight - ((UINT32)(fixedheight - yposition) % fixedheight);
	else if (yposition >= fixedheight)
		yposition %= fixedheight;

	while (count-- && dest <= deststop)
	{
		// The loops here keep the texture coordinates within the texture.
		// They will rarely iterate multiple times, and are cheaper than a modulo operation,
		// even if using libdivide.
		if (xstep < 0) // These if statements are hopefully hoisted by the compiler to above this loop
			while (xposition < 0)
				xposition += fixedwidth;
		else
			while (xposition >= fixedwidth)
				xposition -= fixedwidth;
		if (ystep < 0)
			while (yposition < 0)
				yposition += fixedheight;
		else
			while (yposition >= fixedheight)
				yposition -= fixedheight;

		x = (xposition >> FRACBITS);
		y = (yposition >> FRACBITS);

		*dest = R_DrawSpanPixel<Type>(ds, dsrc, ds->colormap, ((y * ds->flatwidth) + x));
		dest++;
		dsrc++;

		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedSpan_NPO2_8 function
	Draw slopes! Holy sheit!
*/
template<DrawSpanType Type>
static void R_DrawTiltedNPO2SpanTemplate(drawspandata_t* ds)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;
	INT32 tiltlighting[MAXVIDWIDTH];

	struct libdivide_u32_t x_divider = libdivide_u32_gen(ds->flatwidth);
	struct libdivide_u32_t y_divider = libdivide_u32_gen(ds->flatheight);

	iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);

	// Lighting is simple. It's just linear interpolation from start to end
	if constexpr (!(Type & DS_SPRITE))
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

	colormap = ds->colormap;

	dest = ylookup[ds->y] + columnofs[ds->x1];

	if constexpr (Type & DS_RIPPLE)
	{
		dsrc = screens[1] + (ds->y + ds->bgofs) * vid.width + ds->x1;
	}
	else
	{
		dsrc = dest;
	}

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		if constexpr (!(Type & DS_SPRITE))
		{
			colormap = ds->planezlight[tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
		}

		// Lactozilla: Non-powers-of-two
		{
			fixed_t x = (((fixed_t)u) >> FRACBITS);
			fixed_t y = (((fixed_t)v) >> FRACBITS);

			// Carefully align all of my Friends.
			if (x < 0)
				x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds_flatwidth;
			else
				x -= libdivide_u32_do((UINT32)x, &x_divider) * ds_flatwidth;
			if (y < 0)
				y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds_flatheight;
			else
				y -= libdivide_u32_do((UINT32)y, &y_divider) * ds_flatheight;

			*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, ((y * ds->flatwidth) + x));
		}
		dest++;
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
			if constexpr (!(Type & DS_SPRITE))
			{
				colormap = ds->planezlight[tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			}

			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, ((y * ds->flatwidth) + x));
			}
			dest++;
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

			if constexpr (!(Type & DS_SPRITE))
			{
				colormap = ds->planezlight[tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
			}

			// Lactozilla: Non-powers-of-two
			{
				fixed_t x = (((fixed_t)u) >> FRACBITS);
				fixed_t y = (((fixed_t)v) >> FRACBITS);

				// Carefully align all of my Friends.
				if (x < 0)
					x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
				else
					x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
				if (y < 0)
					y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
				else
					y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

				*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, ((y * ds->flatwidth) + x));
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
				if constexpr (!(Type & DS_SPRITE))
				{
					colormap = ds->planezlight[tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
				}

				// Lactozilla: Non-powers-of-two
				{
					fixed_t x = (((fixed_t)u) >> FRACBITS);
					fixed_t y = (((fixed_t)v) >> FRACBITS);

					// Carefully align all of my Friends.
					if (x < 0)
						x += (libdivide_u32_do((UINT32)(-x-1), &x_divider) + 1) * ds->flatwidth;
					else
						x -= libdivide_u32_do((UINT32)x, &x_divider) * ds->flatwidth;
					if (y < 0)
						y += (libdivide_u32_do((UINT32)(-y-1), &y_divider) + 1) * ds->flatheight;
					else
						y -= libdivide_u32_do((UINT32)y, &y_divider) * ds->flatheight;

					*dest = R_DrawSpanPixel<Type>(ds, dsrc, colormap, ((y * ds->flatwidth) + x));
				}
				dest++;
				dsrc++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

#define DEFINE_SPAN_FUNC(name, flags, template) \
	void name(drawspandata_t* ds) \
	{ \
		ZoneScoped; \
		constexpr DrawSpanType opt = static_cast<DrawSpanType>(flags); \
		template<opt>(ds); \
	}

#define DEFINE_SPAN_COMBO(name, flags) \
	DEFINE_SPAN_FUNC(name, flags, R_DrawSpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _Tilted, flags, R_DrawTiltedSpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _NPO2, flags, R_DrawNPO2SpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _Tilted_NPO2, flags, R_DrawTiltedNPO2SpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _Brightmap, flags|DS_BRIGHTMAP, R_DrawSpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _Tilted_Brightmap, flags|DS_BRIGHTMAP, R_DrawTiltedSpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _Brightmap_NPO2, flags|DS_BRIGHTMAP, R_DrawNPO2SpanTemplate) \
	DEFINE_SPAN_FUNC(name ## _Tilted_Brightmap_NPO2, flags|DS_BRIGHTMAP, R_DrawTiltedNPO2SpanTemplate)

DEFINE_SPAN_COMBO(R_DrawSpan, DS_BASIC)
DEFINE_SPAN_COMBO(R_DrawTranslucentSpan, DS_TRANSMAP)
DEFINE_SPAN_COMBO(R_DrawSplat, DS_HOLES)
DEFINE_SPAN_COMBO(R_DrawTranslucentSplat, DS_TRANSMAP|DS_HOLES)
DEFINE_SPAN_COMBO(R_DrawFloorSprite, DS_COLORMAP|DS_SPRITE)
DEFINE_SPAN_COMBO(R_DrawTranslucentFloorSprite, DS_COLORMAP|DS_TRANSMAP|DS_SPRITE)
DEFINE_SPAN_COMBO(R_DrawTranslucentWaterSpan, DS_TRANSMAP|DS_RIPPLE)

void R_DrawFogSpan(drawspandata_t* ds)
{
	ZoneScoped;

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

void R_DrawFogSpan_Tilted(drawspandata_t* ds)
{
	ZoneScoped;

	// x1, x2 = ds_x1, ds_x2
	int width = ds->x2 - ds->x1;
	double iz = ds->szp.z + ds->szp.y*(centery-ds->y) + ds->szp.x*(ds->x1-centerx);
	INT32 tiltlighting[MAXVIDWIDTH];

	UINT8 *dest = ylookup[ds->y] + columnofs[ds->x1];

	// Lighting is simple. It's just linear interpolation from start to end
	{
		float planelightfloat = PLANELIGHTFLOAT;
		float lightstart, lightend;

		lightend = (iz + ds->szp.x*width) * planelightfloat;
		lightstart = iz * planelightfloat;

		R_CalcTiltedLighting(tiltlighting, ds->x1, ds->x2, FLOAT_TO_FIXED(lightstart), FLOAT_TO_FIXED(lightend));
		//CONS_Printf("tilted lighting %f to %f (foc %f)\n", lightstart, lightend, focallengthf);
	}

	do
	{
		UINT8 *colormap = ds->planezlight[tiltlighting[ds->x1++]] + (ds->colormap - colormaps);
		*dest = colormap[*dest];
		dest++;
	}
	while (--width >= 0);
}

void R_DrawSpan_Flat(drawspandata_t* ds)
{
	ZoneScoped;

	UINT8 *dest = ylookup[ds->y] + columnofs[ds->x1];
	memset(dest, ds->colormap[ds->r8_flatcolor], (ds->x2 - ds->x1) + 1);
}

void R_DrawTiltedSpan_Flat(drawspandata_t* ds)
{
	ZoneScoped;

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
