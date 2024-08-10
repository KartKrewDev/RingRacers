// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2021 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_textures.c
/// \brief Texture generation.

#include "doomdef.h"
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "m_misc.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h" // levelflats
#include "byteptr.h"
#include "dehacked.h"
#include "k_terrain.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h" // HWR_LoadMapTextures
#endif

#include <errno.h>

//
// TEXTURE_T CACHING
// When a texture is first needed, it counts the number of composite columns
//  required in the texture and allocates space for a column directory and
//  any new columns.
// The directory will simply point inside other patches if there is only one
//  patch in a given column, but any columns with multiple patches will have
//  new column_ts generated.
//

INT32 numtextures = 0; // total number of textures found,
// size of following tables

texture_t **textures = NULL;
UINT32 **texturecolumnofs; // column offset lookup table for each texture
UINT8 **texturecache; // graphics data for each generated full-size texture
UINT8 **texturebrightmapcache; // graphics data for brightmap converted for use with a specific texture

INT32 *texturewidth;
fixed_t *textureheight; // needed for texture pegging

INT32 *texturetranslation;
INT32 *texturebrightmaps;

INT32 g_texturenum_dbgline;

// Painfully simple texture id cacheing to make maps load faster. :3
static struct {
	char name[9];
	UINT32 hash;
	INT32 id;
} *tidcache = NULL;
static INT32 tidcachelen = 0;

//
// MAPTEXTURE_T CACHING
// When a texture is first needed, it counts the number of composite columns
//  required in the texture and allocates space for a column directory and
//  any new columns.
// The directory will simply point inside other patches if there is only one
//  patch in a given column, but any columns with multiple patches will have
//  new column_ts generated.
//

//
// R_DrawColumnInCache
// Clip and draw a column from a patch into a cached post.
//
static inline void R_DrawColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	(void)patchheight; // This parameter is unused

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		source = (UINT8 *)patch + 3;
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source -= position; // start further down the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		if (count > 0)
			M_Memcpy(cache + position, source, count);

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

//
// R_DrawFlippedColumnInCache
// Similar to R_DrawColumnInCache; it draws the column inverted, however.
//
static inline void R_DrawFlippedColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = patchheight-patch->length-topdelta;
		source = (UINT8 *)patch + 2 + patch->length; // patch + 3 + (patch->length-1)
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source += position; // start further UP the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		if (count > 0)
		{
			for (; dest < cache + position + count; --source)
				*dest++ = *source;
		}

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

//
// R_DrawBlendColumnInCache
// Draws a translucent column into the cache, applying a half-cooked equation to get a proper translucency value (Needs code in R_GenerateTexture()).
//
static inline void R_DrawBlendColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	(void)patchheight; // This parameter is unused

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		source = (UINT8 *)patch + 3;
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source -= position; // start further down the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		if (count > 0)
		{
			for (; dest < cache + position + count; source++, dest++)
				if (*source != 0xFF)
					*dest = ASTBlendPaletteIndexes(*dest, *source, originPatch->style, originPatch->alpha);
		}

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

//
// R_DrawBlendFlippedColumnInCache
// Similar to the one above except that the column is inverted.
//
static inline void R_DrawBlendFlippedColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = patchheight-patch->length-topdelta;
		source = (UINT8 *)patch + 2 + patch->length; // patch + 3 + (patch->length-1)
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source += position; // start further UP the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		if (count > 0)
		{
			for (; dest < cache + position + count; --source, dest++)
				if (*source != 0xFF)
					*dest = ASTBlendPaletteIndexes(*dest, *source, originPatch->style, originPatch->alpha);
		}

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

static UINT8 *R_AllocateTextureBlock(size_t blocksize, UINT8 **user)
{
	texturememory += blocksize;

	return Z_Malloc(blocksize, PU_LEVEL, user);
}

static UINT8 *R_AllocateDummyTextureBlock(size_t width, UINT8 **user)
{
	// Allocate dummy data. Keep 4-bytes aligned.
	// Column offsets will be initialized to 0, which points to the 0xff byte (empty column flag).
	size_t blocksize = 4 + (width * 4);
	UINT8 *block = R_AllocateTextureBlock(blocksize, user);

	memset(block, 0, blocksize);
	block[0] = 0xff;

	return block;
}

static boolean R_CheckTextureLumpLength(texture_t *texture, size_t patch)
{
	UINT16 wadnum = texture->patches[patch].wad;
	UINT16 lumpnum = texture->patches[patch].lump;
	size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);

	// The header does not exist
	if (lumplength < offsetof(softwarepatch_t, columnofs))
	{
		CONS_Alert(
			CONS_ERROR,
			"%.8s: texture lump data is too small. Expected %s bytes, got %s. (%s)\n",
			texture->name,
			sizeu1(offsetof(softwarepatch_t, columnofs)),
			sizeu2(lumplength),
			wadfiles[wadnum]->lumpinfo[lumpnum].fullname
		);

		return false;
	}

	return true;
}

//
// R_GenerateTexture
//
// Allocate space for full size texture, either single patch or 'composite'
// Build the full textures from patches.
// The texture caching system is a little more hungry of memory, but has
// been simplified for the sake of highcolor (lol), dynamic ligthing, & speed.
//
// This is not optimised, but it's supposed to be executed only once
// per level, when enough memory is available.
//
UINT8 *R_GenerateTexture(size_t texnum)
{
	UINT8 *block;
	UINT8 *blocktex;
	texture_t *texture;
	texpatch_t *patch;
	softwarepatch_t *realpatch;
	UINT8 *pdata;
	int x, x1, x2, i, width, height;
	size_t blocksize;
	column_t *patchcol;
	UINT8 *colofs;

	UINT16 wadnum;
	lumpnum_t lumpnum;
	size_t lumplength;

	I_Assert(texnum <= (size_t)numtextures);
	texture = textures[texnum];
	I_Assert(texture != NULL);

	// allocate texture column offset lookup

	// single-patch textures can have holes in them and may be used on
	// 2sided lines so they need to be kept in 'packed' format
	// BUT this is wrong for skies and walls with over 255 pixels,
	// so check if there's holes and if not strip the posts.
	if (texture->patchcount == 1)
	{
		boolean holey = false;
		patch = texture->patches;

		wadnum = patch->wad;
		lumpnum = patch->lump;
		lumplength = W_LumpLengthPwad(wadnum, lumpnum);

		// The header does not exist
		if (R_CheckTextureLumpLength(texture, 0) == false)
		{
			block = R_AllocateDummyTextureBlock(texture->width, &texturecache[texnum]);
			texturecolumnofs[texnum] = (UINT32*)&block[4];
			textures[texnum]->holes = true;
			return block;
		}

		pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_LEVEL);
		realpatch = (softwarepatch_t *)pdata;

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG((UINT8 *)realpatch, lumplength))
			goto multipatch;
#endif
#ifdef WALLFLATS
		if (texture->type == TEXTURETYPE_FLAT)
			goto multipatch;
