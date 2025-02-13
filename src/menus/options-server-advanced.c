// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-server-advanced.c
/// \brief Advanced Server Options

#include "../k_menu.h"

menuitem_t OPTIONS_ServerAdvanced[] =
{

	{IT_HEADER, "Master Server", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Browser Address", "Default is \'https://ms.kartkrew.org/ms/api\'",
		NULL, {.cvar = &cv_masterserver}, 0, 0},

	{IT_STRING | IT_CVAR, "Debug Log", "Save technical info about communication with the Master Server.",
		NULL, {.cvar = &cv_masterserver_debug}, 0, 0},


	{IT_HEADER, "Network Connection", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Delay Limit (tics)", "Players above the delay limit will get kicked from the server.",
		NULL, {.cvar = &cv_maxping}, 0, 0},

	{IT_STRING | IT_CVAR, "Delay Timeout (seconds)", "Players must be above the delay limit for this long before being kicked.",
		NULL, {.cvar = &cv_pingtimeout}, 0, 0},

	{IT_STRING | IT_CVAR, "Connection Timeout (tics)", "Players not giving any network activity for this long are kicked.",
		NULL, {.cvar = &cv_nettimeout}, 0, 0},

	{IT_STRING | IT_CVAR, "Join Timeout (tics)", "Players taking too long to join are kicked.",
		NULL, {.cvar = &cv_jointimeout}, 0, 0},


	{IT_HEADER, "Addon Downloading", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Max File Transfer", "Maximum size of each file that joining players may download. (KB)",
		NULL, {.cvar = &cv_maxsend}, 0, 0},

	{IT_STRING | IT_CVAR, "File Transfer Speed", "File transfer packet rate. Larger values send more data.",
		NULL, {.cvar = &cv_downloadspeed}, 0, 0},


	{IT_HEADER, "Logging", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Log Joiner IPs", "Shows the IP address of connecting players as they join.",
		NULL, {.cvar = &cv_showjoinaddress}, 0, 0},

	{IT_STRING | IT_CVAR, "Log Resynch", "Shows which players need resynchronization.",
		NULL, {.cvar = &cv_blamecfail}, 0, 0},

	{IT_STRING | IT_CVAR, "Log Transfers", "Shows when players are downloading files from you.",
		NULL, {.cvar = &cv_noticedownload}, 0, 0},
};

menu_t OPTIONS_ServerAdvancedDef = {
	sizeof (OPTIONS_ServerAdvanced) / sizeof (menuitem_t),
	&OPTIONS_ServerDef,
	0,
	OPTIONS_ServerAdvanced,
	48, 70,	// This menu here is slightly higher because there's a lot of options...
	SKINCOLOR_VIOLET, 0,
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
