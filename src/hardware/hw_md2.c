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
/// \file hw_md2.c
/// \brief 3D Model Handling
///	Inspired from md2.c by Mete Ciragan (mete@swissquake.ch)

#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../d_main.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../fastcmp.h"

#ifdef HWRENDER
#include "hw_drv.h"
#include "hw_light.h"
#include "hw_md2.h"
#include "../d_main.h"
#include "../r_bsp.h"
#include "../r_fps.h"
#include "../r_main.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_things.h"
#include "../r_draw.h"
#include "../p_tick.h"

#include "hw_model.h"

#include "hw_main.h"
#include "../v_video.h"

// SRB2Kart
#include "../k_color.h"
#include "../k_hitlag.h" // HITLAGJITTERS
#include "../r_fps.h"

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "png.h"
 #ifndef PNG_READ_SUPPORTED
 #undef HAVE_PNG
 #endif
 #if PNG_LIBPNG_VER < 100207
 //#undef HAVE_PNG
 #endif
#endif

#ifndef errno
#include "errno.h"
#endif

md2_t md2_models[NUMSPRITES];
md2_t md2_playermodels[MAXSKINS];


/*
 * free model
 */
#if 0
static void md2_freeModel (model_t *model)
{
	UnloadModel(model);
}
#endif

//
// load model
//
// Hurdler: the current path is the Legacy.exe path
static model_t *md2_readModel(const char *filename)
{
	//Filename checking fixed ~Monster Iestyn and Golden
	if (FIL_FileExists(va("%s"PATHSEP"%s", srb2home, filename)))
		return LoadModel(va("%s"PATHSEP"%s", srb2home, filename), PU_STATIC);

	if (FIL_FileExists(va("%s"PATHSEP"%s", srb2path, filename)))
		return LoadModel(va("%s"PATHSEP"%s", srb2path, filename), PU_STATIC);

	return NULL;
}

static inline void md2_printModelInfo (model_t *model)
{
#if 0
	INT32 i;

	CONS_Debug(DBG_RENDER, "magic:\t\t\t%c%c%c%c\n", model->header.magic>>24,
	            (model->header.magic>>16)&0xff,
	            (model->header.magic>>8)&0xff,
	             model->header.magic&0xff);
	CONS_Debug(DBG_RENDER, "version:\t\t%d\n", model->header.version);
	CONS_Debug(DBG_RENDER, "skinWidth:\t\t%d\n", model->header.skinWidth);
	CONS_Debug(DBG_RENDER, "skinHeight:\t\t%d\n", model->header.skinHeight);
	CONS_Debug(DBG_RENDER, "frameSize:\t\t%d\n", model->header.frameSize);
	CONS_Debug(DBG_RENDER, "numSkins:\t\t%d\n", model->header.numSkins);
	CONS_Debug(DBG_RENDER, "numVertices:\t\t%d\n", model->header.numVertices);
	CONS_Debug(DBG_RENDER, "numTexCoords:\t\t%d\n", model->header.numTexCoords);
	CONS_Debug(DBG_RENDER, "numTriangles:\t\t%d\n", model->header.numTriangles);
	CONS_Debug(DBG_RENDER, "numGlCommands:\t\t%d\n", model->header.numGlCommands);
	CONS_Debug(DBG_RENDER, "numFrames:\t\t%d\n", model->header.numFrames);
	CONS_Debug(DBG_RENDER, "offsetSkins:\t\t%d\n", model->header.offsetSkins);
	CONS_Debug(DBG_RENDER, "offsetTexCoords:\t%d\n", model->header.offsetTexCoords);
	CONS_Debug(DBG_RENDER, "offsetTriangles:\t%d\n", model->header.offsetTriangles);
	CONS_Debug(DBG_RENDER, "offsetFrames:\t\t%d\n", model->header.offsetFrames);
	CONS_Debug(DBG_RENDER, "offsetGlCommands:\t%d\n", model->header.offsetGlCommands);
	CONS_Debug(DBG_RENDER, "offsetEnd:\t\t%d\n", model->header.offsetEnd);

	for (i = 0; i < model->header.numFrames; i++)
		CONS_Debug(DBG_RENDER, "%s ", model->frames[i].name);
	CONS_Debug(DBG_RENDER, "\n");
#else
	(void)model;
#endif
}

#ifdef HAVE_PNG
static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng error at %p: %s", (void*)PNG, pngtext);
	//I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", (void*)PNG, pngtext);
}

static GLTextureFormat_t PNG_Load(const char *filename, int *w, int *h, GLPatch_t *grpatch)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	volatile png_FILE_p png_FILE;
	//Filename checking fixed ~Monster Iestyn and Golden
	char *pngfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2home, filename);

	FIL_ForceExtension(pngfilename, ".png");
	png_FILE = fopen(pngfilename, "rb");
	if (!png_FILE)
	{
		pngfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2path, filename);
		FIL_ForceExtension(pngfilename, ".png");
		png_FILE = fopen(pngfilename, "rb");
		//CONS_Debug(DBG_RENDER, "M_SavePNG: Error on opening %s for loading\n", filename);
		if (!png_FILE)
			return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on initialize libpng\n");
		fclose(png_FILE);
		return 0;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(png_FILE);
		return 0;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng load error on %s\n", filename);
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		fclose(png_FILE);
		Z_Free(grpatch->mipmap->data);
		return 0;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);

	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type,
	 NULL, NULL, NULL);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);

	{
		png_uint_32 i, pitch = png_get_rowbytes(png_ptr, png_info_ptr);
		png_bytep PNG_image = Z_Malloc(pitch*height, PU_HWRMODELTEXTURE, &grpatch->mipmap->data);
		png_bytepp row_pointers = png_malloc(png_ptr, height * sizeof (png_bytep));
		for (i = 0; i < height; i++)
			row_pointers[i] = PNG_image + i*pitch;
		png_read_image(png_ptr, row_pointers);
		png_free(png_ptr, (png_voidp)row_pointers);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	fclose(png_FILE);
	*w = (int)width;
	*h = (int)height;
	return GL_TEXFMT_RGBA;
}
#endif

typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bitsPerPixel;
	INT16 xmin;
	INT16 ymin;
	INT16 xmax;
	INT16 ymax;
	INT16 hDpi;
	INT16 vDpi;
	UINT8 colorMap[48];
	UINT8 reserved;
	UINT8 numPlanes;
	INT16 bytesPerLine;
	INT16 paletteInfo;
	INT16 hScreenSize;
	INT16 vScreenSize;
	UINT8 filler[54];
} PcxHeader;

