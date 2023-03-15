/// \file  menus/options-data-screenshots.c
/// \brief Screeshot Options

#include "../k_menu.h"
#include "../m_misc.h" // screenshot cvars
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
#include "../m_avrecorder.h"
#endif

menuitem_t OPTIONS_DataScreenshot[] =
{
	{IT_HEADER, "MOVIE RECORDING (F9)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Recording Format", "What file format will movies will be recorded in?",
		NULL, {.cvar = &cv_moviemode}, 0, 0},

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	{IT_STRING | IT_CVAR, "Real-Time Data", "If enabled, shows fps, duration and filesize of recording in real-time.",
		NULL, {.cvar = &cv_movie_showfps}, 0, 0},
#else
	{IT_SPACE | IT_NOTHING, NULL, NULL,
		NULL, {NULL}, 0, 0},
#endif
};

menu_t OPTIONS_DataScreenshotDef = {
	sizeof (OPTIONS_DataScreenshot) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataScreenshot,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

void Moviemode_mode_Onchange(void)
{
	// opt 3 in a 0 based array, you get the idea...
	OPTIONS_DataScreenshot[2].status =
		(cv_moviemode.value == MM_AVRECORDER ? IT_CVAR|IT_STRING : IT_DISABLED);
}
