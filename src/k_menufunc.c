/// \file  k_menufunc.c
/// \brief SRB2Kart's menu functions

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "k_menu.h"

#include "doomdef.h"
#include "d_main.h"
#include "console.h"
#include "hu_stuff.h"
#include "s_sound.h"
#include "v_video.h"
#include "f_finale.h"
#include "m_misc.h"
#include "m_cond.h"

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

// ==========================================================================
// GLOBAL VARIABLES
// ==========================================================================

#ifdef HAVE_THREADS
I_mutex k_menu_mutex;
#endif

boolean menuactive = false;

// current menudef
menu_t *currentMenu = &MAIN_ProfilesDef;
menu_t *restoreMenu = NULL;

INT16 itemOn = 0; // menu item skull is on, Hack by Tails 09-18-2002
INT16 skullAnimCounter = 8; // skull animation counter
struct menutransition_s menutransition; // Menu transition properties

INT32 menuKey = -1; // keyboard key pressed for menu
menucmd_t menucmd[MAXSPLITSCREENPLAYERS];


// finish wipes between screens
boolean menuwipe = false;

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

// Consvar onchange functions
static void Dummystaff_OnChange(void);

consvar_t cv_showfocuslost = CVAR_INIT ("showfocuslost", "Yes", CV_SAVE, CV_YesNo, NULL);

consvar_t cv_menujam_update = CVAR_INIT ("menujam_update", "Off", CV_SAVE, CV_OnOff, NULL);
static CV_PossibleValue_t menujam_cons_t[] = {{0, "menu"}, {1, "menu2"}, {2, "menu3"}, {0, NULL}};
static consvar_t cv_menujam = CVAR_INIT ("menujam", "menu", CV_SAVE, menujam_cons_t, NULL);

// first time memory
consvar_t cv_tutorialprompt = CVAR_INIT ("tutorialprompt", "On", CV_SAVE, CV_OnOff, NULL);

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.

static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static CV_PossibleValue_t dummyspectate_cons_t[] = {{0, "Spectator"}, {1, "Playing"}, {0, NULL}};
static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static CV_PossibleValue_t dummystaff_cons_t[] = {{0, "MIN"}, {MAXSTAFF-1, "MAX"}, {0, NULL}};

static consvar_t cv_dummyteam = CVAR_INIT ("dummyteam", "Spectator", CV_HIDDEN, dummyteam_cons_t, NULL);
//static cv_dummyspectate = CVAR_INITconsvar_t  ("dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL);
static consvar_t cv_dummyscramble = CVAR_INIT ("dummyscramble", "Random", CV_HIDDEN, dummyscramble_cons_t, NULL);
consvar_t cv_dummystaff = CVAR_INIT ("dummystaff", "0", CV_HIDDEN|CV_CALL, dummystaff_cons_t, Dummystaff_OnChange);
consvar_t cv_dummyspectate = CVAR_INIT ("dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL);

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

static void Dummystaff_OnChange(void)
{
	if (mapheaderinfo[levellist.choosemap] == NULL || mapheaderinfo[levellist.choosemap]->ghostCount <= 0)
		return;

	dummystaff_cons_t[1].value = mapheaderinfo[levellist.choosemap]->ghostCount-1;
	if (cv_dummystaff.value > dummystaff_cons_t[1].value)
	{
		CV_StealthSetValue(&cv_dummystaff, 0);
		return;
	}
}


// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

void M_ChangeCvarDirect(INT32 choice, consvar_t *cv)
{
	// Backspace sets values to default value
	if (choice == -1)
	{
		CV_Set(cv, cv->defaultvalue);
		return;
	}

	choice = (choice == 0 ? -1 : 1);

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
		|| ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_INVISSLIDER)
		|| ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		CV_SetValue(cv, cv->value+choice);
	}
	else if (cv->flags & CV_FLOAT)
	{
		char s[20];
		sprintf(s, "%f", FIXED_TO_FLOAT(cv->value) + (choice) * (1.0f / 16.0f));
		CV_Set(cv, s);
	}
	else
	{
		if (cv == &cv_nettimeout || cv == &cv_jointimeout)
			choice *= (TICRATE/7);
		else if (cv == &cv_maxsend)
			choice *= 512;
		else if (cv == &cv_maxping)
			choice *= 50;

		CV_AddValue(cv, choice);
	}
}

