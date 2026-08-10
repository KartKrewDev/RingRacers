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
/// \file hw_cache.c
/// \brief load and convert graphics to the hardware format

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_main.h"
#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_batching.h"

#include "../doomstat.h"    //gamemode
#include "../i_video.h"     //rendermode
#include "../r_data.h"
#include "../r_textures.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../r_draw.h"
#include "../r_patch.h"
#include "../r_picformats.h"
#include "../p_setup.h"

int32_t patchformat = GL_TEXFMT_AP_88; // use alpha for holes
int32_t textureformat = GL_TEXFMT_P_8; // use chromakey for hole

static int32_t format2bpp(GLTextureFormat_t format)
{
	if (format == GL_TEXFMT_RGBA)
		return 4;
	else if (format == GL_TEXFMT_ALPHA_INTENSITY_88 || format == GL_TEXFMT_AP_88)
		return 2;
	else
		return 1;
}

// This code was originally placed directly in HWR_DrawPatchInCache.
// It is now split from it for my sanity! (and the sanity of others)
// -- Monster Iestyn (13/02/19)
static void HWR_DrawColumnInCache(const column_t *patchcol, uint8_t *block, GLMipmap_t *mipmap,
								int32_t pblockheight, int32_t blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, int32_t patchheight,
								int32_t bpp)
{
	fixed_t yfrac, position, count;
	uint8_t *dest;
	const uint8_t *source;
	int32_t topdelta, prevdelta = -1;
	int32_t originy = 0;

	// for writing a pixel to dest
	RGBA_t colortemp;
	uint8_t alpha;
	uint8_t texel;
	uint16_t texelu16;

	(void)patchheight; // This parameter is unused

	if (originPatch) // originPatch can be NULL here, unlike in the software version
		originy = originPatch->originy;

	while (patchcol->topdelta != 0xff)
	{
		topdelta = patchcol->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		source = (const uint8_t *)patchcol + 3;
		count  = ((patchcol->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
		position = originy + topdelta;

		yfrac = 0;
		//yfracstep = (patchcol->length << FRACBITS) / count;
		if (position < 0)
		{
			yfrac = -position<<FRACBITS;
			count += (((position * scale_y) + (FRACUNIT/2)) >> FRACBITS);
			position = 0;
		}

		position = ((position * scale_y) + (FRACUNIT/2)) >> FRACBITS;

		if (position < 0)
			position = 0;

		if (position + count >= pblockheight)
			count = pblockheight - position;

		dest = block + (position*blockmodulo);
		while (count > 0)
		{
			count--;

			texel = source[yfrac>>FRACBITS];
			alpha = 0xFF;
			// Make pixel transparent if chroma keyed
			if ((mipmap->flags & TF_CHROMAKEYED) && (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX))
				alpha = 0x00;

			//Hurdler: 25/04/2000: now support colormap in hardware mode
			if (mipmap->colormap)
				texel = mipmap->colormap->data[texel];

			// hope compiler will get this switch out of the loops (dreams...)
			// gcc do it ! but vcc not ! (why don't use cygwin gcc for win32 ?)
			// Alam: SRB2 uses Mingw, HUGS
			switch (bpp)
			{
				case 2 : // uhhhhhhhh..........
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 texel = ASTBlendPaletteIndexes(*(dest+1), texel, originPatch->style, originPatch->alpha);
						 texelu16 = (uint16_t)((alpha<<8) | texel);
						 memcpy(dest, &texelu16, sizeof(uint16_t));
						 break;
				case 3 : colortemp = V_GetColor(texel);
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(uint32_t *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(uint8_t));
						 break;
				case 4 : colortemp = V_GetColor(texel);
						 colortemp.s.alpha = alpha;
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(uint32_t *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t));
						 break;
				// default is 1
				default:
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 *dest = ASTBlendPaletteIndexes(*dest, texel, originPatch->style, originPatch->alpha);
						 else
							 *dest = texel;
						 break;
			}

			dest += blockmodulo;
			yfrac += yfracstep;
		}
		patchcol = (const column_t *)((const uint8_t *)patchcol + patchcol->length + 4);
	}
}

static void HWR_DrawFlippedColumnInCache(const column_t *patchcol, uint8_t *block, GLMipmap_t *mipmap,
								int32_t pblockheight, int32_t blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, int32_t patchheight,
								int32_t bpp)
{
	fixed_t yfrac, position, count;
	uint8_t *dest;
	const uint8_t *source;
	int32_t topdelta, prevdelta = -1;
	int32_t originy = 0;

	// for writing a pixel to dest
	RGBA_t colortemp;
	uint8_t alpha;
	uint8_t texel;
	uint16_t texelu16;

	if (originPatch) // originPatch can be NULL here, unlike in the software version
		originy = originPatch->originy;

	while (patchcol->topdelta != 0xff)
	{
		topdelta = patchcol->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = patchheight-patchcol->length-topdelta;
		source = (const uint8_t *)patchcol + 3;
		count  = ((patchcol->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
		position = originy + topdelta;

		yfrac = (patchcol->length-1) << FRACBITS;

		if (position < 0)
		{
			yfrac += position<<FRACBITS;
			count += (((position * scale_y) + (FRACUNIT/2)) >> FRACBITS);
			position = 0;
		}

		position = ((position * scale_y) + (FRACUNIT/2)) >> FRACBITS;

		if (position < 0)
			position = 0;

		if (position + count >= pblockheight)
			count = pblockheight - position;

		dest = block + (position*blockmodulo);
		while (count > 0)
		{
			count--;

			texel = source[yfrac>>FRACBITS];
			alpha = 0xFF;
			// Make pixel transparent if chroma keyed
			if ((mipmap->flags & TF_CHROMAKEYED) && (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX))
				alpha = 0x00;

			//Hurdler: 25/04/2000: now support colormap in hardware mode
			if (mipmap->colormap)
				texel = mipmap->colormap->data[texel];

			// hope compiler will get this switch out of the loops (dreams...)
			// gcc do it ! but vcc not ! (why don't use cygwin gcc for win32 ?)
			// Alam: SRB2 uses Mingw, HUGS
			switch (bpp)
			{
				case 2 : // uhhhhhhhh..........
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 texel = ASTBlendPaletteIndexes(*(dest+1), texel, originPatch->style, originPatch->alpha);
						 texelu16 = (uint16_t)((alpha<<8) | texel);
						 memcpy(dest, &texelu16, sizeof(uint16_t));
						 break;
				case 3 : colortemp = V_GetColor(texel);
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(uint32_t *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(uint8_t));
						 break;
				case 4 : colortemp = V_GetColor(texel);
						 colortemp.s.alpha = alpha;
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(uint32_t *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t));
						 break;
				// default is 1
				default:
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 *dest = ASTBlendPaletteIndexes(*dest, texel, originPatch->style, originPatch->alpha);
						 else
							 *dest = texel;
						 break;
			}

			dest += blockmodulo;
			yfrac -= yfracstep;
		}
		patchcol = (const column_t *)((const uint8_t *)patchcol + patchcol->length + 4);
	}
}


// Simplified patch caching function
// for use by sprites and other patches that are not part of a wall texture
// no alpha or flipping should be present since we do not want non-texture graphics to have them
// no offsets are used either
// -- Monster Iestyn (13/02/19)
static void HWR_DrawPatchInCache(GLMipmap_t *mipmap,
	int32_t pblockwidth, int32_t pblockheight,
	int32_t pwidth, int32_t pheight,
	const patch_t *realpatch)
{
	int32_t ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfracstep, scale_y;
	const column_t *patchcol;
	uint8_t *block = (uint8_t *)mipmap->data;
	int32_t bpp;
	int32_t blockmodulo;

	if (pwidth <= 0 || pheight <= 0)
		return;

	ncols = pwidth;

	// source advance
	xfrac = 0;
	xfracstep = FRACUNIT;
	yfracstep = FRACUNIT;
	scale_y   = FRACUNIT;

	bpp = format2bpp(mipmap->format);

	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawPatchInCache: no drawer defined for this bpp (%d)\n",bpp);

	// NOTE: should this actually be pblockwidth*bpp?
	blockmodulo = pblockwidth*bpp;

	// Draw each column to the block cache
	for (; ncols--; block += bpp, xfrac += xfracstep)
	{
		patchcol = (const column_t *)((const uint8_t *)realpatch->columns + (realpatch->columnofs[xfrac>>FRACBITS]));

		HWR_DrawColumnInCache(patchcol, block, mipmap,
								pblockheight, blockmodulo,
								yfracstep, scale_y,
								NULL, pheight, // not that pheight is going to get used anyway...
								bpp);
	}
}

// This function we use for caching patches that belong to textures
static void HWR_DrawTexturePatchInCache(GLMipmap_t *mipmap,
	int32_t pblockwidth, int32_t pblockheight,
	texture_t *texture, texpatch_t *patch,
	const softwarepatch_t *realpatch)
{
	int32_t x, x1, x2;
	int32_t col, ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfracstep, scale_y;
	const column_t *patchcol;
	uint8_t *block = (uint8_t *)mipmap->data;
	int32_t bpp;
	int32_t blockmodulo;
	int32_t width, height;
	// Column drawing function pointer.
	static void (*ColumnDrawerPointer)(const column_t *patchcol, uint8_t *block, GLMipmap_t *mipmap,
								int32_t pblockheight, int32_t blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, int32_t patchheight,
								int32_t bpp);

	if (texture->width <= 0 || texture->height <= 0)
		return;

	ColumnDrawerPointer = (patch->flip & 2) ? HWR_DrawFlippedColumnInCache : HWR_DrawColumnInCache;

	x1 = patch->originx;
	width = LSBF_SHORT(realpatch->width);
	height = LSBF_SHORT(realpatch->height);
	x2 = x1 + width;

	if (x1 > texture->width || x2 < 0)
		return; // patch not located within texture's x bounds, ignore

	if (patch->originy > texture->height || (patch->originy + height) < 0)
		return; // patch not located within texture's y bounds, ignore

	// patch is actually inside the texture!
	// now check if texture is partly off-screen and adjust accordingly

	// left edge
	if (x1 < 0)
		x = 0;
	else
		x = x1;

	// right edge
	if (x2 > texture->width)
		x2 = texture->width;


	col = x * pblockwidth / texture->width;
	ncols = ((x2 - x) * pblockwidth) / texture->width;

/*
	CONS_Debug(DBG_RENDER, "patch %dx%d texture %dx%d block %dx%d\n",
															width, height,
															texture->width,          texture->height,
															pblockwidth,             pblockheight);
	CONS_Debug(DBG_RENDER, "      col %d ncols %d x %d\n", col, ncols, x);
*/

	// source advance
	xfrac = 0;
	if (x1 < 0)
		xfrac = -x1<<FRACBITS;

	xfracstep = (texture->width << FRACBITS) / pblockwidth;
	yfracstep = (texture->height<< FRACBITS) / pblockheight;
	scale_y   = (pblockheight  << FRACBITS) / texture->height;

	bpp = format2bpp(mipmap->format);

	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawTexturePatchInCache: no drawer defined for this bpp (%d)\n",bpp);

	// NOTE: should this actually be pblockwidth*bpp?
	blockmodulo = pblockwidth*bpp;

	// Draw each column to the block cache
	for (block += col*bpp; ncols--; block += bpp, xfrac += xfracstep)
	{
		if (patch->flip & 1)
			patchcol = (const column_t *)((const uint8_t *)realpatch + LSBF_LONG(realpatch->columnofs[(width-1)-(xfrac>>FRACBITS)]));
		else
			patchcol = (const column_t *)((const uint8_t *)realpatch + LSBF_LONG(realpatch->columnofs[xfrac>>FRACBITS]));

		ColumnDrawerPointer(patchcol, block, mipmap,
								pblockheight, blockmodulo,
								yfracstep, scale_y,
								patch, height,
								bpp);
	}
}

static uint8_t *MakeBlock(GLMipmap_t *grMipmap)
{
	uint8_t *block;
	int32_t bpp, i;
	uint16_t bu16 = ((0x00 <<8) | HWR_PATCHES_CHROMAKEY_COLORINDEX);
	int32_t blocksize = (grMipmap->width * grMipmap->height);

	bpp =  format2bpp(grMipmap->format);
	block = (uint8_t *)Z_Malloc(blocksize*bpp, PU_HWRCACHE, &(grMipmap->data));

	switch (bpp)
	{
		case 1: memset(block, HWR_PATCHES_CHROMAKEY_COLORINDEX, blocksize); break;
		case 2:
				// fill background with chromakey, alpha = 0
				for (i = 0; i < blocksize; i++)
					memcpy(block+i*sizeof(uint16_t), &bu16, sizeof(uint16_t));

				break;
		case 4: memset(block, 0x00, blocksize*sizeof(uint32_t)); break;
	}

	return block;
}

//
// Create a composite texture from patches, adapt the texture size to a power of 2
// height and width for the hardware texture cache.
//
static void HWR_GenerateTexture(GLMapTexture_t *grtex, int32_t texnum, dboolean noencoremap)
{
	uint8_t *block;
	texture_t *texture;
	texpatch_t *patch;
	softwarepatch_t *realpatch;
	uint8_t *pdata;
	int32_t blockwidth, blockheight, blocksize;

	uint8_t *colormap = colormaps;

	int32_t i;
	dboolean skyspecial = false; //poor hack for Legacy large skies..

	texture = textures[texnum];

	// hack the Legacy skies..
	if (texture->name[0] == 'S' &&
	    texture->name[1] == 'K' &&
	    texture->name[2] == 'Y' &&
	    (texture->name[4] == 0 ||
	     texture->name[5] == 0)
	   )
	{
		skyspecial = true;
		grtex->mipmap.flags = TF_WRAPXY; // don't use the chromakey for sky
	}
	else
		grtex->mipmap.flags = TF_CHROMAKEYED | TF_WRAPXY;

	grtex->mipmap.width = (uint16_t)texture->width;
	grtex->mipmap.height = (uint16_t)texture->height;
	grtex->mipmap.format = (GLTextureFormat_t)textureformat;

	if (!noencoremap && encoremap)
		colormap += COLORMAP_REMAPOFFSET;

	grtex->mipmap.colormap = (GLColormap_t *)Z_Calloc(sizeof(*grtex->mipmap.colormap), PU_HWRPATCHCOLMIPMAP, NULL);
	grtex->mipmap.colormap->source = colormap;
	M_Memcpy(grtex->mipmap.colormap->data, colormap, 256 * sizeof(uint8_t));

	blockwidth = texture->width;
	blockheight = texture->height;
	blocksize = (blockwidth * blockheight);
	block = MakeBlock(&grtex->mipmap);

	if (skyspecial) //Hurdler: not efficient, but better than holes in the sky (and it's done only at level loading)
	{
		int32_t j;
		RGBA_t col;

		col = V_GetColor(HWR_PATCHES_CHROMAKEY_COLORINDEX);
		for (j = 0; j < blockheight; j++)
		{
			for (i = 0; i < blockwidth; i++)
			{
				block[4*(j*blockwidth+i)+0] = col.s.red;
				block[4*(j*blockwidth+i)+1] = col.s.green;
				block[4*(j*blockwidth+i)+2] = col.s.blue;
				block[4*(j*blockwidth+i)+3] = 0xff;
			}
		}
	}

	// Composite the columns together.
	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		dboolean dealloc = true;
		size_t lumplength = W_LumpLengthPwad(patch->wad, patch->lump);
		pdata = (uint8_t*)W_CacheLumpNumPwad(patch->wad, patch->lump, PU_CACHE);
		realpatch = (softwarepatch_t *)pdata;

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG((uint8_t *)realpatch, lumplength))
			realpatch = (softwarepatch_t *)Picture_PNGConvert(pdata, PICFMT_DOOMPATCH, NULL, NULL, NULL, NULL, lumplength, NULL, (pictureflags_t)0);
		else
#endif
#ifdef WALLFLATS
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = (softwarepatch_t *)Picture_Convert(PICFMT_FLAT, pdata, PICFMT_DOOMPATCH, 0, NULL, texture->width, texture->height, 0, 0, (pictureflags_t)0);
		else
#endif
		{
			(void)lumplength;
			dealloc = false;
		}

		HWR_DrawTexturePatchInCache(&grtex->mipmap, blockwidth, blockheight, texture, patch, realpatch);

		if (dealloc)
			Z_Unlock(realpatch);
	}
	//Hurdler: not efficient at all but I don't remember exactly how HWR_DrawPatchInCache works :(
	if (format2bpp(grtex->mipmap.format)==4)
	{
		for (i = 3; i < blocksize*4; i += 4) // blocksize*4 because blocksize doesn't include the bpp
		{
			if (block[i] == 0)
			{
				grtex->mipmap.flags |= TF_TRANSPARENT;
				break;
			}
		}
	}

	grtex->scaleX = 1.0f/(texture->width*FRACUNIT);
	grtex->scaleY = 1.0f/(texture->height*FRACUNIT);
}

