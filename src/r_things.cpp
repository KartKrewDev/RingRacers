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
/// \file  r_things.cpp
/// \brief Refresh of things, i.e. objects represented by sprites

#include <algorithm>

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "r_local.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "k_menu.h" // character select
#include "m_misc.h"
#include "info.h" // spr2names
#include "i_video.h" // rendermode
#include "i_system.h"
#include "r_fps.h"
#include "r_things.h"
#include "r_patch.h"
#include "r_patchrotation.h"
#include "r_picformats.h"
#include "r_plane.h"
#include "r_portal.h"
#include "r_splats.h"
#include "p_tick.h"
#include "p_local.h"
#include "p_slopes.h"
#include "d_netfil.h" // blargh. for nameonly().
#include "m_cheat.h" // objectplace
#include "p_local.h" // stplyr
#include "core/thread_pool.h"
#ifdef HWRENDER
#include "hardware/hw_md2.h"
#include "hardware/hw_glob.h"
#include "hardware/hw_light.h"
#include "hardware/hw_drv.h"
#endif

// SRB2kart
#include "k_color.h"
#include "k_hitlag.h" // HITLAGJITTERS
#include "r_fps.h"

#define MINZ (FRACUNIT*4)
#define BASEYCENTER (BASEVIDHEIGHT/2)

typedef struct
{
	INT32 x1, x2;
	INT32 column;
	INT32 topclip, bottomclip;
} maskdraw_t;

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
static lighttable_t **spritelights;

// constant arrays used for psprite clipping and initializing clipping
INT16 negonearray[MAXVIDWIDTH];
INT16 screenheightarray[MAXVIDWIDTH];

spriteinfo_t spriteinfo[NUMSPRITES];

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up and range check thing_t sprites patches
spritedef_t *sprites;
size_t numsprites;

static spriteframe_t sprtemp[64];
static size_t maxframe;
static const char *spritename;

//
// Clipping against drawsegs optimization, from prboom-plus
//
// TODO: This should be done with proper subsector pass through
// sprites which would ideally remove the need to do it at all.
// Unfortunately, SRB2's drawing loop has lots of annoying
// changes from Doom for portals, which make it hard to implement.

typedef struct drawseg_xrange_item_s
{
	INT16 x1, x2;
	drawseg_t *user;
} drawseg_xrange_item_t;

typedef struct drawsegs_xrange_s
{
	drawseg_xrange_item_t *items;
	INT32 count;
} drawsegs_xrange_t;

#define DS_RANGES_COUNT 3
static drawsegs_xrange_t drawsegs_xranges[DS_RANGES_COUNT];

static drawseg_xrange_item_t *drawsegs_xrange;
static size_t drawsegs_xrange_size = 0;
static INT32 drawsegs_xrange_count = 0;

#define CLIP_UNDEF -2

// ==========================================================================
//
// Sprite loading routines: support sprites in pwad, dehacked sprite renaming,
// replacing not all frames of an existing sprite, add sprites at run-time,
// add wads at run-time.
//
// ==========================================================================

//
//
//
static void R_InstallSpriteLump(UINT16 wad,            // graphics patch
                                UINT16 lump,
                                size_t lumpid,      // identifier
                                UINT8 frame,
                                UINT8 rotation,
                                UINT8 flipped)
{
	char cn = R_Frame2Char(frame), cr = R_Rotation2Char(rotation); // for debugging

	INT32 r;
	lumpnum_t lumppat = wad;
	lumppat <<= 16;
	lumppat += lump;

	if (maxframe ==(size_t)-1 || frame > maxframe)
		maxframe = frame;

#ifdef ROTSPRITE
	for (r = 0; r < 16; r++)
	{
		sprtemp[frame].rotated[0][r] = NULL;
		sprtemp[frame].rotated[1][r] = NULL;
	}
#endif

	if (rotation == 0)
	{
		// the lump should be used for all rotations
		if (sprtemp[frame].rotate == SRF_SINGLE)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has multiple rot = 0 lump\n", spritename, cn);
		else if (sprtemp[frame].rotate != SRF_NONE) // Let's bundle 1-8/16 and L/R rotations into one debug message.
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has rotations and a rot = 0 lump\n", spritename, cn);

		sprtemp[frame].rotate = SRF_SINGLE;
		for (r = 0; r < 16; r++)
		{
			sprtemp[frame].lumppat[r] = lumppat;
			sprtemp[frame].lumpid[r] = lumpid;
		}
		sprtemp[frame].flip = flipped ? 0xFFFF : 0; // 1111111111111111 in binary
		return;
	}

	if (rotation == ROT_L || rotation == ROT_R)
	{
		UINT8 rightfactor = ((rotation == ROT_R) ? 4 : 0);

		// the lump should be used for half of all rotations
		if (sprtemp[frame].rotate == SRF_NONE)
			sprtemp[frame].rotate = SRF_SINGLE;
		else if (sprtemp[frame].rotate == SRF_SINGLE)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has L/R rotations and a rot = 0 lump\n", spritename, cn);
		else if (sprtemp[frame].rotate == SRF_3D)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has both L/R and 1-8 rotations\n", spritename, cn);
		else if (sprtemp[frame].rotate == SRF_3DGE)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has both L/R and 1-G rotations\n", spritename, cn);
		else if ((sprtemp[frame].rotate & SRF_LEFT) && (rotation == ROT_L))
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has multiple L rotations\n", spritename, cn);
		else if ((sprtemp[frame].rotate & SRF_RIGHT) && (rotation == ROT_R))
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has multiple R rotations\n", spritename, cn);

		sprtemp[frame].rotate |= ((rotation == ROT_R) ? SRF_RIGHT : SRF_LEFT);
		if ((sprtemp[frame].rotate & SRF_2D) == SRF_2D)
			sprtemp[frame].rotate &= ~SRF_3DMASK; // SRF_3D|SRF_2D being enabled at the same time doesn't HURT in the current sprite angle implementation, but it DOES mean more to check in some of the helper functions. Let's not allow this scenario to happen.

		// load into every relevant angle, including the front one
		for (r = 0; r < 4; r++)
		{
			sprtemp[frame].lumppat[r + rightfactor] = lumppat;
			sprtemp[frame].lumpid[r + rightfactor] = lumpid;
			sprtemp[frame].lumppat[r + rightfactor + 8] = lumppat;
			sprtemp[frame].lumpid[r + rightfactor + 8] = lumpid;

		}

		if (flipped)
			sprtemp[frame].flip |= (0x0F0F<<rightfactor); // 0000111100001111 or 1111000011110000 in binary, depending on rotation being ROT_L or ROT_R
		else
			sprtemp[frame].flip &= ~(0x0F0F<<rightfactor); // ditto

		return;
	}

	if (sprtemp[frame].rotate == SRF_NONE)
		sprtemp[frame].rotate = SRF_SINGLE;
	else if (sprtemp[frame].rotate == SRF_SINGLE)
		CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has 1-8/G rotations and a rot = 0 lump\n", spritename, cn);
	else if (sprtemp[frame].rotate & SRF_2D)
		CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s frame %c has both L/R and 1-8/G rotations\n", spritename, cn);

	// make 0 based
	rotation--;

	{
		// SRF_3D|SRF_3DGE being enabled at the same time doesn't HURT in the current sprite angle implementation, but it DOES mean more to check in some of the helper functions. Let's not allow this scenario to happen.
		UINT8 threedrot = (rotation > 7) ? SRF_3DGE : (sprtemp[frame].rotate & SRF_3DMASK);
		if (!threedrot)
			threedrot = SRF_3D;

		if (rotation == 0 || rotation == 4) // Front or back...
			sprtemp[frame].rotate = threedrot; // Prevent L and R changeover
		else if ((rotation & 7) > 3) // Right side
			sprtemp[frame].rotate = (threedrot | (sprtemp[frame].rotate & SRF_LEFT)); // Continue allowing L frame changeover
		else // if ((rotation & 7) <= 3) // Left side
			sprtemp[frame].rotate = (threedrot | (sprtemp[frame].rotate & SRF_RIGHT)); // Continue allowing R frame changeover
	}

	if (sprtemp[frame].lumppat[rotation] != LUMPERROR)
		CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s: %c%c has two lumps mapped to it\n", spritename, cn, cr);

	// lumppat & lumpid are the same for original Doom, but different
	// when using sprites in pwad : the lumppat points the new graphics
	sprtemp[frame].lumppat[rotation] = lumppat;
	sprtemp[frame].lumpid[rotation] = lumpid;
	if (flipped)
		sprtemp[frame].flip |= (1<<rotation);
	else
		sprtemp[frame].flip &= ~(1<<rotation);
}

// Install a single sprite, given its identifying name (4 chars)
//
// (originally part of R_AddSpriteDefs)
//
// Pass: name of sprite : 4 chars
//       spritedef_t
//       wadnum         : wad number, indexes wadfiles[], where patches
//                        for frames are found
//       startlump      : first lump to search for sprite frames
//       endlump        : AFTER the last lump to search
//
// Returns true if the sprite was succesfully added
//
boolean R_AddSingleSpriteDef(const char *sprname, spritedef_t *spritedef, UINT16 wadnum, UINT16 startlump, UINT16 endlump)
{
	UINT16 l;
	UINT8 frame;
	UINT8 rotation;
	lumpinfo_t *lumpinfo;
	softwarepatch_t patch;
	UINT16 numadded = 0;

	memset(sprtemp,0xFF, sizeof (sprtemp));
	maxframe = (size_t)-1;

	spritename = sprname;

	// are we 'patching' a sprite already loaded ?
	// if so, it might patch only certain frames, not all
	if (spritedef->numframes) // (then spriteframes is not null)
	{
		// copy the already defined sprite frames
		M_Memcpy(sprtemp, spritedef->spriteframes,
		 spritedef->numframes * sizeof (spriteframe_t));
		maxframe = spritedef->numframes - 1;
	}

	// scan the lumps,
	//  filling in the frames for whatever is found
	lumpinfo = wadfiles[wadnum]->lumpinfo;
	if (endlump > wadfiles[wadnum]->numlumps)
		endlump = wadfiles[wadnum]->numlumps;

	for (l = startlump; l < endlump; l++)
	{
		if (memcmp(lumpinfo[l].name,sprname,4))
			continue;

		{
			INT32 width, height;
			INT16 topoffset, leftoffset;
#ifndef NO_PNG_LUMPS
			boolean isPNG = false;
#endif

			frame = R_Char2Frame(lumpinfo[l].name[4]);
			rotation = R_Char2Rotation(lumpinfo[l].name[5]);

			if (frame >= 64 || rotation == 255) // Give an actual NAME error -_-...
			{
				CONS_Alert(CONS_WARNING, M_GetText("Bad sprite name: %s\n"), W_CheckNameForNumPwad(wadnum,l));
				continue;
			}

			// skip NULL sprites from very old dmadds pwads
			if (W_LumpLengthPwad(wadnum,l)<=8)
				continue;

			// store sprite info in lookup tables
			//FIXME : numspritelumps do not duplicate sprite replacements

			W_ReadLumpHeaderPwad(wadnum, l, &patch, PNG_HEADER_SIZE, 0);

#ifndef NO_PNG_LUMPS
			{
				size_t len = W_LumpLengthPwad(wadnum, l);

				if (Picture_IsLumpPNG((UINT8*)&patch, len))
				{
					UINT8 *png = static_cast<UINT8*>(W_CacheLumpNumPwad(wadnum, l, PU_STATIC));
					Picture_PNGDimensions((UINT8 *)png, &width, &height, &topoffset, &leftoffset, len);
					isPNG = true;
					Z_Free(png);
				}
			}

			if (!isPNG)
#endif
			{
				width = (INT32)(SHORT(patch.width));
				height = (INT32)(SHORT(patch.height));
				topoffset = (INT16)(SHORT(patch.topoffset));
				leftoffset = (INT16)(SHORT(patch.leftoffset));
			}

			spritecachedinfo[numspritelumps].width = width<<FRACBITS;
			spritecachedinfo[numspritelumps].offset = leftoffset<<FRACBITS;
			spritecachedinfo[numspritelumps].topoffset = topoffset<<FRACBITS;
			spritecachedinfo[numspritelumps].height = height<<FRACBITS;

			// BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
			spritecachedinfo[numspritelumps].topoffset += FEETADJUST;

			//----------------------------------------------------

			R_InstallSpriteLump(wadnum, l, numspritelumps, frame, rotation, 0);

			if (lumpinfo[l].name[6])
			{
				frame = R_Char2Frame(lumpinfo[l].name[6]);
				rotation = R_Char2Rotation(lumpinfo[l].name[7]);

				if (frame >= 64 || rotation == 255) // Give an actual NAME error -_-...
				{
					CONS_Alert(CONS_WARNING, M_GetText("Bad sprite name: %s\n"), W_CheckNameForNumPwad(wadnum,l));
					continue;
				}
				R_InstallSpriteLump(wadnum, l, numspritelumps, frame, rotation, 1);
			}

			if (++numspritelumps >= max_spritelumps)
			{
				max_spritelumps *= 2;
				Z_Realloc(spritecachedinfo, max_spritelumps*sizeof(*spritecachedinfo), PU_STATIC, &spritecachedinfo);
			}

			++numadded;
		}
	}

	//
	// if no frames found for this sprite
	//
	if (maxframe == (size_t)-1)
	{
		// the first time (which is for the original wad),
		// all sprites should have their initial frames
		// and then, patch wads can replace it
		// we will skip non-replaced sprite frames, only if
		// they have already have been initially defined (original wad)

		//check only after all initial pwads added
		//if (spritedef->numframes == 0)
		//    I_Error("R_AddSpriteDefs: no initial frames found for sprite %s\n",
		//             namelist[i]);

		// sprite already has frames, and is not replaced by this wad
		return false;
	}
	else if (!numadded)
	{
		// Nothing related to this spritedef has been changed
		// so there is no point going back through these checks again.
		return false;
	}

	maxframe++;

	//
	//  some checks to help development
	//
	for (frame = 0; frame < maxframe; frame++)
	{
		switch (sprtemp[frame].rotate)
		{
			case SRF_NONE:
			// no rotations were found for that frame at all
			I_Error("R_AddSingleSpriteDef: No patches found for %.4s frame %c", sprname, R_Frame2Char(frame));
			break;

			case SRF_SINGLE:
			// only the first rotation is needed
			break;

			case SRF_2D: // both Left and Right rotations
			// we test to see whether the left and right slots are present
			if ((sprtemp[frame].lumppat[2] == LUMPERROR) || (sprtemp[frame].lumppat[6] == LUMPERROR))
				I_Error("R_AddSingleSpriteDef: Sprite %.4s frame %c is missing rotations (L-R mode)",
				sprname, R_Frame2Char(frame));
			break;

			default:
			// must have all 8/16 frames
				rotation = ((sprtemp[frame].rotate & SRF_3DGE) ? 16 : 8);
				while (rotation--)
				// we test the patch lump, or the id lump whatever
				// if it was not loaded the two are LUMPERROR
				if (sprtemp[frame].lumppat[rotation] == LUMPERROR)
					I_Error("R_AddSingleSpriteDef: Sprite %.4s frame %c is missing rotations (1-%c mode)",
					        sprname, R_Frame2Char(frame), ((sprtemp[frame].rotate & SRF_3DGE) ? 'G' : '8'));
			break;
		}
	}

	// allocate space for the frames present and copy sprtemp to it
	if (spritedef->numframes &&             // has been allocated
		spritedef->numframes < maxframe)   // more frames are defined ?
	{
		Z_Free(spritedef->spriteframes);
		spritedef->spriteframes = NULL;
	}

	// allocate this sprite's frames
	if (!spritedef->spriteframes)
		spritedef->spriteframes =
		 static_cast<spriteframe_t*>(Z_Malloc(maxframe * sizeof (*spritedef->spriteframes), PU_STATIC, NULL));

	spritedef->numframes = maxframe;
	M_Memcpy(spritedef->spriteframes, sprtemp, maxframe*sizeof (spriteframe_t));

	return true;
}

