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
#include "g_demo.h"	//menudemo_t
#include "k_profiles.h"	// profile data & functions
#include "g_input.h"	// gc_
#include "i_threads.h"
#include "mserv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVERLISTDEBUG

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

#ifdef HAVE_THREADS
extern I_mutex k_menu_mutex;
#endif

// for server threads etc.
typedef enum
{
	M_NOT_WAITING,

	M_WAITING_VERSION,
	M_WAITING_SERVERS,
}
M_waiting_mode_t;

extern M_waiting_mode_t m_waiting_mode;

typedef union
{
	menu_t *submenu;      // IT_SUBMENU
	consvar_t *cvar;             // IT_CVAR
	void (*routine)(INT32 choice); // IT_CALL, IT_KEYHANDLER, IT_ARROWS
} itemaction_t;

// Player Setup menu colors linked list
struct menucolor_t {
	menucolor_t *next;
	menucolor_t *prev;
	UINT16 color;
};

extern menucolor_t *menucolorhead, *menucolortail;

extern CV_PossibleValue_t gametype_cons_t[];

//
// MENU TYPEDEFS
//

struct menuitem_t
{
	UINT16 status; // show IT_xxx

	const char *text; // option title
	const char *tooltip; // description of option used by K_MenuTooltips
	const char *patch; // image of option used by K_MenuPreviews

	itemaction_t itemaction;

	// extra variables
	INT32 mvar1;
	INT32 mvar2;
};

struct menu_t
{
	INT16          numitems;           // # of menu items
	menu_t        *prevMenu;           // previous menu

	INT16          lastOn;             // last item user was on in menu
	menuitem_t    *menuitems;          // menu items

	INT16          x, y;               // x, y of menu
	INT16 		   extra1, extra2;	   // Can be whatever really! Options menu uses extra1 for bg colour.

	INT16          transitionID;       // only transition if IDs match
	INT16          transitionTics;     // tics for transitions out

	void         (*drawroutine)(void); // draw routine
	void         (*tickroutine)(void); // ticker routine
	void         (*initroutine)(void); // called when starting a new menu
	boolean      (*quitroutine)(void); // called before quit a menu return true if we can
	boolean		 (*inputroutine)(INT32); // if set, called every frame in the input handler. Returning true overwrites normal input handling.
};

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

extern menuitem_t PLAY_GamemodesMenu[];
extern menu_t PLAY_GamemodesDef;

extern menuitem_t PLAY_RaceGamemodesMenu[];
extern menu_t PLAY_RaceGamemodesDef;

typedef enum
{
	drace_gpdifficulty = 0,
	drace_mrkartspeed,
	drace_mrcpu,
	drace_mrracers,
	drace_encore,
	drace_boxend,
	drace_cupselect = drace_boxend,
	drace_mapselect,
	drace_back
} drace_e;

extern menuitem_t PLAY_RaceDifficulty[];
extern menu_t PLAY_RaceDifficultyDef;

extern menuitem_t PLAY_CupSelect[];
extern menu_t PLAY_CupSelectDef;

extern menuitem_t PLAY_LevelSelect[];
extern menu_t PLAY_LevelSelectDef;

extern menuitem_t PLAY_TimeAttack[];
extern menu_t PLAY_TimeAttackDef;

typedef enum
{
	ta_replay = 0,
	ta_guest,
	ta_ghosts,
	ta_spacer,
	ta_start,
} ta_e;

extern menuitem_t PLAY_TAReplay[];
extern menu_t PLAY_TAReplayDef;

extern menuitem_t PLAY_TAReplayGuest[];
extern menu_t PLAY_TAReplayGuestDef;

extern menuitem_t PLAY_TAGhosts[];
extern menu_t PLAY_TAGhostsDef;

extern menuitem_t PLAY_MP_OptSelect[];
extern menu_t PLAY_MP_OptSelectDef;

typedef enum
{
	mhost_sname = 0,
	mhost_public,
	mhost_maxp,
	mhost_gametype,
	mhost_go,
} mhost_e;

extern menuitem_t PLAY_MP_Host[];
extern menu_t PLAY_MP_HostDef;

