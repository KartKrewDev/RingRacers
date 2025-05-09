// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-data-advanced.c
/// \brief Advanced Data Options

#include "../k_menu.h"
#include "../filesrch.h" // addons cvars

// advanced data options menu -- see daopt_e
menuitem_t OPTIONS_DataAdvanced[] =
{

	{IT_STRING | IT_SUBMENU, "Addons...", NULL,
		NULL, {.submenu = &OPTIONS_DataAdvancedAddonDef}, 0, 0},

	{IT_NOTHING | IT_SPACE, NULL, NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Replays (Advanced)...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Net Consistency Quality", "For filesize, how often do we write position data in online replays?",
		NULL, {.cvar = &cv_netdemosyncquality}, 0, 0},

};

menu_t OPTIONS_DataAdvancedDef = {
	sizeof (OPTIONS_DataAdvanced) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataAdvanced,
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