//
// Search for sprites replacements in a wad whose names are in namelist
//
void R_AddSpriteDefs(UINT16 wadnum)
{
	size_t i, addsprites = 0;
	UINT16 start, end;
	char wadname[MAX_WADPATH];

	// Find the sprites section in this resource file.
	switch (wadfiles[wadnum]->type)
	{
	case RET_WAD:
		start = W_CheckNumForMarkerStartPwad("S_START", wadnum, 0);
		if (start == INT16_MAX)
			start = W_CheckNumForMarkerStartPwad("SS_START", wadnum, 0); //deutex compatib.

		end = W_CheckNumForNamePwad("S_END",wadnum,start);
		if (end == INT16_MAX)
			end = W_CheckNumForNamePwad("SS_END",wadnum,start);     //deutex compatib.
		break;
	case RET_PK3:
		start = W_CheckNumForFolderStartPK3("Sprites/", wadnum, 0);
		end = W_CheckNumForFolderEndPK3("Sprites/", wadnum, start);
		break;
	default:
		return;
	}

	if (start == INT16_MAX)
	{
		// ignore skin wads (we don't want skin sprites interfering with vanilla sprites)
		if (W_CheckNumForNamePwad("S_SKIN", wadnum, 0) != UINT16_MAX)
			return;

		start = 0; //let say S_START is lump 0
	}

	if (end == INT16_MAX || start >= end)
	{
		CONS_Debug(DBG_SETUP, "no sprites in pwad %d\n", wadnum);
		return;
	}


	//
	// scan through lumps, for each sprite, find all the sprite frames
	//
	for (i = 0; i < numsprites; i++)
	{
		if (sprnames[i][4] && wadnum >= (UINT16)sprnames[i][4])
			continue;

		if (R_AddSingleSpriteDef(sprnames[i], &sprites[i], wadnum, start, end))
		{
#ifdef HWRENDER
			if (rendermode == render_opengl)
				HWR_AddSpriteModel(i);
#endif
			// if a new sprite was added (not just replaced)
			addsprites++;
			CONS_Debug(DBG_SETUP, "sprite %s set in pwad %d\n", sprnames[i], wadnum);
		}
	}

	nameonly(strcpy(wadname, wadfiles[wadnum]->filename));
	CONS_Printf(M_GetText("%s added %d frames in %s sprites\n"), wadname, end-start, sizeu1(addsprites));
}

//
// GAME FUNCTIONS
//
UINT32 visspritecount;
static UINT32 clippedvissprites;
static vissprite_t *visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS] = {NULL};

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(void)
{
	size_t i;
#ifdef ROTSPRITE
	INT32 angle;
	float fa;
#endif

	// allocate sprite lookup tables
	numspritelumps = 0;
	max_spritelumps = 8192;
	Z_Malloc(max_spritelumps*sizeof(*spritecachedinfo), PU_STATIC, &spritecachedinfo);

	for (i = 0; i < MAXVIDWIDTH; i++)
		negonearray[i] = -1;

#ifdef ROTSPRITE
	for (angle = 1; angle < ROTANGLES; angle++)
	{
		fa = ANG2RAD(FixedAngle((ROTANGDIFF * angle)<<FRACBITS));
		rollcosang[angle] = FLOAT_TO_FIXED(cos(-fa));
		rollsinang[angle] = FLOAT_TO_FIXED(sin(-fa));
	}
#endif

	//
	// count the number of sprite names, and allocate sprites table
	//
	numsprites = 0;
	for (i = 0; i < NUMSPRITES + 1; i++)
		if (sprnames[i][0] != '\0') numsprites++;

	if (!numsprites)
		I_Error("R_AddSpriteDefs: no sprites in namelist\n");

	sprites = static_cast<spritedef_t*>(Z_Calloc(numsprites * sizeof (*sprites), PU_STATIC, NULL));

	// find sprites in each -file added pwad
	for (i = 0; i < numwadfiles; i++)
		R_AddSpriteDefs((UINT16)i);

	//
	// check if all sprites have frames
	//
	/*
	for (i = 0; i < numsprites; i++)
		if (sprites[i].numframes < 1)
			CONS_Debug(DBG_SETUP, "R_InitSprites: sprite %s has no frames at all\n", sprnames[i]);
	*/
}

//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites(void)
{
	visspritecount = clippedvissprites = 0;
}

//
// R_NewVisSprite
//
static vissprite_t overflowsprite;

static vissprite_t *R_GetVisSprite(UINT32 num)
{
		UINT32 chunk = num >> VISSPRITECHUNKBITS;

		// Allocate chunk if necessary
		if (!visspritechunks[chunk])
			Z_Malloc(sizeof(vissprite_t) * VISSPRITESPERCHUNK, PU_LEVEL, &visspritechunks[chunk]);

		return visspritechunks[chunk] + (num & VISSPRITEINDEXMASK);
}

static vissprite_t *R_NewVisSprite(void)
{
	if (visspritecount == MAXVISSPRITES)
		return &overflowsprite;

	return R_GetVisSprite(visspritecount++);
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
INT16 *mfloorclip;
INT16 *mceilingclip;

fixed_t spryscale = 0, sprtopscreen = 0, sprbotscreen = 0;
fixed_t windowtop = 0, windowbottom = 0;

void R_DrawMaskedColumn(drawcolumndata_t* dc, column_t *column, column_t *brightmap, INT32 baseclip)
{
	INT32 topscreen;
	INT32 bottomscreen;
	fixed_t basetexturemid;
	INT32 topdelta, prevdelta = 0;

	basetexturemid = dc->texturemid;

	R_SetColumnFunc(colfunctype, brightmap != NULL);
	dc->brightmap = NULL;

	for (; column->topdelta != 0xff ;)
	{
		// calculate unclipped screen coordinates
		// for post
		topdelta = column->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topscreen = sprtopscreen + spryscale*topdelta;
		bottomscreen = topscreen + spryscale*column->length;

		dc->yl = (topscreen+FRACUNIT-1)>>FRACBITS;
		dc->yh = (bottomscreen-1)>>FRACBITS;

		if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
		{
			if (windowtop > topscreen)
				dc->yl = (windowtop + FRACUNIT - 1)>>FRACBITS;
			if (windowbottom < bottomscreen)
				dc->yh = (windowbottom - 1)>>FRACBITS;
		}

		if (dc->yh >= mfloorclip[dc->x])
			dc->yh = mfloorclip[dc->x]-1;
		if (dc->yl <= mceilingclip[dc->x])
			dc->yl = mceilingclip[dc->x]+1;

		if (dc->yl < 0)
			dc->yl = 0;
		if (dc->yh >= vid.height) // dc_yl must be < vid.height, so reduces number of checks in tight loop
			dc->yh = vid.height - 1;

		if (dc->yh >= baseclip && baseclip != -1)
			dc->yh = baseclip;

		if (dc->yl <= dc->yh && dc->yh > 0 && column->length != 0)
		{
			dc->source = (UINT8 *)column + 3;
			dc->sourcelength = column->length;
			if (brightmap != NULL)
			{
				dc->brightmap = (UINT8 *)brightmap + 3;
			}

			dc->texturemid = basetexturemid - (topdelta<<FRACBITS);

			// Drawn by R_DrawColumn.
			// This stuff is a likely cause of the splitscreen water crash bug.
			// FIXTHIS: Figure out what "something more proper" is and do it.
			// quick fix... something more proper should be done!!!
			if (ylookup[dc->yl])
			{
				drawcolumndata_t dc_copy = *dc;
				coldrawfunc_t* colfunccopy = colfunc;
				colfunccopy(const_cast<drawcolumndata_t*>(&dc_copy));
			}
#ifdef PARANOIA
			else
				I_Error("R_DrawMaskedColumn: Invalid ylookup for dc_yl %d", dc->yl);
#endif
		}
		column = (column_t *)((UINT8 *)column + column->length + 4);
		if (brightmap != NULL)
		{
			brightmap = (column_t *)((UINT8 *)brightmap + brightmap->length + 4);
		}
	}

	dc->texturemid = basetexturemid;
}

INT32 lengthcol; // column->length : for flipped column function pointers and multi-patch on 2sided wall = texture->height

void R_DrawFlippedMaskedColumn(drawcolumndata_t* dc, column_t *column, column_t *brightmap, INT32 baseclip)
{
	INT32 topscreen;
	INT32 bottomscreen;
	fixed_t basetexturemid = dc->texturemid;
	INT32 topdelta, prevdelta = -1;
	UINT8 *d,*s;

	R_SetColumnFunc(colfunctype, brightmap != NULL);
	dc->brightmap = NULL;

	for (; column->topdelta != 0xff ;)
	{
		// calculate unclipped screen coordinates
		// for post
		topdelta = column->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = lengthcol-column->length-topdelta;
		topscreen = sprtopscreen + spryscale*topdelta;
		bottomscreen = sprbotscreen == INT32_MAX ? topscreen + spryscale*column->length
		                                      : sprbotscreen + spryscale*column->length;

		dc->yl = (topscreen+FRACUNIT-1)>>FRACBITS;
		dc->yh = (bottomscreen-1)>>FRACBITS;

		if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
		{
			if (windowtop > topscreen)
				dc->yl = (windowtop + FRACUNIT - 1)>>FRACBITS;
			if (windowbottom < bottomscreen)
				dc->yh = (windowbottom - 1)>>FRACBITS;
		}

		if (dc->yh >= mfloorclip[dc->x])
			dc->yh = mfloorclip[dc->x]-1;
		if (dc->yl <= mceilingclip[dc->x])
			dc->yl = mceilingclip[dc->x]+1;

		if (dc->yh >= baseclip && baseclip != -1)
			dc->yh = baseclip;

		if (dc->yl < 0)
			dc->yl = 0;
		if (dc->yh >= vid.height) // dc_yl must be < vid.height, so reduces number of checks in tight loop
			dc->yh = vid.height - 1;

		if (dc->yl <= dc->yh && dc->yh > 0 && column->length != 0)
		{
			dc->source = static_cast<UINT8*>(ZZ_Alloc(column->length));
			dc->sourcelength = column->length;
			for (s = (UINT8 *)column+2+column->length, d = dc->source; d < dc->source+column->length; --s)
				*d++ = *s;

			if (brightmap != NULL)
			{
				dc->brightmap = static_cast<UINT8*>(ZZ_Alloc(brightmap->length));
				for (s = (UINT8 *)brightmap+2+brightmap->length, d = dc->brightmap; d < dc->brightmap+brightmap->length; --s)
					*d++ = *s;
			}

			dc->texturemid = basetexturemid - (topdelta<<FRACBITS);

			// Still drawn by R_DrawColumn.
			if (ylookup[dc->yl])
			{
				drawcolumndata_t dc_copy = *dc;
				coldrawfunc_t* colfunccopy = colfunc;
				colfunccopy(const_cast<drawcolumndata_t*>(&dc_copy));
			}
#ifdef PARANOIA
			else
				I_Error("R_DrawMaskedColumn: Invalid ylookup for dc_yl %d", dc->yl);
#endif
			Z_Free(dc->source);
		}
		column = (column_t *)((UINT8 *)column + column->length + 4);
		if (brightmap != NULL)
		{
			brightmap = (column_t *)((UINT8 *)brightmap + brightmap->length + 4);
		}
	}

	dc->texturemid = basetexturemid;
}

static boolean hitlag_is_flashing(mobj_t *thing)
{
	return
		(thing->hitlag > 0) &&
		(thing->eflags & (MFE_DAMAGEHITLAG));
}

static boolean baddie_is_flashing(mobj_t *thing)
{
	return
		(thing->flags & (MF_ENEMY|MF_BOSS)) &&
		(thing->flags2 & (MF2_FRET)) &&
		!(thing->flags & MF_GRENADEBOUNCE);
}

boolean R_ThingIsFlashing(mobj_t *thing)
{
	return
		(thing->frame & FF_INVERT) ||
		hitlag_is_flashing(thing) ||
		baddie_is_flashing(thing);
}

boolean R_ThingIsUsingBakedOffsets(mobj_t* thing)
{
	return ((thing->bakexoff) || (thing->bakeyoff) || (thing->bakezoff) ||
			(thing->bakexpiv) || (thing->bakeypiv) || (thing->bakezpiv));
}

UINT8 *R_GetSpriteTranslation(vissprite_t *vis)
{
	if (vis->cut & SC_PRECIP)
	{
		// Simplified func, less safe properties to check
		if (vis->mobj->color)
			R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(vis->mobj->color), GTC_CACHE);
		return NULL;
	}

	size_t skinnum = TC_DEFAULT;

	if (vis->mobj->skin && vis->mobj->sprite == SPR_PLAY) // This thing is a player!
	{
		skinnum = ((skin_t*)vis->mobj->skin)->skinnum;

		// Hide not-yet-unlocked characters in replays from other people
		if (!R_CanShowSkinInDemo(skinnum))
		{
			skinnum = TC_BLINK;
		}
	}

	if (R_ThingIsFlashing(vis->mobj))
	{
		if (skinnum != (size_t)TC_BLINK)
			skinnum = TC_HITLAG;

		return R_GetTranslationColormap(skinnum, static_cast<skincolornum_t>(0), GTC_CACHE);
	}

	if (vis->mobj->color)
	{
		if (skinnum != (size_t)TC_BLINK && vis->mobj->colorized)
			skinnum = TC_RAINBOW;

		return R_GetTranslationColormap(skinnum, static_cast<skincolornum_t>(vis->mobj->color), GTC_CACHE);
	}

	return NULL;
}

