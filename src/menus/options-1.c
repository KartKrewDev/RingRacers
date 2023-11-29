/// \file  menus/options-1.c
/// \brief Options Menu

#include "../k_menu.h"
#include "../k_grandprix.h" // K_CanChangeRules
#include "../m_cond.h" // Condition Sets
#include "../s_sound.h"

// options menu --  see mopt_e
menuitem_t OPTIONS_Main[] =
{

	{IT_STRING | IT_CALL, "Profile Setup", "Remap keys & buttons to your likings.",
		NULL, {.routine = M_ProfileSelectInit}, 0, 0},

	{IT_STRING | IT_CALL, "Video Options", "Change video settings such as the resolution.",
		NULL, {.routine = M_VideoOptions}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Sound Options", "Adjust various sound settings such as the volume.",
		NULL, {.routine = M_SoundOptions}, 0, 0},

	{IT_STRING | IT_SUBMENU, "HUD Options", "Options related to the Heads-Up Display.",
		NULL, {.submenu = &OPTIONS_HUDDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Gameplay Options", "Change various game related options",
		NULL, {.submenu = &OPTIONS_GameplayDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Server Options", "Change various specific options for your game server.",
		NULL, {.submenu = &OPTIONS_ServerDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Data Options", "Miscellaneous data options such as the screenshot format.",
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
	0,
	NULL,
	2, 5,
	M_DrawOptions,
	M_OptionsTick,
	NULL,
	NULL,
	M_OptionsInputs
};

struct optionsmenu_s optionsmenu;

void M_ResetOptions(void)
{
	optionsmenu.ticker = 0;
	optionsmenu.offset = 0;

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
		OPTIONS_MainDef.menuitems[mopt_gameplay].status = IT_STRING | IT_SUBMENU;
		OPTIONS_MainDef.menuitems[mopt_server].status = IT_STRING | IT_SUBMENU;
		OPTIONS_GameplayDef.menuitems[gopt_encore].status =
			(M_SecretUnlocked(SECRET_ENCORE, false) ? (IT_STRING | IT_CVAR) : IT_DISABLED);
	}

	OPTIONS_DataDef.menuitems[dopt_addon].status = (M_SecretUnlocked(SECRET_ADDONS, true)
		? (IT_STRING | IT_SUBMENU)
		: (IT_TRANSTEXT2 | IT_SPACE));
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
	optionsmenu.toptx = 140-1;
	optionsmenu.topty = 70+1;

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

	if (!instanttransmission)
	{
		optionsmenu.offset /= 2;
	
		optionsmenu.optx += (optionsmenu.toptx - optionsmenu.optx)/2;
		optionsmenu.opty += (optionsmenu.topty - optionsmenu.opty)/2;

		if (abs(optionsmenu.optx - optionsmenu.opty) < 2)
		{
			optionsmenu.optx = optionsmenu.toptx;
			optionsmenu.opty = optionsmenu.topty;	// Avoid awkward 1 px errors.
		}
	}

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
			optionsmenu.toptx = 420;
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
		optionsmenu.offset = optionsmenu.fade = 0;

		optionsmenu.optx = optionsmenu.toptx;
		optionsmenu.opty = optionsmenu.topty;
	}
	else
	{
		if (optionsmenu.fade)
			optionsmenu.fade--;
		if (optionsmenu.currcolour != currentMenu->extra1)
			M_OptionsChangeBGColour(currentMenu->extra1);
	}

	// And one last giggle...
	if (shitsfree)
		shitsfree--;
}
static void M_OptionsMenuGoto(menu_t *assignment)
{
	assignment->prevMenu = currentMenu;
	M_SetupNextMenu(assignment, false);
}

void M_VideoOptions(INT32 choice)
{
	(void)choice;
	M_OptionsMenuGoto(&OPTIONS_VideoDef);
}

void M_SoundOptions(INT32 choice)
{
	(void)choice;
	M_OptionsMenuGoto(&OPTIONS_SoundDef);
}

boolean M_OptionsInputs(INT32 ch)
{

	const UINT8 pid = 0;
	(void)ch;

	if (menucmd[pid].dpad_ud > 0)
	{
		M_SetMenuDelay(pid);
		optionsmenu.offset += 48;
		M_NextOpt();
		S_StartSound(NULL, sfx_s3k5b);

		if (itemOn == 0)
			optionsmenu.offset -= currentMenu->numitems*48;


		return true;
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		M_SetMenuDelay(pid);
		optionsmenu.offset -= 48;
		M_PrevOpt();
		S_StartSound(NULL, sfx_s3k5b);

		if (itemOn == currentMenu->numitems-1)
			optionsmenu.offset += currentMenu->numitems*48;


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
