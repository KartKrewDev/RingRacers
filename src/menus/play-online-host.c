/// \file  menus/play-online-host.c
/// \brief MULTIPLAYER HOST SCREEN -- see mhost_e

#include "../k_menu.h"
#include "../s_sound.h"
#include "../z_zone.h"
#include "../mserv.h"

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

	{IT_STRING | IT_ARROWS, "Gamemode", "Choose the type of play on your server.",
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
	M_DrawEggaChannel,
	M_MPOptSelectTick,	// This handles the unfolding options
	NULL,
	M_MPResetOpts,
	NULL
};

void M_PopupMasterServerRules(void)
{
#ifdef MASTERSERVER
	if (cv_advertise.value && (serverrunning || currentMenu == &PLAY_MP_HostDef))
	{
		char *rules = GetMasterServerRules();

		if (rules != NULL)
		{
			M_StartMessage("Server List Rules", rules, NULL, MM_NOTHING, NULL, NULL);
			Z_Free(rules);
		}
	}
#endif
}

void M_MPHostInit(INT32 choice)
{
	(void)choice;
	mpmenu.modewinextend[0][0] = 1;
	M_SetupNextMenu(&PLAY_MP_HostDef, true);
	itemOn = mhost_go;

	Get_rules();
	// There's one downside to doing it this way:
	// if you turn advertise on via the console,
	// then access this menu for the first time,
	// no rules will pop up because they haven't
	// arrived yet.
	M_PopupMasterServerRules();
	// HOWEVER, this menu popup isn't for people
	// who know how to use the Developer Console.
	// People who CAN do that should already know
	// what kind of service they're connecting to.
	// (it'll still appear in the logs later, too!)
}

void M_HandleHostMenuGametype(INT32 choice)
{
	const UINT32 forbidden = GTR_FORBIDMP;

	if (choice > 0)
	{
		M_NextMenuGametype(forbidden);
	}
	else if (choice == -1)
	{
		menugametype = GT_RACE;
		M_PrevMenuGametype(forbidden);
		M_NextMenuGametype(forbidden);
	}
	else
	{
		M_PrevMenuGametype(forbidden);
	}
}

void M_MPSetupNetgameMapSelect(INT32 choice)
{
	(void)choice;

	// Yep, we'll be starting a netgame.
	levellist.netgame = true;
	// Make sure we reset those
	levellist.levelsearch.timeattack = false;
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
