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
#include "k_kart.h" // SRB2kart
#include "d_player.h" // KITEM_ constants
#include "doomstat.h" // MAXSPLITSCREENPLAYERS

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

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

boolean menuactive = false;
boolean fromlevelselect = false;

// current menudef
menu_t *currentMenu = &MainDef;

char dummystaffname[22];

INT16 itemOn = 0; // menu item skull is on, Hack by Tails 09-18-2002
INT16 skullAnimCounter = 10; // skull animation counter
struct menutransition_s menutransition; // Menu transition properties

// finish wipes between screens
boolean menuwipe = false;

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

// Consvar onchange functions
static void Nextmap_OnChange(void);
static void Newgametype_OnChange(void);
static void Dummymenuplayer_OnChange(void);
//static void Dummymares_OnChange(void);
static void Dummystaff_OnChange(void);

consvar_t cv_showfocuslost = {"showfocuslost", "Yes", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL };

static CV_PossibleValue_t serversort_cons_t[] = {
	{0,"Ping"},
	{1,"Modified State"},
	{2,"Most Players"},
	{3,"Least Players"},
	{4,"Max Player Slots"},
	{5,"Gametype"},
	{0,NULL}
};
consvar_t cv_serversort = {"serversort", "Ping", CV_CALL, serversort_cons_t, M_SortServerList, 0, NULL, NULL, 0, 0, NULL};

