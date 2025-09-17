// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'".
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-1.c
/// \brief Options Menu

#include "../i_time.h"
#include "../k_menu.h"
#include "../k_grandprix.h" // K_CanChangeRules
#include "../m_cond.h" // Condition Sets
#include "../k_follower.h"
#include "../s_sound.h"

// options menu --  see mopt_e
menuitem_t OPTIONS_Main[] =
{

	{IT_STRING | IT_CALL, "Profile Setup", "Remap keys & buttons.",
		NULL, {.routine = M_ProfileSelectInit}, 0, 0},

	{IT_STRING | IT_CALL, "Video Options", "Change the resolution.",
		NULL, {.routine = M_VideoOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Sound Options", "Adjust the volume.",
		NULL, {.routine = M_SoundOptions}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Voice Options", "Adjust voice chat.",
		NULL, {.submenu = &OPTIONS_VoiceDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "HUD Options", "Tweak the Heads-Up Display.",
		NULL, {.submenu = &OPTIONS_HUDDef}, 0, 0},

	{IT_STRING | IT_CALL, "Gameplay Options", "Modify game mechanics.",
		NULL, {.routine = M_GameplayOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Server Options", "Update server settings.",
		NULL, {.routine = M_ServerOptions}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Data Options", "Video recording, file saving, Discord status.",
		NULL, {.submenu = &OPTIONS_DataDef}, 0, 0},

#ifdef TODONEWMANUAL
	{IT_STRING | IT_CALL, "Tricks & Secrets", "Those who bother reading a game manual always get the edge over those who don't!",
		NULL, {.routine = M_Manual}, 0, 0},
#endif
};

// For options menu, the 'extra1' field will determine the background colour to use for... the background! (What a concept!)
menu_t OPTIONS_MainDef = {
	sizeof (OPTIONS_Main) / sizeof (menuitem_t),
	&MainDef,
	0,
	OPTIONS_Main,
	0, 0,
	SKINCOLOR_SLATE, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	M_OptionsInputs
};

struct optionsmenu_s optionsmenu;

void M_ResetOptions(void)
{
	optionsmenu.ticker = 0;
	optionsmenu.offset.start = 0;

	optionsmenu.optx = 0;
	optionsmenu.opty = 0;
	optionsmenu.toptx = 0;
	optionsmenu.topty = 0;

	// BG setup:
	optionsmenu.currcolour = OPTIONS_MainDef.extra1;
	optionsmenu.lastcolour = 0;
	optionsmenu.fade = 0;

	// For profiles:
	memset(setup_player, 0, sizeof(setup_player));
	optionsmenu.profile = NULL;
	optionsmenu.profilemenu = false;
	optionsmenu.resetprofilemenu = false;
}

void M_InitOptions(INT32 choice)
{
	(void)choice;

	OPTIONS_MainDef.menuitems[mopt_gameplay].status = IT_STRING | IT_TRANSTEXT;
	OPTIONS_MainDef.menuitems[mopt_server].status = IT_STRING | IT_TRANSTEXT;

	// enable gameplay & server options under the right circumstances.
	if (gamestate == GS_MENU
		|| ((server || IsPlayerAdmin(consoleplayer)) && K_CanChangeRules(false)))
	{
		OPTIONS_MainDef.menuitems[mopt_gameplay].status = IT_STRING | IT_CALL;
		OPTIONS_MainDef.menuitems[mopt_server].status = IT_STRING | IT_CALL;
		OPTIONS_GameplayDef.menuitems[gopt_encore].status =
			(M_SecretUnlocked(SECRET_ENCORE, false) ? (IT_STRING | IT_CVAR) : IT_DISABLED);
	}

	// Data Options
	OPTIONS_DataAdvancedDef.menuitems[daopt_addon].status = (M_SecretUnlocked(SECRET_ADDONS, true)
		? (IT_STRING | IT_SUBMENU)
		: (IT_NOTHING | IT_SPACE));
	OPTIONS_DataDef.menuitems[dopt_erase].status = (gamestate == GS_MENU
		? (IT_STRING | IT_SUBMENU)
		: (IT_TRANSTEXT2 | IT_SPACE));

	M_ResetOptions();

	// So that pause doesn't go to the main menu...
	OPTIONS_MainDef.prevMenu = currentMenu;

	M_SetupNextMenu(&OPTIONS_MainDef, false);
}

// Prepares changing the colour of the background
void M_OptionsChangeBGColour(INT16 newcolour)
{
	optionsmenu.fade = 10;
	optionsmenu.lastcolour = optionsmenu.currcolour;
	optionsmenu.currcolour = newcolour;
}

boolean M_OptionsQuit(void)
{
	optionsmenu.toptx = 140;
	optionsmenu.topty = 70;

	// Reset button behaviour because profile menu is different, since of course it is.
	if (optionsmenu.resetprofilemenu)
	{
		optionsmenu.profilemenu = false;
		optionsmenu.profile = NULL;
		optionsmenu.resetprofilemenu = false;
	}

	return true;	// Always allow quitting, duh.
}

void M_OptionsTick(void)
{
	boolean instanttransmission = optionsmenu.ticker == 0 && menuwipe;

	optionsmenu.ticker++;

	// Move the button for cool animations
	if (currentMenu == &OPTIONS_MainDef)
	{
		M_OptionsQuit();	// ...So now this is used here.
	}
	else if (optionsmenu.profile == NULL)	// Not currently editing a profile (otherwise we're using these variables for other purposes....)
	{
		// I don't like this, it looks like shit but it needs to be done..........
		if (optionsmenu.profilemenu)
		{
			optionsmenu.toptx = 440;
			optionsmenu.topty = 70+1;
		}
		else if (currentMenu == &OPTIONS_GameplayItemsDef)
		{
			optionsmenu.toptx = -160; // off the side of the screen
			optionsmenu.topty = 50;
		}
		else
		{
			optionsmenu.toptx = 160;
			optionsmenu.topty = 50;
		}
	}

	// Handle the background stuff:
	if (optionsmenu.fade)
		optionsmenu.fade--;

	// change the colour if we aren't matching the current menu colour
	if (instanttransmission)
	{
		optionsmenu.currcolour = currentMenu->extra1;
		optionsmenu.offset.start = optionsmenu.fade = 0;

		optionsmenu.optx = optionsmenu.toptx;
		optionsmenu.opty = optionsmenu.topty;
	}
	else
	{
		if (optionsmenu.fade)
			optionsmenu.fade--;
		if (optionsmenu.currcolour != currentMenu->extra1)
			M_OptionsChangeBGColour(currentMenu->extra1);

		if (optionsmenu.optx != optionsmenu.toptx || optionsmenu.opty != optionsmenu.topty)
		{
			tic_t t = I_GetTime();
			tic_t n = t - optionsmenu.topt_start;
			if (n == M_OPTIONS_OFSTIME)
			{
				optionsmenu.optx = optionsmenu.toptx;
				optionsmenu.opty = optionsmenu.topty;
			}
			else if (n > M_OPTIONS_OFSTIME)
			{
				optionsmenu.topt_start = I_GetTime();
			}
		}

		M_GonerCheckLooking();
	}

	// And one last giggle...
	if (shitsfree)
		shitsfree--;
}
static void M_OptionsMenuGoto(menu_t *assignment)
{
	assignment->prevMenu = currentMenu;
	assignment->music = currentMenu->music;
	M_SetupNextMenu(assignment, false);
	if (currentMenu != &OPTIONS_MainDef)
	{
		optionsmenu.ticker = 0;
		M_OptionsTick();
	}
}

void M_VideoOptions(INT32 choice)
{
	(void)choice;
	M_OptionsMenuGoto(&OPTIONS_VideoDef);
	M_GonerResetLooking(GDGONER_VIDEO);
}

void M_SoundOptions(INT32 choice)
{
	(void)choice;
	M_OptionsMenuGoto(&OPTIONS_SoundDef);
	M_GonerResetLooking(GDGONER_SOUND);
}

void M_GameplayOptions(INT32 choice)
{
	(void)choice;
	OPTIONS_GameplayItemsDef.music = currentMenu->music;
	M_OptionsMenuGoto(&OPTIONS_GameplayDef);
	OPTIONS_MainDef.lastOn = mopt_gameplay;
}

void M_ServerOptions(INT32 choice)
{
	(void)choice;
	OPTIONS_ServerAdvancedDef.music = currentMenu->music;
	M_OptionsMenuGoto(&OPTIONS_ServerDef);
	OPTIONS_MainDef.lastOn = mopt_server;
}

boolean M_OptionsInputs(INT32 ch)
{

	const UINT8 pid = 0;
	(void)ch;

	if (menucmd[pid].dpad_ud > 0)
	{
		M_SetMenuDelay(pid);
		optionsmenu.offset.dist = 48;
		M_NextOpt();
		S_StartSound(NULL, sfx_s3k5b);

		if (itemOn == 0)
			optionsmenu.offset.dist -= currentMenu->numitems*48;

		optionsmenu.offset.start = I_GetTime();

		return true;
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		M_SetMenuDelay(pid);
		optionsmenu.offset.dist = -48;
		M_PrevOpt();
		S_StartSound(NULL, sfx_s3k5b);

		if (itemOn == currentMenu->numitems-1)
			optionsmenu.offset.dist += currentMenu->numitems*48;

		optionsmenu.offset.start = I_GetTime();

		return true;
	}
	else if (M_MenuConfirmPressed(pid))
	{

		if (currentMenu->menuitems[itemOn].status & IT_TRANSTEXT)
			return true;	// No.

		optionsmenu.optx = 140;
		optionsmenu.opty = 70;	// Default position for the currently selected option.
		return false;	// Don't eat.
	}
	return false;
}
