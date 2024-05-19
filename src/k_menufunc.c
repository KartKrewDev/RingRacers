// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
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
#include "music.h"

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

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

CV_PossibleValue_t dummystaff_cons_t[] = {{0, "MIN"}, {999, "MAX"}, {0, NULL}};


// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

static void M_AddFloatVar(consvar_t *cv, fixed_t step)
{
	int minopt = 0;
	int maxopt = 0;
	int curopt = -1;

	int i;

	const CV_PossibleValue_t *values = cv->PossibleValue;

	if (values == NULL) //cvar is unbounded and will not work! return is here only as a failsafe to prevent crashes
		return;

	for (i = 0; values[i].strvalue; ++i)
	{
		if (cv->value == values[i].value)
		{
			curopt = i;

			if (i > 1)
				break;
		}
		else if (i > 1)
		{
			if (!minopt || values[minopt].value > values[i].value)
				minopt = i;

			if (!maxopt || values[maxopt].value < values[i].value)
				maxopt = i;
		}
	}

	if (curopt > 1 || curopt == (step > 0))
	{
		CV_Set(cv, step < 0 ? (maxopt ? values[maxopt].strvalue : "MAX") : (minopt ? values[minopt].strvalue : "MIN"));
		return;
	}

	fixed_t n = cv->value;

	if (step > 0)
	{
		if (values[1].value - n <= step)
		{
			CV_Set(cv, "MAX");
			return;
		}
		n = n + step;
		n -= n % step;
	}
	else
	{
		if (n - values[0].value <= -step)
		{
			CV_Set(cv, "MIN");
			return;
		}
		fixed_t p = n % -step;
		n -= p ? p : -step;
	}

	char s[20];
	double f = FIXED_TO_FLOAT(n);
	const char *d = M_Ftrim(f);
	sprintf(s, "%ld%s", (long)f, *d ? d : ".0");

	CV_Set(cv, s);
}

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
		M_AddFloatVar(cv, (cv->step_amount ? cv->step_amount : FRACUNIT/16) * choice);
	}
	else
	{
		if (cv == &cv_nettimeout || cv == &cv_jointimeout)
			choice *= (TICRATE/7);
		else if (cv == &cv_maxsend)
			choice *= 512;

		CV_AddValue(cv, choice);
	}
}

static void M_ChangeCvarResponse(INT32 choice)
{
	if (choice != MA_YES)
		return;

	consvar_t *cvar = currentMenu->menuitems[itemOn].itemaction.cvar;
	M_ChangeCvarDirect(choice, cvar);
}

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cvar = currentMenu->menuitems[itemOn].itemaction.cvar;

#ifdef HWRENDER
	if (cvar == &cv_renderer &&
		// Switching from Software [to Legacy GL]
		cv_renderer.value == 1 &&
		// Not setting to default (ie changing the value)
		choice != -1)
	{
		M_StartMessage(
			"Switching to Legacy GL",
			"Legacy GL is \x85" "INCOMPLETE and BROKEN.\x80\n"
			"\n"
			"ARE YOU SURE?",
			M_ChangeCvarResponse,
			MM_YESNO,
			NULL,
			NULL
		);
		return;
	}
#endif

	if (cvar == &cv_dummyprofileautoroulette &&
		// Turning Auto Roulette on
		cv_dummyprofileautoroulette.value == 0 &&
		// Not setting to default (ie changing the value)
		choice != -1)
	{
		M_StartMessage(
			"Turning on Auto Roulette",
			"\"Ring Racers\" is not designed with random items in mind. With Auto Roulette, you cannot select the item results you want or select an item early."
			"\n"
			"You will be at a distinct \x85" "disadvantage. \x80\n"
			"\n"
			"ARE YOU SURE?",
			M_ChangeCvarResponse,
			MM_YESNO,
			NULL,
			NULL
		);
		return;
	}

	M_ChangeCvarDirect(choice, cvar);
}

