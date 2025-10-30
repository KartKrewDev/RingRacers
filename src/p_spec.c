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
/// \file  p_spec.c
/// \brief Implements special effects:
///        Texture animation, height or lighting changes
///        according to adjacent sectors, respective
///        utility functions, etc.
///        Line Tag handling. Line and Sector triggers.

#include "d_think.h"
#include "dehacked.h"
#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "p_setup.h" // levelflats for flat animation
#include "r_data.h"
#include "r_fps.h"
#include "r_textures.h"
#include "m_random.h"
#include "p_mobj.h"
#include "i_system.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_main.h" //Two extra includes.
#include "r_sky.h"
#include "p_polyobj.h"
#include "p_slopes.h"
#include "hu_stuff.h"
#include "m_misc.h"
#include "m_cond.h" //unlock triggers
#include "lua_hook.h" // LUA_HookSpecialExecute
#include "f_finale.h" // control text prompt
#include "r_skins.h" // skins

// SRB2kart
#include "k_kart.h"
#include "k_specialstage.h"
#include "console.h" // CON_LogMessage
#include "k_respawn.h"
#include "k_terrain.h"
#include "k_objects.h"
#include "acs/interface.h"
#include "m_easing.h"
#include "music.h"
#include "k_battle.h" // battleprisons
#include "k_endcam.h" // K_EndCameraIsFreezing()
#include "k_race.h" // K_SpawnFinishEXP
#include "k_grandprix.h" // grandprixinfo

// Not sure if this is necessary, but it was in w_wad.c, so I'm putting it here too -Shadow Hog
#include <errno.h>

mobj_t *skyboxviewpnts[16]; // array of MT_SKYBOX viewpoint mobjs
mobj_t *skyboxcenterpnts[16]; // array of MT_SKYBOX centerpoint mobjs

/** Animated texture descriptor
  * This keeps track of an animated texture or an animated flat.
  * \sa P_UpdateSpecials, P_InitPicAnims, animdef_t
  */
typedef struct
{
	SINT8 istexture; ///< ::true for a texture, ::false for a flat
	INT32 picnum;    ///< The end flat number
	INT32 basepic;   ///< The start flat number
	INT32 numpics;   ///< Number of frames in the animation
	tic_t speed;     ///< Number of tics for which each frame is shown
} anim_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

/** Animated texture definition.
  * Used for loading an ANIMDEFS lump from a wad.
  *
  * Animations are defined by the first and last frame (i.e., flat or texture).
  * The animation sequence uses all flats between the start and end entry, in
  * the order found in the wad.
  *
  * \sa anim_t
  */
typedef struct
{
	SINT8 istexture; ///< True for a texture, false for a flat.
	char endname[9]; ///< Name of the last frame, null-terminated.
	char startname[9]; ///< Name of the first frame, null-terminated.
	INT32 speed ; ///< Number of tics for which each frame is shown.
} ATTRPACK animdef_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	UINT32 count;
	thinker_t **thinkers;
} thinkerlist_t;

static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, fixed_t z_mag, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider); //SoM: 3/9/2000
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline);
static void P_ResetFakeFloorFader(ffloor_t *rover, fade_t *data, boolean finalize);
#define P_RemoveFakeFloorFader(l) P_ResetFakeFloorFader(l, NULL, false);
static boolean P_FadeFakeFloor(ffloor_t *rover, INT16 sourcevalue, INT16 destvalue, INT16 speed, boolean ticbased, INT32 *timer,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha);
static void P_AddFakeFloorFader(ffloor_t *rover, size_t sectornum, size_t ffloornum,
	INT16 destvalue, INT16 speed, boolean ticbased, boolean relative,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha);
static void P_ResetColormapFader(sector_t *sector);
static void Add_ColormapFader(sector_t *sector, extracolormap_t *source_exc, extracolormap_t *dest_exc,
	boolean ticbased, INT32 duration);
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline);
static void P_AddFloatThinker(sector_t *sec, UINT16 tag, line_t *sourceline);
//static void P_AddBridgeThinker(line_t *sourceline, sector_t *sec);
static void P_AddFakeFloorsByLine(size_t line, INT32 alpha, UINT8 blendmode, ffloortype_e ffloorflags, thinkerlist_t *secthinkers);
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec);
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer);
static void P_AddPlaneDisplaceThinker(INT32 type, fixed_t speed, INT32 control, INT32 affectee, UINT8 reverse);


//SoM: 3/7/2000: New sturcture without limits.
static anim_t *lastanim;
static anim_t *anims = NULL; /// \todo free leak
static size_t maxanims;

// Animating line specials

// Init animated textures
// - now called at level loading P_SetupLevel()

static animdef_t *animdefs = NULL;

// Increase the size of animdefs to make room for a new animation definition
static void GrowAnimDefs(void)
{
	maxanims++;
	animdefs = (animdef_t *)Z_Realloc(animdefs, sizeof(animdef_t)*(maxanims + 1), PU_STATIC, NULL);
}

// A prototype; here instead of p_spec.h, so they're "private"
void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum, boolean photosens);
void P_ParseAnimationDefintion(boolean photosens);

/** Sets up texture and flat animations.
  *
  * Converts an ::animdef_t array loaded from a lump into
  * ::anim_t format.
  *
  * Issues an error if any animation cycles are invalid.
  *
  * \sa P_FindAnimatedFlat, P_SetupLevelFlatAnims
  * \author Steven McGranahan (original), Shadow Hog (had to rewrite it to handle multiple WADs), JTE (had to rewrite it to handle multiple WADs _correctly_)
  */

void P_InitPicAnims(void)
{
	// Init animation
	INT32 w; // WAD
	size_t i;

	I_Assert(animdefs == NULL);

	maxanims = 0;

	for (w = numwadfiles-1; w >= 0; w--)
	{
		UINT16 animdefsLumpNum;

		if (cv_reducevfx.value)
		{
			// Find RVFXANIM lump in the WAD *first*
			animdefsLumpNum = W_CheckNumForNamePwad("RVFXANIM", w, 0);

			while (animdefsLumpNum != INT16_MAX)
			{
				P_ParseANIMDEFSLump(w, animdefsLumpNum, true);
				animdefsLumpNum = W_CheckNumForNamePwad("RVFXANIM", (UINT16)w, animdefsLumpNum + 1);
			}
		}

		// Find ANIMDEFS lump in the WAD
		animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", w, 0);

		while (animdefsLumpNum != INT16_MAX)
		{
			P_ParseANIMDEFSLump(w, animdefsLumpNum, false);
			animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", (UINT16)w, animdefsLumpNum + 1);
		}
	}

	// Define the last one
	animdefs[maxanims].istexture = -1;
	strncpy(animdefs[maxanims].endname, "", 9);
	strncpy(animdefs[maxanims].startname, "", 9);
	animdefs[maxanims].speed = 0;

	if (anims)
		free(anims);

	anims = (anim_t *)malloc(sizeof (*anims)*(maxanims + 1));
	if (!anims)
		I_Error("Not enough free memory for ANIMDEFS data");

	lastanim = anims;
	for (i = 0; animdefs[i].istexture != -1; i++)
	{
		if (animdefs[i].istexture != 1)
			continue;

		if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
			continue;

		lastanim->picnum = R_TextureNumForName(animdefs[i].endname);
		lastanim->basepic = R_TextureNumForName(animdefs[i].startname);

		lastanim->istexture = animdefs[i].istexture;
		lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

		if (lastanim->numpics < 2)
		{
			free(anims);
			I_Error("P_InitPicAnims: bad cycle from %s to %s",
				animdefs[i].startname, animdefs[i].endname);
		}

		lastanim->speed = LONG(animdefs[i].speed);
		lastanim++;
	}
	lastanim->istexture = -1;
	R_ClearTextureNumCache(false);

	// Clear animdefs now that we're done with it.
	// We'll only be using anims from now on.
	Z_Free(animdefs);
	animdefs = NULL;
}

void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum, boolean photosens)
{
	char *animdefsLump;
	size_t animdefsLumpLength;
	char *animdefsText;
	char *animdefsToken;
	char *p;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	animdefsLump = (char *)W_CacheLumpNumPwad(wadNum,lumpnum,PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (animdefsLump == NULL) return;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	animdefsLumpLength = W_LumpLengthPwad(wadNum,lumpnum);
	animdefsText = (char *)Z_Malloc((animdefsLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(animdefsText,animdefsLump,animdefsLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	animdefsText[animdefsLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(animdefsLump);

	// Now, let's start parsing this thing
	p = animdefsText;
	animdefsToken = M_GetToken(p);
	while (animdefsToken != NULL)
	{
		if (stricmp(animdefsToken, "TEXTURE") == 0)
		{
			Z_Free(animdefsToken);
			P_ParseAnimationDefintion(photosens);
		}
		else if (stricmp(animdefsToken, "FLAT") == 0)
		{
			I_Error("Error parsing ANIMDEFS lump: FLats are no longer supported by Ring Racers");
		}
		else if (stricmp(animdefsToken, "OSCILLATE") == 0)
		{
			// This probably came off the tail of an earlier definition. It's technically legal syntax, but we don't support it.
			I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"OSCILLATE\" (the animation plays in reverse when it reaches the end) are not supported by Ring Racers");
		}
		else
		{
			I_Error("Error parsing ANIMDEFS lump: Expected \"TEXTURE\", got \"%s\"",animdefsToken);
		}

		do // get next content line to parse
		{
			while (*p != '\0' && *p != '\n') ++p; // skips content of evaluated line
			while (*p == '\n') ++p; // skips extra blank lines
		} while (*p == '/' && *(p+1) == '/'); // skips comments

		animdefsToken = M_GetToken(p);
	}
	Z_Free(animdefsToken);
	Z_Free((void *)animdefsText);
}

void P_ParseAnimationDefintion(boolean photosens)
{
	char *animdefsToken;
	size_t animdefsTokenLength;
	char *endPos;
	INT32 animSpeed;
	size_t i;

	// Startname
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where start texture/flat name should be");
	}
	if (stricmp(animdefsToken, "OPTIONAL") == 0)
	{
		// This is meaningful to ZDoom - it tells the program NOT to bomb out
		// if the textures can't be found - but it's useless in SRB2, so we'll
		// just smile, nod, and carry on
		Z_Free(animdefsToken);
		animdefsToken = M_GetToken(NULL);

		if (animdefsToken == NULL)
		{
			I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where start texture/flat name should be");
		}
		else if (stricmp(animdefsToken, "RANGE") == 0)
		{
			// Oh. Um. Apparently "OPTIONAL" is a texture name. Naughty.
			// I should probably handle this more gracefully, but right now
			// I can't be bothered; especially since ZDoom doesn't handle this
			// condition at all.
			I_Error("Error parsing ANIMDEFS lump: \"OPTIONAL\" is a keyword; you cannot use it as the startname of an animation");
		}
	}
	animdefsTokenLength = strlen(animdefsToken);
	if (animdefsTokenLength>8)
	{
		I_Error("Error parsing ANIMDEFS lump: lump name \"%s\" exceeds 8 characters", animdefsToken);
	}

	// Search for existing animdef
	for (i = 0; i < maxanims; i++)
		if (stricmp(animdefsToken, animdefs[i].startname) == 0)
		{
			//CONS_Alert(CONS_NOTICE, "Duplicate animation: %s\n", animdefsToken);

			// If we weren't parsing in reverse order, we would `break` here and parse the new data into the existing slot we found.
			// Instead, we're just going to skip parsing the rest of this line entirely.
			Z_Free(animdefsToken);
			return;
		}

	// Not found
	if (i == maxanims)
	{
		// Increase the size to make room for the new animation definition
		GrowAnimDefs();
		strncpy(animdefs[i].startname, animdefsToken, 9);
	}

	// animdefs[i].startname is now set to animdefsToken either way.
	Z_Free(animdefsToken);

	// set texture type
	animdefs[i].istexture = 1;

	// "RANGE"
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"RANGE\" after \"%s\"'s startname should be", animdefs[i].startname);
	}
	if (stricmp(animdefsToken, "ALLOWDECALS") == 0)
	{
		// Another ZDoom keyword, ho-hum. Skip it, move on to the next token.
		Z_Free(animdefsToken);
		animdefsToken = M_GetToken(NULL);
	}
	if (stricmp(animdefsToken, "PIC") == 0)
	{
		// This is technically legitimate ANIMDEFS syntax, but SRB2 doesn't support it.
		I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"PIC\" (specific frames instead of a consecutive range) are not supported by SRB2");
	}
	if (stricmp(animdefsToken, "RANGE") != 0)
	{
		I_Error("Error parsing ANIMDEFS lump: Expected \"RANGE\" after \"%s\"'s startname, got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	Z_Free(animdefsToken);

	// Endname
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s end texture/flat name should be", animdefs[i].startname);
	}
	animdefsTokenLength = strlen(animdefsToken);
	if (animdefsTokenLength>8)
	{
		I_Error("Error parsing ANIMDEFS lump: lump name \"%s\" exceeds 8 characters", animdefsToken);
	}
	strncpy(animdefs[i].endname, animdefsToken, 9);
	Z_Free(animdefsToken);

	// "TICS"
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s \"TICS\" should be", animdefs[i].startname);
	}
	if (stricmp(animdefsToken, "RAND") == 0)
	{
		// This is technically legitimate ANIMDEFS syntax, but SRB2 doesn't support it.
		I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"RAND\" (random duration per frame) are not supported by SRB2");
	}
	if (stricmp(animdefsToken, "TICS") != 0)
	{
		I_Error("Error parsing ANIMDEFS lump: Expected \"TICS\" in animation definition for \"%s\", got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	Z_Free(animdefsToken);

	// Speed
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s animation speed should be", animdefs[i].startname);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	animSpeed = strtol(animdefsToken,&endPos,10);
	if (endPos == animdefsToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		|| animSpeed < 0) // Number is not positive
	{
		I_Error("Error parsing ANIMDEFS lump: Expected a positive integer for \"%s\"'s animation speed, got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	// Not letting anyone mess up a photosensitivity feature like this.
	if ((photosens) && animSpeed < 8)
	{
		CONS_Alert(CONS_WARNING, M_GetText("RVFXANIM: Animation speed of \"%s\" is less than 8 - automatically set to 8\n"), animdefs[i].startname);
		animSpeed = 8;
	}
	animdefs[i].speed = animSpeed;
	Z_Free(animdefsToken);
}

/** Checks for flats in levelflats that are part of a flat animation sequence
  * and sets them up for animation.
  *
  * \param animnum Index into ::anims to find flats for.
  * \sa P_SetupLevelFlatAnims
  */
static inline void P_FindAnimatedFlat(INT32 animnum)
{
	size_t i;
	lumpnum_t startflatnum, endflatnum;
	levelflat_t *foundflats;

	foundflats = levelflats;
	startflatnum = anims[animnum].basepic;
	endflatnum = anims[animnum].picnum;

	// note: high word of lumpnum is the wad number
	if ((startflatnum>>16) != (endflatnum>>16))
		I_Error("AnimatedFlat start %s not in same wad as end %s\n",
			animdefs[animnum].startname, animdefs[animnum].endname);

	//
	// now search through the levelflats if this anim flat sequence is used
	//
	for (i = 0; i < numlevelflats; i++, foundflats++)
	{
		// is that levelflat from the flat anim sequence ?
		if ((anims[animnum].istexture) && (foundflats->type == LEVELFLAT_TEXTURE)
			&& ((UINT16)foundflats->u.texture.num >= startflatnum && (UINT16)foundflats->u.texture.num <= endflatnum))
		{
			foundflats->u.texture.basenum = startflatnum;
			foundflats->animseq = foundflats->u.texture.num - startflatnum;
			foundflats->numpics = endflatnum - startflatnum + 1;
			foundflats->speed = anims[animnum].speed;

			CONS_Debug(DBG_SETUP, "animflat: #%03d name:%.8s animseq:%d numpics:%d speed:%d\n",
					atoi(sizeu1(i)), foundflats->name, foundflats->animseq,
					foundflats->numpics,foundflats->speed);
		}
		else if ((!anims[animnum].istexture) && (foundflats->type == LEVELFLAT_FLAT)
			&& (foundflats->u.flat.lumpnum >= startflatnum && foundflats->u.flat.lumpnum <= endflatnum))
		{
			foundflats->u.flat.baselumpnum = startflatnum;
			foundflats->animseq = foundflats->u.flat.lumpnum - startflatnum;
			foundflats->numpics = endflatnum - startflatnum + 1;
			foundflats->speed = anims[animnum].speed;

			CONS_Debug(DBG_SETUP, "animflat: #%03d name:%.8s animseq:%d numpics:%d speed:%d\n",
					atoi(sizeu1(i)), foundflats->name, foundflats->animseq,
					foundflats->numpics,foundflats->speed);
		}
	}
}

/** Sets up all flats used in a level.
  *
  * \sa P_InitPicAnims, P_FindAnimatedFlat
  */
void P_SetupLevelFlatAnims(void)
{
	INT32 i;

	// the original game flat anim sequences
	for (i = 0; anims[i].istexture != -1; i++)
		P_FindAnimatedFlat(i);
}

//
// UTILITIES
//

#if 0
/** Gets a side from a sector line.
  *
  * \param currentSector Sector the line is in.
  * \param line          Index of the line within the sector.
  * \param side          0 for front, 1 for back.
  * \return Pointer to the side_t of the side you want.
  * \sa getSector, twoSided, getNextSector
  */
static inline side_t *getSide(INT32 currentSector, INT32 line, INT32 side)
{
	return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}

/** Gets a sector from a sector line.
  *
  * \param currentSector Sector the line is in.
  * \param line          Index of the line within the sector.
  * \param side          0 for front, 1 for back.
  * \return Pointer to the ::sector_t of the sector on that side.
  * \sa getSide, twoSided, getNextSector
  */
static inline sector_t *getSector(INT32 currentSector, INT32 line, INT32 side)
{
	return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

/** Determines whether a sector line is two-sided.
  * Uses the Boom method, checking if the line's back side is set to -1, rather
  * than looking for ::ML_TWOSIDED.
  *
  * \param sector The sector.
  * \param line   Line index within the sector.
  * \return 1 if the sector is two-sided, 0 otherwise.
  * \sa getSide, getSector, getNextSector
  */
static inline boolean twoSided(INT32 sector, INT32 line)
{
	return (sectors[sector].lines[line])->sidenum[1] != 0xffff;
}
#endif

/** Finds sector next to current.
  *
  * \param line Pointer to the line to cross.
  * \param sec  Pointer to the current sector.
  * \return Pointer to a ::sector_t of the adjacent sector, or NULL if the line
  *         is one-sided.
  * \sa getSide, getSector, twoSided
  * \author Steven McGranahan
  */
static sector_t *getNextSector(line_t *line, sector_t *sec)
{
	if (line->frontsector == sec)
	{
		if (line->backsector != sec)
			return line->backsector;
		else
			return NULL;
	}
	return line->frontsector;
}

/** Finds lowest floor in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Lowest floor height in an adjacent sector.
  * \sa P_FindHighestFloorSurrounding, P_FindNextLowestFloor,
  *     P_FindLowestCeilingSurrounding
  */
fixed_t P_FindLowestFloorSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t floorh;

	floorh = sec->floorheight;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->floorheight < floorh)
			floorh = other->floorheight;
	}
	return floorh;
}

/** Finds highest floor in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Highest floor height in an adjacent sector.
  * \sa P_FindLowestFloorSurrounding, P_FindNextHighestFloor,
  *     P_FindHighestCeilingSurrounding
  */
fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t floorh = -500*FRACUNIT;
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->floorheight > floorh || !foundsector)
			floorh = other->floorheight;

		if (!foundsector)
			foundsector = 1;
	}
	return floorh;
}

/** Finds next highest floor in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next highest floor height in an adjacent sector, or currentheight
  *         if there are none higher.
  * \sa P_FindHighestFloorSurrounding, P_FindNextLowestFloor,
  *     P_FindNextHighestCeiling
  * \author Lee Killough
  */
fixed_t P_FindNextHighestFloor(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if (other && other->floorheight > currentheight)
		{
			height = other->floorheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if (other &&
					other->floorheight < height &&
					other->floorheight > currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

////////////////////////////////////////////////////
// SoM: Start new Boom functions
////////////////////////////////////////////////////

/** Finds next lowest floor in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next lowest floor height in an adjacent sector, or currentheight
  *         if there are none lower.
  * \sa P_FindLowestFloorSurrounding, P_FindNextHighestFloor,
  *     P_FindNextLowestCeiling
  * \author Lee Killough
  */
fixed_t P_FindNextLowestFloor(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if (other && other->floorheight < currentheight)
		{
			height = other->floorheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if (other &&	other->floorheight > height
					&& other->floorheight < currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

#if 0
/** Finds next lowest ceiling in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next lowest ceiling height in an adjacent sector, or currentheight
  *         if there are none lower.
  * \sa P_FindLowestCeilingSurrounding, P_FindNextHighestCeiling,
  *     P_FindNextLowestFloor
  * \author Lee Killough
  */
static fixed_t P_FindNextLowestCeiling(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if (other &&	other->ceilingheight < currentheight)
		{
			height = other->ceilingheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if (other &&	other->ceilingheight > height
					&& other->ceilingheight < currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}

/** Finds next highest ceiling in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next highest ceiling height in an adjacent sector, or currentheight
  *         if there are none higher.
  * \sa P_FindHighestCeilingSurrounding, P_FindNextLowestCeiling,
  *     P_FindNextHighestFloor
  * \author Lee Killough
  */
static fixed_t P_FindNextHighestCeiling(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if (other && other->ceilingheight > currentheight)
		{
			height = other->ceilingheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if (other && other->ceilingheight < height
					&& other->ceilingheight > currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}
#endif

////////////////////////////
// End New Boom functions
////////////////////////////

/** Finds lowest ceiling in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Lowest ceiling height in an adjacent sector.
  * \sa P_FindHighestCeilingSurrounding, P_FindNextLowestCeiling,
  *     P_FindLowestFloorSurrounding
  */
fixed_t P_FindLowestCeilingSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t height = 32000*FRACUNIT; //SoM: 3/7/2000: Remove ovf
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight < height || !foundsector)
			height = other->ceilingheight;

		if (!foundsector)
			foundsector = 1;
	}
	return height;
}

/** Finds Highest ceiling in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Highest ceiling height in an adjacent sector.
  * \sa P_FindLowestCeilingSurrounding, P_FindNextHighestCeiling,
  *     P_FindHighestFloorSurrounding
  */
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t height = 0;
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight > height || !foundsector)
			height = other->ceilingheight;

		if (!foundsector)
			foundsector = 1;
	}
	return height;
}

#if 0
//SoM: 3/7/2000: UTILS.....
//
// P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
//
static fixed_t P_FindShortestTextureAround(INT32 secnum)
{
	fixed_t minsize = 32000<<FRACBITS;
	side_t *side;
	size_t i;
	sector_t *sec= &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if (side->bottomtexture > 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
			side = getSide(secnum,i,1);
			if (side->bottomtexture > 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
		}
	}
	return minsize;
}

//SoM: 3/7/2000: Stuff.... (can you tell I'm getting tired? It's 12 : 30!)
//
// P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
//
static fixed_t P_FindShortestUpperAround(INT32 secnum)
{
	fixed_t minsize = 32000<<FRACBITS;
	side_t *side;
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if (side->toptexture > 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
			side = getSide(secnum,i,1);
			if (side->toptexture > 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
		}
	}
	return minsize;
}

//SoM: 3/7/2000
//
// P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
static sector_t *P_FindModelFloorSector(fixed_t floordestheight, INT32 secnum)
{
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			if (getSide(secnum,i,0)->sector-sectors == secnum)
				sec = getSector(secnum,i,1);
			else
				sec = getSector(secnum,i,0);

			if (sec->floorheight == floordestheight)
				return sec;
		}
	}
	return NULL;
}

//
// P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
static sector_t *P_FindModelCeilingSector(fixed_t ceildestheight, INT32 secnum)
{
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			if (getSide(secnum, i, 0)->sector - sectors == secnum)
				sec = getSector(secnum, i, 1);
			else
				sec = getSector(secnum, i, 0);

			if (sec->ceilingheight == ceildestheight)
				return sec;
		}
	}
	return NULL;
}
#endif

// Parses arguments for parameterized polyobject door types
static boolean PolyDoor(INT32 *args)
{
	polydoordata_t pdd;

	pdd.polyObjNum = args[0]; // polyobject id

	pdd.doorType = POLY_DOOR_SLIDE;
	pdd.speed    = args[1] << (FRACBITS - 3);
	pdd.angle    = FixedAngle(args[2] << FRACBITS); // angle of motion
	pdd.distance = args[3] << FRACBITS;
	pdd.delay    = args[4]; // delay in tics

	return EV_DoPolyDoor(&pdd);
}

static boolean PolyDoorSwing(INT32 *args)
{
	polydoordata_t pdd;

	pdd.polyObjNum = args[0]; // polyobject id

	pdd.doorType = POLY_DOOR_SWING;
	pdd.speed    = args[1]; // angular speed
	pdd.distance = args[2]; // angular distance
	pdd.delay    = args[3]; // delay in tics

	return EV_DoPolyDoor(&pdd);
}

// Parses arguments for parameterized polyobject move special
static boolean PolyMove(INT32 *args)
{
	polymovedata_t pmd;

	pmd.polyObjNum = args[0];
	pmd.speed      = args[1] << (FRACBITS - 3);
	pmd.angle      = FixedAngle(args[2] << FRACBITS);
	pmd.distance   = args[3] << FRACBITS;

	pmd.overRide = !!args[4]; // Polyobj_OR_Move

	return EV_DoPolyObjMove(&pmd);
}

static void PolySetVisibilityTangibility(INT32 *args)
{
	INT32 polyObjNum = args[0];
	polyobj_t* po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolySetVisibilityTangibility: bad polyobj %d\n", polyObjNum);
		return;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return;

	if (args[1] == TMF_ADD)
	{
		po->flags &= ~POF_NOSPECIALS;
		po->flags |= (po->spawnflags & POF_RENDERALL);
	}
	else if (args[1] == TMF_REMOVE)
	{
		po->flags |= POF_NOSPECIALS;
		po->flags &= ~POF_RENDERALL;
	}

	if (args[2] == TMF_ADD)
		po->flags |= POF_SOLID;
	else if (args[2] == TMF_REMOVE)
		po->flags &= ~POF_SOLID;
}

// Sets the translucency of a polyobject
static void PolyTranslucency(INT32 *args)
{
	INT32 polyObjNum = args[0];
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyTranslucency: bad polyobj %d\n", polyObjNum);
		return;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return;

	if (args[2]) // relative calc
		po->translucency += args[1];
	else
		po->translucency = args[1];

	po->translucency = max(min(po->translucency, NUMTRANSMAPS), 0);
}

// Makes a polyobject translucency fade and applies tangibility
static boolean PolyFade(INT32 *args)
{
	INT32 polyObjNum = args[0];
	polyobj_t *po;
	polyfadedata_t pfd;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyFade: bad polyobj %d\n", polyObjNum);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return 0;

	// Prevent continuous execs from interfering on an existing fade
	if (!(args[3] & TMPF_OVERRIDE)
		&& po->thinker
		&& po->thinker->function.acp1 == (actionf_p1)T_PolyObjFade)
	{
		CONS_Debug(DBG_POLYOBJ, "Line type 492 Executor: Fade PolyObject thinker already exists\n");
		return 0;
	}

	pfd.polyObjNum = polyObjNum;

	if (args[3] & TMPF_RELATIVE) // relative calc
		pfd.destvalue = po->translucency + args[1];
	else
		pfd.destvalue = args[1];

	pfd.destvalue = max(min(pfd.destvalue, NUMTRANSMAPS), 0);

	// already equal, nothing to do
	if (po->translucency == pfd.destvalue)
		return 1;

	pfd.docollision = !(args[3] & TMPF_IGNORECOLLISION); // do not handle collision flags
	pfd.doghostfade = (args[3] & TMPF_GHOSTFADE);        // do ghost fade (no collision flags during fade)
	pfd.ticbased = (args[3] & TMPF_TICBASED);            // Speed = Tic Duration

	pfd.speed = args[2];

	return EV_DoPolyObjFade(&pfd);
}

// Parses arguments for parameterized polyobject waypoint movement
static boolean PolyWaypoint(INT32 *args)
{
	polywaypointdata_t pwd;

	pwd.polyObjNum     = args[0];
	pwd.speed          = args[1] << (FRACBITS - 3);
	pwd.sequence       = args[2];
	pwd.returnbehavior = args[3];
	pwd.flags          = args[4];

	return EV_DoPolyObjWaypoint(&pwd);
}

// Parses arguments for parameterized polyobject rotate special
static boolean PolyRotate(INT32 *args)
{
	polyrotdata_t prd;

	prd.polyObjNum = args[0];
	prd.speed      = args[1]; // angular speed
	prd.distance   = abs(args[2]); // angular distance
	prd.direction  = (args[2] < 0) ? -1 : 1;
	prd.flags      = args[3];

	return EV_DoPolyObjRotate(&prd);
}

// Parses arguments for polyobject flag waving special
static boolean PolyFlag(INT32 *args)
{
	polyflagdata_t pfd;

	pfd.polyObjNum = args[0];
	pfd.speed = args[1];
	pfd.angle = FixedAngle(args[2] << FRACBITS) >> ANGLETOFINESHIFT;
	pfd.momx = args[3];

	return EV_DoPolyObjFlag(&pfd);
}

// Parses arguments for parameterized polyobject move-by-sector-heights specials
static boolean PolyDisplace(INT32 *args, line_t *fallback)
{
	polydisplacedata_t pdd;
	fixed_t speed;
	angle_t angle;

	pdd.polyObjNum = args[0];

	if (args[1] == 0)
	{
		if (fallback == NULL)
		{
			CONS_Debug(DBG_GAMELOGIC, "PolyDisplace: No frontsector to displace from!\n");
			return false;
		}
		pdd.controlSector = fallback->frontsector;
	}
	else
	{
		INT32 destsec = Tag_Iterate_Sectors(args[1], 0);
		if (destsec == -1)
		{
			CONS_Debug(DBG_GAMELOGIC, "PolyDisplace: No tagged sector to displace from (tag %d)!\n", args[1]);
			return false;
		}
		pdd.controlSector = &sectors[destsec];
	}

	speed = args[2] << FRACBITS;
	angle = FixedAngle(args[3] << FRACBITS) >> ANGLETOFINESHIFT;

	pdd.dx = FixedMul(FINECOSINE(angle), speed) >> 8;
	pdd.dy = FixedMul(  FINESINE(angle), speed) >> 8;

	return EV_DoPolyObjDisplace(&pdd);
}

// Parses arguments for parameterized polyobject rotate-by-sector-heights specials
static boolean PolyRotDisplace(INT32 *args, line_t *fallback)
{
	polyrotdisplacedata_t pdd;
	fixed_t anginter, distinter;

	pdd.polyObjNum = args[0];

	if (args[1] == 0)
	{
		if (fallback == NULL)
		{
			CONS_Debug(DBG_GAMELOGIC, "PolyRotDisplace: No frontsector to displace from!\n");
			return false;
		}
		pdd.controlSector = fallback->frontsector;
	}
	else
	{
		INT32 destsec = Tag_Iterate_Sectors(args[1], 0);
		if (destsec == -1)
		{
			CONS_Debug(DBG_GAMELOGIC, "PolyRotDisplace: No tagged sector to displace from (tag %d)!\n", args[1]);
			return false;
		}
		pdd.controlSector = &sectors[destsec];
	}

	// Rotate 'anginter' interval for each 'distinter' interval from the control sector.
	distinter	= args[2] << FRACBITS;
	anginter	= args[3] << FRACBITS;
	pdd.rotscale = FixedDiv(anginter, distinter);

	// Same behavior as other rotators when carrying things.
	pdd.turnobjs = args[4];

	return EV_DoPolyObjRotDisplace(&pdd);
}

/** Finds minimum light from an adjacent sector.
  *
  * \param sector Sector to start in.
  * \param max    Maximum value to return.
  * \return Minimum light value from an adjacent sector, or max if the minimum
  *         light value is greater than max.
  */
INT32 P_FindMinSurroundingLight(sector_t *sector, INT32 max)
{
	size_t i;
	INT32 min = max;
	line_t *line;
	sector_t *check;

	for (i = 0; i < sector->linecount; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line,sector);

		if (!check)
			continue;

		if (check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}

void T_ExecutorDelay(executor_t *e)
{
	if (--e->timer <= 0)
	{
		if (e->caller && P_MobjWasRemoved(e->caller)) // If the mobj died while we were delaying
			P_SetTarget(&e->caller, NULL); // Call with no mobj!
		P_ProcessLineSpecial(e->line, e->caller, e->sector);
		P_SetTarget(&e->caller, NULL); // Let the mobj know it can be removed now.
		P_RemoveThinker(&e->thinker);
	}
}

static void P_AddExecutorDelay(line_t *line, mobj_t *mobj, sector_t *sector)
{
	executor_t *e;
	INT32 delay;

	if (udmf)
		delay = line->executordelay;
	else
	{
		if (!line->backsector)
			I_Error("P_AddExecutorDelay: Line has no backsector!\n");

		delay = (line->backsector->ceilingheight >> FRACBITS) + (line->backsector->floorheight >> FRACBITS);
	}

	e = Z_LevelPoolCalloc(sizeof (*e));
	e->thinker.alloctype = TAT_LEVELPOOL;
	e->thinker.size = sizeof (*e);

	e->thinker.function.acp1 = (actionf_p1)T_ExecutorDelay;
	e->line = line;
	e->sector = sector;
	e->timer = delay;
	P_SetTarget(&e->caller, mobj); // Use P_SetTarget to make sure the mobj doesn't get freed while we're delaying.
	P_AddThinker(THINK_MAIN, &e->thinker);
}

static boolean P_CheckPlayerRings(line_t *triggerline, mobj_t *actor)
{
	INT32 rings = 0;
	INT32 targetrings = triggerline->args[1];
	size_t i;

	// Count all players' rings.
	if (triggerline->args[3])
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo || ((gametyperules & GTR_SPHERES) ? players[i].spheres : players[i].rings) <= 0)
				continue;

			rings += (gametyperules & GTR_SPHERES) ? players[i].spheres : players[i].rings;
		}
	}
	else
	{
		if (!(actor && actor->player))
			return false; // no player to count rings from here, sorry

		rings = (gametyperules & GTR_SPHERES) ? actor->player->spheres : actor->player->rings;
	}

	switch (triggerline->args[2])
	{
		case TMC_EQUAL:
		default:
			return rings == targetrings;
		case TMC_GTE:
			return rings >= targetrings;
		case TMC_LTE:
			return rings <= targetrings;
	}
}

static boolean P_CheckPushables(line_t *triggerline, sector_t *caller)
{
	msecnode_t *node;
	mobj_t *mo;
	INT32 numpushables = 0;
	INT32 targetpushables = triggerline->args[1];

	if (!caller)
		return false; // we need a calling sector to find pushables in, silly!

	// Count the pushables in this sector
	for (node = caller->touching_thinglist; node; node = node->m_thinglist_next)
	{
		mo = node->m_thing;
		if ((mo->flags & MF_PUSHABLE) || ((mo->info->flags & MF_PUSHABLE) && mo->fuse))
			numpushables++;
	}

	switch (triggerline->args[2])
	{
		case TMC_EQUAL:
		default:
			return numpushables == targetpushables;
		case TMC_GTE:
			return numpushables >= targetpushables;
		case TMC_LTE:
			return numpushables <= targetpushables;
	}
}

