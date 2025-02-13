// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/main-profile-select.c
/// \brief Duplicate for main profile select.

#include "../k_menu.h"

menuitem_t MAIN_Profiles[] = {
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a profile to use or create a new Profile.",
		NULL, {.routine = M_HandleProfileSelect}, 0, 0},     // dummy menuitem for the control func
};

menu_t MAIN_ProfilesDef = {
	sizeof (MAIN_Profiles) / sizeof (menuitem_t),
	NULL,
	0,
	MAIN_Profiles,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	MBF_DRAWBGWHILEPLAYING,
	"FILE",
	2, 5,
	M_DrawProfileSelect,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
