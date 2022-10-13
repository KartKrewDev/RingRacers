/// \file  k_menufunc.c
/// \brief SRB2Kart's menu functions

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "k_menu.h"

#include "doomdef.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "console.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"
#include "m_argv.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

// Addfile
#include "filesrch.h"

#include "v_video.h"
#include "i_video.h"
#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_setup.h"
#include "f_finale.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "mserv.h"
#include "m_misc.h"
#include "m_anigif.h"
#include "byteptr.h"
#include "st_stuff.h"
#include "i_sound.h"
#include "i_time.h"
#include "k_kart.h"
#include "k_follower.h"
#include "d_player.h" // KITEM_ constants
#include "doomstat.h" // MAXSPLITSCREENPLAYERS
#include "k_grandprix.h" // MAXSPLITSCREENPLAYERS

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#include "r_skins.h"

#if defined(HAVE_SDL)
#include "SDL.h"
#if SDL_VERSION_ATLEAST(2,0,0)
#include "sdl/sdlmain.h" // JOYSTICK_HOTPLUG
#endif
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
boolean fromlevelselect = false;

// current menudef
menu_t *currentMenu = &MAIN_ProfilesDef;

char dummystaffname[22];

INT16 itemOn = 0; // menu item skull is on, Hack by Tails 09-18-2002
INT16 skullAnimCounter = 8; // skull animation counter
struct menutransition_s menutransition; // Menu transition properties

INT32 menuKey = -1; // keyboard key pressed for menu
menucmd_t menucmd[MAXSPLITSCREENPLAYERS];

// message prompt struct
struct menumessage_s menumessage;

// Typing "sub"-menu
struct menutyping_s menutyping;

