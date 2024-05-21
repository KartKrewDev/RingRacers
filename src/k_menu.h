// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
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
#include "p_saveg.h" // savedata_cup_t
#include "k_profiles.h"	// profile data & functions
#include "g_input.h"	// gc_
#include "i_threads.h"
#include "mserv.h"

#ifdef __cplusplus
extern "C" {
#endif

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
#define IT_LINKTEXT          48     // draw an arrow beside, like IT_SUBMENU
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

extern INT16 menugametype;
void M_NextMenuGametype(UINT32 forbidden);
void M_PrevMenuGametype(UINT32 forbidden);
void M_HandleHostMenuGametype(INT32 choice);
void M_HandlePauseMenuGametype(INT32 choice);
void M_HandlePauseMenuAddons(INT32 choice);

extern UINT32 menucallvote; // not midVoteType_e to prevent #include k_zvote
extern UINT32 menuaddonoptions;
void M_HandlePauseMenuCallVote(INT32 choice);

//
// MENU TYPEDEFS
//

typedef enum
{
	MBF_UD_LR_FLIPPED		= 1,    // flip up-down and left-right axes
	MBF_SOUNDLESS		 	= 1<<1, // do not play base menu sounds
	MBF_NOLOOPENTRIES		= 1<<2, // do not loop M_NextOpt/M_PrevOpt
	MBF_DRAWBGWHILEPLAYING	= 1<<3, // run backroutine() outside of GS_MENU
} menubehaviourflags_t;

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
	INT16          behaviourflags;     // menubehaviourflags_t
	const char    *music;              // Track to play in M_PlayMenuJam. NULL for default, "." to stop

	INT16          transitionID;       // only transition if IDs match
	INT16          transitionTics;     // tics for transitions out

	void         (*drawroutine)(void); // draw routine
	void           (*bgroutine)(void); // draw routine, but, like, for the background
	void         (*tickroutine)(void); // ticker routine
	void         (*initroutine)(void); // called when starting a new menu
	boolean      (*quitroutine)(void); // called before quit a menu return true if we can
	boolean		 (*inputroutine)(INT32); // if set, called every frame in the input handler. Returning true overwrites normal input handling.
};

struct menu_anim_t
{
	tic_t start;
	INT16 dist;
};

fixed_t M_TimeFrac(tic_t tics, tic_t duration);
fixed_t M_ReverseTimeFrac(tic_t tics, tic_t duration);
fixed_t M_DueFrac(tic_t start, tic_t duration);

// FIXME: C++ template
#define M_EaseWithTransition(EasingFunc, N) \
	(menutransition.tics != menutransition.dest ? EasingFunc(menutransition.in ?\
		M_ReverseTimeFrac(menutransition.tics, menutransition.endmenu->transitionTics) :\
		M_TimeFrac(menutransition.tics, menutransition.startmenu->transitionTics), 0, N) : 0)

typedef enum
{
	MM_NOTHING = 0, // is just displayed until the user do someting
	MM_YESNO,       // routine is called with only 'y' or 'n' in param
	//MM_EVENTHANDLER // the same of above but without 'y' or 'n' restriction
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

extern menuitem_t MAIN_Goner[];
extern menu_t MAIN_GonerDef;

void M_GonerTick(void);
void M_GonerBGTick(void);
void M_GonerBGImplyPassageOfTime(void);
void M_DrawGonerBack(void);
void M_GonerProfile(INT32 choice);
void M_GonerTutorial(INT32 choice);
void M_GonerResetLooking(int type);
void M_GonerCheckLooking(void);
void M_GonerGDQ(boolean opinion);
boolean M_GonerMusicPlayable(void);

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
	drace_mritems,
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

// If you add another Time Attach submenu, remember to catch level-select.c's music/bgroutine update

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
	mp_host = 0,
	mp_browse,
	mp_directjoin,
	mp_back,
} mp_e;

