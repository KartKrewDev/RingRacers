/// \file  menus/transient/discord-requests.c
/// \brief Discord Requests menu

#ifdef HAVE_DISCORDRPC
#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../discord.h"

struct discordrequestmenu_s discordrequestmenu;

static menuitem_t MISC_DiscordRequests[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_DiscordRequestHandler}, 0, 0},
};

menu_t MISC_DiscordRequestsDef = {
	sizeof(MISC_DiscordRequests) / sizeof(menuitem_t),
	&PAUSE_MainDef,
	0,
	MISC_DiscordRequests,
	0, 0,
	0, 0,
	0,
	NULL,
	0, 0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

 void M_DiscordRequestHandler(INT32 choice)
{
	const UINT8 pid = 0;
	(void)choice;

	if (M_MenuConfirmPressed(pid))
	{
			M_SetMenuDelay(pid);

			Discord_Respond(discordRequestList->userID, DISCORD_REPLY_YES);
			discordrequestmenu.confirmAccept = true;
			discordrequestmenu.confirmDelay = menucmd[pid].delay;
			S_StartSound(NULL, sfx_s3k63);
	}
	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);

		Discord_Respond(discordRequestList->userID, DISCORD_REPLY_NO);
		discordrequestmenu.confirmAccept = false;
		discordrequestmenu.confirmDelay = menucmd[pid].delay;
		S_StartSound(NULL, sfx_s3kb2);
	}
}

void M_DiscordRequests(INT32 choice)
{
	(void)choice;

	MISC_SoundTestDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_DiscordRequestsDef, false);
}

#endif // HAVE_DISCORDRPC
