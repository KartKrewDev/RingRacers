// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Inuyasha" Walsh.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_menu.h
/// \brief Menu widget stuff, selection and such

#ifndef __K_MENU__
#define __K_MENU__

#include "d_event.h"
#include "command.h"
#include "doomstat.h" // MAXSPLITSCREENPLAYERS

// flags for items in the menu
// menu handle (what we do when key is pressed
#define IT_TYPE             14     // (2+4+8)
#define IT_CALL              0     // call the function
#define IT_ARROWS            2     // call function with 0 for left arrow and 1 for right arrow in param
#define IT_KEYHANDLER        4     // call with the key in param
#define IT_SUBMENU           6     // go to sub menu
#define IT_CVAR              8     // handle as a cvar
#define IT_SPACE            10     // no handling
#define IT_MSGHANDLER       12     // same as key but with event and sometime can handle y/n key (special for message

#define IT_DISPLAY   (48+64+128)    // 16+32+64+128
#define IT_NOTHING            0     // space
#define IT_PATCH             16     // a patch or a string with big font
#define IT_STRING            32     // little string (spaced with 10)
#define IT_WHITESTRING       48     // little string in white
#define IT_DYBIGSPACE        64     // same as noting
#define IT_DYLITLSPACE   (16+64)    // little space
#define IT_STRING2       (32+64)    // a simple string
#define IT_GRAYPATCH     (16+32+64) // grayed patch or big font string
#define IT_BIGSLIDER        128     // volume sound use this
#define IT_TRANSTEXT     (16+128)   // Transparent text
#define IT_TRANSTEXT2    (32+128)   // used for control names
#define IT_HEADERTEXT    (48+128)   // Non-selectable header option, displays in yellow offset to the left a little
#define IT_QUESTIONMARKS (64+128)   // Displays as question marks, used for secrets
#define IT_CENTER           256     // if IT_PATCH, center it on screen

//consvar specific
#define IT_CVARTYPE   (512+1024+2048)
#define IT_CV_NORMAL         0
#define IT_CV_SLIDER       512
#define IT_CV_STRING      1024
#define IT_CV_NOPRINT     1536
#define IT_CV_NOMOD       2048
#define IT_CV_INVISSLIDER 2560
#define IT_CV_PASSWORD    3072

//call/submenu specific
// There used to be a lot more here but ...
// A lot of them became redundant with the advent of the Pause menu, so they were removed
#define IT_CALLTYPE   (512+1024)
#define IT_CALL_NORMAL          0
#define IT_CALL_NOTMODIFIED   512

// in INT16 for some common use
#define IT_BIGSPACE    (IT_SPACE  +IT_DYBIGSPACE)
#define IT_LITLSPACE   (IT_SPACE  +IT_DYLITLSPACE)
#define IT_CONTROL     (IT_STRING2+IT_CALL)
#define IT_CVARMAX     (IT_CVAR   +IT_CV_NOMOD)
#define IT_DISABLED    (IT_SPACE  +IT_GRAYPATCH)
#define IT_GRAYEDOUT   (IT_SPACE  +IT_TRANSTEXT)
#define IT_GRAYEDOUT2  (IT_SPACE  +IT_TRANSTEXT2)
#define IT_HEADER      (IT_SPACE  +IT_HEADERTEXT)
#define IT_SECRET      (IT_SPACE  +IT_QUESTIONMARKS)

#define MAXSTRINGLENGTH 32

typedef union
{
	struct menu_s *submenu;      // IT_SUBMENU
	consvar_t *cvar;             // IT_CVAR
	void (*routine)(INT32 choice); // IT_CALL, IT_KEYHANDLER, IT_ARROWS
} itemaction_t;

//
// MENU TYPEDEFS
//
typedef struct menuitem_s
{
	UINT16 status; // show IT_xxx

	const char *text; // option title
	const char *tooltip; // description of option used by K_MenuTooltips
	const char *patch; // image of option used by K_MenuPreviews

	void *itemaction; // FIXME: should be itemaction_t

	// extra variables
	UINT8 mvar1;
	UINT8 mvar2;
} menuitem_t;