// keyboard layouts
INT16 virtualKeyboard[5][13] = {

	{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0},
	{'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0},
	{'a', 's', 'd', 'f', 'g', 'h', 'i', 'j', 'k', 'l', ';', '\'', '\\'},
	{'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0},
	{KEY_SPACE, KEY_RSHIFT, KEY_BACKSPACE, KEY_CAPSLOCK, KEY_ENTER, 0, 0, 0, 0, 0, 0, 0, 0}
};

INT16 shift_virtualKeyboard[5][13] = {

	{'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0},
	{'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0},
	{'A', 'S', 'D', 'F', 'G', 'H', 'I', 'J', 'K', 'L', ':', '\"', '|'},
	{'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0},
	{KEY_SPACE, KEY_RSHIFT, KEY_BACKSPACE, KEY_CAPSLOCK, KEY_ENTER, 0, 0, 0, 0, 0, 0, 0, 0}
};

// finish wipes between screens
boolean menuwipe = false;

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

// Consvar onchange functions
static void Dummymenuplayer_OnChange(void);
static void Dummystaff_OnChange(void);

consvar_t cv_showfocuslost = CVAR_INIT ("showfocuslost", "Yes", CV_SAVE, CV_YesNo, NULL);

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = CVAR_INIT ("chooseskin", DEFAULTSKIN, CV_HIDDEN, skins_cons_t, NULL);

consvar_t cv_menujam_update = CVAR_INIT ("menujam_update", "Off", CV_SAVE, CV_OnOff, NULL);
static CV_PossibleValue_t menujam_cons_t[] = {{0, "menu"}, {1, "menu2"}, {2, "menu3"}, {0, NULL}};
static consvar_t cv_menujam = CVAR_INIT ("menujam", "0", CV_SAVE, menujam_cons_t, NULL);

// This gametype list is integral for many different reasons.
// When you add gametypes here, don't forget to update them in dehacked.c and doomstat.h!
CV_PossibleValue_t gametype_cons_t[NUMGAMETYPES+1];

static CV_PossibleValue_t serversort_cons_t[] = {
	{0,"Ping"},
	{1,"AVG. Power Level"},
	{2,"Most Players"},
	{3,"Least Players"},
	{4,"Max Player Slots"},
	{5,"Gametype"},
	{0,NULL}
};
consvar_t cv_serversort = CVAR_INIT ("serversort", "Ping", CV_CALL, serversort_cons_t, M_SortServerList);

// first time memory
consvar_t cv_tutorialprompt = CVAR_INIT ("tutorialprompt", "On", CV_SAVE, CV_OnOff, NULL);

// autorecord demos for time attack
static consvar_t cv_autorecord = CVAR_INIT ("autorecord", "Yes", 0, CV_YesNo, NULL);

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show Character"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_besttime  = CVAR_INIT ("ghost_besttime",  "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_bestlap   = CVAR_INIT ("ghost_bestlap",   "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_last      = CVAR_INIT ("ghost_last",      "Show All", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_guest     = CVAR_INIT ("ghost_guest",     "Show", CV_SAVE, ghost2_cons_t, NULL);
consvar_t cv_ghost_staff     = CVAR_INIT ("ghost_staff",     "Show", CV_SAVE, ghost2_cons_t, NULL);

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.
static void Splitplayers_OnChange(void);
CV_PossibleValue_t splitplayers_cons_t[] = {{1, "One"}, {2, "Two"}, {3, "Three"}, {4, "Four"}, {0, NULL}};
consvar_t cv_splitplayers = CVAR_INIT ("splitplayers", "One", CV_CALL, splitplayers_cons_t, Splitplayers_OnChange);

static CV_PossibleValue_t dummymenuplayer_cons_t[] = {{0, "NOPE"}, {1, "P1"}, {2, "P2"}, {3, "P3"}, {4, "P4"}, {0, NULL}};
static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static CV_PossibleValue_t dummyspectate_cons_t[] = {{0, "Spectator"}, {1, "Playing"}, {0, NULL}};
static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static CV_PossibleValue_t dummystaff_cons_t[] = {{0, "MIN"}, {100, "MAX"}, {0, NULL}};
static CV_PossibleValue_t dummygametype_cons_t[] = {{0, "Race"}, {1, "Battle"}, {0, NULL}};

//static consvar_t cv_dummymenuplayer = CVAR_INIT ("dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange);
static consvar_t cv_dummyteam = CVAR_INIT ("dummyteam", "Spectator", CV_HIDDEN, dummyteam_cons_t, NULL);
//static cv_dummyspectate = CVAR_INITconsvar_t  ("dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL);
static consvar_t cv_dummyscramble = CVAR_INIT ("dummyscramble", "Random", CV_HIDDEN, dummyscramble_cons_t, NULL);
static consvar_t cv_dummystaff = CVAR_INIT ("dummystaff", "0", CV_HIDDEN|CV_CALL, dummystaff_cons_t, Dummystaff_OnChange);
consvar_t cv_dummygametype = CVAR_INIT ("dummygametype", "Race", CV_HIDDEN, dummygametype_cons_t, NULL);
consvar_t cv_dummyip = CVAR_INIT ("dummyip", "", CV_HIDDEN, NULL, NULL);
consvar_t cv_dummymenuplayer = CVAR_INIT ("dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange);
consvar_t cv_dummyspectate = CVAR_INIT ("dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL);

consvar_t cv_dummyprofilename = CVAR_INIT ("dummyprofilename", "", CV_HIDDEN, NULL, NULL);
consvar_t cv_dummyprofileplayername = CVAR_INIT ("dummyprofileplayername", "", CV_HIDDEN, NULL, NULL);
consvar_t cv_dummyprofilekickstart = CVAR_INIT ("dummyprofilekickstart", "Off", CV_HIDDEN, CV_OnOff, NULL);

consvar_t cv_dummygpdifficulty = CVAR_INIT ("dummygpdifficulty", "Normal", CV_HIDDEN, gpdifficulty_cons_t, NULL);
consvar_t cv_dummykartspeed = CVAR_INIT ("dummykartspeed", "Normal", CV_HIDDEN, dummykartspeed_cons_t, NULL);
consvar_t cv_dummygpencore = CVAR_INIT ("dummygpencore", "Off", CV_HIDDEN, CV_OnOff, NULL);

static void M_UpdateAddonsSearch(void);
consvar_t cv_dummyaddonsearch = CVAR_INIT ("dummyaddonsearch", "", CV_HIDDEN|CV_CALL|CV_NOINIT, NULL, M_UpdateAddonsSearch);

static CV_PossibleValue_t dummymatchbots_cons_t[] = {
	{0, "Off"},
	{1, "Lv.1"},
	{2, "Lv.2"},
	{3, "Lv.3"},
	{4, "Lv.4"},
	{5, "Lv.5"},
	{6, "Lv.6"},
	{7, "Lv.7"},
	{8, "Lv.8"},
	{9, "Lv.9"},
	{10, "Lv.10"},
	{11, "Lv.11"},
	{12, "Lv.12"},
	{13, "Lv.MAX"},
	{0, NULL}
};
consvar_t cv_dummymatchbots = CVAR_INIT ("dummymatchbots", "Off", CV_HIDDEN, dummymatchbots_cons_t, NULL);

// for server fetch threads...
M_waiting_mode_t m_waiting_mode = M_NOT_WAITING;

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

static void Dummymenuplayer_OnChange(void)
{
	if (cv_dummymenuplayer.value < 1)
		CV_StealthSetValue(&cv_dummymenuplayer, splitscreen+1);
	else if (cv_dummymenuplayer.value > splitscreen+1)
		CV_StealthSetValue(&cv_dummymenuplayer, 1);
}

static void Dummystaff_OnChange(void)
{
#ifdef STAFFGHOSTS
	lumpnum_t l;

	dummystaffname[0] = '\0';

	if ((l = W_CheckNumForName(va("%sS01",G_BuildMapName(cv_nextmap.value)))) == LUMPERROR)
	{
		CV_StealthSetValue(&cv_dummystaff, 0);
		return;
	}
	else
	{
		char *temp = dummystaffname;
		UINT8 numstaff = 1;
		while (numstaff < 99 && (l = W_CheckNumForName(va("%sS%02u",G_BuildMapName(cv_nextmap.value),numstaff+1))) != LUMPERROR)
			numstaff++;

		if (cv_dummystaff.value < 1)
			CV_StealthSetValue(&cv_dummystaff, numstaff);
		else if (cv_dummystaff.value > numstaff)
			CV_StealthSetValue(&cv_dummystaff, 1);

		if ((l = W_CheckNumForName(va("%sS%02u",G_BuildMapName(cv_nextmap.value), cv_dummystaff.value))) == LUMPERROR)
			return; // shouldn't happen but might as well check...

		G_UpdateStaffGhostName(l);

		while (*temp)
			temp++;

		sprintf(temp, " - %d", cv_dummystaff.value);
	}
#endif //#ifdef STAFFGHOSTS
}

void Screenshot_option_Onchange(void)
{
	// Screenshot opt is at #3, 0 based array obv.
	OPTIONS_DataScreenshot[2].status =
		(cv_screenshot_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);

}

void Moviemode_mode_Onchange(void)
{
#if 0
	INT32 i, cstart, cend;
	for (i = op_screenshot_gif_start; i <= op_screenshot_apng_end; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_DISABLED;

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			cstart = op_screenshot_gif_start;
			cend = op_screenshot_gif_end;
			break;
		case MM_APNG:
			cstart = op_screenshot_apng_start;
			cend = op_screenshot_apng_end;
			break;
		default:
			return;
	}
	for (i = cstart; i <= cend; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_STRING|IT_CVAR;
#endif
}

void Moviemode_option_Onchange(void)
{
	// opt 7 in a 0 based array, you get the idea...
	OPTIONS_DataScreenshot[6].status =
		(cv_movie_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

void Addons_option_Onchange(void)
{
	// Option 2 will always be the textbar.
	// (keep in mind this is a 0 indexed array and the first element is a header...)
	OPTIONS_DataAddon[2].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

static void M_EraseDataResponse(INT32 ch)
{
	if (ch == MA_NO)
		return;

	S_StartSound(NULL, sfx_itrole); // bweh heh heh

	// Delete the data
	if (optionsmenu.erasecontext == 2)
	{
		// SRB2Kart: This actually needs to be done FIRST, so that you don't immediately regain playtime/matches secrets
		totalplaytime = 0;
		matchesplayed = 0;
	}
	if (optionsmenu.erasecontext != 1)
		G_ClearRecords();
	if (optionsmenu.erasecontext != 0)
		M_ClearSecrets();

	F_StartIntro();
	M_ClearMenus(true);
}

void M_EraseData(INT32 choice)
{
	const char *eschoice, *esstr = M_GetText("Are you sure you want to erase\n%s?\n\n(Press A to confirm)\n");

	optionsmenu.erasecontext = (UINT8)choice;

	if (choice == 0)
		eschoice = M_GetText("Time Attack data");
	else if (choice == 1)
		eschoice = M_GetText("Secrets data");
	else
		eschoice = M_GetText("ALL game data");

	M_StartMessage(va(esstr, eschoice), FUNCPTRCAST(M_EraseDataResponse), MM_YESNO);
}


// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

UINT16 nummenucolors = 0;

void M_AddMenuColor(UINT16 color) {
	menucolor_t *c;

	if (color >= numskincolors) {
		CONS_Printf("M_AddMenuColor: color %d does not exist.",color);
		return;
	}

	// SRB2Kart: I do not understand vanilla doesn't need this but WE do???!?!??!
	if (!skincolors[color].accessible) {
		return;
	}

	c = (menucolor_t *)malloc(sizeof(menucolor_t));
	c->color = color;
	if (menucolorhead == NULL) {
		c->next = c;
		c->prev = c;
		menucolorhead = c;
		menucolortail = c;
	} else {
		c->next = menucolorhead;
		c->prev = menucolortail;
		menucolortail->next = c;
		menucolorhead->prev = c;
		menucolortail = c;
	}

	nummenucolors++;
}

void M_MoveColorBefore(UINT16 color, UINT16 targ) {
	menucolor_t *look, *c = NULL, *t = NULL;

	if (color == targ)
		return;
	if (color >= numskincolors) {
		CONS_Printf("M_MoveColorBefore: color %d does not exist.",color);
		return;
	}
	if (targ >= numskincolors) {
		CONS_Printf("M_MoveColorBefore: target color %d does not exist.",targ);
		return;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			c = look;
		else if (look->color == targ)
			t = look;
		if (c != NULL && t != NULL)
			break;
		if (look==menucolortail)
			return;
	}

	if (c == t->prev)
		return;

	if (t==menucolorhead)
		menucolorhead = c;
	if (c==menucolortail)
		menucolortail = c->prev;

	c->prev->next = c->next;
	c->next->prev = c->prev;

	c->prev = t->prev;
	c->next = t;
	t->prev->next = c;
	t->prev = c;
}

void M_MoveColorAfter(UINT16 color, UINT16 targ) {
	menucolor_t *look, *c = NULL, *t = NULL;

	if (color == targ)
		return;
	if (color >= numskincolors) {
		CONS_Printf("M_MoveColorAfter: color %d does not exist.\n",color);
		return;
	}
	if (targ >= numskincolors) {
		CONS_Printf("M_MoveColorAfter: target color %d does not exist.\n",targ);
		return;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			c = look;
		else if (look->color == targ)
			t = look;
		if (c != NULL && t != NULL)
			break;
		if (look==menucolortail)
			return;
	}

	if (t == c->prev)
		return;

	if (t==menucolortail)
		menucolortail = c;
	else if (c==menucolortail)
		menucolortail = c->prev;

	c->prev->next = c->next;
	c->next->prev = c->prev;

	c->next = t->next;
	c->prev = t;
	t->next->prev = c;
	t->next = c;
}

UINT16 M_GetColorBefore(UINT16 color, UINT16 amount, boolean follower)
{
	menucolor_t *look = NULL;

	for (; amount > 0; amount--)
	{
		if (follower == true)
		{
			if (color == FOLLOWERCOLOR_OPPOSITE)
			{
				look = menucolortail;
				color = menucolortail->color;
				continue;
			}

			if (color == FOLLOWERCOLOR_MATCH)
			{
				look = NULL;
				color = FOLLOWERCOLOR_OPPOSITE;
				continue;
			}

			if (color == menucolorhead->color)
			{
				look = NULL;
				color = FOLLOWERCOLOR_MATCH;
				continue;
			}
		}

		if (color == 0 || color >= numskincolors)
		{
			CONS_Printf("M_GetColorBefore: color %d does not exist.\n",color);
			return 0;
		}

		if (look == NULL)
		{
			for (look = menucolorhead;; look = look->next)
			{
				if (look->color == color)
				{
					break;
				}
				if (look == menucolortail)
				{
					return 0;
				}
			}
		}

		look = look->prev;
		color = look->color;
	}
	return color;
}

UINT16 M_GetColorAfter(UINT16 color, UINT16 amount, boolean follower)
{
	menucolor_t *look = NULL;

	for (; amount > 0; amount--)
	{
		if (follower == true)
		{
			if (color == menucolortail->color)
			{
				look = NULL;
				color = FOLLOWERCOLOR_OPPOSITE;
				continue;
			}

			if (color == FOLLOWERCOLOR_OPPOSITE)
			{
				look = NULL;
				color = FOLLOWERCOLOR_MATCH;
				continue;
			}

			if (color == FOLLOWERCOLOR_MATCH)
			{
				look = menucolorhead;
				color = menucolorhead->color;
				continue;
			}
		}

		if (color == 0 || color >= numskincolors)
		{
			CONS_Printf("M_GetColorAfter: color %d does not exist.\n",color);
			return 0;
		}

		if (look == NULL)
		{
			for (look = menucolorhead;; look = look->next)
			{
				if (look->color == color)
				{
					break;
				}
				if (look == menucolortail)
				{
					return 0;
				}
			}
		}

		look = look->next;
		color = look->color;
	}
	return color;
}

void M_InitPlayerSetupColors(void) {
	UINT8 i;
	numskincolors = SKINCOLOR_FIRSTFREESLOT;
	menucolorhead = menucolortail = NULL;
	for (i=0; i<numskincolors; i++)
		M_AddMenuColor(i);
}

void M_FreePlayerSetupColors(void) {
	menucolor_t *look = menucolorhead, *tmp;

	if (menucolorhead==NULL)
		return;

	while (true) {
		if (look != menucolortail) {
			tmp = look;
			look = look->next;
			free(tmp);
		} else {
			free(look);
			return;
		}
	}

	menucolorhead = menucolortail = NULL;
}

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cv = currentMenu->menuitems[itemOn].itemaction.cvar;

	// Backspace sets values to default value
	if (choice == -1)
	{
		CV_Set(cv, cv->defaultvalue);
		return;
	}

	choice = (choice<<1) - 1;

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

static boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = currentMenu->menuitems[itemOn].itemaction.cvar;
	char buf[MAXSTRINGLENGTH];
	size_t len;

	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				S_StartSound(NULL, sfx_s3k5b); // Tails
				M_Memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		case KEY_DEL:
			if (cv->string[0])
			{
				S_StartSound(NULL, sfx_s3k5b); // Tails
				CV_Set(cv, "");
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					S_StartSound(NULL, sfx_s3k5b); // Tails
					M_Memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}

	return false;
}

static boolean M_NextOpt(void)
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

static boolean M_PrevOpt(void)
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

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	menuKey = -1;

	if (dedicated || (demo.playback && demo.title)
		|| gamestate == GS_INTRO || gamestate == GS_CUTSCENE || gamestate == GS_GAMEEND
		|| gamestate == GS_CREDITS || gamestate == GS_EVALUATION)
	{
		return false;
	}

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}

	if (ev->type == ev_keydown && ev->data1 < NUMKEYS)
	{
		// Record keyboard presses
		menuKey = ev->data1;
	}

	M_MapMenuControls(ev);

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
			if (chat_on)
			{
				HU_clearChatChars();
				chat_on = false;
			}
			else
			{
				M_StartControlPanel();
			}

			return true;
		}

		noFurtherInput = false; // turns out we didn't care
		return false;
	}

	// Typing for CV_IT_STRING
	if (menutyping.active && !menutyping.menutypingclose && menutyping.keyboardtyping)
	{
		M_ChangeStringCvar(menuKey);
	}

	// We're in the menu itself now.
	// M_Ticker will take care of the rest.
	return true;
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

	// No instantly skipping the titlescreen.
	// (We can change this timer later when extra animation is added.)
	if (gamestate == GS_TITLESCREEN && finalecount < 1)
		return;

	// intro might call this repeatedly
	if (menuactive)
	{
		CON_ToggleOff(); // move away console
		return;
	}

	if (gamestate == GS_TITLESCREEN) // Set up menu state
	{
		G_SetGamestate(GS_MENU);

		gameaction = ga_nothing;
		paused = false;
		CON_ToggleOff();

		if (cv_menujam_update.value)
		{
			CV_AddValue(&cv_menujam, 1);
			CV_SetValue(&cv_menujam_update, 0);
		}

		S_ChangeMusicInternal(cv_menujam.string, true);
	}

	menuactive = true;

	if (!Playing())
	{
		if (cv_currprofile.value == -1) // Only ask once per session.
		{
			// Make sure the profile data is ready now since we need to select a profile.
			M_ResetOptions();

			// we need to do this before setting ApplyProfile otherwise funky things are going to happen.
			currentMenu = &MAIN_ProfilesDef;
			optionsmenu.profilen = cv_ttlprofilen.value;

			// options don't need initializing here.
			PR_ApplyProfile(0, 0); // apply guest profile to player 0 by default.
			// this is to garantee that the controls aren't fucked up.

			// make sure we don't overstep that.
			if (optionsmenu.profilen > PR_GetNumProfiles())
				optionsmenu.profilen = PR_GetNumProfiles();
			else if (optionsmenu.profilen < 0)
				optionsmenu.profilen = 0;

			currentMenu->lastOn = 0;

			CV_StealthSetValue(&cv_currprofile, -1); // Make sure to reset that as it is set by PR_ApplyProfile which we kind of hack together to force it.
		}
		else
		{
			currentMenu = &MainDef;
		}
	}
	else
	{
		if (demo.playback)
		{
			currentMenu = &PAUSE_PlaybackMenuDef;
		}
		else
		{
			M_OpenPauseMenu();
		}
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

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

	if (currentMenu == &MessageDef) // Oh sod off!
		currentMenu = &MainDef; // Not like it matters

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
			F_RunWipe(wipedefs[wipe_menu_toblack], false, "FADEMAP0", false, false);
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

static void M_UpdateMenuCMD(UINT8 i)
{
	UINT8 mp = max(1, setup_numplayers);

	menucmd[i].dpad_ud = 0;
	menucmd[i].dpad_lr = 0;

	menucmd[i].buttonsHeld = menucmd[i].buttons;
	menucmd[i].buttons = 0;

	if (G_PlayerInputDown(i, gc_up, mp)) { menucmd[i].dpad_ud--; }
	if (G_PlayerInputDown(i, gc_down, mp)) { menucmd[i].dpad_ud++; }

	if (G_PlayerInputDown(i, gc_left, mp)) { menucmd[i].dpad_lr--; }
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

void M_MapMenuControls(event_t *ev)
{
	INT32 i;

	if (ev)
	{
		// update keys current state
		G_MapEventsToControls(ev);
	}

	// Update menu CMD
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		M_UpdateMenuCMD(i);
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
static boolean M_MenuConfirmPressed(UINT8 pid)
{
	 return M_MenuButtonPressed(pid, MBT_A);
}

// Returns true if we press the Cancel button
static boolean M_MenuBackPressed(UINT8 pid)
{
	 return (M_MenuButtonPressed(pid, MBT_B) || M_MenuButtonPressed(pid, MBT_X));
}

// Retrurns true if we press the tertiary option button (C)
static boolean M_MenuExtraPressed(UINT8 pid)
{
	 return M_MenuButtonPressed(pid, MBT_C);
}


// Updates the x coordinate of the keybord so prevent it from going in weird places
static void M_UpdateKeyboardX(void)
{
	// 0s are only at the rightmost edges of the keyboard table, so just go backwards until we get something.
	while (!virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx])
		menutyping.keyboardx--;
}

static boolean M_IsTypingKey(INT32 key)
{
	return key == KEY_BACKSPACE || key == KEY_ENTER ||
		key == KEY_ESCAPE || key == KEY_DEL || isprint(key);
}

static void M_MenuTypingInput(INT32 key)
{

	const UINT8 pid = 0;

	// Fade-in

	if (menutyping.menutypingclose)	// closing
	{
		menutyping.menutypingfade--;
		if (!menutyping.menutypingfade)
			menutyping.active = false;

		return;	// prevent inputs while closing the menu.
	}
	else					// opening
	{
		menutyping.menutypingfade++;
		if (menutyping.menutypingfade > 9)	// Don't fade all the way, but have it VERY strong to be readable
			menutyping.menutypingfade = 9;
		else if (menutyping.menutypingfade < 9)
			return;	// Don't allow typing until it's fully opened.
	}

	// Determine when to check for keyboard inputs or controller inputs using menuKey, which is the key passed here as argument.
	if (!menutyping.keyboardtyping)	// controller inputs
	{
		// we pressed a keyboard input that's not any of our buttons
		if (M_IsTypingKey(key) && menucmd[pid].dpad_lr == 0 && menucmd[pid].dpad_ud == 0
			&& !(menucmd[pid].buttons & MBT_A)
			&& !(menucmd[pid].buttons & MBT_B)
			&& !(menucmd[pid].buttons & MBT_C)
			&& !(menucmd[pid].buttons & MBT_X)
			&& !(menucmd[pid].buttons & MBT_Y)
			&& !(menucmd[pid].buttons & MBT_Z))
		{
			menutyping.keyboardtyping = true;
		}
	}
	else	// Keyboard inputs.
	{
		// On the flipside, if we're pressing any keyboard input, switch to controller inputs.
		if (!M_IsTypingKey(key) && (
			M_MenuButtonPressed(pid, MBT_A)
			|| M_MenuButtonPressed(pid, MBT_B)
			|| M_MenuButtonPressed(pid, MBT_C)
			|| M_MenuButtonPressed(pid, MBT_X)
			|| M_MenuButtonPressed(pid, MBT_Y)
			|| M_MenuButtonPressed(pid, MBT_Z)
			|| menucmd[pid].dpad_lr != 0
			|| menucmd[pid].dpad_ud != 0
		))
		{
			menutyping.keyboardtyping = false;
			return;
		}

		// OTHERWISE, process keyboard inputs for typing!
		if (key == KEY_ENTER || key == KEY_ESCAPE)
		{
			menutyping.menutypingclose = true;	// close menu.
			return;
		}

	}

	if (menucmd[pid].delay == 0 && !menutyping.keyboardtyping)	// We must check for this here because we bypass the normal delay check to allow for normal keyboard inputs
	{
		if (menucmd[pid].dpad_ud > 0)	// down
		{
			menutyping.keyboardy++;
			if (menutyping.keyboardy > 4)
				menutyping.keyboardy = 0;

			M_UpdateKeyboardX();
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_ud < 0) // up
		{
			menutyping.keyboardy--;
			if (menutyping.keyboardy < 0)
				menutyping.keyboardy = 4;

			M_UpdateKeyboardX();
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_lr > 0)	// right
		{
			menutyping.keyboardx++;
			if (!virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx])
				menutyping.keyboardx = 0;

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (menucmd[pid].dpad_lr < 0)	// left
		{
			menutyping.keyboardx--;
			if (menutyping.keyboardx < 0)
			{
				menutyping.keyboardx = 12;
				M_UpdateKeyboardX();
			}
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_MenuConfirmPressed(pid))
		{
			// Add the character. First though, check what we're pressing....
			INT16 c = virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx];
			if (menutyping.keyboardshift ^ menutyping.keyboardcapslock)
				c = shift_virtualKeyboard[menutyping.keyboardy][menutyping.keyboardx];

			if (c == KEY_RSHIFT)
				menutyping.keyboardshift = !menutyping.keyboardshift;
			else if (c == KEY_CAPSLOCK)
				menutyping.keyboardcapslock = !menutyping.keyboardcapslock;
			else if (c == KEY_ENTER)
			{
				menutyping.menutypingclose = true;	// close menu.
				return;
			}
			else
			{
				M_ChangeStringCvar((INT32)c);	// Write!
				menutyping.keyboardshift = false;			// undo shift if it had been pressed
			}

			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
		}
	}
}

static void M_HandleMenuInput(void)
{
	void (*routine)(INT32 choice); // for some casting problem
	UINT8 pid = 0; // todo: Add ability for any splitscreen player to bring up the menu.
	SINT8 lr = 0, ud = 0;

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
		M_MenuTypingInput(menuKey);
		return;
	}

	if (menucmd[pid].delay > 0)
	{
		return;
	}

	// Handle menu-specific input handling. If this returns true, we skip regular input handling.
	if (currentMenu->inputroutine)
	{
		if (currentMenu->inputroutine(menuKey))
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
				menutyping.keyboardtyping = menuKey != 0 ? true : false;	// If we entered this menu by pressing a menu Key, default to keyboard typing, otherwise use controller.
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

	// LR does nothing in the default menu, just remap as dpad.
	if (menucmd[pid].buttons & MBT_L) { lr--; }
	if (menucmd[pid].buttons & MBT_R) { lr++; }

	// Keys usable within menu
	if (ud > 0)
	{
		if (M_NextOpt())
			S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		return;
	}
	else if (ud < 0)
	{
		if (M_PrevOpt())
			S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		return;
	}
	else if (lr < 0)
	{
		if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
			|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
		{
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
			S_StartSound(NULL, sfx_s3k5b);

			if (((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CALL
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SUBMENU)
				&& (currentMenu->menuitems[itemOn].status & IT_CALLTYPE))
			{
				if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && majormods)
				{
					M_StartMessage(M_GetText("This cannot be done with complex addons\nor in a cheated game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
					return;
				}
			}

			switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
			{
				case IT_CVAR:
				case IT_ARROWS:
					routine(1); // right arrow
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
			consvar_t *cv = currentMenu->menuitems[itemOn].itemaction.cvar;

			// Make these CVar options?
			if (cv == &cv_chooseskin
				|| cv == &cv_dummystaff
				/*
				|| cv == &cv_nextmap
				|| cv == &cv_newgametype
				*/
				)
			{
				return;
			}

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
				F_RunWipe(wipedefs[wipe_menu_toblack], false, "FADEMAP0", false, false);
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
	CV_RegisterVar(&cv_dummygametype);
	CV_RegisterVar(&cv_dummyip);

	CV_RegisterVar(&cv_dummyprofilename);
	CV_RegisterVar(&cv_dummyprofileplayername);
	CV_RegisterVar(&cv_dummyprofilekickstart);

	CV_RegisterVar(&cv_dummygpdifficulty);
	CV_RegisterVar(&cv_dummykartspeed);
	CV_RegisterVar(&cv_dummygpencore);
	CV_RegisterVar(&cv_dummymatchbots);

	CV_RegisterVar(&cv_dummyaddonsearch);

	M_UpdateMenuBGImage(true);
}

// ==================================================
// MESSAGE BOX (aka: a hacked, cobbled together menu)
// ==================================================
// Because this is just a "fake" menu, these definitions are not with the others
static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0, NULL, NULL, NULL, {NULL}, 0, 0}
};

menu_t MessageDef =
{
	1,					// # of menu items
	NULL,				// previous menu       (TO HACK)
	0,					// lastOn, flags       (TO HACK)
	MessageMenu,		// menuitem_t ->
	0, 0,				// x, y                (TO HACK)
	0, 0,				// extra1, extra2
	0, 0,				// transition tics
	NULL,				// drawing routine ->
	NULL,				// ticker routine
	NULL,				// init routine
	NULL,				// quit routine
	NULL				// input routine
};

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

// default message handler
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype)
{
	const UINT8 pid = 0;
	size_t max = 0, start = 0, i, strlines;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, colors, etc. but who cares.
	strlines = 0;
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
		}
		else if (message[i] == '\n')
		{
			strlines = i;
			start = 0;
			max = 0;
			continue;
		}
		else
			max += 8;

		// Start trying to wrap if presumed length exceeds the screen width.
		if (max >= BASEVIDWIDTH && start > 0)
		{
			message[start] = '\n';
			max -= (start-strlines)*8;
			strlines = start;
			start = 0;
		}
	}

	strncpy(menumessage.message, string, MAXMENUMESSAGE);
	menumessage.flags = itemtype;
	*(void**)&menumessage.routine = routine;
	menumessage.fadetimer = 1;
	menumessage.active = true;

	start = 0;
	max = 0;

	if (!routine || menumessage.flags == MM_NOTHING)
	{
		menumessage.flags = MM_NOTHING;
		menumessage.routine = M_StopMessage;
	}

	// event routine
	if (menumessage.flags == MM_EVENTHANDLER)
	{
		*(void**)&menumessage.eroutine = routine;
		menumessage.routine = NULL;
	}

	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i;
				i = (size_t)-1; //added : 07-02-98 : damned!
				start++;
				break;
			}
		}

		if (i == strlen(message+start))
		{
			start += i;
			if (i > max)
				max = i;
		}
	}

	menumessage.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	menumessage.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	menumessage.m = (INT16)((strlines<<8)+max);

	M_SetMenuDelay(pid);	// Set menu delay to avoid setting off any of the handlers.
}

void M_StopMessage(INT32 choice)
{
	const char pid = 0;
	(void) choice;

	menumessage.active = false;
	M_SetMenuDelay(pid);
}

// regular handler for MM_NOTHING and MM_YESNO
void M_HandleMenuMessage(void)
{
	const UINT8 pid = 0;
	boolean btok = M_MenuConfirmPressed(pid);
	boolean btnok = M_MenuBackPressed(pid);

	menumessage.fadetimer++;

	if (menumessage.fadetimer > 9)
		menumessage.fadetimer = 9;


	switch (menumessage.flags)
	{
		// Send 1 to the routine if we're pressing A/B/X/Y
		case MM_NOTHING:
		{
			// send 1 if any button is pressed, 0 otherwise.
			if (btok || btnok)
				menumessage.routine(0);

			break;
		}
		// Send 1 to the routine if we're pressing A/X, 2 if B/Y, 0 otherwise.
		case MM_YESNO:
		{
			INT32 answer = MA_NONE;
			if (btok)
				answer = MA_YES;
			else if (btnok)
				answer = MA_NO;

			// send 1 if btok is pressed, 2 if nok is pressed, 0 otherwise.
			if (answer)
			{
				menumessage.routine(answer);
				M_StopMessage(0);
			}

			break;
		}
		// MM_EVENTHANDLER: In M_Responder to allow full event compat.
		default:
			break;
	}

	// if we detect any keypress, don't forget to set the menu delay regardless.
	if (btok || btnok)
		M_SetMenuDelay(pid);
}

// =========
// IMAGEDEFS
// =========

// Handles the ImageDefs.  Just a specialized function that
// uses left and right movement.
void M_HandleImageDef(INT32 choice)
{
	const UINT8 pid = 0;
	boolean exitmenu = false;
	(void) choice;

	if (menucmd[pid].dpad_lr > 0)
	{
		if (itemOn >= (INT16)(currentMenu->numitems-1))
			return;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		itemOn++;
	}
	else if (menucmd[pid].dpad_lr < 0)
	{
		if (!itemOn)
			return;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		itemOn--;
	}
	else if (M_MenuConfirmPressed(pid) || M_MenuButtonPressed(pid, MBT_X) || M_MenuButtonPressed(pid, MBT_Y))
	{
		exitmenu = true;
		M_SetMenuDelay(pid);
	}


	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

// =========
// MAIN MENU
// =========

// Quit Game
static INT32 quitsounds[] =
{
	// holy shit we're changing things up!
	// srb2kart: you ain't seen nothing yet
	sfx_kc2e,
	sfx_kc2f,
	sfx_cdfm01,
	sfx_ddash,
	sfx_s3ka2,
	sfx_s3k49,
	sfx_slip,
	sfx_tossed,
	sfx_s3k7b,
	sfx_itrolf,
	sfx_itrole,
	sfx_cdpcm9,
	sfx_s3k4e,
	sfx_s259,
	sfx_3db06,
	sfx_s3k3a,
	sfx_peel,
	sfx_cdfm28,
	sfx_s3k96,
	sfx_s3kc0s,
	sfx_cdfm39,
	sfx_hogbom,
	sfx_kc5a,
	sfx_kc46,
	sfx_s3k92,
	sfx_s3k42,
	sfx_kpogos,
	sfx_screec
};

void M_QuitResponse(INT32 ch)
{
	tic_t ptime;
	INT32 mrand;

	if (ch == MA_YES)
	{
		if (!(netgame || cht_debug))
		{
			mrand = M_RandomKey(sizeof(quitsounds) / sizeof(INT32));
			if (quitsounds[mrand])
				S_StartSound(NULL, quitsounds[mrand]);

			//added : 12-02-98: do that instead of I_WaitVbl which does not work
			ptime = I_GetTime() + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
			while (ptime > I_GetTime())
			{
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				V_DrawSmallScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_CACHE)); // Demo 3 Quit Screen Tails 06-16-2001
				I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
				I_Sleep(cv_sleep.value);
				I_UpdateTime(cv_timescale.value);
			}
		}
		I_Quit();
	}
}

void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	(void)choice;
	M_StartMessage("Are you sure you want to quit playing?\n\n(Press A to exit)", FUNCPTRCAST(M_QuitResponse), MM_YESNO);
}

// =========
// PLAY MENU
// =========

// Character Select!
// @TODO: Splitscreen handling when profiles are added into the game. ...I probably won't be the one to handle this however. -Lat'

struct setup_chargrid_s setup_chargrid[9][9];
setup_player_t setup_player[MAXSPLITSCREENPLAYERS];
struct setup_explosions_s setup_explosions[48];

UINT8 setup_numplayers = 0; // This variable is very important, it was extended to determine how many players exist in ALL menus.
tic_t setup_animcounter = 0;

UINT8 setup_page = 0;
UINT8 setup_maxpage = 0;	// For charsel page to identify alts easier...

// sets up the grid pos for the skin used by the profile.
static void M_SetupProfileGridPos(setup_player_t *p)
{
	profile_t *pr = PR_GetProfile(p->profilen);
	INT32 i;

	// While we're here, read follower values.
	p->followern = K_FollowerAvailable(pr->follower);
	p->followercolor = pr->followercolor;

	// Now position the grid for skin
	for (i = 0; i < numskins; i++)
	{
		if (!(strcmp(pr->skinname, skins[i].name)))
		{
			INT32 alt = 0;	// Hey it's my character's name!
			p->gridx = skins[i].kartspeed-1;
			p->gridy = skins[i].kartweight-1;

			// Now this put our cursor on the good alt
			while (setup_chargrid[p->gridx][p->gridy].skinlist[alt] != i)
				alt++;

			p->clonenum = alt;
			p->color = PR_GetProfileColor(pr);
			return;	// we're done here
		}
	}
}

static void M_SetupMidGameGridPos(setup_player_t *p, UINT8 num)
{
	INT32 i;

	// While we're here, read follower values.
	p->followern = cv_follower[num].value;
	p->followercolor = cv_followercolor[num].value;

	// Now position the grid for skin
	for (i = 0; i < numskins; i++)
	{
		if (!(strcmp(cv_skin[num].zstring, skins[i].name)))
		{
			INT32 alt = 0;	// Hey it's my character's name!
			p->gridx = skins[i].kartspeed-1;
			p->gridy = skins[i].kartweight-1;

			// Now this put our cursor on the good alt
			while (setup_chargrid[p->gridx][p->gridy].skinlist[alt] != i)
				alt++;

			p->clonenum = alt;
			p->color = cv_playercolor[num].value;
			return;	// we're done here
		}
	}
}


void M_CharacterSelectInit(void)
{
	UINT8 i, j;
	setup_maxpage = 0;

	// While we're editing profiles, don't unset the devices for p1
	if (gamestate == GS_MENU)
	{
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			// Un-set devices for other players.
			if (i != 0 || optionsmenu.profile)
			{
				CV_SetValue(&cv_usejoystick[i], -1);
				CONS_Printf("M_CharacterSelectInit: Device for %d set to %d\n", i, -1);
			}
		}
	}

	memset(setup_chargrid, -1, sizeof(setup_chargrid));
	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
			setup_chargrid[i][j].numskins = 0;
	}

	memset(setup_player, 0, sizeof(setup_player));
	setup_numplayers = 0;

	memset(setup_explosions, 0, sizeof(setup_explosions));
	setup_animcounter = 0;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		// Default to no follower / match colour.
		setup_player[i].followern = -1;
		setup_player[i].followercolor = FOLLOWERCOLOR_MATCH;

		// Set default selected profile to the last used profile for each player:
		// (Make sure we don't overshoot it somehow if we deleted profiles or whatnot)
		setup_player[i].profilen = min(cv_lastprofile[i].value, PR_GetNumProfiles());
	}

	for (i = 0; i < numskins; i++)
	{
		UINT8 x = skins[i].kartspeed-1;
		UINT8 y = skins[i].kartweight-1;

		if (setup_chargrid[x][y].numskins >= MAXCLONES)
			CONS_Alert(CONS_ERROR, "Max character alts reached for %d,%d\n", x+1, y+1);
		else
		{
			setup_chargrid[x][y].skinlist[setup_chargrid[x][y].numskins] = i;
			setup_chargrid[x][y].numskins++;

			setup_maxpage = max(setup_maxpage, setup_chargrid[x][y].numskins-1);
		}

		for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
		{
			if (!strcmp(cv_skin[j].string, skins[i].name))
			{
				setup_player[j].gridx = x;
				setup_player[j].gridy = y;
				setup_player[j].color = skins[i].prefcolor;

				// If we're on prpfile select, skip straight to CSSTEP_CHARS
				// do the same if we're midgame, but make sure to consider splitscreen properly.
				if ((optionsmenu.profile && j == 0) || (gamestate != GS_MENU && j <= splitscreen))
				{
					if (optionsmenu.profile)	// In menu, setting up profile character/follower
					{
						setup_player[j].profilen = optionsmenu.profilen;
						PR_ApplyProfileLight(setup_player[j].profilen, 0);
						M_SetupProfileGridPos(&setup_player[j]);
					}
					else	// gamestate != GS_MENU, in that case, assume this is whatever profile we chose to play with.
					{
						setup_player[j].profilen = cv_lastprofile[j].value;
						M_SetupMidGameGridPos(&setup_player[j], j);
					}

					// Don't reapply the profile here, it was already applied.
					setup_player[j].mdepth = CSSTEP_CHARS;
				}
			}
		}
	}

	setup_page = 0;
}

void M_CharacterSelect(INT32 choice)
{
	(void)choice;
	PLAY_CharSelectDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PLAY_CharSelectDef, false);
}

