// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-data-screenshots.c
/// \brief Screeshot Options

#include "../k_menu.h"
#include "../m_misc.h" // screenshot cvars
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
#include "../m_avrecorder.h"
#endif

menuitem_t OPTIONS_DataScreenshot[] =
{
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	{IT_HEADER, "WebM Recording (F9)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Max. Duration (s)", "Automatically stop recording after this much time.",
		NULL, {.cvar = &cv_movie_duration}, 0, 0},

	{IT_STRING | IT_CVAR, "Max. File Size (MB)", "Automatically stop recording when the video file reaches this size.",
		NULL, {.cvar = &cv_movie_size}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Real-Time Data", "If enabled, shows fps, duration and filesize of recording in real-time.",
		NULL, {.cvar = &cv_movie_showfps}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Resolution", "Video resolution.",
		NULL, {.cvar = &cv_movie_resolution}, 0, 0},

	{IT_STRING | IT_CVAR, "Max. FPS", "Video framerate.",
		NULL, {.cvar = &cv_movie_fps}, 0, 0},

	{IT_STRING | IT_CVAR, "Record Audio", "Record audio.",
		NULL, {.cvar = &cv_movie_sound}, 0, 0},
#endif

	{IT_HEADER, "Lossless Recording (F10)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Recording Format", "Which file format will lossless recordings use?",
		NULL, {.cvar = &cv_lossless_recorder}, 0, 0},

};

menu_t OPTIONS_DataScreenshotDef = {
	sizeof (OPTIONS_DataScreenshot) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataScreenshot,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
