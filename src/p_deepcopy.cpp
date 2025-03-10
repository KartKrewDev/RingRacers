// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_deepcopy.cpp
/// \brief Methods for deep copying

#include "p_deepcopy.h"

#include "typedef.h"
#include "z_zone.h"
#include "taglist.h"
#include "r_defs.h"

/*--------------------------------------------------
	static void P_DeepCopy(Type *target, Type *source, void (*callback)(Type *, Type *))

		Make a deep copy of a struct, by using memcpy
		for an initial shallow copy, and a callback
		function to handle any memory addresses.

	Input Arguments:-
		Type: The type of the structs.
		target: The struct to copy into.
		source: The struct to copy from.
		callback: A callback function, intended to replace
			pointers after the initial shallow copy.

	Return:-
		N/A
--------------------------------------------------*/

template <typename Type>
static void P_DeepCopy(Type *target, Type *source, void (*callback)(Type *, Type *))
{
	memcpy(target, source, sizeof(Type));

	if (callback != nullptr)
	{
		callback(target, source);
	}
}

/*--------------------------------------------------
	static void P_DeepCopyArray(Type **target_array, Type **source_array, size_t source_len, void (*callback)(Type *, Type *))

		Make a deep copy of an array of structs, by using
		memcpy for an initial shallow copy, and a callback
		function to handle any memory addresses for each
		individual struct.

	Input Arguments:-
		Type: The type of the structs.
		target_array: The start of the array to copy into.
			This will be allocated by this function, so
			it should be freed beforehand.
		source_array: The start of the array to copy from.
		source_len: The length of the array to copy from.
		callback: A callback function, intended to replace
			pointers after the initial shallow copy.

	Return:-
		N/A
--------------------------------------------------*/

template <typename Type>
static void P_DeepCopyArray(Type **target_array, Type **source_array, size_t source_len, void (*callback)(Type *, Type *))
{
	const size_t source_total = source_len * sizeof(Type);

	*target_array = static_cast<Type *>(Z_Calloc(source_total, PU_LEVEL, nullptr));
	memcpy(*target_array, *source_array, source_total);

	if (callback != nullptr)
	{
		for (size_t i = 0; i < source_len; i++)
		{
			Type *target = &(*target_array)[i];
			Type *source = &(*source_array)[i];

			callback(target, source);
		}
	}
}

/*--------------------------------------------------
	static void copy_taglist_tags(taglist_t *target, taglist_t *source)

		Make a deep copy of taglist's tags fields.
		Used as a helper function for other callbacks,
		does not work as a full deep copy of taglist_t
		on its own.

	Input Arguments:-
		target: The struct to copy into.
		source: The struct to copy from.

	Return:-
		N/A
--------------------------------------------------*/

static void copy_taglist_tags(taglist_t *target, taglist_t *source)
{
	if (source->count)
	{
		target->tags = static_cast<mtag_t *>(
			memcpy(
				Z_Malloc(
					source->count * sizeof(mtag_t),
					PU_LEVEL,
					nullptr
				),
				source->tags,
				source->count * sizeof(mtag_t)
			)
		);
	}
}

/*--------------------------------------------------
	static void copy_stringarg(char **target, const char *source)

		Make a deep copy of a string argument.

	Input Arguments:-
		target: Double pointer to the string to copy to.
		source: The string to copy from.

	Return:-
		N/A
--------------------------------------------------*/

static void copy_stringarg(char **target, const char *source)
{
	// stringarg memory is really freaking touchy,
	// so I am being careful and being explicit
	// on how it is copied over instead of just
	// using strcpy or smth

	size_t len = 0;
	if (source != nullptr)
	{
		len = strlen(source);
	}

	if (len > 0)
	{
		*target = static_cast<char *>(
			memcpy(
				Z_Malloc(
					len + 1,
					PU_LEVEL,
					nullptr
				),
				source,
				len
			)
		);
		(*target)[len] = '\0';
	}
}

/*--------------------------------------------------
	static void copy_sector_callback(sector_t *target, sector_t *source)

		Handles memory addresses after creating a shallow copy
		of a sector_t, to turn it into a deep copy.

	Input Arguments:-
		target: The struct to copy into.
		source: The struct to copy from.

	Return:-
		N/A
--------------------------------------------------*/

static void copy_sector_callback(sector_t *target, sector_t *source)
{
	// (Not a true deep copy until all of the memory addresses are accounted for.)
	copy_taglist_tags(&target->tags, &source->tags);

	for (size_t i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
	{
		copy_stringarg(&target->stringargs[i], source->stringargs[i]);
	}
}

/*--------------------------------------------------
	void P_DeepCopySector(sector_t *target, sector_t *source)

		See header file for description.
--------------------------------------------------*/

void P_DeepCopySector(sector_t *target, sector_t *source)
{
	P_DeepCopy<sector_t>(target, source, copy_sector_callback);
}

/*--------------------------------------------------
	void P_DeepCopySectors(sector_t **target_array, sector_t **source_array, size_t source_len)

		See header file for description.
--------------------------------------------------*/

void P_DeepCopySectors(sector_t **target_array, sector_t **source_array, size_t source_len)
{
	P_DeepCopyArray<sector_t>(target_array, source_array, source_len, copy_sector_callback);
}

/*--------------------------------------------------
	static void copy_line_callback(line_t *target, line_t *source)

		Handles memory addresses after creating a shallow copy
		of a line_t, to turn it into a deep copy.

	Input Arguments:-
		target: The struct to copy into.
		source: The struct to copy from.

	Return:-
		N/A
--------------------------------------------------*/

static void copy_line_callback(line_t *target, line_t *source)
{
	// (Not a true deep copy until all of the memory addresses are accounted for.)
	copy_taglist_tags(&target->tags, &source->tags);

	for (size_t i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
	{
		copy_stringarg(&target->stringargs[i], source->stringargs[i]);
	}
}

/*--------------------------------------------------
	void P_DeepCopyLine(line_t *target, line_t *source)

		See header file for description.
--------------------------------------------------*/

void P_DeepCopyLine(line_t *target, line_t *source)
{
	P_DeepCopy<line_t>(target, source, copy_line_callback);
}

/*--------------------------------------------------
	void P_DeepCopyLines(line_t **target_array, line_t **source_array, size_t source_len)

		See header file for description.
--------------------------------------------------*/

void P_DeepCopyLines(line_t **target_array, line_t **source_array, size_t source_len)
{
	P_DeepCopyArray<line_t>(target_array, source_array, source_len, copy_line_callback);
}

/*--------------------------------------------------
	void P_DeepCopySide(line_t *target, line_t *source)

		See header file for description.
--------------------------------------------------*/

void P_DeepCopySide(side_t *target, side_t *source)
{
	P_DeepCopy<side_t>(target, source, nullptr); // (Not a true deep copy until all of the memory addresses are accounted for.)
}

/*--------------------------------------------------
	void P_DeepCopySides(side_t **target_array, side_t **source_array, size_t source_len)

		See header file for description.
--------------------------------------------------*/

void P_DeepCopySides(side_t **target_array, side_t **source_array, size_t source_len)
{
	P_DeepCopyArray<side_t>(target_array, source_array, source_len, nullptr); // (Not a true deep copy until all of the memory addresses are accounted for.)
}
