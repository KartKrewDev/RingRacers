// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/pause-kick.c
/// \brief Player Kick menu

#include "../../k_menu.h"
#include "../../s_sound.h"
#include "../../p_local.h"
#include "../../k_zvote.h"

struct playerkickmenu_s playerkickmenu;

static void M_PlayerKickHandler(INT32 choice)
{
	const UINT8 pid = 0;

	UINT8 kicktype = UINT8_MAX;

	(void)choice;

	if (menucmd[pid].dpad_lr != 0) // symmetrical in this case
	{
		S_StartSound(NULL, sfx_s3k5b);

		playerkickmenu.player = ((playerkickmenu.player + 8) % MAXPLAYERS);

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);

		playerkickmenu.player = ((playerkickmenu.player + 1) & 7) + (playerkickmenu.player & 8);

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);

		playerkickmenu.player = ((playerkickmenu.player + 7) & 7) + (playerkickmenu.player & 8);

		M_SetMenuDelay(pid);
	}

	else if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuExtraPressed(pid) && playerkickmenu.adminpowered)
	{
		kicktype = KICK_MSG_BANNED;
	}

	else if (M_MenuConfirmPressed(pid))
	{
		kicktype = KICK_MSG_KICKED;
	}

	if (kicktype != UINT8_MAX)
	{
		M_SetMenuDelay(pid);

		if (
			playeringame[playerkickmenu.player]
			&& P_IsMachineLocalPlayer(&players[playerkickmenu.player]) == false
			&& playerkickmenu.player != serverplayer
		)
		{
			if (playerkickmenu.adminpowered)
			{
				if (consoleplayer == serverplayer || IsPlayerAdmin(consoleplayer))
				{
					playerkickmenu.poke = (kicktype == KICK_MSG_BANNED) ? 16 : 12;
					SendKick(playerkickmenu.player, kicktype);
					return;
				}
			}
			else if (
				K_MinimalCheckNewMidVote(menucallvote) == true
#ifndef DEVELOP
				&& IsPlayerAdmin(playerkickmenu.player) == false
#endif
			)
			{
				M_ClearMenus(true);
				K_SendCallMidVote(menucallvote, playerkickmenu.player);
				return;
			}
		}

		playerkickmenu.poke = 8;
		S_StartSound(NULL, sfx_s3k7b);
	}
}

static menuitem_t PAUSE_KickHandler[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_PlayerKickHandler}, 0, 0},
};

static void M_KickHandlerTick(void)
{
	playerkickmenu.ticker++;

	if (playerkickmenu.poke)
		playerkickmenu.poke--;
}

menu_t PAUSE_KickHandlerDef = {
	sizeof(PAUSE_KickHandler) / sizeof(menuitem_t),
	&PAUSE_MainDef,
	0,
	PAUSE_KickHandler,
	0, 0,
	0, 0,
	0,
	NULL,
	0, 0,
	M_DrawKickHandler,
	NULL,
	M_KickHandlerTick,
	NULL,
	NULL,
	NULL,
};

void M_KickHandler(INT32 choice)
{
	playerkickmenu.adminpowered = (choice >= 0);

	PAUSE_KickHandlerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PAUSE_KickHandlerDef, true);
}