// patch may be NULL if grMipmap has been initialised already and makebitmap is false
void HWR_MakePatch (const patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, dboolean makebitmap)
{
	if (grMipmap->width == 0)
	{
		grMipmap->width = grMipmap->height = 1;
		while (grMipmap->width < patch->width) grMipmap->width <<= 1;
		while (grMipmap->height < patch->height) grMipmap->height <<= 1;

		// no wrap around, no chroma key
		grMipmap->flags = 0;

		// setup the texture info
		grMipmap->format = (GLTextureFormat_t)patchformat;

		grPatch->max_s = (float)patch->width / (float)grMipmap->width;
		grPatch->max_t = (float)patch->height / (float)grMipmap->height;
	}

	Z_Free(grMipmap->data);
	grMipmap->data = NULL;

	if (makebitmap)
	{
		MakeBlock(grMipmap);
		HWR_DrawPatchInCache(grMipmap,
			grMipmap->width, grMipmap->height,
			patch->width, patch->height,
			patch);
	}
}

// =================================================
//             CACHING HANDLING
// =================================================

static size_t gl_numtextures = 0; // Texture count
static GLMapTexture_t *gl_textures; // For all textures
static GLMapTexture_t *gl_flats; // For all (texture) flats, as normal flats don't need to be cached
dboolean gl_maptexturesloaded = false;