static GLTextureFormat_t PCX_Load(const char *filename, int *w, int *h,
	GLPatch_t *grpatch)
{
	PcxHeader header;
#define PALSIZE 768
	UINT8 palette[PALSIZE];
	const UINT8 *pal;
	RGBA_t *image;
	size_t pw, ph, size, ptr = 0;
	INT32 ch, rep;
	FILE *file;
	//Filename checking fixed ~Monster Iestyn and Golden
	char *pcxfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2home, filename);

	FIL_ForceExtension(pcxfilename, ".pcx");
	file = fopen(pcxfilename, "rb");
	if (!file)
	{
		pcxfilename = va("%s"PATHSEP"models"PATHSEP"%s", srb2path, filename);
		FIL_ForceExtension(pcxfilename, ".pcx");
		file = fopen(pcxfilename, "rb");
		if (!file)
			return 0;
	}

	if (fread(&header, sizeof (PcxHeader), 1, file) != 1)
	{
		fclose(file);
		return 0;
	}

	if (header.bitsPerPixel != 8)
	{
		fclose(file);
		return 0;
	}

	fseek(file, -PALSIZE, SEEK_END);

	pw = *w = header.xmax - header.xmin + 1;
	ph = *h = header.ymax - header.ymin + 1;
	image = Z_Malloc(pw*ph*4, PU_HWRMODELTEXTURE, &grpatch->mipmap->data);

	if (fread(palette, sizeof (UINT8), PALSIZE, file) != PALSIZE)
	{
		Z_Free(image);
		fclose(file);
		return 0;
	}
	fseek(file, sizeof (PcxHeader), SEEK_SET);

	size = pw * ph;
	while (ptr < size)
	{
		ch = fgetc(file);  //Hurdler: beurk
		if (ch >= 192)
		{
			rep = ch - 192;
			ch = fgetc(file);
		}
		else
		{
			rep = 1;
		}
		while (rep--)
		{
			pal = palette + ch*3;
			image[ptr].s.red   = *pal++;
			image[ptr].s.green = *pal++;
			image[ptr].s.blue  = *pal++;
			image[ptr].s.alpha = 0xFF;
			ptr++;
		}
	}
	fclose(file);
	return GL_TEXFMT_RGBA;
}

// -----------------+
// md2_loadTexture  : Download a pcx or png texture for models
// -----------------+
static void md2_loadTexture(md2_t *model)
{
	patch_t *patch;
	GLPatch_t *grPatch = NULL;
	const char *filename = model->filename;

	if (model->grpatch)
	{
		patch = model->grpatch;
		grPatch = (GLPatch_t *)(patch->hardware);
		if (grPatch)
			Z_Free(grPatch->mipmap->data);
	}
	else
		model->grpatch = patch = Patch_Create(NULL, 0, NULL);

	if (!patch->hardware)
		Patch_AllocateHardwarePatch(patch);

	if (grPatch == NULL)
		grPatch = (GLPatch_t *)(patch->hardware);

	if (!grPatch->mipmap->downloaded && !grPatch->mipmap->data)
	{
		int w = 0, h = 0;
		UINT32 size;
		RGBA_t *image;

#ifdef HAVE_PNG
		grPatch->mipmap->format = PNG_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
#endif
		grPatch->mipmap->format = PCX_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
		{
			model->notexturefile = true; // mark it so its not searched for again repeatedly
			return;
		}

		grPatch->mipmap->downloaded = 0;
		grPatch->mipmap->flags = 0;

		patch->width = (INT16)w;
		patch->height = (INT16)h;
		grPatch->mipmap->width = (UINT16)w;
		grPatch->mipmap->height = (UINT16)h;

		// Lactozilla: Apply colour cube
		image = grPatch->mipmap->data;
		size = w*h;
		while (size--)
		{
			V_CubeApply(image);
			image++;
		}
	}

	HWD.pfnSetTexture(grPatch->mipmap);
}

// -----------------+
// md2_loadBlendTexture  : Download a pcx or png texture for blending MD2 models
// -----------------+
static void md2_loadBlendTexture(md2_t *model)
{
	patch_t *patch;
	GLPatch_t *grPatch = NULL;
	char *filename = Z_Malloc(strlen(model->filename)+7, PU_STATIC, NULL);

	strcpy(filename, model->filename);
	FIL_ForceExtension(filename, "_blend.png");

	if (model->blendgrpatch)
	{
		patch = model->blendgrpatch;
		grPatch = (GLPatch_t *)(patch->hardware);
		if (grPatch)
			Z_Free(grPatch->mipmap->data);
	}
	else
		model->blendgrpatch = patch = Patch_Create(NULL, 0, NULL);

	if (!patch->hardware)
		Patch_AllocateHardwarePatch(patch);

	if (grPatch == NULL)
		grPatch = (GLPatch_t *)(patch->hardware);

	if (!grPatch->mipmap->downloaded && !grPatch->mipmap->data)
	{
		int w = 0, h = 0;
#ifdef HAVE_PNG
		grPatch->mipmap->format = PNG_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
#endif
		grPatch->mipmap->format = PCX_Load(filename, &w, &h, grPatch);
		if (grPatch->mipmap->format == 0)
		{
			model->noblendfile = true; // mark it so its not searched for again repeatedly
			Z_Free(filename);
			return;
		}

		grPatch->mipmap->downloaded = 0;
		grPatch->mipmap->flags = 0;

		patch->width = (INT16)w;
		patch->height = (INT16)h;
		grPatch->mipmap->width = (UINT16)w;
		grPatch->mipmap->height = (UINT16)h;
	}

	HWD.pfnSetTexture(grPatch->mipmap); // We do need to do this so that it can be cleared and knows to recreate it when necessary

	Z_Free(filename);
}

// Don't spam the console, or the OS with fopen requests!
static boolean nomd2s = false;

void HWR_InitModels(void)
{
	size_t i;
	INT32 s;
	FILE *f;
	char name[26], filename[32];
	// name[24] is used to check for names in the models.dat file that match with sprites or player skins
	// sprite names are always 4 characters long, and names is for player skins can be up to 19 characters long
	// PLAYERMODELPREFIX is 6 characters long
	float scale, offset;
	size_t prefixlen;

	CONS_Printf("HWR_InitModels()...\n");
	for (s = 0; s < MAXSKINS; s++)
	{
		md2_playermodels[s].scale = -1.0f;
		md2_playermodels[s].model = NULL;
		md2_playermodels[s].grpatch = NULL;
		md2_playermodels[s].notexturefile = false;
		md2_playermodels[s].noblendfile = false;
		md2_playermodels[s].skin = -1;
		md2_playermodels[s].notfound = true;
		md2_playermodels[s].error = false;
	}
	for (i = 0; i < NUMSPRITES; i++)
	{
		md2_models[i].scale = -1.0f;
		md2_models[i].model = NULL;
		md2_models[i].grpatch = NULL;
		md2_models[i].notexturefile = false;
		md2_models[i].noblendfile = false;
		md2_models[i].skin = -1;
		md2_models[i].notfound = true;
		md2_models[i].error = false;
	}

	// read the models.dat file
	//Filename checking fixed ~Monster Iestyn and Golden
	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		f = fopen(va("%s"PATHSEP"%s", srb2path, "models.dat"), "rt");
		if (!f)
		{
			CONS_Printf("%s %s\n", M_GetText("Error while loading models.dat:"), strerror(errno));
			nomd2s = true;
			return;
		}
	}

	// length of the player model prefix
	prefixlen = strlen(PLAYERMODELPREFIX);

	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		char *skinname = name;
		size_t len = strlen(name);

		// check for the player model prefix.
		if (!strnicmp(name, PLAYERMODELPREFIX, prefixlen) && (len > prefixlen))
		{
			skinname += prefixlen;
			goto addskinmodel;
		}

		// add sprite model
		if (len == 4) // must be 4 characters long exactly. otherwise it's not a sprite name.
		{
			for (i = 0; i < NUMSPRITES; i++)
			{
				if (stricmp(name, sprnames[i]) == 0)
				{
					md2_models[i].scale = scale;
					md2_models[i].offset = offset;
					md2_models[i].notfound = false;
					strcpy(md2_models[i].filename, filename);
					goto modelfound;
				}
			}
		}