static void M_ChangeCvar(INT32 choice)
{
	M_ChangeCvarDirect(choice, currentMenu->menuitems[itemOn].itemaction.cvar);
}

boolean M_NextOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		(currentMenu->menuitems[itemOn].itemaction.cvar)->value = 0;

	do
	{
		if (itemOn + 1 > currentMenu->numitems - 1)
		{
			// Prevent looparound here
			// If you're going to add any extra exceptions, DON'T.
			// Add a "don't loop" flag to the menu_t struct instead.
			if (currentMenu == &MISC_AddonsDef)
				return false;
			itemOn = 0;
		}
		else
			itemOn++;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

	M_UpdateMenuBGImage(false);

	return true;
}

boolean M_PrevOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		(currentMenu->menuitems[itemOn].itemaction.cvar)->value = 0;

	do
	{
		if (!itemOn)
		{
			// Prevent looparound here
			// If you're going to add any extra exceptions, DON'T.
			// Add a "don't loop" flag to the menu_t struct instead.
			if (currentMenu == &MISC_AddonsDef)
				return false;
			itemOn = currentMenu->numitems - 1;
		}
		else
			itemOn--;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

	M_UpdateMenuBGImage(false);

	return true;
}

static boolean M_GamestateCanOpenMenu(void)
{
	switch (gamestate)
	{
		case GS_INTRO:
		case GS_CUTSCENE:
		case GS_GAMEEND:
		case GS_CREDITS:
		case GS_EVALUATION:
		case GS_CEREMONY:
			return false;

		default:
			return true;
	}
}

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	boolean menuKeyJustChanged = false;

	if (dedicated
		|| (demo.playback && demo.title)
		|| M_GamestateCanOpenMenu() == false)
	{
		return false;
	}

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}

	if (gamestate == GS_MENU && ev->type == ev_gamepad_device_removed && G_GetPlayerForDevice(ev->device) != -1)
	{
		int i;
		INT32 player = G_GetPlayerForDevice(ev->device);

		// Unassign all controllers
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			G_SetDeviceForPlayer(i, -1);
		}

		// Return to the title because a controller was removed at the menu.
		CONS_Alert(CONS_NOTICE, "Player %d's assigned gamepad was removed. Returning to the title screen.", player);
		D_StartTitle();
	}

	if (ev->type == ev_keydown && ev->data1 > 0 && ev->data1 < NUMKEYS)
	{
		// Record keyboard presses
		menuKey = ev->data1;
		menuKeyJustChanged = true;
	}

	// Profiles: Control mapping.
	// We take the WHOLE EVENT for convenience.
	if (optionsmenu.bindcontrol)
	{
		M_MapProfileControl(ev);
		return true;	// eat events.
	}

	// event handler for MM_EVENTHANDLER
	if (menumessage.active && menumessage.flags == MM_EVENTHANDLER && menumessage.routine)
	{
		CONS_Printf("MM_EVENTHANDLER...\n");
		menumessage.eroutine(ev); // What a terrible hack...
		return true;
	}

	// Handle menu handling in-game.
	if (menuactive == false)
	{
		noFurtherInput = true;

#if 0
		// The Fx keys.
		switch (menuKey)
		{
			case KEY_F1: // Help key
				Command_Manual_f();
				return true;

			case KEY_F2: // Empty
				return true;

			case KEY_F3: // Toggle HUD
				CV_SetValue(&cv_showhud, !cv_showhud.value);
				return true;

			case KEY_F4: // Sound Volume
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				currentMenu = &OP_SoundOptionsDef;
				itemOn = 0;
				return true;

			case KEY_F5: // Video Mode
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_VideoModeMenu(0);
				return true;

			case KEY_F6: // Empty
				return true;

			case KEY_F7: // Options
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_SetupNextMenu(&OP_MainDef, false);
				return true;

			// Screenshots on F8 now handled elsewhere
			// Same with Moviemode on F9

			case KEY_F10: // Quit SRB2
				M_QuitSRB2(0);
				return true;

			case KEY_F11: // Fullscreen
				CV_AddValue(&cv_fullscreen, 1);
				return true;

			// Spymode on F12 handled in game logic
		}
#endif

		if (CON_Ready() == false && G_PlayerInputDown(0, gc_start, splitscreen + 1) == true)
		{
			if (!chat_on)
			{
				M_StartControlPanel();
				return true;
			}
		}

		noFurtherInput = false; // turns out we didn't care
		return false;
	}

	// Typing for CV_IT_STRING
	if (menuKeyJustChanged && menutyping.active && !menutyping.menutypingclose && menutyping.keyboardtyping)
	{
		M_ChangeStringCvar(menuKey);
	}

	// We're in the menu itself now.
	// M_Ticker will take care of the rest.
	return true;
}