void HWR_FreeTextureData(patch_t *patch)
{
	GLPatch_t *grPatch;

	if (!patch || !patch->hardware)
		return;

	grPatch = (GLPatch_t*)patch->hardware;

	if (vid.glstate == VID_GL_LIBRARY_LOADED)
		HWD.pfnDeleteTexture(grPatch->mipmap);
	if (grPatch->mipmap->data)
		Z_Free(grPatch->mipmap->data);
}

void HWR_FreeTexture(patch_t *patch)
{
	if (!patch)
		return;

	if (patch->hardware)
	{
		GLPatch_t *grPatch = (GLPatch_t*)patch->hardware;

		HWR_FreeTextureColormaps(patch);

		if (grPatch->mipmap)
		{
			HWR_FreeTextureData(patch);
			Z_Free(grPatch->mipmap);
		}

		Z_Free(patch->hardware);
	}

	patch->hardware = NULL;
}

// Called by HWR_FreePatchCache.
void HWR_FreeTextureColormaps(patch_t *patch)
{
	GLPatch_t *pat;

	// The patch must be valid, obviously
	if (!patch)
		return;

	pat = (GLPatch_t*)patch->hardware;
	if (!pat)
		return;

	// The mipmap must be valid, obviously
	while (pat->mipmap)
	{
		// Confusing at first, but pat->mipmap->nextcolormap
		// at the beginning of the loop is the first colormap
		// from the linked list of colormaps.
		GLMipmap_t *next = NULL;

		// No mipmap in this patch, break out of the loop.
		if (!pat->mipmap)
			break;

		// No colormap mipmaps either.
		if (!pat->mipmap->nextcolormap)
			break;

		// Set the first colormap to the one that comes after it.
		next = pat->mipmap->nextcolormap;
		pat->mipmap->nextcolormap = next->nextcolormap;

		// Free image data from memory.
		if (next->data)
			Z_Free(next->data);
		if (next->colormap)
			Z_Free(next->colormap);
		next->data = NULL;
		next->colormap = NULL;
		HWD.pfnDeleteTexture(next);

		// Free the old colormap mipmap from memory.
		free(next);
	}
}

