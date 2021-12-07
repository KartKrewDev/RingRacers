// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2021 by ZDoom + GZDoom teams, and contributors
// Copyright (C) 2021 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2021 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_terrain.c
/// \brief Implementation of TERRAIN lump from GZDoom codebase for DRRR.

#include "k_terrain.h"

#include "z_zone.h"

t_splash_t *splashDefs = NULL;
UINT16 numSplashDefs = 0;

t_footstep_t *footstepDefs = NULL;
UINT16 numFootstepDefs = 0;

terrain_t *terrainDefs = NULL;
UINT16 numTerrainDefs = 0;

UINT16 defaultTerrain = UINT16_MAX;

terrain_t *K_GetTerrainByIndex(UINT16 checkIndex)
{
	if (checkIndex >= numTerrainDefs)
	{
		return NULL;
	}

	return terrainDefs[checkIndex];
}

terrain_t *K_GetTerrainByName(const char *checkName)
{
	INT32 i;

	if (numTerrainDefs == 0)
	{
		return NULL;
	}

	// Search backwards through all terrain definitions.
	// The latest one will have priority over the older one.
	for (i = numTerrainDefs-1; i >= 0; i--)
	{
		terrain_t t = terrainDefs[i];

		if (stricmp(checkName, t->name) == 0)
		{
			// Name matches.
			return t;
		}
	}

	return NULL;
}

terrain_t *K_GetDefaultTerrain(void)
{
	return K_GetTerrainByIndex(defaultTerrain);
}

terrain_t *K_GetTerrainForTextureNum(INT32 textureNum)
{
	INT32 i, j;

	if (textureNum == -1)
	{
		return NULL;
	}

	if (numTerrainDefs == 0)
	{
		return NULL;
	}

	// Search backwards through all terrain definitions.
	// The latest one will have priority over the older one.

	for (i = numTerrainDefs-1; i >= 0; i--)
	{
		terrain_t t = terrainDefs[i];

		if (t->numTextureIDs == 0)
		{
			// No textures are applied to this terrain type.
			continue;
		}

		for (j = 0; j < numTextureIDs; j++)
		{
			if (textureNum == t->textureIDs[j])
			{
				// Texture matches.
				return t;
			}
		}
	}

	// This texture doesn't have a terrain directly applied to it,
	// so we fallback to the default terrain.
	return K_GetDefaultTerrain();
}

terrain_t *K_GetTerrainForTextureName(const char *checkName)
{
	return K_GetTerrainForTextureNum( R_CheckTextureNumForName(checkName) );
}

static void K_GrowSplashDefs(void)
{
	numSplashDefs++;
	splashDefs = (t_splash_t *)Z_Realloc(splashDefs, sizeof(t_splash_t) * (numSplashDefs + 1), PU_STATIC, NULL);
}

static void K_GrowFootstepDefs(void)
{
	numFootstepDefs++;
	footstepDefs = (t_footstep_t *)Z_Realloc(footstepDefs, sizeof(t_footstep_t) * (numFootstepDefs + 1), PU_STATIC, NULL);
}

static void K_ParseNextLine(char *p, char *token)
{
	// parse next line
	while (*p != '\0' && *p != '\n')
	{
		++p;
	}

	if (*p == '\n')
	{
		++p;
	}

	token = M_GetToken(p);
}

static void K_GrowTerrainDefs(void)
{
	numTerrainDefs++;
	terrainDefs = (terrain_t *)Z_Realloc(terrainDefs, sizeof(terrain_t) * (numTerrainDefs + 1), PU_STATIC, NULL);
}

static void K_ParseTerrainDefintion(void)
{
	char *token;
	size_t tokenLength;
	char *endPos;
	size_t i;

	// Startname
	token = M_GetToken(NULL);

	if (token == NULL)
	{
		I_Error("Error parsing TERRAIN lump: Expected terrain definition, got end of file");
	}

	if (stricmp(token, "{") != 0)
	{
		I_Error("Error parsing TERRAIN lump: No starting bracket");
	}

	while (token != NULL)
	{
		
	}
	tokenLength = strlen(token);

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
		if (animdefs[i].istexture == istexture // Check if it's the same type!
		&& stricmp(animdefsToken, animdefs[i].startname) == 0)
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
	animdefs[i].istexture = istexture;

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

	animdefs[i].speed = animSpeed;

	Z_Free(animdefsToken);
}

void K_ParseTERRAINLump(INT32 wadNum, UINT16 lumpnum)
{
	char *lump;
	size_t lumpLength;
	char *fullText;
	char *token;
	char *p;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	lump = (char *)W_CacheLumpNumPwad(wadNum, lumpnum, PU_STATIC);

	// If that didn't exist, we have nothing to do here.
	if (lump == NULL)
	{
		return;
	}

	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	lumpLength = W_LumpLengthPwad(wadNum, lumpnum);
	fullText = (char *)Z_Malloc((lumpLength + 1) * sizeof(char), PU_STATIC, NULL);

	// Now move the contents of the lump into this new location.
	memmove(fullText, lump, lumpLength);

	// Make damn well sure the last character in our new memory location is \0.
	fullText[lumpLength] = '\0';

	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(lump);

	// Now, let's start parsing this thing
	p = fullText;
	token = M_GetToken(p);

	while (token != NULL)
	{
		if (stricmp(token, "TERRAIN") == 0)
		{
			Z_Free(token);
			K_ParseTerrainDefintion(&p, &token);
		}
		else
		{
			I_Error("Error parsing TERRAIN lump: Expected \"SPLASH\", \"FOOTSTEP\", \"TERRAIN\", \"FLOOR\"; got \"%s\"", token);
		}

		K_ParseNextLine(&p, &token);
	}

	Z_Free(token);
	Z_Free((void *)fullText);
}