#define NotCurrentlyPlaying(desiredname) (!S_MusicPlaying() || strcmp(desiredname, S_MusicName()))

void M_PlayMenuJam(void)
{
	menu_t *refMenu = (menuactive ? currentMenu : restoreMenu);
	static boolean loserclubpermitted = false;
	boolean loserclub = (loserclubpermitted && (gamedata->musicflags & GDMUSIC_LOSERCLUB));

	if (challengesmenu.pending)
	{
		S_StopMusic();
		S_StopMusicCredit();

		loserclubpermitted = true;
		return;
	}

	if (Playing() || soundtest.playing)
		return;

	if (refMenu != NULL && refMenu->music != NULL)
	{
		if (refMenu->music[0] == '.' && refMenu->music[1] == '\0')
		{
			S_StopMusic();
			S_StopMusicCredit();
			return;
		}
		else if (!loserclub)
		{
			if (NotCurrentlyPlaying(refMenu->music))
			{
				S_ChangeMusicInternal(refMenu->music, true);
				S_ShowMusicCredit();
			}
			return;
		}
	}

	if (loserclub)
	{
		if (refMenu != NULL && NotCurrentlyPlaying("LOSERC"))
		{
			S_ChangeMusicInternal("LOSERC", true);
			S_ShowMusicCredit();
		}

		return;
	}

	if (cv_menujam_update.value)
	{
		CV_AddValue(&cv_menujam, 1);
		CV_SetValue(&cv_menujam_update, 0);
	}

	if (!NotCurrentlyPlaying(cv_menujam.string))
		return;

	S_ChangeMusicInternal(cv_menujam.string, true);
	S_ShowMusicCredit();
}

#undef IsCurrentlyPlaying