// autorecord demos for time attack
static consvar_t cv_autorecord = {"autorecord", "Yes", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show Character"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_besttime  = {"ghost_besttime",  "Show All", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_bestlap   = {"ghost_bestlap",   "Show All", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_last      = {"ghost_last",      "Show All", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_guest     = {"ghost_guest",     "Show", CV_SAVE, ghost2_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_staff     = {"ghost_staff",     "Show", CV_SAVE, ghost2_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static void Splitplayers_OnChange(void);
CV_PossibleValue_t splitplayers_cons_t[] = {{1, "One"}, {2, "Two"}, {3, "Three"}, {4, "Four"}, {0, NULL}};
consvar_t cv_splitplayers = {"splitplayers", "One", CV_CALL, splitplayers_cons_t, Splitplayers_OnChange, 0, NULL, NULL, 0, 0, NULL};

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.
static CV_PossibleValue_t map_cons_t[] = {
	{0,"MIN"},
	{NUMMAPS, "MAX"},
	{0, NULL}
};
consvar_t cv_nextmap = {"nextmap", "1", CV_HIDDEN|CV_CALL, map_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = {"chooseskin", DEFAULTSKIN, CV_HIDDEN|CV_CALL, skins_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

// This gametype list is integral for many different reasons.
// When you add gametypes here, don't forget to update them in dehacked.c and doomstat.h!
CV_PossibleValue_t gametype_cons_t[NUMGAMETYPES+1];
consvar_t cv_newgametype = {"newgametype", "Race", CV_HIDDEN|CV_CALL, gametype_cons_t, Newgametype_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t dummymenuplayer_cons_t[] = {{0, "NOPE"}, {1, "P1"}, {2, "P2"}, {3, "P3"}, {4, "P4"}, {0, NULL}};
static consvar_t cv_dummymenuplayer = {"dummymenuplayer", "P1", CV_HIDDEN|CV_CALL, dummymenuplayer_cons_t, Dummymenuplayer_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static consvar_t cv_dummyteam = {"dummyteam", "Spectator", CV_HIDDEN, dummyteam_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t dummyspectate_cons_t[] = {{0, "Spectator"}, {1, "Playing"}, {0, NULL}};
static consvar_t cv_dummyspectate = {"dummyspectate", "Spectator", CV_HIDDEN, dummyspectate_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static consvar_t cv_dummyscramble = {"dummyscramble", "Random", CV_HIDDEN, dummyscramble_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t dummystaff_cons_t[] = {{0, "MIN"}, {100, "MAX"}, {0, NULL}};
static consvar_t cv_dummystaff = {"dummystaff", "0", CV_HIDDEN|CV_CALL, dummystaff_cons_t, Dummystaff_OnChange, 0, NULL, NULL, 0, 0, NULL};

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

// Prototypes
#if 0
static INT32 M_FindFirstMap(INT32 gtype);
static INT32 M_GetFirstLevelInList(void);
#endif

// Nextmap.  Used for Time Attack.
static void Nextmap_OnChange(void)
{
	char *leveltitle;

	// Update the string in the consvar.
	Z_Free(cv_nextmap.zstring);
	leveltitle = G_BuildMapTitle(cv_nextmap.value);
	cv_nextmap.string = cv_nextmap.zstring = leveltitle ? leveltitle : Z_StrDup(G_BuildMapName(cv_nextmap.value));

#if 0
	if (currentMenu == &SP_TimeAttackDef)
	{
		// see also p_setup.c's P_LoadRecordGhosts
		const size_t glen = strlen(srb2home)+1+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
		char *gpath = malloc(glen);
		INT32 i;
		UINT8 active;

		if (!gpath)
			return;

		sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));

		CV_StealthSetValue(&cv_dummystaff, 0);

		active = false;
		SP_TimeAttackMenu[taguest].status = IT_DISABLED;
		SP_TimeAttackMenu[tareplay].status = IT_DISABLED;
		//SP_TimeAttackMenu[taghost].status = IT_DISABLED;

		// Check if file exists, if not, disable REPLAY option
		for (i = 0; i < 4; i++)
		{
			SP_ReplayMenu[i].status = IT_DISABLED;
			SP_GuestReplayMenu[i].status = IT_DISABLED;
		}
		SP_ReplayMenu[4].status = IT_DISABLED;

		SP_GhostMenu[3].status = IT_DISABLED;
		SP_GhostMenu[4].status = IT_DISABLED;

		if (FIL_FileExists(va("%s-%s-time-best.lmp", gpath, cv_chooseskin.string))) {
			SP_ReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active |= 3;
		}
		if (FIL_FileExists(va("%s-%s-lap-best.lmp", gpath, cv_chooseskin.string))) {
			SP_ReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active |= 3;
		}
		if (FIL_FileExists(va("%s-%s-last.lmp", gpath, cv_chooseskin.string))) {
			SP_ReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active |= 3;
		}

		if (FIL_FileExists(va("%s-guest.lmp", gpath)))
		{
			SP_ReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_GhostMenu[3].status = IT_STRING|IT_CVAR;
			active |= 3;
		}

		CV_SetValue(&cv_dummystaff, 1);
		if (cv_dummystaff.value)
		{
			SP_ReplayMenu[4].status = IT_WHITESTRING|IT_KEYHANDLER;
			SP_GhostMenu[4].status = IT_STRING|IT_CVAR;
			CV_StealthSetValue(&cv_dummystaff, 1);
			active |= 1;
		}

		if (active) {
			if (active & 1)
				SP_TimeAttackMenu[tareplay].status = IT_WHITESTRING|IT_SUBMENU;
			if (active & 2)
				SP_TimeAttackMenu[taguest].status = IT_WHITESTRING|IT_SUBMENU;
		}
		else if (itemOn == tareplay) // Reset lastOn so replay isn't still selected when not available.
		{
			currentMenu->lastOn = itemOn;
			itemOn = tastart;
		}

		if (mapheaderinfo[cv_nextmap.value-1] && mapheaderinfo[cv_nextmap.value-1]->forcecharacter[0] != '\0')
			CV_Set(&cv_chooseskin, mapheaderinfo[cv_nextmap.value-1]->forcecharacter);

		free(gpath);
	}
#endif
}

static void Dummymenuplayer_OnChange(void)
{
	if (cv_dummymenuplayer.value < 1)
		CV_StealthSetValue(&cv_dummymenuplayer, splitscreen+1);
	else if (cv_dummymenuplayer.value > splitscreen+1)
		CV_StealthSetValue(&cv_dummymenuplayer, 1);
}

static void Dummystaff_OnChange(void)
{
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
}

// Newgametype.  Used for gametype changes.
static void Newgametype_OnChange(void)
{
#if 0
	if (cv_nextmap.value && menuactive)
	{
		if (!mapheaderinfo[cv_nextmap.value-1])
			P_AllocMapHeader((INT16)(cv_nextmap.value-1));

		if ((cv_newgametype.value == GT_RACE && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_RACE))
			|| (cv_newgametype.value == GT_MATCH && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_MATCH)))
		{
			INT32 value = 0;

			switch (cv_newgametype.value)
			{
				default:
				case GT_RACE:
					value = TOL_RACE;
					break;
				case GT_MATCH:
					value = TOL_MATCH;
					break;
			}

			CV_SetValue(&cv_nextmap, M_FindFirstMap(value));
		}
	}
#endif
}

void Screenshot_option_Onchange(void)
{
#if 0
	OP_ScreenshotOptionsMenu[op_screenshot_folder].status =
		(cv_screenshot_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
#endif
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

void Addons_option_Onchange(void)
{
#if 0
	OP_AddonsOptionsMenu[op_addons_folder].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
#endif
}

void M_SortServerList(void)
{
#if 0
#ifndef NONET
	switch(cv_serversort.value)
	{
	case 0:		// Ping.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_time);
		break;
	case 1:		// Modified state.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_modified);
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
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametype);
		break;
	}
#endif
#endif
}

// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

	if (choice == -1)
	{
		if (cv == &cv_playercolor)
		{
			SINT8 skinno = R_SkinAvailable(cv_chooseskin.string);
			if (skinno != -1)
				CV_SetValue(cv,skins[skinno].prefcolor);
			return;
		}
		CV_Set(cv,cv->defaultvalue);
		return;
	}

	choice = (choice<<1) - 1;

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_INVISSLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		CV_SetValue(cv,cv->value+choice);
	}
	else if (cv->flags & CV_FLOAT)
	{
		char s[20];
		sprintf(s,"%f",FIXED_TO_FLOAT(cv->value)+(choice)*(1.0f/16.0f));
		CV_Set(cv,s);
	}
	else
	{
#ifndef NONET
		if (cv == &cv_nettimeout || cv == &cv_jointimeout)
			choice *= (TICRATE/7);
		else if (cv == &cv_maxsend)
			choice *= 512;
		else if (cv == &cv_maxping)
			choice *= 50;
#endif
		CV_AddValue(cv,choice);
	}
}

static boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
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

static void M_NextOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		((consvar_t *)currentMenu->menuitems[itemOn].itemaction)->value = 0;

	do
	{
		if (itemOn + 1 > currentMenu->numitems - 1)
			itemOn = 0;
		else
			itemOn++;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);
}

static void M_PrevOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop

	if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		((consvar_t *)currentMenu->menuitems[itemOn].itemaction)->value = 0;

	do
	{
		if (!itemOn)
			itemOn = currentMenu->numitems - 1;
		else
			itemOn--;
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);
}

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	INT32 ch = -1;
//	INT32 i;
	static tic_t joywait = 0, mousewait = 0;
	static INT32 pjoyx = 0, pjoyy = 0;
	static INT32 pmousex = 0, pmousey = 0;
	static INT32 lastx = 0, lasty = 0;
	void (*routine)(INT32 choice); // for some casting problem

	if (dedicated || (demo.playback && demo.title)
	|| gamestate == GS_INTRO || gamestate == GS_CUTSCENE || gamestate == GS_GAMEEND
	|| gamestate == GS_CREDITS || gamestate == GS_EVALUATION)
		return false;

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}
	else if (ev->type == ev_keydown)
	{
		ch = ev->data1;

		// added 5-2-98 remap virtual keys (mouse & joystick buttons)
		switch (ch)
		{
			case KEY_MOUSE1:
				//case KEY_JOY1:
				//case KEY_JOY1 + 2:
				ch = KEY_ENTER;
				break;
				/*case KEY_JOY1 + 3: // Brake can function as 'n' for message boxes now.
					ch = 'n';
					break;*/
			case KEY_MOUSE1 + 1:
				//case KEY_JOY1 + 1:
				ch = KEY_BACKSPACE;
				break;
			case KEY_HAT1:
				ch = KEY_UPARROW;
				break;
			case KEY_HAT1 + 1:
				ch = KEY_DOWNARROW;
				break;
			case KEY_HAT1 + 2:
				ch = KEY_LEFTARROW;
				break;
			case KEY_HAT1 + 3:
				ch = KEY_RIGHTARROW;
				break;
		}
	}
	else if (menuactive)
	{
		if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			const INT32 jdeadzone = ((JOYAXISRANGE-1) * cv_deadzone.value) >> FRACBITS;
			if (ev->data3 != INT32_MAX)
			{
				if (Joystick.bGamepadStyle || abs(ev->data3) > jdeadzone)
				{
					if (ev->data3 < 0 && pjoyy >= 0)
					{
						ch = KEY_UPARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					else if (ev->data3 > 0 && pjoyy <= 0)
					{
						ch = KEY_DOWNARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					pjoyy = ev->data3;
				}
				else
					pjoyy = 0;
			}

			if (ev->data2 != INT32_MAX)
			{
				if (Joystick.bGamepadStyle || abs(ev->data2) > jdeadzone)
				{
					if (ev->data2 < 0 && pjoyx >= 0)
					{
						ch = KEY_LEFTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					else if (ev->data2 > 0 && pjoyx <= 0)
					{
						ch = KEY_RIGHTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					pjoyx = ev->data2;
				}
				else
					pjoyx = 0;
			}
		}
		else if (ev->type == ev_mouse && mousewait < I_GetTime())
		{
			pmousey += ev->data3;
			if (pmousey < lasty-30)
			{
				ch = KEY_DOWNARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty -= 30;
			}
			else if (pmousey > lasty + 30)
			{
				ch = KEY_UPARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty += 30;
			}

			pmousex += ev->data2;
			if (pmousex < lastx - 30)
			{
				ch = KEY_LEFTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx -= 30;
			}
			else if (pmousex > lastx+30)
			{
				ch = KEY_RIGHTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx += 30;
			}
		}
	}

	if (ch == -1)
		return false;
	else if (ch == gamecontrol[gc_systemmenu][0] || ch == gamecontrol[gc_systemmenu][1]) // allow remappable ESC key
		ch = KEY_ESCAPE;
	else if ((ch == gamecontrol[gc_accelerate][0] || ch == gamecontrol[gc_accelerate][1])  && ch >= KEY_MOUSE1)
		ch = KEY_ENTER;

	// F-Keys
	if (!menuactive)
	{
		noFurtherInput = true;

		switch (ch)
		{
#if 0
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

#ifndef DC
			case KEY_F5: // Video Mode
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_VideoModeMenu(0);
				return true;
#endif

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

			case KEY_F11: // Gamma Level
				CV_AddValue(&cv_usegamma, 1);
				return true;

			// Spymode on F12 handled in game logic
#endif
			case KEY_ESCAPE: // Pop up menu
				if (chat_on)
				{
					HU_clearChatChars();
					chat_on = false;
				}
				else
					M_StartControlPanel();
				return true;
		}

		noFurtherInput = false; // turns out we didn't care
		return false;
	}


	if ((ch == gamecontrol[gc_brake][0] || ch == gamecontrol[gc_brake][1]) && ch >= KEY_MOUSE1) // do this here, otherwise brake opens the menu mid-game
		ch = KEY_ESCAPE;

	routine = currentMenu->menuitems[itemOn].itemaction;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		if (shiftdown && ch >= 32 && ch <= 127)
			ch = shiftxform[ch];
		routine(ch);
		return true;
	}

	if (currentMenu->menuitems[itemOn].status == IT_MSGHANDLER)
	{
		if (currentMenu->menuitems[itemOn].mvar1 != MM_EVENTHANDLER)
		{
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER)
			{
				if (routine)
					routine(ch);
				M_StopMessage(0);
				noFurtherInput = true;
				return true;
			}
			return true;
		}
		else
		{
			// dirty hack: for customising controls, I want only buttons/keys, not moves
			if (ev->type == ev_mouse || ev->type == ev_mouse2 || ev->type == ev_joystick
				|| ev->type == ev_joystick2 || ev->type == ev_joystick3 || ev->type == ev_joystick4)
				return true;
			if (routine)
			{
				void (*otherroutine)(event_t *sev) = currentMenu->menuitems[itemOn].itemaction;
				otherroutine(ev); //Alam: what a hack
			}
			return true;
		}
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING || (currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
		{
			if (ch == KEY_TAB && (currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_PASSWORD)
				((consvar_t *)currentMenu->menuitems[itemOn].itemaction)->value ^= 1;

			if (shiftdown && ch >= 32 && ch <= 127)
				ch = shiftxform[ch];
			if (M_ChangeStringCvar(ch))
				return true;
			else
				routine = NULL;
		}
		else
			routine = M_ChangeCvar;
	}

	if (currentMenu == &PAUSE_PlaybackMenuDef)
	{
		// Flip left/right with up/down for the playback menu, since it's a horizontal icon row.
		switch (ch)
		{
			case KEY_LEFTARROW: ch = KEY_UPARROW; break;
			case KEY_UPARROW: ch = KEY_RIGHTARROW; break;
			case KEY_RIGHTARROW: ch = KEY_DOWNARROW; break;
			case KEY_DOWNARROW: ch = KEY_LEFTARROW; break;
			default: break;
		}
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_s3k5b);
			return true;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_s3k5b);
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
#if 0
				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
#endif
					S_StartSound(NULL, sfx_s3k5b);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
#if 0
				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
#endif
					S_StartSound(NULL, sfx_s3k5b);
				routine(1);
			}
			return true;

		case KEY_ENTER:
			noFurtherInput = true;
			currentMenu->lastOn = itemOn;

#if 0
			if (currentMenu == &PAUSE_PlaybackMenuDef)
			{
				boolean held = (boolean)playback_enterheld;
				playback_enterheld = TICRATE/7;
				if (held)
					return true;
			}
#endif

			if (routine)
			{
				S_StartSound(NULL, sfx_s3k5b);

				if (((currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_CALL
				 || (currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_SUBMENU)
				 && (currentMenu->menuitems[itemOn].status & IT_CALLTYPE))
				{
					if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && majormods)
					{
						M_StartMessage(M_GetText("This cannot be done with complex add-ons\nor in a cheated game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
						return true;
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
						M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction, false);
						break;
				}
			}

			return true;

		case KEY_ESCAPE:
		//case KEY_JOY1 + 2:
			M_GoBack(0);
			return true;

		case KEY_BACKSPACE:
#if 0
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].mvar1);
				S_StartSound(NULL, sfx_shldls);
				return true;
			}
#endif

			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

				if (cv == &cv_chooseskin
					|| cv == &cv_dummystaff
					|| cv == &cv_nextmap
					|| cv == &cv_newgametype)
					return true;

#if 0
				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
#endif
					S_StartSound(NULL, sfx_s3k5b);

				routine(-1);
				return true;
			}

			return false;

		default:
			break;
	}

	return true;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
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

		S_ChangeMusicInternal("menu", true);
	}

	menuactive = true;

	if (demo.playback)
	{
		currentMenu = &PAUSE_PlaybackMenuDef;
	}
	else if (!Playing())
	{
		currentMenu = &MainDef;
		itemOn = 0;
	}