boolean P_CanActivateSpecial(INT16 special)
{
	switch (special)
	{
		case 2001: // Finish line
		case 2003: // Respawn line
		case 2004: // Bot controller
		case 2005: // Dismount Flying Object
		{
			return true;
		}
		default:
		{
			// Linedef executors
			return (special >= 400 && special < 500);
		}
	}
}

static void P_ActivateLinedefExecutor(line_t *line, mobj_t *actor, sector_t *caller)
{
	if (P_CanActivateSpecial(line->special) == false)
		return;

	if (line->executordelay)
		P_AddExecutorDelay(line, actor, caller);
	else
		P_ProcessLineSpecial(line, actor, caller);
}

static boolean P_ActivateLinedefExecutorsInSector(line_t *triggerline, mobj_t *actor, sector_t *caller)
{
	sector_t *ctlsector = triggerline->frontsector;
	size_t sectori = (size_t)(ctlsector - sectors);
	size_t linecnt = ctlsector->linecount;
	size_t i;

	if (!udmf && triggerline->flags & ML_WRAPMIDTEX) // disregard order for efficiency
	{
		for (i = 0; i < linecnt; i++)
			P_ActivateLinedefExecutor(ctlsector->lines[i], actor, caller);
	}
	else // walk around the sector in a defined order
	{
		boolean backwards = false;
		size_t j, masterlineindex = (size_t)-1;

		for (i = 0; i < linecnt; i++)
			if (ctlsector->lines[i] == triggerline)
			{
				masterlineindex = i;
				break;
			}

#ifdef PARANOIA
		if (masterlineindex == (size_t)-1)
		{
			const size_t li = (size_t)(ctlsector->lines[i] - lines);
			I_Error("Line %s isn't linked into its front sector", sizeu1(li));
		}
#endif

		// i == masterlineindex
		for (;;)
		{
			if (backwards) // v2 to v1
			{
				for (j = 0; j < linecnt; j++)
				{
					if (i == j)
						continue;
					if (ctlsector->lines[i]->v1 == ctlsector->lines[j]->v2)
					{
						i = j;
						break;
					}
					if (ctlsector->lines[i]->v1 == ctlsector->lines[j]->v1)
					{
						i = j;
						backwards = false;
						break;
					}
				}
				if (j == linecnt)
				{
					const size_t vertexei = (size_t)(ctlsector->lines[i]->v1 - vertexes);
					CONS_Debug(DBG_GAMELOGIC, "Warning: Sector %s is not closed at vertex %s (%d, %d)\n",
						sizeu1(sectori), sizeu2(vertexei), ctlsector->lines[i]->v1->x, ctlsector->lines[i]->v1->y);
					return false; // abort
				}
			}
			else // v1 to v2
			{
				for (j = 0; j < linecnt; j++)
				{
					if (i == j)
						continue;
					if (ctlsector->lines[i]->v2 == ctlsector->lines[j]->v1)
					{
						i = j;
						break;
					}
					if (ctlsector->lines[i]->v2 == ctlsector->lines[j]->v2)
					{
						i = j;
						backwards = true;
						break;
					}
				}
				if (j == linecnt)
				{
					const size_t vertexei = (size_t)(ctlsector->lines[i]->v1 - vertexes);
					CONS_Debug(DBG_GAMELOGIC, "Warning: Sector %s is not closed at vertex %s (%d, %d)\n",
						sizeu1(sectori), sizeu2(vertexei), ctlsector->lines[i]->v2->x, ctlsector->lines[i]->v2->y);
					return false; // abort
				}
			}

			if (i == masterlineindex)
				break;

			P_ActivateLinedefExecutor(ctlsector->lines[i], actor, caller);
		}
	}

	return true;
}

/** Used by P_LinedefExecute to check a trigger linedef's conditions
  * The linedef executor specials in the trigger linedef's sector are run if all conditions are met.
  * Return false cancels P_LinedefExecute, this happens if a condition is not met.
  *
  * \param triggerline Trigger linedef to check conditions for; should NEVER be NULL.
  * \param actor Object initiating the action; should not be NULL.
  * \param caller Sector in which the action was started. May be NULL.
  * \sa P_ProcessLineSpecial, P_LinedefExecute
  */
boolean P_RunTriggerLinedef(line_t *triggerline, mobj_t *actor, sector_t *caller)
{
	INT16 specialtype = triggerline->special;

	if (udmf)
		return false;

	////////////////////////
	// Trigger conditions //
	////////////////////////

	switch (specialtype)
	{
		case 303:
			if (!P_CheckPlayerRings(triggerline, actor))
				return false;
			break;
		case 309:
			// Only red/blue team members can activate this.
			if (!(actor && actor->player))
				return false;
			if (actor->player->team != ((triggerline->args[1] == TMT_ORANGE) ? TEAM_ORANGE : TEAM_BLUE))
				return false;
			break;
		case 314:
			if (!P_CheckPushables(triggerline, caller))
				return false;
			break;
		case 317:
			if (actor && actor->player)
			{ // Unlockable triggers required
				INT32 trigid = triggerline->args[1];

				if (trigid < 0 || trigid > 31) // limited by 32 bit variable
				{
					CONS_Debug(DBG_GAMELOGIC, "Unlockable trigger (sidedef %hu): bad trigger ID %d\n", triggerline->sidenum[0], trigid);
					return false;
				}
				else if (!(actor && actor->player))
					return false;
				else if (!(actor->player->roundconditions.unlocktriggers & (1 << trigid)))
					return false;
			}
			break;
		case 319:
			{ // An unlockable itself must be unlocked!
				INT32 unlockid = triggerline->args[1];

				if (modifiedgame && !savemoddata)
					return false;
				else if (unlockid <= 0 || unlockid > MAXUNLOCKABLES) // limited by unlockable count
				{
					CONS_Debug(DBG_GAMELOGIC, "Unlockable check (sidedef %hu): bad unlockable ID %d\n", triggerline->sidenum[0], unlockid);
					return false;
				}
				else if (!(M_CheckNetUnlockByID(unlockid)))
					return false;
			}
			break;
		case 321:
			// decrement calls left before triggering
			if (triggerline->callcount > 0)
			{
				if (--triggerline->callcount > 0)
					return false;
			}
			break;
		case 331:
			if (!(actor && actor->player))
				return false;
			if (!triggerline->stringargs[0])
				return false;
			if (!(stricmp(triggerline->stringargs[0], skins[actor->player->skin]->name) == 0) ^ !!(triggerline->args[1]))
				return false;
			break;
		case 334: // object dye
			{
				INT32 triggercolor = triggerline->stringargs[0] ? get_number(triggerline->stringargs[0]) : SKINCOLOR_NONE;
				UINT16 color = (actor->player ? actor->player->dye : actor->color);

				if (!!(triggerline->args[1]) ^ (triggercolor != color))
					return false;
			}
			break;
		default:
			break;
	}

	/////////////////////////////////
	// Processing linedef specials //
	/////////////////////////////////

	if (!P_ActivateLinedefExecutorsInSector(triggerline, actor, caller))
		return false;

	// "Trigger on X calls" linedefs reset if args[2] is set
	if (specialtype == 321 && triggerline->args[2])
		triggerline->callcount = triggerline->args[1];
	else
	{
		// These special types work only once
		if (specialtype == 313  // No more enemies
			|| specialtype == 321 // Trigger on X calls
			|| specialtype == 399) // Level Load
			triggerline->special = 0;
		else if ((specialtype == 323 // Nightserize
			|| specialtype == 325 // DeNightserize
			|| specialtype == 327 // Nights lap
			|| specialtype == 329) // Nights bonus time
			&& triggerline->args[0])
			triggerline->special = 0;
		else if ((specialtype == 300 // Basic
			|| specialtype == 303 // Ring count
			|| specialtype == 305 // Character ability
			|| specialtype == 308 // Gametype
			|| specialtype == 309 // CTF team
			|| specialtype == 314 // No of pushables
			|| specialtype == 317 // Unlockable trigger
			|| specialtype == 319 // Unlockable
			|| specialtype == 331 // Player skin
			|| specialtype == 334 // Object dye
			|| specialtype == 337) // Emerald check
			&& triggerline->args[0] == TMT_ONCE)
			triggerline->special = 0;
	}

	return true;
}

/** Runs a linedef executor.
  * Can be called by:
  *   - a player moving into a special sector or FOF.
  *   - a pushable object moving into a special sector or FOF.
  *   - a ceiling or floor movement from a previous linedef executor finishing.
  *   - any object in a state with the A_LinedefExecute() action.
  *
  * \param tag Tag of the linedef executor to run.
  * \param actor Object initiating the action; should not be NULL.
  * \param caller Sector in which the action was started. May be NULL.
  * \sa P_ProcessLineSpecial, P_RunTriggerLinedef
  * \author Graue <graue@oceanbase.org>
  */
void P_LinedefExecute(INT16 tag, mobj_t *actor, sector_t *caller)
{
	INT32 masterline;

	CONS_Debug(DBG_GAMELOGIC, "P_LinedefExecute: Executing trigger linedefs of tag %d\n", tag);

	I_Assert(!actor || !P_MobjWasRemoved(actor)); // If actor is there, it must be valid.

	TAG_ITER_LINES(tag, masterline)
	{
		if (lines[masterline].special < 300
			|| lines[masterline].special > 399)
			continue;

		// "No More Enemies" and "Level Load" take care of themselves.
		if (lines[masterline].special == 313  || lines[masterline].special == 399)
			continue;

		// Each-time executors handle themselves, too
		if ((lines[masterline].special == 300 // Basic
			|| lines[masterline].special == 303 // Ring count
			|| lines[masterline].special == 305 // Character ability
			|| lines[masterline].special == 308 // Gametype
			|| lines[masterline].special == 309 // CTF team
			|| lines[masterline].special == 314 // Number of pushables
			|| lines[masterline].special == 317 // Condition set trigger
			|| lines[masterline].special == 319 // Unlockable trigger
			|| lines[masterline].special == 331 // Player skin
			|| lines[masterline].special == 334 // Object dye
			|| lines[masterline].special == 337) // Emerald check
			&& lines[masterline].args[0] > TMT_EACHTIMEMASK)
			continue;

		if (lines[masterline].special == 321 && lines[masterline].args[0] > TMXT_EACHTIMEMASK) // Trigger after X calls
			continue;

		if (!P_RunTriggerLinedef(&lines[masterline], actor, caller))
			return; // cancel P_LinedefExecute if function returns false
	}
}

static void P_PlaySFX(INT32 sfxnum, mobj_t *mo, sector_t *callsec, INT16 tag, textmapsoundsource_t source, textmapsoundlistener_t listener)
{
	if (sfxnum == sfx_None)
		return; // Do nothing!

	if (sfxnum < sfx_None || sfxnum >= NUMSFX)
	{
		CONS_Debug(DBG_GAMELOGIC, "Line type 414 Executor: sfx number %d is invalid!\n", sfxnum);
		return;
	}

	// Check if you can hear the sound
	switch (listener)
	{
		case TMSL_TRIGGERER: // only play sound if displayplayer
			if (!mo)
				return;

			if (!mo->player)
				return;

			if (!P_IsDisplayPlayer(mo->player))
				return;

			break;
		case TMSL_TAGGEDSECTOR: // only play if touching tagged sectors
		{
			UINT8 i = 0;
			mobj_t *camobj = players[displayplayers[0]].mo;
			ffloor_t *rover;
			boolean foundit = false;

			for (i = 0; i <= r_splitscreen; camobj = players[displayplayers[i]].mo, i++)
			{
				if (!camobj)
					continue;

				if (foundit || Tag_Find(&camobj->subsector->sector->tags, tag))
				{
					foundit = true;
					break;
				}

				// Only trigger if mobj is touching the tag
				for (rover = camobj->subsector->sector->ffloors; rover; rover = rover->next)
				{
					if (!Tag_Find(&rover->master->frontsector->tags, tag))
						continue;

					if (camobj->z > P_GetSpecialTopZ(camobj, sectors + rover->secnum, camobj->subsector->sector))
						continue;

					if (camobj->z + camobj->height < P_GetSpecialBottomZ(camobj, sectors + rover->secnum, camobj->subsector->sector))
						continue;

					foundit = true;
					break;
				}
			}

			if (!foundit)
				return;

			break;
		}
		case TMSL_EVERYONE: // no additional check
		default:
			break;
	}

	// Play the sound from the specified source
	switch (source)
	{
		case TMSS_TRIGGERMOBJ: // play the sound from mobj that triggered it
			if (mo)
				S_StartSound(mo, sfxnum);
			break;
		case TMSS_TRIGGERSECTOR: // play the sound from calling sector's soundorg
			if (callsec)
				S_StartSound(&callsec->soundorg, sfxnum);
			else if (mo)
				S_StartSound(&mo->subsector->sector->soundorg, sfxnum);
			break;
		case TMSS_NOWHERE: // play the sound from nowhere
			S_StartSound(NULL, sfxnum);
			break;
		case TMSS_TAGGEDSECTOR: // play the sound from tagged sectors' soundorgs
		{
			INT32 secnum;

			TAG_ITER_SECTORS(tag, secnum)
				S_StartSound(&sectors[secnum].soundorg, sfxnum);
			break;
		}
		default:
			break;
	}
}

//
// P_SwitchWeather
//
// Switches the weather!
//
void P_SwitchWeather(preciptype_t newWeather)
{
	boolean purge = false;
	mobjtype_t swap = MT_NULL;
	INT32 oldEffects = precipprops[curWeather].effects;

	if (newWeather >= precip_freeslot)
	{
		// Weather type invalid, set to no weather.
		CONS_Debug(DBG_SETUP, "Weather ID %d out of bounds\n", newWeather);
		newWeather = PRECIP_NONE;
	}

	if (precipprops[newWeather].type == MT_NULL)
	{
		// New type is null, we want to purge the weather.
		if (precipprops[curWeather].type == MT_NULL)
			return; // Nothing to do.
		purge = true;
	}
	else
	{
		if (precipprops[curWeather].type != MT_NULL)
		{
			// There are already existing weather particles to reuse.
			swap = precipprops[newWeather].type;
		}
	}

	curWeather = newWeather;

	if (purge == true)
	{
		thinker_t *think;
		thinker_t *next;
		precipmobj_t *precipmobj;

		for (think = thlist[THINK_PRECIP].next; think != &thlist[THINK_PRECIP]; think = next)
		{
			next = think->next;

			if (think->function.acp1 != (actionf_p1)P_NullPrecipThinker)
				continue; // not a precipmobj thinker

			precipmobj = (precipmobj_t *)think;
			P_FreePrecipMobj(precipmobj);
		}
	}
	else if (swap != MT_NULL) // Rather than respawn all that crap, reuse it!
	{
		UINT8 randomstates = (UINT8)mobjinfo[swap].damage;
		thinker_t *think;
		precipmobj_t *precipmobj;
		statenum_t st;

		for (think = thlist[THINK_PRECIP].next; think != &thlist[THINK_PRECIP]; think = think->next)
		{
			if (think->function.acp1 != (actionf_p1)P_NullPrecipThinker)
				continue; // not a precipmobj thinker

			precipmobj = (precipmobj_t *)think;

			precipmobj->flags = mobjinfo[swap].flags;

			st = mobjinfo[swap].spawnstate;

			if (randomstates > 0)
			{
				UINT8 mrand = M_RandomByte();
				UINT8 threshold = UINT8_MAX / (randomstates + 1);
				UINT8 i;

				for (i = 0; i < randomstates; i++)
				{
					if (mrand < (threshold * (i+1)))
					{
						st += i+1;
						break;
					}
				}
			}

			precipmobj->state = &states[st];
			precipmobj->tics = precipmobj->state->tics;
			precipmobj->sprite = precipmobj->state->sprite;
			precipmobj->frame = precipmobj->state->frame;

			precipmobj->momz = FixedMul(-mobjinfo[swap].speed, mapobjectscale);
			precipmobj->precipflags &= ~(PCF_INVISIBLE|PCF_FLIP);

			if (precipmobj->momz > 0)
			{
				precipmobj->precipflags |= PCF_FLIP;
			}

			if ((oldEffects & PRECIPFX_WATERPARTICLES) != (precipprops[curWeather].effects & PRECIPFX_WATERPARTICLES))
			{
				P_CalculatePrecipFloor(precipmobj);
			}
		}
	}

	if (swap == MT_NULL && precipprops[curWeather].type != MT_NULL)
		P_SpawnPrecipitation();
}

static boolean K_IgnoreFinishLine(player_t *player)
{
	// Lightsnake travels to the first waypoint in a straight
	// line (init).
	// This has the potential to inadvertently cross a finish
	// line and remove a lap (happened on Hardhat Havoc).
	// After the first waypoint, lightsnake follows the
	// waypoints in order so it's not an issue there.
	if (player->respawn.state == RESPAWNST_MOVE && player->respawn.init == true)
		return true;

	// If potential lap cheating has been detected, do not
	// interact with the finish line at all.
	if (player->bigwaypointgap)
		return true;

	return false;
}

// Passed over the finish line forwards
static void K_HandleLapIncrement(player_t *player)
{
	if (player)
	{
		if (K_IgnoreFinishLine(player))
			return;
		if (!G_TimeAttackStart() && leveltime < starttime && !(gametyperules & GTR_ROLLINGSTART))
		{
			// freeze 'em until fault penalty is over
			player->mo->hitlag = starttime - leveltime + 2*TICRATE;
			P_ResetPlayer(player);
			player->pflags |= PF_VOID;
			player->mo->renderflags |= RF_DONTDRAW;
			player->mo->flags |= MF_NOCLIPTHING;
			player->nocontrol = UINT16_MAX;
			player->mo->momx = 0;
			player->mo->momy = 0;
			player->mo->momz = 0;
			K_StripItems(player);
			player->faultflash = TICRATE/3;
			player->karthud[khud_fault] = 1;
			ClearFakePlayerSkin(player);
			S_StartSound(player->mo, sfx_s3k8a);
			P_MoveOrigin(player->mo, player->mo->old_x, player->mo->old_y, player->mo->z);

			if (player->roundconditions.faulted == false)
			{
				player->roundconditions.faulted = true;
				player->roundconditions.checkthisframe = true;
			}

			if (P_IsDisplayPlayer(player))
			{
				S_StartSound(player->mo, sfx_s3kb2);
			}

			player->karthud[khud_splitcolor] = 0;
			player->karthud[khud_splitposition] = 1;
			player->karthud[khud_splitskin] = -1;
			player->karthud[khud_splittime] = (INT32)(starttime - leveltime);
			player->karthud[khud_splittimer] = 3*TICRATE;
			player->karthud[khud_splitwin] = -2;

			return;
		}

		if ((player->cheatchecknum == numcheatchecks) || (player->laps == 0))
		{
			size_t i = 0;
			UINT8 nump = 0;
			UINT8 lowestLap = UINT8_MAX;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;
				if (players[i].laps < lowestLap)
					lowestLap = players[i].laps;
				nump++;
			}

			player->cheatchecknum = 0;
			player->laps++;

			// P_DoPlayerExit can edit latestlap, so we do this first
			boolean lapisfresh = (player->laps > player->latestlap);
			if (lapisfresh) // mcgamer would be proud
			{
				player->latestlap = player->laps;
			}

			// finished race exit setup
			if (player->laps > numlaps)
			{
				pflags_t applyflags = 0;
				if (specialstageinfo.valid == true)
				{
					// Don't permit a win just by sneaking ahead of the UFO/emerald.
					if (K_PlayerIsEmptyHandedInSpecial(player))
					{
						applyflags |= PF_NOCONTEST;

						HU_DoTitlecardCEcho(player, "EMPTY\\HANDED?", false);
					}
				}

				P_DoPlayerExit(player, applyflags);

				if (netgame && lapisfresh)
					CON_LogMessage(va(M_GetText("%s has finished the race.\n"), player_names[player-players]));
			}
			else
			{
				UINT32 skinflags = (demo.playback)
					? demo.skinlist[demo.currentskinid[(player-players)]].flags
					: skins[player->skin]->flags;
				if (skinflags & SF_IRONMAN)
				{
					if ((player->laps == 1 && lapisfresh) || !K_InRaceDuel()) // We'll do this in K_CheckpointCrossAward if necessary.
						SetRandomFakePlayerSkin(player, true, false);
				}

				// Always trust waypoints entering the first lap.
				// This accounts for special POSITION room setups.
				// debuglapcheat can be used to expose errors that would be hidden by this exception.
				extern consvar_t cv_debuglapcheat;
				if (!cv_debuglapcheat.value && player->laps == 1)
					player->pflags |= PF_TRUSTWAYPOINTS;

				K_UpdateAllPlayerPositions(); // P_DoPlayerExit calls this
			}

			if (!G_TimeAttackStart() && !(gametyperules & GTR_ROLLINGSTART) && player->laps == 1 && lapisfresh)
			{
				boolean setupsplits = false;

				if (rainbowstartavailable)
				{
					// CONS_Printf("%d: %s gimme first blood\n", leveltime, player_names[player - players]);
					player->pflags2 |= PF2_GIMMEFIRSTBLOOD;
					setupsplits = true;
				}
				else if (!K_InRaceDuel() && M_NotFreePlay())
				{
					// CONS_Printf("%d: %s gimme start award\n", leveltime, player_names[player - players]);
					player->pflags2 |= PF2_GIMMESTARTAWARDS;
					setupsplits = true;
				}

				if (setupsplits)
				{
					player->karthud[khud_splitcolor] = 0;
					player->karthud[khud_splitposition] = 1;
					player->karthud[khud_splitskin] = -1;
					player->karthud[khud_splittime] = (INT32)(starttime - leveltime);
					player->karthud[khud_splittimer] = 2*TICRATE;
					player->karthud[khud_splitwin] = (rainbowstartavailable) ? 2 : 0;
				}
			}

			if (rainbowstartavailable == true && player->mo->hitlag == 0 && G_TimeAttackStart())
			{
				S_StartSound(player->mo, sfx_s23c);
				player->startboost = 125;

				K_SpawnDriftBoostExplosion(player, 4);
				K_SpawnDriftElectricSparks(player, SKINCOLOR_SILVER, false);

				rainbowstartavailable = false;
			}

			if (player->laps == 1 && (modeattacking & ATTACKING_SPB))
			{
				P_SpawnMobj(player->mo->x - FixedMul(1000*mapobjectscale, FINECOSINE(player->mo->angle >> ANGLETOFINESHIFT)),
					player->mo->y - FixedMul(1000*mapobjectscale, FINESINE(player->mo->angle >> ANGLETOFINESHIFT)),
					player->mo->z, MT_SPB);
			}

			if (gametyperules & GTR_SPECIALSTART)
			{
				if (player->laps > numlaps)
				{
					// Warp out
					S_StartSound(NULL, sfx_s3kb3);
				}
			}
			else if (P_IsDisplayPlayer(player))
			{
				if (K_InRaceDuel())
					S_StartSound(NULL, sfx_s221);
				else if (numlaps > 1 && player->laps == numlaps) // final lap
					S_StartSound(NULL, sfx_s3k68);
				else if ((player->laps > 1) && (player->laps < numlaps)) // non-final lap
					S_StartSound(NULL, sfx_s221);
				else if (player->laps > numlaps)
				{
					// finished
					S_StartSound(NULL, sfx_s3k6a);
				}

			}
			else
			{
				if ((player->laps > numlaps) && (player->position == 1) && (!K_InRaceDuel()))
				{
					// opponent finished
					S_StartSound(NULL, sfx_s253);
				}
			}

			if (lapisfresh)
			{
				if (player->laps > 1)
				{
					// save best lap for record attack
					if (player->laptime[LAP_CUR] < player->laptime[LAP_BEST] || player->laptime[LAP_BEST] == 0)
					{
						player->laptime[LAP_BEST] = player->laptime[LAP_CUR];
					}

					player->laptime[LAP_LAST] = player->laptime[LAP_CUR];
					player->laptime[LAP_CUR] = 0;

					UINT16 oldexp = player->exp;
					K_CheckpointCrossAward(player);
					K_UpdatePowerLevels(player, player->gradingpointnum, false);

					if (player->exp > oldexp)
					{
						K_SpawnFinishEXP(player, player->exp - oldexp);
					}

					if (player->position == 1 && !(gametyperules & GTR_CHECKPOINTS))
					{
						Obj_DeactivateCheckpoints();
					}
					player->checkpointId = 0;
				}

				// Set up lap animation vars
				if (player->latestlap > 1)
				{
					if (nump > 1)
					{
						if (K_IsPlayerLosing(player))
							player->karthud[khud_laphand] = 3;
						else
						{
							if (nump > 2 && player->position == 1) // 1st place in 1v1 uses thumbs up
								player->karthud[khud_laphand] = 1;
							else
								player->karthud[khud_laphand] = 2;
						}
					}
					else
						player->karthud[khud_laphand] = 0; // No hands in FREE PLAY

					player->karthud[khud_lapanimation] = 80;
				}
			}

			thwompsactive = true; // Lap 2 effects

			{
				UINT8 prevLowest = lowestLap;

				lowestLap = P_FindLowestLap();

				if (lowestLap > prevLowest) // last place finished the lap
				{
					Obj_UpdateRocks();
				}
			}

			for (i = 0; i < numlines; i++)
			{
				if (lines[i].special == 2002) // Race lap trigger
				{
					UINT8 lap;

					if (lines[i].flags & ML_MIDSOLID)
					{
						lap = player->laps;
					}
					else
					{
						lap = lowestLap;

						if (lap <= lastLowestLap)
						{
							// Need to be able to search for E4 linedefs
							continue;
						}
					}

					if (lines[i].flags & ML_NOCLIMB) // Need higher than or equal to
					{
						if (lap < (sides[lines[i].sidenum[0]].textureoffset >> FRACBITS))
							continue;
					}
					else if (lines[i].flags & ML_BLOCKPLAYERS) // Need lower than or equal to
					{
						if (lap > (sides[lines[i].sidenum[0]].textureoffset >> FRACBITS))
							continue;
					}
					else // Need equal to
					{
						if (lap != (sides[lines[i].sidenum[0]].textureoffset >> FRACBITS))
							continue;
					}

					P_RunTriggerLinedef(&lines[i], player->mo, NULL);
				}
			}

			lastLowestLap = lowestLap;

			if (P_IsPartyPlayer(player))
			{
				player->roundconditions.checkthisframe = true;
				gamedata->deferredconditioncheck = true;
			}
		}
		else if (player->cheatchecknum)
		{
			S_StartSound(player->mo, sfx_s26d);
		}
	}
}

// player went backwards over the line
static void K_HandleLapDecrement(player_t *player)
{
	if (player)
	{
		if (K_IgnoreFinishLine(player))
			return;
		if ((player->cheatchecknum == 0) && (player->laps > 0))
		{
			player->cheatchecknum = numcheatchecks;
			player->laps--;
			K_UpdateAllPlayerPositions();
			player->laptime[LAP_CUR] = UINT32_MAX;
		}
	}
}

static void P_LineSpecialWasActivated(line_t *line)
{
	if (!(line->activation & SPAC_REPEATSPECIAL))
	{
		line->special = 0;
	}
}

static boolean P_AllowSpecialCross(line_t *line, mobj_t *thing)
{
	if (P_CanActivateSpecial(line->special) == false)
	{
		// No special to even activate.
		return false;
	}

	if (thing->player != NULL)
	{
		return !!(line->activation & SPAC_CROSS);
	}
	else if ((thing->flags & (MF_ENEMY|MF_BOSS)) != 0)
	{
		return !!(line->activation & SPAC_CROSSMONSTER);
	}
	else if (K_IsMissileOrKartItem(thing) == true)
	{
		return !!(line->activation & SPAC_CROSSMISSILE);
	}

	// No activation flags for you.
	return false;
}

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with specific specials
//
void P_CrossSpecialLine(line_t *line, INT32 side, mobj_t *thing)
{
	player_t *player = NULL;
	activator_t *activator = NULL;
	boolean result = false;

	if (thing == NULL || P_MobjWasRemoved(thing) == true || thing->health <= 0)
	{
		// Invalid mobj.
		return;
	}

	player = thing->player;

	if (player != NULL)
	{
		if (player->spectator == true)
		{
			// Ignore spectators.
			return;
		}

		if (player->pflags & PF_NOCONTEST)
		{
			// Ignore NO CONTEST.
			return;
		}

		// Tripwire effect
		if (P_IsLineTripWire(line))
		{
			K_ApplyTripWire(player, TRIPSTATE_PASSED);
		}
	}

	if (P_AllowSpecialCross(line, thing) == false)
	{
		// This special can't be activated this way.
		return;
	}

	activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
	I_Assert(activator != NULL);

	P_SetTarget(&activator->mo, thing);
	activator->line = line;
	activator->side = side;
	activator->sector = (side != 0) ? line->backsector : line->frontsector;

	result = P_ProcessSpecial(activator, line->special, line->args, line->stringargs);

	P_SetTarget(&activator->mo, NULL);
	Z_Free(activator);

	if (result == true)
	{
		P_LineSpecialWasActivated(line);
	}
}

static boolean P_AllowSpecialPush(line_t *line, mobj_t *thing)
{
	if (P_CanActivateSpecial(line->special) == false)
	{
		// No special to even activate.
		return false;
	}

	if (thing->player != NULL)
	{
		return !!(line->activation & SPAC_PUSH);
	}
	else if ((thing->flags & (MF_ENEMY|MF_BOSS)) != 0)
	{
		return !!(line->activation & SPAC_PUSHMONSTER);
	}
	else if (K_IsMissileOrKartItem(thing) == true)
	{
		return !!(line->activation & SPAC_IMPACT);
	}

	// No activation flags for you.
	return false;
}

//
// P_PushSpecialLine - TRIGGER
// Called every time a thing origin is blocked
//  by a line with specific specials
//
void P_PushSpecialLine(line_t *line, mobj_t *thing)
{
	player_t *player = NULL;
	activator_t *activator = NULL;
	boolean result = false;

	if (thing == NULL || P_MobjWasRemoved(thing) == true || thing->health <= 0)
	{
		// Invalid mobj.
		return;
	}

	if (line == NULL)
	{
		// Invalid line.
		return;
	}

	player = thing->player;

	if (player != NULL)
	{
		if (player->spectator == true)
		{
			// Ignore spectators.
			return;
		}

		if (player->pflags & PF_NOCONTEST)
		{
			// Ignore NO CONTEST.
			return;
		}
	}

	if (P_AllowSpecialPush(line, thing) == false)
	{
		// This special can't be activated this way.
		return;
	}

	activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
	I_Assert(activator != NULL);

	P_SetTarget(&activator->mo, thing);
	activator->line = line;
	activator->side = P_PointOnLineSide(thing->x, thing->y, line);
	activator->sector = (activator->side != 0) ? line->backsector : line->frontsector;

	result = P_ProcessSpecial(activator, line->special, line->args, line->stringargs);

	P_SetTarget(&activator->mo, NULL);
	Z_Free(activator);

	if (result == true)
	{
		P_LineSpecialWasActivated(line);
	}
}

//
// P_ActivateThingSpecial - TRIGGER
// Called when a thing is killed, or upon
//  any other type-specific conditions
//
void P_ActivateThingSpecial(mobj_t *mo, mobj_t *source)
{
	player_t *player = NULL;
	activator_t *activator = NULL;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid mobj.
		return;
	}

	// Is this necessary? Probably not, but I hate
	// spectators so I will manually ensure they
	// can't impact the gamestate anyway.
	player = mo->player;
	if (player != NULL)
	{
		if (player->spectator == true)
		{
			// Ignore spectators.
			return;
		}

		if (player->pflags & PF_NOCONTEST)
		{
			// Ignore NO CONTEST.
			return;
		}
	}

	if (P_CanActivateSpecial(mo->special) == false)
	{
		// No special to even activate.
		return;
	}

	activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
	I_Assert(activator != NULL);

	if (source != NULL)
	{
		P_SetTarget(&activator->mo, source);
		activator->sector = source->subsector->sector;
	}

	P_ProcessSpecial(activator, mo->special, mo->script_args, mo->script_stringargs);

	P_SetTarget(&activator->mo, NULL);
	Z_Free(activator);
}

/** Gets an object.
  *
  * \param type Object type to look for.
  * \param s Sector number to look in.
  * \return Pointer to the first ::type found in the sector.
  * \sa P_GetPushThing
  */
static mobj_t *P_GetObjectTypeInSectorNum(mobjtype_t type, size_t s)
{
	sector_t *sec = sectors + s;
	mobj_t *thing = sec->thinglist;

	while (thing)
	{
		if (thing->type == type)
			return thing;
		thing = thing->snext;
	}
	return NULL;
}

static void P_SwitchSkybox(INT32 args, player_t *player, skybox_t *skybox)
{
	// After a player finishes, their camera is locked at the
	// finish line. If bot behavior drives them through the
	// circuit and changes the skybox, that can look very
	// strange if the skybox change is intended to be
	// localized to a certain section of the level.
	if (player->exiting)
	{
		return;
	}

	if (args != TMS_CENTERPOINT) // Only viewpoint, or both.
	{
		P_SetTarget(&player->skybox.viewpoint, skybox->viewpoint);
	}

	if (args != TMS_VIEWPOINT) // Only centerpoint, or both.
	{
		P_SetTarget(&player->skybox.centerpoint, skybox->centerpoint);
	}
}

mobj_t* P_FindObjectTypeFromTag(mobjtype_t type, mtag_t tag)
{
	if (udmf)
	{
		INT32 mtnum;
		mobj_t *mo;

		TAG_ITER_THINGS(tag, mtnum)
		{
			mo = mapthings[mtnum].mobj;

			if (!mo)
				continue;

			if (mo->type != type)
				continue;

			return mo;
		}

		return NULL;
	}
	else
	{
		INT32 secnum;

		if ((secnum = Tag_Iterate_Sectors(tag, 0)) < 0)
			return NULL;

		return P_GetObjectTypeInSectorNum(type, secnum);
	}
}

static void K_UpdateBotControllers(INT32 *args)
{
	INT32 secnum;

	TAG_ITER_SECTORS(args[0], secnum)
	{
		sector_t *const sec = sectors + secnum;

		sec->botController.trick = args[1];
		sec->botController.flags = args[2];
		sec->botController.forceAngle = FixedAngle(args[3] * FRACUNIT);
	}
}

/** Processes the line special triggered by an object.
  *
  * \param line Line with the special command on it.
  * \param mo   mobj that triggered the line. Must be valid and non-NULL.
  * \param callsec sector in which action was initiated; this can be NULL.
  *        Because of the A_LinedefExecute() action, even if non-NULL,
  *        this sector might not have the same tag as the linedef executor,
  *        and it might not have the linedef executor sector type.
  * \todo Handle mo being NULL gracefully. T_MoveFloor() and T_MoveCeiling()
  *       don't have an object to pass.
  * \todo Split up into multiple functions.
  * \sa P_LinedefExecute
  * \author Graue <graue@oceanbase.org>
  */
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec)
{
	// This is an old function purely for linedef executor
	// backwards compatibility.

	activator_t *activator = NULL;

	if (line == NULL)
	{
		// No line to activate
		return;
	}

	activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
	I_Assert(activator != NULL);

	P_SetTarget(&activator->mo, mo);
	activator->line = line;
	activator->sector = callsec;

	P_ProcessSpecial(activator, line->special, line->args, line->stringargs);

	P_SetTarget(&activator->mo, NULL);
	Z_Free(activator);

	// Intentionally no P_LineSpecialWasActivated call.
}

