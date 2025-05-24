// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-online-join-ip.c
/// \brief MULTIPLAYER JOIN BY IP

#include "../k_menu.h"
#include "../v_video.h"
#include "../i_system.h" // I_OsPolling
#include "../i_video.h" // I_UpdateNoBlit
#include "../m_misc.h" // NUMLOGIP
#include "../f_finale.h" // g_wipeskiprender
#include "../s_sound.h"

menuitem_t PLAY_MP_JoinIP[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "IP: ", "Type the IPv4 address of the server.",
		NULL, {.cvar = &cv_dummyip}, 0, 0},

	{IT_STRING, "CONNECT ", "Attempt to connect to the server you entered the IP for.",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "LAST IPs JOINED:", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip1", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip2", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip3", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

};

menu_t PLAY_MP_JoinIPDef = {
	sizeof (PLAY_MP_JoinIP) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_JoinIP,
	0, 0,
	0, 0,
	0,
	"NETMD2",
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPJoinIP,
	M_DrawEggaChannel,
	M_MPOptSelectTick,	// This handles the unfolding options
	NULL,
	M_MPResetOpts,
	M_JoinIPInputs
};

void M_MPJoinIPInit(INT32 choice)
{

	(void)choice;
	mpmenu.modewinextend[2][0] = 1;
	M_SetupNextMenu(&PLAY_MP_JoinIPDef, true);
}

void M_PleaseWait(void)
{
	if (rendermode == render_none)
		return;

	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "PLEASE WAIT...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
}

// Attempts to join a given IP from the menu.
void M_JoinIP(const char *ipa)
{
	if (*(ipa) == '\0')	// Jack shit
	{
		return;
	}

	COM_BufAddText(va("connect \"%s\"\n", ipa));

	M_PleaseWait();
}

boolean M_JoinIPInputs(INT32 ch)
{

	const UINT8 pid = 0;
	(void) ch;

	if (itemOn == 1)	// connect field
	{
		// enter: connect
		if (M_MenuConfirmPressed(pid))
		{
			M_JoinIP(cv_dummyip.string);
			M_SetMenuDelay(pid);
			return true;
		}
	}
	else if (currentMenu->numitems - itemOn <= NUMLOGIP && M_MenuConfirmPressed(pid))	// On one of the last 3 options for IP rejoining
	{
		UINT8 index = NUMLOGIP - (currentMenu->numitems - itemOn);
		M_SetMenuDelay(pid);

		// Is there an address at this part of the table?
		if (*joinedIPlist[index][0])
			M_JoinIP(joinedIPlist[index][0]);
		else
			S_StartSound(NULL, sfx_lose);

		return true;	// eat input.
	}

	return false;
}