typedef enum
{
	mhost_gametype = 0,
	mhost_gameplay_options,
	mhost_server_options,
	mhost_boxend,
	mhost_mapselect = mhost_boxend,
	mhost_back,

	// TODO, remove these (old code)
	mhost_sname = 0,
	mhost_public,
	mhost_maxp,
	//mhost_gametype,
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
	dopt_advanced,
	dopt_spacer1,
	dopt_replay,
	dopt_rprecord,
	dopt_rpsize,
#ifdef HAVE_DISCORDRPC
	dopt_discord,
	dopt_drp,
	dopt_drpstreamer,
	dopt_drpjoins,
	dopt_drpinvites,
#endif
	dopt_spacer2,
	dopt_erase,
} dopt_e;

typedef enum
{
	daopt_addon = 0,
	daopt_spacer1,
	daopt_replay,
	daopt_replaycons,
} daopt_e;

typedef enum
{
	sopt_volume,
	sopt_sfxvolume,
	sopt_musicvolume,
	sopt_spacer1,
	sopt_preferences,
	sopt_chatnotifs,
	sopt_charvoices,
	sopt_followhorns,
	sopt_attackmusic,
	sopt_spacer2,
	sopt_advanced,
	sopt_tabbedout,
	sopt_spacer3,
	sopt_restart,
} sopt_e;

typedef enum
{
	vaopt_spacer1,
	vaopt_drawdist,
	vaopt_weatherdist,
	vaopt_skybox,
	vaopt_parallel,
	vaopt_frameskip,
	vaopt_spacer2,
	vaopt_spacer3,
	vaopt_spacer4,
	vaopt_spacer5,
	vaopt_spacer6,
	vaopt_spacer7,
	vaopt_spacer8,
	vaopt_spacer9,
	vaopt_renderer,
	vaopt_legacygl_begin,
	vaopt_spacer10 = vaopt_legacygl_begin,
	vaopt_3dmodels,
	vaopt_shaders,
	vaopt_spacer11,
	vaopt_texturequal,
	vaopt_anisotropic,
	vaopt_spacer12,
	vaopt_billboarding,
	vaopt_perspective,
	vaopt_legacygl_end,
} vaopt_e;

extern menuitem_t OPTIONS_Profiles[];
extern menu_t OPTIONS_ProfilesDef;

// Separate menu to avoid spaghetti code etc.
extern menuitem_t MAIN_Profiles[];
extern menu_t MAIN_ProfilesDef;

typedef enum
{
	popt_profilename = 0,
	popt_profilepname,
	popt_controls,
	popt_accessibility,
	popt_char,
	popt_confirm,
} popt_e;

extern menuitem_t OPTIONS_EditProfile[];
extern menu_t OPTIONS_EditProfileDef;

void M_StartEditProfile(INT32 c);

extern menuitem_t OPTIONS_ProfileControls[];
extern menu_t OPTIONS_ProfileControlsDef;

extern menuitem_t OPTIONS_ProfileAccessibility[];
extern menu_t OPTIONS_ProfileAccessibilityDef;

extern menuitem_t OPTIONS_Video[];
extern menu_t OPTIONS_VideoDef;

extern menuitem_t OPTIONS_VideoModes[];
extern menu_t OPTIONS_VideoModesDef;

extern menuitem_t OPTIONS_VideoAdvanced[];
extern menu_t OPTIONS_VideoAdvancedDef;

extern menuitem_t OPTIONS_Sound[];
extern menu_t OPTIONS_SoundDef;

extern menuitem_t OPTIONS_HUD[];
extern menu_t OPTIONS_HUDDef;

extern menuitem_t OPTIONS_HUDOnline[];
extern menu_t OPTIONS_HUDOnlineDef;

