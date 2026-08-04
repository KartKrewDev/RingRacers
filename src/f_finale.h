// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_finale.h
/// \brief Title screen, intro, game evaluation, and credits.
///        Also includes protos for screen wipe functions.

#ifndef F_FINALE_H
#define F_FINALE_H

#include "doomtype.h"
#include "d_event.h"
#include "p_mobj.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// FINALE
//

// Called by main loop.
dboolean F_IntroResponder(event_t *ev);
dboolean F_CutsceneResponder(event_t *ev);

// Called by main loop.
void F_IntroTicker(void);
void F_TitleScreenTicker(dboolean run);
void F_CutsceneTicker(void);
void F_AttractDemoTicker(void);
void F_TextPromptTicker(void);

// Called by main loop.
void F_IntroDrawer(void);
void F_TitleScreenDrawer(void);
void F_SkyScroll(INT32 scrollxspeed, INT32 scrollyspeed, const char *patchname);

void F_StartWaitingPlayers(void);
void F_WaitingPlayersTicker(void);
void F_WaitingPlayersDrawer(void);

void F_GameEvaluationDrawer(void);
void F_InitGameEvaluation(void); // depends on grandprixinfo!
void F_StartGameEvaluation(void);
void F_GameEvaluationTicker(void);

void F_VersionDrawer(void);

void F_StartCustomCutscene(INT32 cutscenenum, dboolean precutscene, dboolean resetplayer);
void F_CutsceneDrawer(void);
void F_EndCutScene(void);

void F_StartTextPrompt(INT32 promptnum, INT32 pagenum, mobj_t *mo, UINT16 postexectag, dboolean blockcontrols, dboolean freezerealtime);
void F_GetPromptPageByNamedTag(const char *tag, INT32 *promptnum, INT32 *pagenum);
void F_TextPromptDrawer(void);
void F_EndTextPrompt(dboolean forceexec, dboolean noexec);
dboolean F_GetPromptHideHudAll(void);
dboolean F_GetPromptHideHud(fixed_t y);

INT32 F_AttractDemoExitFade(void);

void F_StartGameEnd(void);
void F_StartIntro(void);
void F_StartTitleScreen(void);
void F_StartEnding(void);

void F_PlayTitleScreenMusic(void);

extern INT32 finalecount;
extern INT32 titlescrollxspeed;
extern INT32 titlescrollyspeed;
extern UINT32 titlemusicstart;

typedef enum
{
	TTMODE_NONE = 0,
	TTMODE_RINGRACERS,
	TTMODE_USER
} ttmode_enum;

#define TTMAX_ALACROIX 30 // max frames for SONIC typeface, plus one for NULL terminating entry
#define TTMAX_USER 100

extern ttmode_enum ttmode;
extern UINT8 ttscale;
// ttmode user vars
extern char ttname[9];
extern INT16 ttx;
extern INT16 tty;
extern INT16 ttloop;
extern UINT16 tttics;
extern dboolean ttavailable[6];

// Current menu parameters
extern char curbgname[9];
extern SINT8 curfadevalue;
extern INT32 curbgcolor;
extern INT32 curbgxspeed;
extern INT32 curbgyspeed;
extern dboolean curbghide;
extern dboolean hidetitlemap;

extern dboolean curhidepics;
extern ttmode_enum curttmode;
extern UINT8 curttscale;
// ttmode user vars
extern char curttname[9];
extern INT16 curttx;
extern INT16 curtty;
extern INT16 curttloop;
extern UINT16 curtttics;

#define TITLEBACKGROUNDACTIVE (curfadevalue >= 0 || curbgname[0])

//
// WIPE
//

extern dboolean WipeInAction;
extern UINT8 g_wipemode;
extern UINT8 g_wipetype;
extern UINT8 g_wipeframe;
extern dboolean g_wipereverse;
extern dboolean g_wipeencorewiggle;
extern dboolean WipeStageTitle;

extern INT32 lastwipetic;

extern dboolean g_attractnowipe;

// Don't know where else to place this constant
// But this file seems appropriate
#define PRELEVELTIME TICRATE // frames in tics

void F_WipeStartScreen(void);
void F_WipeEndScreen(void);
void F_RunWipe(UINT8 wipemode, UINT8 wipetype, dboolean drawMenu, const char *colormap, dboolean reverse, dboolean encorewiggle);
void F_WipeStageTitle(void);
#define F_WipeColorFill(c) V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, c)
tic_t F_GetWipeLength(UINT8 wipetype);
dboolean F_WipeExists(UINT8 wipetype);
/// @brief true if the wipetype is to-black
dboolean F_WipeIsToBlack(UINT8 wipemode);
/// @brief true if the wipetype is to-white
dboolean F_WipeIsToWhite(UINT8 wipemode);
/// @brief true if the wipetype is to-invert
dboolean F_WipeIsToInvert(UINT8 wipemode);
/// @brief true if the wipetype is modulated from the previous frame
dboolean F_WipeIsCrossfade(UINT8 wipemode);

enum
{
	wipe_credits_intermediate, // makes a good 0 I guess.

	// Gamestate wipes
	wipe_level_toblack,
	wipe_intermission_toblack,
	wipe_voting_toblack,
	wipe_continuing_toblack,
	wipe_titlescreen_toblack,
	wipe_menu_toblack,
	wipe_credits_toblack,
	wipe_evaluation_toblack,
	wipe_ceremony_toblack,
	wipe_intro_toblack,
	wipe_cutscene_toblack,

	// Specialized wipes
	wipe_encore_toinvert,
	wipe_encore_towhite,

	// "From black" wipes
	wipe_level_final,
	wipe_intermission_final,
	wipe_voting_final,
	wipe_continuing_final,
	wipe_titlescreen_final,
	wipe_menu_final,
	wipe_credits_final,
	wipe_evaluation_final,
	wipe_ceremony_final,
	wipe_intro_final,
	wipe_cutscene_final,

	// custom intermissions
	wipe_specinter_final,
	wipe_multinter_final,

	NUMWIPEDEFS,
	WIPEFINALSHIFT = (wipe_level_final-wipe_level_toblack)
};

extern UINT8 wipedefs[NUMWIPEDEFS];

#ifdef __cplusplus
} // extern "C"
#endif

#endif