static void M_SetupReadyExplosions(setup_player_t *p)
{
	UINT8 i, j;
	UINT8 e = 0;

	while (setup_explosions[e].tics)
	{
		e++;
		if (e == CSEXPLOSIONS)
			return;
	}

	for (i = 0; i < 3; i++)
	{
		UINT8 t = 5 + (i*2);
		UINT8 offset = (i+1);

		for (j = 0; j < 4; j++)
		{
			SINT8 x = p->gridx, y = p->gridy;

			switch (j)
			{
				case 0: x += offset; break;
				case 1: x -= offset; break;
				case 2: y += offset; break;
				case 3: y -= offset; break;
			}

			if ((x < 0 || x > 8) || (y < 0 || y > 8))
				continue;

			setup_explosions[e].tics = t;
			setup_explosions[e].color = p->color;
			setup_explosions[e].x = x;
			setup_explosions[e].y = y;

			while (setup_explosions[e].tics)
			{
				e++;
				if (e == CSEXPLOSIONS)
					return;
			}
		}
	}
}


// Gets the selected follower's state for a given setup player.
static void M_GetFollowerState(setup_player_t *p)
{

	p->follower_state = &states[followers[p->followern].followstate];

	if (p->follower_state->frame & FF_ANIMATE)
		p->follower_tics = p->follower_state->var2;	// support for FF_ANIMATE
	else
		p->follower_tics = p->follower_state->tics;

	p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
}

static boolean M_DeviceAvailable(INT32 deviceID, UINT8 numPlayers)
{
	INT32 i;

	if (numPlayers == 0)
	{
		// All of them are available!
		return true;
	}

	for (i = 0; i < numPlayers; i++)
	{
		if (cv_usejoystick[i].value == deviceID)
		{
			// This one's already being used.
			return false;
		}
	}

	// This device is good to go.
	return true;
}

static boolean M_HandlePressStart(setup_player_t *p, UINT8 num)
{
	INT32 i, j;

	if (optionsmenu.profile)
		return false;	// Don't allow for the possibility of SOMEHOW another player joining in.

	// Detect B press first ... this means P1 can actually exit out of the menu.
	if (M_MenuBackPressed(num))
	{
		M_SetMenuDelay(num);

		if (num == 0)
		{
			// We're done here.
			memset(setup_player, 0, sizeof(setup_player));	// Reset this to avoid funky things with profile display.
			M_GoBack(0);
			return true;
		}

		// Don't allow this press to ever count as "start".
		return false;
	}

	if (num != setup_numplayers)
	{
		// Only detect devices for the last player.
		return false;
	}

	// Now detect new devices trying to join.
	for (i = 0; i < MAXDEVICES; i++)
	{
		if (deviceResponding[i] != true)
		{
			// No buttons are being pushed.
			continue;
		}

		if (M_DeviceAvailable(i, setup_numplayers) == true)
		{
			// Available!! Let's use this one!!

			// if P1 is setting up using keyboard (device 0), save their last used device.
			// this is to allow them to retain controller usage when they play alone.
			// Because let's face it, when you test mods, you're often lazy to grab your controller for menuing :)
			if (!i && !num)
			{
				setup_player[num].ponedevice = cv_usejoystick[num].value;
			}
			else if (num)
			{
				// For any player past player 1, set controls to default profile controls, otherwise it's generally awful to do any menuing...
				memcpy(&gamecontrol[num], gamecontroldefault, sizeof(gamecontroldefault));
			}


			CV_SetValue(&cv_usejoystick[num], i);
			CONS_Printf("M_HandlePressStart: Device for %d set to %d\n", num, i);

			for (j = num+1; j < MAXSPLITSCREENPLAYERS; j++)
			{
				// Un-set devices for other players.
				CV_SetValue(&cv_usejoystick[j], -1);
				CONS_Printf("M_HandlePressStart: Device for %d set to %d\n", j, -1);
			}

			//setup_numplayers++;
			p->mdepth = CSSTEP_PROFILE;
			S_StartSound(NULL, sfx_s3k65);

			// Prevent quick presses for multiple players
			for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
			{
				setup_player[j].delay = MENUDELAYTIME;
				M_SetMenuDelay(j);
				menucmd[j].buttonsHeld |= MBT_X;
			}

			memset(deviceResponding, false, sizeof(deviceResponding));
			return true;
		}
	}

	return false;
}

static boolean M_HandleCSelectProfile(setup_player_t *p, UINT8 num)
{
	const UINT8 maxp = PR_GetNumProfiles() -1;
	UINT8 realnum = num;	// Used for profile when using splitdevice.
	UINT8 i;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_ud > 0)
	{
		p->profilen++;
		if (p->profilen > maxp)
			p->profilen = 0;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_ud < 0)
	{
		if (p->profilen == 0)
			p->profilen = maxp;
		else
			p->profilen--;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		if (num == setup_numplayers-1)
		{

			p->mdepth = CSSTEP_NONE;
			S_StartSound(NULL, sfx_s3k5b);

			// Prevent quick presses for multiple players
			for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
			{
				setup_player[i].delay = MENUDELAYTIME;
				M_SetMenuDelay(i);
				menucmd[i].buttonsHeld |= MBT_X;
			}

			if (num > 0)
			{
				CV_StealthSetValue(&cv_usejoystick[num], -1);
				CONS_Printf("M_HandleCSelectProfile: Device for %d set to %d\n", num, -1);
			}

			return true;
		}
		else
		{
			S_StartSound(NULL, sfx_s3kb2);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuConfirmPressed(num))
	{
		SINT8 belongsTo = -1;

		if (p->profilen != PROFILE_GUEST)
		{
			for (i = 0; i < setup_numplayers; i++)
			{
				if (setup_player[i].mdepth > CSSTEP_PROFILE
					&& setup_player[i].profilen == p->profilen)
				{
					belongsTo = i;
					break;
				}
			}
		}

		if (belongsTo != -1 && belongsTo != num)
		{
			S_StartSound(NULL, sfx_s3k7b);
			M_SetMenuDelay(num);
			return false;
		}

		// Apply the profile.
		PR_ApplyProfile(p->profilen, realnum);	// Otherwise P1 would inherit the last player's profile in splitdevice and that's not what we want...
		M_SetupProfileGridPos(p);

		p->changeselect = 0;

		if (p->profilen == PROFILE_GUEST)
		{
			// Guest profile, always ask for options.
			p->mdepth = CSSTEP_CHARS;
		}
		else
		{
			p->mdepth = CSSTEP_ASKCHANGES;
		}

		S_StartSound(NULL, sfx_s3k63);
	}

	return false;

}


static void M_HandleCharAskChange(setup_player_t *p, UINT8 num)
{

	if (cv_splitdevice.value)
		num = 0;

	// there's only 2 options so lol
	if (menucmd[num].dpad_ud != 0)
	{
		p->changeselect = (p->changeselect == 0) ? 1 : 0;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->changeselect = 0;
		p->mdepth = CSSTEP_PROFILE;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (M_MenuConfirmPressed(num))
	{
		if (!p->changeselect)
		{
			// no changes
			M_GetFollowerState(p);
			p->mdepth = CSSTEP_READY;
			p->delay = TICRATE;

			S_StartSound(NULL, sfx_s3k4e);
			M_SetupReadyExplosions(p);
		}
		else
		{
			// changes
			p->mdepth = CSSTEP_CHARS;
			S_StartSound(NULL, sfx_s3k63);
		}

		M_SetMenuDelay(num);
	}
}

static boolean M_HandleCharacterGrid(setup_player_t *p, UINT8 num)
{
	UINT8 numclones;
	INT32 skin;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_ud > 0)
	{
		p->gridy++;
		if (p->gridy > 8)
			p->gridy = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_ud < 0)
	{
		p->gridy--;
		if (p->gridy < 0)
			p->gridy = 8;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}

	if (menucmd[num].dpad_lr > 0)
	{
		p->gridx++;
		if (p->gridx > 8)
			p->gridx = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->gridx--;
		if (p->gridx < 0)
			p->gridx = 8;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}

	// try to set the clone num to the page # if possible.
	p->clonenum = setup_page;

	// Process this after possible pad movement,
	// this makes sure we don't have a weird ghost hover on a character with no clones.
	numclones = setup_chargrid[p->gridx][p->gridy].numskins;

	if (p->clonenum >= numclones)
		p->clonenum = 0;

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		skin = setup_chargrid[p->gridx][p->gridy].skinlist[setup_page];
		if (setup_page >= setup_chargrid[p->gridx][p->gridy].numskins || skin == -1)
		{
			S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
		}
		else
		{
			if (setup_page+1 == setup_chargrid[p->gridx][p->gridy].numskins)
				p->mdepth = CSSTEP_COLORS; // Skip clones menu if there are none on this page.
			else
				p->mdepth = CSSTEP_ALTS;

			S_StartSound(NULL, sfx_s3k63);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		// for profiles / gameplay, exit out of the menu instantly,
		// we don't want to go to the input detection menu.
		if (optionsmenu.profile || gamestate != GS_MENU)
		{
			memset(setup_player, 0, sizeof(setup_player));	// Reset setup_player otherwise it does some VERY funky things.
			M_SetMenuDelay(0);
			M_GoBack(0);
			return true;
		}
		else	// in main menu
		{
			p->mdepth = CSSTEP_PROFILE;
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(num);
	}

	if (num == 0 && setup_numplayers == 1 && setup_maxpage)	// ONLY one player.
	{
		if (M_MenuButtonPressed(num, MBT_L))
		{
			if (setup_page == 0)
				setup_page = setup_maxpage;
			else
				setup_page--;

			S_StartSound(NULL, sfx_s3k63);
			M_SetMenuDelay(num);
		}
		else if (M_MenuButtonPressed(num, MBT_R))
		{
			if (setup_page == setup_maxpage)
				setup_page = 0;
			else
				setup_page++;

			S_StartSound(NULL, sfx_s3k63);
			M_SetMenuDelay(num);
		}
	}

	return false;
}

static void M_HandleCharRotate(setup_player_t *p, UINT8 num)
{
	UINT8 numclones = setup_chargrid[p->gridx][p->gridy].numskins;

	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->clonenum++;
		if (p->clonenum >= numclones)
			p->clonenum = 0;
		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->clonenum--;
		if (p->clonenum < 0)
			p->clonenum = numclones-1;
		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	 if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		p->mdepth = CSSTEP_COLORS;
		S_StartSound(NULL, sfx_s3k63);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_CHARS;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
}

static void M_HandleColorRotate(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->color = M_GetColorAfter(p->color, 1, false);
		p->rotate = CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->color = M_GetColorBefore(p->color, 1, false);
		p->rotate = -CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}

	 if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		p->mdepth = CSSTEP_FOLLOWER;
		M_GetFollowerState(p);
		S_StartSound(NULL, sfx_s3k63);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		if (setup_chargrid[p->gridx][p->gridy].numskins == 1)
		{
			p->mdepth = CSSTEP_CHARS; // Skip clones menu
		}
		else
		{
			p->mdepth = CSSTEP_ALTS;
		}
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
}

static void M_AnimateFollower(setup_player_t *p)
{
	if (--p->follower_tics <= 0)
	{

		// FF_ANIMATE; cycle through FRAMES and get back afterwards. This will be prominent amongst followers hence why it's being supported here.
		if (p->follower_state->frame & FF_ANIMATE)
		{
			p->follower_frame++;
			p->follower_tics = p->follower_state->var2;
			if (p->follower_frame > (p->follower_state->frame & FF_FRAMEMASK) + p->follower_state->var1)	// that's how it works, right?
				p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
		}
		else
		{
			if (p->follower_state->nextstate != S_NULL)
				p->follower_state = &states[p->follower_state->nextstate];
			p->follower_tics = p->follower_state->tics;
			/*if (p->follower_tics == -1)
				p->follower_tics = 15;	// er, what?*/
			// get spritedef:
			p->follower_frame = p->follower_state->frame & FF_FRAMEMASK;
		}
	}

	p->follower_timer++;
}