#endif

		// Check the patch for holes.
		if (texture->width > SHORT(realpatch->width) || texture->height > SHORT(realpatch->height))
			holey = true;
		colofs = (UINT8 *)realpatch->columnofs;
		for (x = 0; x < texture->width && !holey; x++)
		{
			column_t *col = (column_t *)((UINT8 *)realpatch + LONG(*(UINT32 *)&colofs[x<<2]));
			INT32 topdelta, prevdelta = -1, y = 0;
			while (col->topdelta != 0xff)
			{
				topdelta = col->topdelta;
				if (topdelta <= prevdelta)
					topdelta += prevdelta;
				prevdelta = topdelta;
				if (topdelta > y)
					break;
				y = topdelta + col->length + 1;
				col = (column_t *)((UINT8 *)col + col->length + 4);
			}
			if (y < texture->height)
				holey = true; // this texture is HOLEy! D:
		}

		// If the patch uses transparency, we have to save it this way.
		if (holey)
		{
			texture->holes = true;
			texture->flip = patch->flip;
			blocksize = lumplength;
			block = Z_Calloc(blocksize, PU_LEVEL, // will change tag at end of this function
				&texturecache[texnum]);
			M_Memcpy(block, realpatch, blocksize);
			texturememory += blocksize;

			// use the patch's column lookup
			colofs = (block + 8);
			texturecolumnofs[texnum] = (UINT32 *)colofs;
			blocktex = block;
			if (patch->flip & 1) // flip the patch horizontally
			{
				UINT8 *realcolofs = (UINT8 *)realpatch->columnofs;
				for (x = 0; x < texture->width; x++)
					*(UINT32 *)&colofs[x<<2] = realcolofs[( texture->width-1-x )<<2]; // swap with the offset of the other side of the texture
			}
			// we can't as easily flip the patch vertically sadly though,
			//  we have wait until the texture itself is drawn to do that
			for (x = 0; x < texture->width; x++)
				*(UINT32 *)&colofs[x<<2] = LONG(LONG(*(UINT32 *)&colofs[x<<2]) + 3);
			goto done;
		}

		// Otherwise, do multipatch format.
	}

	// multi-patch textures (or 'composite')
	multipatch:
	texture->holes = false;
	texture->flip = 0;
	blocksize = (texture->width * 4) + (texture->width * texture->height);
	texturememory += blocksize;
	block = Z_Malloc(blocksize+1, PU_LEVEL, &texturecache[texnum]);

	memset(block, TRANSPARENTPIXEL, blocksize+1); // Transparency hack

	// columns lookup table
	colofs = block;
	texturecolumnofs[texnum] = (UINT32 *)colofs;

	// texture data after the lookup table
	blocktex = block + (texture->width*4);

	// Composite the columns together.
	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		boolean dealloc = true;
		static void (*ColumnDrawerPointer)(column_t *, UINT8 *, texpatch_t *, INT32, INT32); // Column drawing function pointer.
		if (patch->style != AST_COPY)
			ColumnDrawerPointer = (patch->flip & 2) ? R_DrawBlendFlippedColumnInCache : R_DrawBlendColumnInCache;
		else
			ColumnDrawerPointer = (patch->flip & 2) ? R_DrawFlippedColumnInCache : R_DrawColumnInCache;

		wadnum = patch->wad;
		lumpnum = patch->lump;
		pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_LEVEL);
		lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		realpatch = (softwarepatch_t *)pdata;
		dealloc = true;

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG((UINT8 *)realpatch, lumplength))
			realpatch = (softwarepatch_t *)Picture_PNGConvert((UINT8 *)realpatch, PICFMT_DOOMPATCH, NULL, NULL, NULL, NULL, lumplength, NULL, 0);
		else
#endif
#ifdef WALLFLATS
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = (softwarepatch_t *)Picture_Convert(PICFMT_FLAT, pdata, PICFMT_DOOMPATCH, 0, NULL, texture->width, texture->height, 0, 0, 0);
		else
#endif
		{
			(void)lumplength;
			dealloc = false;
		}

		x1 = patch->originx;
		width = SHORT(realpatch->width);
		height = SHORT(realpatch->height);
		x2 = x1 + width;

		if (x1 > texture->width || x2 < 0)
		{
			if (dealloc)
				Z_Free(realpatch);
			continue; // patch not located within texture's x bounds, ignore
		}

		if (patch->originy > texture->height || (patch->originy + height) < 0)
		{
			if (dealloc)
				Z_Free(realpatch);
			continue; // patch not located within texture's y bounds, ignore
		}

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

		for (; x < x2; x++)
		{
			if (patch->flip & 1)
				patchcol = (column_t *)((UINT8 *)realpatch + LONG(realpatch->columnofs[(x1+width-1)-x]));
			else
				patchcol = (column_t *)((UINT8 *)realpatch + LONG(realpatch->columnofs[x-x1]));

			// generate column ofset lookup
			*(UINT32 *)&colofs[x<<2] = LONG((x * texture->height) + (texture->width*4));
			ColumnDrawerPointer(patchcol, block + LONG(*(UINT32 *)&colofs[x<<2]), patch, texture->height, height);
		}

		if (dealloc)
			Z_Free(realpatch);
	}

done:
	return blocktex;
}

//
// R_GenerateTextureAsFlat
//
// Generates a flat picture for a texture.
//
UINT8 *R_GenerateTextureAsFlat(size_t texnum)
{
	texture_t *texture = textures[texnum];
	UINT8 *converted = NULL;
	size_t size = (texture->width * texture->height);

	// The flat picture for this texture was not generated yet.
	if (!texture->flat)
	{
		// Well, let's do it now, then.
		Z_Malloc(size, PU_LEVEL, &texture->flat);

		// Picture_TextureToFlat handles everything for us.
		converted = (UINT8 *)Picture_TextureToFlat(texnum);
		M_Memcpy(texture->flat, converted, size);
		Z_Free(converted);
	}

	return texture->flat;
}

// This function writes a column to p, using the posts from
// tcol, but the pixel values from bcol. If bcol is larger
// than tcol, the pixels are cropped. If bcol is smaller than
// tcol, the empty space is filled with TRANSPARENTPIXEL.
static void R_ConvertBrightmapColumn(UINT8 *p, const column_t *tcol, const column_t *bcol)
{
	/*

	___t1
	|
	|     ___b1
	|     |
	|     |
	|__t2 |
	|     |__b2
	|
	|     ___b3
	|__t3 |
		  |
	___t4 |
	|     |__b4
	|__t5

	___t6
	|__t7

	*/

	// copy post header
	memcpy(&p[-3], tcol, 3);

	INT32 ttop = tcol->topdelta;
	INT32 btop = bcol->topdelta;

	INT32 y = ttop;

	while (tcol->topdelta != 0xff && bcol->topdelta != 0xff)
	{
		INT32 tbot = ttop + tcol->length;
		INT32 bbot = btop + bcol->length;

		INT32 n;

		// t1 to b1
		// b2 to b3
		// --------
		// The brightmap column starts below the
		// texture column, so pad it with black
		// pixels.

		n = max(0, min(btop, tbot) - y);
		memset(&p[y - ttop], TRANSPARENTPIXEL, n);
		y += n;

		// b1 to t2
		// t2 to b2
		// b3 to t3
		// t4 to b4
		// --------
		// Copy parts of the brightmap column which
		// line up with the texture column.

		n = max(0, min(bbot, tbot) - y);
		memcpy(&p[y - ttop], (const UINT8*)bcol + 3 + (y - btop), n);
		y += n;

		if (y == tbot)
		{
			p += 4 + tcol->length;
			tcol = (const column_t*)((const UINT8*)tcol + 4 + tcol->length);

			memcpy(&p[-3], tcol, 3); // copy post header

			// Tall patches add the topdelta if it is less
			// than the running topdelta.
			ttop = tcol->topdelta <= ttop ? ttop + tcol->topdelta : tcol->topdelta;
			y = ttop;
		}

		if (y >= bbot)
		{
			bcol = (const column_t*)((const UINT8*)bcol + 4 + bcol->length);
			btop = bcol->topdelta <= btop ? btop + bcol->topdelta : bcol->topdelta; // tall patches
		}
	}

	if (tcol->topdelta != 0xff)
	{
		// b4 to t5
		// t6 to t7
		// --------
		// The texture column continues past the end
		// of the brightmap column, so pad it with
		// black pixels.

		y -= ttop;
		memset(&p[y], TRANSPARENTPIXEL, tcol->length - y);

		while
		(
			(
				p += 4 + tcol->length,
				tcol = (const column_t*)((const UINT8*)tcol + 4 + tcol->length)
			)->topdelta != 0xff
		)
		{
			memcpy(&p[-3], tcol, 3); // copy post header
			memset(p, TRANSPARENTPIXEL, tcol->length);
		}
	}

	p[-3] = 0xff;
}

