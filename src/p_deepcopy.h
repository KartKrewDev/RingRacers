// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_deepcopy.h
/// \brief Methods for deep copying

#ifndef __P_DEEPCOPY__
#define __P_DEEPCOPY__

#include "doomdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------
	void P_DeepCopySector(sector_t *target, sector_t *source);

		Make a deep copy of a single sector_t.

	Input Arguments:-
		target: The struct to copy into.
		source: The struct to copy from.

	Return:-
		N/A
--------------------------------------------------*/

void P_DeepCopySector(sector_t *target, sector_t *source);


/*--------------------------------------------------
	void P_DeepCopySectors(sector_t **target_array, sector_t **source_array, size_t source_len);

		Make a deep copy of an array of sector_t.

	Input Arguments:-
		target_array: The start of the array to copy into.
			This will be allocated by this function, so
			it should be freed beforehand.
		source_array: The start of the array to copy from.
		source_len: The length of the array to copy from.

	Return:-
		N/A
--------------------------------------------------*/

void P_DeepCopySectors(sector_t **target_array, sector_t **source_array, size_t source_len);


/*--------------------------------------------------
	void P_DeepCopyLine(line_t *target, line_t *source)

		Make a deep copy of a single line_t.

	Input Arguments:-
		target: The struct to copy into.
		source: The struct to copy from.

	Return:-
		N/A
--------------------------------------------------*/

void P_DeepCopyLine(line_t *target, line_t *source);


/*--------------------------------------------------
	void P_DeepCopyLines(line_t **target_array, line_t **source_array, size_t source_len)

		Make a deep copy of an array of line_t.

	Input Arguments:-
		target_array: The start of the array to copy into.
			This will be allocated by this function, so
			it should be freed beforehand.
		source_array: The start of the array to copy from.
		source_len: The length of the array to copy from.

	Return:-
		N/A
--------------------------------------------------*/

void P_DeepCopyLines(line_t **target_array, line_t **source_array, size_t source_len);


/*--------------------------------------------------
	void P_DeepCopySide(line_t *target, line_t *source)

		Make a deep copy of a single side_t.

	Input Arguments:-
		target: The struct to copy into.
		source: The struct to copy from.

	Return:-
		N/A
--------------------------------------------------*/

void P_DeepCopySide(side_t *target, side_t *source);


/*--------------------------------------------------
	void P_DeepCopySides(side_t **target_array, side_t **source_array, size_t source_len)

		Make a deep copy of an array of side_t.

	Input Arguments:-
		target_array: The start of the array to copy into.
			This will be allocated by this function, so
			it should be freed beforehand.
		source_array: The start of the array to copy from.
		source_len: The length of the array to copy from.

	Return:-
		N/A
--------------------------------------------------*/

void P_DeepCopySides(side_t **target_array, side_t **source_array, size_t source_len);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __P_DEEPCOPY__