//
// M_SpecificMenuRestore
//
menu_t *M_SpecificMenuRestore(menu_t *torestore)
{
	// I'd advise the following not be a switch case because they're pointers...

	if (torestore == &PLAY_CupSelectDef
	|| torestore == &PLAY_LevelSelectDef
	|| torestore == &PLAY_TimeAttackDef)
	{
		// Handle unlock restrictions
		cupheader_t *currentcup = levellist.levelsearch.cup;

		M_SetupGametypeMenu(-1);

		if (levellist.newgametype == GT_RACE)
		{
			M_SetupRaceMenu(-1);
			M_SetupDifficultyOptions((cupgrid.grandprix == false));
		}

		if (!M_LevelListFromGametype(-1))
		{
			if (PLAY_LevelSelectDef.prevMenu == &PLAY_CupSelectDef)
			{
				torestore = PLAY_CupSelectDef.prevMenu;
			}
			else
			{
				torestore = PLAY_LevelSelectDef.prevMenu;
			}
		}
		else
		{
			if (currentcup != NULL && levellist.levelsearch.cup == NULL)
			{
				torestore = &PLAY_CupSelectDef;
			}
			else if (torestore == &PLAY_TimeAttackDef)
			{
				M_PrepareTimeAttack(0);
			}
		}
	}
	else if (torestore == &PLAY_RaceDifficultyDef)
	{
		// Handle a much smaller subset of unlock restrictions
		M_SetupGametypeMenu(-1);
		M_SetupRaceMenu(-1);
		M_SetupDifficultyOptions((cupgrid.grandprix == false));
	}
	else if (torestore == &PLAY_MP_OptSelectDef)
	{
		// Ticker init
		M_MPOptSelectInit(-1);
	}

	// One last catch.
	M_SetupPlayMenu(-1);
	PLAY_CharSelectDef.prevMenu = &MainDef;

	return torestore;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	INT32 i;

	memset(gamekeydown, 0, sizeof (gamekeydown));
	memset(menucmd, 0, sizeof (menucmd));
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		menucmd[i].delay = MENUDELAYTIME;
	}

	// intro might call this repeatedly
	if (mapchangepending || (menuactive && gamestate != GS_NULL))
	{
		CON_ToggleOff(); // move away console
		return;
	}

	if (gamestate == GS_TITLESCREEN && restoreMenu == NULL) // Set up menu state
	{
		// No instantly skipping the titlescreen.
		// (We can change this timer later when extra animation is added.)
		if (finalecount < 1)
			return;
	}

	menuactive = true;

	if (demo.playback)
	{
		currentMenu = &PAUSE_PlaybackMenuDef;
	}
	else if (!Playing())
	{
		M_StopMessage(0); // Doesn't work with MM_YESNO or MM_EVENTHANDLER... but good enough to get the game as it is currently functional again

		if (gamestate != GS_MENU)
		{
			G_SetGamestate(GS_MENU);

			gameaction = ga_nothing;
			paused = false;
			CON_ToggleOff();

			modeattacking = ATTACKING_NONE;
		}

		if (cv_currprofile.value == -1) // Only ask once per session.
		{
			// Make sure the profile data is ready now since we need to select a profile.
			M_ResetOptions();

			// we need to do this before setting ApplyProfile otherwise funky things are going to happen.
			currentMenu = &MAIN_ProfilesDef;
			optionsmenu.profilen = cv_ttlprofilen.value;

			// options don't need initializing here.

			// make sure we don't overstep that.
			if (optionsmenu.profilen > PR_GetNumProfiles())
				optionsmenu.profilen = PR_GetNumProfiles();
			else if (optionsmenu.profilen < 0)
				optionsmenu.profilen = 0;

			currentMenu->lastOn = 0;

			CV_StealthSetValue(&cv_currprofile, -1); // Make sure to reset that as it is set by PR_ApplyProfile which we kind of hack together to force it.

			// Ambient ocean sounds
			S_ChangeMusicInternal("_OCEAN", true);
		}
		else
		{
			if (restoreMenu == NULL)
				restoreMenu = &MainDef;
			currentMenu = M_SpecificMenuRestore(M_InterruptMenuWithChallenges(restoreMenu));
			restoreMenu = NULL;

			M_PlayMenuJam();
		}
	}
	else
	{
		M_OpenPauseMenu();
	}

	itemOn = currentMenu->lastOn;

	CON_ToggleOff(); // move away console
}

//
// M_ClearMenus
//
void M_ClearMenus(boolean callexitmenufunc)
{
	if (!menuactive)
		return;

	CON_ClearHUD();

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

	if (gamestate == GS_MENU) // Back to title screen
		D_StartTitle();

	menutyping.active = false;
	menumessage.active = false;

	menuactive = false;
}

