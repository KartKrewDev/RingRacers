// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-hud-inline.c
/// \brief Online HUD Options

#include "../k_menu.h"
#include "../console.h" // console cvars

menuitem_t OPTIONS_HUDOnline[] =
{

	{IT_STRING | IT_CVAR, "Show Chat", "Show chat by default or keep it hidden until you open it.",
		NULL, {.cvar = &cv_consolechat}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Box Tint", "Change the background color of the chat box.",
		NULL, {.cvar = &cons_backcolor}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Width", "Change the width of the Chat Box.",
		NULL, {.cvar = &cv_chatwidth}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Height", "Change the height of the Chat Box.",
		NULL, {.cvar = &cv_chatheight}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Message Fadeout Time (s)", "How long chat messages stay displayed with the chat closed.",
		NULL, {.cvar = &cv_chattime}, 0, 0},

	{IT_STRING | IT_CVAR, "Message Fadeout Tint", "Shows the tint for new chat messages when the box is closed.",
		NULL, {.cvar = &cv_chatbacktint}, 0, 0},

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
