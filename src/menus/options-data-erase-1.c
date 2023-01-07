/// \file  menus/options-data-erase-1.c
/// \brief Erase Data Menu

#include "../k_menu.h"

menuitem_t OPTIONS_DataErase[] =
{

	{IT_STRING | IT_CALL, "Erase Time Attack Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

	{IT_STRING | IT_CALL, "Erase Unlockable Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "Erase Profile Data...", "Select a Profile to erase.",
		NULL, {.routine = M_CheckProfileData}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "\x85\x45rase all Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

};

menu_t OPTIONS_DataEraseDef = {
	sizeof (OPTIONS_DataErase) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataErase,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