struct rawcheckcolumn_state
{
	softwarepatch_t *patch;
	size_t data_size;
	boolean error;
	const char *name;
};

static void R_InitRawCheckColumn(
		struct rawcheckcolumn_state *state,
		softwarepatch_t *patch,
		size_t size,
		const char *name
)
{
	state->patch = patch;
	state->data_size = size;
	state->error = (patch == NULL);
	state->name = name;
}

static void R_CheckRawColumn_Error(struct rawcheckcolumn_state *state, const char *error)
{
	if (state->error)
	{
		return;
	}

	CONS_Alert(CONS_WARNING, "%.8s: %s\n", state->name, error);

	state->error = true;
}

static column_t *R_CheckRawColumn(struct rawcheckcolumn_state *state, INT32 x)
{
	static column_t empty = {0xff, 0};

	if (state->error)
	{
		return &empty;
	}

	if (x < SHORT(state->patch->width))
	{
		size_t ofs = LONG(state->patch->columnofs[x]);

		if (ofs < state->data_size)
		{
			return (column_t*)((UINT8*)state->patch + ofs);
		}
		else
		{
			R_CheckRawColumn_Error(state, "Patch column offsets go out of bounds."
					" Make sure the lump is in Doom Graphics format and not a Flat or PNG or anything else.");
		}
	}

	return &empty;
}

// Remember, this function must generate a texture that
// matches the layout of texnum. It must have the same width
// and same columns. Only the pixels that overlap are copied
// from the brightmap texture.
UINT8 *R_GenerateTextureBrightmap(size_t texnum)
{
	texture_t *texture = textures[texnum];
	texture_t *bright = textures[R_GetTextureBrightmap(texnum)];

	if (R_TextureHasBrightmap(texnum) && bright->patchcount > 1)
	{
		CONS_Alert(
				CONS_WARNING,
				"%.8s: BRIGHTMAP should not be a composite texture. Only using the first patch.\n",
				bright->name
		);
	}

	if (R_CheckTextureLumpLength(texture, 0) == false)
	{
		return R_AllocateDummyTextureBlock(texture->width, &texturebrightmapcache[texnum]);
	}

	R_CheckTextureCache(texnum);

	softwarepatch_t *bmap = NULL;
	struct rawcheckcolumn_state rchk;

	if (R_TextureHasBrightmap(texnum) && R_CheckTextureLumpLength(bright, 0))
	{
		INT32 wad = bright->patches[0].wad;
		INT32 lump = bright->patches[0].lump;

		bmap = W_CacheLumpNumPwad(wad, lump, PU_STATIC);
		R_InitRawCheckColumn(&rchk, bmap, W_LumpLengthPwad(wad, lump), bright->name);
	}
	else
	{
		R_InitRawCheckColumn(&rchk, NULL, 0, bright->name);
	}

	UINT8 *block;

	if (texture->holes)
	{
		block = R_AllocateTextureBlock(
				W_LumpLengthPwad(texture->patches[0].wad, texture->patches[0].lump),
				&texturebrightmapcache[texnum]
		);

		INT32 x;

		for (x = 0; x < texture->width; ++x)
		{
			const column_t *tcol = (column_t*)(R_GetColumn(texnum, x) - 3);
			const column_t *bcol = R_CheckRawColumn(&rchk, x);

			R_ConvertBrightmapColumn(block + LONG(texturecolumnofs[texnum][x]), tcol, bcol);
		}
	}
	else
	{
		// Allocate the same size as composite textures.
		size_t blocksize = (texture->width * 4) + (texture->width * texture->height) + 1;

		block = R_AllocateTextureBlock(blocksize, &texturebrightmapcache[texnum]);
		memset(block, TRANSPARENTPIXEL, blocksize); // Transparency hack

		texpatch_t origin = {0};
		INT32 x;

		for (x = 0; x < texture->width; ++x)
		{
			R_DrawColumnInCache(
					R_CheckRawColumn(&rchk, x),
					block + LONG(texturecolumnofs[texnum][x]),
					&origin,
					texture->height,
					SHORT(bmap->height)
			);
		}
	}

	Z_Free(bmap);

	return block;
}

//
// R_GetTextureNum
//
// Returns the actual texture id that we should use.
// This can either be texnum, the current frame for texnum's anim (if animated),
// or 0 if not valid.
//
INT32 R_GetTextureNum(INT32 texnum)
{
	if (texnum < 0 || texnum >= numtextures)
		return 0;
	return texturetranslation[texnum];
}

//
// R_GetTextureBrightmap
//
// Returns the actual texture id that we should use.
// This can either be the texture's brightmap,
// or 0 if not valid.
//
INT32 R_GetTextureBrightmap(INT32 texnum)
{
	if (texnum < 0 || texnum >= numtextures)
		return 0;
	return texturebrightmaps[texnum];
}

boolean R_TextureHasBrightmap(INT32 texnum)
{
	return R_GetTextureBrightmap(texnum) != 0;
}

boolean R_TextureCanRemap(INT32 texnum)
{
	const terrain_t *t = K_GetTerrainForTextureNum(texnum);
	return !t || t->flags & TRF_REMAP;
}

//
// R_CheckTextureCache
//
// Use this if you need to make sure the texture is cached before R_GetColumn calls
// e.g.: midtextures and FOF walls
//
void R_CheckTextureCache(INT32 tex)
{
	if (!texturecache[tex])
		R_GenerateTexture(tex);
}

static inline INT32 wrap_column(fixed_t tex, INT32 col)
{
	INT32 width = texturewidth[tex];

	if (width & (width - 1))
		col = (UINT32)col % width;
	else
		col &= (width - 1);

	return col;
}

//
// R_GetColumn
//
UINT8 *R_GetColumn(fixed_t tex, INT32 col)
{
	if (!texturecache[tex])
		R_GenerateTexture(tex);

	return texturecache[tex] + LONG(texturecolumnofs[tex][wrap_column(tex, col)]);
}