#if 0
	else if (modeattacking)
	{
		currentMenu = &MAPauseDef;
		itemOn = mapause_continue;
	}
	else if (!(netgame || multiplayer)) // Single Player
	{
		if (gamestate != GS_LEVEL) // intermission, so gray out stuff.
			SPauseMenu[spause_retry].status = IT_GRAYEDOUT;
		else
		{
			//INT32 numlives = 2;

			/*if (&players[consoleplayer])
			{
				numlives = players[consoleplayer].lives;
				if (players[consoleplayer].playerstate != PST_LIVE)
					++numlives;
			}

			// The list of things that can disable retrying is (was?) a little too complex
			// for me to want to use the short if statement syntax
			if (numlives <= 1 || G_IsSpecialStage(gamemap))
				SPauseMenu[spause_retry].status = (IT_GRAYEDOUT);
			else*/
				SPauseMenu[spause_retry].status = (IT_STRING | IT_CALL);
		}

		currentMenu = &SPauseDef;
		itemOn = spause_continue;
	}
	else // multiplayer
	{
		MPauseMenu[mpause_switchmap].status = IT_DISABLED;
		MPauseMenu[mpause_addons].status = IT_DISABLED;
		MPauseMenu[mpause_scramble].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit2].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit3].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit4].status = IT_DISABLED;
		MPauseMenu[mpause_spectate].status = IT_DISABLED;
		MPauseMenu[mpause_entergame].status = IT_DISABLED;
		MPauseMenu[mpause_canceljoin].status = IT_DISABLED;
		MPauseMenu[mpause_switchteam].status = IT_DISABLED;
		MPauseMenu[mpause_switchspectate].status = IT_DISABLED;
		MPauseMenu[mpause_psetup].status = IT_DISABLED;
		MISC_ChangeTeamMenu[0].status = IT_DISABLED;
		MISC_ChangeSpectateMenu[0].status = IT_DISABLED;
		// Reset these in case splitscreen messes things up
		MPauseMenu[mpause_switchteam].mvar1 = 48;
		MPauseMenu[mpause_switchspectate].mvar1 = 48;
		MPauseMenu[mpause_options].mvar1 = 64;
		MPauseMenu[mpause_title].mvar1 = 80;
		MPauseMenu[mpause_quit].mvar1 = 88;
		Dummymenuplayer_OnChange();

		if ((server || IsPlayerAdmin(consoleplayer)))
		{
			MPauseMenu[mpause_switchmap].status = IT_STRING | IT_CALL;
			MPauseMenu[mpause_addons].status = IT_STRING | IT_CALL;
			if (G_GametypeHasTeams())
				MPauseMenu[mpause_scramble].status = IT_STRING | IT_SUBMENU;
		}

		if (splitscreen)
		{
			MPauseMenu[mpause_psetupsplit].status = MPauseMenu[mpause_psetupsplit2].status = IT_STRING | IT_CALL;
			MISC_ChangeTeamMenu[0].status = MISC_ChangeSpectateMenu[0].status = IT_STRING|IT_CVAR;

			if (netgame)
			{
				if (G_GametypeHasTeams())
				{
					MPauseMenu[mpause_switchteam].status = IT_STRING | IT_SUBMENU;
					MPauseMenu[mpause_switchteam].mvar1 += ((splitscreen+1) * 8);
					MPauseMenu[mpause_options].mvar1 += 8;
					MPauseMenu[mpause_title].mvar1 += 8;
					MPauseMenu[mpause_quit].mvar1 += 8;
				}
				else if (G_GametypeHasSpectators())
				{
					MPauseMenu[mpause_switchspectate].status = IT_STRING | IT_SUBMENU;
					MPauseMenu[mpause_switchspectate].mvar1 += ((splitscreen+1) * 8);
					MPauseMenu[mpause_options].mvar1 += 8;
					MPauseMenu[mpause_title].mvar1 += 8;
					MPauseMenu[mpause_quit].mvar1 += 8;
				}
			}

			if (splitscreen > 1)
			{
				MPauseMenu[mpause_psetupsplit3].status = IT_STRING | IT_CALL;

				MPauseMenu[mpause_options].mvar1 += 8;
				MPauseMenu[mpause_title].mvar1 += 8;
				MPauseMenu[mpause_quit].mvar1 += 8;

				if (splitscreen > 2)
				{
					MPauseMenu[mpause_psetupsplit4].status = IT_STRING | IT_CALL;
					MPauseMenu[mpause_options].mvar1 += 8;
					MPauseMenu[mpause_title].mvar1 += 8;
					MPauseMenu[mpause_quit].mvar1 += 8;
				}
			}
		}
		else
		{
			MPauseMenu[mpause_psetup].status = IT_STRING | IT_CALL;

			if (G_GametypeHasTeams())
				MPauseMenu[mpause_switchteam].status = IT_STRING | IT_SUBMENU;
			else if (G_GametypeHasSpectators())
			{
				if (!players[consoleplayer].spectator)
					MPauseMenu[mpause_spectate].status = IT_STRING | IT_CALL;
				else if (players[consoleplayer].pflags & PF_WANTSTOJOIN)
					MPauseMenu[mpause_canceljoin].status = IT_STRING | IT_CALL;
				else
					MPauseMenu[mpause_entergame].status = IT_STRING | IT_CALL;
			}
			else // in this odd case, we still want something to be on the menu even if it's useless
				MPauseMenu[mpause_spectate].status = IT_GRAYEDOUT;
		}

		currentMenu = &MPauseDef;
		itemOn = mpause_continue;
	}