boolean P_ProcessSpecial(activator_t *activator, INT16 special, INT32 *args, char **stringargs)
{
	line_t *const line = activator->line; // If called from a linedef executor, this is the control sector linedef. If from a script, then it's the actual activator.
	UINT8 const side = activator->side;
	mobj_t *const mo = activator->mo;
	sector_t *const callsec = activator->sector;

	// All of these conditions being met means this is a binary map using a linedef executor.
	boolean const backwardsCompat = (!udmf && activator->fromLineSpecial && line != NULL);

	INT32 secnum = -1;

	// note: only specials that P_CanActivateSpecial returns true on can be used
	switch (special)
	{
		case 400: // Copy tagged sector's heights/flats
			{
				sector_t *copySector = NULL;

				if (args[0] == 0)
				{
					if (line == NULL)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 400 Executor: No frontsector to copy planes from!\n");
						return false;
					}
					copySector = line->frontsector;
				}
				else
				{
					INT32 destsec = Tag_Iterate_Sectors(args[0], 0);
					if (destsec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 400 Executor: No sector to copy planes from (tag %d)!\n", args[0]);
						return false;
					}
					copySector = &sectors[destsec];
				}

				if (args[2] != TMP_CEILING)
				{
					EV_DoInstantMoveFloorByHeight(
						args[1],
						copySector->floorheight,
						args[3] ? copySector->floorpic : -1
					);
				}

				if (args[2] != TMP_FLOOR)
				{
					EV_DoInstantMoveCeilingByHeight(
						args[1],
						copySector->ceilingheight,
						args[3] ? copySector->ceilingpic : -1
					);
				}
			}
			break;

		case 402: // Copy light level to tagged sectors
			{
				sector_t *copySector = NULL;

				INT16 newlightlevel;
				INT16 newfloorlightlevel, newceilinglightlevel;
				boolean newfloorlightabsolute, newceilinglightabsolute;
				INT32 newfloorlightsec, newceilinglightsec;

				if (args[0] == 0)
				{
					if (line == NULL)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 402 Executor: No frontsector to copy light level from!\n");
						return false;
					}
					copySector = line->frontsector;
				}
				else
				{
					INT32 destsec = Tag_Iterate_Sectors(args[0], 0);
					if (destsec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 402 Executor: No sector to copy light level from (tag %d)!\n", args[0]);
						return false;
					}
					copySector = &sectors[destsec];
				}

				newlightlevel = copySector->lightlevel;
				newfloorlightlevel = copySector->floorlightlevel;
				newfloorlightabsolute = copySector->floorlightabsolute;
				newceilinglightlevel = copySector->ceilinglightlevel;
				newceilinglightabsolute = copySector->ceilinglightabsolute;
				newfloorlightsec = copySector->floorlightsec;
				newceilinglightsec = copySector->ceilinglightsec;

				TAG_ITER_SECTORS(args[1], secnum)
				{
					if (sectors[secnum].lightingdata)
					{
						// Stop the lighting madness going on in this sector!
						P_RemoveThinker(&((thinkerdata_t *)sectors[secnum].lightingdata)->thinker);
						sectors[secnum].lightingdata = NULL;
					}

					if (!(args[2] & TMLC_NOSECTOR))
						sectors[secnum].lightlevel = newlightlevel;

					if (!(args[2] & TMLC_NOFLOOR))
					{
						sectors[secnum].floorlightlevel = newfloorlightlevel;
						sectors[secnum].floorlightabsolute = newfloorlightabsolute;
						sectors[secnum].floorlightsec = newfloorlightsec;
					}

					if (!(args[2] & TMLC_NOCEILING))
					{
						sectors[secnum].ceilinglightlevel = newceilinglightlevel;
						sectors[secnum].ceilinglightabsolute = newceilinglightabsolute;
						sectors[secnum].ceilinglightsec = newceilinglightsec;
					}
				}
			}
			break;

		case 403: // Copy-move tagged sector's heights/flats
			{
				sector_t *copySector = NULL;

				if (args[0] == 0)
				{
					if (line == NULL)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 403 Executor: No frontsector to copy planes from!\n");
						return false;
					}
					copySector = line->frontsector;
				}
				else
				{
					INT32 destsec = Tag_Iterate_Sectors(args[0], 0);
					if (destsec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 403 Executor: No sector to copy planes from (tag %d)!\n", args[0]);
						return false;
					}
					copySector = &sectors[destsec];
				}

				if (args[2] != TMP_CEILING)
				{
					EV_DoMoveFloorByHeight(
						args[1],
						copySector->floorheight,
						args[3] << (FRACBITS - 3),
						args[4],
						args[5] ? copySector->floorpic : -1
					);
				}

				if (args[2] != TMP_FLOOR)
				{
					EV_DoMoveCeilingByHeight(
						args[1],
						copySector->ceilingheight,
						args[3] << (FRACBITS - 3),
						args[4],
						args[5] ? copySector->ceilingpic : -1
					);
				}
			}
			break;

		case 405: // Move planes by offsets
			if (args[1] != TMP_CEILING)
			{
				EV_DoMoveFloorByDistance(
					args[0],
					args[2] << FRACBITS,
					args[3] << (FRACBITS - 3),
					args[4]
				);
			}

			if (args[1] != TMP_FLOOR)
			{
				EV_DoMoveCeilingByDistance(
					args[0],
					args[2] << FRACBITS,
					args[3] << (FRACBITS - 3),
					args[4]
				);
			}
			break;

		case 408: // Copy flats
		{
			sector_t *copySector = NULL;

			if (args[0] == 0)
			{
				if (line == NULL)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 408 Executor: No frontsector to copy flats from!\n");
					return false;
				}
				copySector = line->frontsector;
			}
			else
			{
				INT32 destsec = Tag_Iterate_Sectors(args[0], 0);
				if (destsec == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 408 Executor: No sector to copy flats from (tag %d)!\n", args[0]);
					return false;
				}
				copySector = &sectors[destsec];
			}

			TAG_ITER_SECTORS(args[1], secnum)
			{
				if (args[2] != TMP_CEILING)
					sectors[secnum].floorpic = copySector->floorpic;
				if (args[2] != TMP_FLOOR)
					sectors[secnum].ceilingpic = copySector->ceilingpic;
			}
			break;
		}

		case 409: // Change tagged sectors' tag
		// (formerly "Change calling sectors' tag", but behavior was changed)
		{
			mtag_t newtag = args[1];

			TAG_ITER_SECTORS(args[0], secnum)
			{
				switch (args[2])
				{
					case TMT_ADD:
						Tag_SectorAdd(secnum, newtag);
						break;
					case TMT_REMOVE:
						Tag_SectorRemove(secnum, newtag);
						break;
					case TMT_REPLACEFIRST:
					default:
						Tag_SectorFSet(secnum, newtag);
						break;
				}
			}
			break;
		}

		case 410: // Change front sector's tag
		{
			mtag_t newtag;
			line_t *editLine = NULL;

			if (args[0] == 0)
			{
				if (line == NULL)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 410 Executor: No linedef to change frontsector tag of!\n");
					return false;
				}
				editLine = line;
			}
			else
			{
				INT32 destline = Tag_Iterate_Sectors(args[0], 0);
				if (destline == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 408 Executor: No linedef to change frontsector tag of (tag %d)!\n", args[0]);
					return false;
				}
				editLine = &lines[destline];
			}

			newtag = args[1];
			secnum = (UINT32)(editLine->frontsector - sectors);

			switch (args[2])
			{
				case TMT_ADD:
					Tag_SectorAdd(secnum, newtag);
					break;
				case TMT_REMOVE:
					Tag_SectorRemove(secnum, newtag);
					break;
				case TMT_REPLACEFIRST:
				default:
					Tag_SectorFSet(secnum, newtag);
					break;
			}
			break;
		}

		case 411: // Stop floor/ceiling movement in tagged sector(s)
			TAG_ITER_SECTORS(args[0], secnum)
			{
				if (sectors[secnum].floordata)
				{
					if (sectors[secnum].floordata == sectors[secnum].ceilingdata) // elevator
					{
						P_RemoveThinker(&((elevator_t *)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = sectors[secnum].ceilingdata = NULL;
						sectors[secnum].floorspeed = sectors[secnum].ceilspeed = 0;
					}
					else // floormove
					{
						P_RemoveThinker(&((floormove_t *)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = NULL;
						sectors[secnum].floorspeed = 0;
					}
				}

				if (sectors[secnum].ceilingdata) // ceiling
				{
					P_RemoveThinker(&((ceiling_t *)sectors[secnum].ceilingdata)->thinker);
					sectors[secnum].ceilingdata = NULL;
					sectors[secnum].ceilspeed = 0;
				}
			}
			break;

		case 412: // Teleport the player or thing
			{
				mobj_t *dest;

				if (!mo) // nothing to teleport
					return false;

				if (args[1] & TMT_RELATIVE) // Relative silent teleport
				{
					fixed_t x, y, z;

					x = args[2] << FRACBITS;
					y = args[3] << FRACBITS;
					z = args[4] << FRACBITS;

					P_SetOrigin(mo, mo->x + x, mo->y + y, mo->z + z);

					if (mo->player)
					{
						UINT8 i;

						for (i = 0; i <= r_splitscreen; i++)
						{
							if (mo->player == &players[displayplayers[i]] && camera[i].chase)
							{
								camera[i].x += x;
								camera[i].y += y;
								camera[i].z += z;
								camera[i].subsector = R_PointInSubsector(camera[i].x, camera[i].y);
								R_RelativeTeleportViewInterpolation(i, x, y, z, 0);
							}
						}
					}
				}
				else
				{
					angle_t angle;
					boolean silent, keepmomentum;

					dest = P_FindObjectTypeFromTag(MT_TELEPORTMAN, args[0]);
					if (!dest)
						return false;

					angle = (args[1] & TMT_KEEPANGLE) ? mo->angle : dest->angle;
					silent = !!(args[1] & TMT_SILENT);
					keepmomentum = !!(args[1] & TMT_KEEPMOMENTUM);

					P_Teleport(mo, dest->x, dest->y, dest->z, angle, !silent, keepmomentum);

					if (!silent)
						S_StartSound(dest, sfx_mixup); // Play the 'bowrwoosh!' sound
				}
			}
			break;

		case 414: // Play SFX
			P_PlaySFX(stringargs[0] ? get_number(stringargs[0]) : sfx_None, mo, callsec, args[3], args[1], args[2]);
			break;

		case 416: // Spawn adjustable fire flicker
			TAG_ITER_SECTORS(args[0], secnum)
				P_SpawnAdjustableFireFlicker(&sectors[secnum], args[2],
					args[3] ? sectors[secnum].lightlevel : args[4], args[1]);
			break;

		case 417: // Spawn adjustable glowing light
			TAG_ITER_SECTORS(args[0], secnum)
				P_SpawnAdjustableGlowingLight(&sectors[secnum], args[2],
					args[3] ? sectors[secnum].lightlevel : args[4], args[1]);
			break;

		case 418: // Spawn adjustable strobe flash
			TAG_ITER_SECTORS(args[0], secnum)
				P_SpawnAdjustableStrobeFlash(&sectors[secnum], args[3],
					(args[4] & TMB_USETARGET) ? sectors[secnum].lightlevel : args[5],
					args[1], args[2], args[4] & TMB_SYNC);
			break;

		case 420: // Fade light levels in tagged sectors to new value
			P_FadeLight(args[0], args[1], args[2], args[3] & TMF_TICBASED, args[3] & TMF_OVERRIDE, args[3] & TMF_RELATIVE);
			break;

		case 421: // Stop lighting effect in tagged sectors
			TAG_ITER_SECTORS(args[0], secnum)
				if (sectors[secnum].lightingdata)
				{
					P_RemoveThinker(&((thinkerdata_t *)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}
			break;

		case 422: // Cut away to another view
			{
				altview_t *modifyView = NULL;
				mobj_t *newViewMobj = NULL;
				INT32 i;

				if (gamestate != GS_LEVEL)
				{
					modifyView = &titlemapcam;
				}
				else if (mo != NULL && mo->player != NULL)
				{
					modifyView = &mo->player->awayview;
				}
				else
				{
					return false;
				}

				newViewMobj = P_FindObjectTypeFromTag(MT_ALTVIEWMAN, args[0]);
				if (newViewMobj == NULL)
				{
					return false;
				}

				P_SetTarget(&modifyView->mobj, newViewMobj);

				if (gamestate != GS_LEVEL)
				{
					// If titlemap, awayview.tics is ignored
					modifyView->tics = -1;

					R_ResetViewInterpolation(0);
				}
				else
				{
					modifyView->tics = args[1];

					for (i = 0; i <= r_splitscreen; i++)
					{
						if (displayplayers[i] == (mo->player - players))
						{
							R_ResetViewInterpolation(i + 1);
						}
					}
				}

				if (args[2] != 0)
				{
					switch (args[2])
					{
						case TMCAM_FIRST:
						case TMCAM_SECOND:
						case TMCAM_THIRD:
						{
							mobj_t *firstPlace = NULL;

							for (i = 0; i < MAXPLAYERS; i++)
							{
								player_t *player = NULL;

								if (playeringame[i] == false)
								{
									continue;
								}

								player = &players[i];
								if (player->spectator == true)
								{
									continue;
								}

								if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
								{
									continue;
								}

								if (player->position == abs(args[2])) // a bit of a hack
								{
									firstPlace = player->mo;
									break;
								}
							}

							P_SetTarget(
								&newViewMobj->target,
								firstPlace
							);
							break;
						}
						case TMCAM_CONSOLE:
						{
							mobj_t *consoleMo = NULL;
							if (playeringame[consoleplayer] == true)
							{
								consoleMo = players[consoleplayer].mo;
							}

							P_SetTarget(
								&newViewMobj->target,
								consoleMo
							);
							break;
						}
						default:
						{
							P_SetTarget(
								&newViewMobj->target,
								P_FindMobjFromTID(args[2], NULL, NULL)
							);
							break;
						}
					}

					// If target is valid, then we'll focus on it.
					// See also MT_ALTVIEWMAN mobjthinker
					if (newViewMobj->target != NULL && P_MobjWasRemoved(newViewMobj->target) == false)
					{
						newViewMobj->angle = R_PointToAngle2(
							newViewMobj->x,
							newViewMobj->y,
							newViewMobj->target->x,
							newViewMobj->target->y
						);

						newViewMobj->pitch = R_PointToAngle2(
							0,
							newViewMobj->z,
							R_PointToDist2(
								newViewMobj->x, newViewMobj->y,
								newViewMobj->target->x, newViewMobj->target->y
							),
							newViewMobj->target->z + (newViewMobj->target->height >> 1)
						);
					}
				}
				else
				{
					P_SetTarget(&newViewMobj->target, NULL);
				}

				if (args[3] > 0 && args[3] <= NUMTUBEWAYPOINTSEQUENCES)
				{
					P_SetTarget(
						&newViewMobj->tracer,
						P_GetFirstTubeWaypoint(args[3] - 1)
					);
					newViewMobj->movecount = 0; // time
					newViewMobj->movedir = newViewMobj->angle; // start angle
					newViewMobj->lastlook = newViewMobj->pitch; // start pitch
					newViewMobj->extravalue1 = newViewMobj->x; // start x
					newViewMobj->extravalue2 = newViewMobj->y; // start y
					newViewMobj->cusval = newViewMobj->z; // start z

					if (args[4] > 0)
					{
						newViewMobj->movefactor = FRACUNIT / args[4];
					}
					else
					{
						newViewMobj->movefactor = FRACUNIT / TICRATE; // default speed
					}
				}
				else
				{
					P_SetTarget(&newViewMobj->tracer, NULL);
				}
			}
			break;

		case 423: // Change Sky
			if ((mo && mo->player && P_IsPartyPlayer(mo->player)) || args[1])
				P_SetupLevelSky(stringargs[0], args[1]);
			break;

		case 424: // Change Weather
		{
			preciptype_t new_precip = PRECIP_NONE;
			if (udmf_version < 2)
			{
				new_precip = args[0];
			}
			else
			{
				new_precip = stringargs[0] ? get_number(stringargs[0]) : PRECIP_NONE;
			}

			if (args[1])
			{
				globalweather = new_precip;
				P_SwitchWeather(globalweather);
			}
			else
			{
				if (mo && mo->player && P_IsPartyPlayer(mo->player))
				{
					P_SwitchWeather(new_precip);
				}
			}
			break;
		}

		case 425: // Calls P_SetMobjState on calling mobj
			{
				mobj_t *targetThing = NULL;
				statenum_t state = stringargs[0] ? get_number(stringargs[0]) : S_NULL;

				if (state < 0 || state >= NUMSTATES)
				{
					return false;
				}

				mobj_t *next = P_FindMobjFromTID(args[1], targetThing, mo);

				while ((targetThing = next) != NULL)
				{
					// First in case of deletion. (Can't check for state == S_NULL because of A_ calls, etc)
					next = P_FindMobjFromTID(args[1], targetThing, mo);

					if (targetThing->player != NULL)
					{
						continue;
					}

					P_SetMobjState(targetThing, state);
				}
			}
			break;

		case 426: // Moves the mobj to its sector's soundorg and on the floor, and stops it
			if (!mo)
				return false;

			if (args[0])
			{
				P_UnsetThingPosition(mo);
				mo->x = mo->subsector->sector->soundorg.x;
				mo->y = mo->subsector->sector->soundorg.y;
				mo->z = mo->floorz;
				P_SetThingPosition(mo);
			}

			mo->momx = mo->momy = mo->momz = 1;
			mo->pmomz = 0;

			if (mo->player)
			{
				mo->player->rmomx = mo->player->rmomy = 1;
				mo->player->cmomx = mo->player->cmomy = 0;
				P_ResetPlayer(mo->player);
				P_SetPlayerMobjState(mo, S_KART_STILL);
			}
			break;

		case 427: // Awards points if the mobj is a player
			if (mo && mo->player)
				P_AddPlayerScore(mo->player, args[0]);
			break;

		case 428: // Start floating platform movement
			EV_DoContinuousElevator(
				args[0],
				args[1] << (FRACBITS - 2),
				args[2],
				args[3],
				(args[4] == 0)
			);
			break;

		case 429: // Crush planes once
			if (args[1] == TMP_FLOOR)
			{
				EV_DoCrushFloorOnce(
					args[0],
					args[2] << (FRACBITS - 2)
				);
			}
			else if (args[1] == TMP_CEILING)
			{
				EV_DoCrushCeilingOnce(
					args[0],
					args[2] << (FRACBITS - 2)
				);
			}
			else
			{
				EV_DoCrushBothOnce(
					args[0],
					args[2] << (FRACBITS - 2)
				);
			}
			break;

		case 433: // Flip/flop gravity. Works on pushables, too!
			if (!mo || !P_MobjCanChangeFlip(mo))
				return false;

			if (args[0])
				mo->flags2 &= ~MF2_OBJECTFLIP;
			else
				mo->flags2 |= MF2_OBJECTFLIP;
			break;

		case 435: // Change scroller direction
			{
				scroll_t *scroller;
				thinker_t *th;

				fixed_t speed;
				angle_t angle;
				fixed_t dx;
				fixed_t dy;

				speed = args[1] << FRACBITS;
				angle = FixedAngle(args[2] << FRACBITS) >> ANGLETOFINESHIFT;
				dx = FixedMul(FINECOSINE(angle), speed) >> SCROLL_SHIFT;
				dy = FixedMul(  FINESINE(angle), speed) >> SCROLL_SHIFT;

				for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)T_Scroll)
						continue;

					scroller = (scroll_t *)th;

					const taglist_t* taglist = (scroller->type == sc_side)
						? &sides[scroller->affectee].line->tags
						: &sectors[scroller->affectee].tags;

					if (!Tag_Find(taglist, args[0]))
						continue;

					switch (scroller->type)
					{
						case sc_carry:
						case sc_carry_ceiling:
						{
							scroller->dx = FixedMul(-dx, CARRYFACTOR);
							scroller->dy = FixedMul(dy, CARRYFACTOR);
							break;
						}
						default:
						{
							scroller->dx = dx;
							scroller->dy = dy;
							break;
						}
					}
				}
			}
			break;

		case 436: // Shatter block remotely
			{
				INT16 sectag = (INT16)(args[0]);
				INT16 foftag = (INT16)(args[1]);
				sector_t *sec; // Sector that the FOF is visible in
				ffloor_t *rover; // FOF that we are going to crumble
				boolean foundrover = false; // for debug, "Can't find a FOF" message

				TAG_ITER_SECTORS(sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 436: Target sector #%d has no FOFs.\n", secnum);
						return false;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							EV_CrumbleChain(sec, rover);
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 436: Can't find a FOF control sector with tag %d\n", foftag);
						return false;
					}
				}
			}
			break;

		case 437: // Toggle Player Controls
			if (mo && mo->player)
			{
				mo->player->nocontrol = ((args[0] == 0) ? UINT16_MAX : 0);
			}
			break;

		case 438: // Set player scale
			if (mo)
			{
				mo->destscale = FixedMul(FixedDiv(args[0]<<FRACBITS, 100<<FRACBITS), mapobjectscale);
				if (mo->destscale < mapobjectscale/100)
					mo->destscale = mapobjectscale/100;
			}
			break;

		case 439: // Set texture
			{
				line_t *copyLine = NULL;
				size_t linenum;
				side_t *set, *this;
				boolean always;

				if (args[0] == 0)
				{
					if (line == NULL)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 439 Executor: No activating line to copy textures from!\n");
						return false;
					}
					copyLine = line;
				}
				else
				{
					INT32 origline = Tag_Iterate_Lines(args[0], 0);
					if (origline == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 439 Executor: No tagged line to copy textures from (tag %d)!\n", args[0]);
						return false;
					}
					copyLine = &lines[origline];
				}

				set = &sides[copyLine->sidenum[0]];
				always = !(args[3]); // If args[3] is set: Only change mid texture if mid texture already exists on tagged lines, etc.

				for (linenum = 0; linenum < numlines; linenum++)
				{
					if (lines[linenum].special == 439)
						continue; // Don't override other set texture lines!

					if (!Tag_Find(&lines[linenum].tags, args[1]))
						continue; // Find tagged lines

					// Front side
					if (args[2] != TMSD_BACK)
					{
						this = &sides[lines[linenum].sidenum[0]];
						if (always || this->toptexture) this->toptexture = set->toptexture;
						if (always || this->midtexture) this->midtexture = set->midtexture;
						if (always || this->bottomtexture) this->bottomtexture = set->bottomtexture;
					}

					// Back side
					if (args[2] != TMSD_FRONT && lines[linenum].sidenum[1] != 0xffff)
					{
						this = &sides[lines[linenum].sidenum[1]];
						if (always || this->toptexture) this->toptexture = set->toptexture;
						if (always || this->midtexture) this->midtexture = set->midtexture;
						if (always || this->bottomtexture) this->bottomtexture = set->bottomtexture;
					}
				}
			}
			break;

		case 441: // Trigger unlockable
			{
				INT32 trigid = args[0];

				if (trigid < 0 || trigid > 31) // limited by 32 bit variable
					CONS_Debug(DBG_GAMELOGIC, "Unlockable trigger: bad trigger ID %d\n", trigid);
				else if (mo && mo->player)
				{
					UINT32 flag = 1 << trigid;

					if (mo->player->roundconditions.unlocktriggers & flag)
					{
						// Execute one time only
						break;
					}

					mo->player->roundconditions.unlocktriggers |= flag;
					mo->player->roundconditions.checkthisframe = true;
				}
			}
			break;

		case 442: // Calls P_SetMobjState on mobjs of a given type in the tagged sectors
		{
			const mobjtype_t type = stringargs[0] ? get_number(stringargs[0]) : MT_NULL;
			statenum_t state = NUMSTATES;
			mobj_t *thing;

			if (type < 0 || type >= NUMMOBJTYPES)
				break;

			if (!args[3])
			{
				state = stringargs[1] ? get_number(stringargs[1]) : S_NULL;

				if (state < 0 || state >= NUMSTATES)
					break;
			}

			TAG_ITER_SECTORS(args[2], secnum)
			{
				boolean tryagain;
				do {
					tryagain = false;
					for (thing = sectors[secnum].thinglist; thing; thing = thing->snext)
					{
						if (thing->type != type)
							continue;

						if (!P_SetMobjState(thing, args[3] ? thing->state->nextstate : state))
						{ // mobj was removed
							tryagain = true; // snext is corrupt, we'll have to start over.
							break;
						}
					}
				} while (tryagain);
			}
			break;
		}

		case 443: // Calls a named Lua function
			if (stringargs[0])
			{
				LUA_HookSpecialExecute(activator, args, stringargs);
			}
			else
			{
				CONS_Alert(CONS_WARNING, "Special 443 is missing the hook name of the Lua function to call! (This should be given in stringarg0)\n");
			}
			break;

		case 444: // Earthquake camera
		{
			tic_t q_time = args[0];
			fixed_t q_intensity = args[1] * mapobjectscale;
			fixed_t q_radius = args[2] * mapobjectscale;
			// TODO: epicenter

			// reasonable defaults.
			if (q_intensity <= 0)
			{
				q_intensity = 8 * mapobjectscale;
			}

			if (q_radius <= 0)
			{
				q_radius = 512 * mapobjectscale;
			}

			P_StartQuake(q_time, q_intensity, q_radius, NULL);
			break;
		}

		case 445: // Force block disappear remotely (reappear if args[2] is set)
			{
				INT16 sectag = (INT16)(args[0]);
				INT16 foftag = (INT16)(args[1]);
				sector_t *sec; // Sector that the FOF is visible (or not visible) in
				ffloor_t *rover; // FOF to vanish/un-vanish
				boolean foundrover = false; // for debug, "Can't find a FOF" message
				ffloortype_e oldflags; // store FOF's old flags

				TAG_ITER_SECTORS(sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 445 Executor: Target sector #%d has no FOFs.\n", secnum);
						return false;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							oldflags = rover->fofflags;

							// Abracadabra!
							if (args[2])
								rover->fofflags |= FOF_EXISTS;
							else
								rover->fofflags &= ~FOF_EXISTS;

							// if flags changed, reset sector's light list
							if (rover->fofflags != oldflags)
							{
								sec->moved = true;
								P_RecalcPrecipInSector(sec);
							}
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 445 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return false;
					}
				}
			}
			break;

		case 446: // Make block fall remotely (acts like FOF_CRUMBLE)
			{
				INT16 sectag = (INT16)(args[0]);
				INT16 foftag = (INT16)(args[1]);
				sector_t *sec; // Sector that the FOF is visible in
				ffloor_t *rover; // FOF that we are going to make fall down
				boolean foundrover = false; // for debug, "Can't find a FOF" message
				player_t *player = NULL; // player that caused FOF to fall
				boolean respawn = true; // should the fallen FOF respawn?

				if (mo) // NULL check
					player = mo->player;

				if (args[2] & TMFR_NORETURN) // don't respawn!
					respawn = false;

				TAG_ITER_SECTORS(sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 446 Executor: Target sector #%d has no FOFs.\n", secnum);
						return false;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							if (args[2] & TMFR_CHECKFLAG) // FOF flags determine respawn ability instead?
								respawn = !(rover->fofflags & FOF_NORETURN) ^ !!(args[2] & TMFR_NORETURN); // TMFR_NORETURN inverts

							EV_StartCrumble(rover->master->frontsector, rover, (rover->fofflags & FOF_FLOATBOB), player, rover->alpha, respawn);
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 446 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return false;
					}
				}
			}
			break;

		case 447: // Change colormap of tagged sectors!
			// Basically this special applies a colormap to the tagged sectors, just like 606 (the colormap linedef)
			// Except it is activated by linedef executor, not level load
			// This could even override existing colormaps I believe
			// -- Monster Iestyn 14/06/18
		{
			extracolormap_t *source;

			if (backwardsCompat)
			{
				source = sides[line->sidenum[0]].colormap_data;
			}
			else
			{
				if (!args[1])
				{
					if (line == NULL)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 447 Executor: Can't find frontsector with source colormap!\n");
						return false;
					}
					source = line->frontsector->extra_colormap;
				}
				else
				{
					INT32 sourcesec = Tag_Iterate_Sectors(args[1], 0);
					if (sourcesec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 447 Executor: Can't find sector with source colormap (tag %d)!\n", args[1]);
						return false;
					}
					source = sectors[sourcesec].extra_colormap;
				}
			}

			TAG_ITER_SECTORS(args[0], secnum)
			{
				if (sectors[secnum].colormap_protected)
					continue;

				P_ResetColormapFader(&sectors[secnum]);

				if (args[2] & TMCF_RELATIVE)
				{
					extracolormap_t *target = (backwardsCompat && (line->flags & ML_TFERLINE) && line->sidenum[1] != 0xFFFF) ?
						sides[line->sidenum[1]].colormap_data : sectors[secnum].extra_colormap; // use back colormap instead of target sector

					extracolormap_t *exc = R_AddColormaps(
						target,
						source,
						args[2] & TMCF_SUBLIGHTR,
						args[2] & TMCF_SUBLIGHTG,
						args[2] & TMCF_SUBLIGHTB,
						args[2] & TMCF_SUBLIGHTA,
						args[2] & TMCF_SUBFADER,
						args[2] & TMCF_SUBFADEG,
						args[2] & TMCF_SUBFADEB,
						args[2] & TMCF_SUBFADEA,
						args[2] & TMCF_SUBFADESTART,
						args[2] & TMCF_SUBFADEEND,
						args[2] & TMCF_IGNOREFLAGS,
						false
					);

					if (!(sectors[secnum].extra_colormap = R_GetColormapFromList(exc)))
					{
						exc->colormap = R_CreateLightTable(exc);
						R_AddColormapToList(exc);
						sectors[secnum].extra_colormap = exc;
					}
					else
						Z_Free(exc);
				}
				else
					sectors[secnum].extra_colormap = source;
			}
			break;
		}
		case 448: // Change skybox viewpoint/centerpoint
			{
				skybox_t skybox = {0};

				INT32 viewid = args[0];
				INT32 centerid = args[1];

				mobj_t *set;

				// set viewpoint mobj
				if (args[2] != TMS_CENTERPOINT)
				{
					if (viewid >= 0 && viewid < 16)
						set = skyboxviewpnts[viewid];
					else
						set = NULL;

					skybox.viewpoint = set;
				}

				// set centerpoint mobj
				if (args[2] != TMS_VIEWPOINT)
				{
					if (centerid >= 0 && centerid < 16)
						set = skyboxcenterpnts[centerid];
					else
						set = NULL;

					skybox.centerpoint = set;
				}

				if (args[3]) // Applies to all players
				{
					INT32 i;

					for (i = 0; i < MAXPLAYERS; ++i)
					{
						if (playeringame[i])
							P_SwitchSkybox(args[2], &players[i], &skybox);
					}
				}
				else if (mo && mo->player)
				{
					P_SwitchSkybox(args[2], mo->player, &skybox);
				}

				CONS_Debug(DBG_GAMELOGIC, "Special type 448 Executor: viewid = %d, centerid = %d, viewpoint? = %s, centerpoint? = %s\n",
						viewid,
						centerid,
						((args[2] == TMS_CENTERPOINT) ? "no" : "yes"),
						((args[2] == TMS_VIEWPOINT) ? "no" : "yes"));
			}
			break;

		case 449: // Enable bosses with parameter
		{
			INT32 bossid = args[0];
			if (bossid & ~15) // if any bits other than first 16 are set
			{
				CONS_Alert(CONS_WARNING,
					M_GetText("Boss enable special has an invalid boss ID (%d).\nConsider changing it or removing it entirely.\n"),
					bossid);
				break;
			}
			if (args[1])
			{
				bossdisabled |= (1<<bossid);
				CONS_Debug(DBG_GAMELOGIC, "Special type 449 Executor: bossid disabled = %d", bossid);
			}
			else
			{
				bossdisabled &= ~(1<<bossid);
				CONS_Debug(DBG_GAMELOGIC, "Special type 449 Executor: bossid enabled = %d", bossid);
			}
			break;
		}

		case 450: // Execute Linedef Executor - for recursion
			P_LinedefExecute(args[0], mo, NULL);
			break;

		case 451: // Execute Random Linedef Executor
		{
			INT32 rvalue1 = args[0];
			INT32 rvalue2 = args[1];
			INT32 result;

			if (rvalue1 <= rvalue2)
				result = P_RandomRange(PR_EXECUTOR, rvalue1, rvalue2);
			else
				result = P_RandomRange(PR_EXECUTOR, rvalue2, rvalue1);

			P_LinedefExecute((INT16)result, mo, NULL);
			break;
		}

		case 452: // Set FOF alpha
		{
			INT16 destvalue = (INT16)(args[2]);
			INT16 sectag = (INT16)(args[0]);
			INT16 foftag = (INT16)(args[1]);
			sector_t *sec; // Sector that the FOF is visible in
			ffloor_t *rover; // FOF that we are going to operate
			boolean foundrover = false; // for debug, "Can't find a FOF" message

			TAG_ITER_SECTORS(sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 452 Executor: Target sector #%d has no FOFs.\n", secnum);
					return false;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						// If fading an invisible FOF whose render flags we did not yet set,
						// initialize its alpha to 0 for relative alpha calculation
						if (!(args[3] & TMST_DONTDOTRANSLUCENT) &&      // do translucent
							(rover->spawnflags & FOF_NOSHADE) && // do not include light blocks, which don't set FOF_NOSHADE
							!(rover->spawnflags & FOF_RENDERSIDES) &&
							!(rover->spawnflags & FOF_RENDERPLANES) &&
							!(rover->fofflags & FOF_RENDERALL))
							rover->alpha = 0;

						P_RemoveFakeFloorFader(rover);
						P_FadeFakeFloor(rover,
							rover->alpha,
							max(0, min(255, (args[3] & TMST_RELATIVE) ? rover->alpha + destvalue : destvalue)),
							0,                                         // set alpha immediately
							false, NULL,                               // tic-based logic
							false,                                     // do not handle FOF_EXISTS
							!(args[3] & TMST_DONTDOTRANSLUCENT), // handle FOF_TRANSLUCENT
							false,                                     // do not handle lighting
							false,                                     // do not handle colormap
							false,                                     // do not handle collision
							false,                                     // do not do ghost fade (no collision during fade)
							true);                                     // use exact alpha values (for opengl)
					}
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 452 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return false;
				}
			}
			break;
		}

		case 453: // Fade FOF
		{
			INT16 destvalue = (INT16)(args[2]);
			INT16 speed = (INT16)(args[3]);
			INT16 sectag = (INT16)(args[0]);
			INT16 foftag = (INT16)(args[1]);
			sector_t *sec; // Sector that the FOF is visible in
			ffloor_t *rover; // FOF that we are going to operate
			boolean foundrover = false; // for debug, "Can't find a FOF" message
			size_t j = 0; // sec->ffloors is saved as ffloor #0, ss->ffloors->next is #1, etc

			TAG_ITER_SECTORS(sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 453 Executor: Target sector #%d has no FOFs.\n", secnum);
					return false;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						// Prevent continuous execs from interfering on an existing fade
						if (!(args[4] & TMFT_OVERRIDE)
							&& rover->fadingdata)
							//&& ((fade_t*)rover->fadingdata)->timer > (ticbased ? 2 : speed*2))
						{
							CONS_Debug(DBG_GAMELOGIC, "Special type 453 Executor: Fade FOF thinker already exists, timer: %d\n", ((fade_t*)rover->fadingdata)->timer);
							continue;
						}

						if (speed > 0)
							P_AddFakeFloorFader(rover, secnum, j,
								destvalue,
								speed,
								(args[4] & TMFT_TICBASED),           // tic-based logic
								(args[4] & TMFT_RELATIVE),           // Relative destvalue
								!(args[4] & TMFT_DONTDOEXISTS),      // do not handle FOF_EXISTS
								!(args[4] & TMFT_DONTDOTRANSLUCENT), // do not handle FOF_TRANSLUCENT
								!(args[4] & TMFT_DONTDOLIGHTING),    // do not handle lighting
								!(args[4] & TMFT_DONTDOCOLORMAP),    // do not handle colormap
								!(args[4] & TMFT_IGNORECOLLISION),   // do not handle collision
								(args[4] & TMFT_GHOSTFADE),          // do ghost fade (no collision during fade)
								(args[4] & TMFT_USEEXACTALPHA));     // use exact alpha values (for opengl)
						else
						{
							// If fading an invisible FOF whose render flags we did not yet set,
							// initialize its alpha to 0 for relative alpha calculation
							if (!(args[4] & TMFT_DONTDOTRANSLUCENT) &&      // do translucent
								(rover->spawnflags & FOF_NOSHADE) && // do not include light blocks, which don't set FOF_NOSHADE
								!(rover->spawnflags & FOF_RENDERSIDES) &&
								!(rover->spawnflags & FOF_RENDERPLANES) &&
								!(rover->fofflags & FOF_RENDERALL))
								rover->alpha = 0;

							P_RemoveFakeFloorFader(rover);
							P_FadeFakeFloor(rover,
								rover->alpha,
								max(0, min(255, (args[4] & TMFT_RELATIVE) ? rover->alpha + destvalue : destvalue)),
								0,                                         // set alpha immediately
								false, NULL,                               // tic-based logic
								!(args[4] & TMFT_DONTDOEXISTS),      // do not handle FOF_EXISTS
								!(args[4] & TMFT_DONTDOTRANSLUCENT), // do not handle FOF_TRANSLUCENT
								!(args[4] & TMFT_DONTDOLIGHTING),    // do not handle lighting
								!(args[4] & TMFT_DONTDOCOLORMAP),    // do not handle colormap
								!(args[4] & TMFT_IGNORECOLLISION),   // do not handle collision
								(args[4] & TMFT_GHOSTFADE),          // do ghost fade (no collision during fade)
								(args[4] & TMFT_USEEXACTALPHA));     // use exact alpha values (for opengl)
						}
					}
					j++;
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 453 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return false;
				}
			}
			break;
		}

		case 454: // Stop fading FOF
		{
			INT16 sectag = (INT16)(args[0]);
			INT16 foftag = (INT16)(args[1]);
			sector_t *sec; // Sector that the FOF is visible in
			ffloor_t *rover; // FOF that we are going to operate
			boolean foundrover = false; // for debug, "Can't find a FOF" message

			TAG_ITER_SECTORS(sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 454 Executor: Target sector #%d has no FOFs.\n", secnum);
					return false;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						P_ResetFakeFloorFader(rover, NULL,
							!(args[2])); // do not finalize collision flags
					}
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 454 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return false;
				}
			}
			break;
		}

		case 455: // Fade colormap
		{
			extracolormap_t *dest;

			if (backwardsCompat)
			{
				dest = sides[line->sidenum[0]].colormap_data;
			}
			else
			{
				if (!args[1])
				{
					if (line == NULL)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 455 Executor: Can't find frontsector with destination colormap!\n");
						return false;
					}
					dest = line->frontsector->extra_colormap;
				}
				else
				{
					INT32 destsec = Tag_Iterate_Sectors(args[1], 0);
					if (destsec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Special type 455 Executor: Can't find sector with destination colormap (tag %d)!\n", args[1]);
						return false;
					}
					dest = sectors[destsec].extra_colormap;
				}
			}

			TAG_ITER_SECTORS(args[0], secnum)
			{
				extracolormap_t *source_exc, *dest_exc, *exc;

				if (sectors[secnum].colormap_protected)
					continue;

				// Don't interrupt ongoing fade
				if (!(args[3] & TMCF_OVERRIDE)
					&& sectors[secnum].fadecolormapdata)
					//&& ((fadecolormap_t*)sectors[secnum].fadecolormapdata)->timer > (ticbased ? 2 : speed*2))
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 455 Executor: Fade color thinker already exists, timer: %d\n", ((fadecolormap_t*)sectors[secnum].fadecolormapdata)->timer);
					continue;
				}

				if (backwardsCompat && (line->flags & ML_TFERLINE)) // use back colormap instead of target sector
				{
					sectors[secnum].extra_colormap = (line->sidenum[1] != 0xFFFF) ?
						sides[line->sidenum[1]].colormap_data : NULL;
				}
				else
				{
					sectors[secnum].extra_colormap = NULL;
				}

				exc = sectors[secnum].extra_colormap;

				if (!(args[3] & TMCF_FROMBLACK) // Override fade from default rgba
					&& !R_CheckDefaultColormap(dest, true, false, false)
					&& R_CheckDefaultColormap(exc, true, false, false))
				{
					exc = R_CopyColormap(exc, false);
					exc->rgba = R_GetRgbaRGB(dest->rgba) + R_PutRgbaA(R_GetRgbaA(exc->rgba));
					//exc->fadergba = R_GetRgbaRGB(dest->rgba) + R_PutRgbaA(R_GetRgbaA(exc->fadergba));

					if (!(source_exc = R_GetColormapFromList(exc)))
					{
						exc->colormap = R_CreateLightTable(exc);
						R_AddColormapToList(exc);
						source_exc = exc;
					}
					else
						Z_Free(exc);

					sectors[secnum].extra_colormap = source_exc;
				}
				else
					source_exc = exc ? exc : R_GetDefaultColormap();

				if (args[3] & TMCF_RELATIVE)
				{
					exc = R_AddColormaps(
						source_exc,
						dest,
						args[3] & TMCF_SUBLIGHTR,
						args[3] & TMCF_SUBLIGHTG,
						args[3] & TMCF_SUBLIGHTB,
						args[3] & TMCF_SUBLIGHTA,
						args[3] & TMCF_SUBFADER,
						args[3] & TMCF_SUBFADEG,
						args[3] & TMCF_SUBFADEB,
						args[3] & TMCF_SUBFADEA,
						args[3] & TMCF_SUBFADESTART,
						args[3] & TMCF_SUBFADEEND,
						args[3] & TMCF_IGNOREFLAGS,
						false);
				}
				else
					exc = R_CopyColormap(dest, false);

				if (!(dest_exc = R_GetColormapFromList(exc)))
				{
					exc->colormap = R_CreateLightTable(exc);
					R_AddColormapToList(exc);
					dest_exc = exc;
				}
				else
					Z_Free(exc);

				Add_ColormapFader(&sectors[secnum], source_exc, dest_exc, true, // tic-based timing
					args[2]);
			}
			break;
		}
		case 456: // Stop fade colormap
			TAG_ITER_SECTORS(args[0], secnum)
				P_ResetColormapFader(&sectors[secnum]);
			break;

		case 457: // Track mobj angle to point
			if (mo)
			{
				INT32 failureangle = FixedAngle((min(max(abs(args[1]), 0), 360))*FRACUNIT);
				INT32 failuredelay = abs(args[2]);
				boolean persist = !!(args[3]);
				mobj_t *anchormo;

				anchormo = P_FindObjectTypeFromTag(MT_ANGLEMAN, args[0]);
				if (!anchormo)
					return false;

				mo->eflags |= MFE_TRACERANGLE;
				P_SetTarget(&mo->tracer, anchormo);
				mo->lastlook = persist; // don't disable behavior after first failure
				mo->extravalue1 = failureangle; // angle to exceed for failure state
				mo->cusval = mo->cvmem = failuredelay; // cusval = tics to allow failure before line trigger; cvmem = decrement timer
			}
			break;

		case 458: // Stop tracking mobj angle to point
			if (mo && (mo->eflags & MFE_TRACERANGLE))
			{
				mo->eflags &= ~MFE_TRACERANGLE;
				P_SetTarget(&mo->tracer, NULL);
				mo->lastlook = mo->cvmem = mo->cusval = mo->extravalue1 = 0;
			}
			break;

		case 460: // Award rings
			{
				if (gametyperules & GTR_SPHERES)
					return false;

				INT16 rings = args[0];
				INT32 delay = args[1];
				if (
					mo && mo->player // Player
					&& rings != 0 // Any effect
					&& (delay <= 0 || !(leveltime % delay)) // Timing
				)
				{
					if (rings > 0)
					{
						// Don't award rings while SPB is targetting you
						if (mo->player->pflags & PF_RINGLOCK)
							return false;

						// args[2]: don't cap rings to 20
						K_AwardPlayerRings(mo->player, rings, args[2]);
					}
					else
					{
						// args[2]: cap rings to -20 instead of 0
						SINT8 baseline = (args[2] ? -20 : 0);

						// Don't push you below baseline
						if (mo->player->rings <= baseline)
							return false;

						rings = -(rings);

						if (rings > (mo->player->rings - baseline))
							rings = (mo->player->rings - baseline);

						mo->player->rings -= rings;
						S_StartSound(mo, sfx_antiri);
					}
				}
			}
			break;

		case 461: // Spawns an object on the map based on texture offsets
			{
				const mobjtype_t type = stringargs[0] ? get_number(stringargs[0]) : MT_NULL;
				mobj_t *mobj;

				fixed_t x, y, z;

				if (args[5]) // If args[5] is set, spawn randomly within a range
				{
					x = P_RandomRange(PR_UNDEFINED, args[1], args[6])<<FRACBITS;
					y = P_RandomRange(PR_UNDEFINED, args[2], args[7])<<FRACBITS;
					z = P_RandomRange(PR_UNDEFINED, args[3], args[8])<<FRACBITS;
				}
				else
				{
					x = args[1] << FRACBITS;
					y = args[2] << FRACBITS;
					z = args[3] << FRACBITS;
				}

				mobj = P_SpawnMobj(x, y, z, type);
				if (mobj)
				{
					mobj->angle = FixedAngle(args[4] << FRACBITS);
					CONS_Debug(DBG_GAMELOGIC, "Special Type %d - Spawn Object: %d spawned at (%d, %d, %d)\n", special, mobj->type, mobj->x>>FRACBITS, mobj->y>>FRACBITS, mobj->z>>FRACBITS); //TODO: Convert mobj->type to a string somehow.
				}
				else
					CONS_Alert(CONS_ERROR,"Special Type %d - Spawn Object: Object did not spawn!\n", special);
			}
			break;

		case 462: // Stop clock (and end level in record attack)
			stoppedclock = true;
			CONS_Debug(DBG_GAMELOGIC, "Clock stopped!\n");
			if (modeattacking)
			{
				P_DoAllPlayersExit(0, false);
			}
			break;

		case 463: // Dye object
			{
				if (mo)
				{
					INT32 color = stringargs[0] ? get_number(stringargs[0]) : SKINCOLOR_NONE;

					if (color < 0 || color >= numskincolors)
						return false;

					var1 = 0;
					var2 = color;
					A_Dye(mo);
				}
			}
			break;

		case 464: // Trigger Egg Capsule
			{
				INT32 mtnum;
				mobj_t *mo2;

				// Find the center of the Eggtrap and release all the pretty animals!
				// The chimps are my friends.. heeheeheheehehee..... - LouisJM
				TAG_ITER_THINGS(args[0], mtnum)
				{
					mo2 = mapthings[mtnum].mobj;

					if (!mo2)
						continue;

					if (mo2->type != MT_EGGTRAP)
						continue;

					if (mo2->thinker.function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					P_KillMobj(mo2, NULL, mo, DMG_NORMAL);
				}

				if (!(args[1]))
				{
					P_DoAllPlayersExit(0, false);
				}
			}
			break;

		case 466: // Set level failure state
			{
				if (args[0])
				{
					stagefailed = false;
					CONS_Debug(DBG_GAMELOGIC, "Stage can be completed successfully!\n");
				}
				else
				{
					stagefailed = true;
					CONS_Debug(DBG_GAMELOGIC, "Stage will end in failure...\n");
				}
			}
			break;

		case 467: // Set light level
			TAG_ITER_SECTORS(args[0], secnum)
			{
				if (sectors[secnum].lightingdata)
				{
					// Stop any lighting effects going on in the sector
					P_RemoveThinker(&((thinkerdata_t *)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}

				if (args[2] == TML_FLOOR)
				{
					if (args[3])
						sectors[secnum].floorlightlevel += args[1];
					else
						sectors[secnum].floorlightlevel = args[1];
				}
				else if (args[2] == TML_CEILING)
				{
					if (args[3])
						sectors[secnum].ceilinglightlevel += args[1];
					else
						sectors[secnum].ceilinglightlevel = args[1];
				}
				else
				{
					if (args[3])
						sectors[secnum].lightlevel += args[1];
					else
						sectors[secnum].lightlevel = args[1];
					sectors[secnum].lightlevel = max(0, min(255, sectors[secnum].lightlevel));
				}
			}
			break;

		case 468: // Change linedef executor argument
		{
			INT32 linenum;

			if (backwardsCompat)
				break;

			if (args[1] < 0 || args[1] >= NUM_SCRIPT_ARGS)
			{
				CONS_Debug(DBG_GAMELOGIC, "Linedef type 468: Invalid linedef arg %d\n", args[1]);
				break;
			}

			TAG_ITER_LINES(args[0], linenum)
			{
				if (args[3])
					lines[linenum].args[args[1]] += args[2];
				else
					lines[linenum].args[args[1]] = args[2];
			}
		}
		break;

		case 469: // Change sector gravity
		{
			fixed_t gravityvalue;

			if (backwardsCompat)
				break;

			if (!stringargs[0])
				break;

			gravityvalue = FloatToFixed(atof(stringargs[0]));

			TAG_ITER_SECTORS(args[1], secnum)
			{
				if (args[2])
					sectors[secnum].gravity = FixedMul(sectors[secnum].gravity, gravityvalue);
				else
					sectors[secnum].gravity = gravityvalue;

				if (args[3] == TMF_ADD)
					sectors[secnum].flags |= MSF_GRAVITYFLIP;
				else if (args[3] == TMF_REMOVE)
					sectors[secnum].flags &= ~MSF_GRAVITYFLIP;
			}
		}
		break;

		case 475: // ACS_Execute
			{
				if (!stringargs[0])
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 475: No script name given\n");
					return false;
				}

				ACS_Execute(stringargs[0], &args[1], NUM_SCRIPT_ARGS - 1, (const char* const*)&stringargs[1], NUM_SCRIPT_STRINGARGS - 1, activator);
			}
			break;
		case 476: // ACS_ExecuteAlways
			{
				if (!stringargs[0])
				{
					CONS_Debug(DBG_GAMELOGIC, "Special type 475: No script name given\n");
					return false;
				}

				ACS_ExecuteAlways(stringargs[0], &args[1], NUM_SCRIPT_ARGS - 1, (const char* const*)&stringargs[1], NUM_SCRIPT_STRINGARGS - 1, activator);
			}
			break;
		case 477: // ACS_Suspend
			if (!stringargs[0])
			{
				CONS_Debug(DBG_GAMELOGIC, "Special type 477: No script name given\n");
				return false;
			}

			ACS_Suspend(stringargs[0]);
			break;
		case 478: // ACS_Terminate
			if (!stringargs[0])
			{
				CONS_Debug(DBG_GAMELOGIC, "Special type 478: No script name given\n");
				return false;
			}

			ACS_Terminate(stringargs[0]);
			break;

		case 480: // Polyobj_DoorSlide
			PolyDoor(args);
			break;
		case 481: // Polyobj_DoorSwing
			PolyDoorSwing(args);
			break;
		case 482: // Polyobj_Move
			PolyMove(args);
			break;
		case 484: // Polyobj_RotateRight
			PolyRotate(args);
			break;
		case 488: // Polyobj_Waypoint
			PolyWaypoint(args);
			break;
		case 489:
			PolySetVisibilityTangibility(args);
			break;
		case 491:
			PolyTranslucency(args);
			break;
		case 492:
			PolyFade(args);
			break;

		// SRB2kart
		case 499: // Enable/Disable Waypoints in Tagged Sectors
			{
				sector_t *sec;
				mobj_t *thing;

				if (waypointcap == NULL)
				{
					// No point in trying at all if no waypoints exist.
					return false;
				}

				TAG_ITER_SECTORS(args[0], secnum)
				{
					sec = sectors + secnum;

					for (thing = sec->thinglist; thing; thing = thing->snext)
					{
						if (thing->type == MT_WAYPOINT)
						{
							if (args[1])
							{
								thing->extravalue1 = 1;
							}
							else
							{
								thing->extravalue1 = 0;
							}
						}
					}
				}
			}
			break;

		case 2001: // Finish Line
		{
			if (mo->player == NULL)
			{
				return false;
			}

			if ((gametyperules & GTR_CIRCUIT) && (mo->player->exiting == 0) && !(mo->player->pflags & PF_HITFINISHLINE))
			{
				if (((args[0] & TMCFF_FLIP) && (side == 0))
					|| (!(args[0] & TMCFF_FLIP) && (side == 1))) // crossed from behind to infront
				{
					K_HandleLapIncrement(mo->player);

					ACS_RunLapScript(mo, line);
				}
				else
				{
					K_HandleLapDecrement(mo->player);
				}

				mo->player->pflags |= PF_HITFINISHLINE;
			}
		}
		break;

		case 2003: // Respawn Line
		{
			if (mo->player == NULL)
			{
				return false;
			}

			/* No Climb: only trigger from front side */
			if
				(
						mo->player->respawn.state == RESPAWNST_NONE &&
						(!(args[0] & TMCRF_FRONTONLY) || side == 0)
				)
			{
				P_DamageMobj(mo, NULL, NULL, 1, DMG_DEATHPIT);
			}
		}
		break;

		case 2004: // Bot Controller.
		{
			K_UpdateBotControllers(args);
			break;
		}

		case 2005: // Dismount Flying object
			// the rideroid is a bit complex so it's the one controlling the player rather than the player controlling it.
			// so it is the object needing to be checked for rather than the player
			if (mo->player && mo->player->rideroid && mo->tracer
			&& !P_MobjWasRemoved(mo->tracer) && mo->tracer->type == MT_RIDEROID)
				Obj_getPlayerOffRideroid(mo->tracer);

			// dlz rockets are simpler and are tied to the player hence why we check for the player here instead.
			if (mo->player && mo->player->dlzrocket)
				Obj_DLZRocketDismount(mo->player);
		break;

		default:
			break;
	}

	return true;
}