//
// R_GetBrightmapColumn
//
UINT8 *R_GetBrightmapColumn(fixed_t tex, INT32 col)
{
	if (!texturebrightmapcache[tex])
		R_GenerateTextureBrightmap(tex);

	return texturebrightmapcache[tex] + LONG(texturecolumnofs[tex][wrap_column(tex, col)]);
}

void *R_GetFlat(lumpnum_t flatlumpnum)
{
	return W_CacheLumpNum(flatlumpnum, PU_LEVEL);
}

//
// R_GetLevelFlat
//
// If needed, convert a texture or patch to a flat.
//
void *R_GetLevelFlat(drawspandata_t* ds, levelflat_t *levelflat)
{
	boolean isleveltexture = (levelflat->type == LEVELFLAT_TEXTURE);
	texture_t *texture = (isleveltexture ? textures[levelflat->u.texture.num] : NULL);
	boolean texturechanged = (isleveltexture ? (levelflat->u.texture.num != levelflat->u.texture.lastnum) : false);
	UINT8 *flatdata = NULL;

	// Check if the texture changed.
	if (isleveltexture && (!texturechanged))
	{
		if (texture->flat)
		{
			flatdata = texture->flat;
			ds->flatwidth = texture->width;
			ds->flatheight = texture->height;
			texturechanged = false;
		}
		else
			texturechanged = true;
	}

	// If the texture changed, or the flat wasn't generated, convert.
	if (levelflat->picture == NULL || texturechanged)
	{
		// Level texture
		if (isleveltexture)
		{
			levelflat->picture = R_GenerateTextureAsFlat(levelflat->u.texture.num);
			ds->flatwidth = levelflat->width = texture->width;
			ds->flatheight = levelflat->height = texture->height;
		}
		else
		{
#ifndef NO_PNG_LUMPS
			if (levelflat->type == LEVELFLAT_PNG)
			{
				INT32 pngwidth, pngheight;

				levelflat->picture = Picture_PNGConvert(W_CacheLumpNum(levelflat->u.flat.lumpnum, PU_LEVEL), PICFMT_FLAT, &pngwidth, &pngheight, NULL, NULL, W_LumpLength(levelflat->u.flat.lumpnum), NULL, 0);
				levelflat->width = (UINT16)pngwidth;
				levelflat->height = (UINT16)pngheight;

				ds->flatwidth = levelflat->width;
				ds->flatheight = levelflat->height;
			}
			else
#endif
			if (levelflat->type == LEVELFLAT_PATCH)
			{
				UINT8 *converted;
				size_t size;
				softwarepatch_t *patch = W_CacheLumpNum(levelflat->u.flat.lumpnum, PU_LEVEL);

				levelflat->width = ds->flatwidth = SHORT(patch->width);
				levelflat->height = ds->flatheight = SHORT(patch->height);

				levelflat->picture = Z_Malloc(levelflat->width * levelflat->height, PU_LEVEL, NULL);
				converted = Picture_FlatConvert(PICFMT_DOOMPATCH, patch, PICFMT_FLAT, 0, &size, levelflat->width, levelflat->height, SHORT(patch->topoffset), SHORT(patch->leftoffset), 0);
				M_Memcpy(levelflat->picture, converted, size);
				Z_Free(converted);
			}
		}
	}
	else
	{
		ds->flatwidth = levelflat->width;
		ds->flatheight = levelflat->height;
	}

	levelflat->u.texture.lastnum = levelflat->u.texture.num;

	if (flatdata == NULL)
		flatdata = levelflat->picture;
	return flatdata;
}

//
// R_CheckPowersOfTwo
//
// Sets ds_powersoftwo true if the flat's dimensions are powers of two, and returns that.
//
boolean R_CheckPowersOfTwo(drawspandata_t* ds)
{
	boolean wpow2 = (!(ds->flatwidth & (ds->flatwidth - 1)));
	boolean hpow2 = (!(ds->flatheight & (ds->flatheight - 1)));

	// Initially, the flat isn't powers-of-two-sized.
	ds->powersoftwo = false;

	// But if the width and height are powers of two,
	// and are EQUAL, then it's okay :]
	if ((ds->flatwidth == ds->flatheight) && (wpow2 && hpow2))
		ds->powersoftwo = true;

	// Just return ds_powersoftwo.
	return ds->powersoftwo;
}

//
// R_FlatDimensionsFromLumpSize
//
// Returns the flat's square size from its lump length.
//
size_t R_FlatDimensionsFromLumpSize(size_t size)
{
	switch (size)
	{
		case 4194304: // 2048x2048 lump
			return 2048;

		case 1048576: // 1024x1024 lump
			return 1024;

		case 262144:// 512x512 lump
			return 512;

		case 65536: // 256x256 lump
			return 256;

		case 16384: // 128x128 lump
			return 128;

		case 1024: // 32x32 lump
			return 32;

		case 256: // 16x16 lump
			return 16;

		case 64: // 8x8 lump
			return 8;

		default: // 64x64 lump
			return 64;
	}
}

//
// R_CheckFlatLength
//
// Determine the flat's dimensions from its lump length.
//
void R_CheckFlatLength(drawspandata_t* ds, size_t size)
{
	switch (size)
	{
		case 4194304: // 2048x2048 lump
			ds->nflatmask = 0x3FF800;
			ds->nflatxshift = 21;
			ds->nflatyshift = 10;
			ds->nflatshiftup = 5;
			ds->flatwidth = ds->flatheight = 2048;
			break;
		case 1048576: // 1024x1024 lump
			ds->nflatmask = 0xFFC00;
			ds->nflatxshift = 22;
			ds->nflatyshift = 12;
			ds->nflatshiftup = 6;
			ds->flatwidth = ds->flatheight = 1024;
			break;
		case 262144:// 512x512 lump
			ds->nflatmask = 0x3FE00;
			ds->nflatxshift = 23;
			ds->nflatyshift = 14;
			ds->nflatshiftup = 7;
			ds->flatwidth = ds->flatheight = 512;
			break;
		case 65536: // 256x256 lump
			ds->nflatmask = 0xFF00;
			ds->nflatxshift = 24;
			ds->nflatyshift = 16;
			ds->nflatshiftup = 8;
			ds->flatwidth = ds->flatheight = 256;
			break;
		case 16384: // 128x128 lump
			ds->nflatmask = 0x3F80;
			ds->nflatxshift = 25;
			ds->nflatyshift = 18;
			ds->nflatshiftup = 9;
			ds->flatwidth = ds->flatheight = 128;
			break;
		case 1024: // 32x32 lump
			ds->nflatmask = 0x3E0;
			ds->nflatxshift = 27;
			ds->nflatyshift = 22;
			ds->nflatshiftup = 11;
			ds->flatwidth = ds->flatheight = 32;
			break;
		case 256: // 16x16 lump
			ds->nflatmask = 0xF0;
			ds->nflatxshift = 28;
			ds->nflatyshift = 24;
			ds->nflatshiftup = 12;
			ds->flatwidth = ds->flatheight = 16;
			break;
		case 64: // 8x8 lump
			ds->nflatmask = 0x38;
			ds->nflatxshift = 29;
			ds->nflatyshift = 26;
			ds->nflatshiftup = 13;
			ds->flatwidth = ds->flatheight = 8;
			break;
		default: // 64x64 lump
			ds->nflatmask = 0xFC0;
			ds->nflatxshift = 26;
			ds->nflatyshift = 20;
			ds->nflatshiftup = 10;
			ds->flatwidth = ds->flatheight = 64;
			break;
	}
}