extern menuitem_t PLAY_MP_JoinIP[];
extern menu_t PLAY_MP_JoinIPDef;

extern menuitem_t PLAY_MP_RoomSelect[];
extern menu_t PLAY_MP_RoomSelectDef;

extern menuitem_t PLAY_MP_ServerBrowser[];
extern menu_t PLAY_MP_ServerBrowserDef;

extern menuitem_t PLAY_BattleGamemodesMenu[];
extern menu_t PLAY_BattleGamemodesDef;

// OPTIONS
extern menuitem_t OPTIONS_Main[];
extern menu_t OPTIONS_MainDef;

// We'll need this since we're gonna have to dynamically enable and disable options depending on which state we're in.
typedef enum
{
	mopt_profiles = 0,
	mopt_video,
	mopt_sound,
	mopt_hud,
	mopt_gameplay,
	mopt_server,
	mopt_data,
	mopt_manual,
} mopt_e;

typedef enum
{
	dopt_screenshot = 0,
	dopt_addon,
	dopt_replay,
#ifdef HAVE_DISCORDRPC
	dopt_discord,
#endif
	dopt_spacer,
	dopt_erase,
} dopt_e;

extern menuitem_t OPTIONS_Profiles[];
extern menu_t OPTIONS_ProfilesDef;

// Separate menu to avoid spaghetti code etc.
extern menuitem_t MAIN_Profiles[];
extern menu_t MAIN_ProfilesDef;

typedef enum
{
	popt_profilename = 0,
	popt_profilepname,
	popt_char,
	popt_controls,
	popt_confirm,
} popt_e;

extern menuitem_t OPTIONS_EditProfile[];
extern menu_t OPTIONS_EditProfileDef;

extern menuitem_t OPTIONS_ProfileControls[];
extern menu_t OPTIONS_ProfileControlsDef;

extern menuitem_t OPTIONS_Video[];
extern menu_t OPTIONS_VideoDef;

extern menuitem_t OPTIONS_VideoModes[];
extern menu_t OPTIONS_VideoModesDef;

#ifdef HWRENDER
extern menuitem_t OPTIONS_VideoOGL[];
extern menu_t OPTIONS_VideoOGLDef;
#endif

extern menuitem_t OPTIONS_Sound[];
extern menu_t OPTIONS_SoundDef;

extern menuitem_t OPTIONS_HUD[];
extern menu_t OPTIONS_HUDDef;

extern menuitem_t OPTIONS_HUDOnline[];
extern menu_t OPTIONS_HUDOnlineDef;

typedef enum
{
	gopt_gamespeed = 0,
	gopt_baselapcount,
	gopt_frantic,
	gopt_encore,
	gopt_exitcountdown,
	gopt_spacer1,
	gopt_timelimit,
	gopt_startingbumpers,
	gopt_karmacomeback,
	gopt_spacer2,
	gopt_itemtoggles
} gopt_e;

extern menuitem_t OPTIONS_Gameplay[];
extern menu_t OPTIONS_GameplayDef;

extern menuitem_t OPTIONS_GameplayItems[];
extern menu_t OPTIONS_GameplayItemsDef;

extern menuitem_t OPTIONS_Server[];
extern menu_t OPTIONS_ServerDef;

extern menuitem_t OPTIONS_ServerAdvanced[];
extern menu_t OPTIONS_ServerAdvancedDef;

extern menuitem_t OPTIONS_Data[];
extern menu_t OPTIONS_DataDef;

extern menuitem_t OPTIONS_DataScreenshot[];
extern menu_t OPTIONS_DataScreenshotDef;

extern menuitem_t OPTIONS_DataAddon[];
extern menu_t OPTIONS_DataAddonDef;

extern menuitem_t OPTIONS_DataReplay[];
extern menu_t OPTIONS_DataReplayDef;

#ifdef HAVE_DISCORDRPC
extern menuitem_t OPTIONS_DataDiscord[];
extern menu_t OPTIONS_DataDiscordDef;
#endif

extern menuitem_t OPTIONS_DataErase[];
extern menu_t OPTIONS_DataEraseDef;

extern menuitem_t OPTIONS_DataProfileErase[];
extern menu_t OPTIONS_DataProfileEraseDef;

