/// \file  menus/options-1.c
/// \brief Options Menu

#include "../k_menu.h"

// options menu --  see mopt_e
menuitem_t OPTIONS_Main[] =
{

	{IT_STRING | IT_CALL, "Profile Setup", "Remap keys & buttons to your likings.",
		NULL, {.routine = M_ProfileSelectInit}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Video Options", "Change video settings such as the resolution.",
		NULL, {.submenu = &OPTIONS_VideoDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Sound Options", "Adjust various sound settings such as the volume.",
		NULL, {.submenu = &OPTIONS_SoundDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "HUD Options", "Options related to the Heads-Up Display.",
		NULL, {.submenu = &OPTIONS_HUDDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Gameplay Options", "Change various game related options",
		NULL, {.submenu = &OPTIONS_GameplayDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Server Options", "Change various specific options for your game server.",
		NULL, {.submenu = &OPTIONS_ServerDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Data Options", "Miscellaneous data options such as the screenshot format.",
		NULL, {.submenu = &OPTIONS_DataDef}, 0, 0},

	{IT_STRING | IT_CALL, "Tricks & Secrets", "Those who bother reading a game manual always get the edge over those who don't!",
		NULL, {.routine = M_Manual}, 0, 0},
};

// For options menu, the 'extra1' field will determine the background colour to use for... the background! (What a concept!)
menu_t OPTIONS_MainDef = {
	sizeof (OPTIONS_Main) / sizeof (menuitem_t),
	&MainDef,
	0,
	OPTIONS_Main,
	0, 0,
	SKINCOLOR_SLATE, 0,
	2, 5,
	M_DrawOptions,
	M_OptionsTick,
	NULL,
	NULL,
	M_OptionsInputs
};
