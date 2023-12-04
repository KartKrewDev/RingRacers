/// \file  menus/options-data-addons.c
/// \brief Addon Options

#include "../k_menu.h"
#include "../filesrch.h" // addons cvars

menuitem_t OPTIONS_DataAddon[] =
{

	{IT_HEADER, "MENU", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Identify Addons via", "Set whether to consider the extension or contents of a file.",
		NULL, {.cvar = &cv_addons_md5}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Unsupported Files", "Sets whether non-addon files should be shown.",
		NULL, {.cvar = &cv_addons_showall}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "SEARCH", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Matching", "Set where to check for the text pattern when looking up addons via name.",
		NULL, {.cvar = &cv_addons_search_type}, 0, 0},

	{IT_STRING | IT_CVAR, "Case Sensitivity", "Set whether to consider the case when searching for addons..",
		NULL, {.cvar = &cv_addons_search_case}, 0, 0},

};

menu_t OPTIONS_DataAddonDef = {
	sizeof (OPTIONS_DataAddon) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataAddon,
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