addskinmodel:
		// add player model
		s = R_SkinAvailableEx(skinname, false);
		if (s != -1)
		{
			md2_playermodels[s].skin = s;
			md2_playermodels[s].scale = scale;
			md2_playermodels[s].offset = offset;
			md2_playermodels[s].notfound = false;
			strcpy(md2_playermodels[s].filename, filename);
			goto modelfound;
		}

		// no sprite/player skin name found?!?D
		CONS_Printf("Unknown sprite/player skin %s detected in models.dat\n", name);

modelfound:
		// move on to next line...
		continue;
	}
	fclose(f);
}

void HWR_AddPlayerModel(INT32 skin) // For skins that were added after startup
{
	FILE *f;
	char name[24], filename[32];
	float scale, offset;
	size_t prefixlen;

	if (nomd2s)
		return;

	//CONS_Printf("HWR_AddPlayerModel()...\n");

	// read the models.dat file
	//Filename checking fixed ~Monster Iestyn and Golden
	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		f = fopen(va("%s"PATHSEP"%s", srb2path, "models.dat"), "rt");
		if (!f)
		{
			CONS_Printf("%s %s\n", M_GetText("Error while loading models.dat:"), strerror(errno));
			nomd2s = true;
			return;
		}
	}

	// length of the player model prefix
	prefixlen = strlen(PLAYERMODELPREFIX);

	// Check for any models that match the names of player skins!
	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		char *skinname = name;
		size_t len = strlen(name);

		// ignore the player model prefix.
		if (!strnicmp(name, PLAYERMODELPREFIX, prefixlen) && (len > prefixlen))
			skinname += prefixlen;

		if (stricmp(skinname, skins[skin]->name) == 0)
		{
			md2_playermodels[skin].skin = skin;
			md2_playermodels[skin].scale = scale;
			md2_playermodels[skin].offset = offset;
			md2_playermodels[skin].notfound = false;
			strcpy(md2_playermodels[skin].filename, filename);
			goto playermodelfound;
		}
	}

	md2_playermodels[skin].notfound = true;
playermodelfound:
	fclose(f);
}