static void M_HandleFollowerRotate(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	if (menucmd[num].dpad_lr > 0)
	{
		p->followern++;
		if (p->followern >= numfollowers)
			p->followern = -1;

		M_GetFollowerState(p);

		p->rotate = CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->followern--;
		if (p->followern < -1)
			p->followern = numfollowers-1;

		p->rotate = -CSROTATETICS;
		p->delay = CSROTATETICS;
		S_StartSound(NULL, sfx_s3kc3s);
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		if (p->followern > -1)
		{
			p->mdepth = CSSTEP_FOLLOWERCOLORS;
			S_StartSound(NULL, sfx_s3k63);
		}
		else
		{
			p->mdepth = CSSTEP_READY;
			p->delay = TICRATE;
			M_SetupReadyExplosions(p);
			S_StartSound(NULL, sfx_s3k4e);
		}

		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_COLORS;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
}

static void M_HandleFollowerColorRotate(setup_player_t *p, UINT8 num)
{
	if (cv_splitdevice.value)
		num = 0;

	M_AnimateFollower(p);

	if (menucmd[num].dpad_lr > 0)
	{
		p->followercolor = M_GetColorAfter(p->followercolor, 1, true);
		p->rotate = CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}
	else if (menucmd[num].dpad_lr < 0)
	{
		p->followercolor = M_GetColorBefore(p->followercolor, 1, true);
		p->rotate = -CSROTATETICS;
		M_SetMenuDelay(num); //CSROTATETICS
		S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
	}

	if (M_MenuConfirmPressed(num) /*|| M_MenuButtonPressed(num, MBT_START)*/)
	{
		p->mdepth = CSSTEP_READY;
		p->delay = TICRATE;
		M_SetupReadyExplosions(p);
		S_StartSound(NULL, sfx_s3k4e);
		M_SetMenuDelay(num);
	}
	else if (M_MenuBackPressed(num))
	{
		p->mdepth = CSSTEP_FOLLOWER;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(num);
	}
}

boolean M_CharacterSelectHandler(INT32 choice)
{
	INT32 i;

	(void)choice;

	for (i = MAXSPLITSCREENPLAYERS-1; i >= 0; i--)
	{
		setup_player_t *p = &setup_player[i];
		boolean playersChanged = false;

		if (p->delay == 0 && menucmd[i].delay == 0)
		{
			if (!optionsmenu.profile)
			{
				// If splitdevice is true, only do the last non-ready setups.
				if (cv_splitdevice.value)
				{
					// Previous setup isn't ready, go there.
					// In any case, do setup 0 first.
					if (i > 0 && setup_player[i-1].mdepth < CSSTEP_READY)
						continue;
				}
			}

			switch (p->mdepth)
			{
				case CSSTEP_NONE: // Enter Game
					if (gamestate == GS_MENU)	// do NOT handle that outside of GS_MENU.
						playersChanged = M_HandlePressStart(p, i);
					break;
				case CSSTEP_PROFILE:
					playersChanged = M_HandleCSelectProfile(p, i);
					break;
				case CSSTEP_ASKCHANGES:
					M_HandleCharAskChange(p, i);
					break;
				case CSSTEP_CHARS: // Character Select grid
					M_HandleCharacterGrid(p, i);
					break;
				case CSSTEP_ALTS: // Select clone
					M_HandleCharRotate(p, i);
					break;
				case CSSTEP_COLORS: // Select color
					M_HandleColorRotate(p, i);
					break;
				case CSSTEP_FOLLOWER:
					M_HandleFollowerRotate(p, i);
					break;
				case CSSTEP_FOLLOWERCOLORS:
					M_HandleFollowerColorRotate(p, i);
					break;
				case CSSTEP_READY:
				default: // Unready
					if (M_MenuBackPressed(i))
					{
						p->mdepth = CSSTEP_COLORS;
						S_StartSound(NULL, sfx_s3k5b);
						M_SetMenuDelay(i);
					}
					break;
			}
		}

		// Just makes it easier to access later
		p->skin = setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum];

		// Keep profile colour.
		/*if (p->mdepth < CSSTEP_COLORS)
		{
			p->color = skins[p->skin].prefcolor;

		}*/

		if (playersChanged == true)
		{
			setup_page = 0;	// reset that.
			break;
		}
	}

	// Setup new numplayers
	setup_numplayers = 0;
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].mdepth == CSSTEP_NONE)
			break;

		setup_numplayers = i+1;
	}

	return true;
}

// Apply character skin and colour changes while ingame (we just call the skin / color commands.)
// ...Will this cause command buffer issues? -Lat'
static void M_MPConfirmCharacterSelection(void)
{
	UINT8 i;
	INT16 col;

	char commandnames[][MAXSTRINGLENGTH] = { "skin ", "skin2 ", "skin3 ", "skin4 "};
	// ^ laziness 100 (we append a space directly so that we don't have to do it later too!!!!)

	for (i = 0; i < splitscreen +1; i++)
	{
		char cmd[MAXSTRINGLENGTH];

		// colour
		// (convert the number that's saved to a string we can use)
		col = setup_player[i].color;
		CV_StealthSetValue(&cv_playercolor[i], col);

		// follower
		CV_StealthSetValue(&cv_follower[i], setup_player[i].followern);

		// follower color
		CV_StealthSetValue(&cv_followercolor[i], setup_player[i].followercolor);

		// finally, call the skin[x] console command.
		// This will call SendNameAndColor which will synch everything we sent here and apply the changes!

		// This is a hack to make sure we call Skin[x]_OnChange afterwards
		CV_StealthSetValue(&cv_skin[i], -1);

		strcpy(cmd, commandnames[i]);
		strcat(cmd, skins[setup_player[i].skin].name);
		COM_ImmedExecute(cmd);

	}
	M_ClearMenus(true);
}

void M_CharacterSelectTick(void)
{
	UINT8 i;
	boolean setupnext = true;

	setup_animcounter++;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].delay)
			setup_player[i].delay--;

		if (setup_player[i].rotate > 0)
			setup_player[i].rotate--;
		else if (setup_player[i].rotate < 0)
			setup_player[i].rotate++;

		if (i >= setup_numplayers)
			continue;

		if (setup_player[i].mdepth < CSSTEP_READY || setup_player[i].delay > 0)
		{
			// Someone's not ready yet.
			setupnext = false;
		}
	}

	for (i = 0; i < CSEXPLOSIONS; i++)
	{
		if (setup_explosions[i].tics > 0)
			setup_explosions[i].tics--;
	}

	if (setupnext && setup_numplayers > 0)
	{
		// Selecting from the menu
		if (gamestate == GS_MENU)
		{
			// in a profile; update the selected profile and then go back to the profile menu.
			if (optionsmenu.profile)
			{
				// save player
				strcpy(optionsmenu.profile->skinname, skins[setup_player[0].skin].name);
				optionsmenu.profile->color = setup_player[0].color;

				// save follower
				strcpy(optionsmenu.profile->follower, followers[setup_player[0].followern].skinname);
				optionsmenu.profile->followercolor = setup_player[0].followercolor;

				// reset setup_player
				memset(setup_player, 0, sizeof(setup_player));
				setup_numplayers = 0;

				M_GoBack(0);
				return;
			}
			else	// in a normal menu, stealthset the cvars and then go to the play menu.
			{
				for (i = 0; i < setup_numplayers; i++)
				{
					CV_StealthSet(&cv_skin[i], skins[setup_player[i].skin].name);
					CV_StealthSetValue(&cv_playercolor[i], setup_player[i].color);

					CV_StealthSetValue(&cv_follower[i], setup_player[i].followern);
					CV_StealthSetValue(&cv_followercolor[i], setup_player[i].followercolor);
				}

				CV_StealthSetValue(&cv_splitplayers, setup_numplayers);

				// P1 is alone, set their old device just in case.
				if (setup_numplayers < 2 && setup_player[0].ponedevice)
				{
					CV_StealthSetValue(&cv_usejoystick[0], setup_player[0].ponedevice);
				}

				M_SetupNextMenu(&PLAY_MainDef, false);
			}
		}
		else	// In a game
		{
			// 23/05/2022: Since there's already restrictskinchange, just allow this to happen regardless.
			M_MPConfirmCharacterSelection();
		}
	}
}

boolean M_CharacterSelectQuit(void)
{
	return true;
}

void M_SetupGametypeMenu(INT32 choice)
{
	(void)choice;

	PLAY_GamemodesDef.prevMenu = currentMenu;

	if (cv_splitplayers.value <= 1)
	{
		// Remove Battle, add Capsules
		PLAY_GamemodesMenu[1].status = IT_DISABLED;
		PLAY_GamemodesMenu[2].status = IT_STRING | IT_CALL;
	}
	else
	{
		// Add Battle, remove Capsules
		PLAY_GamemodesMenu[1].status = IT_STRING | IT_CALL;
		PLAY_GamemodesMenu[2].status = IT_DISABLED;
	}

	M_SetupNextMenu(&PLAY_GamemodesDef, false);
}

void M_SetupRaceMenu(INT32 choice)
{
	(void)choice;

	PLAY_RaceGamemodesDef.prevMenu = currentMenu;

	// Time Attack is 1P only
	if (cv_splitplayers.value <= 1)
	{
		PLAY_RaceGamemodesMenu[2].status = IT_STRING | IT_CALL;
	}
	else
	{
		PLAY_RaceGamemodesMenu[2].status = IT_DISABLED;
	}

	M_SetupNextMenu(&PLAY_RaceGamemodesDef, false);
}

// DIFFICULTY SELECT

void M_SetupDifficultySelect(INT32 choice)
{
	// check what we picked.
	choice = currentMenu->menuitems[itemOn].mvar1;

	// setup the difficulty menu and then remove choices depending on choice
	PLAY_RaceDifficultyDef.prevMenu = currentMenu;

	PLAY_RaceDifficulty[drace_gpdifficulty].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrkartspeed].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrcpu].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mrracers].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_encore].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_cupselect].status = IT_DISABLED;
	PLAY_RaceDifficulty[drace_mapselect].status = IT_DISABLED;

	if (choice)		// Match Race
	{
		PLAY_RaceDifficulty[drace_mrkartspeed].status = IT_STRING|IT_CVAR; // Kart Speed
		PLAY_RaceDifficulty[drace_mrcpu].status = IT_STRING2|IT_CVAR;	// CPUs on/off
		PLAY_RaceDifficulty[drace_mrracers].status = IT_STRING2|IT_CVAR;	// CPU amount
		PLAY_RaceDifficulty[drace_mapselect].status = IT_STRING|IT_CALL;	// Level Select (Match Race)
		PLAY_RaceDifficultyDef.lastOn = drace_mapselect;	// Select map select by default.
	}
	else			// GP
	{
		PLAY_RaceDifficulty[drace_gpdifficulty].status = IT_STRING|IT_CVAR; // Difficulty
		PLAY_RaceDifficulty[drace_cupselect].status = IT_STRING|IT_CALL;	// Level Select (GP)
		PLAY_RaceDifficultyDef.lastOn = drace_cupselect;	// Select cup select by default.
	}

	if (M_SecretUnlocked(SECRET_ENCORE))
	{
		PLAY_RaceDifficulty[drace_encore].status = IT_STRING2|IT_CVAR;	// Encore on/off
	}

	M_SetupNextMenu(&PLAY_RaceDifficultyDef, false);
}

// LEVEL SELECT

//
// M_CanShowLevelInList
//
// Determines whether to show a given map in the various level-select lists.
// Set gt = -1 to ignore gametype.
//
boolean M_CanShowLevelInList(INT16 mapnum, UINT8 gt)
{
	UINT32 tolflag = G_TOLFlag(gt);

	// Does the map exist?
	if (!mapheaderinfo[mapnum])
		return false;

	// Does the map have a name?
	if (!mapheaderinfo[mapnum]->lvlttl[0])
		return false;

	// Does the map have a LUMP?
	if (mapheaderinfo[mapnum]->lumpnum == LUMPERROR)
		return false;

	if (M_MapLocked(mapnum+1))
		return false; // not unlocked

	// Check for TOL
	if (!(mapheaderinfo[mapnum]->typeoflevel & tolflag))
		return false;

	// Should the map be hidden?
	if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
		return false;

	// I don't know why, but some may have exceptions.
	if (levellist.timeattack && (mapheaderinfo[mapnum]->menuflags & LF2_NOTIMEATTACK))
		return false;

	if (gametypedefaultrules[gt] & GTR_CAMPAIGN && levellist.selectedcup)
	{
		if (mapheaderinfo[mapnum]->cup != levellist.selectedcup)
			return false;
	}

	// Survived our checks.
	return true;
}

INT16 M_CountLevelsToShowInList(UINT8 gt)
{
	INT16 mapnum, count = 0;

	for (mapnum = 0; mapnum < nummapheaders; mapnum++)
		if (M_CanShowLevelInList(mapnum, gt))
			count++;

	return count;
}

INT16 M_GetFirstLevelInList(UINT8 gt)
{
	INT16 mapnum;

	for (mapnum = 0; mapnum < nummapheaders; mapnum++)
		if (M_CanShowLevelInList(mapnum, gt))
			return mapnum;

	return 0;
}

struct cupgrid_s cupgrid;
struct levellist_s levellist;

static void M_LevelSelectScrollDest(void)
{
	UINT16 m = M_CountLevelsToShowInList(levellist.newgametype)-1;

	levellist.dest = (6*levellist.cursor);

	if (levellist.dest < 3)
		levellist.dest = 3;

	if (levellist.dest > (6*m)-3)
		levellist.dest = (6*m)-3;
}

//  Builds the level list we'll be using from the gametype we're choosing and send us to the apropriate menu.
static void M_LevelListFromGametype(INT16 gt)
{
	levellist.newgametype = gt;
	PLAY_CupSelectDef.prevMenu = currentMenu;

	// Obviously go to Cup Select in gametypes that have cups.
	// Use a really long level select in gametypes that don't use cups.

	if (levellist.newgametype == GT_RACE)
	{
		cupheader_t *cup = kartcupheaders;
		UINT8 highestid = 0;

		// Make sure there's valid cups before going to this menu.
		if (cup == NULL)
			I_Error("Can you really call this a racing game, I didn't recieve any Cups on my pillow or anything");

		while (cup)
		{
			if (cup->unlockrequired == -1 || unlockables[cup->unlockrequired].unlocked)
				highestid = cup->id;
			cup = cup->next;
		}

		cupgrid.numpages = (highestid / (CUPMENU_COLUMNS * CUPMENU_ROWS)) + 1;

		PLAY_LevelSelectDef.prevMenu = &PLAY_CupSelectDef;
		M_SetupNextMenu(&PLAY_CupSelectDef, false);

		return;
	}

	// Reset position properly if you go back & forth between gametypes
	if (levellist.selectedcup)
	{
		levellist.cursor = 0;
		levellist.selectedcup = NULL;
	}

	M_LevelSelectScrollDest();
	levellist.y = levellist.dest;

	PLAY_LevelSelectDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PLAY_LevelSelectDef, false);

}

// Init level select for use in local play using the last choice we made.
// For the online MP version used to START HOSTING A GAME, see M_MPSetupNetgameMapSelect()
// (We still use this one midgame)

void M_LevelSelectInit(INT32 choice)
{
	(void)choice;

	levellist.netgame = false;	// Make sure this is reset as we'll only be using this function for offline games!
	cupgrid.netgame = false;	// Ditto

	switch (currentMenu->menuitems[itemOn].mvar1)
	{
		case 0:
			cupgrid.grandprix = false;
			levellist.timeattack = false;
			break;
		case 1:
			cupgrid.grandprix = false;
			levellist.timeattack = true;
			break;
		case 2:
			cupgrid.grandprix = true;
			levellist.timeattack = false;
			break;
		default:
			CONS_Alert(CONS_WARNING, "Bad level select init\n");
			return;
	}

	levellist.newgametype = currentMenu->menuitems[itemOn].mvar2;

	M_LevelListFromGametype(levellist.newgametype);
}

void M_CupSelectHandler(INT32 choice)
{
	cupheader_t *newcup = kartcupheaders;
	const UINT8 pid = 0;

	(void)choice;

	while (newcup)
	{
		if (newcup->id == CUPMENU_CURSORID)
			break;
		newcup = newcup->next;
	}

	if (menucmd[pid].dpad_lr > 0)
	{
		cupgrid.x++;
		if (cupgrid.x >= CUPMENU_COLUMNS)
		{
			cupgrid.x = 0;
			cupgrid.pageno++;
			if (cupgrid.pageno >= cupgrid.numpages)
				cupgrid.pageno = 0;
		}
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_lr < 0)
	{
		cupgrid.x--;
		if (cupgrid.x < 0)
		{
			cupgrid.x = CUPMENU_COLUMNS-1;
			cupgrid.pageno--;
			if (cupgrid.pageno < 0)
				cupgrid.pageno = cupgrid.numpages-1;
		}
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (menucmd[pid].dpad_ud > 0)
	{
		cupgrid.y++;
		if (cupgrid.y >= CUPMENU_ROWS)
			cupgrid.y = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		cupgrid.y--;
		if (cupgrid.y < 0)
			cupgrid.y = CUPMENU_ROWS-1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	if (M_MenuConfirmPressed(pid) /*|| M_MenuButtonPressed(pid, MBT_START)*/)
	{
		M_SetMenuDelay(pid);

		if ((!newcup)
			|| (newcup && newcup->unlockrequired != -1 && !unlockables[newcup->unlockrequired].unlocked)
			|| (newcup->cachedlevels[0] == NEXTMAP_INVALID))
		{
			S_StartSound(NULL, sfx_s3kb2);
			return;
		}

		if (cupgrid.grandprix == true)
		{
			INT32 levelNum;
			UINT8 ssplayers = cv_splitplayers.value-1;

			S_StartSound(NULL, sfx_s3k63);

			// Early fadeout to let the sound finish playing
			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();
			F_RunWipe(wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

			memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));

			if (cv_maxconnections.value < ssplayers+1)
				CV_SetValue(&cv_maxconnections, ssplayers+1);

			if (splitscreen != ssplayers)
			{
				splitscreen = ssplayers;
				SplitScreen_OnChange();
			}

			// read our dummy cvars

			grandprixinfo.gamespeed = min(KARTSPEED_HARD, cv_dummygpdifficulty.value);
			grandprixinfo.masterbots = (cv_dummygpdifficulty.value == 3);
			grandprixinfo.encore = (boolean)cv_dummygpencore.value;

			grandprixinfo.cup = newcup;

			grandprixinfo.gp = true;
			grandprixinfo.roundnum = 1;
			grandprixinfo.initalize = true;

			paused = false;

			// Don't restart the server if we're already in a game lol
			if (gamestate == GS_MENU)
			{
				SV_StartSinglePlayerServer();
				multiplayer = true; // yeah, SV_StartSinglePlayerServer clobbers this...
				netgame = levellist.netgame;	// ^ ditto.
			}

			levelNum = grandprixinfo.cup->cachedlevels[0];

			D_MapChange(
				levelNum + 1,
				GT_RACE,
				grandprixinfo.encore,
				true,
				1,
				false,
				false
			);

			M_ClearMenus(true);
		}
		else
		{
			// Keep cursor position if you select the same cup again, reset if it's a different cup
			if (!levellist.selectedcup || newcup->id != levellist.selectedcup->id)
			{
				levellist.cursor = 0;
				levellist.selectedcup = newcup;
			}

			M_LevelSelectScrollDest();
			levellist.y = levellist.dest;

			M_SetupNextMenu(&PLAY_LevelSelectDef, false);
			S_StartSound(NULL, sfx_s3k63);
		}
	}
	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

void M_CupSelectTick(void)
{
	cupgrid.previewanim++;
}

void M_LevelSelectHandler(INT32 choice)
{
	INT16 start = M_GetFirstLevelInList(levellist.newgametype);
	INT16 maxlevels = M_CountLevelsToShowInList(levellist.newgametype);
	const UINT8 pid = 0;

	(void)choice;

	if (levellist.y != levellist.dest)
	{
		return;
	}

	if (menucmd[pid].dpad_ud > 0)
	{
		levellist.cursor++;
		if (levellist.cursor >= maxlevels)
			levellist.cursor = 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		levellist.cursor--;
		if (levellist.cursor < 0)
			levellist.cursor = maxlevels-1;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	M_LevelSelectScrollDest();

	if (M_MenuConfirmPressed(pid) /*|| M_MenuButtonPressed(pid, MBT_START)*/)
	{
		INT16 map = start;
		INT16 add = levellist.cursor;

		M_SetMenuDelay(pid);

		while (add > 0)
		{
			map++;

			while (!M_CanShowLevelInList(map, levellist.newgametype) && map < nummapheaders)
				map++;

			if (map >= nummapheaders)
				break;

			add--;
		}

		if (map >= nummapheaders)
		{
			// This shouldn't happen
			return;
		}

		levellist.choosemap = map;

		if (levellist.timeattack)
		{
			M_SetupNextMenu(&PLAY_TimeAttackDef, false);
			S_StartSound(NULL, sfx_s3k63);
		}
		else
		{
			if (gamestate == GS_MENU)
			{
				UINT8 ssplayers = cv_splitplayers.value-1;

				netgame = false;
				multiplayer = true;

				strncpy(connectedservername, cv_servername.string, MAXSERVERNAME);

				// Still need to reset devmode
				cht_debug = 0;

				if (demo.playback)
					G_StopDemo();
				if (metalrecording)
					G_StopMetalDemo();

				/*if (levellist.choosemap == 0)
					levellist.choosemap = G_RandMap(G_TOLFlag(levellist.newgametype), -1, 0, 0, false, NULL);*/

				if (cv_maxconnections.value < ssplayers+1)
					CV_SetValue(&cv_maxconnections, ssplayers+1);

				if (splitscreen != ssplayers)
				{
					splitscreen = ssplayers;
					SplitScreen_OnChange();
				}

				S_StartSound(NULL, sfx_s3k63);

				paused = false;

				// Early fadeout to let the sound finish playing
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

				SV_StartSinglePlayerServer();
				multiplayer = true; // yeah, SV_StartSinglePlayerServer clobbers this...
				netgame = levellist.netgame;	// ^ ditto.

				CV_StealthSet(&cv_kartbot, cv_dummymatchbots.string);
				CV_StealthSet(&cv_kartencore, (cv_dummygpencore.value == 1) ? "On" : "Auto");
				CV_StealthSet(&cv_kartspeed, (cv_dummykartspeed.value == KARTSPEED_NORMAL) ? "Auto" : cv_dummykartspeed.string);

				D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_kartencore.value == 1), 1, 1, false, false);
			}
			else
			{
				// directly do the map change
				D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_kartencore.value == 1), 1, 1, false, false);
			}

			M_ClearMenus(true);
		}
	}
	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

