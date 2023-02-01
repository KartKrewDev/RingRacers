/// \file  menus/options-data-addons.c
/// \brief Addon Options

#include "../k_menu.h"
#include "../filesrch.h" // addons cvars

menuitem_t OPTIONS_DataAddon[] =
{

	{IT_HEADER, "MENU", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Location", "Where to start searching addons from in the menu.",
		NULL, {.cvar = &cv_addons_option}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Custom Folder", "Specify which folder to start searching from if the location is set to custom.",
		NULL, {.cvar = &cv_addons_folder}, 24, 0},

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
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

void Addons_option_Onchange(void)
{
	// Option 2 will always be the textbar.
	// (keep in mind this is a 0 indexed array and the first element is a header...)
	OPTIONS_DataAddon[2].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}
