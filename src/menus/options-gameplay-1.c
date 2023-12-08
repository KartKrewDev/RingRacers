/// \file  menus/options-gameplay-1.c
/// \brief Gameplay Options -- see gopt_e

#include "../k_menu.h"

menuitem_t OPTIONS_Gameplay[] =
{

	{IT_STRING | IT_CVAR, "Game Speed", "Change Game Speed for the next map.",
		NULL, {.cvar = &cv_kartspeed}, 0, 0},

	{IT_STRING | IT_CVAR, "Base Lap Count", "Change how many laps must be completed per race.",
		NULL, {.cvar = &cv_numlaps}, 0, 0},

	{IT_STRING | IT_CVAR, "Frantic Items", "Make item odds crazier with more powerful items!",
		NULL, {.cvar = &cv_kartfrantic}, 0, 0},

	{IT_STRING | IT_CVAR, "Encore Mode", "Forces Encore Mode on for the next map.",
		NULL, {.cvar = &cv_kartencore}, 0, 0},

	{IT_STRING | IT_CVAR, "Exit Countdown", "How long players have to finish after 1st place finishes.",
		NULL, {.cvar = &cv_countdowntime}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Time Limit", "Change the time limit for Battle rounds.",
		NULL, {.cvar = &cv_timelimit}, 0, 0},

	{IT_STRING | IT_CVAR, "Starting Bumpers", "Change how many bumpers player start with in Battle.",
		NULL, {.cvar = &cv_kartbumpers}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},
	
	{IT_STRING | IT_CVAR, "Minimum Input Delay", "Practice for online play! Higher = more delay, 0 = instant response.",
		NULL, {.cvar = &cv_mindelay}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Random Item Toggles...", "Change which items to enable for your games.",
		NULL, {.submenu = &OPTIONS_GameplayItemsDef}, 0, 0},

};

menu_t OPTIONS_GameplayDef = {
	sizeof (OPTIONS_Gameplay) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Gameplay,
	48, 80,
	SKINCOLOR_SCARLET, 0,
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