// EXTRAS
extern menuitem_t EXTRAS_Main[];
extern menu_t EXTRAS_MainDef;

extern menuitem_t EXTRAS_ReplayHut[];
extern menu_t EXTRAS_ReplayHutDef;

extern menuitem_t EXTRAS_ReplayStart[];
extern menu_t EXTRAS_ReplayStartDef;

// PAUSE
extern menuitem_t PAUSE_Main[];
extern menu_t PAUSE_MainDef;

// EXTRAS
extern menuitem_t MISC_Manual[];
extern menu_t MISC_ManualDef;

extern menuitem_t MISC_Addons[];
extern menu_t MISC_AddonsDef;

extern menuitem_t MISC_ChallengesStatsDummyMenu[];
extern menu_t MISC_ChallengesDef;
extern menu_t MISC_StatisticsDef;

// We'll need this since we're gonna have to dynamically enable and disable options depending on which state we're in.
typedef enum
{
	mpause_addons = 0,
	mpause_switchmap,
	mpause_restartmap,
	mpause_tryagain,
#ifdef HAVE_DISCORDRPC
	mpause_discordrequests,
#endif

	mpause_continue,
	mpause_spectate,
	mpause_entergame,
	mpause_canceljoin,
	mpause_spectatemenu,
	mpause_psetup,
	mpause_options,

	mpause_title,
} mpause_e;

extern menuitem_t PAUSE_GamemodesMenu[];
extern menu_t PAUSE_GamemodesDef;

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

// Moviemode menu updating
void Moviemode_option_Onchange(void);

extern menu_t *currentMenu;
extern char dummystaffname[22];

extern INT16 itemOn; // menu item skull is on, Hack by Tails 09-18-2002
extern INT16 skullAnimCounter; // skull animation counter

extern INT32 menuKey; // keyboard key pressed for menu

extern INT16 virtualKeyboard[5][13];
extern INT16 shift_virtualKeyboard[5][13];

extern struct menutyping_s
{
	boolean active;				// Active
	boolean menutypingclose;	// Closing
	boolean keyboardtyping;		// If true, all keystrokes are treated as typing (ignores MBT_A etc). This is unset if you try moving the cursor on the virtual keyboard or use your controller
	SINT8 menutypingfade;		// fade in and out

	SINT8 keyboardx;
	SINT8 keyboardy;
	boolean keyboardcapslock;
	boolean keyboardshift;

} menutyping;
// While typing, we'll have a fade strongly darken the screen to overlay the typing menu instead

typedef enum
{
	MA_NONE = 0,
	MA_YES,
	MA_NO
} manswer_e;

#define MAXMENUMESSAGE 256
extern struct menumessage_s
{
	boolean active;
	INT32 flags;		// MM_
	char message[MAXMENUMESSAGE];	// message to display

	SINT8 fadetimer;	// opening
	INT32 x;
	INT32 y;
	INT32 m;

	void (*routine)(INT32 choice);	// Normal routine
	void (*eroutine)(event_t *ev);	// Event routine	(MM_EVENTHANDLER)
} menumessage;

void M_HandleMenuMessage(void);

#define MENUDELAYTIME 7
#define MENUMINDELAY 2

typedef enum
{
	MBT_A = 1,
	MBT_B = 1<<1,
	MBT_C = 1<<2,
	MBT_X = 1<<3,
	MBT_Y = 1<<4,
	MBT_Z = 1<<5,
	MBT_L = 1<<6,
	MBT_R = 1<<7,
	MBT_START = 1<<8
} menuButtonCode_t;

struct menucmd_t
{
	SINT8 dpad_ud; // up / down dpad
	SINT8 dpad_lr; // left / right
	UINT32 buttons; // buttons
	UINT32 buttonsHeld; // prev frame's buttons
	UINT16 delay; // menu wait
	UINT32 delayCount; // num times ya did menu wait (to make the wait shorter each time)
};

extern menucmd_t menucmd[MAXSPLITSCREENPLAYERS];

extern struct menutransition_s {
	INT16 tics;
	INT16 dest;
	menu_t *startmenu;
	menu_t *endmenu;
	boolean in;
} menutransition;