#endif

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
		if (currentMenu->transitionOutTics)
		{
			menutransition.tics = 0;
			menutransition.dest = currentMenu->transitionOutTics;
			menutransition.in = false;
			menutransition.newmenu = menudef;
			return; // Don't change menu yet, the transition will call this again
		}
		else if (gamestate == GS_MENU)
		{
			menuwipe = true;
			F_WipeStartScreen();
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();
			F_RunWipe(wipedefs[wipe_menu_toblack], false, "FADEMAP0", false);
		}
	}

	if (currentMenu->quitroutine)
	{
		// If you're going from a menu to itself, why are you running the quitroutine? You're not quitting it! -SH
		if (currentMenu != menudef && !currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
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
			MSCloseUDPSocket();		// Clean up so we can re-open the connection later.
			netgame = false;
			multiplayer = false;
		}

		M_SetupNextMenu(currentMenu->prevMenu, false);
	}
	else
		M_ClearMenus(true);
}

//
// M_Ticker
//
void M_Ticker(void)
{
	if (menutransition.tics != 0 || menutransition.dest != 0)
	{
		noFurtherInput = true;

		if (menutransition.tics < menutransition.dest)
			menutransition.tics++;
		else if (menutransition.tics > menutransition.dest)
			menutransition.tics--;

		// If dest is non-zero, we've started transition and want to switch menus
		// If dest is zero, we're mid-transition and want to end it
		if (menutransition.tics == menutransition.dest && menutransition.newmenu != NULL)
		{
			M_SetupNextMenu(menutransition.newmenu, true);

			if (menutransition.newmenu->transitionInTics)
			{
				menutransition.tics = currentMenu->transitionOutTics;
				menutransition.dest = 0;
				menutransition.in = true;
				menutransition.newmenu = NULL;
			}
			else if (gamestate == GS_MENU)
			{
				memset(&menutransition, 0, sizeof(menutransition));

				menuwipe = true;
				F_WipeStartScreen();
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
				F_WipeEndScreen();
				F_RunWipe(wipedefs[wipe_menu_toblack], false, "FADEMAP0", false);
			}
		}
	}
	else
	{
		// reset input trigger
		noFurtherInput = false;
	}

	if (currentMenu->tickroutine)
		currentMenu->tickroutine();

	if (dedicated)
		return;

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
	//COM_AddCommand("manual", Command_Manual_f);

	CV_RegisterVar(&cv_nextmap);
	CV_RegisterVar(&cv_newgametype);
	CV_RegisterVar(&cv_chooseskin);
	CV_RegisterVar(&cv_autorecord);

	if (dedicated)
		return;

	// Menu hacks
	CV_RegisterVar(&cv_dummymenuplayer);
	CV_RegisterVar(&cv_dummyteam);
	CV_RegisterVar(&cv_dummyspectate);
	CV_RegisterVar(&cv_dummyscramble);
	CV_RegisterVar(&cv_dummystaff);

#if 0
#ifdef HWRENDER
	// Permanently hide some options based on render mode
	if (rendermode == render_soft)
		OP_VideoOptionsMenu[op_video_ogl].status =
			OP_VideoOptionsMenu[op_video_kartman].status =
			OP_VideoOptionsMenu[op_video_md2]    .status = IT_DISABLED;
#endif
#endif

#ifndef NONET
	CV_RegisterVar(&cv_serversort);
#endif

	//todo put this somewhere better...
	CV_RegisterVar(&cv_allcaps);
}

