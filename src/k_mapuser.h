// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_mapuser.h
/// \brief UDMF: Custom user properties

#ifndef __K_MAPUSER__
#define __K_MAPUSER__

#include "doomdef.h"
#include "doomstat.h"
#include "doomdata.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mapUserProperty_t
{
	UINT32 hash;
	char *key;

	mapUserPropertyType_e type;

	boolean valueBool;
	INT32 valueInt;
	fixed_t valueFixed;
	char *valueStr;
};

// mapUserProperties_t has to be defined in doomdata.h instead
// because of circular dependency issues

/*--------------------------------------------------
	void K_UserPropertyPush(mapUserProperties_t *user, const char *key, mapUserPropertyType_e type, void *value);

		Adds a new key value to the user properties struct.

	Input Arguments:-
		user - User properties memory structure.
		key - The key to add.
		type - Enum representing the type of the value to add.
		value - The value to add, as a void pointer.

	Return:-
		N/A
--------------------------------------------------*/

void K_UserPropertyPush(mapUserProperties_t *user, const char *key, mapUserPropertyType_e type, void *value);


/*--------------------------------------------------
	mapUserProperty_t *K_UserPropertyFind(mapUserProperties_t *user, const char *key);

		Tries to find if the key value already exists in this
		user properties struct.

	Input Arguments:-
		user - User properties memory structure.
		key - The key to find.

	Return:-
		The struct of the property we just got, or NULL if the key didn't exist already.
--------------------------------------------------*/

mapUserProperty_t *K_UserPropertyFind(mapUserProperties_t *user, const char *key);


/*--------------------------------------------------
	void K_UserPropertiesClear(mapUserProperties_t *user);

		Frees an entire user properties struct.

	Input Arguments:-
		user - User properties memory structure to free.

	Return:-
		N/A
--------------------------------------------------*/

void K_UserPropertiesClear(mapUserProperties_t *user);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_MAPUSER__
