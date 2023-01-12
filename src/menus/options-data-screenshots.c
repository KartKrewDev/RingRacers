/// \file  menus/options-data-screenshots.c
/// \brief Screeshot Options

#include "../k_menu.h"
#include "../m_misc.h" // screenshot cvars

menuitem_t OPTIONS_DataScreenshot[] =
{

	{IT_HEADER, "SCREENSHOTS (F8)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Storage Location", "Sets where to store screenshots.",
		NULL, {.cvar = &cv_screenshot_option}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Custom Folder", "Specify which folder to save screenshots in.",
		NULL, {.cvar = &cv_screenshot_folder}, 24, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "GIF RECORDING (F9)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Storage Location", "Sets where to store GIFs",
		NULL, {.cvar = &cv_movie_option}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Custom Folder", "Specify which folder to save GIFs in.",
		NULL, {.cvar = &cv_movie_folder}, 24, 0},

};

menu_t OPTIONS_DataScreenshotDef = {
	sizeof (OPTIONS_DataScreenshot) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataScreenshot,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