// ==================================================
// MESSAGE BOX (aka: a hacked, cobbled together menu)
// ==================================================
// Because this is just a "fake" menu, these definitions are not with the others
static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0, NULL, NULL, NULL, NULL, 0, 0}
};

menu_t MessageDef =
{
	1,                  // # of menu items
	NULL,               // previous menu       (TO HACK)
	0,                  // lastOn, flags       (TO HACK)
	MessageMenu,        // menuitem_t ->
	0, 0,               // x, y                (TO HACK)
	0, 0,               // transition tics
	M_DrawMessageMenu,  // drawing routine ->
	NULL,               // ticker routine
	NULL                // quit routine
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

	start = 0;
	max = 0;

	M_StartControlPanel(); // can't put menuactive to true

	if (currentMenu == &MessageDef) // Prevent recursion
		MessageDef.prevMenu = ((demo.playback) ? &PAUSE_PlaybackMenuDef : &MainDef);
	else
		MessageDef.prevMenu = currentMenu;

	MessageDef.menuitems[0].text     = message;
	MessageDef.menuitems[0].mvar1 = (UINT8)itemtype;
	if (!routine && itemtype != MM_NOTHING) itemtype = MM_NOTHING;
	switch (itemtype)
	{
		case MM_NOTHING:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = M_StopMessage;
			break;
		case MM_YESNO:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
		case MM_EVENTHANDLER:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
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
			start += i;
	}

	MessageDef.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	MessageDef.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	MessageDef.lastOn = (INT16)((strlines<<8)+max);

	//M_SetupNextMenu();
	currentMenu = &MessageDef;
	itemOn = 0;
}

