/// \file  menus/options-data-1.c
/// \brief Data Options -- see dopt_e

#include "../k_menu.h"

// data options menu -- see dopt_e
menuitem_t OPTIONS_Data[] =
{

	{IT_STRING | IT_SUBMENU, "Media Options...", "Set options relative to screenshot and movie capture.",
		NULL, {.submenu = &OPTIONS_DataScreenshotDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Addon Options...", "Set options relative to the addons menu.",
		NULL, {.submenu = &OPTIONS_DataAddonDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Replay Options...", "Set options relative to replays.",
		NULL, {.submenu = &OPTIONS_DataReplayDef}, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_STRING | IT_SUBMENU, "Discord Options...", "Set options relative to Discord Rich Presence.",
		NULL, {.submenu = &OPTIONS_DataDiscordDef}, 0, 0},
#endif

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "\x85""Erase Data...", "Erase specific data. Be careful, what's deleted is gone forever!",
		NULL, {.submenu = &OPTIONS_DataEraseDef}, 0, 0},

};

menu_t OPTIONS_DataDef = {
	sizeof (OPTIONS_Data) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Data,
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
