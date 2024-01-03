/// \file  menus/options-server-1.c
/// \brief Server Options

#include "../k_menu.h"

menuitem_t OPTIONS_Server[] =
{

	{IT_HEADER, "Advertising...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Change the name of your server.",
		NULL, {.cvar = &cv_servername}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Contact", "Where you should be contacted for Master Server moderation.",
		NULL, {.cvar = &cv_server_contact}, 0, 0},

	{IT_STRING | IT_CVAR, "Advertise", "Display your game in the Server Browser for other players.",
		NULL, {.cvar = &cv_advertise}, 0, 0},


	{IT_HEADER, "Progression...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Intermission", "Set how long to stay on the result screen.",
		NULL, {.cvar = &cv_inttime}, 0, 0},

	{IT_STRING | IT_CVAR, "Map Progression", "Set how the next map is chosen.",
		NULL, {.cvar = &cv_advancemap}, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Timer", "Set how long players have to vote.",
		NULL, {.cvar = &cv_votetime}, 0, 0},


	{IT_HEADER, "Joining...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Maximum Players", "How many players can play at once.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING | IT_CVAR, "Maximum Connections", "How many players & spectators can connect to the server.",
		NULL, {.cvar = &cv_maxconnections}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Joining", "Sets whether players can connect to your server.",
		NULL, {.cvar = &cv_allownewplayer}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Downloads", "Allows joiners to download missing files from you.",
		NULL, {.cvar = &cv_downloading}, 0, 0},

	{IT_STRING | IT_CVAR, "Pause Permissions", "Sets who can pause the game.",
		NULL, {.cvar = &cv_pause}, 0, 0},


	{IT_HEADER, "Chat...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Mute Chat", "Prevents non-admins from sending chat messages.",
		NULL, {.cvar = &cv_mute}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Spam Protection", "Prevents too many message from a single player.",
		NULL, {.cvar = &cv_chatspamprotection}, 0, 0},


	{IT_SPACE | IT_DYBIGSPACE, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Advanced...", "Advanced options. Be careful when messing with these!",
		NULL, {.submenu = &OPTIONS_ServerAdvancedDef}, 0, 0},

};

menu_t OPTIONS_ServerDef = {
	sizeof (OPTIONS_Server) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Server,
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