void M_StopMessage(INT32 choice)
{
	(void)choice;
	if (menuactive)
		M_SetupNextMenu(MessageDef.prevMenu, true);
}

// =========
// IMAGEDEFS
// =========

// Handles the ImageDefs.  Just a specialized function that
// uses left and right movement.
void M_HandleImageDef(INT32 choice)
{
	boolean exitmenu = false;

	switch (choice)
	{
		case KEY_RIGHTARROW:
			if (itemOn >= (INT16)(currentMenu->numitems-1))
				break;
			S_StartSound(NULL, sfx_s3k5b);
			itemOn++;
			break;

		case KEY_LEFTARROW:
			if (!itemOn)
				break;

			S_StartSound(NULL, sfx_s3k5b);
			itemOn--;
			break;

		case KEY_ESCAPE:
		case KEY_ENTER:
			exitmenu = true;
			break;
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

	if (ch != 'y' && ch != KEY_ENTER)
		return;

	if (!(netgame || cv_debug))
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
			I_Sleep();
		}
	}

	I_Quit();
}

void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	(void)choice;
	M_StartMessage("Are you sure you want to quit playing?\n\n(Press 'Y' to exit)", M_QuitResponse, MM_YESNO);
}

// =========
// PLAY MENU
// =========

// Character Select!
struct setup_chargrid_s setup_chargrid[9][9];
setup_player_t setup_player[MAXSPLITSCREENPLAYERS];
struct setup_explosions_s setup_explosions[48];