//
// Empty the texture cache (used for load wad at runtime)
//
void R_FlushTextureCache(void)
{
	INT32 i;

	if (numtextures)
		for (i = 0; i < numtextures; i++)
			Z_Free(texturecache[i]);
}

// Need these prototypes for later; defining them here instead of r_textures.h so they're "private"
int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum);
void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *index);

#ifdef WALLFLATS
static INT32
Rloadflats (INT32 i, INT32 w)
{
	UINT16 j;
	UINT16 texstart, texend;
	texture_t *texture;
	texpatch_t *patch;
	UINT8 header[PNG_HEADER_SIZE];

	// Yes
	if (wadfiles[w]->type == RET_PK3)
	{
		texstart = W_CheckNumForFolderStartPK3("flats/", (UINT16)w, 0);
		texend = W_CheckNumForFolderEndPK3("flats/", (UINT16)w, texstart);
	}
	else
	{
		texstart = W_CheckNumForMarkerStartPwad("F_START", (UINT16)w, 0);
		texend = W_CheckNumForNamePwad("F_END", (UINT16)w, texstart);
	}

	if (!( texstart == INT16_MAX || texend == INT16_MAX ))
	{
		// Work through each lump between the markers in the WAD.
		for (j = 0; j < (texend - texstart); j++)
		{
			UINT16 wadnum = (UINT16)w;
			lumpnum_t lumpnum = texstart + j;
			size_t lumplength;
			size_t flatsize = 0;

			if (wadfiles[w]->type == RET_PK3)
			{
				if (W_IsLumpFolder(wadnum, lumpnum)) // Check if lump is a folder
					continue; // If it is then SKIP IT
			}

			W_ReadLumpHeaderPwad(wadnum, lumpnum, header, sizeof header, 0);
			lumplength = W_LumpLengthPwad(wadnum, lumpnum);

			flatsize = R_FlatDimensionsFromLumpSize(lumplength);

			//CONS_Printf("\n\"%s\" is a flat, dimensions %d x %d",W_CheckNameForNumPwad((UINT16)w,texstart+j),flatsize,flatsize);
			texture = textures[i] = Z_Calloc(sizeof(texture_t) + sizeof(texpatch_t), PU_STATIC, NULL);

			// Set texture properties.
			M_Memcpy(texture->name, W_CheckNameForNumPwad(wadnum, lumpnum), sizeof(texture->name));
			texture->hash = quickncasehash(texture->name, 8);

#ifndef NO_PNG_LUMPS
			if (Picture_IsLumpPNG(header, lumplength))
			{
				UINT8 *flatlump = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
				INT32 width, height;
				Picture_PNGDimensions((UINT8 *)flatlump, &width, &height, NULL, NULL, lumplength);
				texture->width = (INT16)width;
				texture->height = (INT16)height;
				Z_Free(flatlump);
			}
			else
#endif
				texture->width = texture->height = flatsize;

			texture->type = TEXTURETYPE_FLAT;
			texture->patchcount = 1;
			texture->holes = false;
			texture->flip = 0;
			texture->terrainID = K_GetTerrainIDForTextureName(texture->name);

			// Allocate information for the texture's patches.
			patch = &texture->patches[0];

			patch->originx = patch->originy = 0;
			patch->wad = (UINT16)w;
			patch->lump = texstart + j;
			patch->flip = 0;

			texturewidth[i] = texture->width;
			textureheight[i] = texture->height << FRACBITS;
			i++;
		}
	}

	return i;
}
#endif/*WALLFLATS*/

#define TX_START "TX_START"
#define TX_END "TX_END"

static INT32
Rloadtextures (INT32 i, INT32 w)
{
	UINT16 j;
	UINT16 texstart, texend, texturesLumpPos;
	texture_t *texture;
	texpatch_t *patch;
	softwarepatch_t patchlump;

	// Get the lump numbers for the markers in the WAD, if they exist.
	if (wadfiles[w]->type == RET_PK3)
	{
		texstart = W_CheckNumForFolderStartPK3("textures/", (UINT16)w, 0);
		texend = W_CheckNumForFolderEndPK3("textures/", (UINT16)w, texstart);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		while (texturesLumpPos != INT16_MAX)
		{
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);
			texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, texturesLumpPos + 1);
		}
	}
	else
	{
		texstart = W_CheckNumForMarkerStartPwad(TX_START, (UINT16)w, 0);
		texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, 0);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		if (texturesLumpPos != INT16_MAX)
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);
	}

	if (!( texstart == INT16_MAX || texend == INT16_MAX ))
	{
		// Work through each lump between the markers in the WAD.
		for (j = 0; j < (texend - texstart); j++)
		{
			UINT16 wadnum = (UINT16)w;
			lumpnum_t lumpnum = texstart + j;
#ifndef NO_PNG_LUMPS
			size_t lumplength;
#endif
			INT32 width, height;

			if (wadfiles[w]->type == RET_PK3)
			{
				if (W_IsLumpFolder(wadnum, lumpnum)) // Check if lump is a folder
					continue; // If it is then SKIP IT
			}

			W_ReadLumpHeaderPwad(wadnum, lumpnum, &patchlump, PNG_HEADER_SIZE, 0);
#ifndef NO_PNG_LUMPS
			lumplength = W_LumpLengthPwad(wadnum, lumpnum);
#endif

#ifndef NO_PNG_LUMPS
			if (Picture_IsLumpPNG((UINT8 *)&patchlump, lumplength))
			{
				UINT8 *png = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
				Picture_PNGDimensions(png, &width, &height, NULL, NULL, lumplength);
				width = (INT16)width;
				height = (INT16)height;
				Z_Free(png);
			}
			else
#endif
			{
				width = SHORT(patchlump.width);
				height = SHORT(patchlump.height);
			}

			INT32 sizeLimit = 2048;
			if (w <= mainwads)
			{
				// TODO: we ran out of time to do this properly.
				// 4096 limit on textures may be incompatible with some older graphics cards (circa 2005-2008?).
				// This only a consideration for Legacy GL at the moment -- these will still work in Software.
				// In the future, we may rely more on hardware rendering and this would become a problem.
				sizeLimit = 4096;
#ifdef DEVELOP
				if ((width > 2048 && width < sizeLimit) || (height > 2048 && height < sizeLimit))
				{
					R_InsertTextureWarning(
						" \x87(2.x developer warning, will not appear on release)\n"
						"\x87These textures should ideally not be larger than 2048x2048:\n",
						va("\x86%s", wadfiles[wadnum]->lumpinfo[lumpnum].fullname)
					);
				}
#endif
			}
			if (width > sizeLimit || height > sizeLimit)
			{
				// This is INTENTIONAL. Even if software can handle it, very old GL hardware will not.
				// For the sake of a compatibility baseline, we will not allow anything larger than this.
				char header[1024];
				sprintf(header,
					"Texture patch size cannot be greater than %dx%d!\n"
					"List of affected textures:\n",
					sizeLimit, sizeLimit);
				R_InsertTextureWarning(header, va("\x82" "WARNING: %s", wadfiles[wadnum]->lumpinfo[lumpnum].fullname));
				continue;
			}

			//CONS_Printf("\n\"%s\" is a single patch, dimensions %d x %d",W_CheckNameForNumPwad((UINT16)w,texstart+j),patchlump->width, patchlump->height);
			texture = textures[i] = Z_Calloc(sizeof(texture_t) + sizeof(texpatch_t), PU_STATIC, NULL);

			// Set texture properties.
			M_Memcpy(texture->name, W_CheckNameForNumPwad(wadnum, lumpnum), sizeof(texture->name));
			texture->hash = quickncasehash(texture->name, 8);

			texture->width = width;
			texture->height = height;

			texture->type = TEXTURETYPE_SINGLEPATCH;
			texture->patchcount = 1;
			texture->holes = false;
			texture->flip = 0;
			texture->terrainID = K_GetTerrainIDForTextureName(texture->name);

			// Allocate information for the texture's patches.
			patch = &texture->patches[0];

			patch->originx = patch->originy = 0;
			patch->wad = (UINT16)w;
			patch->lump = texstart + j;
			patch->flip = 0;

			texturewidth[i] = texture->width;
			textureheight[i] = texture->height << FRACBITS;
			i++;
		}
	}

	return i;
}