static const char *M_QueryCvarAction(const char *replace)
{
	consvar_t *cvar = currentMenu->menuitems[itemOn].itemaction.cvar;
	if (replace)
		CV_Set(cvar, replace);
	return cvar->string;
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
			if (currentMenu->behaviourflags & MBF_NOLOOPENTRIES)
			{
				itemOn = oldItemOn;
				return false;
			}
			itemOn = 0;
		}
		else
			itemOn++;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

	M_UpdateMenuBGImage(false);
	M_FlipKartGamemodeMenu(true);

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
			if (currentMenu->behaviourflags & MBF_NOLOOPENTRIES)
			{
				itemOn = oldItemOn;
				return false;
			}
			itemOn = currentMenu->numitems - 1;
		}
		else
			itemOn--;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

	M_UpdateMenuBGImage(false);
	M_FlipKartGamemodeMenu(true);

	return true;
}

static boolean M_GamestateCanOpenMenu(void)
{
	switch (gamestate)
	{
		case GS_INTRO:
		case GS_CUTSCENE:
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
	if (ev->type == ev_keydown && !ev->data2)
	{
		extern consvar_t cv_showhud;
		switch (ev->data1)
		{
			case KEY_F3: // Toggle HUD
				// I am lazy so this button is also
				// hardcoded.
				CV_SetValue(&cv_showhud, !cv_showhud.value);
				return true;

			case KEY_F11: // Fullscreen
				// F11 can always be used to toggle
				// fullscreen, it's a safe key.
				CV_AddValue(&cv_fullscreen, 1);
				return true;
		}
	}

	if (dedicated
		|| (demo.playback && demo.attract)
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
	}

	// Profiles: Control mapping.
	// We take the WHOLE EVENT for convenience.
	if (optionsmenu.bindtimer)
	{
		M_MapProfileControl(ev);
		return true;	// eat events.
	}

	// event handler for MM_EVENTHANDLER
	/*if (menumessage.active && menumessage.flags == MM_EVENTHANDLER && menumessage.routine)
	{
		CONS_Printf("MM_EVENTHANDLER...\n");
		menumessage.eroutine(ev); // What a terrible hack...
		return true;
	}*/

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

		if (CON_Ready() == false)
		{
			boolean allowmpause = true;

			// Special mid-game input behaviours
			if (Playing() && !demo.playback)
			{
				// Quick Retry (Y in modeattacking)
				if (modeattacking && G_PlayerInputDown(0, gc_y, splitscreen + 1) == true)
				{
					M_TryAgain(0);
					return true;
				}

				// Quick Spectate (L+R+A+Start online)
				if (G_GametypeHasSpectators())
				{
					UINT8 workingpid = 0;
					for (workingpid = 0; workingpid <= splitscreen; workingpid++)
					{
						if (players[g_localplayers[workingpid]].spectator == true)
							continue;

						if (G_PlayerInputDown(workingpid, gc_l, 0) == false)
							continue;
						if (G_PlayerInputDown(workingpid, gc_r, 0) == false)
							continue;
						if (G_PlayerInputDown(workingpid, gc_a, 0) == false)
							continue;
						if (G_PlayerInputDown(workingpid, gc_start, 0) == false)
							continue;

						if (workingpid == 0)
						{
							allowmpause = false;
							COM_ImmedExecute("changeteam spectator");
							continue;
						}

						COM_ImmedExecute(
							va(
								"changeteam%u spectator",
								workingpid + 1
							)
						);
					}
				}
			}

			// Bog-standard Pause
			if (allowmpause && G_PlayerInputDown(0, gc_start, splitscreen + 1) == true)
			{
				if (!chat_on)
				{
					M_StartControlPanel();
					return true;
				}
			}
		}

		noFurtherInput = false; // turns out we didn't care
		return false;
	}

	// We're in the menu itself now.
	// M_Ticker will take care of the rest.
	return true;
}

#define NotCurrentlyPlaying(desiredname) strcmp(desiredname, Music_CurrentSong())