extern boolean menuwipe;

extern consvar_t cv_showfocuslost;
extern consvar_t cv_chooseskin, cv_serversort, cv_menujam_update;

void M_SetMenuDelay(UINT8 i);

void Moviemode_mode_Onchange(void);
void Screenshot_option_Onchange(void);
void Addons_option_Onchange(void);

void M_SortServerList(void);

void M_MapMenuControls(event_t *ev);
boolean M_Responder(event_t *ev);
boolean M_MenuButtonPressed(UINT8 pid, UINT32 bt);
boolean M_MenuButtonHeld(UINT8 pid, UINT32 bt);
void M_StartControlPanel(void);
void M_ClearMenus(boolean callexitmenufunc);
void M_SelectableClearMenus(INT32 choice);
void M_SetupNextMenu(menu_t *menudef, boolean nofade);
void M_GoBack(INT32 choice);
void M_Ticker(void);
void M_Init(void);

extern menu_t MessageDef;
void M_StartMessage(const char *string, void *routine, menumessagetype_t itemtype);
void M_StopMessage(INT32 choice);
void M_DrawMenuMessage(void);

void M_QuitResponse(INT32 ch);
void M_QuitSRB2(INT32 choice);

extern UINT16 nummenucolors;
void M_AddMenuColor(UINT16 color);
void M_MoveColorBefore(UINT16 color, UINT16 targ);
void M_MoveColorAfter(UINT16 color, UINT16 targ);
UINT16 M_GetColorBefore(UINT16 color, UINT16 amount, boolean follower);
UINT16 M_GetColorAfter(UINT16 color, UINT16 amount, boolean follower);
void M_InitPlayerSetupColors(void);
void M_FreePlayerSetupColors(void);

// If you want to waste a bunch of memory for a limit no one will hit, feel free to boost this to MAXSKINS :P
// I figure this will be enough clone characters to fit onto one grid space.
#define MAXCLONES MAXSKINS/8

extern struct setup_chargrid_s {
	INT16 skinlist[MAXCLONES];
	UINT8 numskins;
} setup_chargrid[9][9];

extern UINT8 setup_followercategories[MAXFOLLOWERCATEGORIES][2];
extern UINT8 setup_numfollowercategories;

typedef enum
{
	CSSTEP_NONE = 0,
	CSSTEP_PROFILE,
	CSSTEP_ASKCHANGES,
	CSSTEP_CHARS,
	CSSTEP_ALTS,
	CSSTEP_COLORS,
	CSSTEP_FOLLOWERCATEGORY,
	CSSTEP_FOLLOWER,
	CSSTEP_FOLLOWERCOLORS,
	CSSTEP_READY
} setup_mdepth_t;

struct setup_player_t
{
	SINT8 gridx, gridy;
	UINT8 profilen;
	INT16 skin;
	SINT8 clonenum;
	SINT8 rotate;
	UINT8 delay;
	UINT16 color;
	UINT8 mdepth;
	boolean hitlag;

	// Hack, save player 1's original device even if they init charsel with keyboard.
	// If they play ALONE, allow them to retain that original device, otherwise, ignore this.
	// We can allow them to retain the device with no consequence as when P1 is alone, they have exclusive keyboard fallback options.
	UINT8 ponedevice;

	UINT8 changeselect;

	INT16 followercategory;
	INT16 followern;
	UINT16 followercolor;
	tic_t follower_tics;
	tic_t follower_timer;
	UINT8 follower_frame;
	state_t *follower_state;
};

extern setup_player_t setup_player[MAXSPLITSCREENPLAYERS];

extern UINT8 setup_numplayers;
extern tic_t setup_animcounter;

// for charsel pages.
extern UINT8 setup_page;
extern UINT8 setup_maxpage;

#define CSROTATETICS 6

// The selection spawns 3 explosions in 4 directions, and there's 4 players -- 3 * 4 * 4 = 48
#define CSEXPLOSIONS 48

extern struct setup_explosions_s {
	INT16 x, y;
	UINT8 tics;
	UINT16 color;
} setup_explosions[CSEXPLOSIONS];