static void P_SetupSignObject(mobj_t *sign, mobj_t *pmo, boolean error)
{
	mobj_t *cur = sign, *prev = NULL;

	// Setup the sign itself
	P_SetScale(sign, (sign->destscale = 5 * sign->scale / 2)); // 2.5x
	P_SetTarget(&sign->target, pmo);
	P_SetMobjState(sign, S_SIGN_POLE);

	sign->movefactor = sign->z;
	sign->z += (576*sign->scale) * P_MobjFlip(sign);
	sign->old_z = sign->z; // interp hijack
	sign->movecount = 1;
	sign->extravalue1 = AngleFixed(sign->angle) >> FRACBITS;

	// Setup the overlay pieces
	// Front
	cur->hnext = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_SIGN_PIECE);
	P_SetTarget(&cur->hnext->target, sign);
	P_SetMobjState(cur->hnext, S_SIGN_FACE);
	cur->hnext->extravalue1 = 6;
	cur->hnext->extravalue2 = 0;

	prev = cur;
	cur = cur->hnext;
	cur->hprev = prev;

	// Player icon
	cur->hnext = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_SIGN_PIECE);
	P_SetTarget(&cur->hnext->target, sign);
	cur->hnext->skin = pmo->skin;
	P_SetMobjState(cur->hnext, (error == true) ? S_SIGN_ERROR : S_KART_SIGN);
	cur->hnext->extravalue1 = 7;
	cur->hnext->extravalue2 = 0;

	prev = cur;
	cur = cur->hnext;
	cur->hprev = prev;

	// Back
	cur->hnext = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_SIGN_PIECE);
	P_SetTarget(&cur->hnext->target, sign);
	P_SetMobjState(cur->hnext, S_SIGN_BACK);
	cur->hnext->extravalue1 = 6;
	cur->hnext->extravalue2 = 2;

	prev = cur;
	cur = cur->hnext;
	cur->hprev = prev;

	// Sides
	cur->hnext = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_SIGN_PIECE);
	P_SetTarget(&cur->hnext->target, sign);
	P_SetMobjState(cur->hnext, S_SIGN_SIDE);
	cur->hnext->extravalue1 = 30;
	cur->hnext->extravalue2 = 1;

	prev = cur;
	cur = cur->hnext;
	cur->hprev = prev;

	cur->hnext = P_SpawnMobjFromMobj(sign, 0, 0, 0, MT_SIGN_PIECE);
	P_SetTarget(&cur->hnext->target, sign);
	P_SetMobjState(cur->hnext, S_SIGN_SIDE);
	cur->hnext->extravalue1 = 30;
	cur->hnext->extravalue2 = 3;

	prev = cur;
	cur = cur->hnext;
	cur->hprev = prev;
}

//
// P_SetupSignExit
//
// Finds the exit sign in the current sector and
// sets its target to the player who passed the map.
//
void P_SetupSignExit(player_t *player, boolean tie)
{
	mobj_t *thing;
	msecnode_t *node = player->mo->subsector->sector->touching_thinglist; // things touching this sector
	thinker_t *think;
	INT32 numfound = 0;

	if (player->position != 1
	|| (gametyperules & GTR_SPECIALSTART))
		return;

	angle_t bestAngle = K_MomentumAngle(player->mo) + ANGLE_180;

	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (thing->type != MT_SIGN)
			continue;

		bestAngle = thing->angle;

		if (tie)
		{
			break;
		}

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetupSignObject(thing, player->mo, false);
		++numfound;
	}

	if (numfound)
		return;

	// didn't find any signposts in the exit sector.
	// spin all signposts in the level then.
	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		thing = (mobj_t *)think;
		if (thing->type != MT_SIGN)
			continue;

		bestAngle = thing->angle;

		if (tie)
		{
			break;
		}

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetupSignObject(thing, player->mo, false);
		++numfound;
	}

	if (numfound)
		return;

	// SRB2Kart: FINALLY, add in an alternative if no place is found
	if (player->mo && !P_MobjWasRemoved(player->mo) && !K_EndCameraIsFreezing())
	{
		thing = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->floorz, MT_SIGN);
		thing->angle = bestAngle;
		P_SetupSignObject(thing, player->mo, (tie == false)); // Use :youfuckedup: sign face, except during ties
	}
}

static boolean P_IsMobjTouchingPlane(mobj_t *mo, sector_t *sec, fixed_t floorz, fixed_t ceilingz)
{
	boolean floorallowed = ((sec->flags & MSF_FLIPSPECIAL_FLOOR) && ((sec->flags & MSF_TRIGGERSPECIAL_HEADBUMP) || !(mo->eflags & MFE_VERTICALFLIP)) && (mo->z == floorz));
	boolean ceilingallowed = ((sec->flags & MSF_FLIPSPECIAL_CEILING) && ((sec->flags &  MSF_TRIGGERSPECIAL_HEADBUMP) || (mo->eflags & MFE_VERTICALFLIP)) && (mo->z + mo->height == ceilingz));
	return (floorallowed || ceilingallowed);
}

boolean P_IsMobjTouchingSectorPlane(mobj_t *mo, sector_t *sec)
{
	return P_IsMobjTouchingPlane(mo, sec, P_GetSpecialBottomZ(mo, sec, sec), P_GetSpecialTopZ(mo, sec, sec));
}

boolean P_IsMobjTouching3DFloor(mobj_t *mo, ffloor_t *ffloor, sector_t *sec)
{
	fixed_t topheight = P_GetSpecialTopZ(mo, sectors + ffloor->secnum, sec);
	fixed_t bottomheight = P_GetSpecialBottomZ(mo, sectors + ffloor->secnum, sec);

	if (((ffloor->fofflags & FOF_BLOCKPLAYER) && mo->player)
		|| ((ffloor->fofflags & FOF_BLOCKOTHERS) && !mo->player))
	{
		// Solid 3D floor: Mobj must touch the top or bottom
		return P_IsMobjTouchingPlane(mo, ffloor->master->frontsector, topheight, bottomheight);
	}
	else
	{
		// Water or intangible 3D floor: Mobj must be inside
		return mo->z <= topheight && (mo->z + mo->height) >= bottomheight;
	}
}

boolean P_IsMobjTouchingPolyobj(mobj_t *mo, polyobj_t *po, sector_t *polysec)
{
	if (!(po->flags & POF_TESTHEIGHT)) // Don't do height checking
		return true;

	if (po->flags & POF_SOLID)
	{
		// Solid polyobject: Player must touch the top or bottom
		return P_IsMobjTouchingPlane(mo, polysec, polysec->ceilingheight, polysec->floorheight);
	}
	else
	{
		// Water or intangible polyobject: Player must be inside
		return mo->z <= polysec->ceilingheight && (mo->z + mo->height) >= polysec->floorheight;
	}
}

static sector_t *P_MobjTouching3DFloorSpecial(mobj_t *mo, sector_t *sector, INT32 section, INT32 number)
{
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (GETSECSPECIAL(rover->master->frontsector->special, section) != number)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, sector))
			continue;

		// This FOF has the special we're looking for, but are we allowed to touch it?
		if (sector == mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			return rover->master->frontsector;
	}

	return NULL;
}

static sector_t *P_MobjTouching3DFloorSpecialFlag(mobj_t *mo, sector_t *sector, sectorspecialflags_t flag)
{
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->master->frontsector->specialflags & flag))
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, sector))
			continue;

		// This FOF has the special we're looking for, but are we allowed to touch it?
		if (sector == mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			return rover->master->frontsector;
	}

	return NULL;
}

static sector_t *P_MobjTouchingPolyobjSpecial(mobj_t *mo, INT32 section, INT32 number)
{
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (GETSECSPECIAL(polysec->special, section) != number)
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(mo, po, polysec))
			continue;

		return polysec;
	}

	return NULL;
}

static sector_t *P_MobjTouchingPolyobjSpecialFlag(mobj_t *mo, sectorspecialflags_t flag)
{
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!(polysec->specialflags & flag))
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(mo, po, polysec))
			continue;

		return polysec;
	}

	return NULL;
}

sector_t *P_MobjTouchingSectorSpecial(mobj_t *mo, INT32 section, INT32 number)
{
	msecnode_t *node;
	sector_t *result;

	result = P_MobjTouching3DFloorSpecial(mo, mo->subsector->sector, section, number);
	if (result)
		return result;

	result = P_MobjTouchingPolyobjSpecial(mo, section, number);
	if (result)
		return result;

	if (GETSECSPECIAL(mo->subsector->sector->special, section) == number)
		return mo->subsector->sector;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (node->m_sector == mo->subsector->sector) // Don't duplicate
			continue;

		result = P_MobjTouching3DFloorSpecial(mo, node->m_sector, section, number);
		if (result)
			return result;

		if (!(node->m_sector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		if (GETSECSPECIAL(node->m_sector->special, section) == number)
			return node->m_sector;
	}

	return NULL;
}

sector_t *P_MobjTouchingSectorSpecialFlag(mobj_t *mo, sectorspecialflags_t flag)
{
	msecnode_t *node;
	sector_t *result;

	result = P_MobjTouching3DFloorSpecialFlag(mo, mo->subsector->sector, flag);
	if (result)
		return result;

	result = P_MobjTouchingPolyobjSpecialFlag(mo, flag);
	if (result)
		return result;

	if (mo->subsector->sector->specialflags & flag)
		return mo->subsector->sector;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (node->m_sector == mo->subsector->sector) // Don't duplicate
			continue;

		result = P_MobjTouching3DFloorSpecialFlag(mo, node->m_sector, flag);
		if (result)
			return result;

		if (!(node->m_sector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		if (node->m_sector->specialflags & flag)
			return node->m_sector;
	}

	return NULL;
}

//
// P_PlayerTouchingSectorSpecial
//
// Replaces the old player->specialsector.
// This allows a player to touch more than
// one sector at a time, if necessary.
//
// Returns a pointer to the first sector of
// the particular type that it finds.
// Returns NULL if it doesn't find it.
//
sector_t *P_PlayerTouchingSectorSpecial(player_t *player, INT32 section, INT32 number)
{
	if (!player->mo)
		return NULL;

	return P_MobjTouchingSectorSpecial(player->mo, section, number);
}

sector_t *P_PlayerTouchingSectorSpecialFlag(player_t *player, sectorspecialflags_t flag)
{
	if (!player->mo)
		return NULL;

	return P_MobjTouchingSectorSpecialFlag(player->mo, flag);
}

static sector_t *P_CheckPlayer3DFloorTrigger(player_t *player, sector_t *sector, line_t *sourceline)
{
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->triggertag)
			continue;

		if (rover->master->frontsector->triggerer == TO_MOBJ)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!Tag_Find(&sourceline->tags, rover->master->frontsector->triggertag))
			continue;

		if (!P_IsMobjTouching3DFloor(player->mo, rover, sector))
			continue;

		// This FOF has the special we're looking for, but are we allowed to touch it?
		if (sector == player->mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			return rover->master->frontsector;
	}

	return NULL;
}

static sector_t *P_CheckPlayerPolyobjTrigger(player_t *player, line_t *sourceline)
{
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = player->mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!polysec->triggertag)
			continue;

		if (polysec->triggerer == TO_MOBJ)
			continue;

		if (!Tag_Find(&sourceline->tags, polysec->triggertag))
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, player->mo);
		inside = P_MobjInsidePolyobj(po, player->mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(player->mo, po, polysec))
			continue;

		return polysec;
	}

	return NULL;
}

static boolean P_CheckPlayerSectorTrigger(player_t *player, sector_t *sector, line_t *sourceline)
{
	if (!sector->triggertag)
		return false;

	if (sector->triggerer == TO_MOBJ)
		return false;

	if (!Tag_Find(&sourceline->tags, sector->triggertag))
		return false;

	if (!(sector->flags & MSF_TRIGGERLINE_PLANE))
		return true; // Don't require plane touch

	return P_IsMobjTouchingSectorPlane(player->mo, sector);

}

sector_t *P_FindPlayerTrigger(player_t *player, line_t *sourceline)
{
	sector_t *originalsector;
	sector_t *loopsector;
	msecnode_t *node;
	sector_t *caller;

	if (udmf)
		return NULL;

	if (!player->mo)
		return NULL;

	originalsector = player->mo->subsector->sector;

	caller = P_CheckPlayer3DFloorTrigger(player, originalsector, sourceline); // Handle FOFs first.

	if (caller)
		return caller;

	// Allow sector specials to be applied to polyobjects!
	caller = P_CheckPlayerPolyobjTrigger(player, sourceline);

	if (caller)
		return caller;

	if (P_CheckPlayerSectorTrigger(player, originalsector, sourceline))
		return originalsector;

	// Iterate through touching_sectorlist for SF_TRIGGERSPECIAL_TOUCH
	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		loopsector = node->m_sector;

		if (loopsector == originalsector) // Don't duplicate
			continue;

		// Check 3D floors...
		caller = P_CheckPlayer3DFloorTrigger(player, loopsector, sourceline); // Handle FOFs first.

		if (caller)
			return caller;

		if (!(loopsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		if (P_CheckPlayerSectorTrigger(player, loopsector, sourceline))
			return loopsector;
	}

	return NULL;
}

boolean P_IsPlayerValid(size_t playernum)
{
	if (!playeringame[playernum])
		return false;

	if (!players[playernum].mo)
		return false;

	if (players[playernum].mo->health <= 0)
		return false;

	if (players[playernum].spectator)
		return false;

	return true;
}

boolean P_CanPlayerTrigger(size_t playernum)
{
	return P_IsPlayerValid(playernum);
}

/// \todo check continues for proper splitscreen support?
static boolean P_DoAllPlayersTrigger(mtag_t triggertag)
{
	INT32 i;
	line_t dummyline;

	if (udmf)
		return false;

	dummyline.tags.count = 1;
	dummyline.tags.tags = &triggertag;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!P_CanPlayerTrigger(i))
			continue;
		if (!P_FindPlayerTrigger(&players[i], &dummyline))
			return false;
	}

	return true;
}

static void P_ProcessEggCapsule(player_t *player, sector_t *sector)
{
	thinker_t *th;
	mobj_t *mo2;

	if (sector->ceilingdata || sector->floordata)
		return;

	// Find the center of the Eggtrap and release all the pretty animals!
	// The chimps are my friends.. heeheeheheehehee..... - LouisJM
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;
		mo2 = (mobj_t *)th;
		if (mo2->type != MT_EGGTRAP)
			continue;
		P_KillMobj(mo2, NULL, player->mo, 0);
	}

	// clear the special so you can't push the button twice.
	sector->special = 0;

	// Move the button down
	EV_DoElevator(LE_CAPSULE0, NULL, elevateDown);

	// Open the top FOF
	EV_DoFloor(LE_CAPSULE1, NULL, raiseFloorToNearestFast);
	// Open the bottom FOF
	EV_DoCeiling(LE_CAPSULE2, NULL, lowerToLowestFast);

	P_DoAllPlayersExit(0, false);
}

static void P_ProcessExitSector(player_t *player, mtag_t sectag)
{
	INT32 lineindex;

#if 0
	if (!(gametyperules & GTR_ALLOWEXIT))
		return;
#endif

	// Exit (for FOF exits; others are handled in P_PlayerThink in p_user.c)
	P_DoPlayerExit(player, 0);

	P_SetupSignExit(player, false);

#if 0
	if (!G_CoopGametype())
		return;
#endif

	// Custom exit!
	// important: use sectag on next line instead of player->mo->subsector->tag
	// this part is different from in P_PlayerThink, this is what was causing
	// FOF custom exits not to work.
	lineindex = Tag_FindLineSpecial(2, sectag);

	if (lineindex == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Exit sector missing line special #2.\n");
		return;
	}

	nextmapoverride = (INT16)(udmf ? lines[lineindex].args[0] : lines[lineindex].frontsector->floorheight>>FRACBITS);

	if (lines[lineindex].args[1] & TMEF_SKIPTALLY)
		skipstats = 1;
}

