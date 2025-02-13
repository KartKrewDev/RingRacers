// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_mapuser.c
/// \brief UDMF: Custom user properties

#include "k_mapuser.h"

#include "doomdef.h"
#include "doomstat.h"
#include "doomdata.h"
#include "doomtype.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_fixed.h"

/*--------------------------------------------------
	void K_UserPropertyPush(mapUserProperties_t *user, const char *key, mapUserPropertyType_e type, void *value)

		See header file for description.
--------------------------------------------------*/
void K_UserPropertyPush(mapUserProperties_t *user, const char *key, mapUserPropertyType_e type, void *value)
{
	const size_t keyLength = strlen(key);
	mapUserProperty_t *prop = NULL;

	I_Assert(keyLength > 0);

	if (user->length >= user->capacity)
	{
		if (user->capacity == 0)
		{
			user->capacity = 8;
		}
		else
		{
			user->capacity *= 2;
		}

		user->properties = (mapUserProperty_t *)Z_ReallocAlign(
			user->properties,
			sizeof(mapUserProperty_t) * user->capacity,
			PU_LEVEL,
			NULL,
			sizeof(mapUserProperty_t) * 8
		);
	}

	prop = &user->properties[ user->length ];

	prop->key = Z_Malloc(keyLength + 1, PU_LEVEL, NULL);
	M_Memcpy(prop->key, key, keyLength + 1);
	prop->key[keyLength] = '\0';

	prop->hash = quickncasehash(prop->key, keyLength);

	prop->type = type;
	switch (type)
	{
		case USER_PROP_BOOL:
		{
			prop->valueBool = *(boolean *)value;
			break;
		}
		case USER_PROP_INT:
		{
			prop->valueInt = *(INT32 *)value;
			break;
		}
		case USER_PROP_FIXED:
		{
			prop->valueFixed = *(fixed_t *)value;
			break;
		}
		case USER_PROP_STR:
		{
			const char *string = *(const char **)value;
			const size_t stringLength = strlen(string);

			prop->valueStr = Z_Malloc(stringLength + 1, PU_LEVEL, NULL);
			M_Memcpy(prop->valueStr, string, stringLength + 1);
			prop->valueStr[stringLength] = '\0';
			break;
		}
	}

	user->length++;
}

/*--------------------------------------------------
	mapUserProperty_t *K_UserPropertyFind(mapUserProperties_t *user, const char *key)

		See header file for description.
--------------------------------------------------*/
mapUserProperty_t *K_UserPropertyFind(mapUserProperties_t *user, const char *key)
{
	const size_t keyLength = strlen(key);
	const UINT32 hash = quickncasehash(key, keyLength);
	size_t i;

	if (user->length == 0)
	{
		return NULL;
	}

	for (i = 0; i < user->length; i++)
	{
		mapUserProperty_t *const prop = &user->properties[ i ];

		if (hash != prop->hash)
		{
			continue;
		}

		if (strcasecmp(key, prop->key))
		{
			continue;
		}

		return prop;
	}

	return NULL;
}

/*--------------------------------------------------
	static void K_UserPropertyFree(mapUserProperty_t *prop)

		Frees the memory of a single user property.

	Input Arguments:-
		prop - User property memory structure to free.

	Return:-
		N/A
--------------------------------------------------*/
static void K_UserPropertyFree(mapUserProperty_t *prop)
{
	if (prop->key != NULL)
	{
		Z_Free(prop->key);
		prop->key = NULL;
	}

	if (prop->valueStr != NULL)
	{
		Z_Free(prop->valueStr);
		prop->valueStr = NULL;
	}
}

/*--------------------------------------------------
	void K_UserPropertiesClear(mapUserProperties_t *user)

		See header file for description.
--------------------------------------------------*/
void K_UserPropertiesClear(mapUserProperties_t *user)
{
	size_t i;

	if (user->properties == NULL)
	{
		return;
	}

	for (i = 0; i < user->length; i++)
	{
		K_UserPropertyFree(&user->properties[i]);
	}

	Z_Free(user->properties);
	user->properties = NULL;
	user->length = user->capacity = 0;
}
