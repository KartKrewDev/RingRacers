/// \file  menus/options-profiles-1.c
/// \brief Profiles Menu

#include "../k_menu.h"

// profile select
menuitem_t OPTIONS_Profiles[] = {
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a Profile.",
		NULL, {.routine = M_HandleProfileSelect}, 0, 0},     // dummy menuitem for the control func
};

menu_t OPTIONS_ProfilesDef = {
	sizeof (OPTIONS_Profiles) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Profiles,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	2, 5,
	M_DrawProfileSelect,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