UINT8 setup_numplayers = 0;
UINT16 setup_animcounter = 0;

consvar_t *setup_playercvars[MAXSPLITSCREENPLAYERS][SPLITCV_MAX];

void M_CharacterSelectInit(INT32 choice)
{
	UINT8 i, j;

	(void)choice;

	memset(setup_chargrid, -1, sizeof(setup_chargrid));
	for (i = 0; i < 9; i++)
	{
		for (j = 0; j < 9; j++)
			setup_chargrid[i][j].numskins = 0;
	}

	memset(setup_player, 0, sizeof(setup_player));
	setup_player[0].mdepth = CSSTEP_CHARS;
	setup_numplayers = 1;

	memset(setup_explosions, 0, sizeof(setup_explosions));
	setup_animcounter = 0;

	// Keep these in a table for the sake of my sanity later
	setup_playercvars[0][SPLITCV_SKIN] = &cv_skin;
	setup_playercvars[1][SPLITCV_SKIN] = &cv_skin2;
	setup_playercvars[2][SPLITCV_SKIN] = &cv_skin3;
	setup_playercvars[3][SPLITCV_SKIN] = &cv_skin4;

	setup_playercvars[0][SPLITCV_COLOR] = &cv_playercolor;
	setup_playercvars[1][SPLITCV_COLOR] = &cv_playercolor2;
	setup_playercvars[2][SPLITCV_COLOR] = &cv_playercolor3;
	setup_playercvars[3][SPLITCV_COLOR] = &cv_playercolor4;

	setup_playercvars[0][SPLITCV_NAME] = &cv_playername;
	setup_playercvars[1][SPLITCV_NAME] = &cv_playername2;
	setup_playercvars[2][SPLITCV_NAME] = &cv_playername3;
	setup_playercvars[3][SPLITCV_NAME] = &cv_playername4;

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
		}

		for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
		{
			if (!strcmp(setup_playercvars[j][SPLITCV_SKIN]->string, skins[i].name))
			{
				setup_player[j].gridx = x;
				setup_player[j].gridy = y;
				setup_player[j].color = skins[i].prefcolor;
			}
		}
	}

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

static void M_HandleCharacterGrid(INT32 choice, setup_player_t *p, UINT8 num)
{
	switch (choice)
	{
		case KEY_DOWNARROW:
			p->gridy++;
			if (p->gridy > 8)
				p->gridy = 0;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_UPARROW:
			p->gridy--;
			if (p->gridy < 0)
				p->gridy = 8;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_RIGHTARROW:
			p->gridx++;
			if (p->gridx > 8)
				p->gridx = 0;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_LEFTARROW:
			p->gridx--;
			if (p->gridx < 0)
				p->gridx = 8;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_ENTER:
			if (setup_chargrid[p->gridx][p->gridy].numskins == 0)
				S_StartSound(NULL, sfx_s3k7b); //sfx_s3kb2
			else
			{
				if (setup_chargrid[p->gridx][p->gridy].numskins == 1)
					p->mdepth = CSSTEP_COLORS; // Skip clones menu
				else
					p->mdepth = CSSTEP_ALTS;

				S_StartSound(NULL, sfx_s3k5b);
			}
			break;
		case KEY_ESCAPE:
			if (num == setup_numplayers-1)
			{
				p->mdepth = CSSTEP_NONE;
				S_StartSound(NULL, sfx_s3k5b);
			}
			else
			{
				S_StartSound(NULL, sfx_s3kb2);
			}
			break;
		default:
			break;
	}
}