void M_PlayMenuJam(void)
{
	menu_t *refMenu = (menuactive ? currentMenu : restoreMenu);
	static boolean musicstatepermitted = false;

	if (challengesmenu.pending)
	{
		Music_StopAll();

		musicstatepermitted = true;
		return;
	}

	if (soundtest.playing)
		return;

	const boolean trulystarted = M_GameTrulyStarted();
	const boolean profilemode = (
		optionsmenu.profilemenu
		&& !optionsmenu.resetprofilemenu
	);

	if (!profilemode && Playing())
	{
		if (optionsmenu.resetprofilemenu)
			Music_Stop("menu");

		return;
	}

	// trulystarted == false in the Tutorial.
	// But profile menu music should play during the Tutorial (Playing()).
	if (!trulystarted && !Playing())
	{
		if (M_GonerMusicPlayable() && NotCurrentlyPlaying("_GONER"))
		{
			Music_Remap("menu", "_GONER");
			Music_Play("menu");
		}

		return;
	}

	gdmusic_t override = musicstatepermitted ? gamedata->musicstate : 0;

	if (refMenu != NULL && refMenu->music != NULL)
	{
		if (refMenu->music[0] == '.' && refMenu->music[1] == '\0')
		{
			Music_StopAll();
			return;
		}
		else if (override == 0)
		{
			if (NotCurrentlyPlaying(refMenu->music))
			{
				Music_Remap("menu", refMenu->music);
				Music_Play("menu");
			}
			return;
		}
	}

	if (override != 0)
	{
		// See also gdmusic_t
		const char* overridetotrack[GDMUSIC_MAX-1] = {
			"KEYGEN",
			"LOSERC",
			"TRACKA",
		};

		if (refMenu != NULL && NotCurrentlyPlaying(overridetotrack[override - 1]))
		{
			Music_Remap("menu", overridetotrack[override - 1]);
			Music_Play("menu");

			if (override < GDMUSIC_KEEPONMENU)
				gamedata->musicstate = GDMUSIC_NONE;
		}

		return;
	}

	if (!profilemode && cv_menujam_update.value)
	{
		CV_AddValue(&cv_menujam, 1);
		CV_SetValue(&cv_menujam_update, 0);
	}

	if (!NotCurrentlyPlaying(cv_menujam.string))
		return;

	Music_Remap("menu", cv_menujam.string);
	Music_Play("menu");
}

#undef IsCurrentlyPlaying

boolean M_ConsiderSealedSwapAlert(void)
{
	if (gamedata->sealedswapalerted == true)
		return false;

	if (gamedata->sealedswaps[GDMAX_SEALEDSWAPS-1] != NULL // all found
	|| M_SecretUnlocked(SECRET_SPECIALATTACK, true)) // true order
	{
		gamedata->sealedswapalerted = true;

		// Don't make a message if no Sealed Stars have yet been found.
		if (gamedata->everseenspecial == false)
			return false;

		M_StartMessage(
			"Message from the Stars",
			"As if called by fate, the Emeralds you've\n"
			"collected return to their rightful places...\n"
			"\n"
			"The Sealed Stars are now ordered via Cups!\n",
			NULL, MM_NOTHING, NULL, NULL
		);

		if (gamedata->musicstate < GDMUSIC_TRACK10)
			gamedata->musicstate = GDMUSIC_TRACK10;

		return true;
	}

	return false;
}

void M_ValidateRestoreMenu(void)
{
	if (restoreMenu == NULL || restoreMenu == &MAIN_GonerDef)
		restoreMenu = &MainDef;
}

