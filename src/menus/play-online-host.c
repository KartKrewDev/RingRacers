/// \file  menus/play-online-host.c
/// \brief MULTIPLAYER HOST SCREEN -- see mhost_e

#include "../k_menu.h"
#include "../s_sound.h"

// MULTIPLAYER HOST SCREEN -- see mhost_e
menuitem_t PLAY_MP_Host[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Display name for your game online. Other players will see this.",
		NULL, {.cvar = &cv_servername}, 0, 0},

	{IT_STRING | IT_CVAR, "Public Server", "Display or not your game in the Server Browser for other players.",
		NULL, {.cvar = &cv_advertise}, 0, 0},

	{IT_STRING | IT_CVAR, "Max. Players", "Set how many players can play at once. Others will spectate.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING | IT_KEYHANDLER, "Gamemode", "Choose the type of play on your server.",
	NULL, {.routine = M_HandleHostMenuGametype}, 0, 0},

	{IT_STRING | IT_CALL, "GO", "Select a map with the currently selected gamemode",
		NULL, {.routine = M_MPSetupNetgameMapSelect}, 0, 0},

};

menu_t PLAY_MP_HostDef = {
	sizeof (PLAY_MP_Host) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_Host,
	0, 0,
	0, 0,
	0,
	"NETMD2",
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPHost,
	M_MPOptSelectTick,	// This handles the unfolding options
	NULL,
	M_MPResetOpts,
	NULL
};

void M_MPHostInit(INT32 choice)
{

	(void)choice;
	mpmenu.modewinextend[0][0] = 1;
	M_SetupNextMenu(&PLAY_MP_HostDef, true);
	itemOn = mhost_go;
}

void M_HandleHostMenuGametype(INT32 choice)
{
	const UINT8 pid = 0;
	const UINT32 forbidden = GTR_FORBIDMP;

	(void)choice;

	if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
		return;
	}
	else if (menucmd[pid].dpad_lr > 0 || M_MenuConfirmPressed(pid))
	{
		M_NextMenuGametype(forbidden);
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_lr < 0)
	{
		M_PrevMenuGametype(forbidden);
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (menucmd[pid].dpad_ud > 0)
	{
		M_NextOpt();
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		M_PrevOpt();
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
}

void M_MPSetupNetgameMapSelect(INT32 choice)
{
	(void)choice;

	// Yep, we'll be starting a netgame.
	levellist.netgame = true;
	// Make sure we reset those
	levellist.levelsearch.timeattack = false;
	levellist.levelsearch.tutorial = false;
	levellist.levelsearch.checklocked = true;
	cupgrid.grandprix = false;

	// okay this is REALLY stupid but this fixes the host menu re-folding on itself when we go back.
	mpmenu.modewinextend[0][0] = 1;

	if (!M_LevelListFromGametype(menugametype))
	{
		S_StartSound(NULL, sfx_s3kb2);
		M_StartMessage("Online Play", va("No levels available for\n%s Mode!\n", gametypes[menugametype]->name), NULL, MM_NOTHING, NULL, NULL);
	}
}