void HWR_AddSpriteModel(size_t spritenum) // For sprites that were added after startup
{
	FILE *f;
	// name[24] is used to check for names in the models.dat file that match with sprites or player skins
	// sprite names are always 4 characters long, and names is for player skins can be up to 19 characters long
	// PLAYERMODELPREFIX is 6 characters long
	char name[24], filename[32];
	float scale, offset;

	if (nomd2s)
		return;

	if (spritenum == SPR_PLAY) // Handled already NEWMD2: Per sprite, per-skin check
		return;

	// Read the models.dat file
	//Filename checking fixed ~Monster Iestyn and Golden
	f = fopen(va("%s"PATHSEP"%s", srb2home, "models.dat"), "rt");

	if (!f)
	{
		f = fopen(va("%s"PATHSEP"%s", srb2path, "models.dat"), "rt");
		if (!f)
		{
			CONS_Printf("%s %s\n", M_GetText("Error while loading models.dat:"), strerror(errno));
			nomd2s = true;
			return;
		}
	}

	// Check for any models that match the names of sprite names!
	while (fscanf(f, "%25s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		// length of the sprite name
		size_t len = strlen(name);
		if (len != 4) // must be 4 characters long exactly. otherwise it's not a sprite name.
			continue;

		// check for the player model prefix.
		if (!strnicmp(name, PLAYERMODELPREFIX, strlen(PLAYERMODELPREFIX)))
			continue; // that's not a sprite...

		if (stricmp(name, sprnames[spritenum]) == 0)
		{
			md2_models[spritenum].scale = scale;
			md2_models[spritenum].offset = offset;
			md2_models[spritenum].notfound = false;
			strcpy(md2_models[spritenum].filename, filename);
			goto spritemodelfound;
		}
	}

	md2_models[spritenum].notfound = true;
spritemodelfound:
	fclose(f);
}

//#define BlendLuminance (K_ColorRelativeLuminance(r, g, b))
#define BlendLuminance(r, g, b) ((UINT8)((r * 0.2126) + (g * 0.7152) + (b * 0.0722)))

static void HWR_CreateBlendedTexture(patch_t *gpatch, patch_t *blendgpatch, GLMipmap_t *grMipmap, INT32 skinnum, skincolornum_t color)
{
	GLPatch_t *hwrPatch = gpatch->hardware;
	GLPatch_t *hwrBlendPatch = blendgpatch->hardware;
	UINT16 w = gpatch->width, h = gpatch->height;
	UINT32 size = w*h;
	RGBA_t *image, *blendimage, *cur, blendcolor;
	UINT8 translation[17]; // First the color index
	UINT8 cutoff[17]; // Brightness cutoff before using the next color
	UINT8 translen = 0;
	UINT8 i;
	UINT8 colorbrightnesses[17];
	UINT8 color_match_lookup[256]; // optimization attempt

	blendcolor = V_GetColor(0); // initialize
	memset(translation, 0, sizeof(translation));
	memset(cutoff, 0, sizeof(cutoff));

	if (grMipmap->width == 0)
	{
		grMipmap->width = gpatch->width;
		grMipmap->height = gpatch->height;

		// no wrap around, no chroma key
		grMipmap->flags = 0;

		// setup the texture info
		grMipmap->format = GL_TEXFMT_RGBA;
	}

	if (grMipmap->data)
	{
		Z_Free(grMipmap->data);
		grMipmap->data = NULL;
	}

	cur = Z_Malloc(size*4, PU_HWRMODELTEXTURE, &grMipmap->data);
	memset(cur, 0x00, size*4);

	image = hwrPatch->mipmap->data;
	blendimage = hwrBlendPatch->mipmap->data;

	// TC_METALSONIC includes an actual skincolor translation, on top of its flashing.
	if (skinnum == TC_METALSONIC)
		color = SKINCOLOR_MIDNIGHT;

	if (color != SKINCOLOR_NONE && color < numskincolors)
	{
		UINT8 numdupes = 1;
		UINT8 prevdupes = numdupes;

		translation[translen] = skincolors[color].ramp[0];
		cutoff[translen] = 255;

		for (i = 1; i < 16; i++)
		{
			if (translation[translen] == skincolors[color].ramp[i])
			{
				numdupes++;
				continue;
			}

			if (translen > 0)
			{
				INT16 newcutoff = cutoff[translen-1] - (255 / (16 / prevdupes));

				if (newcutoff < 0)
					newcutoff = 0;

				cutoff[translen] = (UINT8)newcutoff;
			}

			prevdupes = numdupes;
			numdupes = 1;
			translen++;

			translation[translen] = (UINT8)skincolors[color].ramp[i];
		}

		translen++;
	}

	if (translen > 0)
		translation[translen] = translation[translen-1]; // extended to accomodate secondi if firsti equal to translen-1
	if (translen > 1)
		cutoff[translen] = cutoff[translen-1] = 0; // as above

	if (skinnum == TC_RAINBOW && translen > 0)
	{
		UINT16 b;
		INT32 compare;

		for (i = 0; i < translen; i++) // moved from inside the loop to here
		{
			RGBA_t tempc = V_GetColor(translation[i]);
			colorbrightnesses[i] = BlendLuminance(tempc.s.red, tempc.s.green, tempc.s.blue); // store brightnesses for comparison
		}

		// generate lookup table for color brightness matching
		for (b = 0; b < 256; b++)
		{
			UINT16 brightdif = 256;

			color_match_lookup[b] = 0;
			for (i = 0; i < translen; i++)
			{
				if (b > colorbrightnesses[i]) // don't allow greater matches (because calculating a makeshift gradient for this is already a huge mess as is)
					continue;

				compare = abs((INT16)(colorbrightnesses[i]) - (INT16)(b));

				if (compare < brightdif)
				{
					brightdif = (UINT16)compare;
					color_match_lookup[b] = i; // best matching color that's equal brightness or darker
				}
			}
		}

		colorbrightnesses[translen] = colorbrightnesses[translen-1];
	}

	if (skinnum == TC_BLINK)
		blendcolor = V_GetColor(skincolors[color].ramp[3]);

	while (size--)
	{
		if (skinnum == TC_HITLAG)
		{
			cur->s.red = cur->s.green = cur->s.blue = K_HitlagColorValue(*image);
			cur->s.alpha = image->s.alpha;
		}
		else if (skinnum == TC_BOSS)
		{
			// Turn everything below a certain threshold white
			if ((image->s.red == image->s.green) && (image->s.green == image->s.blue) && image->s.blue < 127)
			{
				// Lactozilla: Invert the colors
				cur->s.red = cur->s.green = cur->s.blue = (255 - image->s.blue);
			}
			else
			{
				cur->s.red = image->s.red;
				cur->s.green = image->s.green;
				cur->s.blue = image->s.blue;
			}

			cur->s.alpha = image->s.alpha;
		}
		else if (skinnum == TC_ALLWHITE)
		{
			// Turn everything white
			cur->s.red = cur->s.green = cur->s.blue = 255;
			cur->s.alpha = image->s.alpha;
		}
		else if (skinnum == TC_BLINK)
		{
			*cur = blendcolor;
			cur->s.alpha = image->s.alpha;
		}
		else
		{
			// Everything below requires a blend image
			if (blendimage == NULL)
			{
				cur->rgba = image->rgba;
				goto skippixel;
			}

			// Metal Sonic dash mode
			if (skinnum == TC_DASHMODE)
			{
				if (image->s.alpha == 0 && blendimage->s.alpha == 0)
				{
					// Don't bother with blending the pixel if the alpha of the blend pixel is 0
					cur->rgba = image->rgba;
				}
				else
				{
					UINT8 ialpha = 255 - blendimage->s.alpha, balpha = blendimage->s.alpha;
					RGBA_t icolor = *image, bcolor;

					memset(&bcolor, 0x00, sizeof(RGBA_t));

					if (blendimage->s.alpha)
					{
						bcolor.s.blue = 0;
						bcolor.s.red = 255;
						bcolor.s.green = (blendimage->s.red + blendimage->s.green + blendimage->s.blue) / 3;
					}

					if (image->s.alpha && image->s.red > image->s.green << 1) // this is pretty arbitrary, but it works well for Metal Sonic
					{
						icolor.s.red = image->s.blue;
						icolor.s.blue = image->s.red;
					}

					cur->s.red = (ialpha * icolor.s.red + balpha * bcolor.s.red)/255;
					cur->s.green = (ialpha * icolor.s.green + balpha * bcolor.s.green)/255;
					cur->s.blue = (ialpha * icolor.s.blue + balpha * bcolor.s.blue)/255;
					cur->s.alpha = image->s.alpha;
				}
			}
			else
			{
				// All settings that use skincolors!
				UINT16 brightness;

				if (translen <= 0)
				{
					cur->rgba = image->rgba;
					goto skippixel;
				}

				// Don't bother with blending the pixel if the alpha of the blend pixel is 0
				if (skinnum == TC_RAINBOW)
				{
					if (image->s.alpha == 0 && blendimage->s.alpha == 0)
					{
						cur->rgba = image->rgba;
						goto skippixel;
					}
					else
					{
						UINT16 imagebright, blendbright;

						imagebright = BlendLuminance(image->s.red, image->s.green, image->s.blue);
						blendbright = BlendLuminance(blendimage->s.red, blendimage->s.green, blendimage->s.blue);

						// slightly dumb average between the blend image color and base image colour, usually one or the other will be fully opaque anyway
						brightness = (imagebright*(255-blendimage->s.alpha))/255 + (blendbright*blendimage->s.alpha)/255;
					}
				}
				else
				{
					if (blendimage->s.alpha == 0)
					{
						cur->rgba = image->rgba;
						goto skippixel; // for metal sonic blend
					}
					else
					{
						brightness = BlendLuminance(blendimage->s.red, blendimage->s.green, blendimage->s.blue);
					}
				}

				// Calculate a sort of "gradient" for the skincolor
				// (Me splitting this into a function didn't work, so I had to ruin this entire function's groove...)
				{
					RGBA_t nextcolor;
					UINT8 firsti, secondi, mul, mulmax;
					INT32 r, g, b;

					// Rainbow needs to find the closest match to the textures themselves, instead of matching brightnesses to other colors.
					// Ensue horrible mess.
					if (skinnum == TC_RAINBOW)
					{
						//UINT16 brightdif = 256;
						INT32 /*compare,*/ m, d;

						// Ignore pure white & pitch black
						if (brightness > 253 || brightness < 2)
						{
							cur->rgba = image->rgba;
							cur++; image++; blendimage++;
							continue;
						}

						firsti = 0;
						mul = 0;
						mulmax = 1;

						/*for (i = 0; i < translen; i++)
						{
							if (brightness > colorbrightnesses[i]) // don't allow greater matches (because calculating a makeshift gradient for this is already a huge mess as is)
								continue;

							compare = abs((INT16)(colorbrightnesses[i]) - (INT16)(brightness));

							if (compare < brightdif)
							{
								brightdif = (UINT16)compare;
								firsti = i; // best matching color that's equal brightness or darker
							}
						}*/
						firsti = color_match_lookup[brightness];

						secondi = firsti+1; // next color in line

						m = (INT16)brightness - (INT16)colorbrightnesses[secondi];
						d = (INT16)colorbrightnesses[firsti] - (INT16)colorbrightnesses[secondi];

						if (m >= d)
							m = d-1;

						mulmax = 16;

						// calculate the "gradient" multiplier based on how close this color is to the one next in line
						if (m <= 0 || d <= 0)
							mul = 0;
						else
							mul = (mulmax-1) - ((m * mulmax) / d);
					}
					else
					{
						// Just convert brightness to a skincolor value, use distance to next position to find the gradient multipler
						firsti = 0;

						for (i = 1; i < translen; i++)
						{
							if (brightness >= cutoff[i])
								break;
							firsti = i;
						}

						secondi = firsti+1;

						mulmax = cutoff[firsti] - cutoff[secondi];
						mul = cutoff[firsti] - brightness;
					}

					blendcolor = V_GetColor(translation[firsti]);

					if (mul > 0) // If it's 0, then we only need the first color.
					{
						nextcolor = V_GetColor(translation[secondi]);

						// Find difference between points
						r = (INT32)(nextcolor.s.red - blendcolor.s.red);
						g = (INT32)(nextcolor.s.green - blendcolor.s.green);
						b = (INT32)(nextcolor.s.blue - blendcolor.s.blue);

						// Find the gradient of the two points
						r = ((mul * r) / mulmax);
						g = ((mul * g) / mulmax);
						b = ((mul * b) / mulmax);

						// Add gradient value to color
						blendcolor.s.red += r;
						blendcolor.s.green += g;
						blendcolor.s.blue += b;
					}
				}

				if (skinnum == TC_RAINBOW)
				{
					UINT32 tempcolor;
					UINT16 colorbright;

					colorbright = BlendLuminance(blendcolor.s.red, blendcolor.s.green, blendcolor.s.blue);

					if (colorbright == 0)
					{
						colorbright = 1; // no dividing by 0 please
					}

					tempcolor = (brightness * blendcolor.s.red) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.red = (UINT8)tempcolor;

					tempcolor = (brightness * blendcolor.s.green) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.green = (UINT8)tempcolor;

					tempcolor = (brightness * blendcolor.s.blue) / colorbright;
					tempcolor = min(255, tempcolor);
					cur->s.blue = (UINT8)tempcolor;
					cur->s.alpha = image->s.alpha;
				}
				else
				{
					// Color strength depends on image alpha
					INT32 tempcolor;

					tempcolor = ((image->s.red * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.red * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.red = (UINT8)tempcolor;

					tempcolor = ((image->s.green * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.green * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.green = (UINT8)tempcolor;

					tempcolor = ((image->s.blue * (255-blendimage->s.alpha)) / 255) + ((blendcolor.s.blue * blendimage->s.alpha) / 255);
					tempcolor = min(255, tempcolor);
					cur->s.blue = (UINT8)tempcolor;
					cur->s.alpha = image->s.alpha;
				}

skippixel:

				// *Now* we can do Metal Sonic's flashing
				if (skinnum == TC_METALSONIC)
				{
					// Blend dark blue into white
					if (cur->s.alpha > 0 && cur->s.red == 0 && cur->s.green == 0 && cur->s.blue < 255 && cur->s.blue > 31)
					{
						// Sal: Invert non-blue
						cur->s.red = cur->s.green = (255 - cur->s.blue);
						cur->s.blue = 255;
					}

					cur->s.alpha = image->s.alpha;
				}
			}
		}

		cur++; image++;

		if (blendimage != NULL)
			blendimage++;
	}

	return;
}

static void HWR_GetBlendedTexture(patch_t *patch, patch_t *blendpatch, INT32 skinnum, const UINT8 *colormap, skincolornum_t color)
{
	// mostly copied from HWR_GetMappedPatch, hence the similarities and comment
	GLPatch_t *grPatch = patch->hardware;
	GLPatch_t *grBlendPatch = NULL;
	GLMipmap_t *grMipmap, *newMipmap;

	if (blendpatch == NULL || colormap == colormaps || colormap == NULL)
	{
		// Don't do any blending
		HWD.pfnSetTexture(grPatch->mipmap);
		return;
	}

	if ((blendpatch && (grBlendPatch = blendpatch->hardware) && grBlendPatch->mipmap->format)
		&& (patch->width != blendpatch->width || patch->height != blendpatch->height))
	{
		// Blend image exists, but it's bad.
		HWD.pfnSetTexture(grPatch->mipmap);
		return;
	}

	// search for the mipmap
	// skip the first (no colormap translated)
	for (grMipmap = grPatch->mipmap; grMipmap->nextcolormap; )
	{
		grMipmap = grMipmap->nextcolormap;
		if (grMipmap->colormap && grMipmap->colormap->source == colormap)
		{
			if (grMipmap->downloaded && grMipmap->data)
			{
				if (memcmp(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8)))
				{
					M_Memcpy(grMipmap->colormap->data, colormap, 256 * sizeof(UINT8));
					HWR_CreateBlendedTexture(patch, blendpatch, grMipmap, skinnum, color);
					HWD.pfnUpdateTexture(grMipmap);
				}
				else
					HWD.pfnSetTexture(grMipmap); // found the colormap, set it to the correct texture

				Z_ChangeTag(grMipmap->data, PU_HWRMODELTEXTURE_UNLOCKED);
				return;
			}
		}
	}

	// If here, the blended texture has not been created
	// So we create it

	//BP: WARNING: don't free it manually without clearing the cache of harware renderer
	//              (it have a liste of mipmap)
	//    this malloc is cleared in HWR_FreeColormapCache
	//    (...) unfortunately z_malloc fragment alot the memory :(so malloc is better
	newMipmap = calloc(1, sizeof (*newMipmap));
	if (newMipmap == NULL)
		I_Error("%s: Out of memory", "HWR_GetBlendedTexture");
	grMipmap->nextcolormap = newMipmap;

	newMipmap->colormap = Z_Calloc(sizeof(*newMipmap->colormap), PU_HWRPATCHCOLMIPMAP, NULL);
	newMipmap->colormap->source = colormap;
	M_Memcpy(newMipmap->colormap->data, colormap, 256 * sizeof(UINT8));

	HWR_CreateBlendedTexture(patch, blendpatch, newMipmap, skinnum, color);

	HWD.pfnSetTexture(newMipmap);
	Z_ChangeTag(newMipmap->data, PU_HWRMODELTEXTURE_UNLOCKED);
}

static boolean HWR_AllowModel(mobj_t *mobj)
{
	// Don't allow rendering of model for these.
	if (mobj->sprite2 == SPR2_XTRA || mobj->sprite2 == SPR2_SIGN || mobj->sprite2 == SPR2_SIGL || mobj->sprite2 == SPR2_SSIG || mobj->sprite2 == SPR2_TALK)
		return false;

	// Otherwise, render the model.
	return true;
}

static boolean HWR_CanInterpolateModel(mobj_t *mobj, model_t *model)
{
	// SRB2Kart: Interpoleration should ALWAYS be up to the modeler.
#ifdef BAD_MODEL_OPTIONS
	if (cv_glmodelinterpolation.value == 2) // Always interpolate
		return true;
#endif
	return model->interpolate[(mobj->frame & FF_FRAMEMASK)];
}

static boolean HWR_CanInterpolateSprite2(modelspr2frames_t *spr2frame)
{
#ifdef BAD_MODEL_OPTIONS
	if (cv_glmodelinterpolation.value == 2) // Always interpolate
		return true;
#endif
	return spr2frame->interpolate;
}

//
// HWR_GetModelSprite2 (see P_GetSkinSprite2)
// For non-super players, tries each sprite2's immediate predecessor until it finds one with a number of frames or ends up at standing.
// For super players, does the same as above - but tries the super equivalent for each sprite2 before the non-super version.
//

static UINT8 HWR_GetModelSprite2(md2_t *md2, skin_t *skin, UINT8 spr2, player_t *player)
{
	UINT8 super = 0, i = 0;

	(void)player;

	if (!md2 || !md2->model || !md2->model->spr2frames || !skin)
		return 0;

	if ((playersprite_t)(spr2 & ~FF_SPR2SUPER) >= free_spr2)
		return 0;

	while (!md2->model->spr2frames[spr2].numframes
		&& spr2 != SPR2_STIL
		&& ++i != 32) // recursion limiter
	{
		if (spr2 & FF_SPR2SUPER)
		{
			super = FF_SPR2SUPER;
			spr2 &= ~FF_SPR2SUPER;
			continue;
		}

		switch(spr2)
		{
		// Normal special cases.
		// (none in kart)

		// Use the handy list, that's what it's there for!
		default:
			spr2 = spr2defaults[spr2];
			break;
		}

		spr2 |= super;
	}

	if (i >= 32) // probably an infinite loop...
		return 0;

	return spr2;
}

// Adjust texture coords of model to fit into a patch's max_s and max_t
static void adjustTextureCoords(model_t *model, patch_t *patch)
{
	int i;
	GLPatch_t *gpatch = ((GLPatch_t *)patch->hardware);

	for (i = 0; i < model->numMeshes; i++)
	{
		int j;
		mesh_t *mesh = &model->meshes[i];
		int numVertices;
		float *uvReadPtr = mesh->originaluvs;
		float *uvWritePtr;

		// i dont know if this is actually possible, just logical conclusion of structure in CreateModelVBOs
		if (!mesh->frames && !mesh->tinyframes) continue;

		if (mesh->frames) // again CreateModelVBO and CreateModelVBOTiny iterate like this so I'm gonna do that too
			numVertices = mesh->numTriangles * 3;
		else
			numVertices = mesh->numVertices;

		// if originaluvs points to uvs, we need to allocate new memory for adjusted uvs
		// the old uvs are kept around for use in possible readjustments
		if (mesh->uvs == mesh->originaluvs)
			mesh->uvs = Z_Malloc(numVertices * 2 * sizeof(float), PU_STATIC, NULL);

		uvWritePtr = mesh->uvs;

		// fix uvs (texture coordinates) to take into account that the actual texture
		// has empty space added until the next power of two
		for (j = 0; j < numVertices; j++)
		{
			*uvWritePtr++ = *uvReadPtr++ * gpatch->max_s;
			*uvWritePtr++ = *uvReadPtr++ * gpatch->max_t;
		}
	}
	// Save the values we adjusted the uvs for
	model->max_s = gpatch->max_s;
	model->max_t = gpatch->max_t;
}

//
// HWR_DrawModel
//

boolean HWR_DrawModel(gl_vissprite_t *spr)
{
	md2_t *md2;

	char filename[64];
	INT32 frame = 0;
	INT32 nextFrame = -1;
	UINT8 spr2 = 0;
	FTransform p;
	FSurfaceInfo Surf;

	if (!cv_glmodels.value)
		return false;

	if (spr->precip)
		return false;

	// Lactozilla: Disallow certain models from rendering
	if (!HWR_AllowModel(spr->mobj))
		return false;

	memset(&p, 0x00, sizeof(FTransform));

	// MD2 colormap fix
	// colormap test
	if (spr->mobj->subsector)
	{
		sector_t *sector = spr->mobj->subsector->sector;
		INT32 lightlevel = 255;
		boolean lightset = HWR_OverrideObjectLightLevel(spr->mobj, &lightlevel);
		extracolormap_t *colormap = NULL;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false); // Always use the light at the top instead of whatever I was doing before

			if (!lightset)
				lightlevel = *sector->lightlist[light].lightlevel > 255 ? 255 : *sector->lightlist[light].lightlevel;

			if (!R_ThingIsFullBright(spr->mobj) && *sector->lightlist[light].extra_colormap)
				colormap = *sector->lightlist[light].extra_colormap;
		}
		else if (!lightset)
		{
			lightlevel = sector->lightlevel > 255 ? 255 : sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		if (!lightset)
			HWR_ObjectLightLevelPost(spr, sector, &lightlevel, true);

		HWR_Lighting(&Surf, lightlevel, colormap, P_SectorUsesDirectionalLighting(sector) && !R_ThingIsFullBright(spr->mobj) && R_ThingModelUsesDirectionalLighting(spr->mobj));
	}
	else
		Surf.PolyColor.rgba = 0xFFFFFFFF;

	// Look at HWR_ProjectSprite for more
	{
		patch_t *gpatch, *blendgpatch;
		GLPatch_t *hwrPatch = NULL, *hwrBlendPatch = NULL;
		float durs = (float)spr->mobj->state->tics;
		float tics = (float)spr->mobj->tics;
		const boolean papersprite = (R_ThingIsPaperSprite(spr->mobj) && !R_ThingIsFloorSprite(spr->mobj));
		const UINT8 flip = (UINT8)(!(spr->mobj->eflags & MFE_VERTICALFLIP) != !R_ThingVerticallyFlipped(spr->mobj));
		const UINT8 hflip = (UINT8)(!(spr->mobj->mirrored) != !R_ThingHorizontallyFlipped(spr->mobj));
		spritedef_t *sprdef;
		spriteframe_t *sprframe;
		INT32 mod;
		interpmobjstate_t interp;

		if (R_UsingFrameInterpolation() && !paused)
		{
			R_InterpolateMobjState(spr->mobj, rendertimefrac, &interp);
		}
		else
		{
			R_InterpolateMobjState(spr->mobj, FRACUNIT, &interp);
		}

		// hitlag vibrating
		if (spr->mobj->hitlag > 0 && (spr->mobj->eflags & MFE_DAMAGEHITLAG))
		{
			fixed_t jitters = HITLAGJITTERS;
			if (R_UsingFrameInterpolation() && !paused)
				jitters += (rendertimefrac / HITLAGDIV);
			fixed_t mul = spr->mobj->hitlag * jitters;

			if (leveltime & 1)
			{
				mul = -mul;
			}

			interp.x += FixedMul(spr->mobj->momx, mul);
			interp.y += FixedMul(spr->mobj->momy, mul);
			interp.z += FixedMul(spr->mobj->momz, mul);
		}

		// sprite offset
		interp.x += spr->mobj->sprxoff;
		interp.y += spr->mobj->spryoff;
		interp.z += spr->mobj->sprzoff;

		// Apparently people don't like jump frames like that, so back it goes
		//if (tics > durs)
			//durs = tics;

		// Determine the blendmode and translucency value
		{
			UINT32 blendmode, trans;
			if (spr->mobj->renderflags & RF_BLENDMASK)
				blendmode = (spr->mobj->renderflags & RF_BLENDMASK) >> RF_BLENDSHIFT;
			else
				blendmode = (spr->mobj->frame & FF_BLENDMASK) >> FF_BLENDSHIFT;
			if (blendmode)
				blendmode++; // realign to constants

			if (spr->mobj->renderflags & RF_TRANSMASK)
				trans = (spr->mobj->renderflags & RF_TRANSMASK) >> RF_TRANSSHIFT;
			else
				trans = (spr->mobj->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;
			if (trans >= NUMTRANSMAPS)
				return false; // cap

			Surf.PolyFlags = HWR_SurfaceBlend(blendmode, trans, &Surf);
		}

		// don't forget to enable the depth test because we can't do this
		// like before: model polygons are not sorted

		// 1. load model+texture if not already loaded
		// 2. draw model with correct position, rotation,...
		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY && !md2_playermodels[((skin_t*)spr->mobj->skin)->skinnum].notfound) // Use the player MD2 list if the mobj has a skin and is using the player sprites
		{
			md2 = &md2_playermodels[((skin_t*)spr->mobj->skin)->skinnum];
			md2->skin = ((skin_t*)spr->mobj->skin)->skinnum;
		}
		else
		{
			md2 = &md2_models[spr->mobj->sprite];
		}

		// texture loading before model init, so it knows if sprite graphics are used, which
		// means that texture coordinates have to be adjusted
		gpatch = md2->grpatch;
		if (gpatch)
			hwrPatch = ((GLPatch_t *)gpatch->hardware);

		if (!gpatch || !hwrPatch
		|| ((!hwrPatch->mipmap->format || !hwrPatch->mipmap->downloaded) && !md2->notexturefile))
			md2_loadTexture(md2);

		// Load it again, because it isn't being loaded into gpatch after md2_loadtexture...
		gpatch = md2->grpatch;
		if (gpatch)
			hwrPatch = ((GLPatch_t *)gpatch->hardware);

		// Load blend texture
		blendgpatch = md2->blendgrpatch;
		if (blendgpatch)
			hwrBlendPatch = ((GLPatch_t *)blendgpatch->hardware);

		if ((gpatch && hwrPatch && hwrPatch->mipmap->format) // don't load the blend texture if the base texture isn't available
			&& (!blendgpatch || !hwrBlendPatch
			|| ((!hwrBlendPatch->mipmap->format || !hwrBlendPatch->mipmap->downloaded) && !md2->noblendfile)))
			md2_loadBlendTexture(md2);

		// Load it again, because it isn't being loaded into blendgpatch after md2_loadblendtexture...
		blendgpatch = md2->blendgrpatch;
		if (blendgpatch)
			hwrBlendPatch = ((GLPatch_t *)blendgpatch->hardware);

		if (md2->error)
			return false; // we already failed loading this before :(
		if (!md2->model)
		{
			//CONS_Debug(DBG_RENDER, "Loading model... (%s)", sprnames[spr->mobj->sprite]);
			sprintf(filename, "models/%s", md2->filename);
			md2->model = md2_readModel(filename);

			if (md2->model)
			{
				md2_printModelInfo(md2->model);
				// If model uses sprite patch as texture, then
				// adjust texture coordinates to take power of two textures into account
				if (!gpatch || !hwrPatch->mipmap->format)
					adjustTextureCoords(md2->model, spr->gpatch);
				// note down the max_s and max_t that end up in the VBO
				md2->model->vbo_max_s = md2->model->max_s;
				md2->model->vbo_max_t = md2->model->max_t;
				HWD.pfnCreateModelVBOs(md2->model);
			}
			else
			{
				//CONS_Debug(DBG_RENDER, " FAILED\n");
				md2->error = true; // prevent endless fail
				return false;
			}
		}

		//HWD.pfnSetBlend(blend); // This seems to actually break translucency?
		//Hurdler: arf, I don't like that implementation at all... too much crappy

		if (gpatch && hwrPatch && hwrPatch->mipmap->format) // else if meant that if a texture couldn't be loaded, it would just end up using something else's texture
		{
			INT32 skinnum = TC_DEFAULT;

			if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY) // This thing is a player!
			{
				skinnum = ((skin_t*)spr->mobj->skin)->skinnum;
			}

			// Hide not-yet-unlocked characters in replays from other people
			if (skinnum >= 0 && !R_CanShowSkinInDemo(skinnum))
			{
				skinnum = TC_BLINK;
			}
			else if (R_ThingIsFlashing(spr->mobj))
			{
				skinnum = TC_HITLAG;
			}
			else if (spr->mobj->color && spr->mobj->colorized)
			{
				skinnum = TC_RAINBOW;
			}

			// Translation or skin number found
			HWR_GetBlendedTexture(gpatch, blendgpatch, skinnum, spr->colormap, (skincolornum_t)spr->mobj->color);
		}
		else // Sprite
		{
			// Check if sprite dimensions are different from previously used sprite.
			// If so, uvs need to be readjusted.
			// Comparing floats with the != operator here should be okay because they
			// are just copies of glpatches' max_s and max_t values.
			// Instead of the != operator, memcmp is used to avoid a compiler warning.
			if (memcmp(&(hwrPatch->max_s), &(md2->model->max_s), sizeof(md2->model->max_s)) != 0 ||
				memcmp(&(hwrPatch->max_t), &(md2->model->max_t), sizeof(md2->model->max_t)) != 0)
				adjustTextureCoords(md2->model, spr->gpatch);
			HWR_GetMappedPatch(spr->gpatch, spr->colormap);
		}

		if (spr->mobj->frame & FF_ANIMATE)
		{
			// set duration and tics to be the correct values for FF_ANIMATE states
			durs = (float)spr->mobj->state->var2;
			tics = (float)spr->mobj->anim_duration;
		}

		frame = (spr->mobj->frame & FF_FRAMEMASK);
		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY && md2->model->spr2frames)
		{
			spr2 = HWR_GetModelSprite2(md2, spr->mobj->skin, spr->mobj->sprite2, spr->mobj->player);
			mod = md2->model->spr2frames[spr2].numframes;
#ifndef DONTHIDEDIFFANIMLENGTH // by default, different anim length is masked by the mod
			if (mod > (INT32)((skin_t *)spr->mobj->skin)->sprites[spr2].numframes)
				mod = ((skin_t *)spr->mobj->skin)->sprites[spr2].numframes;
#endif
			if (!mod)
				mod = 1;
			frame = md2->model->spr2frames[spr2].frames[frame%mod];
		}
		else
		{
			mod = md2->model->meshes[0].numFrames;
			if (!mod)
				mod = 1;
		}

#ifdef USE_MODEL_NEXTFRAME
		// Interpolate the model interpolation. (lol)
		tics -= FixedToFloat(rendertimefrac);

#define INTERPOLERATION_LIMIT TICRATE/4
		if (
#ifdef BAD_MODEL_OPTIONS
			cv_glmodelinterpolation.value &&
#endif
			tics <= durs && tics <= INTERPOLERATION_LIMIT)
		{
			if (durs > INTERPOLERATION_LIMIT)
				durs = INTERPOLERATION_LIMIT;

			if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY && md2->model->spr2frames)
			{
				if (HWR_CanInterpolateSprite2(&md2->model->spr2frames[spr2])
					&& (spr->mobj->frame & FF_ANIMATE
					|| (spr->mobj->state->nextstate != S_NULL
					&& states[spr->mobj->state->nextstate].sprite == SPR_PLAY
					&& ((P_GetSkinSprite2(spr->mobj->skin, (states[spr->mobj->state->nextstate].frame) & FF_FRAMEMASK, spr->mobj->player) == spr->mobj->sprite2)))))
				{
					nextFrame = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextFrame >= mod)
					{
						if (spr->mobj->state->frame & FF_SPR2ENDSTATE)
							nextFrame--;
						else
							nextFrame = 0;
					}
					if (frame || !(spr->mobj->state->frame & FF_SPR2ENDSTATE))
						nextFrame = md2->model->spr2frames[spr2].frames[nextFrame];
					else
						nextFrame = -1;
				}
			}
			else if (HWR_CanInterpolateModel(spr->mobj, md2->model))
			{
				// frames are handled differently for states with FF_ANIMATE, so get the next frame differently for the interpolation
				if (spr->mobj->frame & FF_ANIMATE)
				{
					nextFrame = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextFrame >= (INT32)(spr->mobj->state->var1 + (spr->mobj->state->frame & FF_FRAMEMASK)))
						nextFrame = (spr->mobj->state->frame & FF_FRAMEMASK) % mod;
				}
				else
				{
					if (spr->mobj->state->nextstate != S_NULL && states[spr->mobj->state->nextstate].sprite != SPR_NULL)
						nextFrame = (states[spr->mobj->state->nextstate].frame & FF_FRAMEMASK) % mod;
				}
			}
		}