typedef struct menu_s
{
	INT16          numitems;           // # of menu items
	struct menu_s *prevMenu;           // previous menu
	INT16          lastOn;             // last item user was on in menu
	menuitem_t    *menuitems;          // menu items
	INT16          x, y;               // x, y of menu
	INT16          transitionID;       // only transition if IDs match
	INT16          transitionTics;     // tics for transitions out
	void         (*drawroutine)(void); // draw routine
	void         (*tickroutine)(void); // ticker routine
	boolean      (*quitroutine)(void); // called before quit a menu return true if we can
} menu_t;

typedef enum
{
	MM_NOTHING = 0, // is just displayed until the user do someting
	MM_YESNO,       // routine is called with only 'y' or 'n' in param
	MM_EVENTHANDLER // the same of above but without 'y' or 'n' restriction
	                // and routine is void routine(event_t *) (ex: set control)
} menumessagetype_t;

// ===========
// PROTOTYPING
// ===========

// K_MENUDEF.C
extern menuitem_t MainMenu[];
extern menu_t MainDef;

typedef enum
{
	play = 0,
	extra,
	options,
	quitkart
} main_e;

extern menuitem_t PLAY_CharSelect[];
extern menu_t PLAY_CharSelectDef;

extern menuitem_t PLAY_MainMenu[];
extern menu_t PLAY_MainDef;

extern menuitem_t PLAY_Gamemodes[];
extern menu_t PLAY_GamemodesDef;

extern menuitem_t PLAY_RaceGamemodesMenu[];
extern menu_t PLAY_RaceGamemodesDef;

extern menuitem_t PLAY_CupSelect[];
extern menu_t PLAY_CupSelectDef;

extern menuitem_t PLAY_LevelSelect[];
extern menu_t PLAY_LevelSelectDef;

extern menuitem_t PLAY_TimeAttack[];
extern menu_t PLAY_TimeAttackDef;

extern menuitem_t PLAY_BattleGamemodesMenu[];
extern menu_t PLAY_BattleGamemodesDef;

extern menuitem_t PAUSE_PlaybackMenu[];
extern menu_t PAUSE_PlaybackMenuDef;

typedef enum
{
	playback_hide,
	playback_rewind,
	playback_pause,
	playback_fastforward,
	playback_backframe,
	playback_resume,
	playback_advanceframe,
	playback_viewcount,
	playback_view1,
	playback_view2,
	playback_view3,
	playback_view4,
	playback_quit
} playback_e;

// K_MENUFUNC.C

extern menu_t *currentMenu;
extern char dummystaffname[22];

extern INT16 itemOn; // menu item skull is on, Hack by Tails 09-18-2002
extern INT16 skullAnimCounter; // skull animation counter

extern struct menutransition_s {
	INT16 tics;
	INT16 dest;
	struct menu_s *startmenu;
	struct menu_s *endmenu;
	boolean in;
} menutransition;

extern boolean menuwipe;

extern consvar_t cv_showfocuslost;
extern consvar_t cv_newgametype, cv_nextmap, cv_chooseskin, cv_serversort;
extern CV_PossibleValue_t gametype_cons_t[];

void Moviemode_mode_Onchange(void);
void Screenshot_option_Onchange(void);
void Addons_option_Onchange(void);

void M_SortServerList(void);

boolean M_Responder(event_t *ev);
void M_StartControlPanel(void);
void M_ClearMenus(boolean callexitmenufunc);
void M_SelectableClearMenus(INT32 choice);
void M_SetupNextMenu(menu_t *menudef, boolean nofade);
void M_GoBack(INT32 choice);
void M_Ticker(void);
void M_Init(void);

menu_t MessageDef;
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype);
void M_StopMessage(INT32 choice);

void M_HandleImageDef(INT32 choice);

void M_QuitResponse(INT32 ch);
void M_QuitSRB2(INT32 choice);

// If you want to waste a bunch of memory for a limit no one will hit, feel free to boost this to MAXSKINS :P
// I figure this will be enough clone characters to fit onto the character select.
// (If someone runs into it after release I'll probably boost it, though.)
#define MAXCLONES MAXSKINS/16