static dboolean FreeTextureCallback(void *mem)
{
	patch_t *patch = (patch_t *)mem;
	HWR_FreeTexture(patch);
	return false;
}

static dboolean FreeColormapsCallback(void *mem)
{
	patch_t *patch = (patch_t *)mem;
	HWR_FreeTextureColormaps(patch);
	return false;
}

static void HWR_FreePatchCache(dboolean freeall)
{
	dboolean (*callback)(void *mem) = FreeTextureCallback;

	if (!freeall)
		callback = FreeColormapsCallback;

	Z_IterateTags(PU_PATCH, PU_PATCH_ROTATED, callback);
	Z_IterateTags(PU_SPRITE, PU_HUDGFX, callback);
}

// free all textures after each level
void HWR_ClearAllTextures(void)
{
	HWD.pfnClearMipMapCache(); // free references to the textures
	HWR_FreePatchCache(true);
}

void HWR_FreeColormapCache(void)
{
	HWR_FreePatchCache(false);
}

void HWR_InitMapTextures(void)
{
	gl_textures = NULL;
	gl_flats = NULL;
	gl_maptexturesloaded = false;
}

static void FreeMapTexture(GLMapTexture_t *tex)
{
	HWD.pfnDeleteTexture(&tex->mipmap);
	if (tex->mipmap.data)
		Z_Free(tex->mipmap.data);
	tex->mipmap.data = NULL;
}

void HWR_FreeMapTextures(void)
{
	size_t i;

	for (i = 0; i < gl_numtextures; i++)
	{
		FreeMapTexture(&gl_textures[i]);
		FreeMapTexture(&gl_flats[i]);
	}

	// now the heap don't have any 'user' pointing to our
	// texturecache info, we can free it
	if (gl_textures)
		free(gl_textures);
	if (gl_flats)
		free(gl_flats);
	gl_textures = NULL;
	gl_flats = NULL;
	gl_numtextures = 0;
	gl_maptexturesloaded = false;
}

void HWR_LoadMapTextures(size_t pnumtextures)
{
	// we must free it since numtextures may have changed
	HWR_FreeMapTextures();

	gl_numtextures = pnumtextures;
	gl_textures = (GLMapTexture_t *)calloc(gl_numtextures, sizeof(*gl_textures));
	gl_flats = (GLMapTexture_t *)calloc(gl_numtextures, sizeof(*gl_flats));

	if ((gl_textures == NULL) || (gl_flats == NULL))
		I_Error("HWR_LoadMapTextures: ran out of memory for OpenGL textures");

	gl_maptexturesloaded = true;
}

void HWR_SetPalette(RGBA_t *palette)
{
	HWD.pfnSetPalette(palette);

	// hardware driver will flush there own cache if cache is non paletized
	// now flush data texture cache so 32 bit texture are recomputed
	if (patchformat == GL_TEXFMT_RGBA || textureformat == GL_TEXFMT_RGBA)
	{
		Z_FreeTag(PU_HWRCACHE);
		Z_FreeTag(PU_HWRCACHE_UNLOCKED);
	}
}

