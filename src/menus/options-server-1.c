// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-server-1.c
/// \brief Server Options

#include "../k_menu.h"
#include "../s_sound.h"

menuitem_t OPTIONS_Server[] =
{

	{IT_HEADER, "Broadcast...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Name of your server.",
		NULL, {.cvar = &cv_servername}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Contact", "How you should be contacted for Master Server moderation.",
		NULL, {.cvar = &cv_server_contact}, 0, 0},

	{IT_STRING | IT_CVAR, "Make Public", "Display your server in the Browser for other players to join.",
		NULL, {.cvar = &cv_advertise}, 0, 0},


	{IT_HEADER, "Players...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Maximum Players", "How many players can play at once.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING | IT_CVAR, "Maximum Connections", "How many players & spectators can connect to the server.",
		NULL, {.cvar = &cv_maxconnections}, 0, 0},

	{IT_STRING | IT_CVAR, "CPU Level", "Bots can fill unused slots. How strong should they be?",
		NULL, {.cvar = &cv_kartbot}, 0, 0},

	{IT_STRING | IT_CVAR, "Use Mobiums", "Should players should be rated on their performance?",
		NULL, {.cvar = &cv_kartusepwrlv}, 0, 0},

	{IT_STRING | IT_CVAR, "Antigrief Timer (seconds)", "How long can players stop progressing before they're removed?",
		NULL, {.cvar = &cv_antigrief}, 0, 0},


	{IT_HEADER, "Progression...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Intermission", "How long to stay on the result screen.",
		NULL, {.cvar = &cv_inttime}, 0, 0},

	{IT_STRING | IT_CVAR, "Map Progression", "How the next map is chosen.",
		NULL, {.cvar = &cv_advancemap}, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Timer", "How long players have to vote.",
		NULL, {.cvar = &cv_votetime}, 0, 0},


	{IT_HEADER, "Permissions...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Joining", "Let players connect to your server.",
		NULL, {.cvar = &cv_allownewplayer}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Downloads", "Let players download missing files from your server.",
		NULL, {.cvar = &cv_downloading}, 0, 0},

	{IT_STRING | IT_CVAR, "Pause Permissions", "Who is allowed to pause the game?",
		NULL, {.cvar = &cv_pause}, 0, 0},


	{IT_HEADER, "Text Chat...", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Mute Chat", "Prevent everyone but admins from sending chat messages.",
		NULL, {.cvar = &cv_mute}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Spam Protection", "Prevent too many messages from a single player.",
		NULL, {.cvar = &cv_chatspamprotection}, 0, 0},

	{IT_STRING | IT_CVAR, "Rounds Needed To Chat", "How many rounds players must complete before they can chat. Good vs. ban evaders.",
		NULL, {.cvar = &cv_gamestochat}, 0, 0},
		

	{IT_HEADER, "Voice Chat...",  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Server Voice Chat", "All voice chat will be enabled on your server.",
		NULL, {.cvar = &cv_voice_allowservervoice}, 0, 0},

	{IT_STRING | IT_CVAR, "Proximity Effects", "Player voices will be adjusted relative to you.",
		NULL, {.cvar = &cv_voice_proximity}, 0, 0},

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
