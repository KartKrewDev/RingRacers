/// \file  menus/options-hud-1.c
/// \brief HUD Options

#include "../k_menu.h"
#include "../r_main.h"	// cv_showhud
#include "../v_video.h" // cv_constextsize
#include "../console.h" // console cvars

menuitem_t OPTIONS_HUD[] =
{

	{IT_STRING | IT_CVAR, "Show HUD (F3)", "Toggles HUD display. Great for taking screenshots!",
		NULL, {.cvar = &cv_showhud}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Speedometer", "Choose to what speed unit to display or toggle off the speedometer.",
		NULL, {.cvar = &cv_kartspeedometer}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Console Text Size", "Size of the text within the console.",
		NULL, {.cvar = &cv_constextsize}, 0, 0},

	// we spell words properly here.
	{IT_STRING | IT_CVAR, "Console Tint", "Change the background colour of the console.",
		NULL, {.cvar = &cons_backcolor}, 0, 0},

	{IT_STRING | IT_CVAR, "Show \"FOCUS LOST\"", "Displays \"FOCUS LOST\" when the game window isn't the active window.",
		NULL, {.cvar = &cv_showfocuslost}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Online HUD Options...", "HUD options related to the online chat box and other features.",
		NULL, {.submenu = &OPTIONS_HUDOnlineDef}, 0, 0},
};

menu_t OPTIONS_HUDDef = {
	sizeof (OPTIONS_HUD) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_HUD,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
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
