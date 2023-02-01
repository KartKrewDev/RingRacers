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
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

void Screenshot_option_Onchange(void)
{
	// Screenshot opt is at #3, 0 based array obv.
	OPTIONS_DataScreenshot[2].status =
		(cv_screenshot_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);

}

void Moviemode_mode_Onchange(void)
{
#if 0
	INT32 i, cstart, cend;
	for (i = op_screenshot_gif_start; i <= op_screenshot_apng_end; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_DISABLED;

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			cstart = op_screenshot_gif_start;
			cend = op_screenshot_gif_end;
			break;
		case MM_APNG:
			cstart = op_screenshot_apng_start;
			cend = op_screenshot_apng_end;
			break;
		default:
			return;
	}
	for (i = cstart; i <= cend; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_STRING|IT_CVAR;
#endif
}

void Moviemode_option_Onchange(void)
{
	// opt 7 in a 0 based array, you get the idea...
	OPTIONS_DataScreenshot[6].status =
		(cv_movie_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}