static INT32
count_range
(		const char * marker_start,
		const char * marker_end,
		const char * folder,
		UINT16 wadnum)
{
	UINT16 j;
	UINT16 texstart, texend;
	INT32 count = 0;

	// Count flats
	if (wadfiles[wadnum]->type == RET_PK3)
	{
		texstart = W_CheckNumForFolderStartPK3(folder, wadnum, 0);
		texend = W_CheckNumForFolderEndPK3(folder, wadnum, texstart);
	}
	else
	{
		texstart = W_CheckNumForMarkerStartPwad(marker_start, wadnum, 0);
		texend = W_CheckNumForNamePwad(marker_end, wadnum, texstart);
	}

	if (texstart != INT16_MAX && texend != INT16_MAX)
	{
		// PK3s have subfolders, so we can't just make a simple sum
		if (wadfiles[wadnum]->type == RET_PK3)
		{
			for (j = texstart; j < texend; j++)
			{
				if (!W_IsLumpFolder(wadnum, j)) // Check if lump is a folder; if not, then count it
					count++;
			}
		}
		else // Add all the textures between markers
		{
			count += (texend - texstart);
		}
	}

	return count;
}

static INT32 R_CountTextures(UINT16 wadnum)
{
	UINT16 texturesLumpPos;
	INT32 count = 0;

	// Load patches and textures.

	// Get the number of textures to check.
	// NOTE: Make SURE the system does not process
	// the markers.
	// This system will allocate memory for all duplicate/patched textures even if it never uses them,
	// but the alternative is to spend a ton of time checking and re-checking all previous entries just to skip any potentially patched textures.

#ifdef WALLFLATS
	count += count_range("F_START", "F_END", "flats/", wadnum);
#endif

	// Count the textures from TEXTURES lumps
	texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", wadnum, 0);

	while (texturesLumpPos != INT16_MAX)
	{
		count += R_CountTexturesInTEXTURESLump(wadnum, texturesLumpPos);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", wadnum, texturesLumpPos + 1);
	}

	// Count single-patch textures
	count += count_range(TX_START, TX_END, "textures/", wadnum);

	return count;
}

static void
recallocuser
(		void * user,
		size_t old,
		size_t new)
{
	char *p = Z_Realloc(*(void**)user,
			new, PU_STATIC, user);

	if (new > old)
		memset(&p[old], 0, (new - old));
}

static void R_AllocateTextures(INT32 add)
{
	const INT32 newtextures = (numtextures + add);
	const size_t newsize = newtextures * sizeof (void*);
	const size_t oldsize = numtextures * sizeof (void*);

	INT32 i;

	// Allocate memory and initialize to 0 for all the textures we are initialising.
	recallocuser(&textures, oldsize, newsize);

	// Allocate texture column offset table.
	recallocuser(&texturecolumnofs, oldsize, newsize);
	// Allocate texture referencing cache.
	recallocuser(&texturecache, oldsize, newsize);
	recallocuser(&texturebrightmapcache, oldsize, newsize);
	// Allocate texture width table.
	recallocuser(&texturewidth, oldsize, newsize);
	// Allocate texture height table.
	recallocuser(&textureheight, oldsize, newsize);
	// Create translation table for global animation.
	Z_Realloc(texturetranslation, (newtextures + 1) * sizeof(*texturetranslation), PU_STATIC, &texturetranslation);
	// Create brightmap texture table.
	Z_Realloc(texturebrightmaps, (newtextures + 1) * sizeof(*texturebrightmaps), PU_STATIC, &texturebrightmaps);

	for (i = 0; i < numtextures; ++i)
	{
		// R_FlushTextureCache relies on the user for
		// Z_Free, texturecache has been reallocated so the
		// user is now garbage memory.
		Z_SetUser(texturecache[i], (void**)&texturecache[i]);
		Z_SetUser(texturebrightmapcache[i], (void**)&texturebrightmapcache[i]);
	}

	while (i < newtextures)
	{
		texturetranslation[i] = i;
		texturebrightmaps[i] = 0;
		i++;
	}
}

void R_UpdateTextureBrightmap(INT32 tx, INT32 bm)
{
	I_Assert(tx > 0 && tx < numtextures);
	I_Assert(bm >= 0 && bm < numtextures);

	texturebrightmaps[tx] = bm;
}

static INT32 R_DefineTextures(INT32 i, UINT16 w)
{
#ifdef WALLFLATS
	i = Rloadflats(i, w);
#endif
	return Rloadtextures(i, w);
}

static void R_FinishLoadingTextures(INT32 add)
{
	numtextures += add;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_LoadMapTextures(numtextures);
#endif

	g_texturenum_dbgline = R_CheckTextureNumForName("DBGLINE");
}

//
// R_LoadTextures
// Initializes the texture list with the textures from the world map.
//
void R_LoadTextures(void)
{
	INT32 i, w;
	INT32 newtextures = 0;
#ifdef DEVELOP
	INT32 maintextures = 0;
#endif

	for (w = 0; w < numwadfiles; w++)
	{
		newtextures += R_CountTextures((UINT16)w);
	}

	// If no textures found by this point, bomb out
	if (!newtextures)
		I_Error("No textures detected in any WADs!\n");

	R_AllocateTextures(newtextures);

	for (i = 0, w = 0; w < numwadfiles; w++)
	{
		i = R_DefineTextures(i, w);

#ifdef DEVELOP
		if (w == mainwads)
		{
			maintextures = i;
		}
#endif
	}

	R_FinishLoadingTextures(i);

#ifdef DEVELOP
	R_CheckTextureDuplicates(0, maintextures);
#endif

	R_PrintTextureWarnings();
}

void R_LoadTexturesPwad(UINT16 wadnum)
{
	INT32 newtextures = R_CountTextures(wadnum);

	R_AllocateTextures(newtextures);
	newtextures = R_DefineTextures(numtextures, wadnum) - numtextures;
	R_FinishLoadingTextures(newtextures);

	R_PrintTextureWarnings();
}

