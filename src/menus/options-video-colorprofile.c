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

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of the displayed image.",
		NULL, {.cvar = &cv_globalsaturation}, 0, 0},

	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of the displayed image.",
		NULL, {.cvar = &cv_globalgamma}, 0, 0},

	{IT_STRING | IT_CALL, "Reset All", "Reset the color profile to default settings.",
		NULL, {.routine = M_ColorProfileDefault}, 0, 0},
		
	{IT_HEADER, "Red...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of red in the displayed image.",
		NULL, {.cvar = &cv_rsaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of red in the displayed image.",
		NULL, {.cvar = &cv_rgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Hue", "Adjust the hue of red in the displayed image.",
		NULL, {.cvar = &cv_rhue}, 0, 0},
		
	{IT_HEADER, "Yellow...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of yellow in the displayed image.",
		NULL, {.cvar = &cv_ysaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of yellow in the displayed image.",
		NULL, {.cvar = &cv_ygamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Hue", "Adjust the hue of yellow in the displayed image.",
		NULL, {.cvar = &cv_yhue}, 0, 0},
		
	{IT_HEADER, "Green...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of green in the displayed image.",
		NULL, {.cvar = &cv_gsaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of green in the displayed image.",
		NULL, {.cvar = &cv_ggamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Hue", "Adjust the hue of green in the displayed image.",
		NULL, {.cvar = &cv_ghue}, 0, 0},
		
	{IT_HEADER, "Cyan...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of cyan in the displayed image.",
		NULL, {.cvar = &cv_csaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of cyan in the displayed image.",
		NULL, {.cvar = &cv_cgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Hue", "Adjust the hue of cyan in the displayed image.",
		NULL, {.cvar = &cv_chue}, 0, 0},
		
	{IT_HEADER, "Blue...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of blue in the displayed image.",
		NULL, {.cvar = &cv_bsaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of blue in the displayed image.",
		NULL, {.cvar = &cv_bgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Hue", "Adjust the hue of blue in the displayed image.",
		NULL, {.cvar = &cv_bhue}, 0, 0},
		
	{IT_HEADER, "Magenta...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Saturation", "Reduce the saturation of magenta in the displayed image.",
		NULL, {.cvar = &cv_msaturation}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Gamma", "Increase or decrease the brightness of magenta in the displayed image.",
		NULL, {.cvar = &cv_mgamma}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Hue", "Adjust the hue of magenta in the displayed image.",
		NULL, {.cvar = &cv_mhue}, 0, 0},

};

menu_t OPTIONS_VideoColorProfileDef = {
	sizeof (OPTIONS_VideoColorProfile) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoColorProfile,
	96, 80-8,
	SKINCOLOR_WHITE, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsColorProfile,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

// Set all color profile settings to the default values.
void M_ColorProfileDefault(INT32 choice)
{
	(void)choice;
	
	// The set value army approaches - gotta be a better way to handle this.
	CV_SetValue(&cv_globalsaturation, 10);
	CV_SetValue(&cv_rsaturation, 10);
	CV_SetValue(&cv_ysaturation, 10);
	CV_SetValue(&cv_gsaturation, 10);
	CV_SetValue(&cv_csaturation, 10);
	CV_SetValue(&cv_bsaturation, 10);
	CV_SetValue(&cv_msaturation, 10);
	CV_SetValue(&cv_globalgamma, 0);
	CV_SetValue(&cv_rgamma, 0);
	CV_SetValue(&cv_ygamma, 0);
	CV_SetValue(&cv_ggamma, 0);
	CV_SetValue(&cv_cgamma, 0);
	CV_SetValue(&cv_bgamma, 0);
	CV_SetValue(&cv_mgamma, 0);
	CV_SetValue(&cv_rhue, 0);
	CV_SetValue(&cv_yhue, 4);
	CV_SetValue(&cv_ghue, 8);
	CV_SetValue(&cv_chue, 12);
	CV_SetValue(&cv_bhue, 16);
	CV_SetValue(&cv_mhue, 20);
}