//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
static void R_DrawVisSprite(vissprite_t *vis)
{
	column_t *column, *bmcol = NULL;
	void (*localcolfunc)(drawcolumndata_t*, column_t *, column_t *, INT32);
	INT32 texturecolumn;
	INT32 pwidth;
	fixed_t frac;
	patch_t *patch = vis->patch;
	patch_t *bmpatch = vis->bright;
	fixed_t this_scale = vis->thingscale;
	INT32 x1, x2;
	INT64 overflow_test;
	INT32 baseclip = -1;
	drawcolumndata_t dc {0};

	if (!patch)
		return;

	// Check for overflow
	overflow_test = (INT64)centeryfrac - (((INT64)vis->texturemid*vis->scale)>>FRACBITS);
	if (overflow_test < 0) overflow_test = -overflow_test;
	if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL) return; // fixed point mult would overflow

	if (vis->scalestep) // handles right edge too
	{
		overflow_test = (INT64)centeryfrac - (((INT64)vis->texturemid*(vis->scale + (vis->scalestep*(vis->x2 - vis->x1))))>>FRACBITS);
		if (overflow_test < 0) overflow_test = -overflow_test;
		if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL) return; // ditto
	}

	// TODO This check should not be necessary. But Papersprites near to the camera will sometimes create invalid values
	// for the vissprite's startfrac. This happens because they are not depth culled like other sprites.
	// Someone who is more familiar with papersprites pls check and try to fix <3
	if (vis->startfrac < 0 || vis->startfrac > (patch->width << FRACBITS))
	{
		// never draw vissprites with startfrac out of patch range
		return;
	}

	// Prevent an out of bounds error
	//
	// FIXME: The following check doesn't account for
	// differences in transparency between the patches.
	//
	// Sprite BRIGHTMAPs should be converted on load,
	// as like textures. I'm too tired to bother with it
	// right now, though.
	//
	if (bmpatch && (bmpatch->width != patch->width ||
				bmpatch->height != patch->height))
	{
		return;
	}

	R_SetColumnFunc(BASEDRAWFUNC, false); // hack: this isn't resetting properly somewhere.
	dc.colormap = vis->colormap;
	dc.fullbright = colormaps;
	dc.translation = R_GetSpriteTranslation(vis);

	// Hack: Use a special column function for drop shadows that bypasses
	// invalid memory access crashes caused by R_ProjectDropShadow putting wrong values
	// in dc_texturemid and dc_iscale when the shadow is sloped.
	if (vis->cut & SC_SHADOW)
	{
		R_SetColumnFunc(COLDRAWFUNC_DROPSHADOW, false);
		dc.transmap = vis->transmap;
		dc.shadowcolor = vis->color;
	}
	else if (!(vis->cut & SC_PRECIP) &&
			R_ThingIsFlashing(vis->mobj)) // Bosses "flash"
	{
		R_SetColumnFunc(COLDRAWFUNC_TRANS, false); // translate certain pixels to white
	}
	else if (vis->transmap && dc.translation) // Color mapping
	{
		R_SetColumnFunc(COLDRAWFUNC_TRANSTRANS, false);
		dc.transmap = vis->transmap;
	}
	else if (vis->transmap)
	{
		R_SetColumnFunc(COLDRAWFUNC_FUZZY, false);
		dc.transmap = vis->transmap;    //Fab : 29-04-98: translucency table
	}
	else if (dc.translation) // translate green skin to another color
		R_SetColumnFunc(COLDRAWFUNC_TRANS, false);

	if (vis->extra_colormap && !(vis->cut & SC_FULLBRIGHT) && !(vis->renderflags & RF_NOCOLORMAPS))
	{
		if (!dc.colormap)
			dc.colormap = vis->extra_colormap->colormap;
		else
			dc.colormap = &vis->extra_colormap->colormap[dc.colormap - colormaps];
	}
	if (!dc.colormap)
		dc.colormap = colormaps;

	dc.lightmap = colormaps;

	dc.fullbright = colormaps;

	if (encoremap && !vis->mobj->color && !(vis->mobj->flags & MF_DONTENCOREMAP))
	{
		dc.colormap += COLORMAP_REMAPOFFSET;
		dc.fullbright += COLORMAP_REMAPOFFSET;
	}

	dc.texturemid = vis->texturemid;
	dc.texheight = patch->height;

	frac = vis->startfrac;
	windowtop = windowbottom = sprbotscreen = INT32_MAX;

	if (this_scale <= 0)
		this_scale = 1;

	if (this_scale != FRACUNIT)
	{
		if (!(vis->cut & SC_ISSCALED))
		{
			vis->scale = FixedMul(vis->scale, this_scale);
			vis->scalestep = FixedMul(vis->scalestep, this_scale);
			vis->xiscale = FixedDiv(vis->xiscale,this_scale);
			vis->cut = static_cast<spritecut_e>(vis->cut | SC_ISSCALED);
		}
		dc.texturemid = FixedDiv(dc.texturemid,this_scale);
	}

	spryscale = vis->scale;

	if (!(vis->scalestep))
	{
		sprtopscreen = centeryfrac - FixedMul(dc.texturemid, spryscale);
		sprtopscreen += vis->shear.tan * vis->shear.offset;
		dc.iscale = FixedDiv(FRACUNIT, vis->scale);
	}

	if (vis->floorclip)
	{
		sprbotscreen = sprtopscreen + FixedMul(patch->height << FRACBITS, spryscale);
		baseclip = (sprbotscreen - FixedMul(vis->floorclip, spryscale)) >> FRACBITS;
	}
	else
	{
		baseclip = -1;
	}

	x1 = vis->x1;
	x2 = vis->x2;

	if (vis->x1 < 0)
	{
		spryscale += vis->scalestep*(-vis->x1);
		vis->x1 = 0;
	}

	if (vis->x2 >= vid.width)
		vis->x2 = vid.width-1;

	localcolfunc = (vis->cut & SC_VFLIP) ? R_DrawFlippedMaskedColumn : R_DrawMaskedColumn;
	lengthcol = patch->height;

	// Split drawing loops for paper and non-paper to reduce conditional checks per sprite
	if (vis->scalestep)
	{
		fixed_t horzscale = FixedMul(vis->spritexscale, this_scale);
		fixed_t scalestep = FixedMul(vis->scalestep, vis->spriteyscale);

		pwidth = patch->width;

		// Papersprite drawing loop
		for (dc.x = vis->x1; dc.x <= vis->x2; dc.x++, spryscale += scalestep)
		{
			angle_t angle = ((vis->centerangle + xtoviewangle[viewssnum][dc.x]) >> ANGLETOFINESHIFT) & 0xFFF;
			texturecolumn = (vis->paperoffset - FixedMul(FINETANGENT(angle), vis->paperdistance)) / horzscale;

			if (texturecolumn < 0 || texturecolumn >= pwidth)
				continue;

			if (vis->xiscale < 0) // Flipped sprite
				texturecolumn = pwidth - 1 - texturecolumn;

			sprtopscreen = (centeryfrac - FixedMul(dc.texturemid, spryscale));
			dc.iscale = (0xffffffffu / (unsigned)spryscale);

			column = (column_t *)((UINT8 *)patch->columns + (patch->columnofs[texturecolumn]));

			if (bmpatch)
				bmcol = (column_t *)((UINT8 *)bmpatch->columns + (bmpatch->columnofs[texturecolumn]));

			localcolfunc (&dc, column, bmcol, baseclip);
		}
	}
	else if (vis->cut & SC_SHEAR)
	{
		pwidth = patch->width;

		// Vertically sheared sprite
		for (dc.x = vis->x1; dc.x <= vis->x2; dc.x++, frac += vis->xiscale, dc.texturemid -= vis->shear.tan)
		{
			texturecolumn = std::clamp<fixed_t>(frac >> FRACBITS, 0, patch->width - 1);

			column = (column_t *)((UINT8 *)patch->columns + (patch->columnofs[texturecolumn]));
			if (bmpatch)
				bmcol = (column_t *)((UINT8 *)bmpatch->columns + (bmpatch->columnofs[texturecolumn]));

			sprtopscreen = (centeryfrac - FixedMul(dc.texturemid, spryscale));
			localcolfunc (&dc, column, bmcol, baseclip);
		}
	}
	else
	{

#if 0
		if (vis->x1test && vis->x2test)
		{
			INT32 x1test = vis->x1test;
			INT32 x2test = vis->x2test;

			if (x1test < 0)
				x1test = 0;

			if (x2test >= vid.width)
				x2test = vid.width-1;

			const INT32 t = (vis->startfrac + (vis->xiscale * (x2test - x1test))) >> FRACBITS;

			if (x1test <= x2test && (t < 0 || t >= patch->width))
			{
				CONS_Printf("THE GAME WOULD HAVE CRASHED, %d (old) vs %d (new)\n", (x2test - x1test), (vis->x2 - vis->x1));
			}
		}
#endif

		// Non-paper drawing loop
		for (dc.x = vis->x1; dc.x <= vis->x2; dc.x++, frac += vis->xiscale, sprtopscreen += vis->shear.tan)
		{
			texturecolumn = std::clamp<fixed_t>(frac >> FRACBITS, 0, patch->width - 1);

			column = (column_t *)((UINT8 *)patch->columns + (patch->columnofs[texturecolumn]));

			if (bmpatch)
				bmcol = (column_t *)((UINT8 *)bmpatch->columns + (bmpatch->columnofs[texturecolumn]));

			localcolfunc (&dc, column, bmcol, baseclip);
		}
	}

	R_SetColumnFunc(BASEDRAWFUNC, false);
	dc.hires = 0;

	vis->x1 = x1;
	vis->x2 = x2;
}

// Special precipitation drawer Tails 08-18-2002
static void R_DrawPrecipitationVisSprite(vissprite_t *vis)
{
	column_t *column;
	INT32 texturecolumn;
	fixed_t frac;
	patch_t *patch;
	fixed_t this_scale = vis->thingscale;
	INT64 overflow_test;
	drawcolumndata_t dc {0};

	//Fab : R_InitSprites now sets a wad lump number
	patch = vis->patch;
	if (!patch)
		return;

	// Check for overflow
	overflow_test = (INT64)centeryfrac - (((INT64)vis->texturemid*vis->scale)>>FRACBITS);
	if (overflow_test < 0) overflow_test = -overflow_test;
	if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL) return; // fixed point mult would overflow

	if (vis->transmap)
	{
		R_SetColumnFunc(COLDRAWFUNC_FUZZY, false);
		dc.transmap = vis->transmap;    //Fab : 29-04-98: translucency table
	}

	dc.colormap = colormaps;
	dc.fullbright = colormaps;
	if (encoremap)
	{
		dc.colormap += COLORMAP_REMAPOFFSET;
		dc.fullbright += COLORMAP_REMAPOFFSET;
	}

	dc.lightmap = colormaps;

	dc.iscale = FixedDiv(FRACUNIT, vis->scale);
	dc.texturemid = FixedDiv(vis->texturemid, this_scale);
	dc.texheight = patch->height;

	frac = vis->startfrac;
	spryscale = vis->scale;
	sprtopscreen = centeryfrac - FixedMul(dc.texturemid,spryscale);
	windowtop = windowbottom = sprbotscreen = INT32_MAX;

	if (vis->x1 < 0)
		vis->x1 = 0;

	if (vis->x2 >= vid.width)
		vis->x2 = vid.width-1;

	for (dc.x = vis->x1; dc.x <= vis->x2; dc.x++, frac += vis->xiscale)
	{
		texturecolumn = frac>>FRACBITS;

		if (texturecolumn < 0 || texturecolumn >= patch->width)
		{
			CONS_Debug(DBG_RENDER, "R_DrawPrecipitationSpriteRange: bad texturecolumn\n");
			break;
		}

		column = (column_t *)((UINT8 *)patch->columns + (patch->columnofs[texturecolumn]));

		R_DrawMaskedColumn(&dc, column, NULL, -1);
	}

	R_SetColumnFunc(BASEDRAWFUNC, false);
}

//
// R_SplitSprite
// runs through a sector's lightlist and Knuckles
static void R_SplitSprite(vissprite_t *sprite)
{
	INT32 i, lightnum, lindex;
	INT16 cutfrac;
	sector_t *sector;
	vissprite_t *newsprite;

	sector = sprite->sector;

	for (i = 1; i < sector->numlights; i++)
	{
		fixed_t testheight;

		if (!(sector->lightlist[i].caster->fofflags & FOF_CUTSPRITES))
			continue;

		testheight = P_GetLightZAt(&sector->lightlist[i], sprite->gx, sprite->gy);

		if (testheight >= sprite->gzt)
			continue;
		if (testheight <= sprite->gz)
			return;

		cutfrac = (INT16)((centeryfrac - FixedMul(testheight - viewz, sprite->sortscale))>>FRACBITS);
		if (cutfrac < 0)
			continue;
		if (cutfrac > viewheight)
			return;

		// Found a split! Make a new sprite, copy the old sprite to it, and
		// adjust the heights.
		newsprite = static_cast<vissprite_t*>(M_Memcpy(R_NewVisSprite(), sprite, sizeof (vissprite_t)));

		newsprite->cut = static_cast<spritecut_e>(newsprite->cut | (sprite->cut & SC_FLAGMASK));

		sprite->cut = static_cast<spritecut_e>(sprite->cut | SC_BOTTOM);
		sprite->gz = testheight;

		newsprite->gzt = sprite->gz;

		sprite->sz = cutfrac;
		newsprite->szt = (INT16)(sprite->sz - 1);

		newsprite->szt -= 8;

		newsprite->cut = static_cast<spritecut_e>(newsprite->cut | SC_TOP);
		if (!(sector->lightlist[i].caster->fofflags & FOF_NOSHADE))
		{
			lightnum = (*sector->lightlist[i].lightlevel >> LIGHTSEGSHIFT);

			if (lightnum < 0)
				spritelights = scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
				spritelights = scalelight[LIGHTLEVELS-1];
			else
				spritelights = scalelight[lightnum];

			newsprite->extra_colormap = *sector->lightlist[i].extra_colormap;

			if (!(newsprite->cut & SC_FULLBRIGHT)
				|| (newsprite->extra_colormap && (newsprite->extra_colormap->flags & CMF_FADEFULLBRIGHTSPRITES)))
			{
				lindex = FixedMul(sprite->xscale, LIGHTRESOLUTIONFIX)>>(LIGHTSCALESHIFT);

				if (lindex >= MAXLIGHTSCALE)
					lindex = MAXLIGHTSCALE-1;

				if (newsprite->cut & SC_SEMIBRIGHT)
					lindex = (MAXLIGHTSCALE/2) + (lindex >> 1);

				newsprite->colormap = spritelights[lindex];
			}
		}
		sprite = newsprite;
	}
}

static patch_t *R_CacheSpriteBrightMap(const spriteinfo_t *sprinfo, UINT8 frame)
{
	const char *name = sprinfo->bright[frame];

	if (name == NULL)
	{
		name = sprinfo->bright[SPRINFO_DEFAULT_PIVOT];
	}

	const lumpnum_t num = W_CheckNumForLongName(name);

	if (num == LUMPERROR)
	{
		return NULL;
	}

	return static_cast<patch_t*>(W_CachePatchNum(num, PU_SPRITE));
}

//
// R_GetShadowZ(thing, shadowslope)
// Get the first visible floor below the object for shadows
// shadowslope is filled with the floor's slope, if provided
//
fixed_t R_GetShadowZ(
	mobj_t *thing, pslope_t **shadowslope)
{
	fixed_t halfHeight;
	boolean isflipped = thing->eflags & MFE_VERTICALFLIP;
	fixed_t floorz;
	fixed_t ceilingz;
	fixed_t z, groundz = isflipped ? INT32_MAX : INT32_MIN;
	pslope_t *slope, *groundslope = NULL;
	msecnode_t *node;
	sector_t *sector;
	ffloor_t *rover;

	if (R_CustomShadowZ(thing, &groundz, &groundslope))
	{
		if (shadowslope != NULL)
			*shadowslope = groundslope;

		return groundz;
	}

	// for frame interpolation
	interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	halfHeight = interp.z + (thing->height >> 1);
	floorz = P_GetFloorZ(thing, interp.subsector->sector, interp.x, interp.y, NULL);
	ceilingz = P_GetCeilingZ(thing, interp.subsector->sector, interp.x, interp.y, NULL);

#define CHECKZ (isflipped ? z > halfHeight && z < groundz : z < halfHeight && z > groundz)

	for (node = thing->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		sector = node->m_sector;

		slope = sector->heightsec != -1 ? NULL : (isflipped ? sector->c_slope : sector->f_slope);

		if (sector->heightsec != -1)
			z = isflipped ? sectors[sector->heightsec].ceilingheight : sectors[sector->heightsec].floorheight;
		else
			z = isflipped ? P_GetSectorCeilingZAt(sector, interp.x, interp.y) : P_GetSectorFloorZAt(sector, interp.x, interp.y);

		if CHECKZ
		{
			groundz = z;
			groundslope = slope;
		}

		if (sector->ffloors)
			for (rover = sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERPLANES) || (rover->alpha < 90 && !(rover->fofflags & FOF_SWIMMABLE)))
					continue;

				z = isflipped ? P_GetFFloorBottomZAt(rover, interp.x, interp.y) : P_GetFFloorTopZAt(rover, interp.x, interp.y);
				if CHECKZ
				{
					groundz = z;
					groundslope = isflipped ? *rover->b_slope : *rover->t_slope;
				}
			}
	}

	if (isflipped ? (ceilingz < groundz - (!groundslope ? 0 : FixedMul(abs(groundslope->zdelta), thing->radius*3/2)))
		: (floorz > groundz + (!groundslope ? 0 : FixedMul(abs(groundslope->zdelta), thing->radius*3/2))))
	{
		groundz = isflipped ? ceilingz : floorz;
		groundslope = NULL;
	}

	if (shadowslope != NULL)
		*shadowslope = groundslope;

#undef CHECKZ

	return groundz;
}

static void R_SkewShadowSprite(
			mobj_t *thing, pslope_t *groundslope,
			fixed_t groundz, INT32 spriteheight, fixed_t scalemul,
			fixed_t *shadowyscale, fixed_t *shadowskew)
{

	// haha let's try some dumb stuff
	fixed_t xslope, zslope;
	angle_t sloperelang;

	// for frame interpolation
	interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	sloperelang = (R_PointToAngle(interp.x, interp.y) - groundslope->xydirection) >> ANGLETOFINESHIFT;

	xslope = FixedMul(FINESINE(sloperelang), groundslope->zdelta);
	zslope = FixedMul(FINECOSINE(sloperelang), groundslope->zdelta);

	//CONS_Printf("Shadow is sloped by %d %d\n", xslope, zslope);

	if (viewz < groundz)
		*shadowyscale += FixedMul(FixedMul(thing->radius*2 / spriteheight, scalemul), zslope);
	else
		*shadowyscale -= FixedMul(FixedMul(thing->radius*2 / spriteheight, scalemul), zslope);

	*shadowyscale = abs((*shadowyscale));
	*shadowskew = xslope;
}