// --------------------------------------------------------------------------
// Make sure texture is downloaded and set it as the source
// --------------------------------------------------------------------------
GLMapTexture_t *HWR_GetTexture(int32_t tex, int32_t basetex)
{
	GLMapTexture_t *grtex;
#ifdef PARANOIA
	if ((unsigned)tex >= gl_numtextures)
		I_Error("HWR_GetTexture: tex >= numtextures\n");
#endif

	// Every texture in memory, stored in the
	// hardware renderer's bit depth format. Wow!
	grtex = &gl_textures[tex];

	// Generate texture if missing from the cache
	if (!grtex->mipmap.data && !grtex->mipmap.downloaded)
		HWR_GenerateTexture(grtex, tex, !R_TextureCanRemap(basetex));

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!grtex->mipmap.downloaded)
		HWD.pfnSetTexture(&grtex->mipmap);
	HWR_SetCurrentTexture(&grtex->mipmap);

	// The system-memory data can be purged now.
	Z_ChangeTag(grtex->mipmap.data, PU_HWRCACHE_UNLOCKED);

	if (R_GetTextureBrightmap(tex))
	{
		GLMapTexture_t *grtexbright;

		// Every texture in memory, stored in the
		// hardware renderer's bit depth format. Wow!
		grtexbright = &gl_textures[R_GetTextureBrightmap(tex)];

		// Generate texture if missing from the cache
		if (!grtexbright->mipmap.data && !grtexbright->mipmap.downloaded)
			HWR_GenerateTexture(grtexbright, R_GetTextureBrightmap(tex), true);

		grtexbright->mipmap.flags |= TF_BRIGHTMAP;

		// If hardware does not have the texture, then call pfnSetTexture to upload it
		if (!grtexbright->mipmap.downloaded)
			HWD.pfnSetTexture(&grtexbright->mipmap);
		HWR_SetCurrentTexture(&grtexbright->mipmap);

		// The system-memory data can be purged now.
		Z_ChangeTag(grtexbright->mipmap.data, PU_HWRCACHE_UNLOCKED);
	}

	return grtex;
}

static void HWR_CacheFlat(GLMipmap_t *grMipmap, lumpnum_t flatlumpnum)
{
	uint8_t *flat;
	size_t steppy;
	size_t size, pflatsize;


	// setup the texture info
	grMipmap->format = GL_TEXFMT_P_8;
	grMipmap->flags = TF_WRAPXY|TF_CHROMAKEYED;

	size = W_LumpLength(flatlumpnum);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			pflatsize = 2048;
			break;
		case 1048576: // 1024x1024 lump
			pflatsize = 1024;
			break;
		case 262144:// 512x512 lump
			pflatsize = 512;
			break;
		case 65536: // 256x256 lump
			pflatsize = 256;
			break;
		case 16384: // 128x128 lump
			pflatsize = 128;
			break;
		case 1024: // 32x32 lump
			pflatsize = 32;
			break;
		default: // 64x64 lump
			pflatsize = 64;
			break;
	}

	grMipmap->width  = (uint16_t)pflatsize;
	grMipmap->height = (uint16_t)pflatsize;

	// the flat raw data needn't be converted with palettized textures
	W_ReadLump(flatlumpnum, Z_Malloc(W_LumpLength(flatlumpnum),
		PU_HWRCACHE, &grMipmap->data));

	flat = (uint8_t*)grMipmap->data;
	for (steppy = 0; steppy < size; steppy++)
		if (flat[steppy] != HWR_PATCHES_CHROMAKEY_COLORINDEX)
			flat[steppy] = grMipmap->colormap->source[flat[steppy]];
}

static void HWR_CacheTextureAsFlat(GLMipmap_t *grMipmap, int32_t texturenum, dboolean noencoremap)
{
	uint8_t *flat;
	uint8_t *converted;
	size_t size;
	size_t i;
	uint8_t *colormap = colormaps;

	if (!noencoremap && encoremap)
	{
		colormap += COLORMAP_REMAPOFFSET;
	}

	// setup the texture info
	grMipmap->format = GL_TEXFMT_P_8;
	grMipmap->flags = TF_WRAPXY|TF_CHROMAKEYED;

	grMipmap->width  = (uint16_t)textures[texturenum]->width;
	grMipmap->height = (uint16_t)textures[texturenum]->height;
	size = (grMipmap->width * grMipmap->height);

	flat = (uint8_t *)Z_Malloc(size, PU_HWRCACHE, &grMipmap->data);
	converted = (uint8_t *)Picture_TextureToFlat(texturenum);
	for (i = 0; i < size; i++)
	{
		flat[i] = colormap[converted[i]];
	}
	Z_Free(converted);
}

// Download a Doom 'flat' to the hardware cache and make it ready for use
void HWR_GetRawFlat(lumpnum_t flatlumpnum, dboolean noencoremap)
{
	GLMipmap_t *grmip;
	patch_t *patch;

	uint8_t *colormap = colormaps;

	if (flatlumpnum == LUMPERROR)
		return;

	patch = HWR_GetCachedGLPatch(flatlumpnum);
	grmip = ((GLPatch_t *)Patch_AllocateHardwarePatch(patch))->mipmap;

	if (!grmip->colormap)
	{
		if (!noencoremap && encoremap)
			colormap += COLORMAP_REMAPOFFSET;

		grmip->colormap = (GLColormap_t *)Z_Calloc(sizeof(*grmip->colormap), PU_HWRPATCHCOLMIPMAP, NULL);
		grmip->colormap->source = colormap;
		M_Memcpy(grmip->colormap->data, colormap, 256 * sizeof(uint8_t));
	}

	if (!grmip->downloaded && !grmip->data)
	{
		HWR_CacheFlat(grmip, flatlumpnum);
	}

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!grmip->downloaded)
		HWD.pfnSetTexture(grmip);
	HWR_SetCurrentTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}

