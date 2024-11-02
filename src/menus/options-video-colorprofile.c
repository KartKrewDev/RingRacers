// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// This file created by Freaky Mutant Man.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-video-colorprofile.c
/// \brief Color Profile Options

#include "../k_menu.h"
#include "../v_video.h" // cv_globalgamma
#include "../r_fps.h" // fps cvars

// color profile menu
menuitem_t OPTIONS_VideoColorProfile[] =
{

	{IT_STRING | IT_CVAR, "Global Saturation", "Reduce the saturation of the displayed image.",
		NULL, {.cvar = &cv_globalsaturation}, 0, 0},

	{IT_STRING | IT_CVAR, "Global Gamma", "Increase or decrease the brightness of the displayed image.",
		NULL, {.cvar = &cv_globalgamma}, 0, 0},

	{IT_NOTHING|IT_SPACE, NULL, NULL,
		NULL, {NULL}, 0, 0},
		
	{IT_HEADER, "Red...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Red Saturation", "Reduce the saturation of red in the displayed image.",
		NULL, {.cvar = &cv_rsaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Red Gamma", "Increase or decrease the brightness of red in the displayed image.",
		NULL, {.cvar = &cv_rgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Red Hue", "Adjust the hue of red in the displayed image.",
		NULL, {.cvar = &cv_rhue}, 0, 0},
		
	{IT_HEADER, "Yellow...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Yellow Saturation", "Reduce the saturation of yellow in the displayed image.",
		NULL, {.cvar = &cv_ysaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Yellow Gamma", "Increase or decrease the brightness of yellow in the displayed image.",
		NULL, {.cvar = &cv_ygamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Yellow Hue", "Adjust the hue of yellow in the displayed image.",
		NULL, {.cvar = &cv_yhue}, 0, 0},
		
	{IT_HEADER, "Green...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Green Saturation", "Reduce the saturation of green in the displayed image.",
		NULL, {.cvar = &cv_gsaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Green Gamma", "Increase or decrease the brightness of green in the displayed image.",
		NULL, {.cvar = &cv_ggamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Green Hue", "Adjust the hue of green in the displayed image.",
		NULL, {.cvar = &cv_ghue}, 0, 0},
		
	{IT_HEADER, "Cyan...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Cyan Saturation", "Reduce the saturation of cyan in the displayed image.",
		NULL, {.cvar = &cv_csaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Cyan Gamma", "Increase or decrease the brightness of cyan in the displayed image.",
		NULL, {.cvar = &cv_cgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Cyan Hue", "Adjust the hue of cyan in the displayed image.",
		NULL, {.cvar = &cv_chue}, 0, 0},
		
	{IT_HEADER, "Blue...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Blue Saturation", "Reduce the saturation of blue in the displayed image.",
		NULL, {.cvar = &cv_bsaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Blue Gamma", "Increase or decrease the brightness of blue in the displayed image.",
		NULL, {.cvar = &cv_bgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Blue Hue", "Adjust the hue of blue in the displayed image.",
		NULL, {.cvar = &cv_bhue}, 0, 0},
		
	{IT_HEADER, "Magenta...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Magenta Saturation", "Reduce the saturation of magenta in the displayed image.",
		NULL, {.cvar = &cv_msaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Magenta Gamma", "Increase or decrease the brightness of magenta in the displayed image.",
		NULL, {.cvar = &cv_mgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Magenta Hue", "Adjust the hue of magenta in the displayed image.",
		NULL, {.cvar = &cv_mhue}, 0, 0},

};

menu_t OPTIONS_VideoColorProfileDef = {
	sizeof (OPTIONS_VideoColorProfile) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoColorProfile,
	48, 80-8,
	SKINCOLOR_LAVENDER, 0,
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