typedef enum
{
	gopt_spacer0 = 0,
	gopt_gamespeed,
	gopt_frantic,
	gopt_encore,
	gopt_exitcountdown,
	gopt_spacer1,
	gopt_timelimit,
	gopt_pointlimit,
	gopt_startingbumpers,
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

extern menuitem_t OPTIONS_DataAdvanced[];
extern menu_t OPTIONS_DataAdvancedDef;

extern menuitem_t OPTIONS_DataAdvancedAddon[];
extern menu_t OPTIONS_DataAdvancedAddonDef;

extern menuitem_t OPTIONS_DataErase[];
extern menu_t OPTIONS_DataEraseDef;

extern menuitem_t OPTIONS_DataProfileErase[];
extern menu_t OPTIONS_DataProfileEraseDef;

// EXTRAS
extern menuitem_t EXTRAS_Main[];
extern menu_t EXTRAS_MainDef;


extern menuitem_t EXTRAS_EggTV[];
extern menu_t EXTRAS_EggTVDef;

// PAUSE
extern menuitem_t PAUSE_Main[];
extern menu_t PAUSE_MainDef;

extern menu_t PAUSE_KickHandlerDef;
extern menu_t PAUSE_CheatsDef;
extern menu_t PAUSE_AddonOptionsDef;

// EXTRAS
extern menuitem_t MISC_Manual[];
extern menu_t MISC_ManualDef;

extern menuitem_t MISC_Addons[];
extern menu_t MISC_AddonsDef;

extern menuitem_t MISC_ChallengesStatsDummyMenu[];
extern menu_t MISC_ChallengesDef;
extern menu_t MISC_StatisticsDef;

extern menu_t MISC_WrongWarpDef;

extern menuitem_t MISC_SoundTest[];
extern menu_t MISC_SoundTestDef;

#ifdef HAVE_DISCORDRPC
extern menu_t MISC_DiscordRequestsDef;
#endif

// We'll need this since we're gonna have to dynamically enable and disable options depending on which state we're in.
typedef enum
{
	mpause_addons = 0,
	mpause_stereo,
	mpause_changegametype,
	mpause_switchmap,
#ifdef HAVE_DISCORDRPC
	mpause_discordrequests,
#endif
	mpause_admin,
	mpause_callvote,

	mpause_giveup,
	mpause_restartmap,
	mpause_tryagain,

	mpause_continue,
	mpause_spectatetoggle,
	mpause_psetup,
	mpause_cheats,
	mpause_options,

	mpause_title,
} mpause_e;

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
	playback_director,
	playback_freecam,
	playback_quit
} playback_e;

// K_MENUFUNC.C

extern menu_t *currentMenu;
extern menu_t *restoreMenu;

extern consvar_t cv_dummystaff;

extern INT16 itemOn; // menu item skull is on, Hack by Tails 09-18-2002
extern INT16 skullAnimCounter; // skull animation counter

extern INT32 menuKey; // keyboard key pressed for menu

#define NUMVIRTUALKEYSINROW (10+2) // 1-9, 0, and a right-side gutter of two keys' width
extern INT16 virtualKeyboard[5][NUMVIRTUALKEYSINROW];
extern INT16 shift_virtualKeyboard[5][NUMVIRTUALKEYSINROW];

typedef const char *(*vkb_query_fn_t)(const char *replace);
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

	vkb_query_fn_t queryfn; // callback on open and close
	menu_t *dummymenu;
	size_t cachelen;
	char *cache; // cached string

} menutyping;
// While typing, we'll have a fade strongly darken the screen to overlay the typing menu instead

typedef enum
{
	MA_NONE = 0,
	MA_YES,
	MA_NO
} manswer_e;

#define MAXMENUMESSAGE 256
#define MENUMESSAGECLOSE 2
extern struct menumessage_s
{
	boolean active;
	UINT8 closing;

	INT32 flags;		// MM_
	const char *header;
	char message[MAXMENUMESSAGE];	// message to display

	SINT8 fadetimer;	// opening
	INT32 x;
	INT32 y;
	INT16 timer;

	void (*routine)(INT32 choice);	// Normal routine
	//void (*eroutine)(event_t *ev);	// Event routine	(MM_EVENTHANDLER)
	INT32 answer;

	const char *defaultstr;
	const char *confirmstr;
} menumessage;

void M_StartMessage(const char *header, const char *string, void (*routine)(INT32), menumessagetype_t itemtype, const char *confirmstr, const char *defaultstr);
boolean M_MenuMessageTick(void);
void M_HandleMenuMessage(void);
void M_StopMessage(INT32 choice);
void M_DrawMenuMessage(void);

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
	MBT_START = 1<<8,
	MBT_SCREENSHOT = 1<<9,
	MBT_STARTMOVIE = 1<<10,
	MBT_STARTLOSSLESS = 1<<11,
} menuButtonCode_t;

struct menucmd_t
{
	// Current frame's data
	SINT8 dpad_ud; // up / down dpad
	SINT8 dpad_lr; // left / right
	UINT32 buttons; // buttons

