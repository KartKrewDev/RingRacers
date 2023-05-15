/// \file  menus/transient/pause-game.c
/// \brief In-game/pause menus

#include "../../k_menu.h"
#include "../../k_grandprix.h" // K_CanChangeRules
#include "../../m_cond.h"
#include "../../s_sound.h"

#ifdef HAVE_DISCORDRPC
#include "../../discord.h"
#endif

// ESC pause menu
// Since there's no descriptions to each item, we'll use the descriptions as the names of the patches we want to draw for each option :)

menuitem_t PAUSE_Main[] =
{

	{IT_STRING | IT_CALL, "ADDONS", "M_ICOADD",
		NULL, {.routine = M_Addons}, 0, 0},

	{IT_STRING | IT_CALL, "STEREO MODE", "M_ICOSTM",
		NULL, {.routine = M_SoundTest}, 0, 0},

	{IT_STRING | IT_KEYHANDLER, "GAMETYPE", "M_ICOGAM",
		NULL, {.routine = M_HandlePauseMenuGametype}, 0, 0},

	{IT_STRING | IT_CALL, "CHANGE MAP", "M_ICOMAP",
		NULL, {.routine = M_LevelSelectInit}, 0, -1},

	{IT_STRING | IT_CALL, "RESTART MAP", "M_ICORE",
		NULL, {.routine = M_RestartMap}, 0, 0},

	{IT_STRING | IT_CALL, "TRY AGAIN", "M_ICORE",
		NULL, {.routine = M_TryAgain}, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_STRING | IT_CALL, "DISCORD REQUESTS", "M_ICODIS",
		NULL, {.routine = M_DiscordRequests}, 0, 0},
#endif

	{IT_STRING | IT_CALL, "RESUME GAME", "M_ICOUNP",
		NULL, {.routine = M_QuitPauseMenu}, 0, 0},

	{IT_STRING | IT_CALL, "SPECTATE", "M_ICOSPC",
		NULL, {.routine = M_ConfirmSpectate}, 0, 0},

	{IT_STRING | IT_CALL, "ENTER GAME", "M_ICOENT",
		NULL, {.routine = M_ConfirmEnterGame}, 0, 0},

	{IT_STRING | IT_CALL, "CANCEL JOIN", "M_ICOSPC",
		NULL, {.routine = M_ConfirmSpectate}, 0, 0},

	{IT_STRING | IT_SUBMENU, "JOIN OR SPECTATE", "M_ICOENT",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "PLAYER SETUP", "M_ICOCHR",
		NULL, {.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "OPTIONS", "M_ICOOPT",
		NULL, {.routine = M_InitOptions}, 0, 0},

	{IT_STRING | IT_CALL, "EXIT GAME", "M_ICOEXT",
		NULL, {.routine = M_EndGame}, 0, 0},
};

menu_t PAUSE_MainDef = {
	sizeof (PAUSE_Main) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_Main,
	0, 0,
	0, 0,
	0,
	NULL,
	1, 10,	// For transition with some menus!
	M_DrawPause,
	M_PauseTick,
	NULL,
	NULL,
	M_PauseInputs
};

static void Dummymenuplayer_OnChange(void);

static CV_PossibleValue_t dummymenuplayer_cons_t[] = {{0, "NOPE"}, {1, "P1"}, {2, "P2"}, {3, "P3"}, {4, "P4"}, {0, NULL}};

//static consvar_t cv_dummymenuplayer = CVAR_INIT ("dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange);
consvar_t cv_dummymenuplayer = CVAR_INIT ("dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange);

struct pausemenu_s pausemenu;

static void Dummymenuplayer_OnChange(void)
{
	if (cv_dummymenuplayer.value < 1)
		CV_StealthSetValue(&cv_dummymenuplayer, splitscreen+1);
	else if (cv_dummymenuplayer.value > splitscreen+1)
		CV_StealthSetValue(&cv_dummymenuplayer, 1);
}

// Pause menu!
void M_OpenPauseMenu(void)
{
	INT32 i = 0;

	currentMenu = &PAUSE_MainDef;

	// Ready the variables
	pausemenu.ticker = 0;

	pausemenu.offset = 0;
	pausemenu.openoffset = 256;
	pausemenu.closing = false;

	currentMenu->lastOn = mpause_continue;	// Make sure we select "RESUME GAME" by default

	// Now the hilarious balancing act of deciding what options should be enabled and which ones shouldn't be!
	// By default, disable anything sensitive:

	PAUSE_Main[mpause_addons].status = IT_DISABLED;
	PAUSE_Main[mpause_stereo].status = IT_DISABLED;
	PAUSE_Main[mpause_changegametype].status = IT_DISABLED;
	PAUSE_Main[mpause_switchmap].status = IT_DISABLED;
	PAUSE_Main[mpause_restartmap].status = IT_DISABLED;
	PAUSE_Main[mpause_tryagain].status = IT_DISABLED;
#ifdef HAVE_DISCORDRPC
	PAUSE_Main[mpause_discordrequests].status = IT_DISABLED;
#endif

	PAUSE_Main[mpause_spectate].status = IT_DISABLED;
	PAUSE_Main[mpause_entergame].status = IT_DISABLED;
	PAUSE_Main[mpause_canceljoin].status = IT_DISABLED;
	PAUSE_Main[mpause_spectatemenu].status = IT_DISABLED;
	PAUSE_Main[mpause_psetup].status = IT_DISABLED;

	Dummymenuplayer_OnChange();	// Make sure the consvar is within bounds of the amount of splitscreen players we have.

	if (M_SecretUnlocked(SECRET_SOUNDTEST, true))
	{
		PAUSE_Main[mpause_stereo].status = IT_STRING | IT_CALL;
	}

	if (K_CanChangeRules(false))
	{
		PAUSE_Main[mpause_psetup].status = IT_STRING | IT_CALL;

		if (server || IsPlayerAdmin(consoleplayer))
		{
			PAUSE_Main[mpause_changegametype].status = IT_STRING | IT_KEYHANDLER;
			menugametype = gametype;

			PAUSE_Main[mpause_switchmap].status = IT_STRING | IT_CALL;
			PAUSE_Main[mpause_restartmap].status = IT_STRING | IT_CALL;

			if (M_SecretUnlocked(SECRET_ADDONS, true))
			{
				PAUSE_Main[mpause_addons].status = IT_STRING | IT_CALL;
			}
		}
	}
	else if (!netgame && !demo.playback)
	{
		boolean retryallowed = (modeattacking != ATTACKING_NONE);
		if (G_GametypeUsesLives())
		{
			for (i = 0; i <= splitscreen; i++)
			{
				if (players[g_localplayers[i]].lives <= 1)
					continue;
				retryallowed = true;
				break;
			}
		}

		if (retryallowed)
		{
			PAUSE_Main[mpause_tryagain].status = IT_STRING | IT_CALL;
		}
	}

	if (G_GametypeHasSpectators())
	{
		if (splitscreen)
			PAUSE_Main[mpause_spectatemenu].status = IT_STRING|IT_SUBMENU;
		else
		{
			if (!players[consoleplayer].spectator)
				PAUSE_Main[mpause_spectate].status = IT_STRING | IT_CALL;
			else if (players[consoleplayer].pflags & PF_WANTSTOJOIN)
				PAUSE_Main[mpause_canceljoin].status = IT_STRING | IT_CALL;
			else
				PAUSE_Main[mpause_entergame].status = IT_STRING | IT_CALL;
		}
	}

	G_ResetAllDeviceRumbles();
}

void M_QuitPauseMenu(INT32 choice)
{
	(void)choice;
	// M_PauseTick actually handles the quitting when it's been long enough.
	pausemenu.closing = true;
	pausemenu.openoffset = 4;
}

void M_PauseTick(void)
{
	pausemenu.offset /= 2;
	pausemenu.ticker++;

	if (pausemenu.closing)
	{
		pausemenu.openoffset *= 2;
		if (pausemenu.openoffset > 255)
			M_ClearMenus(true);

	}
	else
		pausemenu.openoffset /= 2;

#ifdef HAVE_DISCORDRPC
	// Show discord requests menu option if any requests are pending
	if (discordRequestList)
	{
		PAUSE_Main[mpause_discordrequests].status = IT_STRING | IT_CALL;
	}
#endif
}

boolean M_PauseInputs(INT32 ch)
{

	const UINT8 pid = 0;
	(void) ch;

	if (pausemenu.closing)
		return true;	// Don't allow inputs.

	if (menucmd[pid].dpad_ud < 0)
	{
		M_SetMenuDelay(pid);
		pausemenu.offset -= 50; // Each item is spaced by 50 px
		S_StartSound(NULL, sfx_s3k5b);
		M_PrevOpt();
		return true;
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		pausemenu.offset += 50;	// Each item is spaced by 50 px
		S_StartSound(NULL, sfx_s3k5b);
		M_NextOpt();
		M_SetMenuDelay(pid);
		return true;
	}

	else if (M_MenuBackPressed(pid) || M_MenuButtonPressed(pid, MBT_START))
	{
		M_QuitPauseMenu(-1);
		M_SetMenuDelay(pid);
		return true;
	}
	return false;
}

// Change gametype
void M_HandlePauseMenuGametype(INT32 choice)
{
	const UINT8 pid = 0;
	const UINT32 forbidden = GTR_FORBIDMP;

	(void)choice;

	if (M_MenuConfirmPressed(pid))
	{
		if (menugametype != gametype)
		{
			M_ClearMenus(true);
			COM_ImmedExecute(va("randommap -gt %s", gametypes[menugametype]->name));
			return;
		}

		M_SetMenuDelay(pid);
		S_StartSound(NULL, sfx_s3k7b);
	}
	else if (M_MenuExtraPressed(pid))
	{
		menugametype = gametype;
		M_SetMenuDelay(pid);
		S_StartSound(NULL, sfx_s3k7b);
	}
	else if (menucmd[pid].dpad_lr > 0)
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
}

// Restart map
void M_RestartMap(INT32 choice)
{
	(void)choice;
	M_ClearMenus(false);
	COM_ImmedExecute("restartlevel");
}

// Try again
void M_TryAgain(INT32 choice)
{
	(void)choice;
	if (demo.playback)
		return;

	if (netgame || !Playing())  // Should never happen!
		return;

	M_ClearMenus(false);

	if (modeattacking != ATTACKING_NONE)
	{
		G_CheckDemoStatus(); // Cancel recording
		M_StartTimeAttack(-1);
	}
	else
	{
		G_SetRetryFlag();
	}
}

// Pause spectate / join functions
void M_ConfirmSpectate(INT32 choice)
{
	(void)choice;
	// We allow switching to spectator even if team changing is not allowed
	M_QuitPauseMenu(-1);
	COM_ImmedExecute("changeteam spectator");
}

void M_ConfirmEnterGame(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value)
	{
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\n\nPress (B)\n"), NULL, MM_NOTHING);
		return;
	}
	M_QuitPauseMenu(-1);
	COM_ImmedExecute("changeteam playing");
}

static void M_ExitGameResponse(INT32 ch)
{
	if (ch != MA_YES)
		return;

	if (modeattacking)
	{
		M_EndModeAttackRun();
	}
	else
	{
		G_SetExitGameFlag();
		M_ClearMenus(true);
	}
}

void M_EndGame(INT32 choice)
{
	(void)choice;
	if (demo.playback)
		return;

	if (!Playing())
		return;

	M_StartMessage(M_GetText("Are you sure you want to\nreturn to the menu?\nPress (A) to confirm or (B) to cancel\n"), FUNCPTRCAST(M_ExitGameResponse), MM_YESNO);
}