void M_SelectableClearMenus(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef, boolean notransition)
{
	INT16 i;

	if (!notransition)
	{
		if (currentMenu->transitionID == menudef->transitionID
			&& currentMenu->transitionTics)
		{
			menutransition.startmenu = currentMenu;
			menutransition.endmenu = menudef;

			menutransition.tics = 0;
			menutransition.dest = currentMenu->transitionTics;
			menutransition.in = false;
			return; // Don't change menu yet, the transition will call this again
		}
		else if (gamestate == GS_MENU)
		{
			menuwipe = true;
			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();
			F_RunWipe(wipe_menu_toblack, wipedefs[wipe_menu_toblack], false, "FADEMAP0", false, false);
		}
	}

	if (currentMenu->quitroutine)
	{
		// If you're going from a menu to itself, why are you running the quitroutine? You're not quitting it! -SH
		if (currentMenu != menudef && !currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}

	if (menudef->initroutine != NULL
#if 0
		&& currentMenu != menudef // Unsure if we need this...
#endif
		)
	{
		// Moving to a new menu, reinitialize.
		menudef->initroutine();
	}

	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = currentMenu->numitems - 1;

	// the curent item can be disabled,
	// this code go up until an enabled item found
	if ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE)
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if ((currentMenu->menuitems[i].status & IT_TYPE) != IT_SPACE)
			{
				itemOn = i;
				break;
			}
		}
	}

	M_UpdateMenuBGImage(false);
	M_PlayMenuJam();
}

void M_GoBack(INT32 choice)
{
	(void)choice;

	noFurtherInput = true;
	currentMenu->lastOn = itemOn;

	if (currentMenu->prevMenu)
	{
		//If we entered the game search menu, but didn't enter a game,
		//make sure the game doesn't still think we're in a netgame.
		if (!Playing() && netgame && multiplayer)
		{
			netgame = false;
			multiplayer = false;
		}

		M_SetupNextMenu(currentMenu->prevMenu, false);
	}
	else
		M_ClearMenus(true);

	S_StartSound(NULL, sfx_s3k5b);
}

//
// M_Ticker
//
void M_SetMenuDelay(UINT8 i)
{
	menucmd[i].delayCount++;
	if (menucmd[i].delayCount < 1)
	{
		// Shouldn't happen, but for safety.
		menucmd[i].delayCount = 1;
	}

	menucmd[i].delay = (MENUDELAYTIME / menucmd[i].delayCount);
	if (menucmd[i].delay < 1)
	{
		menucmd[i].delay = 1;
	}
}