void M_LevelSelectTick(void)
{

	INT16 dist = levellist.dest - levellist.y;

	if (abs(dist) == 1)	// cheating to avoid off by 1 errors with divisions.
		levellist.y = levellist.dest;
	else
		levellist.y += dist/2;
}


// time attack stuff...
void M_HandleStaffReplay(INT32 choice)
{
	// @TODO:
	(void) choice;
}

void M_ReplayTimeAttack(INT32 choice)
{
	// @TODO:
	(void) choice;
}

void M_SetGuestReplay(INT32 choice)
{
	// @TODO:
	(void) choice;
}

void M_StartTimeAttack(INT32 choice)
{
	char *gpath;
	char nameofdemo[256];

	(void)choice;

	switch (levellist.newgametype)
	{
		case GT_BATTLE:
			modeattacking = ATTACKING_CAPSULES;
			break;
		default:
			modeattacking = ATTACKING_TIME;
			break;
	}

	// Still need to reset devmode
	cht_debug = 0;
	emeralds = 0;

	if (demo.playback)
		G_StopDemo();
	if (metalrecording)
		G_StopMetalDemo();

	splitscreen = 0;
	SplitScreen_OnChange();

	S_StartSound(NULL, sfx_s3k63);

	paused = false;

	// Early fadeout to let the sound finish playing
	F_WipeStartScreen();
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	F_WipeEndScreen();
	F_RunWipe(wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

	SV_StartSinglePlayerServer();

	gpath = va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s",
			srb2home, timeattackfolder);
	M_MkdirEach(gpath, M_PathParts(gpath) - 3, 0755);

	strcat(gpath, PATHSEP);
	strcat(gpath, G_BuildMapName(levellist.choosemap+1));

	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-last", gpath, cv_skin[0].string);

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	M_ClearMenus(true);
	D_MapChange(levellist.choosemap+1, levellist.newgametype, (cv_dummygpencore.value == 1), 1, 1, false, false);
}


struct mpmenu_s mpmenu;

// MULTIPLAYER OPTION SELECT

// Use this as a quit routine within the HOST GAME and JOIN BY IP "sub" menus
boolean M_MPResetOpts(void)
{
	UINT8 i = 0;

	for (; i < 3; i++)
		mpmenu.modewinextend[i][0] = 0;	// Undo this

	return true;
}

void M_MPOptSelectInit(INT32 choice)
{
	INT16 arrcpy[3][3] = {{0,68,0}, {0,12,0}, {0,74,0}};
	UINT8 i = 0, j = 0;	// To copy the array into the struct

	(void)choice;

	mpmenu.modechoice = 0;
	mpmenu.ticker = 0;

	for (; i < 3; i++)
		for (j = 0; j < 3; j++)
			mpmenu.modewinextend[i][j] = arrcpy[i][j];	// I miss Lua already

	M_SetupNextMenu(&PLAY_MP_OptSelectDef, false);
}

void M_MPOptSelectTick(void)
{
	UINT8 i = 0;

	// 3 Because we have 3 options in the menu
	for (; i < 3; i++)
	{
		if (mpmenu.modewinextend[i][0])
			mpmenu.modewinextend[i][2] += 8;
		else
			mpmenu.modewinextend[i][2] -= 8;

		mpmenu.modewinextend[i][2] = min(mpmenu.modewinextend[i][1], max(0, mpmenu.modewinextend[i][2]));
		//CONS_Printf("%d - %d,%d,%d\n", i, mpmenu.modewinextend[i][0], mpmenu.modewinextend[i][1], mpmenu.modewinextend[i][2]);
	}
}


// MULTIPLAYER HOST
void M_MPHostInit(INT32 choice)
{

	(void)choice;
	mpmenu.modewinextend[0][0] = 1;
	M_SetupNextMenu(&PLAY_MP_HostDef, true);
	itemOn = mhost_go;
}

void M_MPSetupNetgameMapSelect(INT32 choice)
{

	INT16 gt = GT_RACE;
	(void)choice;

	levellist.netgame = true;		// Yep, we'll be starting a netgame.
	cupgrid.netgame = true;			// Ditto
	levellist.timeattack = false;	// Make sure we reset those
	cupgrid.grandprix = false;	// Ditto

	// In case we ever want to add new gamemodes there somehow, have at it!
	switch (cv_dummygametype.value)
	{
		case 1:	// Battle
		{
			gt = GT_BATTLE;
			break;
		}

		default:
		{
			gt = GT_RACE;
			break;
		}
	}

	// okay this is REALLY stupid but this fixes the host menu re-folding on itself when we go back.
	mpmenu.modewinextend[0][0] = 1;

	M_LevelListFromGametype(gt); // Setup the level select.
	// (This will also automatically send us to the apropriate menu)
}

// MULTIPLAYER JOIN BY IP
void M_MPJoinIPInit(INT32 choice)
{

	(void)choice;
	mpmenu.modewinextend[2][0] = 1;
	M_SetupNextMenu(&PLAY_MP_JoinIPDef, true);
}

// Attempts to join a given IP from the menu.
void M_JoinIP(const char *ipa)
{
	if (*(ipa) == '\0')	// Jack shit
	{
		M_StartMessage("Please specify an address.\n", NULL, MM_NOTHING);
		return;
	}

	COM_BufAddText(va("connect \"%s\"\n", ipa));
	M_ClearMenus(true);

	// A little "please wait" message.
	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Connecting to server...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
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

// MULTIPLAYER ROOM SELECT MENU

void M_MPRoomSelect(INT32 choice)
{
	const UINT8 pid = 0;
	(void) choice;

	if (menucmd[pid].dpad_lr)
	{
		mpmenu.room = (!mpmenu.room) ? 1 : 0;
		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}
	else if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{
		M_ServersMenu(0);
		M_SetMenuDelay(pid);
	}
}

void M_MPRoomSelectTick(void)
{
	mpmenu.ticker++;
}

void M_MPRoomSelectInit(INT32 choice)
{
	(void)choice;
	mpmenu.room = 0;
	mpmenu.ticker = 0;
	mpmenu.servernum = 0;
	mpmenu.scrolln = 0;
	mpmenu.slide = 0;

	M_SetupNextMenu(&PLAY_MP_RoomSelectDef, false);
}

// MULTIPLAYER ROOM FETCH / REFRESH THREADS

// depending on mpmenu.room, either allows only unmodded servers or modded ones. Remove others from the list.
// we do this by iterating backwards.
static void M_CleanServerList(void)
{
	UINT8 i = serverlistcount;

	while (i)
	{

		if (serverlist[i].info.modifiedgame != mpmenu.room)
		{
			// move everything after this index 1 slot down...
			if (i != serverlistcount)
				memcpy(&serverlist[i], &serverlist[i+1], sizeof(serverelem_t)*(serverlistcount-i));

			serverlistcount--;
		}

		i--;
	}
}

void
M_SetWaitingMode (int mode)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&k_menu_mutex);
#endif
	{
		m_waiting_mode = mode;
	}
#ifdef HAVE_THREADS
	I_unlock_mutex(k_menu_mutex);
#endif
}

int
M_GetWaitingMode (void)
{
	int mode;

#ifdef HAVE_THREADS
	I_lock_mutex(&k_menu_mutex);
#endif
	{
		mode = m_waiting_mode;
	}
#ifdef HAVE_THREADS
	I_unlock_mutex(k_menu_mutex);
#endif

	return mode;
}

#ifdef MASTERSERVER
#ifdef HAVE_THREADS
void
Spawn_masterserver_thread (const char *name, void (*thread)(int*))
{
	int *id = malloc(sizeof *id);

	I_lock_mutex(&ms_QueryId_mutex);
	{
		*id = ms_QueryId;
	}
	I_unlock_mutex(ms_QueryId_mutex);

	I_spawn_thread(name, (I_thread_fn)thread, id);
}

int
Same_instance (int id)
{
	int okay;

	I_lock_mutex(&ms_QueryId_mutex);
	{
		okay = ( id == ms_QueryId );
	}
	I_unlock_mutex(ms_QueryId_mutex);

	return okay;
}
#endif/*HAVE_THREADS*/

void
Fetch_servers_thread (int *id)
{
	msg_server_t * server_list;

	(void)id;

	M_SetWaitingMode(M_WAITING_SERVERS);

#ifdef HAVE_THREADS
	server_list = GetShortServersList(*id);
#else
	server_list = GetShortServersList(0);
#endif

	if (server_list)
	{
#ifdef HAVE_THREADS
		if (Same_instance(*id))
#endif
		{
			M_SetWaitingMode(M_NOT_WAITING);

#ifdef HAVE_THREADS
			I_lock_mutex(&ms_ServerList_mutex);
			{
				ms_ServerList = server_list;
			}
			I_unlock_mutex(ms_ServerList_mutex);
#else
			CL_QueryServerList(server_list);
			free(server_list);
#endif
		}
#ifdef HAVE_THREADS
		else
		{
			free(server_list);
		}
#endif
	}

#ifdef HAVE_THREADS
	free(id);
#endif
}
#endif/*MASTERSERVER*/

// updates serverlist
void M_RefreshServers(INT32 choice)
{
	(void)choice;

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Searching for servers...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

#ifdef MASTERSERVER
#ifdef HAVE_THREADS
	Spawn_masterserver_thread("fetch-servers", Fetch_servers_thread);
#else/*HAVE_THREADS*/
	Fetch_servers_thread(NULL);
#endif/*HAVE_THREADS*/
#else/*MASTERSERVER*/
	CL_UpdateServerList();
#endif/*MASTERSERVER*/

#ifdef SERVERLISTDEBUG
	M_ServerListFillDebug();
#endif
	M_CleanServerList();
	M_SortServerList();

}

#ifdef UPDATE_ALERT
static void M_CheckMODVersion(int id)
{
	char updatestring[500];
	const char *updatecheck = GetMODVersion(id);
	if(updatecheck)
	{
		sprintf(updatestring, UPDATE_ALERT_STRING, VERSIONSTRING, updatecheck);
#ifdef HAVE_THREADS
		I_lock_mutex(&k_menu_mutex);
#endif
		M_StartMessage(updatestring, NULL, MM_NOTHING);
#ifdef HAVE_THREADS
		I_unlock_mutex(k_menu_mutex);
#endif
	}
}
#endif/*UPDATE_ALERT*/

#if defined (UPDATE_ALERT) && defined (HAVE_THREADS)
static void
Check_new_version_thread (int *id)
{
	M_SetWaitingMode(M_WAITING_VERSION);

	M_CheckMODVersion(*id);

	if (Same_instance(*id))
	{
		Fetch_servers_thread(id);
	}
	else
	{
		free(id);
	}
}
#endif/*defined (UPDATE_ALERT) && defined (HAVE_THREADS)*/


// Initializes serverlist when entering the menu...
void M_ServersMenu(INT32 choice)
{
	(void)choice;
	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	mpmenu.servernum = 0;
	mpmenu.scrolln = 0;
	mpmenu.slide = 0;

	M_SetupNextMenu(&PLAY_MP_ServerBrowserDef, false);
	itemOn = 0;

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	I_lock_mutex(&ms_QueryId_mutex);
	{
		ms_QueryId++;
	}
	I_unlock_mutex(ms_QueryId_mutex);

	I_lock_mutex(&ms_ServerList_mutex);
	{
		if (ms_ServerList)
		{
			free(ms_ServerList);
			ms_ServerList = NULL;
		}
	}
	I_unlock_mutex(ms_ServerList_mutex);

#ifdef UPDATE_ALERT
	Spawn_masterserver_thread("check-new-version", Check_new_version_thread);
#else/*UPDATE_ALERT*/
	Spawn_masterserver_thread("fetch-servers", Fetch_servers_thread);
#endif/*UPDATE_ALERT*/
#else/*defined (MASTERSERVER) && defined (HAVE_THREADS)*/
#ifdef UPDATE_ALERT
	M_CheckMODVersion(0);
#endif/*UPDATE_ALERT*/
	M_RefreshServers(0);
#endif/*defined (MASTERSERVER) && defined (HAVE_THREADS)*/

#ifdef SERVERLISTDEBUG
	M_ServerListFillDebug();
#endif

	M_CleanServerList();
	M_SortServerList();
}

#ifdef SERVERLISTDEBUG

// Fill serverlist with a bunch of garbage to make our life easier in debugging
void M_ServerListFillDebug(void)
{
	UINT8 i = 0;

	serverlistcount = 10;
	memset(serverlist, 0, sizeof(serverlist));	// zero out the array for convenience...

	for (i = 0; i < serverlistcount; i++)
	{
		// We don't really care about the server node for this, let's just fill in the info so that we have a visual...
		serverlist[i].info.numberofplayer = min(i, 8);
		serverlist[i].info.maxplayer = 8;

		serverlist[i].info.avgpwrlv = P_RandomRange(PR_UNDEFINED, 500, 1500);
		serverlist[i].info.time = P_RandomRange(PR_UNDEFINED, 1, 8);	// ping

		strcpy(serverlist[i].info.servername, va("Serv %d", i+1));

		strcpy(serverlist[i].info.gametypename, i & 1 ? "Race" : "Battle");

		P_RandomRange(PR_UNDEFINED, 0, 5);	// change results...
		serverlist[i].info.kartvars = P_RandomRange(PR_UNDEFINED, 0, 3) & SV_SPEEDMASK;

		serverlist[i].info.modifiedgame = P_RandomRange(PR_UNDEFINED, 0, 1);

		CONS_Printf("Serv %d | %d...\n", i, serverlist[i].info.modifiedgame);
	}
}

#endif // SERVERLISTDEBUG

// Ascending order, not descending.
// The casts are safe as long as the caller doesn't do anything stupid.
#define SERVER_LIST_ENTRY_COMPARATOR(key) \
static int ServerListEntryComparator_##key(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sa->info.key != sb->info.key) \
		return sa->info.key - sb->info.key; \
	return strcmp(sa->info.servername, sb->info.servername); \
}

// This does descending instead of ascending.
#define SERVER_LIST_ENTRY_COMPARATOR_REVERSE(key) \
static int ServerListEntryComparator_##key##_reverse(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sb->info.key != sa->info.key) \
		return sb->info.key - sa->info.key; \
	return strcmp(sb->info.servername, sa->info.servername); \
}

SERVER_LIST_ENTRY_COMPARATOR(time)
SERVER_LIST_ENTRY_COMPARATOR(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(maxplayer)
SERVER_LIST_ENTRY_COMPARATOR(avgpwrlv)


static int ServerListEntryComparator_gametypename(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;
	int c;
	if (( c = strcasecmp(sa->info.gametypename, sb->info.gametypename) ))
		return c;
	return strcmp(sa->info.servername, sb->info.servername); \
}

void M_SortServerList(void)
{
	switch(cv_serversort.value)
	{
	case 0:		// Ping.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_time);
		break;
	case 1:		// AVG. Power Level
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_avgpwrlv);
		break;
	case 2:		// Most players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer_reverse);
		break;
	case 3:		// Least players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer);
		break;
	case 4:		// Max players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_maxplayer_reverse);
		break;
	case 5:		// Gametype.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametypename);
		break;
	}
}


// Server browser inputs & ticker
void M_MPServerBrowserTick(void)
{
	mpmenu.slide /= 2;
}

// Input handler for server browser.
boolean M_ServerBrowserInputs(INT32 ch)
{
	UINT8 pid = 0;
	UINT8 maxscroll = serverlistcount-(SERVERSPERPAGE/2);
	(void) ch;

	if (!itemOn && menucmd[pid].dpad_ud < 0)
	{
		M_PrevOpt();	// go to itemOn 2
		if (serverlistcount)
		{
			UINT8 prevscroll = mpmenu.scrolln;

			mpmenu.servernum = serverlistcount;
			mpmenu.scrolln = maxscroll;
			mpmenu.slide = SERVERSPACE * (prevscroll - mpmenu.scrolln);
		}
		else
		{
			itemOn = 1;	// Sike! If there are no servers, go to refresh instead.
		}

		return true;	// overwrite behaviour.
	}
	else if (itemOn == 2)	// server browser itself...
	{
		// we have to manually do that here.
		if (M_MenuBackPressed(pid))
		{
			M_GoBack(0);
			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_ud > 0)	// down
		{
			if (mpmenu.servernum >= serverlistcount-1)
			{
				UINT8 prevscroll = mpmenu.scrolln;
				mpmenu.servernum = 0;
				mpmenu.scrolln = 0;
				mpmenu.slide = SERVERSPACE * (prevscroll - mpmenu.scrolln);

				M_NextOpt();	// Go back to the top of the real menu.
			}
			else
			{
				mpmenu.servernum++;
				if (mpmenu.scrolln < maxscroll && mpmenu.servernum > SERVERSPERPAGE/2)
				{
					mpmenu.scrolln++;
					mpmenu.slide += SERVERSPACE;
				}
			}
			S_StartSound(NULL, sfx_s3k5b);
			M_SetMenuDelay(pid);

		}
		else if (menucmd[pid].dpad_ud < 0)
		{

			if (!mpmenu.servernum)
			{
				M_PrevOpt();
			}
			else
			{
				if (mpmenu.servernum <= serverlistcount-(SERVERSPERPAGE/2) && mpmenu.scrolln)
				{
					mpmenu.scrolln--;
					mpmenu.slide -= SERVERSPACE;
				}

				mpmenu.servernum--;
			}
			S_StartSound(NULL, sfx_s3k5b);
			M_SetMenuDelay(pid);

		}
		return true;	// Overwrite behaviour.
	}
	return false;	// use normal behaviour.
}


