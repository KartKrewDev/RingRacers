// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-video-gl.c
/// \brief OpenGL Options

#include "../k_menu.h"
#include "../r_main.h"	// cv_skybox
#include "../hardware/hw_main.h"	// gl consvars
#include "../i_video.h" // rendermode

extern consvar_t cv_menuframeskip;

// see vaopt_e
menuitem_t OPTIONS_VideoAdvanced[] =
{
	{IT_HEADER, "Performance...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Draw Distance", "How far objects can be drawn. A tradeoff between performance & visibility.",
		NULL, {.cvar = &cv_drawdist}, 0, 0},

	{IT_STRING | IT_CVAR, "Weather Draw Distance", "Affects how far weather visuals can be drawn. Lower values improve performance.",
		NULL, {.cvar = &cv_drawdist_precip}, 0, 0},

	{IT_STRING | IT_CVAR, "Enable Skyboxes", "Turning this off may improve performance, but reduces courses' background details.",
		NULL, {.cvar = &cv_skybox}, 0, 0},

	{IT_STRING | IT_CVAR, "Parallel Software", "Uses multiple CPU cores for the software renderer if available, for a FPS boost.",
		NULL, {.cvar = &cv_parallelsoftware}, 0, 0},

	{IT_STRING | IT_CVAR, "Extra Frame Skipping", "Skip 3D rendering frames while the menu is open.",
		NULL, {.cvar = &cv_menuframeskip}, 0, 0},


	{IT_HEADER, "Rendering Backend...", "Watch people get confused anyway!!",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "Legacy GL supports 3D models and true", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "perspective, but many visual features", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "are missing/broken.", "",
		NULL, {NULL}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "\x85Legacy GL will eventually be replaced.", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "\x85Please don't report visual bugs!", "",
		NULL, {NULL}, 0, 0}, // Unless you've got an MR to fix them.

	{IT_SPACE | IT_NOTHING, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Renderer", "If you don't know why you're changing this, leave it on Software!",
		NULL, {.cvar = &cv_renderer}, 0, 0},

	{IT_HEADER, "Legacy GL Options...", "Watch people get confused anyway!!",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "3D Models", "Use 3D models instead of sprites when applicable.",
		NULL, {.cvar = &cv_glmodels}, 0, 0},

	{IT_STRING | IT_CVAR, "Shaders", "Use GLSL Shaders. Turning them off increases performance at the expanse of visual quality.",
		NULL, {.cvar = &cv_glshaders}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Texture Quality", "Texture depth. Higher values are recommended.",
		NULL, {.cvar = &cv_scr_depth}, 0, 0},

	/*
	{IT_STRING | IT_CVAR, "Texture Filter", "Texture Filter. Nearest is recommended.",
		NULL, {.cvar = &cv_glfiltermode}, 0, 0},
	*/

	{IT_STRING | IT_CVAR, "Anisotropic", "Lower values will improve performance at a minor quality loss.",
		NULL, {.cvar = &cv_glanisotropicmode}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Sprite Billboarding", "Adjusts sprites when viewed from above or below to not make them appear flat.",
		NULL, {.cvar = &cv_glspritebillboarding}, 0, 0},

	{IT_STRING | IT_CVAR, "Software Perspective", "Emulates Software shearing when looking up or down. Not recommended.",
		NULL, {.cvar = &cv_glshearing}, 0, 0},
};

menu_t OPTIONS_VideoAdvancedDef = {
	sizeof (OPTIONS_VideoAdvanced) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoAdvanced,
	48, 80,
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

void M_RefreshAdvancedVideoOptions(void)
{
	OPTIONS_VideoAdvancedDef.numitems = rendermode == render_opengl
		? sizeof (OPTIONS_VideoAdvanced) / sizeof (menuitem_t)
		: vaopt_legacygl_begin;
}