static void P_ProcessZoomTube(player_t *player, mtag_t sectag, boolean end)
{
	INT32 sequence;
	fixed_t speed;
	INT32 lineindex;
	mobj_t *waypoint = NULL;
	angle_t an;

	if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && player->carry == CR_ZOOMTUBE)
		return;

	if (player->respawn.state != RESPAWNST_NONE)
		return;

	// Find line #3 tagged to this sector
	lineindex = Tag_FindLineSpecial(3, sectag);

	if (lineindex == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Zoom tube missing line special #3.\n");
		return;
	}

	// Grab speed and sequence values
	speed = abs(lines[lineindex].args[0])<<(FRACBITS-3);
	if (end)
		speed *= -1;
	sequence = abs(lines[lineindex].args[1]);

	if (speed == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: Waypoint sequence %d at zero speed.\n", sequence);
		return;
	}

	waypoint = end ? P_GetLastTubeWaypoint(sequence) : P_GetFirstTubeWaypoint(sequence);

	if (!waypoint)
	{
		CONS_Debug(DBG_GAMELOGIC, "ERROR: %s WAYPOINT IN SEQUENCE %d NOT FOUND.\n", end ? "LAST" : "FIRST", sequence);
		return;
	}

	CONS_Debug(DBG_GAMELOGIC, "Waypoint %d found in sequence %d - speed = %d\n", waypoint->health, sequence, speed);

	an = R_PointToAngle2(player->mo->x, player->mo->y, waypoint->x, waypoint->y) - player->mo->angle;

	if (an > ANGLE_90 && an < ANGLE_270 && !(lines[lineindex].args[2]))
		return; // behind back

	P_SetTarget(&player->mo->tracer, waypoint);
	player->carry = CR_ZOOMTUBE;
	player->speed = speed;

	if (player->mo->state-states != S_KART_SPINOUT)
	{
		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
		S_StartSound(player->mo, sfx_spin);
	}
}

static boolean P_SectorHasSpecial(sector_t *sec)
{
	if (sec->specialflags)
		return true;

	if (sec->damagetype != SD_NONE)
		return true;

	if (!udmf && sec->triggertag)
		return true;

	if (sec->special)
		return true;

	return false;
}

static void P_EvaluateSpecialFlags(player_t *player, sector_t *sector, sector_t *roversector, boolean isTouching)
{
	mtag_t sectag = Tag_FGet(&sector->tags);

	(void)roversector;
	(void)isTouching;

	if (sector->specialflags & SSF_WINDCURRENT)
		player->onconveyor = 2;
	if (sector->specialflags & SSF_CONVEYOR)
		player->onconveyor = 4;
	if (sector->specialflags & SSF_CHEATCHECKACTIVATOR)
	{
		mobj_t *post = P_GetObjectTypeInSectorNum(MT_CHEATCHECK, sector - sectors);
		if (post)
			P_TouchCheatcheck(post, player, false);
	}
	if (sector->specialflags & SSF_EXIT)
		P_ProcessExitSector(player, sectag);
	if (sector->specialflags & SSF_FAN)
	{
		player->mo->momz += (5*FRACUNIT)/4;

		if (player->mo->momz > (5*FRACUNIT))
			player->mo->momz = (5*FRACUNIT);

		P_ResetPlayer(player);
		/*
		if (player->panim != PA_FALL)
			P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
		*/
	}
	if (sector->specialflags & SSF_ZOOMTUBESTART)
		P_ProcessZoomTube(player, sectag, false);
	if (sector->specialflags & SSF_ZOOMTUBEEND)
		P_ProcessZoomTube(player, sectag, true);
}

static void P_EvaluateDamageType(player_t *player, sector_t *sector, boolean isTouching)
{
	switch (sector->damagetype)
	{
		case SD_GENERIC:
			if (isTouching && K_IsRidingFloatingTop(player) == false)
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_NORMAL);
			break;
		case SD_DEATHPIT:
			if (isTouching)
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_DEATHPIT);
			break;
		case SD_INSTAKILL:
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
			break;
		case SD_STUMBLE:
 			if (isTouching && G_CompatLevel(0x0010))
			{
				player->pflags2 |= PF2_ALWAYSDAMAGED;
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_STUMBLE);
				player->pflags2 &= ~PF2_ALWAYSDAMAGED;

			}
			else
			{
				if (isTouching && player->mo->hitlag == 0 &&
					(player->mo->momz == 0 || (player->mo->momz > 0) != (P_MobjFlip(player->mo) > 0)))
				{
					player->pflags2 |= PF2_ALWAYSDAMAGED;
					P_DamageMobj(player->mo, NULL, NULL, 1, DMG_STUMBLE);
					player->pflags2 &= ~PF2_ALWAYSDAMAGED;
				}
			}

			break;
		default:
			break;
	}
}

static void P_EvaluateLinedefExecutorTrigger(player_t *player, sector_t *sector, boolean isTouching)
{
	if (udmf)
		return;

	if (!sector->triggertag)
		return;

	if (sector->triggerer == TO_MOBJ)
		return;
	else if (sector->triggerer == TO_ALLPLAYERS && !P_DoAllPlayersTrigger(sector->triggertag))
		return;

	if ((sector->flags & MSF_TRIGGERLINE_PLANE) && !isTouching)
		return;

	P_LinedefExecute(sector->triggertag, player->mo, sector);
}

static void P_EvaluateOldSectorSpecial(player_t *player, sector_t *sector, sector_t *roversector, boolean isTouching)
{
	switch (GETSECSPECIAL(sector->special, 1))
	{
		case 9: // Ring Drainer (Floor Touch)
			if (!isTouching)
				break;
			/* FALLTHRU */
		case 10: // Ring Drainer (No Floor Touch)
			if (leveltime % (TICRATE/2) == 0 && player->rings > 0)
			{
				player->rings--;
				S_StartSound(player->mo, sfx_antiri);
			}
			break;
	}

	switch (GETSECSPECIAL(sector->special, 2))
	{
		case 9: // Egg trap capsule
			if (roversector)
				P_ProcessEggCapsule(player, sector);
			break;
	}
}

/** Applies a sector special to a player.
  *
  * \param player       Player in the sector.
  * \param sector       Sector with the special.
  * \param roversector  If !NULL, sector is actually an FOF; otherwise, sector
  *                     is being physically contacted by the player.
  * \sa P_PlayerInSpecialSector, P_PlayerOnSpecial3DFloor
  */
void P_ProcessSpecialSector(player_t *player, sector_t *sector, sector_t *roversector)
{
	boolean isTouching;

	if (!P_SectorHasSpecial(sector))
		return;

	// Ignore spectators
	if (player->spectator)
		return;

	// Ignore dead players.
	// If this strange phenomenon could be potentially used in levels,
	// TODO: modify this to accommodate for it.
	if (player->playerstate != PST_LIVE)
		return;

	isTouching = roversector || P_IsMobjTouchingSectorPlane(player->mo, sector);

	P_EvaluateSpecialFlags(player, sector, roversector, isTouching);
	P_EvaluateDamageType(player, sector, isTouching);
	P_EvaluateLinedefExecutorTrigger(player, sector, isTouching);

	if (!udmf)
		P_EvaluateOldSectorSpecial(player, sector, roversector, isTouching);
}

#define TELEPORTED(mo) (mo->subsector->sector != originalsector)

/** Checks if a player is standing on or is inside a 3D floor (e.g. water) and
  * applies any specials.
  *
  * \param player Player to check.
  * \sa P_ThingOnSpecial3DFloor, P_PlayerInSpecialSector
  */
static void P_PlayerOnSpecial3DFloor(player_t *player, sector_t *sector)
{
	sector_t *originalsector = player->mo->subsector->sector;
	ffloor_t *rover;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!P_SectorHasSpecial(rover->master->frontsector))
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(player->mo, rover, sector))
			continue;

		// This FOF has the special we're looking for, but are we allowed to touch it?
		if (sector == player->mo->subsector->sector
			|| (rover->master->frontsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
		{
			P_ProcessSpecialSector(player, rover->master->frontsector, sector);
			if TELEPORTED(player->mo) return;
		}
	}
}

static void P_PlayerOnSpecialPolyobj(player_t *player)
{
	sector_t *originalsector = player->mo->subsector->sector;
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = player->mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!P_SectorHasSpecial(polysec))
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, player->mo);
		inside = P_MobjInsidePolyobj(po, player->mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(player->mo, po, polysec))
			continue;

		P_ProcessSpecialSector(player, polysec, originalsector);
		if TELEPORTED(player->mo) return;
	}
}

/** Checks if the player is in a special sector or FOF and apply any specials.
  *
  * \param player Player to check.
  * \sa P_PlayerOnSpecial3DFloor, P_ProcessSpecialSector
  */
void P_PlayerInSpecialSector(player_t *player)
{
	sector_t *originalsector;
	sector_t *loopsector;
	msecnode_t *node;

	if (!player->mo)
		return;

	K_ProcessTerrainEffect(player->mo);
	originalsector = player->mo->subsector->sector;

	P_PlayerOnSpecial3DFloor(player, originalsector); // Handle FOFs first.
	if TELEPORTED(player->mo) return;

	// Allow sector specials to be applied to polyobjects!
	P_PlayerOnSpecialPolyobj(player);
	if TELEPORTED(player->mo) return;

	P_ProcessSpecialSector(player, originalsector, NULL);
	if TELEPORTED(player->mo) return;

	// Iterate through touching_sectorlist for SF_TRIGGERSPECIAL_TOUCH
	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		loopsector = node->m_sector;

		if (loopsector == originalsector) // Don't duplicate
			continue;

		// Check 3D floors...
		P_PlayerOnSpecial3DFloor(player, loopsector);
		if TELEPORTED(player->mo) return;

		if (!(loopsector->flags & MSF_TRIGGERSPECIAL_TOUCH))
			continue;

		P_ProcessSpecialSector(player, loopsector, NULL);
		if TELEPORTED(player->mo) return;
	}
}

static void P_CheckMobj3DFloorTrigger(mobj_t *mo, sector_t *sec)
{
	sector_t *originalsector = mo->subsector->sector;
	ffloor_t *rover;

	for (rover = sec->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->triggertag)
			continue;

		if (rover->master->frontsector->triggerer != TO_MOBJ)
			continue;

		if (!(rover->fofflags & FOF_EXISTS))
			continue;

		if (!P_IsMobjTouching3DFloor(mo, rover, sec))
			continue;

		P_LinedefExecute(rover->master->frontsector->triggertag, mo, rover->master->frontsector);
		if TELEPORTED(mo) return;
	}
}

static void P_CheckMobjPolyobjTrigger(mobj_t *mo)
{
	sector_t *originalsector = mo->subsector->sector;
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		if (po->flags & POF_NOSPECIALS)
			continue;

		polysec = po->lines[0]->backsector;

		if (!polysec->triggertag)
			continue;

		if (polysec->triggerer != TO_MOBJ)
			continue;

		touching = (polysec->flags & MSF_TRIGGERSPECIAL_TOUCH) && P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
			continue;

		if (!P_IsMobjTouchingPolyobj(mo, po, polysec))
			continue;

		P_LinedefExecute(polysec->triggertag, mo, polysec);
		if TELEPORTED(mo) return;
	}
}

static void P_CheckMobjSectorTrigger(mobj_t *mo, sector_t *sec)
{
	if (!sec->triggertag)
		return;

	if (sec->triggerer != TO_MOBJ)
		return;

	if ((sec->flags & MSF_TRIGGERLINE_PLANE) && !P_IsMobjTouchingSectorPlane(mo, sec))
		return;

	P_LinedefExecute(sec->triggertag, mo, sec);
}

void P_CheckMobjTrigger(mobj_t *mobj, boolean pushable)
{
	sector_t *originalsector;

	if (udmf)
		return;

	if (!mobj->subsector)
		return;

	originalsector = mobj->subsector->sector;

	if (!pushable && !(originalsector->flags & MSF_TRIGGERLINE_MOBJ))
		return;

	P_CheckMobj3DFloorTrigger(mobj, originalsector);
	if TELEPORTED(mobj)	return;

	P_CheckMobjPolyobjTrigger(mobj);
	if TELEPORTED(mobj)	return;

	P_CheckMobjSectorTrigger(mobj, originalsector);
}

static void P_SectorActionWasActivated(sector_t *sec)
{
	if ((sec->activation & SECSPAC_TRIGGERMASK) == SECSPAC_ONCESPECIAL)
	{
		sec->action = 0;
	}
}

static boolean P_SectorActionIsContinuous(sector_t *sec)
{
	return ((sec->activation & SECSPAC_TRIGGERMASK) == SECSPAC_CONTINUOUSSPECIAL);
}

static boolean P_AllowSpecialEnter(sector_t *sec, mobj_t *thing)
{
	if (thing->player != NULL)
	{
		return !!(sec->activation & SECSPAC_ENTER);
	}
	else if ((thing->flags & (MF_ENEMY|MF_BOSS)) != 0)
	{
		return !!(sec->activation & SECSPAC_ENTERMONSTER);
	}
	else if (K_IsMissileOrKartItem(thing) == true)
	{
		return !!(sec->activation & SECSPAC_ENTERMISSILE);
	}

	// No activation flags for you.
	return false;
}

static boolean P_AllowSpecialFloor(sector_t *sec, mobj_t *thing)
{
	if (thing->player != NULL)
	{
		return !!(sec->activation & SECSPAC_FLOOR);
	}
	else if ((thing->flags & (MF_ENEMY|MF_BOSS)) != 0)
	{
		return !!(sec->activation & SECSPAC_FLOORMONSTER);
	}
	else if (K_IsMissileOrKartItem(thing) == true)
	{
		return !!(sec->activation & SECSPAC_FLOORMISSILE);
	}

	// No activation flags for you.
	return false;
}

static boolean P_AllowSpecialCeiling(sector_t *sec, mobj_t *thing)
{
	if (thing->player != NULL)
	{
		return !!(sec->activation & SECSPAC_CEILING);
	}
	else if ((thing->flags & (MF_ENEMY|MF_BOSS)) != 0)
	{
		return !!(sec->activation & SECSPAC_CEILINGMONSTER);
	}
	else if (K_IsMissileOrKartItem(thing) == true)
	{
		return !!(sec->activation & SECSPAC_CEILINGMISSILE);
	}

	// No activation flags for you.
	return false;
}

static void P_CheckMobj3DFloorAction(mobj_t *mo, sector_t *sec, boolean continuous, boolean sectorchanged)
{
	sector_t *originalsector = mo->subsector->sector;
	ffloor_t *rover;
	sector_t *roversec;

	activator_t *activator = NULL;
	boolean result = false;

	for (rover = sec->ffloors; rover; rover = rover->next)
	{
		fixed_t top = INT32_MIN;
		fixed_t bottom = INT32_MAX;
		fixed_t mid = 0;

		roversec = rover->master->frontsector;

		if (P_SectorActionIsContinuous(roversec) != continuous)
		{
			// Does not match continuous state.
			continue;
		}

		if (P_CanActivateSpecial(roversec->action) == false)
		{
			// No special to even activate.
			continue;
		}

		top = P_GetSpecialTopZ(mo, roversec, sec);
		bottom = P_GetSpecialBottomZ(mo, roversec, sec);
		mid = bottom + ((top - bottom) / 2);

		if (mo->z > top || mo->z + mo->height < bottom)
		{
			// Out of bounds.
			continue;
		}

		if (P_AllowSpecialEnter(roversec, mo) == false)
		{
			boolean floor = false;
			boolean ceiling = false;

			if (P_AllowSpecialFloor(roversec, mo) == true)
			{
				floor = (P_GetMobjFeet(mo) >= mid);
			}

			if (P_AllowSpecialCeiling(roversec, mo) == true)
			{
				ceiling = (P_GetMobjHead(mo) <= mid);
			}

			if (floor == false && ceiling == false)
			{
				continue;
			}
		}
		else if (sectorchanged == false)
		{
			continue;
		}

		activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
		I_Assert(activator != NULL);

		P_SetTarget(&activator->mo, mo);
		activator->sector = roversec;

		result = P_ProcessSpecial(activator, roversec->action, roversec->args, roversec->stringargs);

		P_SetTarget(&activator->mo, NULL);
		Z_Free(activator);

		if (result == true)
		{
			P_SectorActionWasActivated(roversec);
		}

		if TELEPORTED(mo) return;
	}
}

static void P_CheckMobjPolyobjAction(mobj_t *mo, boolean continuous, boolean sectorchanged)
{
	sector_t *originalsector = mo->subsector->sector;
	polyobj_t *po;
	sector_t *polysec;
	boolean touching = false;
	boolean inside = false;

	activator_t *activator = NULL;
	boolean result = false;

	for (po = mo->subsector->polyList; po; po = (polyobj_t *)(po->link.next))
	{
		polysec = po->lines[0]->backsector;

		if (P_SectorActionIsContinuous(polysec) != continuous)
		{
			// Does not match continuous state.
			continue;
		}

		if (P_CanActivateSpecial(polysec->action) == false)
		{
			// No special to even activate.
			continue;
		}

		touching = P_MobjTouchingPolyobj(po, mo);
		inside = P_MobjInsidePolyobj(po, mo);

		if (!(inside || touching))
		{
			continue;
		}

		if (P_AllowSpecialEnter(polysec, mo) == false)
		{
			boolean floor = false;
			boolean ceiling = false;

			if (P_AllowSpecialFloor(polysec, mo) == true)
			{
				floor = (P_GetMobjFeet(mo) == P_GetSpecialTopZ(mo, polysec, polysec));
			}

			if (P_AllowSpecialCeiling(polysec, mo) == true)
			{
				ceiling = (P_GetMobjHead(mo) == P_GetSpecialBottomZ(mo, polysec, polysec));
			}

			if (floor == false && ceiling == false)
			{
				continue;
			}
		}
		else if (sectorchanged == false)
		{
			continue;
		}

		activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
		I_Assert(activator != NULL);

		P_SetTarget(&activator->mo, mo);
		activator->sector = polysec;

		result = P_ProcessSpecial(activator, polysec->action, polysec->args, polysec->stringargs);

		P_SetTarget(&activator->mo, NULL);
		Z_Free(activator);

		if (result == true)
		{
			P_SectorActionWasActivated(polysec);
		}

		if TELEPORTED(mo) return;
	}
}

static void P_CheckMobjSectorAction(mobj_t *mo, sector_t *sec, boolean continuous, boolean sectorchanged)
{
	activator_t *activator = NULL;
	boolean result = false;

	if (P_SectorActionIsContinuous(sec) != continuous)
	{
		// Does not match continuous state.
		return;
	}

	if (P_CanActivateSpecial(sec->action) == false)
	{
		// No special to even activate.
		return;
	}

	if (P_AllowSpecialEnter(sec, mo) == false)
	{
		boolean floor = false;
		boolean ceiling = false;

		if (P_AllowSpecialFloor(sec, mo) == true)
		{
			floor = (P_GetMobjFeet(mo) == P_GetSpecialBottomZ(mo, sec, sec));
		}

		if (P_AllowSpecialCeiling(sec, mo) == true)
		{
			ceiling = (P_GetMobjHead(mo) == P_GetSpecialTopZ(mo, sec, sec));
		}

		if (floor == false && ceiling == false)
		{
			return;
		}
	}
	else if (sectorchanged == false)
	{
		return;
	}

	activator = Z_Calloc(sizeof(activator_t), PU_LEVEL, NULL);
	I_Assert(activator != NULL);

	P_SetTarget(&activator->mo, mo);
	activator->sector = sec;

	result = P_ProcessSpecial(activator, sec->action, sec->args, sec->stringargs);

	P_SetTarget(&activator->mo, NULL);
	Z_Free(activator);

	if (result == true)
	{
		P_SectorActionWasActivated(sec);
	}
}

void P_CheckMobjTouchingSectorActions(mobj_t *mobj, boolean continuous, boolean sectorchanged)
{
	sector_t *originalsector;

	if (mobj->subsector == NULL)
	{
		return;
	}

	originalsector = mobj->subsector->sector;

	if (mobj->player != NULL)
	{
		if (mobj->player->spectator == true)
		{
			// Ignore spectators.
			return;
		}

		if (mobj->player->pflags & PF_NOCONTEST)
		{
			// Ignore NO CONTEST.
			return;
		}
	}

	P_CheckMobj3DFloorAction(mobj, originalsector, continuous, sectorchanged);
	if TELEPORTED(mobj)	return;

	P_CheckMobjPolyobjAction(mobj, continuous, sectorchanged);
	if TELEPORTED(mobj)	return;

	P_CheckMobjSectorAction(mobj, originalsector, continuous, sectorchanged);
}

#undef TELEPORTED

/** Animate planes, scroll walls, etc. and keeps track of level timelimit and exits if time is up.
  *
  * \sa P_CheckTimeLimit, P_CheckPointLimit
  */
void P_UpdateSpecials(void)
{
	anim_t *anim;
	INT32 i;
	INT32 pic;
	size_t j;

	levelflat_t *foundflats; // for flat animation

	// LEVEL TIMER
	P_CheckTimeLimit();

	// POINT LIMIT
	P_CheckPointLimit();

	// ANIMATE TEXTURES
	for (anim = anims; anim < lastanim; anim++)
	{
		for (i = 0; i < anim->numpics; i++)
		{
			pic = anim->basepic + ((leveltime/anim->speed + i) % anim->numpics);
			if (anim->istexture)
				texturetranslation[anim->basepic+i] = pic;
		}
	}

	// ANIMATE FLATS
	/// \todo do not check the non-animate flat.. link the animated ones?
	/// \note its faster than the original anywaysince it animates only
	///    flats used in the level, and there's usually very few of them
	foundflats = levelflats;
	for (j = 0; j < numlevelflats; j++, foundflats++)
	{
		if (foundflats->speed) // it is an animated flat
		{
			// update the levelflat texture number
			if ((foundflats->type == LEVELFLAT_TEXTURE) && (foundflats->u.texture.basenum != -1))
				foundflats->u.texture.num = foundflats->u.texture.basenum + ((leveltime/foundflats->speed + foundflats->animseq) % foundflats->numpics);
			// update the levelflat lump number
			else if ((foundflats->type == LEVELFLAT_FLAT) && (foundflats->u.flat.baselumpnum != LUMPERROR))
				foundflats->u.flat.lumpnum = foundflats->u.flat.baselumpnum + ((leveltime/foundflats->speed + foundflats->animseq) % foundflats->numpics);
		}
	}
}

//
// Floor over floors (FOFs), 3Dfloors, 3Dblocks, fake floors (ffloors), rovers, or whatever you want to call them
//

/** Gets the ID number for a 3Dfloor in its target sector.
  *
  * \param fflr The 3Dfloor we want an ID for.
  * \return ID of 3Dfloor in target sector. Note that the first FOF's ID is 0. UINT16_MAX is given if invalid.
  * \sa P_GetFFloorByID
  */
UINT16 P_GetFFloorID(ffloor_t *fflr)
{
	ffloor_t *rover;
	sector_t *sec;
	UINT16 i = 0;

	if (!fflr)
		return UINT16_MAX;

	sec = fflr->target;

	if (!sec->ffloors)
		return UINT16_MAX;
	for (rover = sec->ffloors; rover; rover = rover->next, i++)
		if (rover == fflr)
			return i;
	return UINT16_MAX;
}

/** Gets a 3Dfloor by control sector.
  *
  * \param sec  Target sector.
  * \param sec2 Control sector.
  * \return Pointer to found 3Dfloor, or NULL.
  * \sa P_GetFFloorByID
  */
static inline ffloor_t *P_GetFFloorBySec(sector_t *sec, sector_t *sec2)
{
	ffloor_t *rover;

	if (!sec->ffloors)
		return NULL;
	for (rover = sec->ffloors; rover; rover = rover->next)
		if (rover->secnum == (size_t)(sec2 - sectors))
			return rover;
	return NULL;
}

/** Gets a 3Dfloor by ID number.
  *
  * \param sec Target sector.
  * \param id  ID of 3Dfloor in target sector. Note that the first FOF's ID is 0.
  * \return Pointer to found 3Dfloor, or NULL.
  * \sa P_GetFFloorBySec, P_GetFFloorID
  */
ffloor_t *P_GetFFloorByID(sector_t *sec, UINT16 id)
{
	ffloor_t *rover;
	UINT16 i = 0;

	if (!sec->ffloors)
		return NULL;
	for (rover = sec->ffloors; rover; rover = rover->next)
		if (i++ == id)
			return rover;
	return NULL;
}

/** Adds a newly formed 3Dfloor structure to a sector's ffloors list.
  *
  * \param sec    Target sector.
  * \param fflr   Newly formed 3Dfloor structure.
  * \sa P_AddFakeFloor
  */
static inline void P_AddFFloorToList(sector_t *sec, ffloor_t *fflr)
{
	ffloor_t *rover;

	if (!sec->ffloors)
	{
		sec->ffloors = fflr;
		fflr->next = 0;
		fflr->prev = 0;
		return;
	}

	for (rover = sec->ffloors; rover->next; rover = rover->next);

	rover->next = fflr;
	fflr->prev = rover;
	fflr->next = 0;
}

/** Adds a 3Dfloor.
  *
  * \param sec         Target sector.
  * \param sec2        Control sector.
  * \param master      Control linedef.
  * \param alpha       Alpha value (0-255).
  * \param blendmode   Blending mode.
  * \param flags       Options affecting this 3Dfloor.
  * \param secthinkers List of relevant thinkers sorted by sector. May be NULL.
  * \return Pointer to the new 3Dfloor.
  * \sa P_AddFFloor, P_AddFakeFloorsByLine, P_SpawnSpecials
  */
static ffloor_t *P_AddFakeFloor(sector_t *sec, sector_t *sec2, line_t *master, INT32 alpha, UINT8 blendmode, ffloortype_e flags, thinkerlist_t *secthinkers)
{
	ffloor_t *fflr;
	thinker_t *th;
	friction_t *f;
	pusher_t *p;
	size_t sec2num;
	size_t i;

	if (sec == sec2)
		return NULL; //Don't need a fake floor on a control sector.
	if ((fflr = (P_GetFFloorBySec(sec, sec2))))
		return fflr; // If this ffloor already exists, return it

	if (sec2->ceilingheight < sec2->floorheight)
	{
		fixed_t tempceiling = sec2->ceilingheight;
		//flip the sector around and print an error instead of crashing 12.1.08 -Inuyasha
		CONS_Alert(CONS_ERROR, M_GetText("A FOF tagged %d has a top height below its bottom.\n"), master->args[0]);
		sec2->ceilingheight = sec2->floorheight;
		sec2->floorheight = tempceiling;
	}

	if (sec2->numattached == 0)
	{
		sec2->attached = Z_Malloc(sizeof (*sec2->attached) * sec2->maxattached, PU_STATIC, NULL);
		sec2->attachedsolid = Z_Malloc(sizeof (*sec2->attachedsolid) * sec2->maxattached, PU_STATIC, NULL);
		sec2->attached[0] = sec - sectors;
		sec2->numattached = 1;
		sec2->attachedsolid[0] = (flags & FOF_SOLID);
	}
	else
	{
		for (i = 0; i < sec2->numattached; i++)
			if (sec2->attached[i] == (size_t)(sec - sectors))
				return NULL;

		if (sec2->numattached >= sec2->maxattached)
		{
			sec2->maxattached *= 2;
			sec2->attached = Z_Realloc(sec2->attached, sizeof (*sec2->attached) * sec2->maxattached, PU_STATIC, NULL);
			sec2->attachedsolid = Z_Realloc(sec2->attachedsolid, sizeof (*sec2->attachedsolid) * sec2->maxattached, PU_STATIC, NULL);
		}
		sec2->attached[sec2->numattached] = sec - sectors;
		sec2->attachedsolid[sec2->numattached] = (flags & FOF_SOLID);
		sec2->numattached++;
	}

	// Add the floor
	fflr = Z_Calloc(sizeof (*fflr), PU_LEVEL, NULL);
	fflr->secnum = sec2 - sectors;
	fflr->target = sec;
	fflr->bottomheight = &sec2->floorheight;
	fflr->bottompic = &sec2->floorpic;
	fflr->bottomxoffs = &sec2->floor_xoffs;
	fflr->bottomyoffs = &sec2->floor_yoffs;
	fflr->bottomangle = &sec2->floorpic_angle;

	// Add the ceiling
	fflr->topheight = &sec2->ceilingheight;
	fflr->toppic = &sec2->ceilingpic;
	fflr->toplightlevel = &sec2->lightlevel;
	fflr->topxoffs = &sec2->ceiling_xoffs;
	fflr->topyoffs = &sec2->ceiling_yoffs;
	fflr->topangle = &sec2->ceilingpic_angle;

	// Add slopes
	fflr->t_slope = &sec2->c_slope;
	fflr->b_slope = &sec2->f_slope;
	// mark the target sector as having slopes, if the FOF has any of its own
	// (this fixes FOF slopes glitching initially at level load in software mode)
	if (sec2->hasslope)
		sec->hasslope = true;

	fflr->spawnflags = fflr->fofflags = flags;
	fflr->master = master;
	fflr->norender = INFTICS;
	fflr->fadingdata = NULL;

	// Scan the thinkers to check for special conditions applying to this FOF.
	// If we have thinkers sorted by sector, just check the relevant ones;
	// otherwise, check them all. Apologies for the ugly loop...

	sec2num = sec2 - sectors;

	// Just initialise both of these to placate the compiler.
	i = 0;
	th = thlist[THINK_MAIN].next;

	for(;;)
	{
		if(secthinkers)
		{
			if(i < secthinkers[sec2num].count)
				th = secthinkers[sec2num].thinkers[i];
			else break;
		}
		else if (th == &thlist[THINK_MAIN])
			break;

		// Should this FOF have friction?
		if(th->function.acp1 == (actionf_p1)T_Friction)
		{
			f = (friction_t *)th;

			if (f->affectee == (INT32)sec2num)
				Add_Friction(f->friction, f->movefactor, (INT32)(sec-sectors), f->affectee);
		}
		// Should this FOF have wind/current/pusher?
		else if(th->function.acp1 == (actionf_p1)T_Pusher)
		{
			p = (pusher_t *)th;

			if (p->affectee == (INT32)sec2num)
				Add_Pusher(p->type, p->x_mag, p->y_mag, p->z_mag, (INT32)(sec-sectors), p->affectee, p->exclusive, p->slider);
		}

		if(secthinkers) i++;
		else th = th->next;
	}

	//fflr->alpha = max(0, min(0xff, alpha));
	fflr->alpha = alpha;
	if (blendmode != TMB_TRANSLUCENT || fflr->alpha != 0xff || flags & FOF_SPLAT)
	{
		fflr->fofflags |= FOF_TRANSLUCENT;
		fflr->spawnflags = fflr->fofflags;
	}
	fflr->spawnalpha = fflr->alpha; // save for netgames

	switch (blendmode)
	{
		case TMB_TRANSLUCENT:
		default:
			fflr->blend = AST_COPY;
			break;
		case TMB_ADD:
			fflr->blend = AST_ADD;
			break;
		case TMB_SUBTRACT:
			fflr->blend = AST_SUBTRACT;
			break;
		case TMB_REVERSESUBTRACT:
			fflr->blend = AST_REVERSESUBTRACT;
			break;
		case TMB_MODULATE:
			fflr->blend = AST_MODULATE;
			break;
	}

	if (flags & FOF_QUICKSAND)
		CheckForQuicksand = true;

	if (flags & FOF_BUSTUP)
		CheckForBustableBlocks = true;

	if ((flags & FOF_MARIO))
	{
		if (!(flags & FOF_GOOWATER)) // Don't change the textures of a brick block, just a question block
			P_AddBlockThinker(sec2, master);
		CheckForMarioBlocks = true;
	}

	if ((flags & FOF_CRUMBLE))
		sec2->crumblestate = CRUMBLE_WAIT;

	if ((flags & FOF_FLOATBOB))
	{
		P_AddFloatThinker(sec2, master->args[0], master);
		CheckForFloatBob = true;
	}

	P_AddFFloorToList(sec, fflr);

	return fflr;
}

static void
P_RaiseTaggedThingsToFakeFloor (
		UINT16    type,
		mtag_t    tag,
		sector_t *control
){
	sector_t *target;

	mobj_t     *mo;
	mapthing_t *mthing;

	fixed_t offset;

	size_t i;

	for (i = 0; i < control->numattached; ++i)
	{
		target = &sectors[control->attached[i]];

		for (mo = target->thinglist; mo; mo = mo->snext)
		{
			mthing = mo->spawnpoint;

			if (mthing == NULL)
			{
				continue;
			}

			if (
					(type == 0 || mthing->type == type) &&
					(tag == 0 || (udmf ? mthing->tid : mthing->angle) == tag)
			){
				if (( mo->flags2 & MF2_OBJECTFLIP ))
				{
					offset = ( mo->ceilingz - mo->info->height ) - mo->z;
					mo->z = ( P_GetZAt(control->f_slope, mo->x, mo->y, control->floorheight) - mo->info->height ) - offset;
				}
				else
				{
					offset = mo->z - mo->floorz;
					mo->z = P_GetZAt(control->c_slope, mo->x, mo->y, control->ceilingheight) + offset;
				}

				mthing->adjusted_z = mo->z;
			}
		}
	}
}

//
// SPECIAL SPAWNING
//

/** Adds a float thinker.
  * Float thinkers cause solid 3Dfloors to float on water.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \sa P_SpawnSpecials, T_FloatSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddFloatThinker(sector_t *sec, UINT16 tag, line_t *sourceline)
{
	floatthink_t *floater;

	// create and initialize new thinker
	floater = Z_LevelPoolCalloc(sizeof (*floater));
	floater->thinker.alloctype = TAT_LEVELPOOL;
	floater->thinker.size = sizeof (*floater);
	P_AddThinker(THINK_MAIN, &floater->thinker);

	floater->thinker.function.acp1 = (actionf_p1)T_FloatSector;

	floater->sector = sec;
	floater->tag = (INT16)tag;
	floater->sourceline = sourceline;

	// interpolation
	R_CreateInterpolator_SectorPlane(&floater->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&floater->thinker, sec, true);
}

/**
  * Adds a plane displacement thinker.
  * Whenever the "control" sector moves,
  * the "affectee" sector's floor or ceiling plane moves too!
  *
  * \param speed            Rate of movement relative to control sector
  * \param control          Control sector.
  * \param affectee         Target sector.
  * \param reverse          Reverse direction?
  * \sa P_SpawnSpecials, T_PlaneDisplace
  * \author Monster Iestyn
  */
static void P_AddPlaneDisplaceThinker(INT32 type, fixed_t speed, INT32 control, INT32 affectee, UINT8 reverse)
{
	planedisplace_t *displace;

	// create and initialize new displacement thinker
	displace = Z_LevelPoolCalloc(sizeof (*displace));
	displace->thinker.alloctype = TAT_LEVELPOOL;
	displace->thinker.size = sizeof (*displace);
	P_AddThinker(THINK_MAIN, &displace->thinker);

	displace->thinker.function.acp1 = (actionf_p1)T_PlaneDisplace;
	displace->affectee = affectee;
	displace->control = control;
	displace->last_height = sectors[control].floorheight;
	displace->speed = speed;
	displace->type = type;
	displace->reverse = reverse;

	// interpolation
	R_CreateInterpolator_SectorPlane(&displace->thinker, &sectors[affectee], false);
}

