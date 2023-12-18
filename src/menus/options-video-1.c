/// \file  menus/options-video-1.c
/// \brief Video Options

#include "../k_menu.h"
#include "../r_main.h"	// cv_skybox
#include "../v_video.h" // cv_globalgamma
#include "../r_fps.h" // fps cvars

// options menu
menuitem_t OPTIONS_Video[] =
{

	{IT_STRING | IT_CALL, "Set Resolution...", "Change the screen resolution for the game.",
		NULL, {.routine = M_VideoModeMenu}, 0, 0},

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	{IT_STRING | IT_CVAR, "Fullscreen", "Set whether you want to use fullscreen or windowed mode.",
		NULL, {.cvar = &cv_fullscreen}, 0, 0},
#endif

	{IT_STRING | IT_CVAR, "Vertical Sync", "Works with your screen to reduce image tearing and judder.",
		NULL, {.cvar = &cv_vidwait}, 0, 0},

	{IT_STRING | IT_CVAR, "FPS Cap", "Handles the frame rate of the game (35 to match game logic)",
		NULL, {.cvar = &cv_fpscap}, 0, 0},

	{IT_NOTHING|IT_SPACE, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Screen Tilting", "The view rotatation on inclines can be disabled to reduce motion sickness.",
		NULL, {.cvar = &cv_tilting}, 0, 0},

	{IT_STRING | IT_CVAR, "Reduce Visual Effects", "If on, some less-important particle cues will be hidden.",
		NULL, {.cvar = &cv_reducevfx}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Gamma", "Adjusts the overall brightness of the game.",
		NULL, {.cvar = &cv_globalgamma}, 0, 0},

	{IT_NOTHING|IT_SPACE, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Draw Distance", "How far objects can be drawn. A tradeoff between performance & visibility.",
		NULL, {.cvar = &cv_drawdist}, 0, 0},

	{IT_STRING | IT_CVAR, "Weather Draw Distance", "Affects how far weather visuals can be drawn. Lower values improve performance.",
		NULL, {.cvar = &cv_drawdist_precip}, 0, 0},

	{IT_STRING | IT_CVAR, "Enable Skyboxes", "Turning this off may improve performance, but reduces courses' background details.",
		NULL, {.cvar = &cv_skybox}, 0, 0},

#ifdef HWRENDER
	{IT_NOTHING|IT_SPACE, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Hardware Options...", "For usage and configuration of the OpenGL renderer.",
		NULL, {.submenu = &OPTIONS_VideoOGLDef}, 0, 0},
#endif

};

menu_t OPTIONS_VideoDef = {
	sizeof (OPTIONS_Video) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Video,
	32, 80-8,
	SKINCOLOR_PLAGUE, 0,
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
