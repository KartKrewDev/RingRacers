// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
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

#ifndef K_MENU_H
#define K_MENU_H

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

// in int16_t for some common use
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
	void (*routine)(int32_t choice); // IT_CALL, IT_KEYHANDLER, IT_ARROWS
} itemaction_t;

// Player Setup menu colors linked list
struct menucolor_t {
	menucolor_t *next;
	menucolor_t *prev;
	uint16_t color;
};

extern menucolor_t *menucolorhead, *menucolortail;

extern int16_t menugametype;
void M_NextMenuGametype(uint32_t forbidden);
void M_PrevMenuGametype(uint32_t forbidden);
void M_HandleHostMenuGametype(int32_t choice);
void M_HandlePauseMenuGametype(int32_t choice);
void M_HandlePauseMenuAddons(int32_t choice);

extern uint32_t menucallvote; // not midVoteType_e to prevent #include k_zvote
extern uint32_t menuaddonoptions;
void M_HandlePauseMenuCallVote(int32_t choice);

//
// MENU TYPEDEFS
//

typedef int32_t menubehaviourflags_t;
#define MBF_UD_LR_FLIPPED		(1)    // flip up-down and left-right axes
#define MBF_SOUNDLESS		 	(1<<1) // do not play base menu sounds
#define MBF_NOLOOPENTRIES		(1<<2) // do not loop M_NextOpt/M_PrevOpt
#define MBF_DRAWBGWHILEPLAYING	(1<<3) // run backroutine() outside of GS_MENU
#define MBF_CANTRESTORE			(1<<4) // Do not use in restoreMenu

struct menuitem_t
{
	uint16_t status; // show IT_xxx

	const char *text; // option title
	const char *tooltip; // description of option used by K_MenuTooltips
	const char *patch; // image of option used by K_MenuPreviews

	itemaction_t itemaction;

	// extra variables
	int32_t mvar1;
	int32_t mvar2;
};

struct menu_t
{
	int16_t          numitems;           // # of menu items
	menu_t        *prevMenu;           // previous menu

	int16_t          lastOn;             // last item user was on in menu
	menuitem_t    *menuitems;          // menu items

	int16_t          x, y;               // x, y of menu
	int16_t 		   extra1, extra2;	   // Can be whatever really! Options menu uses extra1 for bg colour.
	int16_t          behaviourflags;     // menubehaviourflags_t
	const char    *music;              // Track to play in M_PlayMenuJam. NULL for default, "." to stop

	int16_t          transitionID;       // only transition if IDs match
	int16_t          transitionTics;     // tics for transitions out

	void         (*drawroutine)(void); // draw routine
	void           (*bgroutine)(void); // draw routine, but, like, for the background
	void         (*tickroutine)(void); // ticker routine
	void         (*initroutine)(void); // called when starting a new menu
	dboolean      (*quitroutine)(void); // called before quit a menu return true if we can
	dboolean		 (*inputroutine)(int32_t); // if set, called every frame in the input handler. Returning true overwrites normal input handling.
};

