/// \file  menus/play-online-server-browser.c
/// \brief Online server, CORE / MODDED

#include "../k_menu.h"

// MULTIPLAYER ROOM SELECT (CORE / MODDED)
menuitem_t PLAY_MP_RoomSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_MPRoomSelect}, 0, 0},
};

menu_t PLAY_MP_RoomSelectDef = {
	sizeof (PLAY_MP_RoomSelect) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_RoomSelect,
	0, 0,
	0, 0,
	0, 0,
	M_DrawMPRoomSelect,
	M_MPRoomSelectTick,
	NULL,
	NULL,
	NULL
};

// SERVER BROWSER
menuitem_t PLAY_MP_ServerBrowser[] =
{

	{IT_STRING | IT_CVAR, "SORT BY", NULL,	// tooltip MUST be null.
		NULL, {.cvar = &cv_serversort}, 0, 0},

	{IT_STRING, "REFRESH", NULL,
		NULL, {NULL}, 0, 0},

	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

menu_t PLAY_MP_ServerBrowserDef = {
	sizeof (PLAY_MP_ServerBrowser) / sizeof (menuitem_t),
	&PLAY_MP_RoomSelectDef,
	0,
	PLAY_MP_ServerBrowser,
	32, 36,
	0, 0,
	0, 0,
	M_DrawMPServerBrowser,
	M_MPServerBrowserTick,
	NULL,
	NULL,
	M_ServerBrowserInputs
};