static void M_HandleCharRotate(INT32 choice, setup_player_t *p)
{
	UINT8 numclones = setup_chargrid[p->gridx][p->gridy].numskins;

	switch (choice)
	{
		case KEY_RIGHTARROW:
			p->clonenum++;
			if (p->clonenum >= numclones)
				p->clonenum = 0;
			p->rotate = CSROTATETICS;
			p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3kc3s);
			break;
		case KEY_LEFTARROW:
			p->clonenum--;
			if (p->clonenum < 0)
				p->clonenum = numclones-1;
			p->rotate = -CSROTATETICS;
			p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3kc3s);
			break;
		case KEY_ENTER:
			p->mdepth = CSSTEP_COLORS;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		case KEY_ESCAPE:
			p->mdepth = CSSTEP_CHARS;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		default:
			break;
	}
}

static void M_HandleColorRotate(INT32 choice, setup_player_t *p)
{
	switch (choice)
	{
		case KEY_RIGHTARROW:
			p->color++;
			if (p->color >= MAXSKINCOLORS)
				p->color = 1;
			p->rotate = CSROTATETICS;
			//p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
			break;
		case KEY_LEFTARROW:
			p->color--;
			if (p->color < 1)
				p->color = MAXSKINCOLORS-1;
			p->rotate = -CSROTATETICS;
			//p->delay = CSROTATETICS;
			S_StartSound(NULL, sfx_s3k5b); //sfx_s3kc3s
			break;
		case KEY_ENTER:
			p->mdepth = CSSTEP_READY;
			p->delay = TICRATE;
			M_SetupReadyExplosions(p);
			S_StartSound(NULL, sfx_s3k4e);
			break;
		case KEY_ESCAPE:
			if (setup_chargrid[p->gridx][p->gridy].numskins == 1)
				p->mdepth = CSSTEP_CHARS; // Skip clones menu
			else
				p->mdepth = CSSTEP_ALTS;
			S_StartSound(NULL, sfx_s3k5b);
			break;
		default:
			break;
	}
}

void M_CharacterSelectHandler(INT32 choice)
{
	UINT8 i;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		setup_player_t *p = &setup_player[i];

		if (i > 0)
			break; // temp

		if (p->delay == 0)
		{
			switch (p->mdepth)
			{
				case CSSTEP_NONE: // Enter Game
					if (choice == KEY_ENTER && i == setup_numplayers)
					{
						p->mdepth = CSSTEP_CHARS;
						S_StartSound(NULL, sfx_s3k65);
					}
					break;
				case CSSTEP_CHARS: // Character Select grid
					M_HandleCharacterGrid(choice, p, i);
					break;
				case CSSTEP_ALTS: // Select clone
					M_HandleCharRotate(choice, p);
					break;
				case CSSTEP_COLORS: // Select color
					M_HandleColorRotate(choice, p);
					break;
				case CSSTEP_READY:
				default: // Unready
					if (choice == KEY_ESCAPE)
					{
						p->mdepth = CSSTEP_COLORS;
						S_StartSound(NULL, sfx_s3k5b);
					}
					break;
			}
		}

		if (p->mdepth < CSSTEP_ALTS)
			p->clonenum = 0;

		// Just makes it easier to access later
		p->skin = setup_chargrid[p->gridx][p->gridy].skinlist[p->clonenum];

		if (p->mdepth < CSSTEP_COLORS)
			p->color = skins[p->skin].prefcolor;
	}

	// Setup new numplayers
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (setup_player[i].mdepth == CSSTEP_NONE)
			break;
		else
			setup_numplayers = i+1;
	}

	// If the first player unjoins, then we get outta here
	if (setup_player[0].mdepth == CSSTEP_NONE)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu, false);
		else
			M_ClearMenus(true);
	}
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

	if (setupnext)
	{
		for (i = 0; i < setup_numplayers; i++)
		{
			CV_StealthSet(setup_playercvars[i][SPLITCV_SKIN], skins[setup_player[i].skin].name);
			CV_StealthSetValue(setup_playercvars[i][SPLITCV_COLOR], setup_player[i].color);
		}

		CV_StealthSetValue(&cv_splitplayers, setup_numplayers);
		M_SetupNextMenu(&PLAY_MainDef, false);
	}
}

boolean M_CharacterSelectQuit(void)
{
	M_CharacterSelectInit(0);
	return true;
}

// =====================
// PAUSE / IN-GAME MENUS
// =====================
void M_EndModeAttackRun(void)
{
#if 0
	M_ModeAttackEndGame(0);
#endif
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


void M_ReplayHut(INT32 choice)
{
	(void)choice;
}

static void Splitplayers_OnChange(void)
{
#if 0
	if (cv_splitplayers.value < setupm_pselect)
		setupm_pselect = 1;
#endif
}