void M_UpdateMenuCMD(UINT8 i)
{
	UINT8 mp = max(1, setup_numplayers);

	menucmd[i].prev_dpad_ud = menucmd[i].dpad_ud;
	menucmd[i].prev_dpad_lr = menucmd[i].dpad_lr;

	menucmd[i].dpad_ud = 0;
	menucmd[i].dpad_lr = 0;

	menucmd[i].buttonsHeld = menucmd[i].buttons;
	menucmd[i].buttons = 0;

	if (G_PlayerInputDown(i, gc_screenshot,    mp)) { menucmd[i].buttons |= MBT_SCREENSHOT; }
	if (G_PlayerInputDown(i, gc_startmovie,    mp)) { menucmd[i].buttons |= MBT_STARTMOVIE; }
	if (G_PlayerInputDown(i, gc_startlossless, mp)) { menucmd[i].buttons |= MBT_STARTLOSSLESS; }

	// Screenshot et al take priority
	if (menucmd[i].buttons != 0)
		return;

	if (G_PlayerInputDown(i, gc_up,   mp)) { menucmd[i].dpad_ud--; }
	if (G_PlayerInputDown(i, gc_down, mp)) { menucmd[i].dpad_ud++; }

	if (G_PlayerInputDown(i, gc_left,  mp)) { menucmd[i].dpad_lr--; }
	if (G_PlayerInputDown(i, gc_right, mp)) { menucmd[i].dpad_lr++; }

	if (G_PlayerInputDown(i, gc_a, mp)) { menucmd[i].buttons |= MBT_A; }
	if (G_PlayerInputDown(i, gc_b, mp)) { menucmd[i].buttons |= MBT_B; }
	if (G_PlayerInputDown(i, gc_c, mp)) { menucmd[i].buttons |= MBT_C; }
	if (G_PlayerInputDown(i, gc_x, mp)) { menucmd[i].buttons |= MBT_X; }
	if (G_PlayerInputDown(i, gc_y, mp)) { menucmd[i].buttons |= MBT_Y; }
	if (G_PlayerInputDown(i, gc_z, mp)) { menucmd[i].buttons |= MBT_Z; }
	if (G_PlayerInputDown(i, gc_l, mp)) { menucmd[i].buttons |= MBT_L; }
	if (G_PlayerInputDown(i, gc_r, mp)) { menucmd[i].buttons |= MBT_R; }

	if (G_PlayerInputDown(i, gc_start, mp)) { menucmd[i].buttons |= MBT_START; }

	if (menucmd[i].dpad_ud == 0 && menucmd[i].dpad_lr == 0 && menucmd[i].buttons == 0)
	{
		// Reset delay count with no buttons.
		menucmd[i].delay = min(menucmd[i].delay, MENUMINDELAY);
		menucmd[i].delayCount = 0;
	}
}

boolean M_MenuButtonPressed(UINT8 pid, UINT32 bt)
{
	if (menucmd[pid].buttonsHeld & bt)
	{
		return false;
	}

	return (menucmd[pid].buttons & bt);
}

boolean M_MenuButtonHeld(UINT8 pid, UINT32 bt)
{
	return (menucmd[pid].buttons & bt);
}

// Returns true if we press the confirmation button
boolean M_MenuConfirmPressed(UINT8 pid)
{
	 return M_MenuButtonPressed(pid, MBT_A);
}

boolean M_MenuConfirmHeld(UINT8 pid)
{
	 return M_MenuButtonHeld(pid, MBT_A);
}

// Returns true if we press the Cancel button
boolean M_MenuBackPressed(UINT8 pid)
{
	 return (M_MenuButtonPressed(pid, MBT_B) || M_MenuButtonPressed(pid, MBT_X));
}

boolean M_MenuBackHeld(UINT8 pid)
{
	 return (M_MenuButtonHeld(pid, MBT_B) || M_MenuButtonHeld(pid, MBT_X));
}

// Retrurns true if we press the tertiary option button (C)
boolean M_MenuExtraPressed(UINT8 pid)
{
	 return M_MenuButtonPressed(pid, MBT_C);
}

boolean M_MenuExtraHeld(UINT8 pid)
{
	 return M_MenuButtonHeld(pid, MBT_C);
}


