// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../k_menu.h"
#include "../filesrch.h" // addons cvars

menuitem_t OPTIONS_DataAdvancedAddon[] =
{

	{IT_HEADER, "Addon List", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Identify Addons via", "Set whether to consider the extension or contents of a file.",
		NULL, {.cvar = &cv_addons_md5}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Unsupported Files", "Sets whether non-addon files should be shown.",
		NULL, {.cvar = &cv_addons_showall}, 0, 0},

	{IT_HEADER, "Addon Search", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Matching", "Set where to check for the text pattern when looking up addons via name.",
		NULL, {.cvar = &cv_addons_search_type}, 0, 0},

	{IT_STRING | IT_CVAR, "Case Sensitivity", "Set whether to consider the case when searching for addons.",
		NULL, {.cvar = &cv_addons_search_case}, 0, 0},

};

menu_t OPTIONS_DataAdvancedAddonDef = {
	sizeof (OPTIONS_DataAdvancedAddon) / sizeof (menuitem_t),
	&OPTIONS_DataAdvancedDef,
	0,
	OPTIONS_DataAdvancedAddon,
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