static void R_ProjectDropShadow(
	mobj_t *thing, vissprite_t *vis,
	fixed_t scale, fixed_t tx, fixed_t tz)
{
	vissprite_t *shadow;
	patch_t *patch;
	fixed_t xscale, yscale, shadowxscale, shadowyscale, shadowskew, x1, x2;
	INT32 light = 0;
	fixed_t groundz;
	pslope_t *groundslope;
	interpmobjstate_t interp = {0};

	groundz = R_GetShadowZ(thing, &groundslope);

	if (abs(groundz-viewz)/tz > 4) return; // Prevent stretchy shadows and possible crashes

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	patch = static_cast<patch_t*>(W_CachePatchName("DSHADOW", PU_SPRITE));
	xscale = FixedDiv(projection[viewssnum], tz);
	yscale = FixedDiv(projectiony[viewssnum], tz);
	shadowxscale = FixedMul(thing->radius*2, scale);
	shadowyscale = FixedMul(FixedMul(thing->radius*2, scale), FixedDiv(abs(groundz - viewz), tz));
	shadowyscale = std::min(shadowyscale, shadowxscale) / patch->height;
	shadowxscale /= patch->width;
	shadowskew = 0;

	if (groundslope)
	{
		R_SkewShadowSprite(
			thing,
			groundslope, groundz,
			patch->height, FRACUNIT,
			&shadowyscale, &shadowskew);
	}

	tx -= patch->width * shadowxscale/2;
	x1 = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;
	if (x1 >= viewwidth) return;

	tx += patch->width * shadowxscale;
	x2 = ((centerxfrac + FixedMul(tx,xscale))>>FRACBITS); x2--;
	if (x2 < 0 || x2 <= x1) return;

	if (shadowyscale < FRACUNIT/patch->height) return; // fix some crashes?

	shadow = R_NewVisSprite();
	shadow->patch = patch;
	shadow->bright = NULL;
	shadow->heightsec = vis->heightsec;

	shadow->mobjflags = 0;
	shadow->sortscale = vis->sortscale;
	shadow->dispoffset = vis->dispoffset - 5;
	shadow->gx = interp.x;
	shadow->gy = interp.y;
	shadow->gzt = groundz + patch->height * shadowyscale / 2;
	shadow->gz = shadow->gzt - patch->height * shadowyscale;
	shadow->texturemid = FixedMul(interp.scale, FixedDiv(shadow->gzt - viewz, shadowyscale));
	shadow->scalestep = 0;
	shadow->shear.tan = shadowskew; // repurposed variable

	shadow->mobj = thing; // Easy access! Tails 06-07-2002

	shadow->x1 = x1 < portalclipstart ? portalclipstart : x1;
	shadow->x2 = x2 >= portalclipend ? portalclipend-1 : x2;

	shadow->x1test = 0;
	shadow->x2test = 0;

	shadow->xscale = FixedMul(xscale, shadowxscale); //SoM: 4/17/2000
	shadow->scale = FixedMul(yscale, shadowyscale);
	shadow->thingscale = interp.scale;
	shadow->sector = vis->sector;
	shadow->szt = (INT16)((centeryfrac - FixedMul(shadow->gzt - viewz, yscale))>>FRACBITS);
	shadow->sz = (INT16)((centeryfrac - FixedMul(shadow->gz - viewz, yscale))>>FRACBITS);
	shadow->cut = static_cast<spritecut_e>(SC_ISSCALED|SC_SHADOW); //check this

	shadow->startfrac = 0;
	//shadow->xiscale = 0x7ffffff0 / (shadow->xscale/2);
	shadow->xiscale = (patch->width<<FRACBITS)/(x2-x1+1); // fuck it

	if (shadow->x1 > x1)
		shadow->startfrac += shadow->xiscale*(shadow->x1-x1);

	// reusing x1 variable
	x1 += (x2-x1)/2;
	shadow->shear.offset = shadow->x1-x1;

	if (thing->renderflags & RF_NOCOLORMAPS)
		shadow->extra_colormap = NULL;
	else
	{
		if (thing->subsector->sector->numlights)
		{
			INT32 lightnum;
			light = thing->subsector->sector->numlights - 1;

			// R_GetPlaneLight won't work on sloped lights!
			for (lightnum = 1; lightnum < thing->subsector->sector->numlights; lightnum++) {
				fixed_t h = P_GetLightZAt(&thing->subsector->sector->lightlist[lightnum], interp.x, interp.y);
				if (h <= shadow->gzt) {
					light = lightnum - 1;
					break;
				}
			}
		}

		if (thing->subsector->sector->numlights)
			shadow->extra_colormap = *thing->subsector->sector->lightlist[light].extra_colormap;
		else
			shadow->extra_colormap = thing->subsector->sector->extra_colormap;
	}

	shadow->transmap = R_GetBlendTable(thing->whiteshadow ? AST_ADD : AST_SUBTRACT, 0);
	shadow->colormap = colormaps;

	shadow->color = thing->shadowcolor;

	objectsdrawn++;
}

static void R_ProjectBoundingBox(mobj_t *thing, vissprite_t *vis)
{
	fixed_t gx, gy;
	fixed_t tx, tz;

	vissprite_t *box;

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	if (!R_ThingBoundingBoxVisible(thing))
	{
		return;
	}

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	if (thing->flags & MF_PAPERCOLLISION)
	{
		// 0--1
		// start in the middle
		gx = interp.x - viewx;
		gy = interp.y - viewy;
	}
	else
	{
		// 1--3
		// |  |
		// 0--2
		// start in the (0) corner
		gx = interp.x - thing->radius - viewx;
		gy = interp.y - thing->radius - viewy;
	}

	tz = FixedMul(gx, viewcos) + FixedMul(gy, viewsin);

	// thing is behind view plane?
	// if parent vis is visible, ignore this
	if (!vis && (tz < FixedMul(MINZ, interp.scale)))
	{
		return;
	}

	tx = FixedMul(gx, viewsin) - FixedMul(gy, viewcos);

	// too far off the side?
	if (!vis && abs(tx) > FixedMul(tz, fovtan[viewssnum])<<2)
	{
		return;
	}

	box = R_NewVisSprite();
	box->mobj = thing;
	box->mobjflags = thing->flags;
	box->thingheight = thing->height;
	box->cut = SC_BBOX;

	box->gx = tx;
	box->gy = tz;

	if (thing->flags & MF_PAPERCOLLISION)
	{
		box->scale = FixedMul(thing->radius, FCOS(viewangle - interp.angle));
		box->xscale = FixedMul(thing->radius, FSIN(viewangle - interp.angle));
	}
	else
	{
		box->scale = 2 * FixedMul(thing->radius, viewsin);
		box->xscale = 2 * FixedMul(thing->radius, viewcos);
	}

	box->pz = interp.z;
	box->pzt = box->pz + box->thingheight;

	box->gzt = box->pzt;
	box->gz = box->pz;
	box->texturemid = box->gzt - viewz;

	if (vis)
	{
		box->x1 = vis->x1;
		box->x2 = vis->x2;
		box->szt = vis->szt;
		box->sz = vis->sz;

		box->sortscale = vis->sortscale; // link sorting to sprite
		box->dispoffset = vis->dispoffset + 5;
	}
	else
	{
		fixed_t xscale = FixedDiv(projection[viewssnum], tz);
		fixed_t yscale = FixedDiv(projectiony[viewssnum], tz);
		fixed_t top = (centeryfrac - FixedMul(box->texturemid, yscale));

		box->x1 = (centerxfrac + FixedMul(box->gx, xscale)) / FRACUNIT;
		box->x2 = box->x1;

		box->szt = top / FRACUNIT;
		box->sz = (top + FixedMul(box->thingheight, yscale)) / FRACUNIT;

		box->sortscale = yscale;
		box->dispoffset = 0;
	}

	box->x1test = 0;
	box->x2test = 0;
}

fixed_t R_GetSpriteDirectionalLighting(angle_t angle)
{
	// Copied from P_UpdateSegLightOffset
	const UINT8 contrast = std::min<UINT8>(std::max(0, maplighting.contrast - maplighting.backlight), UINT8_MAX);
	const fixed_t contrastFixed = ((fixed_t)contrast) * FRACUNIT;

	fixed_t light = FRACUNIT;
	fixed_t extralight = 0;

	light = FixedMul(FINECOSINE(angle >> ANGLETOFINESHIFT), FINECOSINE(maplighting.angle >> ANGLETOFINESHIFT))
		+ FixedMul(FINESINE(angle >> ANGLETOFINESHIFT), FINESINE(maplighting.angle >> ANGLETOFINESHIFT));
	light = (light + FRACUNIT) / 2;

	light = FixedMul(light, FRACUNIT - FSIN(abs(AngleDeltaSigned(angle, maplighting.angle)) / 2));

	extralight = -contrastFixed + FixedMul(light, contrastFixed * 2);

	return extralight;
}

//
// R_ProjectSprite
// Generates a vissprite for a thing
// if it might be visible.
//
static void R_ProjectSprite(mobj_t *thing)
{
	mobj_t *oldthing = thing;

	fixed_t tr_x, tr_y;
	fixed_t tx, tz;
	fixed_t xscale, yscale; //added : 02-02-98 : aaargll..if I were a math-guy!!!
	fixed_t sortscale, sortsplat = 0;
	fixed_t sort_x = 0, sort_y = 0, sort_z;

	INT32 x1, x2;
	INT32 x1test = 0, x2test = 0;

	spritedef_t *sprdef;
	spriteframe_t *sprframe;
#ifdef ROTSPRITE
	spriteinfo_t *sprinfo;
#endif
	size_t lump;

	size_t frame, rot;
	UINT16 flip;
	boolean vflip = (!(thing->eflags & MFE_VERTICALFLIP) != !R_ThingVerticallyFlipped(thing));
	boolean mirrored = thing->mirrored;
	boolean hflip = (!R_ThingHorizontallyFlipped(thing) != !mirrored);

	INT32 lindex;
	UINT32 blendmode;
	UINT32 trans;

	vissprite_t *vis;
	patch_t *patch;

	spritecut_e cut = SC_NONE;

	angle_t ang = 0; // compiler complaints
	fixed_t iscale;
	fixed_t scalestep;
	fixed_t offset, offset2;

	fixed_t sheartan = 0;
	fixed_t shadowscale = FRACUNIT;
	fixed_t basetx, basetz; // drop shadows

	boolean shadowdraw, shadoweffects, shadowskew;
	boolean splat = R_ThingIsFloorSprite(thing);
	boolean papersprite = (R_ThingIsPaperSprite(thing) && !splat);
	fixed_t paperoffset = 0, paperdistance = 0;
	angle_t centerangle = 0;

	INT32 dispoffset = thing->dispoffset;

	//SoM: 3/17/2000
	fixed_t gz = 0, gzt = 0;
	INT32 heightsec, phs;
	INT32 light = 0;
	lighttable_t **lights_array = spritelights;
	fixed_t this_scale;
	fixed_t spritexscale, spriteyscale;

	fixed_t floorClip = 0;

	// rotsprite
	fixed_t spr_width, spr_height;
	fixed_t spr_offset, spr_topoffset;

#ifdef ROTSPRITE
	patch_t *rotsprite = NULL;
	INT32 rollangle = 0;
	angle_t spriterotangle = 0;
	vector2_t visoffs;
#endif

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(oldthing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(oldthing, FRACUNIT, &interp);
	}

	this_scale = interp.scale;
	if (thing->skin && ((skin_t *)thing->skin)->highresscale != FRACUNIT)
		this_scale = FixedMul(this_scale, ((skin_t *)thing->skin)->highresscale);

	// hitlag vibrating
	if (thing->hitlag > 0 && (thing->eflags & MFE_DAMAGEHITLAG))
	{
		fixed_t mul = thing->hitlag * HITLAGJITTERS;

		if (leveltime & 1)
		{
			mul = -mul;
		}

		interp.x += FixedMul(thing->momx, mul);
		interp.y += FixedMul(thing->momy, mul);
		interp.z += FixedMul(thing->momz, mul);
	}

	// sprite offset
	interp.x += thing->sprxoff;
	interp.y += thing->spryoff;
	interp.z += thing->sprzoff;

	// transform the origin point
	tr_x = interp.x - viewx;
	tr_y = interp.y - viewy;

	basetz = tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin); // near/far distance

	// thing is behind view plane?
	if (!papersprite && (tz < FixedMul(MINZ, this_scale))) // papersprite clipping is handled later
		return;

	basetx = tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos); // sideways distance

	// too far off the side?
	if (!papersprite && abs(tx) > FixedMul(tz, fovtan[viewssnum])<<2) // papersprite clipping is handled later
		return;

	// aspect ratio stuff
	xscale = FixedDiv(projection[viewssnum], tz);
	sortscale = FixedDiv(projectiony[viewssnum], tz);

	// decide which patch to use for sprite relative to player
	if ((size_t)(thing->sprite) >= numsprites)
	{
		CONS_Debug(DBG_RENDER, "R_ProjectSprite: invalid sprite number %d\n", thing->sprite);
		return;
	}

	frame = thing->frame&FF_FRAMEMASK;

	//Fab : 02-08-98: 'skin' override spritedef currently used for skin
	if (thing->skin && thing->sprite == SPR_PLAY)
	{
		sprdef = &((skin_t *)thing->skin)->sprites[thing->sprite2];
#ifdef ROTSPRITE
		sprinfo = &((skin_t *)thing->skin)->sprinfo[thing->sprite2];
#endif
		if (frame >= sprdef->numframes) {
			CONS_Alert(CONS_ERROR, M_GetText("R_ProjectSprite: invalid skins[\"%s\"].sprites[%sSPR2_%s] frame %s\n"), ((skin_t *)thing->skin)->name, ((thing->sprite2 & FF_SPR2SUPER) ? "FF_SPR2SUPER|": ""), spr2names[(thing->sprite2 & ~FF_SPR2SUPER)], sizeu5(frame));
			thing->sprite = states[S_UNKNOWN].sprite;
			thing->frame = states[S_UNKNOWN].frame;
			sprdef = &sprites[thing->sprite];
#ifdef ROTSPRITE
			sprinfo = &spriteinfo[thing->sprite];
#endif
			frame = thing->frame&FF_FRAMEMASK;
		}
	}
	else
	{
		sprdef = &sprites[thing->sprite];
#ifdef ROTSPRITE
		sprinfo = &spriteinfo[thing->sprite];
#endif

		if (frame >= sprdef->numframes)
		{
			CONS_Alert(CONS_ERROR, M_GetText("R_ProjectSprite: invalid sprite frame %s/%s for %s\n"),
				sizeu1(frame), sizeu2(sprdef->numframes), sprnames[thing->sprite]);
			if (thing->sprite == thing->state->sprite && thing->frame == thing->state->frame)
			{
				thing->state->sprite = states[S_UNKNOWN].sprite;
				thing->state->frame = states[S_UNKNOWN].frame;
			}
			thing->sprite = states[S_UNKNOWN].sprite;
			thing->frame = states[S_UNKNOWN].frame;
			sprdef = &sprites[thing->sprite];
			sprinfo = &spriteinfo[thing->sprite];
			frame = thing->frame&FF_FRAMEMASK;
		}
	}

	sprframe = &sprdef->spriteframes[frame];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("R_ProjectSprite: sprframes NULL for sprite %d\n", thing->sprite);
#endif

	if (splat)
	{
		ang = R_PointToAngle2(0, viewz, 0, interp.z);
	}
	else if (sprframe->rotate != SRF_SINGLE || papersprite)
	{
		ang = R_PointToAngle (interp.x, interp.y) - interp.angle;
		if (mirrored)
			ang = InvAngle(ang);
	}

	if (sprframe->rotate == SRF_SINGLE)
	{
		// use single rotation for all views
		rot = 0;                        //Fab: for vis->patch below
		lump = sprframe->lumpid[0];     //Fab: see note above
		flip = sprframe->flip; 			// Will only be 0 or 0xFFFF
	}
	else
	{
		// choose a different rotation based on player view
		//ang = R_PointToAngle (interp.x, interp.y) - interpangle;

		if ((sprframe->rotate & SRF_RIGHT) && (ang < ANGLE_180)) // See from right
			rot = 6; // F7 slot
		else if ((sprframe->rotate & SRF_LEFT) && (ang >= ANGLE_180)) // See from left
			rot = 2; // F3 slot
		else if (sprframe->rotate & SRF_3DGE) // 16-angle mode
		{
			rot = (ang+ANGLE_180+ANGLE_11hh)>>28;
			rot = ((rot & 1)<<3)|(rot>>1);
		}
		else // Normal behaviour
			rot = (ang+ANGLE_202h)>>29;

		//Fab: lumpid is the index for spritewidth,spriteoffset... tables
		lump = sprframe->lumpid[rot];
		flip = sprframe->flip & (1<<rot);
	}

	I_Assert(lump < max_spritelumps);

	spr_width = spritecachedinfo[lump].width;
	spr_height = spritecachedinfo[lump].height;
	spr_offset = spritecachedinfo[lump].offset;
	spr_topoffset = spritecachedinfo[lump].topoffset;

	//Fab: lumppat is the lump number of the patch to use, this is different
	//     than lumpid for sprites-in-pwad : the graphics are patched
	patch = static_cast<patch_t*>(W_CachePatchNum(sprframe->lumppat[rot], PU_SPRITE));

#ifdef ROTSPRITE
	spriterotangle = R_SpriteRotationAngle(thing, NULL);

	if (spriterotangle
	&& !(splat && !(thing->renderflags & RF_NOSPLATROLLANGLE)))
	{
		if ((papersprite && ang >= ANGLE_180) != vflip)
		{
			rollangle = R_GetRollAngle(InvAngle(spriterotangle));
		}
		else
		{
			rollangle = R_GetRollAngle(spriterotangle);
		}
		rotsprite = Patch_GetRotatedSprite(sprframe, (thing->frame & FF_FRAMEMASK), rot, flip, false, sprinfo, rollangle);

		if (rotsprite != NULL)
		{
			patch = rotsprite;
			cut = static_cast<spritecut_e>(cut | SC_ISROTATED);

			spr_width = rotsprite->width << FRACBITS;
			spr_height = rotsprite->height << FRACBITS;
			spr_offset = rotsprite->leftoffset << FRACBITS;
			spr_topoffset = rotsprite->topoffset << FRACBITS;
			spr_topoffset += FEETADJUST;

			// flip -> rotate, not rotate -> flip
			flip = 0;
		}
	}
