/// \file  menus/extras-addons.c
/// \brief Addons menu!

#include "../k_menu.h"

menuitem_t MISC_AddonsMenu[] =
{
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, NULL,
		NULL, {.cvar = &cv_dummyaddonsearch}, 0, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL,
		NULL, {.routine = M_HandleAddons}, 0, 0},     // dummy menuitem for the control func
};

menu_t MISC_AddonsDef = {
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	NULL,
	0,
	MISC_AddonsMenu,
	50, 28,
	0, 0,
	0, 0,
	M_DrawAddons,
	M_AddonsRefresh,
	NULL,
	NULL,
	NULL
};