/** Adds a Mario block thinker, which changes the block's texture between blank
  * and ? depending on whether it has contents.
  * Needed in case objects respawn inside.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_MarioBlockChecker
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline)
{
	mariocheck_t *block;

	// create and initialize new elevator thinker
	block = Z_LevelPoolCalloc(sizeof (*block));
	block->thinker.alloctype = TAT_LEVELPOOL;
	block->thinker.size = sizeof (*block);
	P_AddThinker(THINK_MAIN, &block->thinker);

	block->thinker.function.acp1 = (actionf_p1)T_MarioBlockChecker;
	block->sourceline = sourceline;

	block->sector = sec;
}

/** Adds a raise thinker.
  * A raise thinker checks to see if the
  * player is standing on its 3D Floor,
  * and if so, raises the platform towards
  * it's destination. Otherwise, it lowers
  * to the lowest nearby height if not
  * there already.
  *
  * \param sec          Control sector.
  * \sa P_SpawnSpecials, T_RaiseSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddRaiseThinker(sector_t *sec, INT16 tag, fixed_t speed, fixed_t ceilingtop, fixed_t ceilingbottom, boolean lower, boolean spindash)
{
	raise_t *raise;

	raise = Z_LevelPoolCalloc(sizeof (*raise));
	raise->thinker.alloctype = TAT_LEVELPOOL;
	raise->thinker.size = sizeof (*raise);
	P_AddThinker(THINK_MAIN, &raise->thinker);

	raise->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	raise->tag = tag;
	raise->sector = sec;

	raise->ceilingtop = ceilingtop;
	raise->ceilingbottom = ceilingbottom;

	raise->basespeed = speed >> 2;

	if (lower)
		raise->flags |= RF_REVERSE;
	if (spindash)
		raise->flags |= RF_SPINDASH;

	// interpolation
	R_CreateInterpolator_SectorPlane(&raise->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&raise->thinker, sec, true);
}

static void P_AddAirbob(sector_t *sec, INT16 tag, fixed_t dist, boolean raise, boolean spindash, boolean dynamic)
{
	raise_t *airbob;

	airbob = Z_LevelPoolCalloc(sizeof (*airbob));
	airbob->thinker.alloctype = TAT_LEVELPOOL;
	airbob->thinker.size = sizeof (*airbob);
	P_AddThinker(THINK_MAIN, &airbob->thinker);

	airbob->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	airbob->tag = tag;
	airbob->sector = sec;

	airbob->ceilingtop = sec->ceilingheight;
	airbob->ceilingbottom = sec->ceilingheight - dist;

	airbob->basespeed = FRACUNIT;

	if (!raise)
		airbob->flags |= RF_REVERSE;
	if (spindash)
		airbob->flags |= RF_SPINDASH;
	if (dynamic)
		airbob->flags |= RF_DYNAMIC;

	// interpolation
	R_CreateInterpolator_SectorPlane(&airbob->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&airbob->thinker, sec, true);
}

/** Adds a thwomp thinker.
  * Even thwomps need to think!
  *
  * \param sec          Control sector.
  * \sa P_SpawnSpecials, T_ThwompSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddThwompThinker(sector_t *sec, line_t *sourceline, fixed_t crushspeed, fixed_t retractspeed, UINT16 sound)
{
	thwomp_t *thwomp;

	// You *probably* already have a thwomp in this sector. If you've combined it with something
	// else that uses the floordata/ceilingdata, you must be weird.
	if (sec->floordata || sec->ceilingdata)
		return;

	// create and initialize new elevator thinker
	thwomp = Z_LevelPoolCalloc(sizeof (*thwomp));
	thwomp->thinker.alloctype = TAT_LEVELPOOL;
	thwomp->thinker.size = sizeof (*thwomp);
	P_AddThinker(THINK_MAIN, &thwomp->thinker);

	thwomp->thinker.function.acp1 = (actionf_p1)T_ThwompSector;

	// set up the fields according to the type of elevator action
	thwomp->sourceline = sourceline;
	thwomp->sector = sec;
	thwomp->crushspeed = crushspeed;
	thwomp->retractspeed = retractspeed;
	thwomp->direction = 0;
	thwomp->floorstartheight = sec->floorheight;
	thwomp->ceilingstartheight = sec->ceilingheight;
	thwomp->delay = 1;
	thwomp->tag = sourceline->args[0];
	thwomp->sound = sound;
	thwomp->initDelay = sourceline->args[3];

	sec->floordata = thwomp;
	sec->ceilingdata = thwomp;
	// Start with 'resting' texture
	sides[sourceline->sidenum[0]].midtexture = sides[sourceline->sidenum[0]].bottomtexture;

	// interpolation
	R_CreateInterpolator_SectorPlane(&thwomp->thinker, sec, false);
	R_CreateInterpolator_SectorPlane(&thwomp->thinker, sec, true);
}

/** Adds a thinker which checks if any MF_ENEMY objects with health are in the defined area.
  * If not, a linedef executor is run once.
  *
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_NoEnemiesSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddNoEnemiesThinker(line_t *sourceline)
{
	noenemies_t *nobaddies;

	// create and initialize new thinker
	nobaddies = Z_LevelPoolCalloc(sizeof (*nobaddies));
	nobaddies->thinker.alloctype = TAT_LEVELPOOL;
	nobaddies->thinker.size = sizeof (*nobaddies);
	P_AddThinker(THINK_MAIN, &nobaddies->thinker);

	nobaddies->thinker.function.acp1 = (actionf_p1)T_NoEnemiesSector;

	nobaddies->sourceline = sourceline;
}

/** Adds a thinker for Each-Time linedef executors. A linedef executor is run
  * only when a player enters the area and doesn't run again until they re-enter.
  *
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_EachTimeThinker
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddEachTimeThinker(line_t *sourceline, boolean triggerOnExit)
{
	eachtime_t *eachtime;

	// create and initialize new thinker
	eachtime = Z_LevelPoolCalloc(sizeof (*eachtime));
	eachtime->thinker.alloctype = TAT_LEVELPOOL;
	eachtime->thinker.size = sizeof (*eachtime);
	P_AddThinker(THINK_MAIN, &eachtime->thinker);

	eachtime->thinker.function.acp1 = (actionf_p1)T_EachTimeThinker;

	eachtime->sourceline = sourceline;
	eachtime->triggerOnExit = triggerOnExit;
}

/** Adds a camera scanner.
  *
  * \param sourcesec    Control sector.
  * \param actionsector Target sector.
  * \param angle        Angle of the source line.
  * \sa P_SpawnSpecials, T_CameraScanner
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddCameraScanner(sector_t *sourcesec, sector_t *actionsector, angle_t angle)
{
	elevator_t *elevator; // Why not? LOL

	CONS_Alert(CONS_WARNING, M_GetText("Detected a camera scanner effect (linedef type 5). This effect is deprecated and will be removed in the future!\n"));

	// create and initialize new elevator thinker
	elevator = Z_LevelPoolCalloc(sizeof (*elevator));
	elevator->thinker.alloctype = TAT_LEVELPOOL;
	elevator->thinker.size = sizeof (*elevator);
	P_AddThinker(THINK_MAIN, &elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_CameraScanner;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sourcesec;
	elevator->actionsector = actionsector;
	elevator->distance = FixedInt(AngleFixed(angle));
}

/** Flashes a laser block.
  *
  * \param flash Thinker structure for this laser.
  * \sa P_AddLaserThinker
  * \author SSNTails <http://www.ssntails.org>
  */
void T_LaserFlash(laserthink_t *flash)
{
	msecnode_t *node;
	mobj_t *thing;
	INT32 s;
	ffloor_t *fflr;
	sector_t *sector;
	sector_t *sourcesec = flash->sourceline->frontsector;
	fixed_t top, bottom;

	TAG_ITER_SECTORS(flash->tag, s)
	{
		sector = &sectors[s];
		for (fflr = sector->ffloors; fflr; fflr = fflr->next)
		{
			if (fflr->master != flash->sourceline)
				continue;

			if (!(fflr->fofflags & FOF_EXISTS))
				break;

			if (leveltime & 2)
				//fflr->flags |= FOF_RENDERALL;
				fflr->alpha = 0xB0;
			else
				//fflr->flags &= ~FOF_RENDERALL;
				fflr->alpha = 0x90;

			top    = P_GetFFloorTopZAt   (fflr, sector->soundorg.x, sector->soundorg.y);
			bottom = P_GetFFloorBottomZAt(fflr, sector->soundorg.x, sector->soundorg.y);
			sector->soundorg.z = (top + bottom)/2;
			S_StartSound(&sector->soundorg, sfx_laser);

			// Seek out objects to DESTROY! MUAHAHHAHAHAA!!!*cough*
			for (node = sector->touching_thinglist; node && node->m_thing; node = node->m_thinglist_next)
			{
				thing = node->m_thing;

				if (flash->nobosses && thing->flags & MF_BOSS)
					continue; // Don't hurt bosses

				// Don't endlessly kill egg guard shields (or anything else for that matter)
				if (thing->health <= 0)
					continue;

				top = P_GetSpecialTopZ(thing, sourcesec, sector);
				bottom = P_GetSpecialBottomZ(thing, sourcesec, sector);

				if (thing->z >= top
				|| thing->z + thing->height <= bottom)
					continue;

				if (thing->flags & MF_SHOOTABLE)
					P_DamageMobj(thing, NULL, NULL, 1, DMG_NORMAL);
			}

			break;
		}
	}
}

static inline void P_AddLaserThinker(INT16 tag, line_t *line, boolean nobosses)
{
	laserthink_t *flash = Z_LevelPoolCalloc(sizeof (*flash));
	flash->thinker.alloctype = TAT_LEVELPOOL;
	flash->thinker.size = sizeof (*flash);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LaserFlash;
	flash->tag = tag;
	flash->sourceline = line;
	flash->nobosses = nobosses;
}

//
// P_RunLevelLoadExecutors
//
// After loading/spawning all other specials
// and items, execute these.
//
static void P_RunLevelLoadExecutors(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 399 || lines[i].special == 328 || lines[i].special == 323)
			P_RunTriggerLinedef(&lines[i], NULL, NULL);
	}
}

/** Before things are loaded, initialises certain stuff in case they're needed
  * by P_SpawnSlopes or P_LoadThings. This was split off from
  * P_SpawnSpecials, in case you couldn't tell.
  *
  * \sa P_SpawnSpecials
  * \author Monster Iestyn
  */
void P_InitSpecials(void)
{
	mapheader_lighting_t *lighting = &mapheaderinfo[gamemap-1]->lighting;

	if (encoremode
#ifdef DEVELOP
			&& cv_kartencoremap.value
#endif
			&& mapheaderinfo[gamemap-1]->use_encore_lighting)
	{
		lighting = &mapheaderinfo[gamemap-1]->lighting_encore;
	}

	// Set the map object scale
	mapobjectscale = mapheaderinfo[gamemap-1]->mobj_scale;

	// Set the default gravity. Custom gravity overrides this setting.
	gravity = mapheaderinfo[gamemap-1]->gravity;

	// Set map lighting settings.
	maplighting.contrast = lighting->light_contrast;
	maplighting.backlight = lighting->sprite_backlight;
	maplighting.directional = lighting->use_light_angle;
	maplighting.angle = lighting->light_angle;

	CheckForBustableBlocks = CheckForBouncySector = CheckForQuicksand = CheckForMarioBlocks = CheckForFloatBob = CheckForReverseGravity = false;

	// Set weather
	curWeather = globalweather = mapheaderinfo[gamemap-1]->weather;
}

void P_ApplyFlatAlignment(sector_t *sector, angle_t flatangle, fixed_t xoffs, fixed_t yoffs, boolean floor, boolean ceiling)
{
	if (floor)
	{
		sector->floorpic_angle = flatangle;
		sector->floor_xoffs += xoffs;
		sector->floor_yoffs += yoffs;
	}

	if (ceiling)
	{
		sector->ceilingpic_angle = flatangle;
		sector->ceiling_xoffs += xoffs;
		sector->ceiling_yoffs += yoffs;
	}
}

static boolean P_IsLineDisabled (const line_t * line)
{
	if (line->special != 7) // This is a hack. I can at least hope nobody wants to prevent flat alignment in netgames...
	{
		if (netgame)
		{
			if (line->flags & ML_NONET)
			{
				return true;
			}
		}
		else if (line->flags & ML_NETONLY)
		{
			return true;
		}
	}

	return false;
}

static void P_MakeFOFBouncy(line_t *paramline, line_t *masterline)
{
	INT32 s;

	if (masterline->special < 100 || masterline->special >= 300)
		return;

	TAG_ITER_SECTORS(masterline->args[0], s)
	{
		ffloor_t *rover;

		for (rover = sectors[s].ffloors; rover; rover = rover->next)
		{
			if (rover->master != masterline)
				continue;

			rover->fofflags |= FOF_BOUNCY;
			rover->spawnflags |= FOF_BOUNCY;
			rover->bouncestrength = (paramline->args[1]<< FRACBITS)/100;
			CheckForBouncySector = true;
			break;
		}
	}

}

static boolean P_CheckGametypeRules(INT32 checktype, UINT32 target)
{
	switch (checktype)
	{
		case TMF_HASALL:
		default:
			return (gametyperules & target) == target;
		case TMF_HASANY:
			return !!(gametyperules & target);
		case TMF_HASEXACTLY:
			return gametyperules == target;
		case TMF_DOESNTHAVEALL:
			return (gametyperules & target) != target;
		case TMF_DOESNTHAVEANY:
			return !(gametyperules & target);
	}
}

fixed_t P_GetSectorGravityFactor(sector_t *sec)
{
	if (sec->gravityptr)
		return FixedDiv(*sec->gravityptr >> FRACBITS, 1000);
	else
		return sec->gravity;
}

/** After the map has loaded, scans for specials that spawn 3Dfloors and
  * thinkers.
  *
  * \todo Split up into multiple functions.
  * \todo Get rid of all the magic numbers.
  * \todo Potentially use 'fromnetsave' to stop any new thinkers from being created
  *       as they'll just be erased by UnArchiveThinkers.
  * \sa P_SpawnPrecipitation, P_SpawnFriction, P_SpawnPushers, P_SpawnScrollers
  */
void P_SpawnSpecials(boolean fromnetsave)
{
	sector_t *sector;
	size_t i;
	INT32 j;
	thinkerlist_t *secthinkers;
	thinker_t *th;
	// This used to be used, and *should* be used in the future,
	// but currently isn't.
	(void)fromnetsave;

	// yep, we do this here - "bossdisabled" is considered an apparatus of specials.
	bossdisabled = 0;
	stoppedclock = false;

	// Init special SECTORs.
	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		CheckForReverseGravity |= (sector->flags & MSF_GRAVITYFLIP);

		if (!sector->special)
			continue;

		// Process Section 1
		switch(GETSECSPECIAL(sector->special, 1))
		{
			case 5: // Spikes
				//Terrible hack to replace an even worse hack:
				//Spike damage automatically sets MSF_TRIGGERSPECIAL_TOUCH.
				//Yes, this also affects other specials on the same sector. Sorry.
				sector->flags |= MSF_TRIGGERSPECIAL_TOUCH;
				break;
			case 15: // Bouncy FOF
				if (udmf)
					break;
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated bouncy FOF sector type detected. Please use linedef type 76 instead.\n"));
				CheckForBouncySector = true;
				break;
		}

		// Process Section 2
		switch(GETSECSPECIAL(sector->special, 2))
		{
			case 11: // Custom global gravity!
				if (udmf)
					break;
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated sector type for global gravity detected. Please use the Gravity level header option instead.\n"));
				gravity = sector->floorheight/1000;
				break;
		}

		// Process Section 4
		switch(GETSECSPECIAL(sector->special, 4))
		{
			case 10: // Circuit finish line
				if (udmf)
					break;
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated finish line sector type detected. Please use the linedef type instead.\n"));
				break;
		}
	}

	P_SpawnScrollers(); // Add generalized scrollers
	P_SpawnFriction();  // Friction model using linedefs
	P_SpawnPushers();   // Pusher model using linedefs

	// Look for thinkers that affect FOFs, and sort them by sector

	secthinkers = Z_Calloc(numsectors * sizeof(thinkerlist_t), PU_STATIC, NULL);

	// Firstly, find out how many there are in each sector
	for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)T_Friction)
			secthinkers[((friction_t *)th)->affectee].count++;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secthinkers[((pusher_t *)th)->affectee].count++;
	}

	// Allocate each list, and then zero the count so we can use it to track
	// the end of the list as we add the thinkers
	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].count > 0)
		{
			secthinkers[i].thinkers = Z_Malloc(secthinkers[i].count * sizeof(thinker_t *), PU_STATIC, NULL);
			secthinkers[i].count = 0;
		}

	// Finally, populate the lists.
	for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
	{
		size_t secnum = (size_t)-1;

		if (th->function.acp1 == (actionf_p1)T_Friction)
			secnum = ((friction_t *)th)->affectee;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secnum = ((pusher_t *)th)->affectee;

		if (secnum != (size_t)-1)
			secthinkers[secnum].thinkers[secthinkers[secnum].count++] = th;
	}

	// Init line EFFECTs
	for (i = 0; i < numlines; i++)
	{
		if (P_IsLineDisabled(&lines[i]))
		{
			continue;
		}

		switch (lines[i].special)
		{
			INT32 s;
			INT32 l;
			size_t sec;
			ffloortype_e ffloorflags;

			case 1: // Definable gravity per sector
				if (udmf)
					break;

				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(Tag_FGet(&lines[i].tags), s)
				{
					sectors[s].gravityptr = &sectors[sec].floorheight; // This allows it to change in realtime!

					if (lines[i].flags & ML_NOCLIMB)
						sectors[s].flags |= MSF_GRAVITYFLIP;
					else
						sectors[s].flags &= ~MSF_GRAVITYFLIP;

					CheckForReverseGravity |= (sectors[s].flags & MSF_GRAVITYFLIP);
				}
				break;

			case 5: // Change camera info
				if (udmf)
					break;

				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(Tag_FGet(&lines[i].tags), s)
					P_AddCameraScanner(&sectors[sec], &sectors[s], R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y));
				break;

			case 7: // Flat alignment - redone by toast
			{
				// Set calculated offsets such that line's v1 is the apparent origin
				angle_t flatangle = InvAngle(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y));
				fixed_t xoffs = -lines[i].v1->x;
				fixed_t yoffs = lines[i].v1->y;

				//If no tag is given, apply to front sector
				if (lines[i].args[0] == 0)
					P_ApplyFlatAlignment(lines[i].frontsector, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				else
				{
					TAG_ITER_SECTORS(lines[i].args[0], s)
						P_ApplyFlatAlignment(sectors + s, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				}
				break;
			}

			case 8: // Set camera collision planes
				if (lines[i].frontsector)
					TAG_ITER_SECTORS(lines[i].args[0], s)
						sectors[s].camsec = lines[i].frontsector-sectors;
				break;

			case 10: // Vertical culling plane for sprites and FOFs
				TAG_ITER_SECTORS(lines[i].args[0], s)
					sectors[s].cullheight = &lines[i]; // This allows it to change in realtime!
				break;

			case 50: // Insta-Lower Sector
				if (!udmf)
					EV_DoFloor(lines[i].args[0], &lines[i], instantLower);
				break;

			case 51: // Instant raise for ceilings
				if (!udmf)
					EV_DoCeiling(lines[i].args[0], &lines[i], instantRaise);
				break;

			case 52: // Continuously Falling sector
				EV_DoContinuousFall(lines[i].frontsector, lines[i].backsector, lines[i].args[0] << FRACBITS, lines[i].args[1]);
				break;

			case 53: // Continuous plane movement (slowdown)
				if (lines[i].backsector)
				{
					if (lines[i].args[1] != TMP_CEILING)
						EV_DoFloor(lines[i].args[0], &lines[i], bounceFloor);
					if (lines[i].args[1] != TMP_FLOOR)
						EV_DoCeiling(lines[i].args[0], &lines[i], bounceCeiling);
				}
				break;

			case 56: // Continuous plane movement (constant)
				if (lines[i].backsector)
				{
					if (lines[i].args[1] != TMP_CEILING)
						EV_DoFloor(lines[i].args[0], &lines[i], bounceFloorCrush);
					if (lines[i].args[1] != TMP_FLOOR)
						EV_DoCeiling(lines[i].args[0], &lines[i], bounceCeilingCrush);
				}
				break;

			case 60: // Moving platform
				EV_DoElevator(lines[i].args[0], &lines[i], elevateContinuous);
				break;

			case 61: // Crusher!
				EV_DoCrush(lines[i].args[0], &lines[i], lines[i].args[1] ? raiseAndCrush : crushAndRaise);
				break;

			case 63: // support for drawn heights coming from different sector
				sec = sides[*lines[i].sidenum].sector-sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					sectors[s].heightsec = (INT32)sec;
				break;

			case 64: // Appearing/Disappearing FOF option
				if (lines[i].args[0] == 0) // Find FOFs by control sector tag
				{
					TAG_ITER_SECTORS(lines[i].args[1], s)
					{
						for (j = 0; (unsigned)j < sectors[s].linecount; j++)
						{
							if (sectors[s].lines[j]->special < 100 || sectors[s].lines[j]->special >= 300)
								continue;

							Add_MasterDisappearer(abs(lines[i].args[2]), abs(lines[i].args[3]), abs(lines[i].args[4]), (INT32)(sectors[s].lines[j] - lines), (INT32)i);
						}
					}
				}
				else // Find FOFs by effect sector tag
				{
					TAG_ITER_LINES(lines[i].args[0], s)
					{
						if (lines[s].special < 100 || lines[s].special >= 300)
							continue;

						if (lines[i].args[1] != 0 && !Tag_Find(&lines[s].frontsector->tags, lines[i].args[1]))
							continue;

						Add_MasterDisappearer(abs(lines[i].args[2]), abs(lines[i].args[3]), abs(lines[i].args[4]), s, (INT32)i);
					}
				}
				break;

			case 66: // Displace planes by front sector
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_AddPlaneDisplaceThinker(lines[i].args[1], abs(lines[i].args[2])<<8, sides[lines[i].sidenum[0]].sector-sectors, s, lines[i].args[2] < 0);
				break;

			case 70: // Add raise thinker to FOF
				if (udmf)
				{
					fixed_t destheight = lines[i].args[2] << FRACBITS;
					fixed_t startheight, topheight, bottomheight;

					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						startheight = lines[l].frontsector->ceilingheight;
						topheight = max(startheight, destheight);
						bottomheight = min(startheight, destheight);

						P_AddRaiseThinker(lines[l].frontsector, lines[l].args[0], lines[i].args[1] << FRACBITS, topheight, bottomheight, (destheight < startheight), !!(lines[i].args[3]));
					}
				}
				break;

			case 71: // Add air bob thinker to FOF
				if (udmf)
				{
					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						P_AddAirbob(lines[l].frontsector, lines[l].args[0], lines[i].args[1] << FRACBITS, !!(lines[i].args[2] & TMFB_REVERSE), !!(lines[i].args[2] & TMFB_SPINDASH), !!(lines[i].args[2] & TMFB_DYNAMIC));
					}
				}
				break;

			case 72: // Add thwomp thinker to FOF
				if (udmf)
				{
					UINT16 sound = (lines[i].stringargs[0]) ? get_number(lines[i].stringargs[0]) : sfx_thwomp;

					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						P_AddThwompThinker(lines[l].frontsector, &lines[l], lines[i].args[1] << (FRACBITS - 3), lines[i].args[2] << (FRACBITS - 3), sound);
					}
				}
				break;

			case 73: // Add laser thinker to FOF
				if (udmf)
				{
					TAG_ITER_LINES(lines[i].args[0], l)
					{
						if (lines[l].special < 100 || lines[l].special >= 300)
							continue;

						P_AddLaserThinker(lines[l].args[0], lines + l, !!(lines[i].args[1]));
					}
				}
				break;

			case 100: // FOF (solid)
				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL;

				//Appearance settings
				if (lines[i].args[3] & TMFA_NOPLANES)
					ffloorflags &= ~FOF_RENDERPLANES;
				if (lines[i].args[3] & TMFA_NOSIDES)
					ffloorflags &= ~FOF_RENDERSIDES;
				if (lines[i].args[3] & TMFA_INSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				if (lines[i].args[3] & TMFA_ONLYINSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_INVERTPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_INVERTSIDES;
				}
				if (lines[i].args[3] & TMFA_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[3] & TMFA_SPLAT)
					ffloorflags |= FOF_SPLAT;

				//Tangibility settings
				if (lines[i].args[4] & TMFT_INTANGIBLETOP)
					ffloorflags |= FOF_REVERSEPLATFORM;
				if (lines[i].args[4] & TMFT_INTANGIBLEBOTTOM)
					ffloorflags |= FOF_PLATFORM;
				if (lines[i].args[4] & TMFT_DONTBLOCKPLAYER)
					ffloorflags &= ~FOF_BLOCKPLAYER;
				if (lines[i].args[4] & TMFT_DONTBLOCKOTHERS)
					ffloorflags &= ~FOF_BLOCKOTHERS;

				//Cutting options
				if (ffloorflags & FOF_RENDERALL)
				{
					//If translucent or player can enter it, cut inner walls
					if ((lines[i].args[1] < 255) || (lines[i].args[4] & TMFT_VISIBLEFROMINSIDE))
						ffloorflags |= FOF_CUTEXTRA|FOF_EXTRA;
					else
						ffloorflags |= FOF_CUTLEVEL;
				}

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 120: // FOF (water)
				ffloorflags = FOF_EXISTS|FOF_RENDERPLANES|FOF_SWIMMABLE|FOF_BOTHPLANES|FOF_CUTEXTRA|FOF_EXTRA|FOF_CUTSPRITES;
				if (!(lines[i].args[3] & TMFW_NOSIDES))
					ffloorflags |= FOF_RENDERSIDES|FOF_ALLSIDES;
				if (lines[i].args[3] & TMFW_DOUBLESHADOW)
					ffloorflags |= FOF_DOUBLESHADOW;
				if (lines[i].args[3] & TMFW_COLORMAPONLY)
					ffloorflags |= FOF_COLORMAPONLY;
				if (!(lines[i].args[3] & TMFW_NORIPPLE))
					ffloorflags |= FOF_RIPPLE;
				if (lines[i].args[3] & TMFW_GOOWATER)
					ffloorflags |= FOF_GOOWATER;
				if (lines[i].args[3] & TMFW_SPLAT)
					ffloorflags |= FOF_SPLAT;
				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 150: // FOF (Air bobbing)
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_SOLID|FOF_RENDERALL, secthinkers);
				P_AddAirbob(lines[i].frontsector, lines[i].args[0], lines[i].args[1] << FRACBITS, !!(lines[i].args[2] & TMFB_REVERSE), !!(lines[i].args[2] & TMFB_SPINDASH), !!(lines[i].args[2] & TMFB_DYNAMIC));
				break;

			case 160: // FOF (Water bobbing)
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_FLOATBOB, secthinkers);
				break;

			case 170: // FOF (Crumbling)
				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_CRUMBLE;

				//Tangibility settings
				if (lines[i].args[3] & TMFT_INTANGIBLETOP)
					ffloorflags |= FOF_REVERSEPLATFORM;
				if (lines[i].args[3] & TMFT_INTANGIBLEBOTTOM)
					ffloorflags |= FOF_PLATFORM;
				if (lines[i].args[3] & TMFT_DONTBLOCKPLAYER)
					ffloorflags &= ~FOF_BLOCKPLAYER;
				if (lines[i].args[3] & TMFT_DONTBLOCKOTHERS)
					ffloorflags &= ~FOF_BLOCKOTHERS;

				//Flags
				if (lines[i].args[4] & TMFC_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[4] & TMFC_NORETURN)
					ffloorflags |= FOF_NORETURN;
				if (lines[i].args[4] & TMFC_FLOATBOB)
					ffloorflags |= FOF_FLOATBOB;
				if (lines[i].args[4] & TMFC_SPLAT)
					ffloorflags |= FOF_SPLAT;

				//If translucent or player can enter it, cut inner walls
				if (lines[i].args[1] < 0xff || (lines[i].args[3] & TMFT_VISIBLEFROMINSIDE))
					ffloorflags |= FOF_CUTEXTRA|FOF_EXTRA;
				else
					ffloorflags |= FOF_CUTLEVEL;

				//If player can enter it, render insides
				if (lines[i].args[3] & TMFT_VISIBLEFROMINSIDE)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				if (lines[i].args[4] & TMFC_AIRBOB)
					P_AddAirbob(lines[i].frontsector, lines[i].args[0], 16*FRACUNIT, false, false, false);
				break;

			case 190: // FOF (Rising)
			{
				fixed_t ceilingtop = P_FindHighestCeilingSurrounding(lines[i].frontsector);
				fixed_t ceilingbottom = P_FindLowestCeilingSurrounding(lines[i].frontsector);

				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL;

				//Appearance settings
				if (lines[i].args[3] & TMFA_NOPLANES)
					ffloorflags &= ~FOF_RENDERPLANES;
				if (lines[i].args[3] & TMFA_NOSIDES)
					ffloorflags &= ~FOF_RENDERSIDES;
				if (lines[i].args[3] & TMFA_INSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				if (lines[i].args[3] & TMFA_ONLYINSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_INVERTPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_INVERTSIDES;
				}
				if (lines[i].args[3] & TMFA_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[3] & TMFA_SPLAT)
					ffloorflags |= FOF_SPLAT;

				//Tangibility settings
				if (lines[i].args[4] & TMFT_INTANGIBLETOP)
					ffloorflags |= FOF_REVERSEPLATFORM;
				if (lines[i].args[4] & TMFT_INTANGIBLEBOTTOM)
					ffloorflags |= FOF_PLATFORM;
				if (lines[i].args[4] & TMFT_DONTBLOCKPLAYER)
					ffloorflags &= ~FOF_BLOCKPLAYER;
				if (lines[i].args[4] & TMFT_DONTBLOCKOTHERS)
					ffloorflags &= ~FOF_BLOCKOTHERS;

				//Cutting options
				if (ffloorflags & FOF_RENDERALL)
				{
					//If translucent or player can enter it, cut inner walls
					if ((lines[i].args[1] < 255) || (lines[i].args[4] & TMFT_VISIBLEFROMINSIDE))
						ffloorflags |= FOF_CUTEXTRA|FOF_EXTRA;
					else
						ffloorflags |= FOF_CUTLEVEL;
				}

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, lines[i].args[0], lines[i].args[5] << FRACBITS, ceilingtop, ceilingbottom, !!(lines[i].args[6] & TMFR_REVERSE), !!(lines[i].args[6] & TMFR_SPINDASH));
				break;
			}
			case 200: // Light block
				ffloorflags = FOF_EXISTS|FOF_CUTSPRITES;
				if (!lines[i].args[1])
					ffloorflags |= FOF_DOUBLESHADOW;
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				break;

			case 202: // Fog
				ffloorflags = FOF_EXISTS|FOF_RENDERALL|FOF_FOG|FOF_INVERTPLANES|FOF_INVERTSIDES|FOF_CUTEXTRA|FOF_EXTRA|FOF_DOUBLESHADOW|FOF_CUTSPRITES;
				sec = sides[*lines[i].sidenum].sector - sectors;
				// SoM: Because it's fog, check for an extra colormap and set the fog flag...
				if (sectors[sec].extra_colormap)
					sectors[sec].extra_colormap->flags = CMF_FOG;
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				break;

			case 220: //Intangible
				ffloorflags = FOF_EXISTS|FOF_RENDERALL|FOF_CUTEXTRA|FOF_EXTRA|FOF_CUTSPRITES;

				//Appearance settings
				if (lines[i].args[3] & TMFA_NOPLANES)
					ffloorflags &= ~FOF_RENDERPLANES;
				if (lines[i].args[3] & TMFA_NOSIDES)
					ffloorflags &= ~FOF_RENDERSIDES;
				if (!(lines[i].args[3] & TMFA_INSIDES))
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_BOTHPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_ALLSIDES;
				}
				if (lines[i].args[3] & TMFA_ONLYINSIDES)
				{
					if (ffloorflags & FOF_RENDERPLANES)
						ffloorflags |= FOF_INVERTPLANES;
					if (ffloorflags & FOF_RENDERSIDES)
						ffloorflags |= FOF_INVERTSIDES;
				}
				if (lines[i].args[3] & TMFA_NOSHADE)
					ffloorflags |= FOF_NOSHADE;
				if (lines[i].args[3] & TMFA_SPLAT)
					ffloorflags |= FOF_SPLAT;

				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 223: // FOF (intangible, invisible) - for combining specials in a sector
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_NOSHADE, secthinkers);
				break;

			case 250: // Mario Block
				ffloorflags = FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL|FOF_MARIO;
				if (lines[i].args[1] & TMFM_BRICK)
					ffloorflags |= FOF_GOOWATER;
				if (lines[i].args[1] & TMFM_INVISIBLE)
					ffloorflags &= ~(FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL);

				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				break;

			case 251: // A THWOMP!
			{
				UINT16 sound = (lines[i].stringargs[0]) ? get_number(lines[i].stringargs[0]) : sfx_thwomp;
				P_AddThwompThinker(lines[i].frontsector, &lines[i], lines[i].args[1] << (FRACBITS - 3), lines[i].args[2] << (FRACBITS - 3), sound);
				P_AddFakeFloorsByLine(i, 0xff, TMB_TRANSLUCENT, FOF_EXISTS|FOF_SOLID|FOF_RENDERALL|FOF_CUTLEVEL, secthinkers);
				break;
			}

			case 254: // Bustable block
			{
				UINT8 busttype = BT_REGULAR;
				ffloorbustflags_e bustflags = 0;

				ffloorflags = FOF_EXISTS|FOF_BLOCKOTHERS|FOF_RENDERALL|FOF_BUSTUP;

				//Bustable type
				switch (lines[i].args[3])
				{
					case TMFB_TOUCH:
						busttype = BT_TOUCH;
						break;
					case TMFB_SPIN:
						busttype = BT_SPINBUST;
						break;
					case TMFB_REGULAR:
						busttype = BT_REGULAR;
						break;
					case TMFB_STRONG:
						busttype = BT_STRONG;
						break;
				}

				//Flags
				if (lines[i].args[4] & TMFB_PUSHABLES)
					bustflags |= FB_PUSHABLES;
				if (lines[i].args[4] & TMFB_EXECUTOR)
					bustflags |= FB_EXECUTOR;
				if (lines[i].args[4] & TMFB_ONLYBOTTOM)
					bustflags |= FB_ONLYBOTTOM;
				if (lines[i].args[4] & TMFB_SPLAT)
					ffloorflags |= FOF_SPLAT;

				if (busttype != BT_TOUCH || bustflags & FB_ONLYBOTTOM)
					ffloorflags |= FOF_BLOCKPLAYER;

				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					ffloor_t *fflr = P_AddFakeFloor(&sectors[s], lines[i].frontsector, lines + i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
					if (!fflr)
						continue;
					fflr->bustflags = bustflags;
					fflr->busttype = busttype;
					fflr->busttag = lines[i].args[5];
				}
				break;
			}
			case 257: // Quicksand
				ffloorflags = FOF_EXISTS|FOF_QUICKSAND|FOF_RENDERALL|FOF_ALLSIDES|FOF_CUTSPRITES;
				if (!(lines[i].args[1]))
					ffloorflags |= FOF_RIPPLE;

				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					ffloor_t *fflr = P_AddFakeFloor(&sectors[s], lines[i].frontsector, lines + i, 0xff, TMB_TRANSLUCENT, ffloorflags, secthinkers);
					if (!fflr)
						continue;
					fflr->sinkspeed = abs(lines[i].args[2]) << (FRACBITS - 1);
					fflr->friction = abs(lines[i].args[3]) << (FRACBITS - 6);
				}
				break;

			case 258: // Laser block
				ffloorflags = FOF_EXISTS|FOF_RENDERALL|FOF_NOSHADE|FOF_EXTRA|FOF_CUTEXTRA|FOF_TRANSLUCENT;
				P_AddLaserThinker(lines[i].args[0], lines + i, !!(lines[i].args[3] & TMFL_NOBOSSES));
				if (lines[i].args[3] & TMFL_SPLAT)
					ffloorflags |= FOF_SPLAT;
				P_AddFakeFloorsByLine(i, lines[i].args[1], lines[i].args[2], ffloorflags, secthinkers);
				break;

			case 259: // Custom FOF
				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					ffloor_t *fflr = P_AddFakeFloor(&sectors[s], lines[i].frontsector, lines + i, lines[i].args[1], lines[i].args[2], lines[i].args[3], secthinkers);
					if (!fflr)
						continue;
					if (!udmf) // Ugly backwards compatibility stuff
					{
						if (lines[i].args[3] & FOF_QUICKSAND)
						{
							fflr->sinkspeed = abs(lines[i].dx) >> 1;
							fflr->friction = abs(lines[i].dy) >> 6;
						}
						if (lines[i].args[3] & FOF_BUSTUP)
						{
							switch (lines[i].args[4] % TMFB_ONLYBOTTOM)
							{
								case TMFB_TOUCH:
									fflr->busttype = BT_TOUCH;
									break;
								case TMFB_SPIN:
									fflr->busttype = BT_SPINBUST;
									break;
								case TMFB_REGULAR:
									fflr->busttype = BT_REGULAR;
									break;
								case TMFB_STRONG:
									fflr->busttype = BT_STRONG;
									break;
							}

							if (lines[i].args[4] & TMFB_ONLYBOTTOM)
								fflr->bustflags |= FB_ONLYBOTTOM;
							if (lines[i].flags & ML_MIDSOLID)
								fflr->bustflags |= FB_PUSHABLES;
							if (lines[i].flags & ML_WRAPMIDTEX)
							{
								fflr->bustflags |= FB_EXECUTOR;
								fflr->busttag = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
							}
						}
					}
				}
				break;

			case 260: // GZDoom-like 3D Floor.
				{
					UINT8 dtype = lines[i].args[1] & 3;
					UINT8 dflags1 = lines[i].args[1] - dtype;
					UINT8 dflags2 = lines[i].args[2];
					UINT8 dopacity = lines[i].args[3];
					boolean isfog = false;

					if (dtype == 0)
						dtype = 1;

					ffloorflags = FOF_EXISTS;

					if (dflags2 & 1) ffloorflags |= FOF_NOSHADE; // Disable light effects (Means no shadowcast)
					if (dflags2 & 2) ffloorflags |= FOF_DOUBLESHADOW; // Restrict light inside (Means doubleshadow)
					if (dflags2 & 4) isfog = true; // Fog effect (Explicitly render like a fog block)

					if (dflags1 & 4) ffloorflags |= FOF_BOTHPLANES|FOF_ALLSIDES; // Render-inside
					if (dflags1 & 16) ffloorflags |= FOF_INVERTSIDES|FOF_INVERTPLANES; // Invert visibility rules

					// Fog block
					if (isfog)
						ffloorflags |= FOF_RENDERALL|FOF_CUTEXTRA|FOF_CUTSPRITES|FOF_BOTHPLANES|FOF_EXTRA|FOF_FOG|FOF_INVERTPLANES|FOF_ALLSIDES|FOF_INVERTSIDES;
					else
					{
						ffloorflags |= FOF_RENDERALL;

						// Solid
						if (dtype == 1)
							ffloorflags |= FOF_SOLID|FOF_CUTLEVEL;
						// Water
						else if (dtype == 2)
							ffloorflags |= FOF_SWIMMABLE|FOF_CUTEXTRA|FOF_CUTSPRITES|FOF_EXTRA|FOF_RIPPLE;
						// Intangible
						else if (dtype == 3)
							ffloorflags |= FOF_CUTEXTRA|FOF_CUTSPRITES|FOF_EXTRA;
					}

					// Non-opaque
					if (dopacity < 255)
					{
						// Invisible
						if (dopacity == 0)
						{
							// True invisible
							if (ffloorflags & FOF_NOSHADE)
								ffloorflags &= ~(FOF_RENDERALL|FOF_CUTEXTRA|FOF_CUTSPRITES|FOF_EXTRA|FOF_BOTHPLANES|FOF_ALLSIDES|FOF_CUTLEVEL);
							// Shadow block
							else
							{
								ffloorflags |= FOF_CUTSPRITES;
								ffloorflags &= ~(FOF_RENDERALL|FOF_CUTEXTRA|FOF_EXTRA|FOF_BOTHPLANES|FOF_ALLSIDES|FOF_CUTLEVEL);
							}
						}
						else
						{
							ffloorflags |= FOF_TRANSLUCENT|FOF_CUTEXTRA|FOF_EXTRA;
							ffloorflags &= ~FOF_CUTLEVEL;
						}
					}

					P_AddFakeFloorsByLine(i, dopacity, TMB_TRANSLUCENT, ffloorflags, secthinkers);
				}
				break;

			case 300: // Trigger linedef executor
			case 303: // Count rings
			case 305: // Character ability
			case 314: // Pushable linedef executors (count # of pushables)
			case 317: // Condition set trigger
			case 319: // Unlockable trigger
			case 331: // Player skin
			case 334: // Object dye
			case 337: // Emerald check
				if (udmf)
					break;
				if (lines[i].args[0] > TMT_EACHTIMEMASK)
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMT_EACHTIMEENTERANDEXIT);
				break;

			case 308: // Race-only linedef executor. Triggers once.
				if (udmf)
					break;
				if (!P_CheckGametypeRules(lines[i].args[2], (UINT32)lines[i].args[1]))
				{
					lines[i].special = 0;
					break;
				}
				if (lines[i].args[0] > TMT_EACHTIMEMASK)
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMT_EACHTIMEENTERANDEXIT);
				break;

			// Linedef executor triggers for CTF teams.
			case 309:
				if (udmf)
					break;
				if (!(gametyperules & GTR_TEAMS))
				{
					lines[i].special = 0;
					break;
				}
				if (lines[i].args[0] > TMT_EACHTIMEMASK)
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMT_EACHTIMEENTERANDEXIT);
				break;

			// No More Enemies Linedef Exec
			case 313:
				if (udmf)
					break;
				P_AddNoEnemiesThinker(&lines[i]);
				break;

			// Trigger on X calls
			case 321:
				if (udmf)
					break;
				lines[i].callcount = (lines[i].args[2] && lines[i].args[3] > 0) ? lines[i].args[3] : lines[i].args[1]; // optional "starting" count
				if (lines[i].args[0] > TMXT_EACHTIMEMASK) // Each time
					P_AddEachTimeThinker(&lines[i], lines[i].args[0] == TMXT_EACHTIMEENTERANDEXIT);
				break;

			case 449: // Enable bosses with parameter
			{
				INT32 bossid = lines[i].args[0];
				if (bossid & ~15) // if any bits other than first 16 are set
				{
					CONS_Alert(CONS_WARNING,
						M_GetText("Boss enable linedef has an invalid boss ID (%d).\nConsider changing it or removing it entirely.\n"),
						bossid);
					break;
				}
				if (!(lines[i].args[1]))
				{
					bossdisabled |= (1<<bossid); // gotta disable in the first place to enable
					CONS_Debug(DBG_GAMELOGIC, "Line type 449 spawn effect: bossid disabled = %d", bossid);
				}
				break;
			}

			case 600: // Copy light level to tagged sector's planes
				sec = sides[*lines[i].sidenum].sector-sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					if (lines[i].args[1] != TMP_CEILING)
						sectors[s].floorlightsec = (INT32)sec;
					if (lines[i].args[1] != TMP_FLOOR)
						sectors[s].ceilinglightsec = (INT32)sec;
				}
				break;

			case 602: // Adjustable pulsating light
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_SpawnAdjustableGlowingLight(&sectors[s], lines[i].args[2],
						lines[i].args[3] ? sectors[s].lightlevel : lines[i].args[4], lines[i].args[1]);
				break;

			case 603: // Adjustable flickering light
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_SpawnAdjustableFireFlicker(&sectors[s], lines[i].args[2],
						lines[i].args[3] ? sectors[s].lightlevel : lines[i].args[4], lines[i].args[1]);
				break;

			case 604: // Adjustable Blinking Light
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(lines[i].args[0], s)
					P_SpawnAdjustableStrobeFlash(&sectors[s], lines[i].args[3],
						(lines[i].args[4] & TMB_USETARGET) ? sectors[s].lightlevel : lines[i].args[5],
						lines[i].args[1], lines[i].args[2], lines[i].args[4] & TMB_SYNC);
				break;

			case 606: // HACK! Copy colormaps. Just plain colormaps.
				TAG_ITER_SECTORS(lines[i].args[0], s)
				{
					extracolormap_t *exc;

					if (sectors[s].colormap_protected)
						continue;

					if (!udmf)
						exc = sides[lines[i].sidenum[0]].colormap_data;
					else
					{
						if (!lines[i].args[1])
							exc = lines[i].frontsector->extra_colormap;
						else
						{
							INT32 sourcesec = Tag_Iterate_Sectors(lines[i].args[1], 0);
							if (sourcesec == -1)
							{
								CONS_Debug(DBG_GAMELOGIC, "Line type 606: Can't find sector with source colormap (tag %d)!\n", lines[i].args[1]);
								return;
							}
							exc = sectors[sourcesec].extra_colormap;
						}
					}
					sectors[s].extra_colormap = sectors[s].spawn_extra_colormap = exc;
				}
				break;

			case 2004: // Bot Controller.
				K_UpdateBotControllers(lines[i].args);
				break;

			default:
				break;
		}
	}

	// And another round, this time with all FOFs already created
	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			INT32 s;
			INT32 l;

			case 74: // Make FOF bustable
			{
				UINT8 busttype = BT_REGULAR;
				ffloorbustflags_e bustflags = 0;

				if (!udmf)
					break;

				switch (lines[i].args[1])
				{
					case TMFB_TOUCH:
						busttype = BT_TOUCH;
						break;
					case TMFB_SPIN:
						busttype = BT_SPINBUST;
						break;
					case TMFB_REGULAR:
						busttype = BT_REGULAR;
						break;
					case TMFB_STRONG:
						busttype = BT_STRONG;
						break;
				}

				if (lines[i].args[2] & TMFB_PUSHABLES)
					bustflags |= FB_PUSHABLES;
				if (lines[i].args[2] & TMFB_EXECUTOR)
					bustflags |= FB_EXECUTOR;
				if (lines[i].args[2] & TMFB_ONLYBOTTOM)
					bustflags |= FB_ONLYBOTTOM;

				TAG_ITER_LINES(lines[i].args[0], l)
				{
					if (lines[l].special < 100 || lines[l].special >= 300)
						continue;

					TAG_ITER_SECTORS(lines[l].args[0], s)
					{
						ffloor_t *rover;

						for (rover = sectors[s].ffloors; rover; rover = rover->next)
						{
							if (rover->master != lines + l)
								continue;

							rover->fofflags |= FOF_BUSTUP;
							rover->spawnflags |= FOF_BUSTUP;
							rover->bustflags = bustflags;
							rover->busttype = busttype;
							rover->busttag = lines[i].args[3];
							CheckForBustableBlocks = true;
							break;
						}
					}
				}
				break;
			}

			case 75: // Make FOF quicksand
			{
				if (!udmf)
					break;
				TAG_ITER_LINES(lines[i].args[0], l)
				{
					if (lines[l].special < 100 || lines[l].special >= 300)
						continue;

					TAG_ITER_SECTORS(lines[l].args[0], s)
					{
						ffloor_t *rover;

						for (rover = sectors[s].ffloors; rover; rover = rover->next)
						{
							if (rover->master != lines + l)
								continue;

							rover->fofflags |= FOF_QUICKSAND;
							rover->spawnflags |= FOF_QUICKSAND;
							rover->sinkspeed = abs(lines[i].args[1]) << (FRACBITS - 1);
							rover->friction = abs(lines[i].args[2]) << (FRACBITS - 6);
							CheckForQuicksand = true;
							break;
						}
					}
				}
				break;
			}

			case 76: // Make FOF bouncy
			{
				if (udmf)
				{
					TAG_ITER_LINES(lines[i].args[0], l)
						P_MakeFOFBouncy(lines + i, lines + l);
				}
				else
				{
					TAG_ITER_SECTORS(lines[i].args[0], s)
						for (j = 0; (unsigned)j < sectors[s].linecount; j++)
							P_MakeFOFBouncy(lines + i, sectors[s].lines[j]);
				}
				break;
			}

			default:
				break;
		}
	}

	// Allocate each list
	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].thinkers)
			Z_Free(secthinkers[i].thinkers);

	Z_Free(secthinkers);
}