#endif

	flip = !flip != !hflip;

	// calculate edges of the shape
	spritexscale = interp.spritexscale;
	spriteyscale = interp.spriteyscale;
	if (spritexscale < 1 || spriteyscale < 1)
		return;

#ifdef ROTSPRITE
	// initialize and rotate pitch/roll vector
	visoffs.x = 0;
	visoffs.y = 0;

	const fixed_t visoffymul = (vflip ? -FRACUNIT : FRACUNIT);

	if (R_ThingIsUsingBakedOffsets(thing))
	{
		R_RotateSpriteOffsetsByPitchRoll(thing,
										 vflip,
										 hflip,
										 &visoffs);
	}
#endif

	if (thing->renderflags & RF_ABSOLUTEOFFSETS)
	{
		spr_offset = interp.spritexoffset;
#ifdef ROTSPRITE
		spr_topoffset = (interp.spriteyoffset + FixedDiv((visoffs.y * visoffymul),
																mapobjectscale));
#else
		spr_topoffset = interp.spriteyoffset;
#endif
	}
	else
	{
		SINT8 flipoffset = 1;

		if ((thing->renderflags & RF_FLIPOFFSETS) && flip)
			flipoffset = -1;

		spr_offset += (interp.spritexoffset) * flipoffset;
#ifdef ROTSPRITE
		spr_topoffset += (interp.spriteyoffset + FixedDiv((visoffs.y * visoffymul),
															mapobjectscale))
																* flipoffset;
#else
		spr_topoffset += interp.spriteyoffset * flipoffset;
#endif
	}

	if (flip)
		offset = spr_offset - spr_width;
	else
		offset = -spr_offset;

#ifdef ROTSPRITE
	if (visoffs.x)
	{
		offset -= FixedDiv((visoffs.x * FRACUNIT), mapobjectscale);
	}
#endif

	offset = FixedMul(offset, FixedMul(spritexscale, this_scale));
	offset2 = FixedMul(spr_width, FixedMul(spritexscale, this_scale));

	if (papersprite)
	{
		fixed_t xscale2, yscale2, cosmul, sinmul, tx2, tz2;
		INT32 range;

		if (ang >= ANGLE_180)
		{
			offset *= -1;
			offset2 *= -1;
		}

		cosmul = FINECOSINE(interp.angle >> ANGLETOFINESHIFT);
		sinmul = FINESINE(interp.angle >> ANGLETOFINESHIFT);

		tr_x += FixedMul(offset, cosmul);
		tr_y += FixedMul(offset, sinmul);
		tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);

		tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos);

		// Get paperoffset (offset) and paperoffset (distance)
		paperoffset = -FixedMul(tr_x, cosmul) - FixedMul(tr_y, sinmul);
		paperdistance = -FixedMul(tr_x, sinmul) + FixedMul(tr_y, cosmul);
		if (paperdistance < 0)
		{
			paperoffset = -paperoffset;
			paperdistance = -paperdistance;
		}
		centerangle = viewangle - interp.angle;

		tr_x += FixedMul(offset2, cosmul);
		tr_y += FixedMul(offset2, sinmul);
		tz2 = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);

		tx2 = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos);

		if (std::max(tz, tz2) < FixedMul(MINZ, this_scale)) // non-papersprite clipping is handled earlier
			return;

		// Needs partially clipped
		if (tz < FixedMul(MINZ, this_scale))
		{
			fixed_t div = FixedDiv(tz2-tz, FixedMul(MINZ, this_scale)-tz);
			tx += FixedDiv(tx2-tx, div);
			tz = FixedMul(MINZ, this_scale);
		}
		else if (tz2 < FixedMul(MINZ, this_scale))
		{
			fixed_t div = FixedDiv(tz-tz2, FixedMul(MINZ, this_scale)-tz2);
			tx2 += FixedDiv(tx-tx2, div);
			tz2 = FixedMul(MINZ, this_scale);
		}

		if ((tx2 / 4) < -(FixedMul(tz2, fovtan[viewssnum])) || (tx / 4) > FixedMul(tz, fovtan[viewssnum])) // too far off the side?
			return;

		yscale = FixedDiv(projectiony[viewssnum], tz);
		xscale = FixedDiv(projection[viewssnum], tz);

		x1 = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;

		// off the right side?
		if (x1 > viewwidth)
			return;

		yscale2 = FixedDiv(projectiony[viewssnum], tz2);
		xscale2 = FixedDiv(projection[viewssnum], tz2);

		x2 = (centerxfrac + FixedMul(tx2,xscale2))>>FRACBITS;

		// off the left side
		if (x2 < 0)
			return;

		range = x2 - x1;
		if (range < 0)
		{
			return;
		}

		range++; // fencepost problem

		if (range > 32767)
		{
			// If the range happens to be too large for fixed_t,
			// abort the draw to avoid xscale becoming negative due to arithmetic overflow.
			return;
		}

		scalestep = ((yscale2 - yscale)/range);

		if (scalestep == 0)
			scalestep = 1;

		xscale = FixedDiv(range<<FRACBITS, abs(offset2));

		// The following two are alternate sorting methods which might be more applicable in some circumstances. TODO - maybe enable via MF2?
		// sortscale = max(yscale, yscale2);
		// sortscale = min(yscale, yscale2);
	}
	else
	{
		scalestep = 0;
		yscale = sortscale;
		tx += offset;
		//x1 = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;
		x1 = centerx + (FixedMul(tx,xscale) / FRACUNIT);

		x1test = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;

		if (x1test > viewwidth)
			x1test = 0;

		// off the right side?
		if (x1 > viewwidth)
			return;

		tx += offset2;
		//x2 = ((centerxfrac + FixedMul(tx,xscale))>>FRACBITS); x2--;
		x2 = (centerx + (FixedMul(tx,xscale) / FRACUNIT)) - 1;

		x2test = ((centerxfrac + FixedMul(tx,xscale))>>FRACBITS) - 1;

		if (x2test < 0)
			x2test = 0;

		// off the left side
		if (x2 < 0)
			return;

#if 0
		if ((x2 - x1) != (x2test - x1test))
			CONS_Printf("[%d] %d != %d\n", objectsdrawn, x2 - x1, x2test - x1test);
#endif
	}

	// Adjust the sort scale if needed
	if (splat)
	{
		sort_z = (patch->height - patch->topoffset) * FRACUNIT;
		ang = (viewangle >> ANGLETOFINESHIFT);
		sort_x = FixedMul(FixedMul(FixedMul(spritexscale, this_scale), sort_z), FINECOSINE(ang));
		sort_y = FixedMul(FixedMul(FixedMul(spriteyscale, this_scale), sort_z), FINESINE(ang));
	}

	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer) // toast 16/09/16 (SYMMETRY)
	{
		interpmobjstate_t tracer_interp = {0};
		fixed_t linkscale;

		thing = thing->tracer;

		if (R_UsingFrameInterpolation() && !paused)
		{
			R_InterpolateMobjState(thing, rendertimefrac, &tracer_interp);
		}
		else
		{
			R_InterpolateMobjState(thing, FRACUNIT, &tracer_interp);
		}

		// hitlag vibrating
		if (thing->hitlag > 0 && (thing->eflags & MFE_DAMAGEHITLAG))
		{
			// previous code multiplied by (FRACUNIT / 10) instead of HITLAGJITTERS, um wadaflip
			fixed_t jitters = HITLAGJITTERS;
			if (R_UsingFrameInterpolation() && !paused)
				jitters += (rendertimefrac / HITLAGDIV);
			
			fixed_t mul = thing->hitlag * jitters;

			if (leveltime & 1)
			{
				mul = -mul;
			}

			tracer_interp.x += FixedMul(thing->momx, mul);
			tracer_interp.y += FixedMul(thing->momy, mul);
			tracer_interp.z += FixedMul(thing->momz, mul);
		}

		// sprite offset
		tracer_interp.x += thing->sprxoff;
		tracer_interp.y += thing->spryoff;
		tracer_interp.z += thing->sprzoff;

		if (! R_ThingVisible(thing))
			return;

		tr_x = (tracer_interp.x + sort_x) - viewx;
		tr_y = (tracer_interp.y + sort_y) - viewy;
		tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);
		linkscale = FixedDiv(projectiony[viewssnum], tz);

		if (tz < FixedMul(MINZ, this_scale))
			return;

		if (sortscale < linkscale)
			dispoffset *= -1; // if it's physically behind, make sure it's ordered behind (if dispoffset > 0)

		sortscale = linkscale; // now make sure it's linked
		cut = static_cast<spritecut_e>(cut | SC_LINKDRAW);
	}
	else if (splat)
	{
		tr_x = (interp.x + sort_x) - viewx;
		tr_y = (interp.y + sort_y) - viewy;
		sort_z = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);
		sortscale = FixedDiv(projectiony[viewssnum], sort_z);
	}

	// Calculate the splat's sortscale
	if (splat)
	{
		tr_x = (interp.x - sort_x) - viewx;
		tr_y = (interp.y - sort_y) - viewy;
		sort_z = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);
		sortsplat = FixedDiv(projectiony[viewssnum], sort_z);
		centerangle = interp.angle;
	}

	// PORTAL SPRITE CLIPPING
	if (portalrender && portalclipline)
	{
		if (x2 < portalclipstart || x1 >= portalclipend)
			return;

		if (x2test < portalclipstart || x1test >= portalclipend)
		{
			x1test = 0;
			x2test = 0;
		}

		if (P_PointOnLineSide(interp.x, interp.y, portalclipline) != 0)
			return;
	}

	// Determine the blendmode and translucency value
	{
		if (oldthing->renderflags & RF_BLENDMASK)
			blendmode = (oldthing->renderflags & RF_BLENDMASK) >> RF_BLENDSHIFT;
		else
			blendmode = (oldthing->frame & FF_BLENDMASK) >> FF_BLENDSHIFT;
		if (blendmode)
			blendmode++; // realign to constants

		if (oldthing->renderflags & RF_TRANSMASK)
			trans = (oldthing->renderflags & RF_TRANSMASK) >> RF_TRANSSHIFT;
		else
			trans = (oldthing->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;
		if (trans >= NUMTRANSMAPS)
			return; // cap
	}

	// Check if this sprite needs to be rendered like a shadow
	shadowdraw = (!!(thing->renderflags & RF_SHADOWDRAW) && !(papersprite || splat));
	shadoweffects = (thing->renderflags & RF_SHADOWEFFECTS);
	shadowskew = (shadowdraw && thing->standingslope);

	if (shadowdraw || shadoweffects)
	{
		fixed_t groundz = R_GetShadowZ(thing, NULL);
		boolean isflipped = (thing->eflags & MFE_VERTICALFLIP);

		if (shadoweffects)
		{
			; // KART TODO: do additive/subtractive styles here
		}

		if (shadowdraw)
		{
			spritexscale = FixedMul(thing->radius * 2, FixedMul(shadowscale, spritexscale));
			spriteyscale = FixedMul(thing->radius * 2, FixedMul(shadowscale, spriteyscale));
			spriteyscale = FixedMul(spriteyscale, FixedDiv(abs(groundz - viewz), tz));
			spriteyscale = std::min(spriteyscale, spritexscale) / patch->height;
			spritexscale /= patch->width;
		}
		else
		{
			spritexscale = FixedMul(shadowscale, spritexscale);
			spriteyscale = FixedMul(shadowscale, spriteyscale);
		}

		if (shadowskew)
		{
			R_SkewShadowSprite(thing, thing->standingslope, groundz, patch->height, shadowscale, &spriteyscale, &sheartan);

			gzt = (isflipped ? (interp.z + thing->height) : interp.z) + patch->height * spriteyscale / 2;
			gz = gzt - patch->height * spriteyscale;

			cut = static_cast<spritecut_e>(cut | SC_SHEAR);
		}
	}

	if (!shadowskew)
	{
		//SoM: 3/17/2000: Disregard sprites that are out of view..
		if (vflip)
		{
			// When vertical flipped, draw sprites from the top down, at least as far as offsets are concerned.
			// sprite height - sprite topoffset is the proper inverse of the vertical offset, of course.
			// remember gz and gzt should be seperated by sprite height, not thing height - thing height can be shorter than the sprite itself sometimes!
			gz = interp.z + oldthing->height - FixedMul(spr_topoffset, FixedMul(spriteyscale, this_scale));
			gzt = gz + FixedMul(spr_height, FixedMul(spriteyscale, this_scale));
		}
		else
		{
			gzt = interp.z + FixedMul(spr_topoffset, FixedMul(spriteyscale, this_scale));
			gz = gzt - FixedMul(spr_height, FixedMul(spriteyscale, this_scale));
		}
	}

	if (thing->subsector->sector->cullheight)
	{
		if (R_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, viewz, gz, gzt))
			return;
	}

	if (oldthing->renderflags & RF_ABSOLUTELIGHTLEVEL)
	{
		const UINT8 n = R_ThingLightLevel(oldthing);

		// n = uint8 aka 0 - 255, so the shift will always be 0 - LIGHTLEVELS - 1
		lights_array = scalelight[n >> LIGHTSEGSHIFT];
	}
	else
	{
		INT32 lightnum;

		if (thing->subsector->sector->numlights)
		{
			fixed_t top = (splat) ? gz : gzt;
			light = thing->subsector->sector->numlights - 1;

			// R_GetPlaneLight won't work on sloped lights!
			for (lightnum = 1; lightnum < thing->subsector->sector->numlights; lightnum++) {
				fixed_t h = P_GetLightZAt(&thing->subsector->sector->lightlist[lightnum], interp.x, interp.y);
				if (h <= top) {
					light = lightnum - 1;
					break;
				}
			}
			//light = R_GetPlaneLight(thing->subsector->sector, gzt, false);
			lightnum = *thing->subsector->sector->lightlist[light].lightlevel;
		}
		else
		{
			lightnum = thing->subsector->sector->lightlevel;
		}

		lightnum = (lightnum + R_ThingLightLevel(oldthing)) >> LIGHTSEGSHIFT;

		if (maplighting.directional == true && P_SectorUsesDirectionalLighting(thing->subsector->sector))
		{
			fixed_t extralight = R_GetSpriteDirectionalLighting(papersprite
					? interp.angle + (ang >= ANGLE_180 ? -ANGLE_90 : ANGLE_90)
					: R_PointToAngle(interp.x, interp.y));

			// Krangle contrast in 3P/4P because scalelight
			// scales differently depending on the screen
			// width (which is halved in 3P/4P).
			if (r_splitscreen > 1)
			{
				extralight *= 2;
			}

			// Less change in contrast in dark sectors
			extralight = FixedMul(extralight, std::min<INT32>(std::max<INT32>(0, lightnum), LIGHTLEVELS - 1) * FRACUNIT / (LIGHTLEVELS - 1));

			if (papersprite)
			{
				// Papersprite contrast should match walls
				lightnum += FixedFloor((extralight / 8) + (FRACUNIT / 2)) / FRACUNIT;
			}
			else
			{
				fixed_t n = FixedDiv(FixedMul(xscale, LIGHTRESOLUTIONFIX), ((MAXLIGHTSCALE-1) << LIGHTSCALESHIFT));

				// Less change in contrast at further distances, to counteract DOOM diminished light
				extralight = FixedMul(extralight, std::min<fixed_t>(n, FRACUNIT));

				// Contrast is stronger for normal sprites, stronger than wall lighting is at the same distance
				lightnum += FixedFloor((extralight / 4) + (FRACUNIT / 2)) / FRACUNIT;
			}

			// Semibright objects will be made slightly brighter to compensate contrast
			if (R_ThingIsSemiBright(oldthing))
			{
				lightnum += 2;
			}
		}

		if (lightnum < 0)
			lights_array = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			lights_array = scalelight[LIGHTLEVELS-1];
		else
			lights_array = scalelight[lightnum];
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer && viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1) // only clip things which are in special sectors
	{
		if (viewz < sectors[phs].floorheight ?
		interp.z >= sectors[heightsec].floorheight :
		gzt < sectors[heightsec].floorheight)
			return;
		if (viewz > sectors[phs].ceilingheight ?
		gzt < sectors[heightsec].ceilingheight && viewz >= sectors[heightsec].ceilingheight :
		interp.z >= sectors[heightsec].ceilingheight)
			return;
	}

	if (thing->terrain != NULL && (thing->flags & MF_APPLYTERRAIN))
	{
		// Clip the bottom of the thing's sprite
		floorClip = thing->terrain->floorClip;
	}

	// store information in a vissprite
	vis = R_NewVisSprite();
	vis->renderflags = thing->renderflags;
	vis->rotateflags = sprframe->rotate;
	vis->heightsec = heightsec; //SoM: 3/17/2000
	vis->mobjflags = thing->flags;
	vis->sortscale = sortscale;
	vis->sortsplat = sortsplat;
	vis->dispoffset = dispoffset; // Monster Iestyn: 23/11/15
	vis->gx = interp.x;
	vis->gy = interp.y;
	vis->gz = gz;
	vis->gzt = gzt;
	vis->thingheight = thing->height;
	vis->pz = interp.z;
	vis->pzt = vis->pz + vis->thingheight;
	vis->floorclip = floorClip;
	vis->texturemid = FixedDiv(gzt - viewz - FixedMul(vis->floorclip, mapobjectscale), spriteyscale);
	vis->scalestep = scalestep;
	vis->paperoffset = paperoffset;
	vis->paperdistance = paperdistance;
	vis->centerangle = centerangle;
	vis->shear.tan = sheartan;
	vis->shear.offset = 0;
	vis->viewpoint.x = viewx;
	vis->viewpoint.y = viewy;
	vis->viewpoint.z = viewz;
	vis->viewpoint.angle = viewangle;

	vis->mobj = thing; // Easy access! Tails 06-07-2002

	vis->x1 = x1 < portalclipstart ? portalclipstart : x1;
	vis->x2 = x2 >= portalclipend ? portalclipend-1 : x2;

	vis->x1test = x1test < portalclipstart ? portalclipstart : x1test;
	vis->x2test = x2test >= portalclipend ? portalclipend-1 : x2test;

	vis->sector = thing->subsector->sector;
	vis->szt = (INT16)((centeryfrac - FixedMul(vis->gzt - viewz, sortscale))>>FRACBITS);
	vis->sz = (INT16)((centeryfrac - FixedMul(vis->gz - viewz, sortscale))>>FRACBITS);
	vis->cut = cut;

	if (thing->subsector->sector->numlights)
		vis->extra_colormap = *thing->subsector->sector->lightlist[light].extra_colormap;
	else
		vis->extra_colormap = thing->subsector->sector->extra_colormap;

	vis->xscale = FixedMul(spritexscale, xscale); //SoM: 4/17/2000
	vis->scale = FixedMul(spriteyscale, yscale); //<<detailshift;
	vis->thingscale = this_scale;

	vis->spritexscale = spritexscale;
	vis->spriteyscale = spriteyscale;
	vis->spritexoffset = spr_offset;
	vis->spriteyoffset = spr_topoffset;

	if (shadowdraw || shadoweffects)
	{
		if (x1test && x2test)
		{
			vis->xiscaletest = (patch->width<<FRACBITS)/(x2test-x1test+1); // fuck it
			x1test += (x2test-x1test)/2; // reusing x1 variable
		}

		iscale = (patch->width<<FRACBITS)/(x2-x1+1); // fuck it
		x1 += (x2-x1)/2; // reusing x1 variable
		vis->shear.offset = vis->x1-x1;
	}
	else
		iscale = FixedDiv(FRACUNIT, vis->xscale);

	vis->shadowscale = shadowscale;

	if (flip)
	{
		vis->startfrac = spr_width-1;
		vis->xiscale = -iscale;
		vis->xiscaletest = -vis->xiscaletest;
	}
	else
	{
		vis->startfrac = 0;
		vis->xiscale = iscale;
	}

	vis->startfractest = vis->startfrac;

	if (vis->x1 > x1)
	{
		vis->startfrac += FixedDiv(vis->xiscale, this_scale) * (vis->x1 - x1);
		vis->scale += FixedMul(scalestep, spriteyscale) * (vis->x1 - x1);
		vis->startfractest += FixedDiv(vis->xiscaletest, this_scale) * (vis->x1test - x1test);
	}

	vis->transmap = R_GetBlendTable(blendmode, trans);

	if (R_ThingIsSemiBright(oldthing))
		vis->cut = static_cast<spritecut_e>(vis->cut | SC_SEMIBRIGHT);
	else if (R_ThingIsFullBright(oldthing))
		vis->cut = static_cast<spritecut_e>(vis->cut | SC_FULLBRIGHT);
	else if (R_ThingIsFullDark(oldthing))
		vis->cut = static_cast<spritecut_e>(vis->cut | SC_FULLDARK);

	//
	// determine the colormap (lightlevel & special effects)
	//
	if (vis->cut & SC_FULLBRIGHT)
	{
		// full bright: goggles
		vis->colormap = colormaps;
	}
	else if (vis->cut & SC_FULLDARK)
	{
		vis->colormap = scalelight[0][0];
	}
	else
	{
		// diminished light
		lindex = FixedMul(xscale, LIGHTRESOLUTIONFIX)>>(LIGHTSCALESHIFT);

		// Mitigate against negative xscale and arithmetic overflow
		lindex = std::clamp<INT32>(lindex, 0, MAXLIGHTSCALE - 1);

		if (vis->cut & SC_SEMIBRIGHT)
			lindex = (MAXLIGHTSCALE/2) + (lindex >> 1);

		vis->colormap = lights_array[lindex];
	}

	if (vflip)
		vis->cut = static_cast<spritecut_e>(vis->cut | SC_VFLIP);
	if (splat)
		vis->cut = static_cast<spritecut_e>(vis->cut | SC_SPLAT); // I like ya cut g

	vis->patch = patch;
	vis->bright = R_CacheSpriteBrightMap(sprinfo, frame);

	if (thing->subsector->sector->numlights && !(shadowdraw || splat) && !(thing->renderflags & RF_ABSOLUTELIGHTLEVEL))
		R_SplitSprite(vis);

	if (oldthing->shadowscale && cv_shadow.value)
	{
		R_ProjectDropShadow(oldthing, vis, oldthing->shadowscale, basetx, basetz);
	}

	R_ProjectBoundingBox(oldthing, vis);

	// Debug
	++objectsdrawn;
}