struct menu_anim_t
{
	tic_t start;
	int16_t dist;
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

extern menu_t MAIN_GonerAccessibilityDef;
extern menu_t MAIN_GonerDef;

void M_GonerTick(void);
void M_GonerBGTick(void);
void M_GonerBGImplyPassageOfTime(void);
void M_DrawGonerBack(void);
void M_GonerProfile(int32_t choice);
void M_GonerChoice(int32_t choice);
void M_GonerTutorial(int32_t choice);
void M_GonerPlayground(int32_t choice);
void M_GonerResetLooking(int type);
void M_GonerCheckLooking(void);
void M_GonerResetText(dboolean completely);
void M_GonerGDQ(dboolean opinion);
dboolean M_GonerMusicPlayable(void);

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
	mopt_voice,
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
	sopt_voicevolume,
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

void M_StartEditProfile(int32_t c);

extern menuitem_t OPTIONS_ProfileControls[];
extern menu_t OPTIONS_ProfileControlsDef;

extern menuitem_t OPTIONS_ProfileAccessibility[];
extern menu_t OPTIONS_ProfileAccessibilityDef;

extern menuitem_t OPTIONS_Video[];
extern menu_t OPTIONS_VideoDef;

extern menuitem_t OPTIONS_VideoModes[];
extern menu_t OPTIONS_VideoModesDef;

extern menuitem_t OPTIONS_VideoColorProfile[];
extern menu_t OPTIONS_VideoColorProfileDef;

extern menuitem_t OPTIONS_VideoAdvanced[];
extern menu_t OPTIONS_VideoAdvancedDef;

extern menuitem_t OPTIONS_Sound[];
extern menu_t OPTIONS_SoundDef;

extern menuitem_t OPTIONS_Voice[];
extern menu_t OPTIONS_VoiceDef;

extern menuitem_t OPTIONS_HUD[];
extern menu_t OPTIONS_HUDDef;

extern menuitem_t OPTIONS_HUDOnline[];
extern menu_t OPTIONS_HUDOnlineDef;

typedef enum
{
	gopt_spacer0 = 0,
	gopt_teamplay,
	gopt_frantic,
	gopt_spacer1,
	gopt_gamespeed,
	gopt_encore,
	gopt_exitcountdown,
	gopt_spacer2,
	gopt_timelimit,
	gopt_pointlimit,
	gopt_startingbumpers,
	gopt_spacer3,
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
	playback_restart,
	playback_rewind,
	playback_pause,
	playback_fastforward,
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

extern int16_t itemOn; // menu item skull is on, Hack by Tails 09-18-2002
extern int16_t skullAnimCounter; // skull animation counter

extern int32_t menuKey; // keyboard key pressed for menu

#define NUMVIRTUALKEYSINROW (10+2) // 1-9, 0, and a right-side gutter of two keys' width
extern int16_t virtualKeyboard[5][NUMVIRTUALKEYSINROW];
extern int16_t shift_virtualKeyboard[5][NUMVIRTUALKEYSINROW];

typedef const char *(*vkb_query_fn_t)(const char *replace);
extern struct menutyping_s
{
	dboolean active;				// Active
	dboolean menutypingclose;	// Closing
	dboolean keyboardtyping;		// If true, all keystrokes are treated as typing (ignores MBT_A etc). This is unset if you try moving the cursor on the virtual keyboard or use your controller
	int8_t menutypingfade;		// fade in and out

	int8_t keyboardx;
	int8_t keyboardy;
	dboolean keyboardcapslock;
	dboolean keyboardshift;

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

#define MAXMENUMESSAGE 448
#define MENUMESSAGECLOSE 2
extern struct menumessage_s
{
	dboolean active;
	uint8_t closing;

	int32_t flags;		// MM_
	const char *header;
	char message[MAXMENUMESSAGE];	// message to display

	int8_t fadetimer;	// opening
	int32_t x;
	int32_t y;
	int16_t timer;

	void (*routine)(int32_t choice);	// Normal routine
	//void (*eroutine)(event_t *ev);	// Event routine	(MM_EVENTHANDLER)
	int32_t answer;

	const char *defaultstr;
	const char *confirmstr;
} menumessage;

void M_StartMessage(const char *header, const char *string, void (*routine)(int32_t), menumessagetype_t itemtype, const char *confirmstr, const char *defaultstr);
dboolean M_MenuMessageTick(void);
void M_HandleMenuMessage(void);
void M_StopMessage(int32_t choice);
void M_DrawMenuMessage(void);

#define MENUDELAYTIME 7
#define MENUMINDELAY 2

typedef int32_t menuButtonCode_t;
#define MBT_A (1)
#define MBT_B (1<<1)
#define MBT_C (1<<2)
#define MBT_X (1<<3)
#define MBT_Y (1<<4)
#define MBT_Z (1<<5)
#define MBT_L (1<<6)
#define MBT_R (1<<7)
#define MBT_START (1<<8)
#define MBT_SCREENSHOT (1<<9)
#define MBT_STARTMOVIE (1<<10)
#define MBT_STARTLOSSLESS (1<<11)

struct menucmd_t
{
	// Current frame's data
	int8_t dpad_ud; // up / down dpad
	int8_t dpad_lr; // left / right
	uint32_t buttons; // buttons

	// Previous frame's data
	int8_t prev_dpad_ud;
	int8_t prev_dpad_lr;
	uint32_t buttonsHeld;

	uint16_t delay; // menu wait
	uint32_t delayCount; // num times ya did menu wait (to make the wait shorter each time)
};

extern menucmd_t menucmd[MAXSPLITSCREENPLAYERS];

extern struct menutransition_s {
	int16_t tics;
	int16_t dest;
	menu_t *startmenu;
	menu_t *endmenu;
	dboolean in;
} menutransition;

extern dboolean menuwipe;

extern consvar_t cv_showfocuslost;
extern consvar_t cv_chooseskin, cv_serversort, cv_menujam_update, cv_menujam;
extern consvar_t cv_autorecord;

extern consvar_t cv_racesplits, cv_attacksplits;

void M_SetMenuDelay(uint8_t i);

void M_SortServerList(void);

void M_UpdateMenuCMD(uint8_t i, dboolean bailrequired, dboolean chat_open);
dboolean M_Responder(event_t *ev);
dboolean M_MenuButtonPressed(uint8_t pid, uint32_t bt);
dboolean M_MenuButtonHeld(uint8_t pid, uint32_t bt);

dboolean M_ChangeStringCvar(int32_t choice);
void M_ChangeCvarDirect(int32_t choice, consvar_t *cv);
dboolean M_NextOpt(void);
dboolean M_PrevOpt(void);

dboolean M_MenuConfirmPressed(uint8_t pid);
dboolean M_MenuConfirmHeld(uint8_t pid);
dboolean M_MenuBackPressed(uint8_t pid);
dboolean M_MenuBackHeld(uint8_t pid);
dboolean M_MenuExtraPressed(uint8_t pid);
dboolean M_MenuExtraHeld(uint8_t pid);

void M_StartControlPanel(void);
void M_ValidateRestoreMenu(void);
menu_t *M_SpecificMenuRestore(menu_t *torestore);
void M_ClearMenus(dboolean callexitmenufunc);
void M_ClearMenusNoTitle(dboolean callexitmenufunc);
void M_SelectableClearMenus(int32_t choice);
void M_SetupNextMenu(menu_t *menudef, dboolean nofade);
void M_GoBack(int32_t choice);
void M_Ticker(void);
void M_Init(void);

void M_PlayMenuJam(void);

dboolean M_ConsiderSealedSwapAlert(void);

void M_OpenVirtualKeyboard(size_t cachelen, vkb_query_fn_t queryfn, menu_t *dummymenu);
void M_AbortVirtualKeyboard(void);
void M_MenuTypingInput(int32_t key);
void M_SwitchVirtualKeyboard(dboolean gamepad);

void M_QuitResponse(int32_t ch);
void M_QuitSRB2(int32_t choice);

uint16_t M_GetColorAfter(setup_player_colors_t *colors, uint16_t value, int32_t amount);
#define M_GetColorBefore(a, b, c) M_GetColorAfter(a, b, -c)

// If you want to waste a bunch of memory for a limit no one will hit, feel free to boost this to MAXSKINS :P
// I figure this will be enough clone characters to fit onto one grid space.
// TODO: Dynamically allocate instead, you KNOW this limit will get hit by someone eventually
#define MAXCLONES MAXSKINS/8

extern struct setup_chargrid_s {
	int16_t skinlist[MAXCLONES];
	uint16_t numskins;
} setup_chargrid[9][9];

extern uint8_t setup_followercategories[MAXFOLLOWERCATEGORIES][2];
extern uint8_t setup_numfollowercategories;

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
	uint16_t *list;
	size_t listLen;
	size_t listCap;
};

struct setup_player_t
{
	int8_t gridx, gridy;
	uint8_t profilen;
	menu_anim_t profilen_slide;
	int16_t skin;
	int8_t clonenum;
	int8_t rotate;
	uint8_t delay;
	uint16_t color;
	uint8_t mdepth;
	dboolean hitlag;
	dboolean showextra;

	// Hack, save player 1's original device even if they init charsel with keyboard.
	// If they play ALONE, allow them to retain that original device, otherwise, ignore this.
	// We can allow them to retain the device with no consequence as when P1 is alone, they have exclusive keyboard fallback options.
	uint8_t ponedevice;

	uint8_t changeselect;

	int16_t followercategory;
	int16_t followern;
	uint16_t followercolor;
	tic_t follower_tics;
	tic_t follower_timer;
	uint8_t follower_frame;
	state_t *follower_state;

	setup_player_colors_t colors;
};

extern setup_player_t setup_player[MAXSPLITSCREENPLAYERS];

extern uint8_t setup_numplayers;
extern tic_t setup_animcounter;

// for charsel pages.
extern uint8_t setup_page;
extern uint8_t setup_maxpage;

#define CSROTATETICS 6

// The selection spawns 3 explosions in 4 directions, and there's 4 players -- 3 * 4 * 4 = 48
#define CSEXPLOSIONS 48

extern struct setup_explosions_s {
	int16_t x, y;
	uint8_t tics;
	uint16_t color;
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
void M_CharacterSelect(int32_t choice);
void M_SetupReadyExplosions(dboolean charsel, uint16_t basex, uint16_t basey, uint16_t color);
dboolean M_CharacterSelectForceInAction(void);
dboolean M_CharacterSelectHandler(int32_t choice);
void M_CharacterSelectTick(void);
dboolean M_CharacterSelectQuit(void);

void M_SetupPlayMenu(int32_t choice);
void M_SetupGametypeMenu(int32_t choice);
void M_SetupRaceMenu(int32_t choice);

#define CUPMENU_CURSORID (cupgrid.x + (cupgrid.y * CUPMENU_COLUMNS) + (cupgrid.pageno * (CUPMENU_COLUMNS * CUPMENU_ROWS)))
#define CUPMENU_SLIDETIME 3

extern struct cupgrid_s {
	int8_t x, y;
	menu_anim_t xslide, yslide;
	size_t pageno;
	cupheader_t **builtgrid;
	size_t numpages;
	size_t cappages;
	tic_t previewanim;
	dboolean cache_secondrowlocked;
} cupgrid;

typedef struct levelsearch_s {
	uint32_t typeoflevel;
	cupheader_t *cup;
	dboolean timeattack;
	dboolean tutorial;
	dboolean grandprix;
	dboolean cupmode;
	dboolean checklocked;
} levelsearch_t;

#define M_LEVELLIST_SLIDETIME 4

typedef struct levellist_s {
	int8_t cursor;
	menu_anim_t slide;
	uint16_t y;
	uint16_t choosemap;
	uint16_t mapcount;
	uint8_t newgametype;
	uint8_t guessgt;
	levelsearch_t levelsearch;
	dboolean netgame;	// Start the game in an actual server
	dboolean canqueue;
	menu_t *backMenu;
} levellist_t;

extern levellist_t levellist;
extern levellist_t restorelevellist;

extern cupheader_t dummy_lostandfound;

dboolean M_CanShowLevelInList(int16_t mapnum, levelsearch_t *levelsearch);
uint16_t M_CountLevelsToShowInList(levelsearch_t *levelsearch);
uint16_t M_GetFirstLevelInList(uint8_t *i, levelsearch_t *levelsearch);
uint16_t M_GetNextLevelInList(uint16_t mapnum, uint8_t *i, levelsearch_t *levelsearch);
void M_LevelSelectScrollDest(void);
dboolean M_LevelListFromGametype(int16_t gt);

void M_LevelSelectInit(int32_t choice);
void M_CupSelectHandler(int32_t choice);
void M_CupSelectTick(void);
void M_LevelSelectHandler(int32_t choice);
void M_LevelSelectTick(void);

int16_t M_LevelFromScrolledList(int16_t add);
void M_MenuToLevelPreamble(uint8_t ssplayers, dboolean nowipe);
void M_LevelSelected(int16_t add, dboolean menuupdate);
dboolean M_LevelSelectCupSwitch(dboolean next, dboolean skipones);

void M_LevelConfirmHandler(void);
void M_ClearQueueHandler(void);
void M_CupQueueHandler(cupheader_t *cup);

// dummy consvars for GP & match race setup
extern consvar_t cv_dummygpdifficulty;
extern consvar_t cv_dummykartspeed;
extern consvar_t cv_dummygpencore;
extern consvar_t cv_dummymatchbots;

extern consvar_t cv_dummyspbattack;

void M_SetupDifficultyOptions(int32_t choice);
void M_SetupDifficultySelect(int32_t choice);
void M_DifficultySelectInputs(int32_t choice);

// Multiplayer menu stuff

// Keep track of multiplayer menu related data
// We'll add more stuff here as we need em...

#define SERVERSPERPAGE 8
#define SERVERSPACE 18

extern struct mpmenu_s {
	uint8_t modechoice;
	int16_t modewinextend[3][3];	// Used to "extend" the options in the mode select screen.
								// format for each option: {extended?, max extension, # lines extended}
								// See M_OptSelectTick, it'll make more sense there. Sorry if this is a bit of a mess!

	uint8_t room;
	tic_t ticker;

	uint8_t servernum;
	uint8_t scrolln;
	// max scrolln is always going to be serverlistcount-4 as we can display 8 servers at any time and we start scrolling at half.

	int16_t slide;

} mpmenu;

void M_PleaseWait(void);
void M_PopupMasterServerRules(void);

// Time Attack
void M_PrepareTimeAttack(dboolean menuupdate);
void M_StartTimeAttack(int32_t choice);
void M_ReplayTimeAttack(int32_t choice);
void M_HandleStaffReplay(int32_t choice);
void M_SetGuestReplay(int32_t choice);
void M_TimeAttackTick(void);
dboolean M_EncoreAttackTogglePermitted(void);
dboolean M_TimeAttackInputs (int32_t choice);

// MP selection
void M_MPOptSelect(int32_t choice);
void M_MPOptSelectInit(int32_t choice);
void M_MPOptSelectTick(void);
dboolean M_MPResetOpts(void);
extern consvar_t cv_dummyip;			// I HAVE
								// HAVE YOUR IP ADDRESS (This just the hack Cvar we'll type into and then it apends itself to "connect" in the console for IP join)

// MP Host
void M_MPHostInit(int32_t choice);
void M_MPSetupNetgameMapSelect(int32_t choice);

// MP join by IP
void M_MPJoinIPInit(int32_t choice);
dboolean M_JoinIPInputs(int32_t ch);
void M_JoinIP(const char *ipa);

// Server browser room selection
void M_MPRoomSelect(int32_t choice);
void M_MPRoomSelectTick(void);
void M_MPRoomSelectInit(int32_t choice);

// Server browser hell with threads...
void M_SetWaitingMode(int mode);
int M_GetWaitingMode(void);

void M_MPServerBrowserTick(void);
dboolean M_ServerBrowserInputs(int32_t ch);

#ifdef MASTERSERVER
#ifdef HAVE_THREADS

void Spawn_masterserver_thread (const char *name, void (*thread)(int*));
int Same_instance (int id);

#endif /*HAVE_THREADS*/

void Fetch_servers_thread (int *id);

#endif /*MASTERSERVER*/

void M_RefreshServers(int32_t choice);
void M_ServersMenu(int32_t choice);

// Options menu:

// mode descriptions for video mode menu
struct modedesc_t
{
	int32_t modenum; // video mode number in the vidmodes list
	const char *desc;  // XXXxYYY
	uint8_t goodratio; // aspect correct if 1
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
	int16_t optx;
	int16_t opty;
	int16_t toptx;
	int16_t topty;
	tic_t topt_start;

	// profile garbage
	dboolean profilemenu;		// In profile menu. (Used to know when to get the "PROFILE SETUP" button away....
	dboolean resetprofilemenu;	// Reset button behaviour when exiting
	int8_t profilen;				// # of the selected profile.

	dboolean resetprofile;		// After going back from the edit menu, this tells the profile select menu to kill the profile data after the transition.
	profile_t *profile;			// Pointer to the profile we're editing

	int32_t tempcontrols[num_gamecontrols][MAXINPUTMAPPING];
	// Temporary buffer where we're gonna store game controls.
	// This is only applied to the profile when you exit out of the controls menu.

	int16_t controlscroll;		// scrolling for the control menu....
	int16_t bindtimer;			// Timer until binding is cancelled (5s)
	uint16_t bindben;				// Hold right timer
	uint8_t bindben_swallow;		// (bool) control is about to be cleared; (int) swallow/pose animation timer
	int32_t bindinputs[MAXINPUTMAPPING]; // Set while binding

	int16_t trycontroller;		// Starts at 3*TICRATE, holding B lowers this, when at 0, cancel controller try mode.

	// Used for horrible axis shenanigans
	int32_t lastkey;
	tic_t keyheldfor;

	// controller coords...
	// Works the same as (t)opt
	int16_t contx;
	int16_t conty;
	int16_t tcontx;
	int16_t tconty;

	// for video mode testing:
	int32_t vidm_testingmode;
	int32_t vidm_previousmode;
	int32_t vidm_selected;
	int32_t vidm_nummodes;
	int32_t vidm_column_size;

	modedesc_t modedescs[MAXMODEDESCS];

	uint8_t erasecontext;

	uint8_t eraseprofilen;

	// background:
	int16_t currcolour;
	int16_t lastcolour;
	tic_t fade;
} optionsmenu;

extern int16_t controlleroffsets[][2];

extern consvar_t cv_dummyprofilename;
extern consvar_t cv_dummyprofileplayername;
extern consvar_t cv_dummyprofilekickstart;
extern consvar_t cv_dummyprofileautoroulette;
extern consvar_t cv_dummyprofilelitesteer;
extern consvar_t cv_dummyprofilestrictfastfall;
extern consvar_t cv_dummyprofiledescriptiveinput;
extern consvar_t cv_dummyprofileautoring;
extern consvar_t cv_dummyprofilerumble;
extern consvar_t cv_dummyprofilefov;

void M_ResetOptions(void);
void M_InitOptions(int32_t choice); // necessary for multiplayer since there's some options we won't want to access
void M_OptionsTick(void);
dboolean M_OptionsInputs(int32_t ch);
dboolean M_OptionsQuit(void);	// resets buttons when you quit the options.
void M_OptionsChangeBGColour(int16_t newcolour);	// changes the background colour for options

void M_VideoOptions(int32_t choice);
void M_SoundOptions(int32_t choice);
void M_GameplayOptions(int32_t choice);
void M_ServerOptions(int32_t choice);

void M_RefreshAdvancedVideoOptions(void);

void M_HandleItemToggles(int32_t choice);	// For item toggling
void M_EraseData(int32_t choice);	// For data erasing
void M_CheckProfileData(int32_t choice);	// check if we have profiles.
void M_ColorProfileDefault(int32_t choice); // For the reset button in the color profile menu.

// profile selection menu
void M_ProfileSelectInit(int32_t choice);
void M_FirstPickProfile(int32_t c);
void M_HandleProfileSelect(int32_t ch);

// profile edition
void M_HandleProfileEdit(void);
void M_ProfileDeviceSelect(int32_t choice);
void M_ConfirmProfile(int32_t choice);
dboolean M_ProfileEditInputs(int32_t ch);

void M_HandleProfileControls(void);
dboolean M_ProfileControlsInputs(int32_t ch);
void M_ProfileSetControl(int32_t ch);
void M_ProfileDefaultControls(int32_t ch);
void M_ProfileClearControls(int32_t ch);

void M_MapProfileControl(event_t *ev);
void M_ProfileTryController(int32_t choice);
void M_ProfileControlsConfirm(int32_t choice);

// video modes menu (resolution)
void M_HandleVideoModes(int32_t ch);

// data stuff
void M_HandleProfileErase(int32_t choice);

// Draws "List via" at the bottom of the screen.
void M_DrawMasterServerReminder(void);

// Draws the EGGA CHANNEL background.
void M_DrawEggaChannelAlignable(dboolean centered);
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
	int16_t extx;
	int16_t exty;
	int16_t textx;
	int16_t texty;


	// The replay vars...... oh no......
	menudemo_t *demolist;

	int16_t replayScrollTitle;
	int8_t replayScrollDelay;
	int8_t replayScrollDir;



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

void M_InitExtras(int32_t choice); // init for the struct
void M_ExtrasTick(void);
dboolean M_ExtrasInputs(int32_t ch);
dboolean M_ExtrasQuit(void);	// resets buttons when you quit


// Extras: Egg TV
void M_EggTV(int32_t choice);
void M_EggTV_RefreshButtonLabels(void);


// Pause menu:

// Keep track of some pause menu data for visual goodness.
extern struct pausemenu_s {

	tic_t ticker;			// How long the menu's been open for
	menu_anim_t offset;		// To make the icons move smoothly when we transition!

	menu_anim_t openoffset;	// Used when you open / close the menu to slide everything in.
	dboolean closing;		// When this is set, the open offset goes backwards to close the menu smoothly.

	uint8_t splitscreenfocusid; // This is not exclusively visual, but thog dont care. For selecting splitscreen players to individually change their spectator state.
} pausemenu;

void M_OpenPauseMenu(void);
void M_QuitPauseMenu(int32_t choice);
dboolean M_PauseInputs(int32_t ch);
void M_PauseTick(void);

extern struct playerkickmenu_s {
	tic_t ticker;
	uint8_t player;
	uint8_t poke;
	dboolean adminpowered;
} playerkickmenu;

void M_KickHandler(int32_t choice);

extern consvar_t cv_dummymenuplayer;
extern consvar_t cv_dummyspectator;

// Bunch of funny functions for the pause menu...~
void M_RestartMap(int32_t choice);				// Restart level (MP)
void M_TryAgain(int32_t choice);					// Try again (SP)
void M_GiveUp(int32_t choice);					// Give up (SP)
void M_HandleSpectateToggle(int32_t choice);		// Spectate confirm
void M_EndGame(int32_t choice);					// Quitting to title

// Replay Playback

extern tic_t playback_last_menu_interaction_leveltime;

void M_EndModeAttackRun(void);
void M_SetPlaybackMenuPointer(void);
void M_PlaybackRewind(int32_t choice);
void M_PlaybackPause(int32_t choice);
void M_PlaybackFastForward(int32_t choice);
void M_PlaybackAdvance(int32_t choice);
void M_PlaybackSetViews(int32_t choice);
void M_PlaybackAdjustView(int32_t choice);
void M_PlaybackToggleFreecam(int32_t choice);
void M_PlaybackQuit(int32_t choice);

// Misc menus:
#define numaddonsshown 4
void M_Addons(int32_t choice);
void M_AddonsRefresh(void);
void M_HandleAddons(int32_t choice);
char *M_AddonsHeaderPath(void);
extern consvar_t cv_dummyaddonsearch;
extern consvar_t cv_dummyextraspassword;

#ifdef TODONEWMANUAL
void M_Manual(int32_t choice);
#endif
void M_HandleImageDef(int32_t choice);

// K_MENUDRAW.C

// flags for text highlights
#define highlightflags V_AQUAMAP
#define recommendedflags V_GREENMAP
#define warningflags V_ORANGEMAP

#define M_ALTCOLOR V_ORANGEMAP

void M_DrawCursorHand(int32_t x, int32_t y);
void M_DrawUnderline(int32_t left, int32_t right, int32_t y);

// For some menu highlights
uint16_t M_GetCvPlayerColor(uint8_t pnum);

void M_PickMenuBGMap(void);
void M_UpdateMenuBGImage(dboolean forceReset);
void M_DrawMenuBackground(void);
void M_DrawMenuForeground(void);
void M_Drawer(void);
void M_DrawGenericMenu(void);
void M_DrawKartGamemodeMenu(void);
void M_FlipKartGamemodeMenu(dboolean slide);
void M_DrawHorizontalMenu(void);
void M_DrawTextBox(int32_t x, int32_t y, int32_t width, int32_t boxlines);
void M_DrawMessageMenu(void);
void M_DrawImageDef(void);

void M_DrawCharacterSelect(void);
dboolean M_DrawCharacterSprite(int16_t x, int16_t y, int16_t skin, uint8_t spr2, uint8_t rotation, uint32_t frame, int32_t addflags, uint8_t *colormap);

void M_DrawCup(cupheader_t *cup, fixed_t x, fixed_t y, int32_t lockedTic, dboolean isTrophy, uint8_t placement);
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
void M_DrawOptionsColorProfile(void);
void M_DrawOptionsMovingButton(void);	// for sick transitions...
void M_DrawOptions(void);
void M_DrawGenericOptions(void);
void M_DrawProfileCard(int32_t x, int32_t y, dboolean greyedout, profile_t *p);
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

#define EASEOFFHORN 50

extern struct timeattackmenu_s {

	tic_t ticker;		// How long the menu's been open for
	tic_t spbflicker;	// used for SPB flicker-in

} timeattackmenu;

// Keep track of some pause menu data for visual goodness.
extern struct challengesmenu_s {

	tic_t ticker;		// How long the menu's been open for
	int16_t offset;		// To make the icons move smoothly when we transition!
	menu_anim_t move;

	uint16_t currentunlock;
	char *unlockcondition;

	tic_t unlockanim;

	int16_t row, hilix, focusx;
	uint16_t col, hiliy;

	challengegridextradata_t *extradata;

	dboolean pending;
	dboolean requestnew;

	dboolean chaokeyadd, keywasadded;
	uint8_t chaokeyhold;

	uint16_t tutorialfound;

	dboolean requestflip;
	uint16_t nowplayingtile;

	uint16_t unlockcount[CMC_MAX];

	uint8_t fade;

	uint8_t hornposting;

	dboolean cache_secondrowlocked;

	patch_t *tile_category[10][2];
} challengesmenu;

menu_t *M_InterruptMenuWithChallenges(menu_t *desiredmenu);
void M_Challenges(int32_t choice);
void M_DrawChallenges(void);
void M_ChallengesTick(void);
dboolean M_ChallengesInputs(int32_t ch);
dboolean M_CanKeyHiliTile(void);

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
	int32_t location;
	int32_t nummaps;
	int32_t gotmedals;
	int32_t nummedals;
	int32_t numextramedals;
	int32_t numcanbonus;
	uint32_t statgridplayed[9][9];
	int32_t maxscroll;
	uint16_t *maplist;
} statisticsmenu;

void M_Statistics(int32_t choice);
void M_DrawStatistics(void);
dboolean M_StatisticsInputs(int32_t ch);

void M_DrawCharacterIconAndEngine(int32_t x, int32_t y, uint16_t skin, uint8_t *colormap, uint16_t baseskin);
fixed_t M_DrawCupWinData(int32_t rankx, int32_t ranky, cupheader_t *cup, uint8_t difficulty, dboolean flash, dboolean statsmode);

#define MAXWRONGPLAYER MAXSPLITSCREENPLAYERS
#define WRONGPLAYEROFFSCREEN 48

extern struct wrongwarp_s {
	int32_t ticker;
	tic_t delaytowrongplayer;
	struct wrongplayer_s
	{
		uint16_t skin;
		int16_t across;
		dboolean spinout;
	} wrongplayers[MAXWRONGPLAYER];
} wrongwarp;

void M_WrongWarp(int32_t choice);
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

void M_SoundTest(int32_t choice);
void M_DrawSoundTest(void);
consvar_t *M_GetSoundTestVolumeCvar(void);

#ifdef HAVE_DISCORDRPC
extern struct discordrequestmenu_s {
	tic_t ticker;
	tic_t confirmDelay;
	tic_t confirmLength;
	dboolean confirmAccept;
	dboolean removeRequest;
} discordrequestmenu;

void M_DrawDiscordRequests(void);
void M_DiscordRequests(int32_t choice);
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

#ifdef __cplusplus
namespace srb2
{
constexpr inline itemaction_t itemaction(menu_t* menu)
{
	itemaction_t ret {};
	ret.submenu = menu;
	return ret;
}
constexpr inline itemaction_t itemaction(consvar_t* consvar)
{
	itemaction_t ret {};
	ret.cvar = consvar;
	return ret;
}
constexpr inline itemaction_t itemaction(void (*routine)(int32_t choice))
{
	itemaction_t ret {};
	ret.routine = routine;
	return ret;
}
}
#endif

#endif //K_MENU_H