// Options menu:
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
		|| ((server || IsPlayerAdmin(consoleplayer)) && K_CanChangeRules()))
	{
		OPTIONS_MainDef.menuitems[mopt_gameplay].status = IT_STRING | IT_SUBMENU;
		OPTIONS_MainDef.menuitems[mopt_server].status = IT_STRING | IT_SUBMENU;
		OPTIONS_GameplayDef.menuitems[gopt_encore].status =
			(M_SecretUnlocked(SECRET_ENCORE) ? (IT_STRING | IT_CVAR) : IT_DISABLED);
	}

	OPTIONS_DataDef.menuitems[dopt_erase].status = (gamestate == GS_MENU
		? (IT_STRING | IT_SUBMENU)
		: (IT_TRANSTEXT2 | IT_SPACE));

	M_ResetOptions();

	// So that pause doesn't go to the main menu...
	OPTIONS_MainDef.prevMenu = currentMenu;

	// This will disable or enable the textboxes of the affected menus before we get to them.
	Screenshot_option_Onchange();
	Moviemode_mode_Onchange();
	Moviemode_option_Onchange();
	Addons_option_Onchange();

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
	optionsmenu.offset /= 2;
	optionsmenu.ticker++;

	optionsmenu.optx += (optionsmenu.toptx - optionsmenu.optx)/2;
	optionsmenu.opty += (optionsmenu.topty - optionsmenu.opty)/2;

	if (abs(optionsmenu.optx - optionsmenu.opty) < 2)
	{
		optionsmenu.optx = optionsmenu.toptx;
		optionsmenu.opty = optionsmenu.topty;	// Avoid awkward 1 px errors.
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
	if (optionsmenu.currcolour != currentMenu->extra1)
		M_OptionsChangeBGColour(currentMenu->extra1);

	// And one last giggle...
	if (shitsfree)
		shitsfree--;
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

void M_ProfileSelectInit(INT32 choice)
{
	(void)choice;
	optionsmenu.profilemenu = true;

	M_SetupNextMenu(&OPTIONS_ProfilesDef, false);
}

// setup video mode menu
void M_VideoModeMenu(INT32 choice)
{
	INT32 i, j, vdup, nummodes;
	UINT32 width, height;
	const char *desc;

	(void)choice;

	memset(optionsmenu.modedescs, 0, sizeof(optionsmenu.modedescs));

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	VID_PrepareModeList(); // FIXME: hack
#endif
	optionsmenu.vidm_nummodes = 0;
	optionsmenu.vidm_selected = 0;
	nummodes = VID_NumModes();

	// DOS does not skip mode 0, because mode 0 is ALWAYS present
	i = 0;
	for (; i < nummodes && optionsmenu.vidm_nummodes < MAXMODEDESCS; i++)
	{
		desc = VID_GetModeName(i);
		if (desc)
		{
			vdup = 0;

			// when a resolution exists both under VGA and VESA, keep the
			// VESA mode, which is always a higher modenum
			for (j = 0; j < optionsmenu.vidm_nummodes; j++)
			{
				if (!strcmp(optionsmenu.modedescs[j].desc, desc))
				{
					// mode(0): 320x200 is always standard VGA, not vesa
					if (optionsmenu.modedescs[j].modenum)
					{
						optionsmenu.modedescs[j].modenum = i;
						vdup = 1;

						if (i == vid.modenum)
							optionsmenu.vidm_selected = j;
					}
					else
						vdup = 1;

					break;
				}
			}

			if (!vdup)
			{
				optionsmenu.modedescs[optionsmenu.vidm_nummodes].modenum = i;
				optionsmenu.modedescs[optionsmenu.vidm_nummodes].desc = desc;

				if (i == vid.modenum)
					optionsmenu.vidm_selected = optionsmenu.vidm_nummodes;

				// Pull out the width and height
				sscanf(desc, "%u%*c%u", &width, &height);

				// Show multiples of 320x200 as green.
				if (SCR_IsAspectCorrect(width, height))
					optionsmenu.modedescs[optionsmenu.vidm_nummodes].goodratio = 1;

				optionsmenu.vidm_nummodes++;
			}
		}
	}

	optionsmenu.vidm_column_size = (optionsmenu.vidm_nummodes+2) / 3;

	M_SetupNextMenu(&OPTIONS_VideoModesDef, false);
}

// Select the current profile for menu use and go to maindef.
static void M_FirstPickProfile(INT32 c)
{
	if (c == MA_YES)
	{
		M_ResetOptions();			// Reset all options variables otherwise things are gonna go reaaal bad lol.
		optionsmenu.profile = NULL;	// Make sure to get rid of that, too.

		PR_ApplyProfile(optionsmenu.profilen, 0);
		M_SetupNextMenu(&MainDef, false);

		// Tell the game this is the last profile we picked.
		CV_StealthSetValue(&cv_ttlprofilen, optionsmenu.profilen);

		// Save em!
		PR_SaveProfiles();
		return;
	}
}

// Start menu edition. Call this with MA_YES if not used with a textbox.
static void M_StartEditProfile(INT32 c)
{

	const INT32 maxp = PR_GetNumProfiles();

	if (c == MA_YES)
	{
		if (optionsmenu.profilen == maxp)
			PR_InitNewProfile();	// initialize the new profile.

		optionsmenu.profile = PR_GetProfile(optionsmenu.profilen);
		// copy this profile's controls into optionsmenu so that we can edit controls without changing them directly.
		// we do this so that we don't edit a profile's controls in real-time and end up doing really weird shit.
		memcpy(&optionsmenu.tempcontrols, optionsmenu.profile->controls, sizeof(gamecontroldefault));

		// This is now used to move the card we've selected.
		optionsmenu.optx = 160;
		optionsmenu.opty = 35;
		optionsmenu.toptx = 130/2;
		optionsmenu.topty = 0;

		// setup cvars
		if (optionsmenu.profile->version)
		{
			CV_StealthSet(&cv_dummyprofilename, optionsmenu.profile->profilename);
			CV_StealthSet(&cv_dummyprofileplayername, optionsmenu.profile->playername);
			CV_StealthSetValue(&cv_dummyprofilekickstart, optionsmenu.profile->kickstartaccel);
		}
		else
		{
			CV_StealthSet(&cv_dummyprofilename, "");
			CV_StealthSet(&cv_dummyprofileplayername, "");
			CV_StealthSetValue(&cv_dummyprofilekickstart, 0);	// off
		}

		// Setup greyout and stuff.
		OPTIONS_EditProfile[popt_profilename].status = IT_STRING | IT_CVAR | IT_CV_STRING;
		OPTIONS_EditProfile[popt_profilepname].status = IT_STRING | IT_CVAR | IT_CV_STRING;
		OPTIONS_EditProfile[popt_char].status = IT_STRING | IT_CALL;

		if (gamestate != GS_MENU)	// If we're modifying things mid game, transtext some of those!
		{
			OPTIONS_EditProfile[popt_profilename].status |= IT_TRANSTEXT;
			OPTIONS_EditProfile[popt_profilepname].status |= IT_TRANSTEXT;
			OPTIONS_EditProfile[popt_char].status |= IT_TRANSTEXT;
		}

		M_SetupNextMenu(&OPTIONS_EditProfileDef, false);
		return;
	}
}

void M_HandleProfileSelect(INT32 ch)
{
	const UINT8 pid = 0;
	INT32 maxp = PR_GetNumProfiles();
	boolean creatable = (maxp < MAXPROFILES);
	(void) ch;

	if (!creatable)
	{
		maxp = MAXPROFILES;
	}

	if (menucmd[pid].dpad_lr > 0)
	{
		optionsmenu.profilen++;
		optionsmenu.offset += (128 + 128/8);

		if (optionsmenu.profilen > maxp)
		{
			optionsmenu.profilen = 0;
			optionsmenu.offset -= (128 + 128/8)*(maxp+1);
		}

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);

	}
	else if (menucmd[pid].dpad_lr < 0)
	{
		optionsmenu.profilen--;
		optionsmenu.offset -= (128 + 128/8);

		if (optionsmenu.profilen < 0)
		{
			optionsmenu.profilen = maxp;
			optionsmenu.offset += (128 + 128/8)*(maxp+1);
		}

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{

		// Boot profile setup has already been done.
		if (cv_currprofile.value > -1)
		{

			if (optionsmenu.profilen == 0)	// Guest profile, you can't edit that one!
			{
				S_StartSound(NULL, sfx_s3k7b);
				M_StartMessage(M_GetText("The Guest profile cannot be edited.\nCreate a new profile instead."), NULL, MM_NOTHING);
				M_SetMenuDelay(pid);
				return;
			}
			else if (creatable && optionsmenu.profilen == maxp && gamestate != GS_MENU)
			{
				S_StartSound(NULL, sfx_s3k7b);
				M_StartMessage(M_GetText("Cannot create a new profile\nmid-game. Return to the\ntitle screen first."), NULL, MM_NOTHING);
				M_SetMenuDelay(pid);
				return;
			}

			S_StartSound(NULL, sfx_s3k5b);
			M_StartEditProfile(MA_YES);
		}
		else
		{
			// We're on the profile selection screen.
			if (creatable && optionsmenu.profilen == maxp)
			{
				M_StartEditProfile(MA_YES);
				M_SetMenuDelay(pid);
				return;
			}
			else
			{
#if 0
				if (optionsmenu.profilen == 0)
				{
					M_StartMessage(M_GetText("Are you sure you wish\nto use the Guest Profile?\nThis profile cannot be customised.\nIt is recommended to create\na new Profile instead.\n\n(Press A to confirm)"), FUNCPTRCAST(M_FirstPickProfile), MM_YESNO);
					return;
				}
#endif

				M_FirstPickProfile(MA_YES);
				M_SetMenuDelay(pid);
				return;
			}
		}
	}

	else if (M_MenuBackPressed(pid))
	{
		optionsmenu.resetprofilemenu = true;
		M_GoBack(0);
		M_SetMenuDelay(pid);
	}

	if (menutransition.tics == 0 && optionsmenu.resetprofile)
	{
		optionsmenu.profile = NULL;	// Make sure to reset that when transitions are done.'
		optionsmenu.resetprofile = false;
	}
}

// Returns true if the profile can be saved, false otherwise. Also starts messages if necessary.
static boolean M_ProfileEditEnd(const UINT8 pid)
{
	UINT8 i;

	// Guest profile, you can't edit that one!
	if (optionsmenu.profilen == 0)
	{
		S_StartSound(NULL, sfx_s3k7b);
		M_StartMessage(M_GetText("Guest profile cannot be edited.\nCreate a new profile instead."), NULL, MM_NOTHING);
		M_SetMenuDelay(pid);
		return false;
	}

	// check if some profiles have the same name
	for (i = 0; i < PR_GetNumProfiles(); i++)
	{
		profile_t *check = PR_GetProfile(i);

		// For obvious reasons don't check if our name is the same as our name....
		if (check != optionsmenu.profile)
		{
			if (!(strcmp(optionsmenu.profile->profilename, check->profilename)))
			{
				S_StartSound(NULL, sfx_s3k7b);
				M_StartMessage(M_GetText("Another profile uses the same name.\nThis must be changed to be able to save."), NULL, MM_NOTHING);
				M_SetMenuDelay(pid);
				return false;
			}
		}
	}

	return true;
}

static void M_ProfileEditExit(void)
{
	optionsmenu.toptx = 160;
	optionsmenu.topty = 35;
	optionsmenu.resetprofile = true;	// Reset profile after the transition is done.

	PR_SaveProfiles();					// save profiles after we do that.
}

// For profile edit, just make sure going back resets the card to its position, the rest is taken care of automatically.
boolean M_ProfileEditInputs(INT32 ch)
{

	(void) ch;
	const UINT8 pid = 0;

	if (M_MenuBackPressed(pid))
	{
		if (M_ProfileEditEnd(pid))
		{
			M_ProfileEditExit();
			if (cv_currprofile.value == -1)
				M_SetupNextMenu(&MAIN_ProfilesDef, false);
			else
				M_GoBack(0);
			M_SetMenuDelay(pid);
		}
		return true;
	}
	else if (M_MenuConfirmPressed(pid))
	{
		if (currentMenu->menuitems[itemOn].status & IT_TRANSTEXT)
			return true;	// No.
	}

	return false;
}

// Handle some actions in profile editing
void M_HandleProfileEdit(void)
{
	// Always copy the profile name and player name in the profile.
	if (optionsmenu.profile)
	{
		// Copy the first 6 chars for profile name
		if (strlen(cv_dummyprofilename.string))
		{
			char *s;
			// convert dummyprofilename to uppercase
			strncpy(optionsmenu.profile->profilename, cv_dummyprofilename.string, PROFILENAMELEN);
			s = optionsmenu.profile->profilename;
			while (*s)
			{
				*s = toupper(*s);
				s++;
			}
		}

		if (strlen(cv_dummyprofileplayername.string))
			strncpy(optionsmenu.profile->playername, cv_dummyprofileplayername.string, MAXPLAYERNAME);
	}

	M_OptionsTick();	//  Has to be afterwards because this can unset optionsmenu.profile
}

// Confirm Profile edi via button.
void M_ConfirmProfile(INT32 choice)
{
	const UINT8 pid = 0;
	(void) choice;

	if (M_ProfileEditEnd(pid))
	{
		if (cv_currprofile.value > -1)
		{
			M_ProfileEditExit();
			M_GoBack(0);
			M_SetMenuDelay(pid);
		}
		else
		{
			M_StartMessage(M_GetText("Are you sure you wish to\nselect this profile?\n\n(Press A to confirm)"), FUNCPTRCAST(M_FirstPickProfile), MM_YESNO);
			M_SetMenuDelay(pid);
		}
	}
	return;
}

// special menuitem key handler for video mode list
void M_HandleVideoModes(INT32 ch)
{

	const UINT8 pid = 0;
	(void)ch;

	if (optionsmenu.vidm_testingmode > 0)
	{
		// change back to the previous mode quickly
		if (M_MenuBackPressed(pid))
		{
			setmodeneeded = optionsmenu.vidm_previousmode + 1;
			optionsmenu.vidm_testingmode = 0;
		}
		else if (M_MenuConfirmPressed(pid))
		{
			S_StartSound(NULL, sfx_s3k5b);
			optionsmenu.vidm_testingmode = 0; // stop testing
		}
	}

	else
	{
		if (menucmd[pid].dpad_ud > 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			if (++optionsmenu.vidm_selected >= optionsmenu.vidm_nummodes)
				optionsmenu.vidm_selected = 0;

			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_ud < 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			if (--optionsmenu.vidm_selected < 0)
				optionsmenu.vidm_selected = optionsmenu.vidm_nummodes - 1;

			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_lr < 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			optionsmenu.vidm_selected -= optionsmenu.vidm_column_size;
			if (optionsmenu.vidm_selected < 0)
				optionsmenu.vidm_selected = (optionsmenu.vidm_column_size*3) + optionsmenu.vidm_selected;
			if (optionsmenu.vidm_selected >= optionsmenu.vidm_nummodes)
				optionsmenu.vidm_selected = optionsmenu.vidm_nummodes - 1;

			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_lr > 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			optionsmenu.vidm_selected += optionsmenu.vidm_column_size;
			if (optionsmenu.vidm_selected >= (optionsmenu.vidm_column_size*3))
				optionsmenu.vidm_selected %= optionsmenu.vidm_column_size;
			if (optionsmenu.vidm_selected >= optionsmenu.vidm_nummodes)
				optionsmenu.vidm_selected = optionsmenu.vidm_nummodes - 1;

			M_SetMenuDelay(pid);
		}

		else if (M_MenuConfirmPressed(pid))
		{
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
			if (vid.modenum == optionsmenu.modedescs[optionsmenu.vidm_selected].modenum)
				SCR_SetDefaultMode();
			else
			{
				optionsmenu.vidm_testingmode = 15*TICRATE;
				optionsmenu.vidm_previousmode = vid.modenum;
				if (!setmodeneeded) // in case the previous setmode was not finished
					setmodeneeded = optionsmenu.modedescs[optionsmenu.vidm_selected].modenum + 1;
			}
		}

		else if (M_MenuBackPressed(pid))
		{
			M_SetMenuDelay(pid);
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu, false);
			else
				M_ClearMenus(true);
		}
	}
}

// sets whatever device has had its key pressed to the active device.
// 20/05/22: Commented out for now but not deleted as it might still find some use in the future?
/*
static void SetDeviceOnPress(void)
{
	UINT8 i;

	for (i=0; i < MAXDEVICES; i++)
	{
		if (deviceResponding[i])
		{
			CV_SetValue(&cv_usejoystick[0], i);	// Force-set this joystick as the current joystick we're using for P1 (which is the only one controlling menus)
			CONS_Printf("SetDeviceOnPress: Device for %d set to %d\n", 0, i);
			return;
		}
	}
}
*/


// Prompt a device selection window (just tap any button on the device you want)
void M_ProfileDeviceSelect(INT32 choice)
{
	(void)choice;

	// While we're here, setup the incoming controls menu to reset the scroll & bind status:
	optionsmenu.controlscroll = 0;
	optionsmenu.bindcontrol = 0;
	optionsmenu.bindtimer = 0;

	optionsmenu.lastkey = 0;
	optionsmenu.keyheldfor = 0;

	optionsmenu.contx = optionsmenu.tcontx = controlleroffsets[gc_a][0];
	optionsmenu.conty = optionsmenu.tconty = controlleroffsets[gc_a][1];

	M_SetupNextMenu(&OPTIONS_ProfileControlsDef, false);	// Don't set device here anymore.
}