static void R_ProjectPrecipitationSprite(precipmobj_t *thing)
{
	fixed_t tr_x, tr_y;
	fixed_t tx, tz;
	fixed_t xscale, yscale; //added : 02-02-98 : aaargll..if I were a math-guy!!!

	INT32 x1, x2;

	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lump;

	vissprite_t *vis;

	fixed_t iscale;

	//SoM: 3/17/2000
	fixed_t gz, gzt;
	fixed_t this_scale;

	UINT32 blendmode;
	UINT32 trans;

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	// okay... this is a hack, but weather isn't networked, so it should be ok
	if (!P_PrecipThinker(thing))
	{
		return;
	}

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolatePrecipMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolatePrecipMobjState(thing, FRACUNIT, &interp);
	}

	this_scale = interp.scale;

	// transform the origin point
	tr_x = interp.x - viewx;
	tr_y = interp.y - viewy;

	tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin); // near/far distance

	// thing is behind view plane?
	if (tz < FixedMul(MINZ, this_scale))
		return;

	tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos); // sideways distance

	// too far off the side?
	if (abs(tx) > FixedMul(tz, fovtan[viewssnum])<<2)
		return;

	// aspect ratio stuff :
	xscale = FixedDiv(projection[viewssnum], tz);
	yscale = FixedDiv(projectiony[viewssnum], tz);

	// decide which patch to use for sprite relative to player
	if ((unsigned)thing->sprite >= numsprites)
	{
		CONS_Debug(DBG_RENDER, "R_ProjectPrecipitationSprite: invalid sprite number %d\n",
			thing->sprite);
		return;
	}

	sprdef = &sprites[thing->sprite];

	if ((UINT8)(thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
	{
		CONS_Debug(DBG_RENDER, "R_ProjectPrecipitationSprite: invalid sprite frame %d : %d for %s\n",
			thing->sprite, thing->frame, sprnames[thing->sprite]);
		return;
	}

	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("R_ProjectPrecipitationSprite: sprframes NULL for sprite %d\n", thing->sprite);
#endif

	// use single rotation for all views
	lump = sprframe->lumpid[0];     //Fab: see note above

	// calculate edges of the shape
	tx -= FixedMul(spritecachedinfo[lump].offset, this_scale);
	x1 = (centerxfrac + FixedMul (tx,xscale)) >>FRACBITS;

	// off the right side?
	if (x1 > viewwidth)
		return;

	tx += FixedMul(spritecachedinfo[lump].width, this_scale);
	x2 = ((centerxfrac + FixedMul (tx,xscale)) >>FRACBITS) - 1;

	// off the left side
	if (x2 < 0)
		return;

	// PORTAL SPRITE CLIPPING
	if (portalrender && portalclipline)
	{
		if (x2 < portalclipstart || x1 >= portalclipend)
			return;

		if (P_PointOnLineSide(interp.x, interp.y, portalclipline) != 0)
			return;
	}

	//SoM: 3/17/2000: Disregard sprites that are out of view..
	gzt = interp.z + FixedMul(spritecachedinfo[lump].topoffset, this_scale);
	gz = gzt - FixedMul(spritecachedinfo[lump].height, this_scale);

	if (thing->subsector->sector->cullheight)
	{
		if (R_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, viewz, gz, gzt))
			return;
	}

	// Determine the blendmode and translucency value
	{
		blendmode = (thing->frame & FF_BLENDMASK) >> FF_BLENDSHIFT;
		if (blendmode)
			blendmode++; // realign to constants

		trans = (thing->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;
		if (trans >= NUMTRANSMAPS)
			return; // cap
	}

	// store information in a vissprite
	vis = R_NewVisSprite();
	vis->scale = FixedMul(yscale, this_scale);
	vis->sortscale = yscale; //<<detailshift;
	vis->thingscale = interp.scale;
	vis->dispoffset = 0; // Monster Iestyn: 23/11/15
	vis->gx = interp.x;
	vis->gy = interp.y;
	vis->gz = gz;
	vis->gzt = gzt;
	vis->thingheight = 4*FRACUNIT;
	vis->pz = interp.z;
	vis->pzt = vis->pz + vis->thingheight;
	vis->floorclip = 0;
	vis->texturemid = vis->gzt - viewz;
	vis->scalestep = 0;
	vis->paperdistance = 0;
	vis->shear.tan = 0;
	vis->shear.offset = 0;

	vis->x1 = x1 < portalclipstart ? portalclipstart : x1;
	vis->x2 = x2 >= portalclipend ? portalclipend-1 : x2;

	vis->x1test = 0;
	vis->x2test = 0;

	vis->xscale = xscale; //SoM: 4/17/2000
	vis->sector = thing->subsector->sector;
	vis->szt = (INT16)((centeryfrac - FixedMul(vis->gzt - viewz, yscale))>>FRACBITS);
	vis->sz = (INT16)((centeryfrac - FixedMul(vis->gz - viewz, yscale))>>FRACBITS);

	iscale = FixedDiv(FRACUNIT, xscale);

	vis->startfrac = 0;
	vis->xiscale = FixedDiv(iscale, this_scale);

	if (vis->x1 > x1)
		vis->startfrac += vis->xiscale*(vis->x1-x1);

	//Fab: lumppat is the lump number of the patch to use, this is different
	//     than lumpid for sprites-in-pwad : the graphics are patched
	vis->patch = static_cast<patch_t*>(W_CachePatchNum(sprframe->lumppat[0], PU_SPRITE));
	vis->bright = R_CacheSpriteBrightMap(&spriteinfo[thing->sprite],
			thing->frame & FF_FRAMEMASK);

	vis->transmap = R_GetBlendTable(blendmode, trans);

	vis->mobj = (mobj_t *)thing;
	vis->mobjflags = 0;
	vis->cut = SC_PRECIP;
	vis->extra_colormap = thing->subsector->sector->extra_colormap;
	vis->heightsec = thing->subsector->sector->heightsec;

	// Fullbright
	vis->colormap = colormaps;
}

// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites(sector_t *sec, INT32 lightlevel)
{
	mobj_t *thing;
	INT32 lightnum;
	fixed_t limit_dist;

	if (rendermode == render_opengl)
		return;

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if (sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	if (!sec->numlights)
	{
		if (sec->heightsec == -1) lightlevel = sec->lightlevel;

		lightnum = (lightlevel >> LIGHTSEGSHIFT);

		if (lightnum < 0)
			spritelights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			spritelights = scalelight[LIGHTLEVELS-1];
		else
			spritelights = scalelight[lightnum];
	}

	// Handle all things in sector.
	// If a limit exists, handle things a tiny bit different.
	limit_dist = (fixed_t)(cv_drawdist.value) * mapobjectscale;
	for (thing = sec->thinglist; thing; thing = thing->snext)
	{
		// do not render in skybox
		if ((thing->renderflags & RF_HIDEINSKYBOX) && portalskipprecipmobjs)
		{
			continue;
		}

		if (R_ThingWithinDist(thing, limit_dist))
		{
			const INT32 oldobjectsdrawn = objectsdrawn;

			if (R_ThingVisible(thing))
			{
				R_ProjectSprite(thing);
			}

			// I'm so smart :^)
			if (objectsdrawn == oldobjectsdrawn)
			{
				/*
				Object is invisible OR is off screen but
				render its bbox even if the latter because
				radius could be bigger than sprite.
				*/
				R_ProjectBoundingBox(thing, NULL);
			}
		}
	}
}

// R_AddPrecipitationSprites
// This renders through the blockmap instead of BSP to avoid
// iterating a huge amount of precipitation sprites in sectors
// that are beyond drawdist.
//
void R_AddPrecipitationSprites(void)
{
	const fixed_t drawdist = cv_drawdist_precip.value * mapobjectscale;

	INT32 xl, xh, yl, yh, bx, by;
	precipmobj_t *th, *next;

	// no, no infinite draw distance for precipitation. this option at zero is supposed to turn it off
	if (drawdist == 0)
	{
		return;
	}

	// do not render in skybox
	if (portalskipprecipmobjs)
	{
		return;
	}

	R_GetRenderBlockMapDimensions(drawdist, &xl, &xh, &yl, &yh);

	for (bx = xl; bx <= xh; bx++)
	{
		for (by = yl; by <= yh; by++)
		{
			for (th = precipblocklinks[(by * bmapwidth) + bx]; th; th = next)
			{
				// Store this beforehand because R_ProjectPrecipitionSprite may free th (see P_PrecipThinker)
				next = th->bnext;

				if (R_PrecipThingVisible(th))
				{
					R_ProjectPrecipitationSprite(th);
				}
			}
		}
	}
}

//
// R_SortVisSprites
//
static void R_SortVisSprites(vissprite_t* vsprsortedhead, UINT32 start, UINT32 end)
{
	UINT32       i, count = 0;
	vissprite_t *ds, *dsprev, *dsnext, *dsfirst;
	vissprite_t *best = NULL;
	vissprite_t  unsorted;
	fixed_t      bestscale;
	INT32        bestdispoffset;

	dsfirst = &unsorted;
	dsprev = dsfirst;
	dsnext = dsfirst;

	I_Assert(start <= end);

	for (i = start; i < end; ++i)
	{
		ds = R_GetVisSprite(i);

		// Do not include this sprite, since it is completely obscured
		if (ds->cut & SC_CULL)
		{
			continue;
		}

		dsnext = ds;
		dsnext->linkdraw = NULL;

		dsprev->next = dsnext;
		dsnext->prev = dsprev;
		dsprev = dsnext;

		count++;
	}

	dsnext->next = dsfirst;
	dsfirst->prev = dsnext;

	// bundle linkdraw
	for (ds = unsorted.prev; ds != &unsorted; ds = ds->prev)
	{
		if (!(ds->cut & SC_LINKDRAW))
			continue;

		if (ds->cut & SC_SHADOW)
			continue;

		// reuse dsfirst...
		for (dsfirst = unsorted.prev; dsfirst != &unsorted; dsfirst = dsfirst->prev)
		{
			// don't connect if it's also a link
			if (dsfirst->cut & SC_LINKDRAW)
				continue;

			// don't connect to your shadow!
			if (dsfirst->cut & SC_SHADOW)
				continue;

			// don't connect to your bounding box!
			if (dsfirst->cut & SC_BBOX)
				continue;

			// don't connect if it's not the tracer
			if (dsfirst->mobj != ds->mobj)
				continue;

			// don't connect if the tracer's top is cut off, but lower than the link's top
			if ((dsfirst->cut & SC_TOP)
			&& dsfirst->szt > ds->szt)
				continue;

			// don't connect if the tracer's bottom is cut off, but higher than the link's bottom
			if ((dsfirst->cut & SC_BOTTOM)
			&& dsfirst->sz < ds->sz)
				continue;

			break;
		}

		// remove from chain
		ds->next->prev = ds->prev;
		ds->prev->next = ds->next;
		count--;

		if (dsfirst != &unsorted)
		{
			ds->extra_colormap = dsfirst->extra_colormap;

			// reusing dsnext...
			dsnext = dsfirst->linkdraw;

			if (!dsnext || ds->dispoffset < dsnext->dispoffset)
			{
				ds->next = dsnext;
				dsfirst->linkdraw = ds;
			}
			else
			{
				for (; dsnext->next != NULL; dsnext = dsnext->next)
					if (ds->dispoffset < dsnext->next->dispoffset)
						break;
				ds->next = dsnext->next;
				dsnext->next = ds;
			}
		}
	}

	// pull the vissprites out by scale
	vsprsortedhead->next = vsprsortedhead->prev = vsprsortedhead;
	for (i = 0; i < count; i++)
	{
		bestscale = bestdispoffset = INT32_MAX;
		for (ds = unsorted.next; ds != &unsorted; ds = ds->next)
		{
#ifdef PARANOIA
			if (ds->cut & SC_LINKDRAW)
				I_Error("R_SortVisSprites: no link or discardal made for linkdraw!");
#endif

			if (ds->sortscale < bestscale)
			{
				bestscale = ds->sortscale;
				bestdispoffset = ds->dispoffset;
				best = ds;
			}
			// order visprites of same scale by dispoffset, smallest first
			else if (ds->sortscale == bestscale && ds->dispoffset < bestdispoffset)
			{
				bestdispoffset = ds->dispoffset;
				best = ds;
			}
		}
		best->next->prev = best->prev;
		best->prev->next = best->next;
		best->next = vsprsortedhead;
		best->prev = vsprsortedhead->prev;
		vsprsortedhead->prev->next = best;
		vsprsortedhead->prev = best;
	}
}

//
// R_CreateDrawNodes
// Creates and sorts a list of drawnodes for the scene being rendered.
static drawnode_t *R_CreateDrawNode(drawnode_t *link);

static drawnode_t nodebankhead;

static void R_CreateDrawNodes(maskcount_t* mask, drawnode_t* head, boolean tempskip)
{
	drawnode_t *entry;
	drawseg_t *ds;
	INT32 i, p, best, x1, x2;
	fixed_t bestdelta, delta;
	vissprite_t *rover;
	static vissprite_t vsprsortedhead;
	drawnode_t *r2;
	visplane_t *plane;
	INT32 sintersect;
	fixed_t scale = 0;

	// Add the 3D floors, thicksides, and masked textures...
	for (ds = drawsegs + mask->drawsegs[1]; ds-- > drawsegs + mask->drawsegs[0];)
	{
		if (ds->numthicksides)
		{
			for (i = 0; i < ds->numthicksides; i++)
			{
				entry = R_CreateDrawNode(head);
				entry->thickseg = ds;
				entry->ffloor = ds->thicksides[i];
			}
		}
		// Check for a polyobject plane, but only if this is a front line
		if (ds->curline->polyseg && ds->curline->polyseg->visplane && !ds->curline->side) {
			plane = ds->curline->polyseg->visplane;
			R_PlaneBounds(plane);

			if (plane->low < 0 || plane->high > vid.height || plane->high > plane->low)
				;
			else {
				// Put it in!
				entry = R_CreateDrawNode(head);
				entry->plane = plane;
				entry->seg = ds;
			}
			ds->curline->polyseg->visplane = NULL;
		}
		if (ds->maskedtexturecol)
		{
			entry = R_CreateDrawNode(head);
			entry->seg = ds;
		}
		if (ds->numffloorplanes)
		{
			for (i = 0; i < ds->numffloorplanes; i++)
			{
				best = -1;
				bestdelta = 0;
				for (p = 0; p < ds->numffloorplanes; p++)
				{
					if (!ds->ffloorplanes[p])
						continue;
					plane = ds->ffloorplanes[p];
					R_PlaneBounds(plane);

					if (plane->low < 0 || plane->high > vid.height || plane->high > plane->low || plane->polyobj)
					{
						ds->ffloorplanes[p] = NULL;
						continue;
					}

					delta = abs(plane->height - viewz);
					if (delta > bestdelta)
					{
						best = p;
						bestdelta = delta;
					}
				}
				if (best != -1)
				{
					entry = R_CreateDrawNode(head);
					entry->plane = ds->ffloorplanes[best];
					entry->seg = ds;
					ds->ffloorplanes[best] = NULL;
				}
				else
					break;
			}
		}
	}

	if (tempskip)
		return;

	// find all the remaining polyobject planes and add them on the end of the list
	// probably this is a terrible idea if we wanted them to be sorted properly
	// but it works getting them in for now
	for (i = 0; i < numPolyObjects; i++)
	{
		if (!PolyObjects[i].visplane)
			continue;
		plane = PolyObjects[i].visplane;
		R_PlaneBounds(plane);

		if (plane->low < 0 || plane->high > vid.height || plane->high > plane->low)
		{
			PolyObjects[i].visplane = NULL;
			continue;
		}
		entry = R_CreateDrawNode(head);
		entry->plane = plane;
		// note: no seg is set, for what should be obvious reasons
		PolyObjects[i].visplane = NULL;
	}

	// No vissprites in this mask?
	if (mask->vissprites[1] - mask->vissprites[0] == 0)
		return;

	R_SortVisSprites(&vsprsortedhead, mask->vissprites[0], mask->vissprites[1]);

	for (rover = vsprsortedhead.prev; rover != &vsprsortedhead; rover = rover->prev)
	{
		const boolean alwaysontop = cv_debugrender_spriteclip.value || (rover->renderflags & RF_ALWAYSONTOP);
		const INT32 ontopflag = cv_debugrender_spriteclip.value ? 0 : (rover->renderflags & RF_ALWAYSONTOP);

		if (rover->szt > vid.height || rover->sz < 0)
			continue;

		sintersect = (rover->x1 + rover->x2) / 2;

		for (r2 = head->next; r2 != head; r2 = r2->next)
		{
			if (alwaysontop)
			{
				// Only sort behind other sprites; sorts in
				// front of everything else.
				if (!r2->sprite)
				{
					continue;
				}

				// Only sort behind other RF_ALWAYSONTOP sprites.
				// This avoids sorting behind a sprite that is
				// behind level geometry and thus sorting this
				// one behind level geometry too.
				if (r2->sprite->renderflags ^ ontopflag)
				{
					continue;
				}
			}

			if (r2->plane)
			{
				fixed_t planeobjectz, planecameraz;
				if (r2->plane->minx > rover->x2 || r2->plane->maxx < rover->x1)
					continue;
				if (rover->szt > r2->plane->low || rover->sz < r2->plane->high)
					continue;

				// Effective height may be different for each comparison in the case of slopes
				planeobjectz = P_GetZAt(r2->plane->slope, rover->gx, rover->gy, r2->plane->height);
				planecameraz = P_GetZAt(r2->plane->slope,     viewx,     viewy, r2->plane->height);

				// bird: if any part of the sprite peeks in front the plane
				if (planecameraz < viewz)
				{
					if (rover->gzt >= planeobjectz)
						continue;
				}
				else if (planecameraz > viewz)
				{
					if (rover->gz <= planeobjectz)
						continue;
				}

				// SoM: NOTE: Because a visplane's shape and scale is not directly
				// bound to any single linedef, a simple poll of it's frontscale is
				// not adequate. We must check the entire frontscale array for any
				// part that is in front of the sprite.

				x1 = rover->x1;
				x2 = rover->x2;
				if (x1 < r2->plane->minx) x1 = r2->plane->minx;
				if (x2 > r2->plane->maxx) x2 = r2->plane->maxx;

				if (r2->seg) // if no seg set, assume the whole thing is in front or something stupid
				{
					for (i = x1; i <= x2; i++)
					{
						if (r2->seg->frontscale[i] > rover->sortscale)
							break;
					}
					if (i > x2)
						continue;
				}

				entry = R_CreateDrawNode(NULL);
				(entry->prev = r2->prev)->next = entry;
				(entry->next = r2)->prev = entry;
				entry->sprite = rover;
				break;
			}
			else if (r2->thickseg)
			{
				//fixed_t topplaneobjectz, topplanecameraz, botplaneobjectz, botplanecameraz;
				if (rover->x1 > r2->thickseg->x2 || rover->x2 < r2->thickseg->x1)
					continue;

				scale = r2->thickseg->scale1 > r2->thickseg->scale2 ? r2->thickseg->scale1 : r2->thickseg->scale2;
				if (scale <= rover->sortscale)
					continue;
				scale = r2->thickseg->scale1 + (r2->thickseg->scalestep * (sintersect - r2->thickseg->x1));
				if (scale <= rover->sortscale)
					continue;

				// bird: Always sort sprites behind segs. This helps the plane
				// sorting above too. Basically if the sprite gets sorted behind
				// the seg here, it will be behind the plane too, since planes
				// are added after segs in the list.
#if 0
				topplaneobjectz = P_GetFFloorTopZAt   (r2->ffloor, rover->gx, rover->gy);
				topplanecameraz = P_GetFFloorTopZAt   (r2->ffloor,     viewx,     viewy);
				botplaneobjectz = P_GetFFloorBottomZAt(r2->ffloor, rover->gx, rover->gy);
				botplanecameraz = P_GetFFloorBottomZAt(r2->ffloor,     viewx,     viewy);

				if ((topplanecameraz > viewz && botplanecameraz < viewz) ||
				    (topplanecameraz < viewz && rover->gzt < topplaneobjectz) ||
				    (botplanecameraz > viewz && rover->gz > botplaneobjectz))
#endif
				{
					entry = R_CreateDrawNode(NULL);
					(entry->prev = r2->prev)->next = entry;
					(entry->next = r2)->prev = entry;
					entry->sprite = rover;
					break;
				}
			}
			else if (r2->seg)
			{
				if (rover->x1 > r2->seg->x2 || rover->x2 < r2->seg->x1)
					continue;

				scale = r2->seg->scale1 > r2->seg->scale2 ? r2->seg->scale1 : r2->seg->scale2;
				if (scale <= rover->sortscale)
					continue;
				scale = r2->seg->scale1 + (r2->seg->scalestep * (sintersect - r2->seg->x1));

				if (rover->sortscale < scale)
				{
					entry = R_CreateDrawNode(NULL);
					(entry->prev = r2->prev)->next = entry;
					(entry->next = r2)->prev = entry;
					entry->sprite = rover;
					break;
				}
			}
			else if (r2->sprite)
			{
				boolean infront = (r2->sprite->sortscale > rover->sortscale
								|| (r2->sprite->sortscale == rover->sortscale && r2->sprite->dispoffset > rover->dispoffset));

				if (rover->cut & SC_SPLAT || r2->sprite->cut & SC_SPLAT)
				{
					fixed_t scale1 = (rover->cut & SC_SPLAT ? rover->sortsplat : rover->sortscale);
					fixed_t scale2 = (r2->sprite->cut & SC_SPLAT ? r2->sprite->sortsplat : r2->sprite->sortscale);
					boolean behind = (scale2 > scale1 || (scale2 == scale1 && r2->sprite->dispoffset > rover->dispoffset));

					if (!behind)
					{
						fixed_t z1 = 0, z2 = 0;

						if (rover->mobj->z - viewz > 0)
						{
							z1 = rover->pz;
							z2 = r2->sprite->pz;
						}
						else
						{
							z1 = r2->sprite->pz;
							z2 = rover->pz;
						}

						z1 -= viewz;
						z2 -= viewz;

						infront = (z1 >= z2);
					}
				}
				else
				{
					if (r2->sprite->x1 > rover->x2 || r2->sprite->x2 < rover->x1)
						continue;
					if (r2->sprite->szt > rover->sz || r2->sprite->sz < rover->szt)
						continue;
				}

				if (infront)
				{
					entry = R_CreateDrawNode(NULL);
					(entry->prev = r2->prev)->next = entry;
					(entry->next = r2)->prev = entry;
					entry->sprite = rover;
					break;
				}
			}
		}
		if (r2 == head)
		{
			entry = R_CreateDrawNode(head);
			entry->sprite = rover;
		}
	}
}

static drawnode_t *R_CreateDrawNode(drawnode_t *link)
{
	drawnode_t *node = nodebankhead.next;

	if (node == &nodebankhead)
	{
		node = static_cast<drawnode_t*>(malloc(sizeof (*node)));
		if (!node)
			I_Error("No more free memory to CreateDrawNode");
	}
	else
		(nodebankhead.next = node->next)->prev = &nodebankhead;

	if (link)
	{
		node->next = link;
		node->prev = link->prev;
		link->prev->next = node;
		link->prev = node;
	}

	node->plane = NULL;
	node->seg = NULL;
	node->thickseg = NULL;
	node->ffloor = NULL;
	node->sprite = NULL;

	ps_numdrawnodes++;
	return node;
}

static void R_DoneWithNode(drawnode_t *node)
{
	(node->next->prev = node->prev)->next = node->next;
	(node->next = nodebankhead.next)->prev = node;
	(node->prev = &nodebankhead)->next = node;
}

static void R_ClearDrawNodes(drawnode_t* head)
{
	drawnode_t *rover;
	drawnode_t *next;

	for (rover = head->next; rover != head;)
	{
		next = rover->next;
		R_DoneWithNode(rover);
		rover = next;
	}

	head->next = head->prev = head;
}

void R_InitDrawNodes(void)
{
	nodebankhead.next = nodebankhead.prev = &nodebankhead;
}

//
// R_DrawSprite
//
//Fab : 26-04-98:
// NOTE : uses con_clipviewtop, so that when console is on,
//        don't draw the part of sprites hidden under the console
static void R_DrawSprite(vissprite_t *spr)
{
	mfloorclip = spr->clipbot;
	mceilingclip = spr->cliptop;

	R_CheckDebugHighlight(SW_HI_THINGS);

	if (spr->cut & SC_BBOX)
		R_DrawThingBoundingBox(spr);
	else if (spr->cut & SC_SPLAT)
		R_DrawFloorSplat(spr);
	else
		R_DrawVisSprite(spr);
}

// Special drawer for precipitation sprites Tails 08-18-2002
static void R_DrawPrecipitationSprite(vissprite_t *spr)
{
	mfloorclip = spr->clipbot;
	mceilingclip = spr->cliptop;
	R_DrawPrecipitationVisSprite(spr);
}

// R_ClipVisSprite
// Clips vissprites without drawing, so that portals can work. -Red
void R_ClipVisSprite(vissprite_t *spr, INT32 x1, INT32 x2, portal_t* portal)
{
	drawseg_t *ds;
	INT32		x;
	INT32		r1;
	INT32		r2;
	fixed_t		scale;
	fixed_t		lowscale;
	INT32		silhouette;
	INT32		xclip;

	if ((spr->renderflags & RF_ALWAYSONTOP) || cv_debugrender_spriteclip.value)
	{
		for (x = x1; x <= x2; x++)
		{
			spr->clipbot[x] = (INT16)viewheight;
			spr->cliptop[x] = (INT16)con_clipviewtop;
		}
		return;
	}

	for (x = x1; x <= x2; x++)
	{
		spr->clipbot[x] = spr->cliptop[x] = CLIP_UNDEF;
	}

	// Scan drawsegs from end to start for obscuring segs.
	// The first drawseg that has a greater scale
	//  is the clip seg.
	//SoM: 4/8/2000:
	// Pointer check was originally nonportable
	// and buggy, by going past LEFT end of array:

	// e6y: optimization
	if (drawsegs_xrange_size)
	{
		const drawseg_xrange_item_t *last = &drawsegs_xrange[drawsegs_xrange_count - 1];
		drawseg_xrange_item_t *curr = &drawsegs_xrange[-1];

		while (++curr <= last)
		{
			// determine if the drawseg obscures the sprite
			if (curr->x1 > spr->x2 || curr->x2 < spr->x1)
			{
				// does not cover sprite
				continue;
			}

			ds = curr->user;

			if (ds->portalpass != 66) // unused?
			{
				if (ds->portalpass > 0 && ds->portalpass <= portalrender)
					continue; // is a portal

				if (ds->scale1 > ds->scale2)
				{
					lowscale = ds->scale2;
					scale = ds->scale1;
				}
				else
				{
					lowscale = ds->scale1;
					scale = ds->scale2;
				}

				if (scale < spr->sortscale ||
					(lowscale < spr->sortscale &&
					 !R_PointOnSegSide (spr->gx, spr->gy, ds->curline)))
				{
					// masked mid texture?
					/*
					if (ds->maskedtexturecol)
						R_RenderMaskedSegRange (ds, r1, r2);
					*/

					// seg is behind sprite
					continue;
				}
			}

			r1 = ds->x1 < x1 ? x1 : ds->x1;
			r2 = ds->x2 > x2 ? x2 : ds->x2;

			// clip this piece of the sprite
			silhouette = ds->silhouette;

			if (spr->gz >= ds->bsilheight)
				silhouette &= ~SIL_BOTTOM;

			if (spr->gzt <= ds->tsilheight)
				silhouette &= ~SIL_TOP;

			if (silhouette == SIL_BOTTOM)
			{
				// bottom sil
				for (x = r1; x <= r2; x++)
					if (spr->clipbot[x] == CLIP_UNDEF)
						spr->clipbot[x] = ds->sprbottomclip[x];
			}
			else if (silhouette == SIL_TOP)
			{
				// top sil
				for (x = r1; x <= r2; x++)
					if (spr->cliptop[x] == CLIP_UNDEF)
						spr->cliptop[x] = ds->sprtopclip[x];
			}
			else if (silhouette == (SIL_TOP|SIL_BOTTOM))
			{
				// both
				for (x = r1; x <= r2; x++)
				{
					if (spr->clipbot[x] == CLIP_UNDEF)
						spr->clipbot[x] = ds->sprbottomclip[x];
					if (spr->cliptop[x] == CLIP_UNDEF)
						spr->cliptop[x] = ds->sprtopclip[x];
				}
			}
		}
	}
	//SoM: 3/17/2000: Clip sprites in water.
	if (spr->heightsec != -1)  // only things in specially marked sectors
	{
		fixed_t mh, h;
		INT32 phs = viewplayer->mo->subsector->sector->heightsec;
		if ((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
			(h = centeryfrac - FixedMul(mh -= viewz, spr->sortscale)) >= 0 &&
			(h >>= FRACBITS) < viewheight)
		{
			if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
			{                          // clip bottom
				for (x = x1; x <= x2; x++)
					if (spr->clipbot[x] == CLIP_UNDEF || h < spr->clipbot[x])
						spr->clipbot[x] = (INT16)h;
			}
			else						// clip top
			{
				for (x = x1; x <= x2; x++)
					if (spr->cliptop[x] == CLIP_UNDEF || h > spr->cliptop[x])
						spr->cliptop[x] = (INT16)h;
			}
		}

		if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
			(h = centeryfrac - FixedMul(mh-viewz, spr->sortscale)) >= 0 &&
			(h >>= FRACBITS) < viewheight)
		{
			if (phs != -1 && viewz >= sectors[phs].ceilingheight)
			{                         // clip bottom
				for (x = x1; x <= x2; x++)
					if (spr->clipbot[x] == CLIP_UNDEF || h < spr->clipbot[x])
						spr->clipbot[x] = (INT16)h;
			}
			else                       // clip top
			{
				for (x = x1; x <= x2; x++)
					if (spr->cliptop[x] == CLIP_UNDEF || h > spr->cliptop[x])
						spr->cliptop[x] = (INT16)h;
			}
		}
	}
	if (spr->cut & SC_TOP && spr->cut & SC_BOTTOM)
	{
		for (x = x1; x <= x2; x++)
		{
			if (spr->cliptop[x] == CLIP_UNDEF || spr->szt > spr->cliptop[x])
				spr->cliptop[x] = spr->szt;

			if (spr->clipbot[x] == CLIP_UNDEF || spr->sz < spr->clipbot[x])
				spr->clipbot[x] = spr->sz;
		}
	}
	else if (spr->cut & SC_TOP)
	{
		for (x = x1; x <= x2; x++)
		{
			if (spr->cliptop[x] == CLIP_UNDEF || spr->szt > spr->cliptop[x])
				spr->cliptop[x] = spr->szt;
		}
	}
	else if (spr->cut & SC_BOTTOM)
	{
		for (x = x1; x <= x2; x++)
		{
			if (spr->clipbot[x] == CLIP_UNDEF || spr->sz < spr->clipbot[x])
				spr->clipbot[x] = spr->sz;
		}
	}

	// all clipping has been performed, so store the values - what, did you think we were drawing them NOW?

	// check for unclipped columns
	for (xclip = x = x1; x <= x2; x++)
	{
		if (spr->clipbot[x] == CLIP_UNDEF)
			spr->clipbot[x] = (INT16)viewheight;

		if (spr->cliptop[x] == CLIP_UNDEF)
			//Fab : 26-04-98: was -1, now clips against console bottom
			spr->cliptop[x] = (INT16)con_clipviewtop;

		// Sprite is completely above or below clip plane
		if (spr->szt >= spr->clipbot[x] || spr->sz <= spr->cliptop[x])
			xclip++;
	}

	if (xclip == x)
	{
		spr->cut = static_cast<spritecut_e>(spr->cut | SC_CULL); // completely skip this sprite going forward
	}
	else if (portal)
	{
		INT32 start_index = std::max(portal->start, x1);
		INT32 end_index = std::min(portal->start + portal->end - portal->start, x2);
		for (x = x1; x < start_index; x++)
		{
			spr->clipbot[x] = -1;
			spr->cliptop[x] = -1;
		}
		for (x = start_index; x <= end_index; x++)
		{
			if (spr->clipbot[x] > portal->floorclip[x - portal->start])
				spr->clipbot[x] = portal->floorclip[x - portal->start];
			if (spr->cliptop[x] < portal->ceilingclip[x - portal->start])
				spr->cliptop[x] = portal->ceilingclip[x - portal->start];
		}
		for (x = end_index + 1; x <= x2; x++)
		{
			spr->clipbot[x] = -1;
			spr->cliptop[x] = -1;
		}
	}
}

void R_ClipSprites(drawseg_t* dsstart, portal_t* portal)
{
	const size_t maxdrawsegs = ds_p - dsstart;
	const INT32 cx = viewwidth / 2;
	drawseg_t* ds;
	INT32 i;

	// e6y
	// Reducing of cache misses in the following R_DrawSprite()
	// Makes sense for scenes with huge amount of drawsegs.
	// ~12% of speed improvement on epic.wad map05
	for (i = 0; i < DS_RANGES_COUNT; i++)
	{
		drawsegs_xranges[i].count = 0;
	}

	if (visspritecount - clippedvissprites <= 0)
	{
		return;
	}

	if (drawsegs_xrange_size < maxdrawsegs)
	{
		drawsegs_xrange_size = 2 * maxdrawsegs;

		for (i = 0; i < DS_RANGES_COUNT; i++)
		{
			drawsegs_xranges[i].items = static_cast<drawseg_xrange_item_t*>(Z_Realloc(
				drawsegs_xranges[i].items,
				drawsegs_xrange_size * sizeof(drawsegs_xranges[i].items[0]),
				PU_STATIC, NULL
			));
		}
	}

	for (ds = ds_p; ds-- > dsstart;)
	{
		if (ds->silhouette || ds->maskedtexturecol)
		{
			drawsegs_xranges[0].items[drawsegs_xranges[0].count].x1 = ds->x1;
			drawsegs_xranges[0].items[drawsegs_xranges[0].count].x2 = ds->x2;
			drawsegs_xranges[0].items[drawsegs_xranges[0].count].user = ds;

			// e6y: ~13% of speed improvement on sunder.wad map10
			if (ds->x1 < cx)
			{
				drawsegs_xranges[1].items[drawsegs_xranges[1].count] =
					drawsegs_xranges[0].items[drawsegs_xranges[0].count];
				drawsegs_xranges[1].count++;
			}

			if (ds->x2 >= cx)
			{
				drawsegs_xranges[2].items[drawsegs_xranges[2].count] =
					drawsegs_xranges[0].items[drawsegs_xranges[0].count];
				drawsegs_xranges[2].count++;
			}

			drawsegs_xranges[0].count++;
		}
	}

	for (; clippedvissprites < visspritecount; clippedvissprites++)
	{
		vissprite_t *spr = R_GetVisSprite(clippedvissprites);
		INT32 x1 = (spr->cut & SC_SPLAT) ? 0 : spr->x1;
		INT32 x2 = (spr->cut & SC_SPLAT) ? viewwidth : spr->x2;

		if (spr->cut & SC_BBOX)
		{
			// Do not clip bounding boxes
			continue;
		}

		if (x2 < cx)
		{
			drawsegs_xrange = drawsegs_xranges[1].items;
			drawsegs_xrange_count = drawsegs_xranges[1].count;
		}
		else if (x1 >= cx)
		{
			drawsegs_xrange = drawsegs_xranges[2].items;
			drawsegs_xrange_count = drawsegs_xranges[2].count;
		}
		else
		{
			drawsegs_xrange = drawsegs_xranges[0].items;
			drawsegs_xrange_count = drawsegs_xranges[0].count;
		}

		R_ClipVisSprite(spr, x1, x2, portal);
	}
}

boolean R_DrawPickups(void)
{
	if (g_takemapthumbnail != TMT_NO)
	{
		return false;
	}

	return (boolean)cv_drawpickups.value;
}

/* Check if thing may be drawn from our current view. */
boolean R_ThingVisible (mobj_t *thing)
{
	if (thing->sprite == SPR_NULL)
		return false;

	if (!R_DrawPickups())
	{
		switch (thing->type)
		{
			// Players
			case MT_PLAYER:
			case MT_FOLLOWER:
			// Individual pickups
			case MT_RING:
			case MT_FLINGRING:
			case MT_BLUESPHERE:
			case MT_SPRAYCAN:
			// Item Boxes and Capsules
			case MT_EXPLODE:
			case MT_RANDOMITEM:
			case MT_SPHEREBOX:
			case MT_ITEMCAPSULE:
			case MT_ITEMCAPSULE_PART:
			case MT_OVERLAY: // mostly capsule numbers :)))
			// Prison Eggs
			case MT_BATTLECAPSULE:
			case MT_BATTLECAPSULE_PIECE:
			// Duel hazards
			case MT_DUELBOMB:
			case MT_LANDMINE:
			case MT_SSMINE:
			case MT_SSMINE_SHIELD:
			case MT_MINERADIUS:
			case MT_POGOSPRING:
			case MT_DROPTARGET:
			case MT_HYUDORO:
			case MT_SHADOW: // hyuu fake shadow
			// Checkpoints
			case MT_CHECKPOINT_END:
			case MT_SIGNSPARKLE:
			case MT_THOK: // checkpoint parts
				return false;

			default:
				break;
		}
	}

	if (r_viewmobj && (thing == r_viewmobj || (r_viewmobj->player && r_viewmobj->player->followmobj == thing)))
		return false;

	if (tic_t t = P_MobjIsReappearing(thing))
	{
		// Flicker back in
		return t <= 2*TICRATE && (leveltime & 1);
	}

	if ((viewssnum == 0 && (thing->renderflags & RF_DONTDRAWP1))
	|| (viewssnum == 1 && (thing->renderflags & RF_DONTDRAWP2))
	|| (viewssnum == 2 && (thing->renderflags & RF_DONTDRAWP3))
	|| (viewssnum == 3 && (thing->renderflags & RF_DONTDRAWP4)))
		return false;

	if ((thing->renderflags & RF_REDUCEVFX) && cv_reducevfx.value && thing->owner != players[displayplayers[viewssnum]].mo)
		return false;

	return true;
}

boolean R_ThingWithinDist (mobj_t *thing, fixed_t limit_dist)
{
	const fixed_t dist = R_PointToDist(thing->x, thing->y);

	if (limit_dist)
	{
		// MF_DRAWFROMFARAWAY: visible from any distance
		if (!(thing->flags & MF_DRAWFROMFARAWAY) && dist > limit_dist)
		{
			return false;
		}
	}

	return true;
}

/* Check if precipitation may be drawn from our current view. */
boolean R_PrecipThingVisible (precipmobj_t *precipthing)
{
	if (( precipthing->precipflags & PCF_INVISIBLE ))
		return false;

	return true;
}

boolean R_ThingHorizontallyFlipped(mobj_t *thing)
{
	return (thing->frame & FF_HORIZONTALFLIP || thing->renderflags & RF_HORIZONTALFLIP);
}

boolean R_ThingVerticallyFlipped(mobj_t *thing)
{
	return (thing->frame & FF_VERTICALFLIP || thing->renderflags & RF_VERTICALFLIP);
}

boolean R_ThingIsPaperSprite(mobj_t *thing)
{
	return (thing->frame & FF_PAPERSPRITE || thing->renderflags & RF_PAPERSPRITE);
}

boolean R_ThingIsFloorSprite(mobj_t *thing)
{
	return (thing->flags2 & MF2_SPLAT || thing->frame & FF_FLOORSPRITE || thing->renderflags & RF_FLOORSPRITE);
}

boolean R_ThingIsFullBright(mobj_t *thing)
{
	if (thing->renderflags & RF_BRIGHTMASK)
		return ((thing->renderflags & RF_BRIGHTMASK) == RF_FULLBRIGHT);
	return ((thing->frame & FF_BRIGHTMASK) == FF_FULLBRIGHT);
}

boolean R_ThingIsSemiBright(mobj_t *thing)
{
	if (thing->renderflags & RF_BRIGHTMASK)
		return ((thing->renderflags & RF_BRIGHTMASK) == RF_SEMIBRIGHT);
	return ((thing->frame & FF_BRIGHTMASK) == FF_SEMIBRIGHT);
}

boolean R_ThingIsFullDark(mobj_t *thing)
{
	if (thing->renderflags & RF_BRIGHTMASK)
		return ((thing->renderflags & RF_BRIGHTMASK) == RF_FULLDARK);
	return ((thing->frame & FF_BRIGHTMASK) == FF_FULLDARK);
}

boolean R_ThingModelUsesDirectionalLighting(mobj_t *thing)
{
	switch (thing->type)
	{
		case MT_ARKARROW:
		case MT_ADVENTUREAIRBOOSTER_PART:
			return false;

		default:
			break;
	}
	return true;
}

//
// R_DrawMasked
//
static void R_DrawMaskedList (drawnode_t* head)
{
	ZoneScoped;
	drawnode_t *r2;
	drawnode_t *next;

	for (r2 = head->next; r2 != head; r2 = r2->next)
	{
		if (r2->plane)
		{
			drawspandata_t ds = {0};
			next = r2->prev;
			R_DrawSinglePlane(&ds, r2->plane, false);
			R_DoneWithNode(r2);
			r2 = next;
		}
		else if (r2->seg && r2->seg->maskedtexturecol != NULL)
		{
			next = r2->prev;
			R_RenderMaskedSegRange(r2->seg, r2->seg->x1, r2->seg->x2);
			r2->seg->maskedtexturecol = NULL;
			R_DoneWithNode(r2);
			r2 = next;
		}
		else if (r2->thickseg)
		{
			next = r2->prev;
			R_RenderThickSideRange(r2->thickseg, r2->thickseg->x1, r2->thickseg->x2, r2->ffloor);
			R_DoneWithNode(r2);
			r2 = next;
		}
		else if (r2->sprite)
		{
			next = r2->prev;

			// Tails 08-18-2002
			if (r2->sprite->cut & SC_PRECIP)
			{
				R_DrawPrecipitationSprite(r2->sprite);
			}
			else if (!r2->sprite->linkdraw)
			{
				R_DrawSprite(r2->sprite);
			}
			else // unbundle linkdraw
			{
				vissprite_t *ds = r2->sprite->linkdraw;

				for (;
				(ds != NULL && r2->sprite->dispoffset > ds->dispoffset);
				ds = ds->next)
				{
					R_DrawSprite(ds);
				}

				R_DrawSprite(r2->sprite);

				for (; ds != NULL; ds = ds->next)
				{
					R_DrawSprite(ds);
				}
			}

			R_DoneWithNode(r2);
			r2 = next;
		}
	}
}

void R_DrawMasked(maskcount_t* masks, INT32 nummasks)
{
	ZoneScoped;
	drawnode_t *heads;	/**< Drawnode lists; as many as number of views/portals. */
	INT32 i;

	heads = static_cast<drawnode_t*>(calloc(nummasks, sizeof(drawnode_t)));

	for (i = 0; i < nummasks; i++)
	{
		heads[i].next = heads[i].prev = &heads[i];

		viewx = masks[i].viewx;
		viewy = masks[i].viewy;
		viewz = masks[i].viewz;
		viewsector = masks[i].viewsector;

		R_CreateDrawNodes(&masks[i], &heads[i], false);
	}

	//for (i = 0; i < nummasks; i++)
	//	CONS_Printf("Mask no.%d:\ndrawsegs: %d\n vissprites: %d\n\n", i, masks[i].drawsegs[1] - masks[i].drawsegs[0], masks[i].vissprites[1] - masks[i].vissprites[0]);

	for (; nummasks > 0; nummasks--)
	{
		viewx = masks[nummasks - 1].viewx;
		viewy = masks[nummasks - 1].viewy;
		viewz = masks[nummasks - 1].viewz;
		viewsector = masks[nummasks - 1].viewsector;

		R_DrawMaskedList(&heads[nummasks - 1]);
		R_ClearDrawNodes(&heads[nummasks - 1]);
	}

	free(heads);
}