/** Fuck polyobjects
  */
void P_SpawnSpecialsThatRequireObjects(boolean fromnetsave)
{
	size_t i;

	// haleyjd 02/20/06: spawn polyobjects
	Polyobj_InitLevel();

	for (i = 0; i < numlines; i++)
	{
		if (P_IsLineDisabled(&lines[i]))
		{
			/* remove the special so it can't even be found during the level */
			lines[i].special = 0;
		}

		switch (lines[i].special)
		{
			case 30: // Polyobj_Flag
				PolyFlag(lines[i].args);
				break;

			case 31: // Polyobj_Displace
				PolyDisplace(lines[i].args, &lines[i]);
				break;

			case 32: // Polyobj_RotDisplace
				PolyRotDisplace(lines[i].args, &lines[i]);
				break;

			case 80: // Raise tagged things by type to this FOF
				{
					P_RaiseTaggedThingsToFakeFloor(
							lines[i].args[0],
							lines[i].args[1],
							lines[i].frontsector
					);
				}
				break;
		}
	}

	if (!fromnetsave)
		P_RunLevelLoadExecutors();
}

/** Adds 3Dfloors as appropriate based on a common control linedef.
  *
  * \param line        Control linedef to use.
  * \param alpha       Alpha value (0-255).
  * \param blendmode   Blending mode.
  * \param ffloorflags 3Dfloor flags to use.
  * \param secthkiners Lists of thinkers sorted by sector. May be NULL.
  * \sa P_SpawnSpecials, P_AddFakeFloor
  * \author Graue <graue@oceanbase.org>
  */
static void P_AddFakeFloorsByLine(size_t line, INT32 alpha, UINT8 blendmode, ffloortype_e ffloorflags, thinkerlist_t *secthinkers)
{
	INT32 s;
	mtag_t tag = lines[line].args[0];
	size_t sec = sides[*lines[line].sidenum].sector-sectors;

	line_t* li = lines + line;
	TAG_ITER_SECTORS(tag, s)
		P_AddFakeFloor(&sectors[s], &sectors[sec], li, alpha, blendmode, ffloorflags, secthinkers);
}

/*
 SoM: 3/8/2000: General scrolling functions.
 T_Scroll,
 Add_Scroller,
 Add_WallScroller,
 P_SpawnScrollers
*/

// helper function for T_Scroll
static void P_DoScrollMove(mobj_t *thing, fixed_t dx, fixed_t dy, INT32 exclusive)
{
	fixed_t fuckaj = 0; // Nov 05 14:12:08 <+MonsterIestyn> I've heard of explicitly defined variables but this is ridiculous
	if (thing->player)
	{
		if (!(dx | dy))
		{
			thing->player->cmomx = 0;
			thing->player->cmomy = 0;
		}
		else
		{
			thing->player->cmomx += dx;
			thing->player->cmomy += dy;
			thing->player->cmomx = FixedMul(thing->player->cmomx, ORIG_FRICTION);
			thing->player->cmomy = FixedMul(thing->player->cmomy, ORIG_FRICTION);
		}
	}

	if (thing->friction != ORIG_FRICTION)
		fuckaj = thing->friction;

	if (fuckaj) {
		// refactor thrust for new friction
		dx = FixedDiv(dx, CARRYFACTOR);
		dy = FixedDiv(dy, CARRYFACTOR);

		dx = FixedMul(dx, FRACUNIT-fuckaj);
		dy = FixedMul(dy, FRACUNIT-fuckaj);
	}

	thing->momx += dx;
	thing->momy += dy;

	if (exclusive)
		thing->eflags |= MFE_PUSHED;
}

/** Processes an active scroller.
  * This function, with the help of r_plane.c and r_bsp.c, supports generalized
  * scrolling floors and walls, with optional mobj-carrying properties, e.g.
  * conveyor belts, rivers, etc. A linedef with a special type affects all
  * tagged sectors the same way, by creating scrolling and/or object-carrying
  * properties. Multiple linedefs may be used on the same sector and are
  * cumulative, although the special case of scrolling a floor and carrying
  * things on it requires only one linedef.
  *
  * The linedef's direction determines the scrolling direction, and the
  * linedef's length determines the scrolling speed. This was designed so an
  * edge around a sector can be used to control the direction of the sector's
  * scrolling, which is usually what is desired.
  *
  * \param s Thinker for the scroller to process.
  * \todo Split up into multiple functions.
  * \todo Use attached lists to make ::sc_carry_ceiling case faster and
  *       cleaner.
  * \sa Add_Scroller, Add_WallScroller, P_SpawnScrollers
  * \author Steven McGranahan
  * \author Graue <graue@oceanbase.org>
  */
void T_Scroll(scroll_t *s)
{
	fixed_t dx = s->dx, dy = s->dy;
	boolean is3dblock = false;

	if (s->control != -1)
	{ // compute scroll amounts based on a sector's height changes
		fixed_t height = sectors[s->control].floorheight +
			sectors[s->control].ceilingheight;
		fixed_t delta = height - s->last_height;
		s->last_height = height;
		dx = FixedMul(dx, delta);
		dy = FixedMul(dy, delta);
	}

	if (s->accel)
	{
		s->vdx = dx += s->vdx;
		s->vdy = dy += s->vdy;
	}

//	if (!(dx | dy)) // no-op if both (x,y) offsets 0
//		return;

	switch (s->type)
	{
		side_t *side;
		sector_t *sec;
		fixed_t height;
		msecnode_t *node;
		mobj_t *thing;
		line_t *line;
		size_t i;
		INT32 sect;
		ffloor_t *rover;

		case sc_side: // scroll wall texture
			side = sides + s->affectee;
			side->textureoffset += dx;
			side->rowoffset += dy;
			break;

		case sc_floor: // scroll floor texture
			sec = sectors + s->affectee;
			sec->floor_xoffs += dx;
			sec->floor_yoffs += dy;
			break;

		case sc_ceiling: // scroll ceiling texture
			sec = sectors + s->affectee;
			sec->ceiling_xoffs += dx;
			sec->ceiling_yoffs += dy;
			break;

		case sc_carry:
			sec = sectors + s->affectee;
			height = sec->floorheight;

			// sec is the control sector, find the real sector(s) to use
			for (i = 0; i < sec->linecount; i++)
			{
				line = sec->lines[i];

				if (line->special < 100 || line->special >= 300)
					is3dblock = false;
				else
					is3dblock = true;

				if (!is3dblock)
					continue;

				TAG_ITER_SECTORS(line->args[0], sect)
				{
					sector_t *psec;
					psec = sectors + sect;

					// Find the FOF corresponding to the control linedef
					for (rover = psec->ffloors; rover; rover = rover->next)
					{
						if (rover->master == sec->lines[i])
							break;
					}

					if (!rover) // This should be impossible, but don't complain if it is the case somehow
						continue;

					if (!(rover->fofflags & FOF_EXISTS)) // If the FOF does not "exist", we pretend that nobody's there
						continue;

					for (node = psec->touching_thinglist; node; node = node->m_thinglist_next)
					{
						thing = node->m_thing;

						if (thing->eflags & MFE_PUSHED) // Already pushed this tic by an exclusive pusher.
							continue;

						height = P_GetSpecialBottomZ(thing, sec, psec);

						if (!(thing->flags & MF_NOCLIP) && // Thing must be clipped
							(!(thing->flags & MF_NOGRAVITY || thing->z+thing->height != height))) // Thing must a) be non-floating and have z+height == height
						{
							// Move objects only if on floor
							// non-floating, and clipped.
							P_DoScrollMove(thing, dx, dy, s->exclusive);
						}
					} // end of for loop through touching_thinglist
				} // end of loop through sectors
			}

			if (!is3dblock)
			{
				for (node = sec->touching_thinglist; node; node = node->m_thinglist_next)
				{
					thing = node->m_thing;

					if (thing->eflags & MFE_PUSHED)
						continue;

					height = P_GetSpecialBottomZ(thing, sec, sec);

					if (!(thing->flags & MF_NOCLIP) &&
						(!(thing->flags & MF_NOGRAVITY || thing->z > height)))
					{
						// Move objects only if on floor or underwater,
						// non-floating, and clipped.
						P_DoScrollMove(thing, dx, dy, s->exclusive);
					}
				}
			}
			break;

		case sc_carry_ceiling: // carry on ceiling (FOF scrolling)
			sec = sectors + s->affectee;
			height = sec->ceilingheight;

			// sec is the control sector, find the real sector(s) to use
			for (i = 0; i < sec->linecount; i++)
			{
				line = sec->lines[i];
				if (line->special < 100 || line->special >= 300)
					is3dblock = false;
				else
					is3dblock = true;

				if (!is3dblock)
					continue;
				TAG_ITER_SECTORS(line->args[0], sect)
				{
					sector_t *psec;
					psec = sectors + sect;

					// Find the FOF corresponding to the control linedef
					for (rover = psec->ffloors; rover; rover = rover->next)
					{
						if (rover->master == sec->lines[i])
							break;
					}

					if (!rover) // This should be impossible, but don't complain if it is the case somehow
						continue;

					if (!(rover->fofflags & FOF_EXISTS)) // If the FOF does not "exist", we pretend that nobody's there
						continue;

					for (node = psec->touching_thinglist; node; node = node->m_thinglist_next)
					{
						thing = node->m_thing;

						if (thing->eflags & MFE_PUSHED)
							continue;

						height = P_GetSpecialTopZ(thing, sec, psec);

						if (!(thing->flags & MF_NOCLIP) && // Thing must be clipped
							(!(thing->flags & MF_NOGRAVITY || thing->z != height))) // Thing must a) be non-floating and have z == height
						{
							// Move objects only if on floor or underwater,
							// non-floating, and clipped.
							P_DoScrollMove(thing, dx, dy, s->exclusive);
						}
					} // end of for loop through touching_thinglist
				} // end of loop through sectors
			}

			if (!is3dblock)
			{
				for (node = sec->touching_thinglist; node; node = node->m_thinglist_next)
				{
					thing = node->m_thing;

					if (thing->eflags & MFE_PUSHED)
						continue;

					height = P_GetSpecialTopZ(thing, sec, sec);

					if (!(thing->flags & MF_NOCLIP) &&
						(!(thing->flags & MF_NOGRAVITY || thing->z+thing->height < height)))
					{
						// Move objects only if on floor or underwater,
						// non-floating, and clipped.
						P_DoScrollMove(thing, dx, dy, s->exclusive);
					}
				}
			}
			break; // end of sc_carry_ceiling
	} // end of switch
}

/** Adds a generalized scroller to the thinker list.
  *
  * \param type     The enumerated type of scrolling.
  * \param dx       x speed of scrolling or its acceleration.
  * \param dy       y speed of scrolling or its acceleration.
  * \param control  Sector whose heights control this scroller's effect
  *                 remotely, or -1 if there is no control sector.
  * \param affectee Index of the affected object, sector or sidedef.
  * \param accel    Nonzero for an accelerative effect.
  * \sa Add_WallScroller, P_SpawnScrollers, T_Scroll
  */
static void Add_Scroller(INT32 type, fixed_t dx, fixed_t dy, INT32 control, INT32 affectee, INT32 accel, INT32 exclusive)
{
	scroll_t *s = Z_LevelPoolCalloc(sizeof (*s));
	s->thinker.alloctype = TAT_LEVELPOOL;
	s->thinker.size = sizeof (*s);
	s->thinker.function.acp1 = (actionf_p1)T_Scroll;
	s->type = type;
	s->dx = dx;
	s->dy = dy;
	s->accel = accel;
	s->exclusive = exclusive;
	s->vdx = s->vdy = 0;
	s->control = control;
	if (s->control != -1)
		s->last_height = sectors[control].floorheight + sectors[control].ceilingheight;
	s->affectee = affectee;
	if (type == sc_carry || type == sc_carry_ceiling)
		sectors[affectee].specialflags |= SSF_CONVEYOR;
	P_AddThinker(THINK_MAIN, &s->thinker);

	// interpolation
	switch (type)
	{
		case sc_side:
			R_CreateInterpolator_SideScroll(&s->thinker, &sides[affectee]);
			break;
		case sc_floor:
			R_CreateInterpolator_SectorScroll(&s->thinker, &sectors[affectee], false);
			break;
		case sc_ceiling:
			R_CreateInterpolator_SectorScroll(&s->thinker, &sectors[affectee], true);
			break;
		default:
			break;
	}
}

static void P_SpawnPlaneScroller(line_t *l, fixed_t dx, fixed_t dy, INT32 control, INT32 affectee, INT32 accel, INT32 exclusive)
{
	if (l->args[1] != TMP_CEILING)
	{
		if (l->args[2] != TMS_SCROLLONLY)
			Add_Scroller(sc_carry, FixedMul(dx, CARRYFACTOR), FixedMul(dy, CARRYFACTOR), control, affectee, accel, exclusive);
		if (l->args[2] != TMS_CARRYONLY)
			Add_Scroller(sc_floor, -dx, dy, control, affectee, accel, exclusive);
	}
	if (l->args[1] != TMP_FLOOR)
	{
		if (l->args[2] != TMS_SCROLLONLY)
			Add_Scroller(sc_carry_ceiling, FixedMul(dx, CARRYFACTOR), FixedMul(dy, CARRYFACTOR), control, affectee, accel, exclusive);
		if (l->args[2] != TMS_CARRYONLY)
			Add_Scroller(sc_ceiling, -dx, dy, control, affectee, accel, exclusive);
	}
}

/** Initializes the scrollers.
  *
  * \todo Get rid of all the magic numbers.
  * \sa P_SpawnSpecials, Add_Scroller, Add_WallScroller
  */
static void P_SpawnScrollers(void)
{
	size_t i;
	line_t *l = lines;

	for (i = 0; i < numlines; i++, l++)
	{
		INT32 control = -1, accel = 0; // no control sector or acceleration
		register INT32 s;

		if (l->special == 502 || l->special == 510)
		{
			if ((l->args[4] & TMST_TYPEMASK) != TMST_REGULAR)
				control = (INT32)(sides[*l->sidenum].sector - sectors);
			if ((l->args[4] & TMST_TYPEMASK) == TMST_ACCELERATIVE)
				accel = 1;
		}

		//
		// FIXME: UDMFify front+back scrollers
		// type 507
		//

		switch (l->special)
		{
			case 510: // plane scroller
			{
				fixed_t length = R_PointToDist2(l->v2->x, l->v2->y, l->v1->x, l->v1->y);
				fixed_t speed = l->args[3] << FRACBITS;
				fixed_t dx = FixedMul(FixedDiv(l->dx, length), speed) >> SCROLL_SHIFT;
				fixed_t dy = FixedMul(FixedDiv(l->dy, length), speed) >> SCROLL_SHIFT;

				if (l->args[0] == 0)
					P_SpawnPlaneScroller(l, dx, dy, control, (INT32)(l->frontsector - sectors), accel, !(l->args[4] & TMST_NONEXCLUSIVE));
				else
				{
					TAG_ITER_SECTORS(l->args[0], s)
						P_SpawnPlaneScroller(l, dx, dy, control, s, accel, !(l->args[4] & TMST_NONEXCLUSIVE));
				}
				break;
			}

			// scroll wall according to linedef
			// (same direction and speed as scrolling floors)
			case 502:
			{
				TAG_ITER_LINES(l->args[0], s)
					if (s != (INT32)i)
					{
						if (l->args[1] != TMSD_BACK)
							Add_Scroller(sc_side, l->args[2] << (FRACBITS - SCROLL_SHIFT), l->args[3] << (FRACBITS - SCROLL_SHIFT), control, lines[s].sidenum[0], accel, 0);
						if (l->args[1] != TMSD_FRONT && lines[s].sidenum[1] != 0xffff)
							Add_Scroller(sc_side, l->args[2] << (FRACBITS - SCROLL_SHIFT), l->args[3] << (FRACBITS - SCROLL_SHIFT), control, lines[s].sidenum[1], accel, 0);
					}
				break;
			}

			case 500:
			{
				if (l->args[0] != TMSD_BACK)
				{
					Add_Scroller(sc_side, -l->args[1] << (FRACBITS - SCROLL_SHIFT), l->args[2] << (FRACBITS - SCROLL_SHIFT), -1, l->sidenum[0], accel, 0);
				}

				if (l->args[0] != TMSD_FRONT)
				{
					if (l->sidenum[1] != 0xffff)
					{
						Add_Scroller(sc_side, -l->args[1] << (FRACBITS - SCROLL_SHIFT), l->args[2] << (FRACBITS - SCROLL_SHIFT), -1, l->sidenum[1], accel, 0);
					}
					else
					{
						CONS_Debug(DBG_GAMELOGIC, "Line special 500 (line #%s) missing back side!\n", sizeu1(i));
					}
				}
				break;
			}
		}
	}
}

/** Adds master appear/disappear thinker.
  *
  * \param appeartime		tics to be existent
  * \param disappeartime	tics to be nonexistent
  * \param sector			pointer to control sector
  */
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline)
{
	disappear_t *d = Z_LevelPoolCalloc(sizeof (*d));
	d->thinker.alloctype = TAT_LEVELPOOL;
	d->thinker.size = sizeof (*d);

	d->thinker.function.acp1 = (actionf_p1)T_Disappear;
	d->appeartime = appeartime;
	d->disappeartime = disappeartime;
	d->offset = offset;
	d->affectee = line;
	d->sourceline = sourceline;
	d->exists = true;
	d->timer = 1;

	P_AddThinker(THINK_MAIN, &d->thinker);
}

/** Makes a FOF appear/disappear
  *
  * \param d Disappear thinker.
  * \sa Add_MasterDisappearer
  */
void T_Disappear(disappear_t *d)
{
	if (d->offset && !d->exists)
	{
		d->offset--;
		return;
	}

	if (--d->timer <= 0)
	{
		ffloor_t *rover;
		register INT32 s;
		mtag_t afftag = lines[d->affectee].args[0];

		TAG_ITER_SECTORS(afftag, s)
		{
			for (rover = sectors[s].ffloors; rover; rover = rover->next)
			{
				if (rover->master != &lines[d->affectee])
					continue;

				if (d->exists)
					rover->fofflags &= ~FOF_EXISTS;
				else
				{
					rover->fofflags |= FOF_EXISTS;

					if (!(lines[d->sourceline].args[5]))
					{
						sectors[s].soundorg.z = P_GetFFloorTopZAt(rover, sectors[s].soundorg.x, sectors[s].soundorg.y);
						S_StartSound(&sectors[s].soundorg, sfx_appear);
					}
				}
			}
			sectors[s].moved = true;
			P_RecalcPrecipInSector(&sectors[s]);
		}

		if (d->exists)
		{
			d->timer = d->disappeartime;
			d->exists = false;
		}
		else
		{
			d->timer = d->appeartime;
			d->exists = true;
		}
	}
}

/** Removes fadingdata from FOF control sector
 *
 * \param line	line to search for target faders
 * \param data	pointer to set new fadingdata to. Can be NULL to erase.
 */
static void P_ResetFakeFloorFader(ffloor_t *rover, fade_t *data, boolean finalize)
{
	fade_t *fadingdata = (fade_t *)rover->fadingdata;
	// find any existing thinkers and remove them, then replace with new data
	if (fadingdata != data)
	{
		if (fadingdata)
		{
			if (finalize)
				P_FadeFakeFloor(rover,
					fadingdata->sourcevalue,
					fadingdata->alpha >= fadingdata->destvalue ?
						fadingdata->alpha - 1 : // trigger fade-out finish
						fadingdata->alpha + 1, // trigger fade-in finish
					0,
					fadingdata->ticbased,
					&fadingdata->timer,
					fadingdata->doexists,
					fadingdata->dotranslucent,
					fadingdata->dolighting,
					fadingdata->docolormap,
					fadingdata->docollision,
					fadingdata->doghostfade,
					fadingdata->exactalpha);
			rover->alpha = fadingdata->alpha;

			if (fadingdata->dolighting)
				P_RemoveLighting(&sectors[rover->secnum]);

			if (fadingdata->docolormap)
				P_ResetColormapFader(&sectors[rover->secnum]);

			P_RemoveThinker(&fadingdata->thinker);
		}

		rover->fadingdata = data;
	}
}