static texpatch_t *R_ParsePatch(boolean actuallyLoadPatch)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	char *patchName = NULL;
	INT16 patchXPos;
	INT16 patchYPos;
	UINT8 flip = 0;
	UINT8 alpha = 255;
	patchalphastyle_t style = AST_COPY;
	texpatch_t *resultPatch = NULL;
	lumpnum_t patchLumpNum;

	// Patch identifier
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch name should be");
	}
	texturesTokenLength = strlen(texturesToken);
	if (texturesTokenLength>8)
	{
		I_Error("Error parsing TEXTURES lump: Patch name \"%s\" exceeds 8 characters",texturesToken);
	}
	else
	{
		if (patchName != NULL)
		{
			Z_Free(patchName);
		}
		patchName = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
		M_Memcpy(patchName,texturesToken,texturesTokenLength*sizeof(char));
		patchName[texturesTokenLength] = '\0';
	}

	// Comma 1
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after \"%s\"'s patch name should be",patchName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after %s's patch name, got \"%s\"",patchName,texturesToken);
	}

	// XPos
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s x coordinate should be",patchName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	patchXPos = strtol(texturesToken,&endPos,10);
	(void)patchXPos; //unused for now
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		)
	{
		I_Error("Error parsing TEXTURES lump: Expected an integer for patch \"%s\"'s x coordinate, got \"%s\"",patchName,texturesToken);
	}

	// Comma 2
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after patch \"%s\"'s x coordinate should be",patchName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after patch \"%s\"'s x coordinate, got \"%s\"",patchName,texturesToken);
	}

	// YPos
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s y coordinate should be",patchName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	patchYPos = strtol(texturesToken,&endPos,10);
	(void)patchYPos; //unused for now
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		)
	{
		I_Error("Error parsing TEXTURES lump: Expected an integer for patch \"%s\"'s y coordinate, got \"%s\"",patchName,texturesToken);
	}
	Z_Free(texturesToken);

	// Patch parameters block (OPTIONAL)
	// added by Monster Iestyn (22/10/16)

	// Left Curly Brace
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
		; // move on and ignore, R_ParseTextures will deal with this
	else
	{
		if (strcmp(texturesToken,"{")==0)
		{
			Z_Free(texturesToken);
			texturesToken = M_GetToken(NULL);
			if (texturesToken == NULL)
			{
				I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s parameters should be",patchName);
			}
			while (strcmp(texturesToken,"}")!=0)
			{
				if (stricmp(texturesToken, "ALPHA")==0)
				{
					Z_Free(texturesToken);
					texturesToken = M_GetToken(NULL);
					alpha = 255*strtof(texturesToken, NULL);
				}
				else if (stricmp(texturesToken, "STYLE")==0)
				{
					Z_Free(texturesToken);
					texturesToken = M_GetToken(NULL);
					if (stricmp(texturesToken, "TRANSLUCENT")==0)
						style = AST_TRANSLUCENT;
					else if (stricmp(texturesToken, "ADD")==0)
						style = AST_ADD;
					else if (stricmp(texturesToken, "SUBTRACT")==0)
						style = AST_SUBTRACT;
					else if (stricmp(texturesToken, "REVERSESUBTRACT")==0)
						style = AST_REVERSESUBTRACT;
					else if (stricmp(texturesToken, "MODULATE")==0)
						style = AST_MODULATE;
				}
				else if (stricmp(texturesToken, "FLIPX")==0)
					flip |= 1;
				else if (stricmp(texturesToken, "FLIPY")==0)
					flip |= 2;
				Z_Free(texturesToken);

				texturesToken = M_GetToken(NULL);
				if (texturesToken == NULL)
				{
					I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s parameters or right curly brace should be",patchName);
				}
			}
		}
		else
		{
			 // this is not what we wanted...
			 // undo last read so R_ParseTextures can re-get the token for its own purposes
			M_UnGetToken();
		}
		Z_Free(texturesToken);
	}

	if (actuallyLoadPatch == true)
	{
		// Check lump exists
		patchLumpNum = W_GetNumForName(patchName);
		// If so, allocate memory for texpatch_t and fill 'er up
		resultPatch = (texpatch_t *)Z_Malloc(sizeof(texpatch_t),PU_STATIC,NULL);
		resultPatch->originx = patchXPos;
		resultPatch->originy = patchYPos;
		resultPatch->lump = patchLumpNum & 65535;
		resultPatch->wad = patchLumpNum>>16;
		resultPatch->flip = flip;
		resultPatch->alpha = alpha;
		resultPatch->style = style;
		// Clean up a little after ourselves
		Z_Free(patchName);
		// Then return it
		return resultPatch;
	}
	else
	{
		Z_Free(patchName);
		return NULL;
	}
}