void M_HandleProfileControls(void)
{
	UINT8 maxscroll = currentMenu->numitems - 5;
	M_OptionsTick();

	optionsmenu.contx += (optionsmenu.tcontx - optionsmenu.contx)/2;
	optionsmenu.conty += (optionsmenu.tconty - optionsmenu.conty)/2;

	if (abs(optionsmenu.contx - optionsmenu.tcontx) < 2 && abs(optionsmenu.conty - optionsmenu.tconty) < 2)
	{
		optionsmenu.contx = optionsmenu.tcontx;
		optionsmenu.conty = optionsmenu.tconty;	// Avoid awkward 1 px errors.
	}

	optionsmenu.controlscroll = itemOn - 3;	// very barebones scrolling, but it works just fine for our purpose.
	if (optionsmenu.controlscroll > maxscroll)
		optionsmenu.controlscroll = maxscroll;

	if (optionsmenu.controlscroll < 0)
		optionsmenu.controlscroll = 0;

	// bindings, cancel if timer is depleted.
	if (optionsmenu.bindcontrol)
	{
		optionsmenu.bindtimer--;
		if (!optionsmenu.bindtimer)
		{
			optionsmenu.bindcontrol = 0;		// we've gone past the max, just stop.
		}

	}
}

void M_ProfileTryController(INT32 choice)
{
	(void)choice;

	optionsmenu.trycontroller = TICRATE*5;

	// Apply these controls right now on P1's end.
	memcpy(&gamecontrol[0], optionsmenu.tempcontrols, sizeof(gamecontroldefault));
}

static void M_ProfileControlSaveResponse(INT32 choice)
{
	if (choice == MA_YES)
	{
		SINT8 belongsto = PR_ProfileUsedBy(optionsmenu.profile);
		// Save the profile
		optionsmenu.profile->kickstartaccel = cv_dummyprofilekickstart.value;
		memcpy(&optionsmenu.profile->controls, optionsmenu.tempcontrols, sizeof(gamecontroldefault));

		// If this profile is in-use by anyone, apply the changes immediately upon exiting.
		// Don't apply the profile itself as that would lead to issues mid-game.
		if (belongsto > -1 && belongsto < MAXSPLITSCREENPLAYERS)
		{
			memcpy(&gamecontrol[belongsto], optionsmenu.tempcontrols, sizeof(gamecontroldefault));
			CV_StealthSetValue(&cv_kickstartaccel[belongsto], cv_dummyprofilekickstart.value);
		}

		M_GoBack(0);
	}
}

void M_ProfileControlsConfirm(INT32 choice)
{
	(void)choice;

	//M_StartMessage(M_GetText("Exiting will save the control changes\nfor this Profile.\nIs this okay?\n\n(Press A to confirm)"), FUNCPTRCAST(M_ProfileControlSaveResponse), MM_YESNO);
	// TODO: Add a graphic for controls saving, instead of obnoxious prompt.

	M_ProfileControlSaveResponse(MA_YES);

	optionsmenu.profile->kickstartaccel = cv_dummyprofilekickstart.value;		// Make sure to save kickstart accel.

	// Reapply player 1's real profile.
	if (cv_currprofile.value > -1)
	{
		PR_ApplyProfile(cv_lastprofile[0].value, 0);
	}
}

boolean M_ProfileControlsInputs(INT32 ch)
{
	const UINT8 pid = 0;
	(void)ch;

	// By default, accept all inputs.
	if (optionsmenu.trycontroller)
	{
		if (menucmd[pid].dpad_ud || menucmd[pid].dpad_lr || menucmd[pid].buttons)
		{
			optionsmenu.trycontroller = 5*TICRATE;
		}
		else
		{
			optionsmenu.trycontroller--;
		}

		if (optionsmenu.trycontroller == 0)
		{
			// Reset controls to that of the current profile.
			profile_t *cpr = PR_GetProfile(cv_currprofile.value);
			if (cpr == NULL)
				cpr = PR_GetProfile(0); // Creating a profile at boot, revert to guest profile
			memcpy(&gamecontrol[0], cpr->controls, sizeof(gamecontroldefault));
		}

		return true;
	}

	if (optionsmenu.bindcontrol)
		return true;	// Eat all inputs there. We'll use a stupid hack in M_Responder instead.

	//SetDeviceOnPress();	// Update device constantly so that we don't stay stuck with otpions saying a device is unavailable just because we're mapping multiple devices...

	if (M_MenuExtraPressed(pid))
	{
		// check if we're on a valid menu option...
		if (currentMenu->menuitems[itemOn].mvar1)
		{
			// clear controls for that key
			INT32 i;

			for (i = 0; i < MAXINPUTMAPPING; i++)
				optionsmenu.tempcontrols[currentMenu->menuitems[itemOn].mvar1][i] = KEY_NULL;

			S_StartSound(NULL, sfx_s3k66);
		}
		M_SetMenuDelay(pid);
		return true;
	}
	else if (M_MenuBackPressed(pid))
	{
		M_ProfileControlsConfirm(0);
		M_SetMenuDelay(pid);
		return true;
	}

	return false;
}

void M_ProfileSetControl(INT32 ch)
{
	INT32 controln = currentMenu->menuitems[itemOn].mvar1;
	UINT8 i;
	(void) ch;

	optionsmenu.bindcontrol = 1;	// Default to control #1

	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		if (optionsmenu.tempcontrols[controln][i] == KEY_NULL)
		{
			optionsmenu.bindcontrol = i+1;
			break;
		}
	}

	// If we could find a null key to map into, map there.
	// Otherwise, this will stay at 1 which means we'll overwrite the first bound control.

	optionsmenu.bindtimer = TICRATE*5;
}

// Map the event to the profile.

#define KEYHOLDFOR 1
void M_MapProfileControl(event_t *ev)
{
	INT32 c = 0;
	UINT8 n = optionsmenu.bindcontrol-1;					// # of input to bind
	INT32 controln = currentMenu->menuitems[itemOn].mvar1;	// gc_
	UINT8 where = n;										// By default, we'll save the bind where we're supposed to map.
	INT32 i;

	//SetDeviceOnPress();	// Update cv_usejoystick

	// Only consider keydown and joystick events to make sure we ignore ev_mouse and other events
	// See also G_MapEventsToControls
	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 < NUMINPUTS)
			{
				c = ev->data1;
			}
#ifdef PARANOIA
			else
			{
				CONS_Debug(DBG_GAMELOGIC, "Bad downkey input %d\n", ev->data1);
			}
#endif
			break;
		case ev_joystick:
			if (ev->data1 >= JOYAXES)
			{
#ifdef PARANOIA
				CONS_Debug(DBG_GAMELOGIC, "Bad joystick axis event %d\n", ev->data1);
#endif
				return;
			}
			else
			{
				INT32 deadzone = deadzone = (JOYAXISRANGE * cv_deadzone[0].value) / FRACUNIT; // TODO how properly account for different deadzone cvars for different devices
				boolean responsivelr = ((ev->data2 != INT32_MAX) && (abs(ev->data2) >= deadzone));
				boolean responsiveud = ((ev->data3 != INT32_MAX) && (abs(ev->data3) >= deadzone));

				i = ev->data1;

				if (i >= JOYANALOGS)
				{
					// The trigger axes are handled specially.
					i -= JOYANALOGS;

					if (responsivelr)
					{
						c = KEY_AXIS1 + (JOYANALOGS * 4) + (i * 2);
					}
					else if (responsiveud)
					{
						c = KEY_AXIS1 + (JOYANALOGS * 4) + (i * 2) + 1;
					}
				}
				else
				{
					// Actual analog sticks

					// Only consider unambiguous assignment.
					if (responsivelr == responsiveud)
						return;

					if (responsivelr)
					{
						if (ev->data2 < 0)
						{
							// Left
							c = KEY_AXIS1 + (i * 4);
						}
						else
						{
							// Right
							c = KEY_AXIS1 + (i * 4) + 1;
						}
					}
					else //if (responsiveud)
					{
						if (ev->data3 < 0)
						{
							// Up
							c = KEY_AXIS1 + (i * 4) + 2;
						}
						else
						{
							// Down
							c = KEY_AXIS1 + (i * 4) + 3;
						}
					}
				}
			}
			break;
		default:
			return;
	}

	// safety result
	if (!c)
		return;

	// Set menu delay regardless of what we're doing to avoid stupid stuff.
	M_SetMenuDelay(0);

	// Check if this particular key (c) is already bound in any slot.
	// If that's the case, simply do nothing.
	for (i = 0; i < MAXINPUTMAPPING; i++)
	{
		if (optionsmenu.tempcontrols[controln][i] == c)
		{
			optionsmenu.bindcontrol = 0;
			return;
		}
	}

	// With the way we do things, there cannot be instances of 'gaps' within the controls, so we don't need to pretend like we need to handle that.
	// Unless of course you tamper with the cfg file, but then it's *your* fault, not mine.

	optionsmenu.tempcontrols[controln][where] = c;
	optionsmenu.bindcontrol = 0;	// not binding anymore

	// If possible, reapply the profile...
	// 19/05/22: Actually, no, don't do that, it just fucks everything up in too many cases.

	/*
	if (gamestate == GS_MENU)	// In menu? Apply this to P1, no questions asked.
	{
		// Apply the profile's properties to player 1 but keep the last profile cv to p1's ACTUAL profile to revert once we exit.
		UINT8 lastp = cv_lastprofile[0].value;
		PR_ApplyProfile(PR_GetProfileNum(optionsmenu.profile), 0);
		CV_StealthSetValue(&cv_lastprofile[0], lastp);
	}
	else	// != GS_MENU
	{
		// ONLY apply the profile if it's in use by anything currently.
		UINT8 pnum = PR_GetProfileNum(optionsmenu.profile);
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			if (cv_lastprofile[i].value == pnum)
			{
				PR_ApplyProfile(pnum, i);
				break;
			}
		}
	}
	*/
}
#undef KEYHOLDFOR