//
// M_SpecificMenuRestore
//
menu_t *M_SpecificMenuRestore(menu_t *torestore)
{
	// I'd advise the following not be a switch case because they're pointers...

	if (torestore == &PLAY_CupSelectDef
	|| torestore == &PLAY_LevelSelectDef
	|| torestore == &PLAY_TimeAttackDef
	|| torestore == &PLAY_TAReplayDef)
	{
		// Handle unlock restrictions

		levellist = restorelevellist;

		cupheader_t *currentcup = levellist.levelsearch.cup;

		if (levellist.levelsearch.tutorial)
		{
			M_InitExtras(-1);
		}
		else
		{
			M_SetupGametypeMenu(-1);

			if (levellist.newgametype == GT_RACE)
			{
				M_SetupRaceMenu(-1);
				M_SetupDifficultyOptions((levellist.levelsearch.grandprix == false));
			}
		}

		if (!M_LevelListFromGametype(-1))
		{
			torestore = levellist.backMenu;
		}
		else
		{
			if (currentcup != NULL && levellist.levelsearch.cup == NULL)
			{
				torestore = &PLAY_CupSelectDef;
			}
			else if (levellist.levelsearch.timeattack)
			{
				M_PrepareTimeAttack(true);
			}
		}
	}
	else if (torestore == &PLAY_MP_OptSelectDef)
	{
		// Ticker init
		M_MPOptSelectInit(-1);
	}
	else if (torestore == &EXTRAS_MainDef)
	{
		// Disable or enable certain options
		M_InitExtras(-1);
	}

	// One last catch.
	M_SetupPlayMenu(-1);
	PLAY_CharSelectDef.prevMenu = &MainDef;

	if (torestore != &MISC_ChallengesDef)
	{
		M_ConsiderSealedSwapAlert();
	}

	return torestore;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	if (demo.playback && gamestate == GS_INTERMISSION)
	{
		// At this point the replay has ended.
		// The only menu option that works is "Stop Playback".
		// And intermission can be finished by pressing the
		// A button, so having a menu at all is useless.
		return;
	}

	INT32 i;

	G_ResetAllDeviceGameKeyDown();
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
		if (finalecount < (
			M_GameTrulyStarted()
				? 1
				: 3*TICRATE
			)
		)
		{
			return;
		}

		if (menumessage.active)
		{
			if (!menumessage.closing && menumessage.fadetimer == 9)
			{
				// The following doesn't work with MM_YESNO.
				// However, because there's no guarantee a profile
				// is selected or controls set up to our liking,
				// we can't call M_HandleMenuMessage.

				M_StopMessage(MA_NONE);
			}

			return;
		}
	}

	menuactive = true;

	if (demo.playback)
	{
		currentMenu = &PAUSE_PlaybackMenuDef;
	}
	else if (!Playing())
	{
		if (gamestate != GS_MENU)
		{
			if (titlemapinaction)
			{
				// We clear a LITTLE bit of state, but not a full D_ClearState.
				// Just enough to guarantee SV_ResetServer is called before session start.
				SV_StopServer();
				SV_ResetServer();
			}

			G_SetGamestate(GS_MENU);

			gameaction = ga_nothing;
			paused = false;
			CON_ToggleOff();

			modeattacking = ATTACKING_NONE;
		}

		Music_Stop("title");

		if (gamedata != NULL
		&& gamedata->gonerlevel < GDGONER_OUTRO
		&& gamestartchallenge < MAXUNLOCKABLES)
		{
			// See M_GameTrulyStarted
			if (
				gamedata->unlockpending[gamestartchallenge]
				|| gamedata->unlocked[gamestartchallenge]
			)
			{
				gamedata->gonerlevel = GDGONER_OUTRO;
				M_GonerBGImplyPassageOfTime();
			}
		}

		if (M_GameTrulyStarted() == false)
		{
			// Are you ready for the First Boot Experience?
			M_ResetOptions();

			currentMenu = &MAIN_GonerDef;
			restoreMenu = NULL;

			M_PlayMenuJam();
		}
		else if (cv_currprofile.value == -1) // Only ask once per session.
		{
			// Make sure the profile data is ready now since we need to select a profile.
			M_ResetOptions();

			// we need to do this before setting ApplyProfile otherwise funky things are going to happen.
			currentMenu = &MAIN_ProfilesDef;
			optionsmenu.profilen = cv_ttlprofilen.value;

			// options don't need initializing here.

			// make sure we don't overstep that.
			const INT32 maxp = PR_GetNumProfiles();
			if (optionsmenu.profilen > maxp)
				optionsmenu.profilen = maxp;
			else if (optionsmenu.profilen < 0)
				optionsmenu.profilen = 0;

			currentMenu->lastOn = 0;

			CV_StealthSetValue(&cv_currprofile, -1); // Make sure to reset that as it is set by PR_ApplyProfile which we kind of hack together to force it.

			// Ambient ocean sounds
			Music_Remap("menu_nocred", "_OCEAN");
			Music_Play("menu_nocred");
		}
		else
		{
			M_ValidateRestoreMenu();
			currentMenu = M_SpecificMenuRestore(M_InterruptMenuWithChallenges(restoreMenu));
			restoreMenu = NULL;

			M_PlayMenuJam();
		}

		itemOn = currentMenu->lastOn;
		M_UpdateMenuBGImage(true);

#ifdef HAVE_DISCORDRPC
		// currentMenu changed during GS_MENU
		DRPC_UpdatePresence();
#endif
	}
	else
	{
		M_OpenPauseMenu();
	}

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

	currentMenu->lastOn = itemOn;

	if (gamestate == GS_MENU) // Back to title screen
	{
		int i;
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			G_SetDeviceForPlayer(i, -1);
		}
		D_StartTitle();
	}

	M_AbortVirtualKeyboard();
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
			M_FlipKartGamemodeMenu(false);
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