	// Previous frame's data
	SINT8 prev_dpad_ud;
	SINT8 prev_dpad_lr;
	UINT32 buttonsHeld;

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
extern consvar_t cv_chooseskin, cv_serversort, cv_menujam_update, cv_menujam;
extern consvar_t cv_autorecord;

void M_SetMenuDelay(UINT8 i);

void M_SortServerList(void);

void M_UpdateMenuCMD(UINT8 i, boolean bailrequired);
boolean M_Responder(event_t *ev);
boolean M_MenuButtonPressed(UINT8 pid, UINT32 bt);
boolean M_MenuButtonHeld(UINT8 pid, UINT32 bt);

boolean M_ChangeStringCvar(INT32 choice);
void M_ChangeCvarDirect(INT32 choice, consvar_t *cv);
boolean M_NextOpt(void);
boolean M_PrevOpt(void);

boolean M_MenuConfirmPressed(UINT8 pid);
boolean M_MenuConfirmHeld(UINT8 pid);
boolean M_MenuBackPressed(UINT8 pid);
boolean M_MenuBackHeld(UINT8 pid);
boolean M_MenuExtraPressed(UINT8 pid);
boolean M_MenuExtraHeld(UINT8 pid);

void M_StartControlPanel(void);
void M_ValidateRestoreMenu(void);
menu_t *M_SpecificMenuRestore(menu_t *torestore);
void M_ClearMenus(boolean callexitmenufunc);
void M_SelectableClearMenus(INT32 choice);
void M_SetupNextMenu(menu_t *menudef, boolean nofade);
void M_GoBack(INT32 choice);
void M_Ticker(void);
void M_Init(void);

void M_PlayMenuJam(void);

boolean M_ConsiderSealedSwapAlert(void);

void M_OpenVirtualKeyboard(size_t cachelen, vkb_query_fn_t queryfn, menu_t *dummymenu);
void M_AbortVirtualKeyboard(void);
void M_MenuTypingInput(INT32 key);
void M_SwitchVirtualKeyboard(boolean gamepad);

void M_QuitResponse(INT32 ch);
void M_QuitSRB2(INT32 choice);

UINT16 M_GetColorAfter(setup_player_colors_t *colors, UINT16 value, INT32 amount);
#define M_GetColorBefore(a, b, c) M_GetColorAfter(a, b, -c)

// If you want to waste a bunch of memory for a limit no one will hit, feel free to boost this to MAXSKINS :P
// I figure this will be enough clone characters to fit onto one grid space.
// TODO: Dynamically allocate instead, you KNOW this limit will get hit by someone eventually
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

struct setup_player_colors_t
{
	UINT16 *list;
	size_t listLen;
	size_t listCap;
};

struct setup_player_t
{
	SINT8 gridx, gridy;
	UINT8 profilen;
	menu_anim_t profilen_slide;
	INT16 skin;
	SINT8 clonenum;
	SINT8 rotate;
	UINT8 delay;
	UINT16 color;
	UINT8 mdepth;
	boolean hitlag;
	boolean showextra;

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

