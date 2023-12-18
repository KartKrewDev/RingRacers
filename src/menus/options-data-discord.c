/// \file  menus/options-data-discord.c
/// \brief Discord Rich Presence Options

#include "../k_menu.h"
#include "../discord.h" // discord rpc cvars

menuitem_t OPTIONS_DataDiscord[] =
{
	{IT_STRING | IT_CVAR, "Rich Presence", "Allow Discord to display game info on your status.",
		NULL, {.cvar = &cv_discordrp}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "RICH PRESENCE SETTINGS", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Streamer Mode", "Prevents the logging of some account information such as your tag in the console.",
		NULL, {.cvar = &cv_discordstreamer}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Ask to Join", "Allow other people to request joining your game from Discord.",
		NULL, {.cvar = &cv_discordasks}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Invites", "Set who is allowed to generate Discord invites to your game.",
		NULL, {.cvar = &cv_discordinvites}, 0, 0},

};

menu_t OPTIONS_DataDiscordDef = {
	sizeof (OPTIONS_DataDiscord) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataDiscord,
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
