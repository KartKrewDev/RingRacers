/// \file  menus/options-hud-inline.c
/// \brief Online HUD Options

#include "../k_menu.h"

menuitem_t OPTIONS_HUDOnline[] =
{

	{IT_STRING | IT_CVAR, "Chat Mode", "Choose whether to display chat in its own window or the console.",
		NULL, {.cvar = &cv_consolechat}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Box Tint", "Changes the background colour of the chat box.",
		NULL, {.cvar = &cv_chatbacktint}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Width", "Change the width of the Chat Box",
		NULL, {.cvar = &cv_chatwidth}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Height", "Change the height of the Chat Box",
		NULL, {.cvar = &cv_chatheight}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Message Fadeout Time", "How long chat messages stay displayed with the chat closed.",
		NULL, {.cvar = &cv_chattime}, 0, 0},

	{IT_STRING | IT_CVAR, "Spam Protection", "Prevents too many message from a single player from being displayed.",
		NULL, {.cvar = &cv_chatspamprotection}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Local Ping Display", "In netgames, displays your ping at the lower right corner of the screen.",
		NULL, {.cvar = &cv_showping}, 0, 0},

};

menu_t OPTIONS_HUDOnlineDef = {
	sizeof (OPTIONS_HUDOnline) / sizeof (menuitem_t),
	&OPTIONS_HUDDef,
	0,
	OPTIONS_HUDOnline,
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