void HWR_GetLevelFlat(levelflat_t *levelflat, dboolean noencoremap)
{
	// Who knows?
	if (levelflat == NULL)
		return;

	if (levelflat->type == LEVELFLAT_FLAT)
		HWR_GetRawFlat(levelflat->u.flat.lumpnum, noencoremap);
	else if (levelflat->type == LEVELFLAT_TEXTURE)
	{
		GLMapTexture_t *grtex;
		int32_t texturenum = levelflat->u.texture.num;
#ifdef PARANOIA
		if ((unsigned)texturenum >= gl_numtextures)
			I_Error("HWR_GetLevelFlat: texturenum >= numtextures");
#endif

		// Who knows?
		if (texturenum == 0 || texturenum == -1)
			return;

		// Every texture in memory, stored as a 8-bit flat. Wow!
		grtex = &gl_flats[texturenum];

		// Generate flat if missing from the cache
		if (!grtex->mipmap.data && !grtex->mipmap.downloaded)
		{
			HWR_CacheTextureAsFlat(&grtex->mipmap, texturenum, noencoremap);
		}

		// If hardware does not have the texture, then call pfnSetTexture to upload it
		if (!grtex->mipmap.downloaded)
			HWD.pfnSetTexture(&grtex->mipmap);
		HWR_SetCurrentTexture(&grtex->mipmap);

		// The system-memory data can be purged now.
		Z_ChangeTag(grtex->mipmap.data, PU_HWRCACHE_UNLOCKED);

		if ((texturenum = R_GetTextureBrightmap(texturenum)))
		{
			// Every texture in memory, stored as a 8-bit flat. Wow!
			grtex = &gl_flats[texturenum];

			// Generate flat if missing from the cache
			if (!grtex->mipmap.data && !grtex->mipmap.downloaded)
				HWR_CacheTextureAsFlat(&grtex->mipmap, texturenum, true);

			grtex->mipmap.flags |= TF_BRIGHTMAP;

			// If hardware does not have the texture, then call pfnSetTexture to upload it
			if (!grtex->mipmap.downloaded)
				HWD.pfnSetTexture(&grtex->mipmap);
			HWR_SetCurrentTexture(&grtex->mipmap);

			// The system-memory data can be purged now.
			Z_ChangeTag(grtex->mipmap.data, PU_HWRCACHE_UNLOCKED);
		}
	}
	else if (levelflat->type == LEVELFLAT_PATCH)
	{
		patch_t *patch = (patch_t *)W_CachePatchNum(levelflat->u.flat.lumpnum, PU_CACHE);
		levelflat->width = (uint16_t)(patch->width);
		levelflat->height = (uint16_t)(patch->height);
		HWR_GetPatch(patch);
	}
#ifndef NO_PNG_LUMPS
	else if (levelflat->type == LEVELFLAT_PNG)
	{
		GLMipmap_t *mipmap = (GLMipmap_t *)levelflat->mipmap;

		// Cache the picture.
		if (!levelflat->mippic)
		{
			int32_t pngwidth = 0, pngheight = 0;
			void *pic = Picture_PNGConvert((const uint8_t*)W_CacheLumpNum(levelflat->u.flat.lumpnum, PU_CACHE), PICFMT_FLAT, &pngwidth, &pngheight, NULL, NULL, W_LumpLength(levelflat->u.flat.lumpnum), NULL, (pictureflags_t)0);

			Z_ChangeTag(pic, PU_LEVEL);
			Z_SetUser(pic, &levelflat->mippic);

			levelflat->width = (uint16_t)pngwidth;
			levelflat->height = (uint16_t)pngheight;
		}

		// Make the mipmap.
		if (mipmap == NULL)
		{
			mipmap = (GLMipmap_t *)Z_Calloc(sizeof(GLMipmap_t), PU_STATIC, NULL);
			mipmap->format = GL_TEXFMT_P_8;
			mipmap->flags = TF_WRAPXY|TF_CHROMAKEYED;
			levelflat->mipmap = mipmap;
		}

		if (!mipmap->data && !mipmap->downloaded)
		{
			uint8_t *flat;
			size_t size;

			if (levelflat->mippic == NULL)
				I_Error("HWR_GetLevelFlat: levelflat->mippic == NULL");

			mipmap->width = levelflat->width;
			mipmap->height = levelflat->height;

			size = (mipmap->width * mipmap->height);
			flat = (uint8_t *)Z_Malloc(size, PU_LEVEL, &mipmap->data);
			M_Memcpy(flat, levelflat->mippic, size);
		}

		// Tell the hardware driver to bind the current texture to the flat's mipmap
		HWR_SetCurrentTexture(mipmap);
	}
#endif
	else // set no texture
		HWR_SetCurrentTexture(NULL);
}

// --------------------+
// HWR_LoadPatchMipmap : Generates a patch into a mipmap, usually the mipmap inside the patch itself
// --------------------+
static void HWR_LoadPatchMipmap(patch_t *patch, GLMipmap_t *grMipmap)
{
	GLPatch_t *grPatch = (GLPatch_t *)patch->hardware;
	if (!grMipmap->downloaded && !grMipmap->data)
		HWR_MakePatch(patch, grPatch, grMipmap, true);

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!grMipmap->downloaded)
		HWD.pfnSetTexture(grMipmap);
	HWR_SetCurrentTexture(grMipmap);

	// The system-memory data can be purged now.
	Z_ChangeTag(grMipmap->data, PU_HWRCACHE_UNLOCKED);
}

// ----------------------+
// HWR_UpdatePatchMipmap : Updates a mipmap.
// ----------------------+
static void HWR_UpdatePatchMipmap(patch_t *patch, GLMipmap_t *grMipmap)
{
	GLPatch_t *grPatch = (GLPatch_t *)patch->hardware;
	HWR_MakePatch(patch, grPatch, grMipmap, true);

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	// If it does have the texture, then call pfnUpdateTexture to update it
	if (!grMipmap->downloaded)
		HWD.pfnSetTexture(grMipmap);
	else
		HWD.pfnUpdateTexture(grMipmap);
	HWR_SetCurrentTexture(grMipmap);

	// The system-memory data can be purged now.
	Z_ChangeTag(grMipmap->data, PU_HWRCACHE_UNLOCKED);
}

// -----------------+
// HWR_GetPatch     : Downloads a patch to the hardware cache and make it ready for use
// -----------------+
void HWR_GetPatch(patch_t *patch)
{
	if (!patch->hardware)
		Patch_CreateGL(patch);
	HWR_LoadPatchMipmap(patch, ((GLPatch_t *)patch->hardware)->mipmap);
}

