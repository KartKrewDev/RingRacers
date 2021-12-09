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

#include "dehacked.h" // get_number
#include "doomdata.h"
#include "doomdef.h"
#include "doomtype.h"
#include "fastcmp.h"
#include "m_fixed.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_textures.h"
#include "w_wad.h"
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

	return &terrainDefs[checkIndex];
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
		terrain_t *t = &terrainDefs[i];

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
	INT32 i;

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
		terrain_t *t = &terrainDefs[i];
		size_t j;

		if (t->numTextureIDs == 0)
		{
			// No textures are applied to this terrain type.
			continue;
		}

		for (j = 0; j < t->numTextureIDs; j++)
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

void K_UpdateMobjTerrain(mobj_t *mo, INT32 flatID)
{
	levelflat_t *levelFlat = NULL;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	if (flatID < 0 || flatID >= (signed)numlevelflats)
	{
		// Clearly invalid floor...
		mo->terrain = NULL;
		return;
	}

	if (mo->flags & MF_NOCLIPHEIGHT)
	{
		// You can't collide with floors anyway!
		mo->terrain = NULL;
		return;
	}

	// Update the object's terrain pointer.
	levelFlat = &levelflats[flatID];
	mo->terrain = K_GetTerrainForTextureName(levelFlat->name);
}

//
// Parser code starts here.
//

static void K_FlagBoolean(UINT32 *inputFlags, UINT32 newFlag, char *val)
{
	if (stricmp(val, "true") == 0)
	{
		*inputFlags |= newFlag;
	}
	else if (stricmp(val, "false") == 0)
	{
		*inputFlags &= ~newFlag;
	}
}

static void K_TerrainDefaults(terrain_t *terrain)
{
	terrain->splashID = UINT16_MAX;
	terrain->footstepID = UINT16_MAX;

	terrain->friction = FRACUNIT;
	terrain->offroad = 0;
	terrain->damageType = -1;
	terrain->trickPanel = 0;
	terrain->flags = 0;
}

static void K_NewTerrainDefs(void)
{
	numTerrainDefs++;
	terrainDefs = (terrain_t *)Z_Realloc(terrainDefs, sizeof(terrain_t) * (numTerrainDefs + 1), PU_STATIC, NULL);
	K_TerrainDefaults( &terrainDefs[numTerrainDefs - 1] );
}

static void K_ParseTerrainParameter(UINT32 i, char *param, char *val)
{
	terrain_t *terrain = &terrainDefs[i];

	if (stricmp(param, "splash") == 0)
	{
		//terrain->splashID = 0;
	}
	else if (stricmp(param, "footstep") == 0)
	{
		//terrain->footstepID = 0;
	}
	else if (stricmp(param, "friction") == 0)
	{
		terrain->friction = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "offroad") == 0)
	{
		terrain->offroad = (UINT8)get_number(val); // offroad strength enum?
	}
	else if (stricmp(param, "damageType") == 0)
	{
		terrain->damageType = (INT16)get_number(val);
	}
	else if (stricmp(param, "trickPanel") == 0)
	{
		terrain->trickPanel = (UINT8)get_number(val); // trick panel strength enum?
	}
	else if (stricmp(param, "liquid") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_LIQUID, val);
	}
	else if (stricmp(param, "sneakerPanel") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_SNEAKERPANEL, val);
	}
}

static boolean K_DoTERRAINLumpParse(size_t num, void (*parser)(UINT32, char *, char *))
{
	char *param, *val;

	param = M_GetToken(NULL);

	if (!fastcmp(param, "{"))
	{
		Z_Free(param);
		CONS_Alert(CONS_WARNING, "Invalid TERRAIN data capsule!\n");
		return false;
	}

	Z_Free(param);

	while (true)
	{
		param = M_GetToken(NULL);

		if (fastcmp(param, "}"))
		{
			Z_Free(param);
			break;
		}

		val = M_GetToken(NULL);
		parser(num, param, val);

		Z_Free(param);
		Z_Free(val);
	}

	return true;
}

static boolean K_TERRAINLumpParser(UINT8 *data, size_t size)
{
	char *tkn = M_GetToken((char *)data);
	size_t pos = 0;
	size_t i;

	while (tkn && (pos = M_GetTokenPos()) < size)
	{
		boolean valid = true;

		// Avoid anything inside bracketed stuff, only look for external keywords.
		if (fastcmp(tkn, "{") || fastcmp(tkn, "}"))
		{
			CONS_Alert(CONS_ERROR, "Rogue bracket detected in TERRAIN lump.\n");
			valid = false;
		}
		// Check for valid fields.
		else if (stricmp(tkn, "terrain") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos < size)
			{
				terrain_t *t = NULL;

				for (i = 0; i < numTerrainDefs; i++)
				{
					t = &terrainDefs[i];

					if (stricmp(tkn, t->name) == 0)
					{
						break;
					}
				}

				if (i == numTerrainDefs)
				{
					K_NewTerrainDefs();
					t = &terrainDefs[i];

					strncpy(t->name, tkn, TERRAIN_NAME_LEN);
					CONS_Printf("Created new Terrain type '%s'\n", t->name);
				}

				valid = K_DoTERRAINLumpParse(i, K_ParseTerrainParameter);
			}
			// TODO: the other block types!
			else
			{
				CONS_Alert(CONS_ERROR, "No terrain type name.\n");
				valid = false;
			}
		}
		else
		{
			CONS_Alert(CONS_ERROR, "Unknown field '%s' found in TERRAIN lump.\n", tkn);
			valid = false;
		}

		Z_Free(tkn);

		if (valid == false)
		{
			return false;
		}

		tkn = M_GetToken(NULL);
	}

	Z_Free(tkn);
	return true;
}

void K_InitTerrain(UINT16 wadNum)
{
	UINT16 lumpNum;
	lumpinfo_t *lump_p = wadfiles[wadNum]->lumpinfo;

	// Iterate through all lumps and compare the name individually.
	// In PK3 files, you can potentially have multiple TERRAIN differentiated by
	// their file extension.
	for (lumpNum = 0; lumpNum < wadfiles[wadNum]->numlumps; lumpNum++, lump_p++)
	{
		UINT8 *data;

		if (memcmp(lump_p->name, "TERRAIN", 8) != 0)
		{
			continue;
		}

		data = (UINT8 *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);

		// If that didn't exist, we have nothing to do here.
		if (data == NULL)
		{
			continue;
		}
		else
		{
			size_t size = W_LumpLengthPwad(wadNum, lumpNum);

			size_t nameLength = strlen(wadfiles[wadNum]->filename) + 1 + strlen(lump_p->fullname); // length of file name, '|', and lump name
			char *name = malloc(nameLength + 1);

			sprintf(name, "%s|%s", wadfiles[wadNum]->filename, lump_p->fullname);
			name[nameLength] = '\0';

			size = W_LumpLengthPwad(wadNum, lumpNum);

			CONS_Printf(M_GetText("Loading TERRAIN from %s\n"), name);
			K_TERRAINLumpParser(data, size);

			free(name);
		}
	}
}