#undef INTERPOLERATION_LIMIT
#endif

		//Hurdler: it seems there is still a small problem with mobj angle
		p.x = FIXED_TO_FLOAT(interp.x);
		p.y = FIXED_TO_FLOAT(interp.y) + md2->offset;

		if (flip)
			p.z = FIXED_TO_FLOAT(interp.z + spr->mobj->height);
		else
			p.z = FIXED_TO_FLOAT(interp.z);

		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			sprdef = &((skin_t *)spr->mobj->skin)->sprites[spr->mobj->sprite2];
		else
			sprdef = &sprites[spr->mobj->sprite];

		sprframe = &sprdef->spriteframes[spr->mobj->frame & FF_FRAMEMASK];

		if (sprframe->rotate || papersprite)
		{
			fixed_t anglef = AngleFixed(interp.angle);

			p.angley = FIXED_TO_FLOAT(anglef);
		}
		else
		{
			const fixed_t anglef = AngleFixed((R_PointToAngle(interp.x, interp.y))-ANGLE_180);
			p.angley = FIXED_TO_FLOAT(anglef);
		}

		angle_t pitchR, rollR, fixedAngY;

		pitchR = 0;
		rollR = 0;
		fixedAngY = 0;

		{
			fixed_t anglef = AngleFixed(R_ModelRotationAngle(spr->mobj, NULL));

			p.rollangle = 0.0f;

			// make fixedAngY a disguised fixed_t first
			fixedAngY = FLOAT_TO_FIXED(p.angley);

			if (anglef)
			{
				fixed_t camAngleDiff = AngleFixed(viewangle) - (fixed_t)(fixedAngY); // dumb reconversion back, I know

				p.rollangle = FIXED_TO_FLOAT(anglef);
				p.roll = true;

				// rotation pivot
				p.centerx = FIXED_TO_FLOAT(spr->mobj->radius / 2);
				p.centery = FIXED_TO_FLOAT(spr->mobj->height / 2);

				// rotation axes relative to camera
				pitchR = FINESINE(FixedAngle(camAngleDiff) >> ANGLETOFINESHIFT);
				rollR = FINECOSINE(FixedAngle(camAngleDiff) >> ANGLETOFINESHIFT);

				p.rollx = FIXED_TO_FLOAT((fixed_t)rollR);
				p.rollz = FIXED_TO_FLOAT((fixed_t)pitchR);

				// convert to angles
				pitchR = FixedAngle((fixed_t)pitchR);
				rollR = FixedAngle((fixed_t)rollR);
			}

			// make this a proper angle now
			fixedAngY = FixedAngle(fixedAngY);
		}

		p.anglez = FIXED_TO_FLOAT(AngleFixed(R_InterpolateAngle(spr->mobj->old_pitch, spr->mobj->pitch)));
		p.anglex = FIXED_TO_FLOAT(AngleFixed(R_InterpolateAngle(spr->mobj->old_roll, spr->mobj->roll)));

		p.flip = atransform.flip;
		p.mirror = atransform.mirror;

		HWD.pfnSetShader(SHADER_MODEL);	// model shader
		{
			float this_scale = FIXED_TO_FLOAT(spr->mobj->scale);
			fixed_t floorClip = spr->mobj->terrain ? spr->mobj->terrain->floorClip : 0;
			float finalfloorClip = FIXED_TO_FLOAT(FixedMul(floorClip, mapobjectscale)*P_MobjFlip(spr->mobj));

			float xs = this_scale * FIXED_TO_FLOAT(spr->mobj->spritexscale);
			float ys = this_scale * FIXED_TO_FLOAT(spr->mobj->spriteyscale);

			float ox = xs * FIXED_TO_FLOAT(spr->mobj->spritexoffset);
			float oy = ys * FIXED_TO_FLOAT(spr->mobj->spriteyoffset);

			// offset perpendicular to the camera angle
			p.x -= ox * gl_viewsin;
			p.y += ox * gl_viewcos;
			p.z += oy - finalfloorClip;

			if (R_ThingIsUsingBakedOffsets(spr->mobj))
			{
				// visoffset stuff
				float xx, xy, yx, yy;
				float zx, zy, zz;
				float xpiv, ypiv, zpiv;
				fixed_t zh;
				fixed_t xoffs, yoffs;

				// xoffs = (cos(xoff) + sin(yoff))
				xoffs =
					FixedMul(spr->mobj->bakeyoff, -FINECOSINE(fixedAngY >> ANGLETOFINESHIFT)) +
					FixedMul(spr->mobj->bakexoff, FINESINE(fixedAngY >> ANGLETOFINESHIFT));

				// yoffs = (-sin(xoff) + cos(yoff))
				yoffs =
					FixedMul(spr->mobj->bakeyoff, -FINESINE(fixedAngY >> ANGLETOFINESHIFT)) +
					FixedMul(spr->mobj->bakexoff, FINECOSINE(fixedAngY >> ANGLETOFINESHIFT));

				const fixed_t hflipmul = hflip ? -FRACUNIT : FRACUNIT;

				xpiv = FIXED_TO_FLOAT(
					FixedMul(
						FixedMul(spr->mobj->bakeypiv,
								 -FINECOSINE(fixedAngY >> ANGLETOFINESHIFT)) +
						FixedMul(spr->mobj->bakexpiv,
								 FINESINE(fixedAngY >> ANGLETOFINESHIFT)),
					hflipmul));
				ypiv = FIXED_TO_FLOAT(
					FixedMul(
						FixedMul(spr->mobj->bakeypiv,
								 -FINESINE(fixedAngY >> ANGLETOFINESHIFT)) +
						FixedMul(spr->mobj->bakexpiv,
								 FINECOSINE(fixedAngY >> ANGLETOFINESHIFT)),
					hflipmul));
				zpiv = FIXED_TO_FLOAT(spr->mobj->bakezpiv * ((flip) ? -1 : 1));

				pitchR = ((pitchR + spr->mobj->pitch) * ((flip) ? -1 : 1));
				rollR = ((rollR + spr->mobj->roll) * ((flip) ? -1 : 1));

				// x offset
				xx = FIXED_TO_FLOAT(FixedMul(FixedMul(
											FixedMul(xoffs,spr->mobj->spritexscale),
											hflipmul), 
											FINECOSINE(pitchR >> ANGLETOFINESHIFT)
											));
				xy = FIXED_TO_FLOAT(FixedMul(FixedMul(
									FixedMul(xoffs,spr->mobj->spritexscale),
									hflipmul),
									-FINESINE(pitchR >> ANGLETOFINESHIFT)
									));

				// y offset
				yx = FIXED_TO_FLOAT(FixedMul(FixedMul(
											FixedMul(yoffs,spr->mobj->spritexscale),
											hflipmul), 
											FINECOSINE(rollR >> ANGLETOFINESHIFT)
											));

				yy = FIXED_TO_FLOAT(FixedMul(FixedMul(
									FixedMul(yoffs,spr->mobj->spritexscale),
									hflipmul),
									-FINESINE(rollR >> ANGLETOFINESHIFT)
									));

				// z offset
				zh = FixedMul(FixedMul(spr->mobj->bakezoff,spr->mobj->spriteyscale),
								FINECOSINE(rollR >> ANGLETOFINESHIFT));

				zz = FIXED_TO_FLOAT(FixedMul(zh,
									FINECOSINE(pitchR >> ANGLETOFINESHIFT)));
				zx = FIXED_TO_FLOAT(FixedMul(zh,
									FINESINE(pitchR >> ANGLETOFINESHIFT)));
				zy = FIXED_TO_FLOAT(FixedMul(zh,
									FINESINE(rollR >> ANGLETOFINESHIFT)));

				// do these namings even make sense at this point?
				p.x += xx + zx + xpiv;
				p.z += (xy + yy + zz * (flip ? -1 : 1)) + zpiv;
				p.y += yx + zy + ypiv;
			}

			HWD.pfnDrawModel(md2->model, frame, durs, tics, nextFrame, &p, md2->scale * xs, md2->scale * ys, flip, hflip, &Surf);
		}
	}

	return true;
}

#endif //HWRENDER