typedef enum
{
	SPLITCV_SKIN = 0,
	SPLITCV_COLOR,
	SPLITCV_NAME,
	SPLITCV_MAX
} splitscreencvars_t;
extern consvar_t *setup_playercvars[MAXSPLITSCREENPLAYERS][SPLITCV_MAX];

void M_CharacterSelectInit(void);
void M_CharacterSelect(INT32 choice);
boolean M_CharacterSelectHandler(INT32 choice);
void M_CharacterSelectTick(void);
boolean M_CharacterSelectQuit(void);

void M_SetupGametypeMenu(INT32 choice);
void M_SetupRaceMenu(INT32 choice);

#define CUPMENU_COLUMNS 7
#define CUPMENU_ROWS 2
#define CUPMENU_CURSORID (cupgrid.x + (cupgrid.y * CUPMENU_COLUMNS) + (cupgrid.pageno * (CUPMENU_COLUMNS * CUPMENU_ROWS)))

extern struct cupgrid_s {
	SINT8 x, y;
	size_t pageno;
	cupheader_t **builtgrid;
	size_t numpages;
	size_t cappages;
	tic_t previewanim;
	boolean grandprix; 	// Setup grand prix server after picking
	boolean netgame;	// Start the game in an actual server
} cupgrid;

typedef struct levelsearch_s {
	UINT32 typeoflevel;
	cupheader_t *cup;
	boolean timeattack;
	boolean cupmode;
	boolean checklocked;
} levelsearch_t;

extern struct levellist_s {
	SINT8 cursor;
	UINT16 y;
	UINT16 dest;
	INT16 choosemap;
	UINT8 newgametype;
	levelsearch_t levelsearch;
	boolean netgame;	// Start the game in an actual server
} levellist;

boolean M_CanShowLevelInList(INT16 mapnum, levelsearch_t *levelsearch);
UINT16 M_CountLevelsToShowInList(levelsearch_t *levelsearch);
UINT16 M_GetFirstLevelInList(UINT8 *i, levelsearch_t *levelsearch);
UINT16 M_GetNextLevelInList(UINT16 mapnum, UINT8 *i, levelsearch_t *levelsearch);

void M_LevelSelectInit(INT32 choice);
void M_CupSelectHandler(INT32 choice);
void M_CupSelectTick(void);
void M_LevelSelectHandler(INT32 choice);
void M_LevelSelectTick(void);

// dummy consvars for GP & match race setup
extern consvar_t cv_dummygpdifficulty;
extern consvar_t cv_dummykartspeed;
extern consvar_t cv_dummygpencore;
extern consvar_t cv_dummymatchbots;

void M_SetupDifficultySelect(INT32 choice);
void M_DifficultySelectInputs(INT32 choice);

// Multiplayer menu stuff

// Keep track of multiplayer menu related data
// We'll add more stuff here as we need em...

#define SERVERSPERPAGE 8
#define SERVERSPACE 18

extern struct mpmenu_s {
	UINT8 modechoice;
	INT16 modewinextend[3][3];	// Used to "extend" the options in the mode select screen.
								// format for each option: {extended?, max extension, # lines extended}
								// See M_OptSelectTick, it'll make more sense there. Sorry if this is a bit of a mess!

	UINT8 room;
	tic_t ticker;

	UINT8 servernum;
	UINT8 scrolln;
	// max scrolln is always going to be serverlistcount-4 as we can display 8 servers at any time and we start scrolling at half.

	INT16 slide;

} mpmenu;

// Time Attack
void M_StartTimeAttack(INT32 choice);
void M_ReplayTimeAttack(INT32 choice);
void M_HandleStaffReplay(INT32 choice);
void M_SetGuestReplay(INT32 choice);

// MP selection
void M_MPOptSelect(INT32 choice);
void M_MPOptSelectInit(INT32 choice);
void M_MPOptSelectTick(void);
boolean M_MPResetOpts(void);
extern consvar_t cv_dummygametype;		// lazy hack to allow us to select the GT on the server host submenu
extern consvar_t cv_dummyip;			// I HAVE
								// HAVE YOUR IP ADDRESS (This just the hack Cvar we'll type into and then it apends itself to "connect" in the console for IP join)

