/// \file  menus/options-data-erase-profile.c
/// \brief Erase Profile Menu

#include "../k_menu.h"

menuitem_t OPTIONS_DataProfileErase[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_HandleProfileErase}, 0, 0},
};

menu_t OPTIONS_DataProfileEraseDef = {
	sizeof (OPTIONS_DataProfileErase) / sizeof (menuitem_t),
	&OPTIONS_DataEraseDef,
	0,
	OPTIONS_DataProfileErase,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawProfileErase,
	M_OptionsTick,
	NULL,
	NULL,
	NULL
};