// -------------------+
// HWR_GetMappedPatch : Same as HWR_GetPatch for sprite color
// -------------------+
void HWR_GetMappedPatch(patch_t *patch, const uint8_t *colormap)
{
	GLPatch_t *grPatch;
	GLMipmap_t *grMipmap, *newMipmap;

	if (!patch->hardware)
		Patch_CreateGL(patch);
	grPatch = (GLPatch_t *)patch->hardware;

	// Blatant hack for encore colormapping aside...
	if (colormap == colormaps || colormap == NULL || colormap == (const uint8_t*)(COLORMAP_REMAPOFFSET))
	{
		// Load the default (green) color in hardware cache
		HWR_GetPatch(patch);
		return;
	}

	// search for the mipmap
	// skip the first (no colormap translated)
	for (grMipmap = grPatch->mipmap; grMipmap->nextcolormap; )
	{
		grMipmap = grMipmap->nextcolormap;
		if (grMipmap->colormap && grMipmap->colormap->source == colormap)
		{
			if (memcmp(grMipmap->colormap->data, colormap, 256 * sizeof(uint8_t)))
			{
				M_Memcpy(grMipmap->colormap->data, colormap, 256 * sizeof(uint8_t));
				HWR_UpdatePatchMipmap(patch, grMipmap);
			}
			else
				HWR_LoadPatchMipmap(patch, grMipmap);
			return;
		}
	}
	// not found, create it!
	// If we are here, the sprite with the current colormap is not already in hardware memory

	//BP: WARNING: don't free it manually without clearing the cache of harware renderer
	//              (it have a liste of mipmap)
	//    this malloc is cleared in HWR_FreeColormapCache
	//    (...) unfortunately z_malloc fragment alot the memory :(so malloc is better
	newMipmap = (GLMipmap_t *)calloc(1, sizeof (*newMipmap));
	if (newMipmap == NULL)
		I_Error("%s: Out of memory", "HWR_GetMappedPatch");
	grMipmap->nextcolormap = newMipmap;

	newMipmap->colormap = (GLColormap_t *)Z_Calloc(sizeof(*newMipmap->colormap), PU_HWRPATCHCOLMIPMAP, NULL);
	newMipmap->colormap->source = colormap;
	M_Memcpy(newMipmap->colormap->data, colormap, 256 * sizeof(uint8_t));

	HWR_LoadPatchMipmap(patch, newMipmap);
}

void HWR_UnlockCachedPatch(GLPatch_t *gpatch)
{
	if (!gpatch)
		return;

	Z_ChangeTag(gpatch->mipmap->data, PU_HWRCACHE_UNLOCKED);
}

static const int32_t picmode2GR[] =
{
	GL_TEXFMT_P_8,                // PALETTE
	0,                            // INTENSITY          (unsupported yet)
	GL_TEXFMT_ALPHA_INTENSITY_88, // INTENSITY_ALPHA    (corona use this)
	0,                            // RGB24              (unsupported yet)
	GL_TEXFMT_RGBA,               // RGBA32             (opengl only)
};

static void HWR_DrawPicInCache(uint8_t *block, int32_t pblockwidth, int32_t pblockheight,
	int32_t blockmodulo, pic_t *pic, int32_t bpp)
{
	int32_t i,j;
	fixed_t posx, posy, stepx, stepy;
	uint8_t *dest, *src, texel;
	uint16_t texelu16;
	int32_t picbpp;
	RGBA_t col;

	stepy = ((int32_t)LSBF_SHORT(pic->height)<<FRACBITS)/pblockheight;
	stepx = ((int32_t)LSBF_SHORT(pic->width)<<FRACBITS)/pblockwidth;
	picbpp = format2bpp((GLTextureFormat_t)picmode2GR[pic->mode]);
	posy = 0;
	for (j = 0; j < pblockheight; j++)
	{
		posx = 0;
		dest = &block[j*blockmodulo];
		src = &pic->data[(posy>>FRACBITS)*LSBF_SHORT(pic->width)*picbpp];
		for (i = 0; i < pblockwidth;i++)
		{
			switch (pic->mode)
			{ // source bpp
				case PALETTE :
					texel = src[(posx+FRACUNIT/2)>>FRACBITS];
					switch (bpp)
					{ // destination bpp
						case 1 :
							*dest++ = texel; break;
						case 2 :
							texelu16 = (uint16_t)(texel | 0xff00);
							memcpy(dest, &texelu16, sizeof(uint16_t));
							dest += sizeof(uint16_t);
							break;
						case 3 :
							col = V_GetColor(texel);
							memcpy(dest, &col, sizeof(RGBA_t)-sizeof(uint8_t));
							dest += sizeof(RGBA_t)-sizeof(uint8_t);
							break;
						case 4 :
							memcpy(dest, &V_GetColor(texel), sizeof(RGBA_t));
							dest += sizeof(RGBA_t);
							break;
					}
					break;
				case INTENSITY :
					*dest++ = src[(posx+FRACUNIT/2)>>FRACBITS];
					break;
				case INTENSITY_ALPHA : // assume dest bpp = 2
					memcpy(dest, src + ((posx+FRACUNIT/2)>>FRACBITS)*sizeof(uint16_t), sizeof(uint16_t));
					dest += sizeof(uint16_t);
					break;
				case RGB24 :
					break;  // not supported yet
				case RGBA32 : // assume dest bpp = 4
					dest += sizeof(uint32_t);
					memcpy(dest, src + ((posx+FRACUNIT/2)>>FRACBITS)*sizeof(uint32_t), sizeof(uint32_t));
					break;
			}
			posx += stepx;
		}
		posy += stepy;
	}
}