#ifdef HAVE_DISCORDRPC
	if (gamestate == GS_MENU)
	{
		// currentMenu changed during GS_MENU
		DRPC_UpdatePresence();
	}
#endif
}

void M_GoBack(INT32 choice)
{
	const INT16 behaviourflags = currentMenu->behaviourflags;

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
	else if (Playing() || M_GameTrulyStarted())
		M_ClearMenus(true);
	else // No returning to the title screen.
		M_QuitSRB2(-1);

	if (!(behaviourflags & MBF_SOUNDLESS))
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

	menucmd[i].delay = (MENUDELAYTIME - min(MENUDELAYTIME - 1, menucmd[i].delayCount));
	if (menucmd[i].delay < MENUMINDELAY)
	{
		menucmd[i].delay = MENUMINDELAY;
	}
}

void M_UpdateMenuCMD(UINT8 i, boolean bailrequired)
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

	if (bailrequired && i == 0)
	{
		if (G_GetDeviceGameKeyDownArray(0)[KEY_ESCAPE]) { menucmd[i].buttons |= MBT_B; }
	}

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

	return !!(menucmd[pid].buttons & bt);
}

boolean M_MenuButtonHeld(UINT8 pid, UINT32 bt)
{
	return !!(menucmd[pid].buttons & bt);
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
		M_MenuTypingInput(-1);
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
				// If we entered this menu by pressing a menu Key, default to keyboard typing, otherwise use controller.
				M_OpenVirtualKeyboard(
					(currentMenu->menuitems[itemOn].itemaction.cvar == &cv_dummyprofilename) ? 6 // this sucks, but there's no time.
						: MAXSTRINGLENGTH,
					M_QueryCvarAction,
					NULL
				);
				return;
			}
			else if (M_MenuExtraPressed(pid))
			{
				if (!(currentMenu->behaviourflags & MBF_SOUNDLESS))
					S_StartSound(NULL, sfx_s3k5b);

				M_ChangeCvar(-1);
				M_SetMenuDelay(pid);
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
					M_StartMessage("Modified Game", M_GetText("This cannot be done with complex addons\nor in a cheated game."), NULL, MM_NOTHING, NULL, NULL);
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
		if (menutransition.tics == menutransition.dest)
		{
			if (menutransition.endmenu != NULL
				&& currentMenu != menutransition.endmenu)
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
			else
			{
				// Menu is done transitioning in
				M_FlipKartGamemodeMenu(true);
			}
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
	{
		skullAnimCounter = 8;
	}

	if (!Playing())
	{
		// Anything in M_Ticker that isn't actively playing is considered "in menus" for time tracking
		gamedata->totalmenutime++;
	}

	if (!Playing() && !M_GameTrulyStarted())
	{
		M_GonerBGTick();
	}

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

	if (dedicated)
		return;

	//COM_AddCommand("manual", Command_Manual_f);

	M_UpdateMenuBGImage(true);
}