void M_HandleItemToggles(INT32 choice)
{
	const INT32 width = 8, height = 4;
	INT32 column = itemOn/height, row = itemOn%height;
	INT16 next;
	UINT8 i;
	boolean exitmenu = false;
	const UINT8 pid = 0;

	(void) choice;

	if (menucmd[pid].dpad_lr > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		column++;
		if (((column*height)+row) >= currentMenu->numitems)
			column = 0;
		next = min(((column*height)+row), currentMenu->numitems-1);
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_lr < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		column--;
		if (column < 0)
			column = width-1;
		if (((column*height)+row) >= currentMenu->numitems)
			column--;
		next = max(((column*height)+row), 0);
		if (next >= currentMenu->numitems)
			next = currentMenu->numitems-1;
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		row = (row+1) % height;
		if (((column*height)+row) >= currentMenu->numitems)
			row = 0;
		next = min(((column*height)+row), currentMenu->numitems-1);
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (menucmd[pid].dpad_ud < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		row = (row-1) % height;
		if (row < 0)
			row = height-1;
		if (((column*height)+row) >= currentMenu->numitems)
			row--;
		next = max(((column*height)+row), 0);
		if (next >= currentMenu->numitems)
			next = currentMenu->numitems-1;
		itemOn = next;

		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{
		M_SetMenuDelay(pid);
		if (currentMenu->menuitems[itemOn].mvar1 == 255)
		{
			//S_StartSound(NULL, sfx_s26d);
			if (!shitsfree)
			{
				shitsfree = TICRATE;
				S_StartSound(NULL, sfx_itfree);
			}
		}
		else
		if (currentMenu->menuitems[itemOn].mvar1 == 0)
		{
			INT32 v = cv_sneaker.value;
			S_StartSound(NULL, sfx_s1b4);
			for (i = 0; i < NUMKARTRESULTS-1; i++)
			{
				if (KartItemCVars[i]->value == v)
					CV_AddValue(KartItemCVars[i], 1);
			}
		}
		else
		{
			if (currentMenu->menuitems[itemOn].mvar2)
			{
				S_StartSound(NULL, currentMenu->menuitems[itemOn].mvar2);
			}
			else
			{
				S_StartSound(NULL, sfx_s1ba);
			}
			CV_AddValue(KartItemCVars[currentMenu->menuitems[itemOn].mvar1-1], 1);
		}
	}

	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);
		exitmenu = true;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
}

// Check if we have any profile loaded.
void M_CheckProfileData(INT32 choice)
{
	UINT8 np = PR_GetNumProfiles();
	(void) choice;

	if (np < 2)
	{
		S_StartSound(NULL, sfx_s3k7b);
		M_StartMessage("There are no custom profiles.\n\n(Press any button)", NULL, MM_NOTHING);
		return;
	}

	optionsmenu.eraseprofilen = 1;
	M_SetupNextMenu(&OPTIONS_DataProfileEraseDef, false);
}

static void M_EraseProfileResponse(INT32 choice)
{
	if (choice == MA_YES)
	{
		S_StartSound(NULL, sfx_itrole); // bweh heh heh

		PR_DeleteProfile(optionsmenu.eraseprofilen);

		// Did we bust our current profile..!?
		if (cv_currprofile.value == -1)
		{
			F_StartIntro();
			M_ClearMenus(true);
		}
		else if (optionsmenu.eraseprofilen > PR_GetNumProfiles()-1)
		{
			optionsmenu.eraseprofilen--;
		}
	}
}

void M_HandleProfileErase(INT32 choice)
{
	const UINT8 pid = 0;
	const UINT8 np = PR_GetNumProfiles()-1;
	(void) choice;

	if (menucmd[pid].dpad_ud > 0)
	{
		S_StartSound(NULL, sfx_s3k5b);
		optionsmenu.eraseprofilen++;

		if (optionsmenu.eraseprofilen > np)
			optionsmenu.eraseprofilen = 1;

		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		S_StartSound(NULL, sfx_s3k5b);

		if (optionsmenu.eraseprofilen == 1)
			optionsmenu.eraseprofilen = np;
		else
			optionsmenu.eraseprofilen--;

		M_SetMenuDelay(pid);
	}
	else if (M_MenuBackPressed(pid))
	{
		M_GoBack(0);
		M_SetMenuDelay(pid);
	}
	else if (M_MenuConfirmPressed(pid))
	{
		if (optionsmenu.eraseprofilen == cv_currprofile.value)
			M_StartMessage("Your ""\x85""current profile""\x80"" will be erased.\nAre you sure you want to proceed?\nDeleting this profile will also\nreturn you to the title screen.\n\n(Press A to confirm)", FUNCPTRCAST(M_EraseProfileResponse), MM_YESNO);
		else
			M_StartMessage("This profile will be erased.\nAre you sure you want to proceed?\n\n(Press A to confirm)", FUNCPTRCAST(M_EraseProfileResponse), MM_YESNO);

		M_SetMenuDelay(pid);
	}
}

// Extras menu;
// this is copypasted from the options menu but all of these are different functions in case we ever want it to look more unique

struct extrasmenu_s extrasmenu;

void M_InitExtras(INT32 choice)
{
	(void)choice;

	extrasmenu.ticker = 0;
	extrasmenu.offset = 0;

	extrasmenu.extx = 0;
	extrasmenu.exty = 0;
	extrasmenu.textx = 0;
	extrasmenu.texty = 0;

	M_SetupNextMenu(&EXTRAS_MainDef, false);
}

// For statistics, will maybe remain unused for a while
boolean M_ExtrasQuit(void)
{
	extrasmenu.textx = 140-1;
	extrasmenu.texty = 70+1;

	return true;	// Always allow quitting, duh.
}

void M_ExtrasTick(void)
{
	extrasmenu.offset /= 2;
	extrasmenu.ticker++;

	extrasmenu.extx += (extrasmenu.textx - extrasmenu.extx)/2;
	extrasmenu.exty += (extrasmenu.texty - extrasmenu.exty)/2;

	if (abs(extrasmenu.extx - extrasmenu.exty) < 2)
	{
		extrasmenu.extx = extrasmenu.textx;
		extrasmenu.exty = extrasmenu.texty;	// Avoid awkward 1 px errors.
	}

	// Move the button for cool animations
	if (currentMenu == &EXTRAS_MainDef)
	{
		M_ExtrasQuit();	// reset the options button.
	}
	else
	{
		extrasmenu.textx = 160;
		extrasmenu.texty = 50;
	}
}

boolean M_ExtrasInputs(INT32 ch)
{

	const UINT8 pid = 0;
	(void) ch;

	if (menucmd[pid].dpad_ud > 0)
	{
		extrasmenu.offset += 48;
		M_NextOpt();
		S_StartSound(NULL, sfx_s3k5b);

		if (itemOn == 0)
			extrasmenu.offset -= currentMenu->numitems*48;

		M_SetMenuDelay(pid);
		return true;
	}

	else if (menucmd[pid].dpad_ud < 0)
	{
		extrasmenu.offset -= 48;
		M_PrevOpt();
		S_StartSound(NULL, sfx_s3k5b);

		if (itemOn == currentMenu->numitems-1)
			extrasmenu.offset += currentMenu->numitems*48;

		M_SetMenuDelay(pid);
		return true;
	}

	else if (M_MenuConfirmPressed(pid))
	{

		if (currentMenu->menuitems[itemOn].status & IT_TRANSTEXT)
			return true;	// No.

		extrasmenu.extx = 140;
		extrasmenu.exty = 70;	// Default position for the currently selected option.

		M_SetMenuDelay(pid);
		return false;	// Don't eat.
	}
	return false;
}

// =====================
// PAUSE / IN-GAME MENUS
// =====================
void M_EndModeAttackRun(void)
{
	G_CheckDemoStatus(); // Cancel recording

	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
		Command_ExitGame_f();

	M_StartControlPanel();

	currentMenu = &PLAY_TimeAttackDef;
	itemOn = currentMenu->lastOn;

	G_SetGamestate(GS_MENU);
	S_ChangeMusicInternal("menu", true);

	modeattacking = ATTACKING_NONE;
}

struct pausemenu_s pausemenu;

// Pause menu!
void M_OpenPauseMenu(void)
{
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
	PAUSE_Main[mpause_switchmap].status = IT_DISABLED;
#ifdef HAVE_DISCORDRPC
	PAUSE_Main[mpause_discordrequests].status = IT_DISABLED;
#endif

	PAUSE_Main[mpause_spectate].status = IT_DISABLED;
	PAUSE_Main[mpause_entergame].status = IT_DISABLED;
	PAUSE_Main[mpause_canceljoin].status = IT_DISABLED;
	PAUSE_Main[mpause_spectatemenu].status = IT_DISABLED;
	PAUSE_Main[mpause_psetup].status = IT_DISABLED;

	Dummymenuplayer_OnChange();	// Make sure the consvar is within bounds of the amount of splitscreen players we have.

	if (K_CanChangeRules())
	{
		PAUSE_Main[mpause_psetup].status = IT_STRING | IT_CALL;

		if (server || IsPlayerAdmin(consoleplayer))
		{
			PAUSE_Main[mpause_switchmap].status = IT_STRING | IT_SUBMENU;
			PAUSE_Main[mpause_addons].status = IT_STRING | IT_CALL;
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

	if (pausemenu.closing)
	{
		pausemenu.openoffset *= 2;
		if (pausemenu.openoffset > 255)
			M_ClearMenus(true);

	}
	else
		pausemenu.openoffset /= 2;
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
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\nPress a key.\n"), NULL, MM_NOTHING);
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

	M_StartMessage(M_GetText("Are you sure you want to return\nto the title screen?\n(Press A to confirm)\n"), FUNCPTRCAST(M_ExitGameResponse), MM_YESNO);
}


// Replay Playback Menu
void M_SetPlaybackMenuPointer(void)
{
	itemOn = playback_pause;
}

void M_PlaybackRewind(INT32 choice)
{
	static tic_t lastconfirmtime;

	(void)choice;

	if (!demo.rewinding)
	{
		if (paused)
		{
			G_ConfirmRewind(leveltime-1);
			paused = true;
			S_PauseAudio();
		}
		else
			demo.rewinding = paused = true;
	}
	else if (lastconfirmtime + TICRATE/2 < I_GetTime())
	{
		lastconfirmtime = I_GetTime();
		G_ConfirmRewind(leveltime);
	}

	CV_SetValue(&cv_playbackspeed, 1);
}

void M_PlaybackPause(INT32 choice)
{
	(void)choice;

	paused = !paused;

	if (demo.rewinding)
	{
		G_ConfirmRewind(leveltime);
		paused = true;
		S_PauseAudio();
	}
	else if (paused)
		S_PauseAudio();
	else
		S_ResumeAudio();

	CV_SetValue(&cv_playbackspeed, 1);
}

void M_PlaybackFastForward(INT32 choice)
{
	(void)choice;

	if (demo.rewinding)
	{
		G_ConfirmRewind(leveltime);
		paused = false;
		S_ResumeAudio();
	}
	CV_SetValue(&cv_playbackspeed, cv_playbackspeed.value == 1 ? 4 : 1);
}

void M_PlaybackAdvance(INT32 choice)
{
	(void)choice;

	paused = false;
	TryRunTics(1);
	paused = true;
}

void M_PlaybackSetViews(INT32 choice)
{
	if (choice > 0)
	{
		if (splitscreen < 3)
			G_AdjustView(splitscreen + 2, 0, true);
	}
	else if (splitscreen)
	{
		splitscreen--;
		R_ExecuteSetViewSize();
	}
}

void M_PlaybackAdjustView(INT32 choice)
{
	G_AdjustView(itemOn - playback_viewcount, (choice > 0) ? 1 : -1, true);
}

// this one's rather tricky
void M_PlaybackToggleFreecam(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);

	// remove splitscreen:
	splitscreen = 0;
	R_ExecuteSetViewSize();

	P_InitCameraCmd();	// init camera controls
	if (!demo.freecam)	// toggle on
	{
		demo.freecam = true;
		democam.cam = &camera[0];	// this is rather useful
	}
	else	// toggle off
	{
		demo.freecam = false;
		// reset democam vars:
		democam.cam = NULL;
		//democam.turnheld = false;
		democam.keyboardlook = false;	// reset only these. localangle / aiming gets set before the cam does anything anyway
	}
}

void M_PlaybackQuit(INT32 choice)
{
	(void)choice;
	G_StopDemo();

	if (demo.inreplayhut)
		M_ReplayHut(choice);
	else if (modeattacking)
		M_EndModeAttackRun();
	else
		D_StartTitle();
}

void M_PrepReplayList(void)
{
	size_t i;

	if (extrasmenu.demolist)
		Z_Free(extrasmenu.demolist);

	extrasmenu.demolist = Z_Calloc(sizeof(menudemo_t) * sizedirmenu, PU_STATIC, NULL);

	for (i = 0; i < sizedirmenu; i++)
	{
		if (dirmenu[i][DIR_TYPE] == EXT_UP)
		{
			extrasmenu.demolist[i].type = MD_SUBDIR;
			sprintf(extrasmenu.demolist[i].title, "UP");
		}
		else if (dirmenu[i][DIR_TYPE] == EXT_FOLDER)
		{
			extrasmenu.demolist[i].type = MD_SUBDIR;
			strncpy(extrasmenu.demolist[i].title, dirmenu[i] + DIR_STRING, 64);
		}
		else
		{
			extrasmenu.demolist[i].type = MD_NOTLOADED;
			snprintf(extrasmenu.demolist[i].filepath, 255, "%s%s", menupath, dirmenu[i] + DIR_STRING);
			sprintf(extrasmenu.demolist[i].title, ".....");
		}
	}
}


void M_ReplayHut(INT32 choice)
{
	(void)choice;

	extrasmenu.replayScrollTitle = 0;
	extrasmenu.replayScrollDelay = TICRATE;
	extrasmenu.replayScrollDir = 1;

	if (!demo.inreplayhut)
	{
		snprintf(menupath, 1024, "%s"PATHSEP"media"PATHSEP"replay"PATHSEP"online"PATHSEP, srb2home);
		menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath);
	}
	if (!preparefilemenu(false, true))
	{
		M_StartMessage("No replays found.\n\n(Press a key)\n", NULL, MM_NOTHING);
		return;
	}
	else if (!demo.inreplayhut)
		dir_on[menudepthleft] = 0;
	demo.inreplayhut = true;

	extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;

	M_PrepReplayList();

	menuactive = true;
	M_SetupNextMenu(&EXTRAS_ReplayHutDef, false);
	//G_SetGamestate(GS_TIMEATTACK);
	//titlemapinaction = TITLEMAP_OFF; // Nope don't give us HOMs please

	demo.rewinding = false;
	CL_ClearRewinds();

	//S_ChangeMusicInternal("replst", true);
}

// key handler
void M_HandleReplayHutList(INT32 choice)
{

	const UINT8 pid = 0;
	(void) choice;

	if (menucmd[pid].dpad_ud < 0)
	{
		if (dir_on[menudepthleft])
			dir_on[menudepthleft]--;
		else
			return;
			//M_PrevOpt();

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;
	}

	else if (menucmd[pid].dpad_ud > 0)
	{
		if (dir_on[menudepthleft] < sizedirmenu-1)
			dir_on[menudepthleft]++;
		else
			return;
			//itemOn = 0; // Not M_NextOpt because that would take us to the extra dummy item

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
		extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;
	}

	else if (M_MenuBackPressed(pid))
	{
		M_SetMenuDelay(pid);
		M_QuitReplayHut();
	}

	else if (M_MenuConfirmPressed(pid))
	{
		M_SetMenuDelay(pid);
		switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
		{
			case EXT_FOLDER:
				strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
				if (menudepthleft)
				{
					menupathindex[--menudepthleft] = strlen(menupath);
					menupath[menupathindex[menudepthleft]] = 0;

					if (!preparefilemenu(false, true))
					{
						S_StartSound(NULL, sfx_s224);
						M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
						menupath[menupathindex[++menudepthleft]] = 0;

						if (!preparefilemenu(true, true))
						{
							M_QuitReplayHut();
							return;
						}
					}
					else
					{
						S_StartSound(NULL, sfx_s3k5b);
						dir_on[menudepthleft] = 1;
						M_PrepReplayList();
					}
				}
				else
				{
					S_StartSound(NULL, sfx_s26d);
					M_StartMessage(va("%c%s\x80\nThis folder is too deep to navigate to!\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
					menupath[menupathindex[menudepthleft]] = 0;
				}
				break;
			case EXT_UP:
				S_StartSound(NULL, sfx_s3k5b);
				menupath[menupathindex[++menudepthleft]] = 0;
				if (!preparefilemenu(false, true))
				{
					M_QuitReplayHut();
					return;
				}
				M_PrepReplayList();
				break;
			default:
				// We can't just use M_SetupNextMenu because that'll run ReplayDef's quitroutine and boot us back to the title screen!
				currentMenu->lastOn = itemOn;
				currentMenu = &EXTRAS_ReplayStartDef;

				extrasmenu.replayScrollTitle = 0; extrasmenu.replayScrollDelay = TICRATE; extrasmenu.replayScrollDir = 1;

				switch (extrasmenu.demolist[dir_on[menudepthleft]].addonstatus)
				{
				case DFILE_ERROR_CANNOTLOAD:
					// Only show "Watch Replay Without Addons"
					EXTRAS_ReplayStart[0].status = IT_DISABLED;
					EXTRAS_ReplayStart[1].status = IT_CALL|IT_STRING;
					//EXTRAS_ReplayStart[1].alphaKey = 0;
					EXTRAS_ReplayStart[2].status = IT_DISABLED;
					itemOn = 1;
					break;

				case DFILE_ERROR_NOTLOADED:
				case DFILE_ERROR_INCOMPLETEOUTOFORDER:
					// Show "Load Addons and Watch Replay" and "Watch Replay Without Addons"
					EXTRAS_ReplayStart[0].status = IT_CALL|IT_STRING;
					EXTRAS_ReplayStart[1].status = IT_CALL|IT_STRING;
					//EXTRAS_ReplayStart[1].alphaKey = 10;
					EXTRAS_ReplayStart[2].status = IT_DISABLED;
					itemOn = 0;
					break;

				case DFILE_ERROR_EXTRAFILES:
				case DFILE_ERROR_OUTOFORDER:
				default:
					// Show "Watch Replay"
					EXTRAS_ReplayStart[0].status = IT_DISABLED;
					EXTRAS_ReplayStart[1].status = IT_DISABLED;
					EXTRAS_ReplayStart[2].status = IT_CALL|IT_STRING;
					//EXTRAS_ReplayStart[2].alphaKey = 0;
					itemOn = 2;
					break;
				}
		}
	}
}

boolean M_QuitReplayHut(void)
{
	// D_StartTitle does its own wipe, since GS_TIMEATTACK is now a complete gamestate.
	menuactive = false;
	D_StartTitle();

	if (extrasmenu.demolist)
		Z_Free(extrasmenu.demolist);
	extrasmenu.demolist = NULL;

	demo.inreplayhut = false;

	return true;
}

void M_HutStartReplay(INT32 choice)
{
	(void)choice;

	M_ClearMenus(false);
	demo.loadfiles = (itemOn == 0);
	demo.ignorefiles = (itemOn != 0);

	G_DoPlayDemo(extrasmenu.demolist[dir_on[menudepthleft]].filepath);
}


static void Splitplayers_OnChange(void)
{
#if 0
	if (cv_splitplayers.value < setupm_pselect)
		setupm_pselect = 1;
#endif
}

// Misc menus

// Addons menu: (Merely copypasted, original code by toaster)

void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

#if 1
	if (cv_addons_option.value == 0)
		pathname = usehome ? srb2home : srb2path;
	else if (cv_addons_option.value == 1)
		pathname = srb2home;
	else if (cv_addons_option.value == 2)
		pathname = srb2path;
	else
#endif
	if (cv_addons_option.value == 3 && *cv_addons_folder.string != '\0')
		pathname = cv_addons_folder.string;

	strlcpy(menupath, pathname, 1024);
	menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath) + 1;

	if (menupath[menupathindex[menudepthleft]-2] != PATHSEP[0])
	{
		menupath[menupathindex[menudepthleft]-1] = PATHSEP[0];
		menupath[menupathindex[menudepthleft]] = 0;
	}
	else
		--menupathindex[menudepthleft];

	if (!preparefilemenu(false, false))
	{
		M_StartMessage(va("No files/folders found.\n\n%s\n\n(Press a key)\n", LOCATIONSTRING1),NULL,MM_NOTHING);
		return;
	}
	else
		dir_on[menudepthleft] = 0;

	MISC_AddonsDef.lastOn = 0; // Always start on search

	MISC_AddonsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_AddonsDef, false);
}


char *M_AddonsHeaderPath(void)
{
	UINT32 len;
	static char header[1024];

	strlcpy(header, va("%s folder%s", cv_addons_option.string, menupath+menupathindex[menudepth-1]-1), 1024);
	len = strlen(header);
	if (len > 34)
	{
		len = len-34;
		header[len] = header[len+1] = header[len+2] = '.';
	}
	else
		len = 0;

	return header+len;
}

#define UNEXIST S_StartSound(NULL, sfx_s26d);\
		M_SetupNextMenu(MISC_AddonsDef.prevMenu, false);\
		M_StartMessage(va("\x82%s\x80\nThis folder no longer exists!\nAborting to main menu.\n\n(Press a key)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING)

#define CLEARNAME Z_Free(refreshdirname);\
					refreshdirname = NULL

static boolean prevmajormods = false;

static void M_AddonsClearName(INT32 choice)
{
	if (!majormods || prevmajormods)
	{
		CLEARNAME;
	}
	M_StopMessage(choice);
}

// Handles messages for addon errors.
void M_AddonsRefresh(void)
{
	if ((refreshdirmenu & REFRESHDIR_NORMAL) && !preparefilemenu(true, false))
	{
		UNEXIST;
		if (refreshdirname)
		{
			CLEARNAME;
		}
		return;// true;
	}

#ifdef DEVELOP
	prevmajormods = majormods;
#else
 	if (!majormods && prevmajormods)
 		prevmajormods = false;
#endif

	if ((refreshdirmenu & REFRESHDIR_ADDFILE) || (majormods && !prevmajormods))
	{
		char *message = NULL;

		if (refreshdirmenu & REFRESHDIR_NOTLOADED)
		{
			S_StartSound(NULL, sfx_s26d);
			if (refreshdirmenu & REFRESHDIR_MAX)
				message = va("%c%s\x80\nMaximum number of addons reached.\nA file could not be loaded.\nIf you wish to play with this addon, restart the game to clear existing ones.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			else
				message = va("%c%s\x80\nA file was not loaded.\nCheck the console log for more info.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
		}
		else if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_s224);
			message = va("%c%s\x80\nA file was loaded with %s.\nCheck the console log for more info.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings"));
		}
		else if (majormods && !prevmajormods)
		{
			S_StartSound(NULL, sfx_s221);
			message = va("%c%s\x80\nYou've loaded a gameplay-modifying addon.\n\nRecord Attack has been disabled, but you\ncan still play alone in local Multiplayer.\n\nIf you wish to play Record Attack mode, restart the game to disable loaded addons.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			prevmajormods = majormods;
		}

		if (message)
		{
			M_StartMessage(message,FUNCPTRCAST(M_AddonsClearName),MM_YESNO);
			return;// true;
		}

		S_StartSound(NULL, sfx_s221);
		CLEARNAME;
	}

	return;// false;
}

static void M_AddonExec(INT32 ch)
{
	if (ch == MA_YES)
	{
		S_StartSound(NULL, sfx_zoom);
		COM_BufAddText(va("exec \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
	}
}

static void M_UpdateAddonsSearch(void)
{
	menusearch[0] = strlen(cv_dummyaddonsearch.string);
	strlcpy(menusearch+1, cv_dummyaddonsearch.string, MAXSTRINGLENGTH);
	if (!cv_addons_search_case.value)
		strupr(menusearch+1);

#if 0 // much slower
	if (!preparefilemenu(true, false))
	{
		UNEXIST;
		return;
	}
#else // streamlined
	searchfilemenu(NULL);
#endif
}

void M_HandleAddons(INT32 choice)
{
	const UINT8 pid = 0;
	boolean exitmenu = false; // exit to previous menu

	(void) choice;

	if (menucmd[pid].dpad_ud > 0)
	{
		if (dir_on[menudepthleft] < sizedirmenu-1)
		{
			dir_on[menudepthleft]++;
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_NextOpt())
		{
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(pid);
	}
	else if (menucmd[pid].dpad_ud < 0)
	{
		if (dir_on[menudepthleft])
		{
			dir_on[menudepthleft]--;
			S_StartSound(NULL, sfx_s3k5b);
		}
		else if (M_PrevOpt())
		{
			S_StartSound(NULL, sfx_s3k5b);
		}
		M_SetMenuDelay(pid);
	}

	else if (M_MenuButtonPressed(pid, MBT_L))
	{
		UINT8 i;
		for (i = numaddonsshown; i && (dir_on[menudepthleft] < sizedirmenu-1); i--)
			dir_on[menudepthleft]++;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuButtonPressed(pid, MBT_R))
	{
		UINT8 i;
		for (i = numaddonsshown; i && (dir_on[menudepthleft]); i--)
			dir_on[menudepthleft]--;

		S_StartSound(NULL, sfx_s3k5b);
		M_SetMenuDelay(pid);
	}

	else if (M_MenuConfirmPressed(pid))
	{
		boolean refresh = true;
		M_SetMenuDelay(pid);

		if (!dirmenu[dir_on[menudepthleft]])
			S_StartSound(NULL, sfx_s26d);
		else
		{
			switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
			{
				case EXT_FOLDER:
					strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
					if (menudepthleft)
					{
						menupathindex[--menudepthleft] = strlen(menupath);
						menupath[menupathindex[menudepthleft]] = 0;

						if (!preparefilemenu(false, false))
						{
							S_StartSound(NULL, sfx_s224);
							M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
							menupath[menupathindex[++menudepthleft]] = 0;

							if (!preparefilemenu(true, false))
							{
								UNEXIST;
								return;
							}
						}
						else
						{
							S_StartSound(NULL, sfx_s3k5b);
							dir_on[menudepthleft] = 1;
						}
						refresh = false;
					}
					else
					{
						S_StartSound(NULL, sfx_s26d);
						M_StartMessage(va("%c%s\x80\nThis folder is too deep to navigate to!\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
						menupath[menupathindex[menudepthleft]] = 0;
					}
					break;

				case EXT_UP:
					S_StartSound(NULL, sfx_s3k5b);
					menupath[menupathindex[++menudepthleft]] = 0;
					if (!preparefilemenu(false, false))
					{
						UNEXIST;
						return;
					}
					break;

				case EXT_TXT:
					M_StartMessage(va("%c%s\x80\nThis file may not be a console script.\nAttempt to run anyways? \n\n(Press A to confirm)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),FUNCPTRCAST(M_AddonExec),MM_YESNO);
					break;

				case EXT_CFG:
					M_StartMessage(va("%c%s\x80\nThis file may modify your settings.\nAttempt to run anyways? \n\n(Press A to confirm)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),FUNCPTRCAST(M_AddonExec),MM_YESNO);
					break;

				case EXT_LUA:
				case EXT_SOC:
				case EXT_WAD:
#ifdef USE_KART
				case EXT_KART:
#endif
				case EXT_PK3:
					COM_BufAddText(va("addfile \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
					break;

				default:
					S_StartSound(NULL, sfx_s26d);
			}

			if (refresh)
				refreshdirmenu |= REFRESHDIR_NORMAL;
		}
	}
	else if (M_MenuBackPressed(pid))
	{
		exitmenu = true;
		M_SetMenuDelay(pid);
	}


	if (exitmenu)
	{
		closefilemenu(true);

		// Secret menu!
		//MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);

		M_SetMenuDelay(pid);
	}
}

// Opening manual
void M_Manual(INT32 choice)
{
	(void)choice;

	MISC_ManualDef.prevMenu = (choice == INT32_MAX ? NULL : currentMenu);
	M_SetupNextMenu(&MISC_ManualDef, true);
}