// -----------------+
// HWR_GetPic       : Download a Doom pic (raw row encoded with no 'holes')
// Returns          :
// -----------------+
patch_t *HWR_GetPic(lumpnum_t lumpnum)
{
	patch_t *patch = HWR_GetCachedGLPatch(lumpnum);
	GLPatch_t *grPatch = (GLPatch_t *)(patch->hardware);

	if (!grPatch->mipmap->downloaded && !grPatch->mipmap->data)
	{
		pic_t *pic;
		uint8_t *block;
		size_t len;

		pic = (pic_t*)W_CacheLumpNum(lumpnum, PU_CACHE);
		patch->width = LSBF_SHORT(pic->width);
		patch->height = LSBF_SHORT(pic->height);
		len = W_LumpLength(lumpnum) - sizeof (pic_t);

		grPatch->mipmap->width = (uint16_t)patch->width;
		grPatch->mipmap->height = (uint16_t)patch->height;

		if (pic->mode == PALETTE)
			grPatch->mipmap->format = (GLTextureFormat_t)textureformat; // can be set by driver
		else
			grPatch->mipmap->format = (GLTextureFormat_t)picmode2GR[pic->mode];

		Z_Free(grPatch->mipmap->data);

		// allocate block
		block = MakeBlock(grPatch->mipmap);

		if (patch->width  == LSBF_SHORT(pic->width) &&
			patch->height == LSBF_SHORT(pic->height) &&
			format2bpp(grPatch->mipmap->format) == format2bpp((GLTextureFormat_t)picmode2GR[pic->mode]))
		{
			// no conversion needed
			M_Memcpy(grPatch->mipmap->data, pic->data,len);
		}
		else
			HWR_DrawPicInCache(block, LSBF_SHORT(pic->width), LSBF_SHORT(pic->height),
			                   LSBF_SHORT(pic->width)*format2bpp(grPatch->mipmap->format),
			                   pic,
			                   format2bpp(grPatch->mipmap->format));

		Z_Unlock(pic);
		Z_ChangeTag(block, PU_HWRCACHE_UNLOCKED);

		grPatch->mipmap->flags = 0;
		grPatch->max_s = grPatch->max_t = 1.0f;
	}
	HWD.pfnSetTexture(grPatch->mipmap);
	//CONS_Debug(DBG_RENDER, "picloaded at %x as texture %d\n",grPatch->mipmap->data, grPatch->mipmap->downloaded);

	return patch;
}

patch_t *HWR_GetCachedGLPatchPwad(uint16_t wadnum, uint16_t lumpnum)
{
	lumpcache_t *lumpcache = wadfiles[wadnum]->patchcache;
	if (!lumpcache[lumpnum])
	{
		patch_t *ptr = (patch_t *)Z_Calloc(sizeof(patch_t), PU_PATCH, &lumpcache[lumpnum]);
		Patch_Create(NULL, 0, ptr);
		Patch_AllocateHardwarePatch(ptr);
	}
	return (patch_t *)(lumpcache[lumpnum]);
}

patch_t *HWR_GetCachedGLPatch(lumpnum_t lumpnum)
{
	return HWR_GetCachedGLPatchPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

// Need to do this because they aren't powers of 2
static void HWR_DrawFadeMaskInCache(GLMipmap_t *mipmap, int32_t pblockwidth, int32_t pblockheight,
	lumpnum_t fademasklumpnum, uint16_t fmwidth, uint16_t fmheight)
{
	int32_t i,j;
	fixed_t posx, posy, stepx, stepy;
	uint8_t *block = (uint8_t *)mipmap->data; // places the data directly into here
	uint8_t *flat;
	uint8_t *dest, *src, texel;
	RGBA_t col;

	// Place the flats data into flat
	W_ReadLump(fademasklumpnum, Z_Malloc(W_LumpLength(fademasklumpnum),
		PU_HWRCACHE, &flat));

	stepy = ((int32_t)fmheight<<FRACBITS)/pblockheight;
	stepx = ((int32_t)fmwidth<<FRACBITS)/pblockwidth;
	posy = 0;
	for (j = 0; j < pblockheight; j++)
	{
		posx = 0;
		dest = &block[j*(mipmap->width)]; // 1bpp
		src = &flat[(posy>>FRACBITS)*LSBF_SHORT(fmwidth)];
		for (i = 0; i < pblockwidth;i++)
		{
			// fademask bpp is always 1, and is used just for alpha
			texel = src[(posx)>>FRACBITS];
			col = V_GetColor(texel);
			*dest = col.s.red; // take the red level of the colour and use it for alpha, as fademasks do

			dest++;
			posx += stepx;
		}
		posy += stepy;
	}

	Z_Free(flat);
}

static void HWR_CacheFadeMask(GLMipmap_t *grMipmap, lumpnum_t fademasklumpnum)
{
	size_t size;
	uint16_t fmheight = 0, fmwidth = 0;

	// setup the texture info
	grMipmap->format = GL_TEXFMT_ALPHA_8; // put the correct alpha levels straight in so I don't need to convert it later
	grMipmap->flags = 0;

	size = W_LumpLength(fademasklumpnum);

	switch (size)
	{
		// None of these are powers of 2, so I'll need to do what is done for textures and make them powers of 2 before they can be used
		case 256000: // 640x400
			fmwidth = 640;
			fmheight = 400;
			break;
		case 64000: // 320x200
			fmwidth = 320;
			fmheight = 200;
			break;
		case 16000: // 160x100
			fmwidth = 160;
			fmheight = 100;
			break;
		case 4000: // 80x50 (minimum)
			fmwidth = 80;
			fmheight = 50;
			break;
		default: // Bad lump
			CONS_Alert(CONS_WARNING, "Fade mask lump of incorrect size, ignored\n"); // I should avoid this by checking the lumpnum in HWR_RunWipe
			break;
	}

	// Thankfully, this will still work for this scenario
	grMipmap->width  = fmwidth;
	grMipmap->height = fmheight;

	MakeBlock(grMipmap);

	HWR_DrawFadeMaskInCache(grMipmap, fmwidth, fmheight, fademasklumpnum, fmwidth, fmheight);

	// I DO need to convert this because it isn't power of 2 and we need the alpha
}


void HWR_GetFadeMask(lumpnum_t fademasklumpnum)
{
	patch_t *patch = HWR_GetCachedGLPatch(fademasklumpnum);
	GLMipmap_t *grmip = ((GLPatch_t *)Patch_AllocateHardwarePatch(patch))->mipmap;
	if (!grmip->downloaded && !grmip->data)
		HWR_CacheFadeMask(grmip, fademasklumpnum);

	HWD.pfnSetTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}

#endif //HWRENDER