static void M_HandleMenuInput(void)
{
	void (*routine)(INT32 choice); // for some casting problem
	UINT8 pid = 0; // todo: Add ability for any splitscreen player to bring up the menu.
	SINT8 lr = 0, ud = 0;
	INT32 thisMenuKey = menuKey;

	menuKey = -1;

	if (menuactive == false)
	{
		// We're not in the menu.
		return;
	}

	if (menumessage.active)
	{
		M_HandleMenuMessage();
		return;
	}

	// Typing for CV_IT_STRING
	if (menutyping.active)
	{
		M_MenuTypingInput(thisMenuKey);
		return;
	}

	if (menucmd[pid].delay > 0)
	{
		return;
	}

	// Handle menu-specific input handling. If this returns true, we skip regular input handling.
	if (currentMenu->inputroutine)
	{
		if (currentMenu->inputroutine(thisMenuKey))
		{
			return;
		}
	}

	routine = currentMenu->menuitems[itemOn].itemaction.routine;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		routine(-1);
		return;
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING)
		{
			// Routine is null either way
			routine = NULL;

			// If we're hovering over a IT_CV_STRING option, pressing A/X opens the typing submenu
			if (M_MenuConfirmPressed(pid))
			{
				menutyping.keyboardtyping = thisMenuKey != -1 ? true : false;	// If we entered this menu by pressing a menu Key, default to keyboard typing, otherwise use controller.
				menutyping.active = true;
				menutyping.menutypingclose = false;
				return;
			}

		}
		else
		{
			routine = M_ChangeCvar;
		}
	}

	lr = menucmd[pid].dpad_lr;
	ud = menucmd[pid].dpad_ud;

	if (currentMenu->behaviourflags & MBF_UD_LR_FLIPPED)
	{
		ud = menucmd[pid].dpad_lr;
		lr = -menucmd[pid].dpad_ud;
	}

	// LR does nothing in the default menu, just remap as dpad.
	if (menucmd[pid].buttons & MBT_L) { lr--; }
	if (menucmd[pid].buttons & MBT_R) { lr++; }

	// Keys usable within menu
	if (ud > 0)
	{
		if (M_NextOpt() && !(currentMenu->behaviourflags & MBF_SOUNDLESS))
			S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		return;
	}
	else if (ud < 0)
	{
		if (M_PrevOpt() && !(currentMenu->behaviourflags & MBF_SOUNDLESS))
			S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		return;
	}
	else if (lr < 0)
	{
		if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
			|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
		{
			if (!(currentMenu->behaviourflags & MBF_SOUNDLESS))
				S_StartSound(NULL, sfx_s3k5b);
			routine(0);
			M_SetMenuDelay(pid);
		}

		return;
	}
	else if (lr > 0)
	{
		if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
			|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
		{
			if (!(currentMenu->behaviourflags & MBF_SOUNDLESS))
				S_StartSound(NULL, sfx_s3k5b);
			routine(1);
			M_SetMenuDelay(pid);
		}

		return;
	}
	else if (M_MenuConfirmPressed(pid) /*|| M_MenuButtonPressed(pid, MBT_START)*/)
	{
		noFurtherInput = true;
		currentMenu->lastOn = itemOn;

		if (routine)
		{
			if (!(currentMenu->behaviourflags & MBF_SOUNDLESS))
				S_StartSound(NULL, sfx_s3k5b);

			if (((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CALL
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SUBMENU)
				&& (currentMenu->menuitems[itemOn].status & IT_CALLTYPE))
			{
				if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && majormods)
				{
					M_StartMessage(M_GetText("This cannot be done with complex addons\nor in a cheated game.\n\nPress (B)"), NULL, MM_NOTHING);
					return;
				}
			}

			switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
			{
				case IT_CVAR:
				case IT_ARROWS:
					routine(2); // usually right arrow
					break;
				case IT_CALL:
					routine(itemOn);
					break;
				case IT_SUBMENU:
					currentMenu->lastOn = itemOn;
					M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction.submenu, false);
					break;
			}
		}

		M_SetMenuDelay(pid);
		return;
	}
	else if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
		return;
	}
	else if (M_MenuExtraPressed(pid))
	{
		if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
			|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
		{
			/*consvar_t *cv = currentMenu->menuitems[itemOn].itemaction.cvar;

			// Make these CVar options?
			if (cv == &cv_chooseskin
				|| cv == &cv_dummystaff
				|| cv == &cv_nextmap
				|| cv == &cv_newgametype
				)
			{
				return;
			}*/

			if (!(currentMenu->behaviourflags & MBF_SOUNDLESS))
				S_StartSound(NULL, sfx_s3k5b);

			routine(-1);
			M_SetMenuDelay(pid);
			return;
		}

		return;
	}

	return;
}