// MP Host
void M_MPHostInit(INT32 choice);
void M_MPSetupNetgameMapSelect(INT32 choice);

// MP join by IP
void M_MPJoinIPInit(INT32 choice);
boolean M_JoinIPInputs(INT32 ch);
void M_JoinIP(const char *ipa);

// Server browser room selection
void M_MPRoomSelect(INT32 choice);
void M_MPRoomSelectTick(void);
void M_MPRoomSelectInit(INT32 choice);

// Server browser hell with threads...
void M_SetWaitingMode(int mode);
int M_GetWaitingMode(void);

void M_MPServerBrowserTick(void);
boolean M_ServerBrowserInputs(INT32 ch);

#ifdef MASTERSERVER
#ifdef HAVE_THREADS

void Spawn_masterserver_thread (const char *name, void (*thread)(int*));
int Same_instance (int id);

#endif /*HAVE_THREADS*/

void Fetch_servers_thread (int *id);

#endif /*MASTERSERVER*/

void M_RefreshServers(INT32 choice);
void M_ServersMenu(INT32 choice);

// for debugging purposes...
#ifdef SERVERLISTDEBUG
void M_ServerListFillDebug(void);
#endif

// Options menu:

// mode descriptions for video mode menu
struct modedesc_t
{
	INT32 modenum; // video mode number in the vidmodes list
	const char *desc;  // XXXxYYY
	UINT8 goodratio; // aspect correct if 1
};


#define MAXCOLUMNMODES   12     //max modes displayed in one column
#define MAXMODEDESCS     (MAXCOLUMNMODES*3)
// Keep track of some options properties
extern struct optionsmenu_s {

	tic_t ticker;		// How long the menu's been open for
	INT16 offset;		// To make the icons move smoothly when we transition!

	tic_t buttflash;	// Button flashing before transitionning to the new submenu.

	// For moving the button when we get into a submenu. it's smooth and cool! (normal x/y and target x/y.)
	// this is only used during menu transitions.

	// For profiles specifically, this moves the card around since we don't have the rest of the menu displayed in that case.
	INT16 optx;
	INT16 opty;
	INT16 toptx;
	INT16 topty;

	// profile garbage
	boolean profilemenu;		// In profile menu. (Used to know when to get the "PROFILE SETUP" button away....
	boolean resetprofilemenu;	// Reset button behaviour when exiting
	SINT8 profilen;				// # of the selected profile.

	boolean resetprofile;		// After going back from the edit menu, this tells the profile select menu to kill the profile data after the transition.
	profile_t *profile;			// Pointer to the profile we're editing

	INT32 tempcontrols[num_gamecontrols][MAXINPUTMAPPING];
	// Temporary buffer where we're gonna store game controls.
	// This is only applied to the profile when you exit out of the controls menu.

	INT16 controlscroll;		// scrolling for the control menu....
	UINT8 bindcontrol;			// 0: not binding, 1: binding control #1, 2: binding control #2
	INT16 bindtimer;			// Timer until binding is cancelled (5s)

	INT16 trycontroller;		// Starts at 3*TICRATE, holding B lowers this, when at 0, cancel controller try mode.

	// Used for horrible axis shenanigans
	INT32 lastkey;
	tic_t keyheldfor;

	// controller coords...
	// Works the same as (t)opt
	INT16 contx;
	INT16 conty;
	INT16 tcontx;
	INT16 tconty;

	// for video mode testing:
	INT32 vidm_testingmode;
	INT32 vidm_previousmode;
	INT32 vidm_selected;
	INT32 vidm_nummodes;
	INT32 vidm_column_size;

	modedesc_t modedescs[MAXMODEDESCS];

	UINT8 erasecontext;

	UINT8 eraseprofilen;

	// background:
	INT16 currcolour;
	INT16 lastcolour;
	tic_t fade;
} optionsmenu;

extern INT16 controlleroffsets[][2];

extern consvar_t cv_dummyprofilename;
extern consvar_t cv_dummyprofileplayername;
extern consvar_t cv_dummyprofilekickstart;