static boolean P_FadeFakeFloor(ffloor_t *rover, INT16 sourcevalue, INT16 destvalue, INT16 speed, boolean ticbased, INT32 *timer,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha)
{
	boolean stillfading = false;
	INT32 alpha;
	fade_t *fadingdata = (fade_t *)rover->fadingdata;
	(void)docolormap; // *shrug* maybe we can use this in the future. For now, let's be consistent with our other function params

	if (rover->master->special == 258) // Laser block
		return false;

	// If fading an invisible FOF whose render flags we did not yet set, initialize its alpha to 0
	if (dotranslucent &&
		(rover->spawnflags & FOF_NOSHADE) && // do not include light blocks, which don't set FOF_NOSHADE
		!(rover->fofflags & FOF_FOG) && // do not include fog
		!(rover->spawnflags & FOF_RENDERSIDES) &&
		!(rover->spawnflags & FOF_RENDERPLANES) &&
		!(rover->fofflags & FOF_RENDERALL))
		rover->alpha = 0;

	if (fadingdata)
		alpha = fadingdata->alpha;
	else
		alpha = rover->alpha;

	// routines specific to fade in and fade out
	if (!ticbased && alpha == destvalue)
		return stillfading;
	else if (alpha > destvalue) // fade out
	{
		// finish fading out
		if (speed < 1 || (!ticbased && alpha - speed <= destvalue + speed) ||
			(ticbased && (--(*timer) <= 0 || alpha <= destvalue)))
		{
			alpha = destvalue;

			if (docollision)
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags &= ~FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags &= ~FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags &= ~FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags &= ~FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags &= ~FOF_MARIO;
			}
		}
		else // continue fading out
		{
			if (!ticbased)
				alpha -= speed;
			else
			{
				INT16 delta = abs(destvalue - sourcevalue);
				fixed_t factor = min(FixedDiv(speed - (*timer), speed), 1*FRACUNIT);
				alpha = max(min(alpha, sourcevalue - (INT16)FixedMul(delta, factor)), destvalue);
			}
			stillfading = true;
		}
	}
	else // fade in
	{
		// finish fading in
		if (speed < 1 || (!ticbased && alpha + speed >= destvalue - speed) ||
			(ticbased && (--(*timer) <= 0 || alpha >= destvalue)))
		{
			alpha = destvalue;

			if (docollision)
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags |= FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags |= FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags |= FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags |= FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags |= FOF_MARIO;
			}
		}
		else // continue fading in
		{
			if (!ticbased)
				alpha += speed;
			else
			{
				INT16 delta = abs(destvalue - sourcevalue);
				fixed_t factor = min(FixedDiv(speed - (*timer), speed), 1*FRACUNIT);
				alpha = min(max(alpha, sourcevalue + (INT16)FixedMul(delta, factor)), destvalue);
			}
			stillfading = true;
		}
	}

	// routines common to both fade in and fade out
	if (!stillfading)
	{
		if (doexists && !(rover->spawnflags & FOF_BUSTUP))
		{
			if (alpha <= 0)
				rover->fofflags &= ~FOF_EXISTS;
			else
				rover->fofflags |= FOF_EXISTS;

			// Re-render lighting at end of fade
			if (dolighting && !(rover->spawnflags & FOF_NOSHADE) && !(rover->fofflags & FOF_EXISTS))
				rover->target->moved = true;
		}

		if (dotranslucent && !(rover->fofflags & FOF_FOG))
		{
			if (alpha >= 255)
			{
				if (!(rover->fofflags & FOF_CUTSOLIDS) &&
					(rover->spawnflags & FOF_CUTSOLIDS))
				{
					rover->fofflags |= FOF_CUTSOLIDS;
					rover->target->moved = true;
				}

				rover->fofflags &= ~FOF_TRANSLUCENT;
			}
			else
			{
				rover->fofflags |= FOF_TRANSLUCENT;

				if ((rover->fofflags & FOF_CUTSOLIDS) &&
					(rover->spawnflags & FOF_CUTSOLIDS))
				{
					rover->fofflags &= ~FOF_CUTSOLIDS;
					rover->target->moved = true;
				}
			}

			if ((rover->spawnflags & FOF_NOSHADE) && // do not include light blocks, which don't set FOF_NOSHADE
				!(rover->spawnflags & FOF_RENDERSIDES) &&
				!(rover->spawnflags & FOF_RENDERPLANES))
			{
				if (rover->alpha > 1)
					rover->fofflags |= FOF_RENDERALL;
				else
					rover->fofflags &= ~FOF_RENDERALL;
			}
		}
	}
	else
	{
		if (doexists && !(rover->spawnflags & FOF_BUSTUP))
		{
			// Re-render lighting if we haven't yet set FOF_EXISTS (beginning of fade)
			if (dolighting && !(rover->spawnflags & FOF_NOSHADE) && !(rover->fofflags & FOF_EXISTS))
				rover->target->moved = true;

			rover->fofflags |= FOF_EXISTS;
		}

		if (dotranslucent && !(rover->fofflags & FOF_FOG))
		{
			rover->fofflags |= FOF_TRANSLUCENT;

			if ((rover->fofflags & FOF_CUTSOLIDS) &&
				(rover->spawnflags & FOF_CUTSOLIDS))
			{
				rover->fofflags &= ~FOF_CUTSOLIDS;
				rover->target->moved = true;
			}

			if ((rover->spawnflags & FOF_NOSHADE) && // do not include light blocks, which don't set FOF_NOSHADE
				!(rover->spawnflags & FOF_RENDERSIDES) &&
				!(rover->spawnflags & FOF_RENDERPLANES))
				rover->fofflags |= FOF_RENDERALL;
		}

		if (docollision)
		{
			if (doghostfade) // remove collision flags during fade
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags &= ~FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags &= ~FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags &= ~FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags &= ~FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags &= ~FOF_MARIO;
			}
			else // keep collision during fade
			{
				if (rover->spawnflags & FOF_SOLID)
					rover->fofflags |= FOF_SOLID;
				if (rover->spawnflags & FOF_SWIMMABLE)
					rover->fofflags |= FOF_SWIMMABLE;
				if (rover->spawnflags & FOF_QUICKSAND)
					rover->fofflags |= FOF_QUICKSAND;
				if (rover->spawnflags & FOF_BUSTUP)
					rover->fofflags |= FOF_BUSTUP;
				if (rover->spawnflags & FOF_MARIO)
					rover->fofflags |= FOF_MARIO;
			}
		}
	}

	if (!(rover->fofflags & FOF_FOG)) // don't set FOG alpha
	{
		if (!stillfading || exactalpha)
			rover->alpha = alpha;
		else // clamp fadingdata->alpha to software's alpha levels
		{
			if (alpha < 12)
				rover->alpha = destvalue < 12 ? destvalue : 0; // Don't even draw it
			else if (alpha < 38)
				rover->alpha = destvalue >= 12 && destvalue < 38 ? destvalue : 25;
			else if (alpha < 64)
				rover->alpha = destvalue >= 38 && destvalue < 64 ? destvalue : 51;
			else if (alpha < 89)
				rover->alpha = destvalue >= 64 && destvalue < 89 ? destvalue : 76;
			else if (alpha < 115)
				rover->alpha = destvalue >= 89 && destvalue < 115 ? destvalue : 102;
			else if (alpha < 140)
				rover->alpha = destvalue >= 115 && destvalue < 140 ? destvalue : 128;
			else if (alpha < 166)
				rover->alpha = destvalue >= 140 && destvalue < 166 ? destvalue : 154;
			else if (alpha < 192)
				rover->alpha = destvalue >= 166 && destvalue < 192 ? destvalue : 179;
			else if (alpha < 217)
				rover->alpha = destvalue >= 192 && destvalue < 217 ? destvalue : 204;
			else if (alpha < 243)
				rover->alpha = destvalue >= 217 && destvalue < 243 ? destvalue : 230;
			else // Opaque
				rover->alpha = destvalue >= 243 ? destvalue : 255;
		}
	}

	if (fadingdata)
		fadingdata->alpha = alpha;

	return stillfading;
}

/** Adds fake floor fader thinker.
  *
  * \param destvalue    transparency value to fade to
  * \param speed        speed to fade by
  * \param ticbased     tic-based logic, speed = duration
  * \param relative     Destvalue is relative to rover->alpha
  * \param doexists	    handle FOF_EXISTS
  * \param dotranslucent handle FOF_TRANSLUCENT
  * \param dolighting  fade FOF light
  * \param docollision handle interactive flags
  * \param doghostfade  no interactive flags during fading
  * \param exactalpha   use exact alpha values (opengl)
  */
static void P_AddFakeFloorFader(ffloor_t *rover, size_t sectornum, size_t ffloornum,
	INT16 destvalue, INT16 speed, boolean ticbased, boolean relative,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha)
{
	fade_t *d;

	// If fading an invisible FOF whose render flags we did not yet set, initialize its alpha to 0
	if (dotranslucent &&
		(rover->spawnflags & FOF_NOSHADE) && // do not include light blocks, which don't set FOF_NOSHADE
		!(rover->spawnflags & FOF_RENDERSIDES) &&
		!(rover->spawnflags & FOF_RENDERPLANES) &&
		!(rover->fofflags & FOF_RENDERALL))
		rover->alpha = 0;

	// already equal, nothing to do
	if (rover->alpha == max(0, min(255, relative ? rover->alpha + destvalue : destvalue)))
		return;

	d = Z_LevelPoolCalloc(sizeof (*d));
	d->thinker.alloctype = TAT_LEVELPOOL;
	d->thinker.size = sizeof (*d);

	d->thinker.function.acp1 = (actionf_p1)T_Fade;
	d->rover = rover;
	d->sectornum = (UINT32)sectornum;
	d->ffloornum = (UINT32)ffloornum;

	d->alpha = d->sourcevalue = rover->alpha;
	d->destvalue = max(0, min(255, relative ? rover->alpha + destvalue : destvalue)); // rover->alpha is 0-255

	if (ticbased)
	{
		d->ticbased = true;
		d->timer = d->speed = abs(speed); // use d->speed as total duration
	}
	else
	{
		d->ticbased = false;
		d->speed = max(1, speed); // minimum speed 1/tic // if speed < 1, alpha is set immediately in thinker
		d->timer = -1;
	}

	d->doexists = doexists;
	d->dotranslucent = dotranslucent;
	d->dolighting = dolighting;
	d->docolormap = docolormap;
	d->docollision = docollision;
	d->doghostfade = doghostfade;
	d->exactalpha = exactalpha;

	// find any existing thinkers and remove them, then replace with new data
	P_ResetFakeFloorFader(rover, d, false);

	// Set a separate thinker for shadow fading
	if (dolighting && !(rover->fofflags & FOF_NOSHADE))
	{
		UINT16 lightdelta = abs(sectors[rover->secnum].spawn_lightlevel - rover->target->lightlevel);
		fixed_t alphapercent = min(FixedDiv(d->destvalue, rover->spawnalpha), 1*FRACUNIT); // don't make darker than spawn_lightlevel
		fixed_t adjustedlightdelta = FixedMul(lightdelta, alphapercent);

		if (rover->target->lightlevel >= sectors[rover->secnum].spawn_lightlevel) // fading out, get lighter
			d->destlightlevel = rover->target->lightlevel - adjustedlightdelta;
		else // fading in, get darker
			d->destlightlevel = rover->target->lightlevel + adjustedlightdelta;

		P_FadeLightBySector(&sectors[rover->secnum],
			d->destlightlevel,
			ticbased ? d->timer :
				FixedFloor(FixedDiv(abs(d->destvalue - d->alpha), d->speed))/FRACUNIT,
			true);
	}
	else
		d->destlightlevel = -1;

	// Set a separate thinker for colormap fading
	if (docolormap && !(rover->fofflags & FOF_NOSHADE) && sectors[rover->secnum].spawn_extra_colormap && !sectors[rover->secnum].colormap_protected)
	{
		extracolormap_t *dest_exc,
			*source_exc = sectors[rover->secnum].extra_colormap ? sectors[rover->secnum].extra_colormap : R_GetDefaultColormap();

		INT32 colordelta = R_GetRgbaA(sectors[rover->secnum].spawn_extra_colormap->rgba); // alpha is from 0
		fixed_t alphapercent = min(FixedDiv(d->destvalue, rover->spawnalpha), 1*FRACUNIT); // don't make darker than spawn_lightlevel
		fixed_t adjustedcolordelta = FixedMul(colordelta, alphapercent);
		INT32 coloralpha;

		coloralpha = adjustedcolordelta;

		dest_exc = R_CopyColormap(sectors[rover->secnum].spawn_extra_colormap, false);
		dest_exc->rgba = R_GetRgbaRGB(dest_exc->rgba) + R_PutRgbaA(coloralpha);

		if (!(d->dest_exc = R_GetColormapFromList(dest_exc)))
		{
			dest_exc->colormap = R_CreateLightTable(dest_exc);
			R_AddColormapToList(dest_exc);
			d->dest_exc = dest_exc;
		}
		else
			Z_Free(dest_exc);

		// If fading from 0, set source_exc rgb same to dest_exc
		if (!R_CheckDefaultColormap(d->dest_exc, true, false, false)
			&& R_CheckDefaultColormap(source_exc, true, false, false))
		{
			extracolormap_t *exc = R_CopyColormap(source_exc, false);
			exc->rgba = R_GetRgbaRGB(d->dest_exc->rgba) + R_PutRgbaA(R_GetRgbaA(source_exc->rgba));
			exc->fadergba = R_GetRgbaRGB(d->dest_exc->rgba) + R_PutRgbaA(R_GetRgbaA(source_exc->fadergba));

			if (!(source_exc = R_GetColormapFromList(exc)))
			{
				exc->colormap = R_CreateLightTable(exc);
				R_AddColormapToList(exc);
				source_exc = exc;
			}
			else
				Z_Free(exc);
		}

		Add_ColormapFader(&sectors[rover->secnum], source_exc, d->dest_exc, true,
			ticbased ? d->timer :
				FixedFloor(FixedDiv(abs(d->destvalue - d->alpha), d->speed))/FRACUNIT);
	}

	P_AddThinker(THINK_MAIN, &d->thinker);
}

/** Makes a FOF fade
  *
  * \param d Fade thinker.
  * \sa P_AddFakeFloorFader
  */
void T_Fade(fade_t *d)
{
	if (d->rover && !P_FadeFakeFloor(d->rover, d->sourcevalue, d->destvalue, d->speed, d->ticbased, &d->timer,
		d->doexists, d->dotranslucent, d->dolighting, d->docolormap, d->docollision, d->doghostfade, d->exactalpha))
	{
		// Finalize lighting, copypasta from P_AddFakeFloorFader
		if (d->dolighting && !(d->rover->fofflags & FOF_NOSHADE) && d->destlightlevel > -1)
			sectors[d->rover->secnum].lightlevel = d->destlightlevel;

		// Finalize colormap
		if (d->docolormap && !(d->rover->fofflags & FOF_NOSHADE) && sectors[d->rover->secnum].spawn_extra_colormap)
			sectors[d->rover->secnum].extra_colormap = d->dest_exc;

		P_RemoveFakeFloorFader(d->rover);
	}
}

static void P_ResetColormapFader(sector_t *sector)
{
	if (sector->fadecolormapdata)
	{
		// The thinker is the first member in all the action structs,
		// so just let the thinker get freed, and that will free the whole
		// structure.
		P_RemoveThinker(&((thinkerdata_t *)sector->fadecolormapdata)->thinker);
		sector->fadecolormapdata = NULL;
	}
}

static void Add_ColormapFader(sector_t *sector, extracolormap_t *source_exc, extracolormap_t *dest_exc,
	boolean ticbased, INT32 duration)
{
	fadecolormap_t *d;

	P_ResetColormapFader(sector);

	// nothing to do, set immediately
	if (!duration || R_CheckEqualColormaps(source_exc, dest_exc, true, true, true))
	{
		sector->extra_colormap = dest_exc;
		return;
	}

	d = Z_LevelPoolCalloc(sizeof (*d));
	d->thinker.alloctype = TAT_LEVELPOOL;
	d->thinker.size = sizeof (*d);
	d->thinker.function.acp1 = (actionf_p1)T_FadeColormap;
	d->sector = sector;
	d->source_exc = source_exc;
	d->dest_exc = dest_exc;

	if (ticbased)
	{
		d->ticbased = true;
		d->duration = d->timer = duration;
	}
	else
	{
		d->ticbased = false;
		d->timer = 256;
		d->duration = duration; // use as speed
	}

	sector->fadecolormapdata = d;
	P_AddThinker(THINK_MAIN, &d->thinker);
}

void T_FadeColormap(fadecolormap_t *d)
{
	if ((d->ticbased && --d->timer <= 0)
		|| (!d->ticbased && (d->timer -= d->duration) <= 0)) // d->duration used as speed decrement
	{
		d->sector->extra_colormap = d->dest_exc;
		P_ResetColormapFader(d->sector);
	}
	else
	{
		extracolormap_t *exc;
		INT32 duration = d->ticbased ? d->duration : 256;
		fixed_t factor = min(FixedDiv(duration - d->timer, duration), 1*FRACUNIT);
		INT16 cr, cg, cb, ca, fadestart, fadeend, flags;
		INT32 rgba, fadergba;

		// NULL failsafes (or intentionally set to signify default)
		if (!d->sector->extra_colormap)
			d->sector->extra_colormap = R_GetDefaultColormap();

		if (!d->source_exc)
			d->source_exc = R_GetDefaultColormap();

		if (!d->dest_exc)
			d->dest_exc = R_GetDefaultColormap();

		// For each var (rgba + fadergba + params = 11 vars), we apply
		// percentage fading: currentval = sourceval + (delta * percent of duration elapsed)
		// delta is negative when fading out (destval is lower)
		// max/min are used to ensure progressive calcs don't go backwards and to cap values to dest.

#define APPLYFADE(dest, src, cur) (\
(dest-src < 0) ? \
	max(\
		min(cur,\
			src + (INT16)FixedMul(dest-src, factor)),\
		dest)\
: (dest-src > 0) ? \
	min(\
		max(cur,\
			src + (INT16)FixedMul(dest-src, factor)),\
		dest)\
: \
	dest\
)

		cr = APPLYFADE(R_GetRgbaR(d->dest_exc->rgba), R_GetRgbaR(d->source_exc->rgba), R_GetRgbaR(d->sector->extra_colormap->rgba));
		cg = APPLYFADE(R_GetRgbaG(d->dest_exc->rgba), R_GetRgbaG(d->source_exc->rgba), R_GetRgbaG(d->sector->extra_colormap->rgba));
		cb = APPLYFADE(R_GetRgbaB(d->dest_exc->rgba), R_GetRgbaB(d->source_exc->rgba), R_GetRgbaB(d->sector->extra_colormap->rgba));
		ca = APPLYFADE(R_GetRgbaA(d->dest_exc->rgba), R_GetRgbaA(d->source_exc->rgba), R_GetRgbaA(d->sector->extra_colormap->rgba));

		rgba = R_PutRgbaRGBA(cr, cg, cb, ca);

		cr = APPLYFADE(R_GetRgbaR(d->dest_exc->fadergba), R_GetRgbaR(d->source_exc->fadergba), R_GetRgbaR(d->sector->extra_colormap->fadergba));
		cg = APPLYFADE(R_GetRgbaG(d->dest_exc->fadergba), R_GetRgbaG(d->source_exc->fadergba), R_GetRgbaG(d->sector->extra_colormap->fadergba));
		cb = APPLYFADE(R_GetRgbaB(d->dest_exc->fadergba), R_GetRgbaB(d->source_exc->fadergba), R_GetRgbaB(d->sector->extra_colormap->fadergba));
		ca = APPLYFADE(R_GetRgbaA(d->dest_exc->fadergba), R_GetRgbaA(d->source_exc->fadergba), R_GetRgbaA(d->sector->extra_colormap->fadergba));

		fadergba = R_PutRgbaRGBA(cr, cg, cb, ca);

		fadestart = APPLYFADE(d->dest_exc->fadestart, d->source_exc->fadestart, d->sector->extra_colormap->fadestart);
		fadeend = APPLYFADE(d->dest_exc->fadeend, d->source_exc->fadeend, d->sector->extra_colormap->fadeend);
		flags = abs(factor) > FRACUNIT/2 ? d->dest_exc->flags : d->source_exc->flags; // set new flags halfway through fade

#undef APPLYFADE

		//////////////////
		// setup new colormap
		//////////////////

		if (!(d->sector->extra_colormap = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags)))
		{
			exc = R_CreateDefaultColormap(false);
			exc->fadestart = fadestart;
			exc->fadeend = fadeend;
			exc->flags = flags;
			exc->rgba = rgba;
			exc->fadergba = fadergba;
			exc->colormap = R_CreateLightTable(exc);
			R_AddColormapToList(exc);
			d->sector->extra_colormap = exc;
		}
	}
}

/*
 SoM: 3/8/2000: Friction functions start.
 Add_Friction,
 T_Friction,
 P_SpawnFriction
*/

/** Adds friction thinker.
  *
  * \param friction      Friction value, 0xe800 is normal.
  * \param affectee      Target sector.
  * \param roverfriction FOF or not
  * \sa T_Friction, P_SpawnFriction
  */
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer)
{
	friction_t *f = Z_LevelPoolCalloc(sizeof (*f));
	f->thinker.alloctype = TAT_LEVELPOOL;
	f->thinker.size = sizeof (*f);

	f->thinker.function.acp1 = (actionf_p1)T_Friction;
	f->friction = friction;
	f->movefactor = movefactor;
	f->affectee = affectee;

	if (referrer != -1)
	{
		f->roverfriction = true;
		f->referrer = referrer;
	}
	else
		f->roverfriction = false;

	P_AddThinker(THINK_MAIN, &f->thinker);
}

/** Applies friction to all things in a sector.
  *
  * \param f Friction thinker.
  * \sa Add_Friction
  */
void T_Friction(friction_t *f)
{
	sector_t *sec, *referrer = NULL;
	mobj_t *thing;
	msecnode_t *node;

	sec = sectors + f->affectee;

	// Get FOF control sector
	if (f->roverfriction)
		referrer = sectors + f->referrer;

	// Assign the friction value to players on the floor, non-floating,
	// and clipped. Normally the object's friction value is kept at
	// ORIG_FRICTION and this thinker changes it for icy or muddy floors.

	// When the object is straddling sectors with the same
	// floorheight that have different frictions, use the lowest
	// friction value (muddy has precedence over icy).

	node = sec->touching_thinglist; // things touching this sector
	while (node)
	{
		thing = node->m_thing;
		// apparently, all I had to do was comment out part of the next line and
		// friction works for all mobj's
		// (or at least MF_PUSHABLEs, which is all I care about anyway)
		if (!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) && thing->z == thing->floorz)
		{
			if (f->roverfriction)
			{
				if (thing->floorz != P_GetSpecialTopZ(thing, referrer, sec))
				{
					node = node->m_thinglist_next;
					continue;
				}

				if ((thing->friction == ORIG_FRICTION) // normal friction?
					|| (f->friction < thing->friction))
				{
					thing->friction = f->friction;
					if (thing->player)
						thing->movefactor = f->movefactor;
				}
			}
			else if (P_GetSpecialBottomZ(thing, sec, sec) == thing->floorz && (thing->friction == ORIG_FRICTION // normal friction?
				|| f->friction < thing->friction))
			{
				thing->friction = f->friction;
				if (thing->player)
					thing->movefactor = f->movefactor;
			}
		}
		node = node->m_thinglist_next;
	}
}

/** Spawns all friction effects.
  *
  * \sa P_SpawnSpecials, Add_Friction
  */
static void P_SpawnFriction(void)
{
	size_t i;
	sector_t *s = sectors;

	fixed_t friction; // friction value to be applied during movement
	INT32 movefactor; // applied to each player move to simulate inertia

	for (i = 0; i < numsectors; i++, s++)
	{
		if (s->friction == ORIG_FRICTION)
			continue;

		friction = s->friction;

		if (friction > FRACUNIT)
			friction = FRACUNIT;
		if (friction < 0)
			friction = 0;

		movefactor = P_MoveFactorFromFriction(friction);

		Add_Friction(friction, movefactor, (INT32)(s-sectors), -1);

	}
}

/*
 SoM: 3/8/2000: Push/Pull/Wind/Current functions.
 Add_Pusher,
 PIT_PushThing,
 T_Pusher,
 P_GetPushThing,
 P_SpawnPushers
*/

#define PUSH_FACTOR 7

/** Adds a pusher.
  *
  * \param type     Type of push/pull effect.
  * \param x_mag    X magnitude.
  * \param y_mag    Y magnitude.
  * \param z_mag    Z magnitude.
  * \param affectee Target sector.
  * \param referrer What sector set it
  * \sa T_Pusher, P_GetPushThing, P_SpawnPushers
  */
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, fixed_t z_mag, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider)
{
	pusher_t *p = Z_LevelPoolCalloc(sizeof (*p));
	p->thinker.alloctype = TAT_LEVELPOOL;
	p->thinker.size = sizeof (*p);

	p->thinker.function.acp1 = (actionf_p1)T_Pusher;
	p->type = type;
	p->x_mag = x_mag;
	p->y_mag = y_mag;
	p->z_mag = z_mag;
	p->exclusive = exclusive;
	p->slider = slider;

	if (referrer != -1)
	{
		p->roverpusher = true;
		p->referrer = referrer;
		sectors[referrer].specialflags |= SSF_WINDCURRENT;
	}
	else
	{
		p->roverpusher = false;
		sectors[affectee].specialflags |= SSF_WINDCURRENT;
	}

	p->affectee = affectee;
	P_AddThinker(THINK_MAIN, &p->thinker);
}

/** Applies a pusher to all affected objects.
  *
  * \param p Thinker for the pusher effect.
  * \todo Split up into multiple functions.
  * \sa Add_Pusher, PIT_PushThing
  */
void T_Pusher(pusher_t *p)
{
	sector_t *sec, *referrer = NULL;
	mobj_t *thing;
	msecnode_t *node;
	fixed_t x_mag, y_mag, z_mag;
	fixed_t xspeed = 0, yspeed = 0, zspeed = 0;
	boolean inFOF;
	boolean touching;
	boolean moved;

	x_mag = p->x_mag >> PUSH_FACTOR;
	y_mag = p->y_mag >> PUSH_FACTOR;
	z_mag = p->z_mag >> PUSH_FACTOR;

	sec = sectors + p->affectee;
	if (p->roverpusher)
		referrer = sectors + p->referrer;

	// For constant pushers (wind/current) there are 3 situations:
	//
	// 1) Affected Thing is above the floor.
	//
	//    Apply the full force if wind, no force if current.
	//
	// 2) Affected Thing is on the ground.
	//
	//    Apply half force if wind, full force if current.
	//
	// 3) Affected Thing is below the ground (underwater effect).
	//
	//    Apply no force if wind, full force if current.
	//
	// Apply the effect to clipped players only for now.
	//
	// In Phase II, you can apply these effects to Things other than players.

	// constant pushers p_wind and p_current
	node = sec->touching_thinglist; // things touching this sector
	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)
			&& !(thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE))
			continue;

		if (!((thing->flags & MF_PUSHABLE) || ((thing->info->flags & MF_PUSHABLE) && thing->fuse))
			&& !(thing->type == MT_PLAYER
			|| thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE
			|| thing->type == MT_LITTLETUMBLEWEED
			|| thing->type == MT_BIGTUMBLEWEED))
			continue;

		if (thing->eflags & MFE_PUSHED)
			continue;

		if (thing->player && (thing->state == &states[thing->info->painstate])
			&& (thing->player->flashing > (K_GetKartFlashing(thing->player)/4)*3
			&& thing->player->flashing <= K_GetKartFlashing(thing->player)))
			continue;

		inFOF = touching = moved = false;

		// Find the area that the 'thing' is in
		if (p->roverpusher)
		{
			fixed_t top, bottom;

			top = P_GetSpecialTopZ(thing, referrer, sec);
			bottom = P_GetSpecialBottomZ(thing, referrer, sec);

			if (thing->eflags & MFE_VERTICALFLIP)
			{
				if (bottom > thing->z + thing->height
					|| top < (thing->z + (thing->height >> 1)))
					continue;

				if (thing->z < bottom)
					touching = true;

				if (thing->z + (thing->height >> 1) > bottom)
					inFOF = true;

			}
			else
			{
				if (top < thing->z || bottom > (thing->z + (thing->height >> 1)))
					continue;
				if (thing->z + thing->height > top)
					touching = true;

				if (thing->z + (thing->height >> 1) < top)
					inFOF = true;
			}
		}
		else // Treat the entire sector as one big FOF
		{
			if (thing->z == P_GetSpecialBottomZ(thing, sec, sec))
				touching = true;
			else if (p->type != p_current)
				inFOF = true;
		}

		if (!touching && !inFOF) // Object is out of range of effect
			continue;

		if (inFOF || (p->type == p_current && touching))
		{
			xspeed = x_mag; // full force
			yspeed = y_mag;
			zspeed = z_mag;
			moved = true;
		}
		else if (p->type == p_wind && touching)
		{
			xspeed = x_mag>>1; // half force
			yspeed = y_mag>>1;
			zspeed = z_mag>>1;
			moved = true;
		}

		thing->momx += xspeed;
		thing->momy += yspeed;

		// Do not apply upwards wind if the player is
		// fastfalling. This could cancel out the increased
		// gravity from fastfalling!
		if (!(thing->player && thing->player->fastfall && zspeed * P_MobjFlip(thing) > 0))
		{
			thing->momz += zspeed;
		}

		if (thing->player)
		{
			thing->player->cmomx += xspeed;
			thing->player->cmomy += yspeed;
			thing->player->cmomx = FixedMul(thing->player->cmomx, ORIG_FRICTION);
			thing->player->cmomy = FixedMul(thing->player->cmomy, ORIG_FRICTION);
		}

		// Tumbleweeds bounce a bit...
		if (thing->type == MT_LITTLETUMBLEWEED || thing->type == MT_BIGTUMBLEWEED)
			thing->momz += P_AproxDistance(xspeed, yspeed) >> 2;

		if (moved)
		{
			if (p->slider && thing->player && !thing->player->carry)
			{
				P_ResetPlayer (thing->player);

				thing->player->carry = CR_SLIDING;
				thing->angle = R_PointToAngle2(0, 0, xspeed, yspeed);

				// Previously modified boostangle, and didn't run in demo playback.
				// (Probably because SRB2's demo format was already storing and applying the camera changes made here?)
				// Whatever the case, this is drawangle now so that players don't fly out of pusher setpieces.
				angle_t angle = thing->player->drawangle;
				if (thing->angle - angle > ANGLE_180)
					thing->player->drawangle = angle - (angle - thing->angle) / 8;
				else
					thing->player->drawangle = angle + (thing->angle - angle) / 8;
				//P_SetPlayerAngle(thing->player, thing->angle);

			}

			if (p->exclusive)
				thing->eflags |= MFE_PUSHED;
		}
	}
}

/** Spawns pushers.
  *
  * \sa P_SpawnSpecials, Add_Pusher
  */
static void P_SpawnPushers(void)
{
	size_t i;
	line_t *l = lines;
	register INT32 s;
	fixed_t length, hspeed, dx, dy;

	for (i = 0; i < numlines; i++, l++)
	{
		if (l->special != 541)
			continue;

		length = R_PointToDist2(l->v2->x, l->v2->y, l->v1->x, l->v1->y);
		hspeed = l->args[1] << FRACBITS;
		dx = FixedMul(FixedDiv(l->dx, length), hspeed);
		dy = FixedMul(FixedDiv(l->dy, length), hspeed);

		if (l->args[0] == 0)
			Add_Pusher(l->args[3], dx, dy, l->args[2] << FRACBITS, (INT32)(l->frontsector - sectors), -1, !(l->args[4] & TMPF_NONEXCLUSIVE), !!(l->args[4] & TMPF_SLIDE));
		else
		{
			TAG_ITER_SECTORS(l->args[0], s)
				Add_Pusher(l->args[3], dx, dy, l->args[2] << FRACBITS, s, -1, !(l->args[4] & TMPF_NONEXCLUSIVE), !!(l->args[4] & TMPF_SLIDE));
		}
	}
}

// Add an earthquake effect to the list.
static quake_t *PushQuake(void)
{
	quake_t *quake = Z_Calloc(sizeof(quake_t), PU_LEVEL, NULL);

	quake->next = g_quakes;
	if (g_quakes != NULL)
	{
		g_quakes->prev = quake;
	}
	g_quakes = quake;

	return quake;
}

void P_StartQuake(tic_t time, fixed_t intensity, fixed_t radius, mappoint_t *epicenter)
{
	quake_t *quake = NULL;

	if (time <= 0 || intensity <= 0)
	{
		// Invalid parameters
		return;
	}

	quake = PushQuake();

	quake->time = quake->startTime = time;
	quake->intensity = intensity;

	if (radius > 0)
	{
		quake->radius = radius;

		if (epicenter != NULL)
		{
			quake->epicenter = (mappoint_t *)Z_Malloc(sizeof(mappoint_t), PU_LEVEL, NULL);
			quake->epicenter->x = epicenter->x;
			quake->epicenter->y = epicenter->y;
			quake->epicenter->z = epicenter->z;
		}
	}
}

void P_StartQuakeFromMobj(tic_t time, fixed_t intensity, fixed_t radius, mobj_t *mobj)
{
	quake_t *quake = NULL;

	if (time <= 0 || intensity <= 0 || radius <= 0 || P_MobjWasRemoved(mobj) == true)
	{
		// Invalid parameters
		return;
	}

	quake = PushQuake();

	quake->time = quake->startTime = time;
	quake->intensity = intensity;

	quake->radius = radius;

	quake->mobj = mobj; // Update epicenter with mobj position every tic

	quake->epicenter = (mappoint_t *)Z_Malloc(sizeof(mappoint_t), PU_LEVEL, NULL);
	quake->epicenter->x = mobj->x;
	quake->epicenter->y = mobj->y;
	quake->epicenter->z = mobj->z + (mobj->height / 2);
}

void P_DoQuakeOffset(UINT8 view, mappoint_t *viewPos, mappoint_t *offset)
{
	const boolean battle = (gametyperules & GTR_PRISONS) && !battleprisons;
	const player_t *viewer = &players[ displayplayers[view] ];
	quake_t *quake = NULL;
	fixed_t ir = 0;
	fixed_t addZ = 0;

	if (cv_screenshake.value == 0)
	{
		return;
	}

	// Add the main quake effects.
	quake = g_quakes;
	while (quake != NULL)
	{
		ir = quake->intensity;

		// Modulate with time remaining.
		const fixed_t timeEase = (FRACUNIT * ((quake->startTime - quake->time) - 1)) / quake->startTime;
		ir = Easing_InCubic(timeEase, ir, 0);

		// Modulate with distance from epicenter, if it exists.
		if (quake->radius > 0 && quake->epicenter != NULL)
		{
			const fixed_t denom = battle ? 2 : 1;
			const fixed_t distBuffer = 256 * mapobjectscale / denom; // add a small buffer zone before it starts to drop off
			const fixed_t epidist = P_AproxDistance(
				P_AproxDistance(
					viewPos->x - quake->epicenter->x,
					viewPos->y - quake->epicenter->y
				),
				viewPos->z - quake->epicenter->z
			) - distBuffer;

			fixed_t distEase = FixedDiv(max(epidist, 0), quake->radius / denom);
			distEase = min(distEase, FRACUNIT);
			ir = Easing_InCubic(distEase, ir, 0);
		}

		addZ += ir;
		quake = quake->next;
	}

	// Add level-based effects
	if (P_MobjWasRemoved(viewer->mo) == false
		&& viewer->speed > viewer->mo->scale
		&& P_IsObjectOnGround(viewer->mo) == true)
	{
		// Add offroad effects.
		if (viewer->boostpower < FRACUNIT)
		{
			ir = FixedMul((FRACUNIT - viewer->boostpower) << 2, mapobjectscale);
			addZ += ir;
		}

		// Add stair jank effects.
		if (viewer->stairjank > 0)
		{
			ir = FixedMul((viewer->stairjank * FRACUNIT * 5) / 17, mapobjectscale);
			addZ += ir;
		}
	}

	fixed_t maxShake = FixedMul(cv_cam_height[view].value, mapobjectscale) * 3 / 4;

	// Map-specific camera height
	if (mapheaderinfo[gamemap-1]->cameraHeight >= 0)
	{
		if (r_splitscreen != 1)
			maxShake = FixedMul(mapheaderinfo[gamemap-1]->cameraHeight, mapobjectscale) * 3 / 4;

		// For 2p SPLITSCREEN SPECIFICALLY:
		// The view is pretty narrow, so move it back 3/20 of the way towards default camera height.
		else
			maxShake = FixedMul((mapheaderinfo[gamemap-1]->cameraHeight*17 + cv_cam_height[view].value*3)/20, mapobjectscale) * 3 / 4;
	}

	if (battle)
	{
		addZ /= 2;
		maxShake /= 2;
	}

	if (addZ > maxShake)
	{
		// Cap screen shake between reasonable values
		addZ = maxShake;
	}

	// Reverse every tic.
	if ((leveltime + view) & 1)
	{
		addZ = -addZ;
	}

	if (cv_screenshake.value == 1) // Half
	{
		addZ /= 2;
	}

	// Finalize the effects.
	offset->z += addZ;
}

void P_FreeQuake(quake_t *remove)
{
	if (remove->prev != NULL)
	{
		remove->prev->next = remove->next;
	}

	if (remove->next != NULL)
	{
		remove->next->prev = remove->prev;
	}

	if (remove == g_quakes)
	{
		g_quakes = remove->next;
	}

	if (remove->epicenter != NULL)
	{
		Z_Free(remove->epicenter);
		remove->epicenter = NULL;
	}

	Z_Free(remove);
}

void P_CheckSectorTransitionalEffects(mobj_t *thing, sector_t *prevsec, boolean wasgrounded)
{
	if (!udmf)
	{
		return;
	}

	boolean sectorchanged = (prevsec != thing->subsector->sector);

	if (!sectorchanged && wasgrounded == P_IsObjectOnGround(thing))
	{
		return;
	}

	// Check for each time / once sector special actions
	P_CheckMobjTouchingSectorActions(thing, false, sectorchanged);
}