void M_Ticker(void)
{
	INT32 i;

	HU_TickSongCredits();

	if (!menuactive)
	{
		noFurtherInput = false;
		return;
	}

	if (menutransition.tics != 0 || menutransition.dest != 0)
	{
		noFurtherInput = true;

		if (menutransition.tics < menutransition.dest)
			menutransition.tics++;
		else if (menutransition.tics > menutransition.dest)
			menutransition.tics--;

		// If dest is non-zero, we've started transition and want to switch menus
		// If dest is zero, we're mid-transition and want to end it
		if (menutransition.tics == menutransition.dest
			&& menutransition.endmenu != NULL
			&& currentMenu != menutransition.endmenu
		)
		{
			if (menutransition.startmenu->transitionID == menutransition.endmenu->transitionID
				&& menutransition.endmenu->transitionTics)
			{
				menutransition.tics = menutransition.endmenu->transitionTics;
				menutransition.dest = 0;
				menutransition.in = true;
			}
			else if (gamestate == GS_MENU)
			{
				memset(&menutransition, 0, sizeof(menutransition));

				menuwipe = true;
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(wipe_menu_toblack, wipedefs[wipe_menu_toblack], false, "FADEMAP0", false, false);
			}

			M_SetupNextMenu(menutransition.endmenu, true);
		}
	}
	else
	{
		if (menuwipe)
		{
			// try not to let people input during the fadeout
			noFurtherInput = true;
		}
		else
		{
			// reset input trigger
			noFurtherInput = false;
		}
	}

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (menucmd[i].delay > 0)
		{
			menucmd[i].delay--;
		}
	}

	if (noFurtherInput == false)
	{
		M_HandleMenuInput();
	}

	if (currentMenu->tickroutine)
	{
		currentMenu->tickroutine();
	}

	if (dedicated)
	{
		return;
	}

	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8;

#if 0
	if (currentMenu == &PAUSE_PlaybackMenuDef)
	{
		if (playback_enterheld > 0)
			playback_enterheld--;
	}
	else
		playback_enterheld = 0;

	//added : 30-01-98 : test mode for five seconds
	if (vidm_testingmode > 0)
	{
		// restore the previous video mode
		if (--vidm_testingmode == 0)
			setmodeneeded = vidm_previousmode + 1;
	}
#endif
}

//
// M_Init
//
void M_Init(void)
{
#if 0
	CV_RegisterVar(&cv_nextmap);
#endif
	CV_RegisterVar(&cv_chooseskin);
	CV_RegisterVar(&cv_autorecord);

	// don't lose your position in the jam cycle
	CV_RegisterVar(&cv_menujam_update);
	CV_RegisterVar(&cv_menujam);

	CV_RegisterVar(&cv_serversort);

	if (dedicated)
		return;

	//COM_AddCommand("manual", Command_Manual_f);

	// Menu hacks
	CV_RegisterVar(&cv_dummymenuplayer);
	CV_RegisterVar(&cv_dummyteam);
	CV_RegisterVar(&cv_dummyspectate);
	CV_RegisterVar(&cv_dummyscramble);
	CV_RegisterVar(&cv_dummystaff);
	CV_RegisterVar(&cv_dummyip);

	CV_RegisterVar(&cv_dummyprofilename);
	CV_RegisterVar(&cv_dummyprofileplayername);
	CV_RegisterVar(&cv_dummyprofilekickstart);
	CV_RegisterVar(&cv_dummyprofilerumble);

	CV_RegisterVar(&cv_dummygpdifficulty);
	CV_RegisterVar(&cv_dummykartspeed);
	CV_RegisterVar(&cv_dummygpencore);
	CV_RegisterVar(&cv_dummymatchbots);

	CV_RegisterVar(&cv_dummyspbattack);

	CV_RegisterVar(&cv_dummyaddonsearch);

	CV_RegisterVar(&cv_dummyextraspassword);

	M_UpdateMenuBGImage(true);
}