extern struct setup_chargrid_s {
	SINT8 skinlist[MAXCLONES];
	UINT8 numskins;
} setup_chargrid[9][9];

#define CSSTEP_NONE 0
#define CSSTEP_CHARS 1
#define CSSTEP_ALTS 2
#define CSSTEP_COLORS 3
#define CSSTEP_READY 4

typedef struct setup_player_s
{
	SINT8 gridx, gridy;
	SINT8 skin;
	SINT8 clonenum;
	SINT8 rotate;
	UINT8 delay;
	UINT8 color;
	UINT8 mdepth;
} setup_player_t;

extern setup_player_t setup_player[MAXSPLITSCREENPLAYERS];

extern UINT8 setup_numplayers;
extern tic_t setup_animcounter;

#define CSROTATETICS 6

// The selection spawns 3 explosions in 4 directions, and there's 4 players -- 3 * 4 * 4 = 48
#define CSEXPLOSIONS 48

extern struct setup_explosions_s {
	UINT8 x, y;
	UINT8 tics;
	UINT8 color;
} setup_explosions[CSEXPLOSIONS];

typedef enum
{
	SPLITCV_SKIN = 0,
	SPLITCV_COLOR,
	SPLITCV_NAME,
	SPLITCV_MAX
} splitscreencvars_t;
consvar_t *setup_playercvars[MAXSPLITSCREENPLAYERS][SPLITCV_MAX];

void M_CharacterSelectInit(INT32 choice);
void M_CharacterSelectHandler(INT32 choice);
void M_CharacterSelectTick(void);
boolean M_CharacterSelectQuit(void);

#define CUPS_COLUMNS 7
#define CUPS_ROWS 2
#define CUPS_MAPSPERCUP 5
#define CUPS_MAX (NUMMAPS / CUPS_MAPSPERCUP)
#define CUPS_PAGES (CUPS_MAX / (CUPS_COLUMNS * CUPS_ROWS))

extern struct levellist_cupgrid_s {
	UINT8 numcups;
	SINT8 x, y;
	SINT8 pageno;
	tic_t previewanim;
} levellist_cupgrid;

extern struct levellist_scroll_s {
	SINT8 cupid;
	SINT8 cursor;
	UINT16 y;
	UINT16 dest;
} levellist_scroll;

boolean M_CanShowLevelInList(INT32 mapnum, INT32 gt);
void M_LevelSelectInit(INT32 choice);

void M_CupSelectHandler(INT32 choice);
void M_CupSelectTick(void);

void M_LevelSelectHandler(INT32 choice);
void M_LevelSelectTick(void);

void M_EndModeAttackRun(void);
void M_SetPlaybackMenuPointer(void);
void M_PlaybackRewind(INT32 choice);
void M_PlaybackPause(INT32 choice);
void M_PlaybackFastForward(INT32 choice);
void M_PlaybackAdvance(INT32 choice);
void M_PlaybackSetViews(INT32 choice);
void M_PlaybackAdjustView(INT32 choice);
void M_PlaybackQuit(INT32 choice);

void M_ReplayHut(INT32 choice);

// M_MENUDRAW.C

void M_Drawer(void);
void M_DrawGenericMenu(void);
void M_DrawKartGamemodeMenu(void);
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines);
void M_DrawMessageMenu(void);
void M_DrawImageDef(void);

void M_DrawCharacterSelect(void);

void M_DrawCupSelect(void);
void M_DrawLevelSelect(void);
void M_DrawTimeAttack(void);

void M_DrawPlaybackMenu(void);

// These defines make it a little easier to make menus
#define DEFAULTMENUSTYLE(source, prev, x, y)\
{\
	sizeof(source) / sizeof(menuitem_t),\
	prev,\
	0,\
	source,\
	x, y,\
	0, 0,\
	M_DrawGenericMenu,\
	NULL,\
	NULL\
}


#define KARTGAMEMODEMENU(source, prev)\
{\
	sizeof(source) / sizeof(menuitem_t),\
	prev,\
	0,\
	source,\
	0, 0,\
	1, 10,\
	M_DrawKartGamemodeMenu,\
	NULL,\
	NULL\
}

#endif //__K_MENU__
