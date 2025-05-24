// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-data-1.c
/// \brief Data Options -- see dopt_e

#include "../k_menu.h"
#include "../discord.h" // discord rpc cvars

extern consvar_t cv_netdemosize;

// data options menu -- see dopt_e
menuitem_t OPTIONS_Data[] =
{

	{IT_STRING | IT_SUBMENU, "Video Recording...", "Options for recording clips.",
		NULL, {.submenu = &OPTIONS_DataScreenshotDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Advanced...", "Technical settings that you probably don't want to change.",
		NULL, {.submenu = &OPTIONS_DataAdvancedDef}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Replays...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Record Replays", "How the save prompt should appear.",
		NULL, {.cvar = &cv_recordmultiplayerdemos}, 0, 0},

	{IT_STRING | IT_CVAR, "Buffer Size (MB)", "Lets replays last longer with more players. Uses more RAM.",
		NULL, {.cvar = &cv_netdemosize}, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_HEADER, "Discord Rich Presence...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Rich Presence", "Allow Discord to display game info on your status.",
		NULL, {.cvar = &cv_discordrp}, 0, 0},

	{IT_STRING | IT_CVAR, "Streamer Mode", "Prevents the logging of some account information such as your tag in the console.",
		NULL, {.cvar = &cv_discordstreamer}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Ask to Join", "Allow other people to request joining your game from Discord.",
		NULL, {.cvar = &cv_discordasks}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Invites", "Set who is allowed to generate Discord invites to your game.",
		NULL, {.cvar = &cv_discordinvites}, 0, 0},
#endif

	{IT_SPACE | IT_DYBIGSPACE, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "\x85""Erase Data...", "Erase save data. Be careful, what's deleted is gone forever!",
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
