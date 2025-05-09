// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef M_AVRECORDER_H
#define M_AVRECORDER_H

#include "typedef.h" // consvar_t

#ifdef __cplusplus
extern "C" {
#endif

void M_AVRecorder_AddCommands(void);

const char *M_AVRecorder_GetFileExtension(void);

// True if successully opened.
boolean M_AVRecorder_Open(const char *filename);

void M_AVRecorder_Close(void);

// Check whether AVRecorder is still valid. Call M_AVRecorder_Close if expired.
boolean M_AVRecorder_IsExpired(void);

const char *M_AVRecorder_GetCurrentFormat(void);

void M_AVRecorder_PrintCurrentConfiguration(void);

void M_AVRecorder_DrawFrameRate(void);

extern consvar_t
	cv_movie_custom_resolution,
	cv_movie_duration,
	cv_movie_fps,
	cv_movie_resolution,
	cv_movie_showfps,
	cv_movie_size,
	cv_movie_sound;

#ifdef __cplusplus
}; // extern "C"
#endif

#endif/*M_AVRECORDER_H*/