	setup_player_colors_t colors;
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
void M_SetupReadyExplosions(boolean charsel, UINT16 basex, UINT16 basey, UINT16 color);
boolean M_CharacterSelectForceInAction(void);
boolean M_CharacterSelectHandler(INT32 choice);
void M_CharacterSelectTick(void);
boolean M_CharacterSelectQuit(void);

void M_SetupPlayMenu(INT32 choice);
void M_SetupGametypeMenu(INT32 choice);
void M_SetupRaceMenu(INT32 choice);

#define CUPMENU_CURSORID (cupgrid.x + (cupgrid.y * CUPMENU_COLUMNS) + (cupgrid.pageno * (CUPMENU_COLUMNS * CUPMENU_ROWS)))
#define CUPMENU_SLIDETIME 3

extern struct cupgrid_s {
	SINT8 x, y;
	menu_anim_t xslide, yslide;
	size_t pageno;
	cupheader_t **builtgrid;
	size_t numpages;
	size_t cappages;
	tic_t previewanim;
	boolean cache_secondrowlocked;
} cupgrid;

typedef struct levelsearch_s {
	UINT32 typeoflevel;
	cupheader_t *cup;
	boolean timeattack;
	boolean tutorial;
	boolean grandprix;
	boolean cupmode;
	boolean checklocked;
} levelsearch_t;

#define M_LEVELLIST_SLIDETIME 4

typedef struct levellist_s {
	SINT8 cursor;
	menu_anim_t slide;
	UINT16 y;
	UINT16 choosemap;
	UINT16 mapcount;
	UINT8 newgametype;
	UINT8 guessgt;
	levelsearch_t levelsearch;
	boolean netgame;	// Start the game in an actual server
	menu_t *backMenu;
} levellist_t;

extern levellist_t levellist;
extern levellist_t restorelevellist;

extern cupheader_t dummy_lostandfound;

boolean M_CanShowLevelInList(INT16 mapnum, levelsearch_t *levelsearch);
UINT16 M_CountLevelsToShowInList(levelsearch_t *levelsearch);
UINT16 M_GetFirstLevelInList(UINT8 *i, levelsearch_t *levelsearch);
UINT16 M_GetNextLevelInList(UINT16 mapnum, UINT8 *i, levelsearch_t *levelsearch);
void M_LevelSelectScrollDest(void);
boolean M_LevelListFromGametype(INT16 gt);

void M_LevelSelectInit(INT32 choice);
void M_CupSelectHandler(INT32 choice);
void M_CupSelectTick(void);
void M_LevelSelectHandler(INT32 choice);
void M_LevelSelectTick(void);

void M_LevelSelected(INT16 add, boolean menuupdate);
boolean M_LevelSelectCupSwitch(boolean next, boolean skipones);

// dummy consvars for GP & match race setup
extern consvar_t cv_dummygpdifficulty;
extern consvar_t cv_dummykartspeed;
extern consvar_t cv_dummygpencore;
extern consvar_t cv_dummymatchbots;

extern consvar_t cv_dummyspbattack;

void M_SetupDifficultyOptions(INT32 choice);
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

void M_PleaseWait(void);
void M_PopupMasterServerRules(void);

// Time Attack
void M_PrepareTimeAttack(boolean menuupdate);
void M_StartTimeAttack(INT32 choice);
void M_ReplayTimeAttack(INT32 choice);
void M_HandleStaffReplay(INT32 choice);
void M_SetGuestReplay(INT32 choice);
void M_TimeAttackTick(void);
boolean M_EncoreAttackTogglePermitted(void);
boolean M_TimeAttackInputs (INT32 choice);

// MP selection
void M_MPOptSelect(INT32 choice);
void M_MPOptSelectInit(INT32 choice);
void M_MPOptSelectTick(void);
boolean M_MPResetOpts(void);
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
#define M_OPTIONS_OFSTIME 5
#define M_OPTIONS_BINDBEN_QUICK 106
// Keep track of some options properties
extern struct optionsmenu_s {

	tic_t ticker;			// How long the menu's been open for
	menu_anim_t offset;		// To make the icons move smoothly when we transition!
	menu_anim_t box;

	// For moving the button when we get into a submenu. it's smooth and cool! (normal x/y and target x/y.)
	// this is only used during menu transitions.

	// For profiles specifically, this moves the card around since we don't have the rest of the menu displayed in that case.
	INT16 optx;
	INT16 opty;
	INT16 toptx;
	INT16 topty;
	tic_t topt_start;

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
	INT16 bindtimer;			// Timer until binding is cancelled (5s)
	UINT16 bindben;				// Hold right timer
	UINT8 bindben_swallow;		// (bool) control is about to be cleared; (int) swallow/pose animation timer
	INT32 bindinputs[MAXINPUTMAPPING]; // Set while binding

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
extern consvar_t cv_dummyprofileautoroulette;
extern consvar_t cv_dummyprofilelitesteer;
extern consvar_t cv_dummyprofileautoring;
extern consvar_t cv_dummyprofilerumble;
extern consvar_t cv_dummyprofilefov;

void M_ResetOptions(void);
void M_InitOptions(INT32 choice); // necessary for multiplayer since there's some options we won't want to access
void M_OptionsTick(void);
boolean M_OptionsInputs(INT32 ch);
boolean M_OptionsQuit(void);	// resets buttons when you quit the options.
void M_OptionsChangeBGColour(INT16 newcolour);	// changes the background colour for options

void M_VideoOptions(INT32 choice);
void M_SoundOptions(INT32 choice);
void M_GameplayOptions(INT32 choice);
void M_ServerOptions(INT32 choice);

void M_RefreshAdvancedVideoOptions(void);

void M_HandleItemToggles(INT32 choice);	// For item toggling
void M_EraseData(INT32 choice);	// For data erasing
void M_CheckProfileData(INT32 choice);	// check if we have profiles.

// profile selection menu
void M_ProfileSelectInit(INT32 choice);
void M_FirstPickProfile(INT32 c);
void M_HandleProfileSelect(INT32 ch);

// profile edition
void M_HandleProfileEdit(void);
void M_ProfileDeviceSelect(INT32 choice);
void M_ConfirmProfile(INT32 choice);
boolean M_ProfileEditInputs(INT32 ch);

void M_HandleProfileControls(void);
boolean M_ProfileControlsInputs(INT32 ch);
void M_ProfileSetControl(INT32 ch);
void M_ProfileDefaultControls(INT32 ch);
void M_ProfileClearControls(INT32 ch);

void M_MapProfileControl(event_t *ev);
void M_ProfileTryController(INT32 choice);
void M_ProfileControlsConfirm(INT32 choice);

// video modes menu (resolution)
void M_HandleVideoModes(INT32 ch);

// data stuff
void M_HandleProfileErase(INT32 choice);

// Draws "List via" at the bottom of the screen.
void M_DrawMasterServerReminder(void);

// Draws the EGGA CHANNEL background.
void M_DrawEggaChannelAlignable(boolean centered);
void M_DrawEggaChannel(void);

// Extras menu:
// woah there pardner, if you modify this check g_demo.cpp too
#define DF_ENCORE       0x40

#define M_EXTRAS_OFSTIME 4

extern struct extrasmenu_s {

	tic_t ticker;			// How long the menu's been open for
	menu_anim_t offset;		// To make the icons move smoothly when we transition!

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

typedef enum
{
	extras_addons = 0,
	extras_challenges,
	extras_manual,
	extras_tutorial,
	extras_statistics,
	extras_eggtv,
	extras_stereo,
	extras_password,
	extras_credits,
} extras_e;

void M_InitExtras(INT32 choice); // init for the struct
void M_ExtrasTick(void);
boolean M_ExtrasInputs(INT32 ch);
boolean M_ExtrasQuit(void);	// resets buttons when you quit


// Extras: Egg TV
void M_EggTV(INT32 choice);
void M_EggTV_RefreshButtonLabels(void);


// Pause menu:

// Keep track of some pause menu data for visual goodness.
extern struct pausemenu_s {

	tic_t ticker;			// How long the menu's been open for
	menu_anim_t offset;		// To make the icons move smoothly when we transition!

	menu_anim_t openoffset;	// Used when you open / close the menu to slide everything in.
	boolean closing;		// When this is set, the open offset goes backwards to close the menu smoothly.

	UINT8 splitscreenfocusid; // This is not exclusively visual, but thog dont care. For selecting splitscreen players to individually change their spectator state.
} pausemenu;

void M_OpenPauseMenu(void);
void M_QuitPauseMenu(INT32 choice);
boolean M_PauseInputs(INT32 ch);
void M_PauseTick(void);

extern struct playerkickmenu_s {
	tic_t ticker;
	UINT8 player;
	UINT8 poke;
	boolean adminpowered;
} playerkickmenu;

void M_KickHandler(INT32 choice);

extern consvar_t cv_dummymenuplayer;
extern consvar_t cv_dummyspectator;

// Bunch of funny functions for the pause menu...~
void M_RestartMap(INT32 choice);				// Restart level (MP)
void M_TryAgain(INT32 choice);					// Try again (SP)
void M_GiveUp(INT32 choice);					// Give up (SP)
void M_HandleSpectateToggle(INT32 choice);		// Spectate confirm
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

// Misc menus:
#define numaddonsshown 4
void M_Addons(INT32 choice);
void M_AddonsRefresh(void);
void M_HandleAddons(INT32 choice);
char *M_AddonsHeaderPath(void);
extern consvar_t cv_dummyaddonsearch;
extern consvar_t cv_dummyextraspassword;

#ifdef TODONEWMANUAL
void M_Manual(INT32 choice);
#endif
void M_HandleImageDef(INT32 choice);

// K_MENUDRAW.C

// flags for text highlights
#define highlightflags V_AQUAMAP
#define recommendedflags V_GREENMAP
#define warningflags V_ORANGEMAP

#define M_ALTCOLOR V_ORANGEMAP

void M_DrawCursorHand(INT32 x, INT32 y);
void M_DrawUnderline(INT32 left, INT32 right, INT32 y);

// For some menu highlights
UINT16 M_GetCvPlayerColor(UINT8 pnum);

void M_PickMenuBGMap(void);
void M_UpdateMenuBGImage(boolean forceReset);
void M_DrawMenuBackground(void);
void M_DrawMenuForeground(void);
void M_Drawer(void);
void M_DrawGenericMenu(void);
void M_DrawKartGamemodeMenu(void);
void M_FlipKartGamemodeMenu(boolean slide);
void M_DrawHorizontalMenu(void);
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines);
void M_DrawMessageMenu(void);
void M_DrawImageDef(void);

void M_DrawCharacterSelect(void);
boolean M_DrawCharacterSprite(INT16 x, INT16 y, INT16 skin, UINT8 spr2, UINT8 rotation, UINT32 frame, INT32 addflags, UINT8 *colormap);

void M_DrawCup(cupheader_t *cup, fixed_t x, fixed_t y, INT32 lockedTic, boolean isTrophy, UINT8 placement);
void M_DrawCupSelect(void);
void M_DrawLevelSelect(void);
void M_DrawSealedBack(void);
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
void M_DrawKickHandler(void);

// Replay Playback
void M_DrawPlaybackMenu(void);

// Options menus:
void M_DrawOptionsCogs(void);
void M_DrawOptionsMovingButton(void);	// for sick transitions...
void M_DrawOptions(void);
void M_DrawGenericOptions(void);
void M_DrawProfileCard(INT32 x, INT32 y, boolean greyedout, profile_t *p);
void M_DrawProfileSelect(void);
void M_DrawEditProfileTooltips(void);
void M_DrawEditProfile(void);
void M_DrawProfileControls(void);
void M_DrawVideoModes(void);
void M_DrawItemToggles(void);
void M_DrawProfileErase(void);
extern tic_t shitsfree;

// Extras menu:
void M_DrawExtrasBack(void);
void M_DrawExtrasMovingButton(void);
void M_DrawExtras(void);

// Misc menus:
#define LOCATIONSTRING1 "Visit \x83SRB2.ORG/MODS\x80 to get & make addons!"
void M_DrawAddons(void);

// Challenges menu:
#define UNLOCKTIME 5
#define MAXUNLOCKTIME TICRATE
#define RIGHTUNLOCKSCROLL 3
#define LEFTUNLOCKSCROLL (RIGHTUNLOCKSCROLL-1)

typedef enum
{
	CMC_TOTAL = 0,
	CMC_UNLOCKED,

	CMC_KEYED,
	CMC_MAJORSKIPPED,

	CMC_PERCENT,

	CMC_MEDALID,
	CMC_MEDALBLANK,
	CMC_MEDALFILLED,

	CMC_ANIM,
	CMC_CHAOANIM,
	CMC_CHAONOPE,

	CMC_MAX,
} challengesmenucount_e;

#define TILEFLIP_MAX 16

#define CHAOHOLD_STANDARD (40) // (Close to 3*TICRATE/2 after padding, but adjusted to evenly divide by 10)
#define CHAOHOLD_MAJOR (60) //(3*CHAOHOLD_STANDARD/2)
#define CHAOHOLD_BEGIN (7)
#define CHAOHOLD_END (3)
#define CHAOHOLD_PADDING (CHAOHOLD_BEGIN + CHAOHOLD_END)

extern struct timeattackmenu_s {

	tic_t ticker;		// How long the menu's been open for
	tic_t spbflicker;	// used for SPB flicker-in

} timeattackmenu;

// Keep track of some pause menu data for visual goodness.
extern struct challengesmenu_s {

	tic_t ticker;		// How long the menu's been open for
	INT16 offset;		// To make the icons move smoothly when we transition!
	menu_anim_t move;

	UINT16 currentunlock;
	char *unlockcondition;

	tic_t unlockanim;

	INT16 row, hilix, focusx;
	UINT16 col, hiliy;

	challengegridextradata_t *extradata;

	boolean pending;
	boolean requestnew;

	boolean chaokeyadd, keywasadded;
	UINT8 chaokeyhold;

	boolean requestflip;

	UINT16 unlockcount[CMC_MAX];

	UINT8 fade;

	boolean cache_secondrowlocked;

	patch_t *tile_category[10][2];
} challengesmenu;

menu_t *M_InterruptMenuWithChallenges(menu_t *desiredmenu);
void M_Challenges(INT32 choice);
void M_DrawChallenges(void);
void M_ChallengesTick(void);
boolean M_ChallengesInputs(INT32 ch);
boolean M_CanKeyHiliTile(void);

typedef enum
{
	//statisticspage_overview = 0,
	statisticspage_chars = 0,
	statisticspage_gp,
	statisticspage_maps,
	statisticspage_time,
	statisticspage_max
} statisticspage_t;

extern struct statisticsmenu_s {
	statisticspage_t page;
	INT32 location;
	INT32 nummaps;
	INT32 gotmedals;
	INT32 nummedals;
	INT32 numextramedals;
	UINT32 statgridplayed[9][9];
	INT32 maxscroll;
	UINT16 *maplist;
} statisticsmenu;

void M_Statistics(INT32 choice);
void M_DrawStatistics(void);
boolean M_StatisticsInputs(INT32 ch);

void M_DrawCharacterIconAndEngine(INT32 x, INT32 y, UINT8 skin, UINT8 *colormap, UINT8 baseskin);
fixed_t M_DrawCupWinData(INT32 rankx, INT32 ranky, cupheader_t *cup, UINT8 difficulty, boolean flash, boolean statsmode);

#define MAXWRONGPLAYER MAXSPLITSCREENPLAYERS
#define WRONGPLAYEROFFSCREEN 48

extern struct wrongwarp_s {
	INT32 ticker;
	tic_t delaytowrongplayer;
	struct wrongplayer_s
	{
		UINT8 skin;
		INT16 across;
		boolean spinout;
	} wrongplayers[MAXWRONGPLAYER];
} wrongwarp;

void M_WrongWarp(INT32 choice);
void M_DrawWrongWarp(void);

typedef enum
{
	stereospecial_none = 0,
	stereospecial_back,
	stereospecial_pause,
	stereospecial_play,
	stereospecial_seq,
	stereospecial_shf,
	stereospecial_vol,
	stereospecial_track,
} stereospecial_e;

void M_SoundTest(INT32 choice);
void M_DrawSoundTest(void);
consvar_t *M_GetSoundTestVolumeCvar(void);

#ifdef HAVE_DISCORDRPC
extern struct discordrequestmenu_s {
	tic_t ticker;
	tic_t confirmDelay;
	tic_t confirmLength;
	boolean confirmAccept;
	boolean removeRequest;
} discordrequestmenu;

void M_DrawDiscordRequests(void);
void M_DiscordRequests(INT32 choice);
const char *M_GetDiscordName(discordRequest_t *r);
#endif

// These defines make it a little easier to make menus
#define DEFAULTMENUSTYLE(source, prev, x, y)\
{\
	sizeof(source) / sizeof(menuitem_t),\
	prev,\
	0,\
	source,\
	x, y,\
	0, 0,\
	0,\
	NULL,\
	0, 0,\
	M_DrawGenericMenu,\
	NULL,\
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
	0, 0,\
	0,\
	NULL,\
	1, 5,\
	M_DrawKartGamemodeMenu,\
	NULL,\
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
	0, 0,\
	0,\
	"EXTRAS",\
	1, 5,\
	M_DrawImageDef,\
	NULL,\
	NULL,\
	NULL,\
	NULL,\
	NULL\
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__K_MENU__
