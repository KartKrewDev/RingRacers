// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/discord-requests.c
/// \brief Discord Requests menu

#ifdef HAVE_DISCORDRPC
#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../discord.h"

struct discordrequestmenu_s discordrequestmenu;

static void M_DiscordRequestHandler(INT32 choice)
{
	const UINT8 pid = 0;
	(void)choice;

	if (discordrequestmenu.confirmDelay > 0)
		return;

	if (M_MenuConfirmPressed(pid))
	{
		Discord_Respond(discordRequestList->userID, DISCORD_REPLY_YES);
		discordrequestmenu.confirmAccept = true;
		discordrequestmenu.confirmDelay = discordrequestmenu.confirmLength;
		S_StartSound(NULL, sfx_s3k63);
	}
	else if (M_MenuBackPressed(pid))
	{
		Discord_Respond(discordRequestList->userID, DISCORD_REPLY_NO);
		discordrequestmenu.confirmAccept = false;
		discordrequestmenu.confirmDelay = discordrequestmenu.confirmLength;
		S_StartSound(NULL, sfx_s3kb2);
	}
}

static menuitem_t MISC_DiscordRequests[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_DiscordRequestHandler}, 0, 0},
};

static void M_DiscordRequestTick(void)
{
	discordrequestmenu.ticker++;

	if (discordrequestmenu.confirmDelay > 0)
	{
		discordrequestmenu.confirmDelay--;

		if (discordrequestmenu.confirmDelay == 0)
		{
			discordrequestmenu.removeRequest = true;
		}
	}

	if (discordrequestmenu.removeRequest == true)
	{
		DRPC_RemoveRequest(discordRequestList);

		if (discordRequestList == NULL)
		{
			// No other requests
			PAUSE_Main[mpause_discordrequests].status = IT_DISABLED;

			if (currentMenu->prevMenu)
			{
				M_SetupNextMenu(currentMenu->prevMenu, true);
				itemOn = mpause_continue;
			}
			else
				M_ClearMenus(true);
		}

		discordrequestmenu.removeRequest = false;
	}
}

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
	M_DrawDiscordRequests,
	NULL,
	M_DiscordRequestTick,
	NULL,
	NULL,
	NULL,
};

void M_DiscordRequests(INT32 choice)
{
	(void)choice;
	static const tic_t confirmLength = 3*TICRATE/4;

	discordrequestmenu.confirmLength = confirmLength;
	MISC_DiscordRequestsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_DiscordRequestsDef, true);
}

const char *M_GetDiscordName(discordRequest_t *r)
{
	if (r == NULL)
		return "";

	if (cv_discordstreamer.value)
		return DRPC_HideUsername(r->username);

	return r->username;
}

#endif // HAVE_DISCORDRPC