static texture_t *R_ParseTexture(boolean actuallyLoadTexture)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	INT32 newTextureWidth;
	INT32 newTextureHeight;
	texture_t *resultTexture = NULL;
	texpatch_t *newPatch;
	char newTextureName[9]; // no longer dynamically allocated

	// Texture name
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture name should be");
	}
	texturesTokenLength = strlen(texturesToken);
	if (texturesTokenLength>8)
	{
		I_Error("Error parsing TEXTURES lump: Texture name \"%s\" exceeds 8 characters",texturesToken);
	}
	else
	{
		memset(&newTextureName, 0, 9);
		M_Memcpy(newTextureName, texturesToken, texturesTokenLength);
		// ^^ we've confirmed that the token is <= 8 characters so it will never overflow a 9 byte char buffer
		strupr(newTextureName); // Just do this now so we don't have to worry about it
	}
	Z_Free(texturesToken);

	// Comma 1
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after texture \"%s\"'s name should be",newTextureName);
	}
	else if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after texture \"%s\"'s name, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Width
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture \"%s\"'s width should be",newTextureName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	newTextureWidth = strtol(texturesToken,&endPos,10);
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		|| newTextureWidth < 0) // Number is not positive
	{
		I_Error("Error parsing TEXTURES lump: Expected a positive integer for texture \"%s\"'s width, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Comma 2
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after texture \"%s\"'s width should be",newTextureName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after texture \"%s\"'s width, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Height
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture \"%s\"'s height should be",newTextureName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	newTextureHeight = strtol(texturesToken,&endPos,10);
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		|| newTextureHeight < 0) // Number is not positive
	{
		I_Error("Error parsing TEXTURES lump: Expected a positive integer for texture \"%s\"'s height, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Left Curly Brace
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where open curly brace for texture \"%s\" should be",newTextureName);
	}
	if (strcmp(texturesToken,"{")==0)
	{
		if (actuallyLoadTexture)
		{
			// Allocate memory for a zero-patch texture. Obviously, we'll be adding patches momentarily.
			resultTexture = (texture_t *)Z_Calloc(sizeof(texture_t),PU_STATIC,NULL);
			M_Memcpy(resultTexture->name, newTextureName, 8);
			resultTexture->hash = quickncasehash(newTextureName, 8);
			resultTexture->width = newTextureWidth;
			resultTexture->height = newTextureHeight;
			resultTexture->type = TEXTURETYPE_COMPOSITE;
			resultTexture->terrainID = K_GetTerrainIDForTextureName(newTextureName);
		}
		Z_Free(texturesToken);
		texturesToken = M_GetToken(NULL);
		if (texturesToken == NULL)
		{
			I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch definition for texture \"%s\" should be",newTextureName);
		}
		while (strcmp(texturesToken,"}")!=0)
		{
			if (stricmp(texturesToken, "PATCH")==0)
			{
				Z_Free(texturesToken);
				if (resultTexture)
				{
					// Get that new patch
					newPatch = R_ParsePatch(true);
					// Make room for the new patch
					resultTexture = Z_Realloc(resultTexture, sizeof(texture_t) + (resultTexture->patchcount+1)*sizeof(texpatch_t), PU_STATIC, NULL);
					// Populate the uninitialized values in the new patch entry of our array
					M_Memcpy(&resultTexture->patches[resultTexture->patchcount], newPatch, sizeof(texpatch_t));
					// Account for the new number of patches in the texture
					resultTexture->patchcount++;
					// Then free up the memory assigned to R_ParsePatch, as it's unneeded now
					Z_Free(newPatch);
				}
				else
				{
					R_ParsePatch(false);
				}
			}
			else
			{
				I_Error("Error parsing TEXTURES lump: Expected \"PATCH\" in texture \"%s\", got \"%s\"",newTextureName,texturesToken);
			}

			texturesToken = M_GetToken(NULL);
			if (texturesToken == NULL)
			{
				I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch declaration or right curly brace for texture \"%s\" should be",newTextureName);
			}
		}
		if (resultTexture && resultTexture->patchcount == 0)
		{
			I_Error("Error parsing TEXTURES lump: Texture \"%s\" must have at least one patch",newTextureName);
		}
	}
	else
	{
		I_Error("Error parsing TEXTURES lump: Expected \"{\" for texture \"%s\", got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	if (actuallyLoadTexture) return resultTexture;
	else return NULL;
}

// Parses the TEXTURES lump... but just to count the number of textures.
int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	UINT32 numTexturesInLump = 0;
	char *texturesToken;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	texturesLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (texturesLump == NULL) return 0;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	texturesLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	texturesText = (char *)Z_Malloc((texturesLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(texturesText,texturesLump,texturesLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	texturesText[texturesLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(texturesLump);

	texturesToken = M_GetToken(texturesText);
	while (texturesToken != NULL)
	{
		if (stricmp(texturesToken, "WALLTEXTURE") == 0 || stricmp(texturesToken, "TEXTURE") == 0)
		{
			numTexturesInLump++;
			Z_Free(texturesToken);
			R_ParseTexture(false);
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\" or \"TEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);

	return numTexturesInLump;
}

// Parses the TEXTURES lump... for real, this time.
void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *texindex)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	char *texturesToken;
	texture_t *newTexture;

	I_Assert(texindex != NULL);

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	texturesLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (texturesLump == NULL) return;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	texturesLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	texturesText = (char *)Z_Malloc((texturesLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(texturesText,texturesLump,texturesLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	texturesText[texturesLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(texturesLump);

	texturesToken = M_GetToken(texturesText);
	while (texturesToken != NULL)
	{
		if (stricmp(texturesToken, "WALLTEXTURE") == 0 || stricmp(texturesToken, "TEXTURE") == 0)
		{
			Z_Free(texturesToken);
			// Get the new texture
			newTexture = R_ParseTexture(true);
			// Store the new texture
			textures[*texindex] = newTexture;
			texturewidth[*texindex] = newTexture->width;
			textureheight[*texindex] = newTexture->height << FRACBITS;
			// Increment i back in R_LoadTextures()
			(*texindex)++;
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\" or \"TEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);
}

// Search for flat name.
lumpnum_t R_GetFlatNumForName(const char *name)
{
	INT32 i;
	lumpnum_t lump;
	lumpnum_t start;
	lumpnum_t end;

	// Scan wad files backwards so patched flats take preference.
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		switch (wadfiles[i]->type)
		{
		case RET_WAD:
			if ((start = W_CheckNumForMarkerStartPwad("F_START", (UINT16)i, 0)) == INT16_MAX)
			{
				if ((start = W_CheckNumForMarkerStartPwad("FF_START", (UINT16)i, 0)) == INT16_MAX)
					continue;
				else if ((end = W_CheckNumForNamePwad("FF_END", (UINT16)i, start)) == INT16_MAX)
					continue;
			}
			else
				if ((end = W_CheckNumForNamePwad("F_END", (UINT16)i, start)) == INT16_MAX)
					continue;
			break;
		case RET_PK3:
			if ((start = W_CheckNumForFolderStartPK3("Flats/", i, 0)) == INT16_MAX)
				continue;
			if ((end = W_CheckNumForFolderEndPK3("Flats/", i, start)) == INT16_MAX)
				continue;
			break;
		default:
			continue;
		}

		// Now find lump with specified name in that range.
		lump = W_CheckNumForNamePwad(name, (UINT16)i, start);
		if (lump < end)
		{
			lump += (i<<16); // found it, in our constraints
			break;
		}
		lump = LUMPERROR;
	}

	return lump;
}

void R_ClearTextureNumCache(boolean btell)
{
	if (tidcache)
		Z_Free(tidcache);
	tidcache = NULL;
	if (btell)
		CONS_Debug(DBG_SETUP, "Fun Fact: There are %d textures used in this map.\n", tidcachelen);
	tidcachelen = 0;
}

//
// R_CheckTextureNumForName
//
// Check whether texture is available. Filter out NoTexture indicator.
//
INT32 R_CheckTextureNumForName(const char *name)
{
	INT32 i;
	UINT32 hash;

	// "NoTexture" marker.
	if (name[0] == '-')
		return 0;

	hash = quickncasehash(name, 8);

	for (i = 0; i < tidcachelen; i++)
		if (tidcache[i].hash == hash && !strncasecmp(tidcache[i].name, name, 8))
			return tidcache[i].id;

	// Need to parse the list backwards, so textures loaded more recently are used in lieu of ones loaded earlier
	//for (i = 0; i < numtextures; i++) <- old
	for (i = (numtextures - 1); i >= 0; i--) // <- new
		if (textures[i]->hash == hash && !strncasecmp(textures[i]->name, name, 8))
		{
			tidcachelen++;
			Z_Realloc(tidcache, tidcachelen * sizeof(*tidcache), PU_STATIC, &tidcache);
			strncpy(tidcache[tidcachelen-1].name, name, 8);
			tidcache[tidcachelen-1].name[8] = '\0';
			CONS_Debug(DBG_SETUP, "texture #%s: %s\n", sizeu1(tidcachelen), tidcache[tidcachelen-1].name);
			tidcache[tidcachelen-1].hash = hash;
			tidcache[tidcachelen-1].id = i;
			return i;
		}

	return -1;
}

//
// R_TextureNumForName
//
// Calls R_CheckTextureNumForName, aborts with error message.
//
INT32 R_TextureNumForName(const char *name)
{
	const INT32 i = R_CheckTextureNumForName(name);

	if (i == -1)
	{
		static INT32 redwall = -2;
		CONS_Debug(DBG_SETUP, "WARNING: R_TextureNumForName: %.8s not found\n", name);
		if (redwall == -2)
			redwall = R_CheckTextureNumForName(MISSING_TEXTURE);
		if (redwall != -1)
			return redwall;
		return 1;
	}
	return i;
}