void M_ResetOptions(void);
void M_InitOptions(INT32 choice); // necessary for multiplayer since there's some options we won't want to access
void M_OptionsTick(void);
boolean M_OptionsInputs(INT32 ch);
boolean M_OptionsQuit(void);	// resets buttons when you quit the options.
void M_OptionsChangeBGColour(INT16 newcolour);	// changes the background colour for options

void M_HandleItemToggles(INT32 choice);	// For item toggling
void M_EraseData(INT32 choice);	// For data erasing
void M_CheckProfileData(INT32 choice);	// check if we have profiles.

// profile selection menu
void M_ProfileSelectInit(INT32 choice);
void M_HandleProfileSelect(INT32 ch);

// profile edition
void M_HandleProfileEdit(void);
void M_ProfileDeviceSelect(INT32 choice);
void M_ConfirmProfile(INT32 choice);
boolean M_ProfileEditInputs(INT32 ch);

void M_HandleProfileControls(void);
boolean M_ProfileControlsInputs(INT32 ch);
void M_ProfileSetControl(INT32 ch);

void M_MapProfileControl(event_t *ev);
void M_ProfileTryController(INT32 choice);
void M_ProfileControlsConfirm(INT32 choice);

// video modes menu (resolution)
void M_VideoModeMenu(INT32 choice);
void M_HandleVideoModes(INT32 ch);

// data stuff
void M_HandleProfileErase(INT32 choice);

// Draws the EGGA CHANNEL background.
void M_DrawEggaChannel(void);

// Extras menu:
#define DF_ENCORE       0x40

extern struct extrasmenu_s {

	tic_t ticker;		// How long the menu's been open for
	INT16 offset;		// To make the icons move smoothly when we transition!

	// For moving the button when we get into a submenu. it's smooth and cool! (normal x/y and target x/y.)
	// this is only used during menu transitions. (and will probably remain unused until we get the statistics menu
	INT16 extx;
	INT16 exty;
	INT16 textx;
	INT16 texty;


	// The replay vars...... oh no......
	menudemo_t *demolist;

	INT16 replayScrollTitle;
	SINT8 replayScrollDelay;
	SINT8 replayScrollDir;



} extrasmenu;

void M_InitExtras(INT32 choice); // init for the struct
void M_ExtrasTick(void);
boolean M_ExtrasInputs(INT32 ch);
boolean M_ExtrasQuit(void);	// resets buttons when you quit

// Extras: Replay Hut
void M_HandleReplayHutList(INT32 choice);
boolean M_QuitReplayHut(void);
void M_HutStartReplay(INT32 choice);
void M_PrepReplayList(void);


// Pause menu:

// Keep track of some pause menu data for visual goodness.
extern struct pausemenu_s {

	tic_t ticker;		// How long the menu's been open for
	INT16 offset;		// To make the icons move smoothly when we transition!

	INT16 openoffset;	// Used when you open / close the menu to slide everything in.
	boolean closing;	// When this is set, the open offset goes backwards to close the menu smoothly.
} pausemenu;

void M_OpenPauseMenu(void);
void M_QuitPauseMenu(INT32 choice);
boolean M_PauseInputs(INT32 ch);
void M_PauseTick(void);

extern consvar_t cv_dummymenuplayer;
extern consvar_t cv_dummyspectator;

// Bunch of funny functions for the pause menu...~
void M_RestartMap(INT32 choice);				// Restart level (MP)
void M_TryAgain(INT32 choice);					// Try again (SP)
void M_ConfirmSpectate(INT32 choice);			// Spectate confirm when you're alone
void M_ConfirmEnterGame(INT32 choice);			// Enter game confirm when you're alone
void M_ConfirmSpectateChange(INT32 choice);		// Splitscreen spectate/play menu func
void M_EndGame(INT32 choice);					// Quitting to title

// Replay Playback

extern tic_t playback_last_menu_interaction_leveltime;

void M_EndModeAttackRun(void);
void M_SetPlaybackMenuPointer(void);
void M_PlaybackRewind(INT32 choice);
void M_PlaybackPause(INT32 choice);
void M_PlaybackFastForward(INT32 choice);
void M_PlaybackAdvance(INT32 choice);
void M_PlaybackSetViews(INT32 choice);
void M_PlaybackAdjustView(INT32 choice);
void M_PlaybackToggleFreecam(INT32 choice);
void M_PlaybackQuit(INT32 choice);

