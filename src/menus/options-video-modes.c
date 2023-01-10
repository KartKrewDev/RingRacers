/// \file  menus/options-video-modes.c
/// \brief Video modes (resolutions)

#include "../k_menu.h"

menuitem_t OPTIONS_VideoModes[] = {

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a resolution.",
		NULL, {.routine = M_HandleVideoModes}, 0, 0},     // dummy menuitem for the control func

};

menu_t OPTIONS_VideoModesDef = {
	sizeof (OPTIONS_VideoModes) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoModes,
	48, 80,
	SKINCOLOR_PLAGUE, 0,
	2, 5,
	M_DrawVideoModes,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