void M_ReplayHut(INT32 choice);

// Misc menus:
#define numaddonsshown 4
void M_Addons(INT32 choice);
void M_AddonsRefresh(void);
void M_HandleAddons(INT32 choice);
char *M_AddonsHeaderPath(void);
extern consvar_t cv_dummyaddonsearch;

void M_Manual(INT32 choice);
void M_HandleImageDef(INT32 choice);

// K_MENUDRAW.C

// flags for text highlights
#define highlightflags V_AQUAMAP
#define recommendedflags V_GREENMAP
#define warningflags V_GRAYMAP

void M_UpdateMenuBGImage(boolean forceReset);
void M_DrawMenuBackground(void);
void M_DrawMenuForeground(void);
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

void M_DrawRaceDifficulty(void);

// Multiplayer menu stuff
void M_DrawMPOptSelect(void);
void M_DrawMPHost(void);
void M_DrawMPJoinIP(void);
void M_DrawMPRoomSelect(void);
void M_DrawMPServerBrowser(void);

// Pause menu:
void M_DrawPause(void);

// Replay Playback
void M_DrawPlaybackMenu(void);

// Options menus:
void M_DrawOptionsMovingButton(void);	// for sick transitions...
void M_DrawOptions(void);
void M_DrawGenericOptions(void);
void M_DrawProfileSelect(void);
void M_DrawEditProfile(void);
void M_DrawProfileControls(void);
void M_DrawVideoModes(void);
void M_DrawItemToggles(void);
void M_DrawProfileErase(void);
extern tic_t shitsfree;

// Extras menu:
void M_DrawExtrasMovingButton(void);
void M_DrawExtras(void);
void M_DrawReplayHut(void);
void M_DrawReplayStartMenu(void);

// Misc menus:
#define LOCATIONSTRING1 "Visit \x83SRB2.ORG/MODS\x80 to get & make addons!"
#define LOCATIONSTRING2 "Visit \x88SRB2.ORG/MODS\x80 to get & make addons!"
void M_DrawAddons(void);

// Challenges menu:
#define UNLOCKTIME 5
#define MAXUNLOCKTIME TICRATE
#define RIGHTUNLOCKSCROLL 3
#define LEFTUNLOCKSCROLL (RIGHTUNLOCKSCROLL-1)

#define CC_TOTAL 0
#define CC_UNLOCKED 1
#define CC_TALLY 2
#define CC_ANIM 3
#define CC_MAX 4

// Keep track of some pause menu data for visual goodness.
extern struct challengesmenu_s {

	tic_t ticker;		// How long the menu's been open for
	INT16 offset;		// To make the icons move smoothly when we transition!

	UINT8 currentunlock;
	char *unlockcondition;

	tic_t unlockanim;

	SINT8 row, hilix, focusx;
	UINT8 col, hiliy;

	UINT8 *extradata;

	boolean pending;
	boolean requestnew;

	UINT8 unlockcount[CC_MAX];

	UINT8 fade;
} challengesmenu;

menu_t *M_InterruptMenuWithChallenges(menu_t *desiredmenu);
void M_Challenges(INT32 choice);
void M_DrawChallenges(void);
void M_ChallengesTick(void);
boolean M_ChallengesInputs(INT32 ch);

extern struct statisticsmenu_s {
	INT32 location;
	INT32 nummaps;
	INT32 maxscroll;
	UINT16 *maplist;
} statisticsmenu;

void M_Statistics(INT32 choice);
void M_DrawStatistics(void);
boolean M_StatisticsInputs(INT32 ch);

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
	NULL,\
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
	0, 0, \
	1, 5,\
	M_DrawKartGamemodeMenu,\
	NULL,\
	NULL,\
	NULL,\
	NULL\
}

#define IMAGEDEF(source)\
{\
	sizeof(source) / sizeof(menuitem_t),\
	NULL,\
	0,\
	source,\
	0, 0,\
	0, 0, \
	1, 5,\
	M_DrawImageDef,\
	NULL,\
	NULL,\
	NULL,\
	NULL\
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__K_MENU__
