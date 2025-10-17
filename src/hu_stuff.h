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
/// \file  hu_stuff.h
/// \brief Heads up display

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"
#include "w_wad.h"
#include "r_defs.h"
#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------
//           heads up font
//------------------------------------
#define HU_FONTSTART '\x16' // the first font character
#define HU_FONTEND '~'

#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1)

// SRB2kart
#define KART_FONTSTART '!' // the first font character
#define KART_FONTEND 'Z'

#define KART_FONTSIZE (KART_FONTEND - KART_FONTSTART + 1)

#define AZ_FONTSTART 'A' // the first font character
#define AZ_FONTEND 'Z'

#define AZ_FONTSIZE (AZ_FONTEND - AZ_FONTSTART + 1)

#define NUM_FONTSTART '"' // the first font character
#define NUM_FONTEND '9'

#define NUM_FONTSIZE (NUM_FONTEND - NUM_FONTSTART + 1)
//

// Level title font
#define LT_FONTSTART '!' // the first font characters
#define LT_FONTEND '~' // the last font characters
#define LT_FONTSIZE (LT_FONTEND - LT_FONTSTART + 1)

#define CRED_FONTSTART '!' // the first font character
#define CRED_FONTEND 'Z' // the last font character
#define CRED_FONTSIZE (CRED_FONTEND - CRED_FONTSTART + 1)

#define X( name ) name ## _FONT
/* fonts */
enum
{
	X        (HU),
	X      (MENU),
	X      (TINY),
	X       (MED),
	X      (FILE),

	X        (LT),
	X      (CRED),

	X      (GTOL),
	X      (GTFN),

	X     (GTOL4),
	X     (GTFN4),

	X   (GENESIS),

	X   (TALLNUM),
	X (NIGHTSNUM),
	X   (PINGNUM),
	X	(PROFNUM),
	X    (ROLNUM),
	X    (RO4NUM),

	X      (KART),
	X     (TIMER),
	X (TINYTIMER),
	X        (GM),
	X      (LSHI),
	X     (LSLOW),

	X     (OPPRF),
	X     (PINGF),
};
#undef  X

extern char *shiftxform; // english translation shift table
extern char english_shiftxform[];

//------------------------------------
//           chat stuff
//------------------------------------
#define CHAT_BUFSIZE 64		// that's enough messages, right? We'll delete the older ones when that gets out of hand.
#define NETSPLITSCREEN // why the hell WOULDN'T we want this?
#ifdef NETSPLITSCREEN
#define OLDCHAT (cv_consolechat.value == 1 || dedicated || vid.width < 640)
#else
#define OLDCHAT (cv_consolechat.value == 1 || dedicated || vid.width < 640)
#endif
#define CHAT_MUTE (cv_mute.value && !(server || IsPlayerAdmin(consoleplayer))) // this still allows to open the chat but not to type. That's used for scrolling and whatnot.
#define OLD_MUTE (OLDCHAT && cv_mute.value && !(server || IsPlayerAdmin(consoleplayer))) // this is used to prevent oldchat from opening when muted.

typedef enum
{
	HU_SHOUT		= 1,		// Shout message
	HU_CSAY			= 1<<1,		// Middle-of-screen server message
	HU_PRIVNOTICE	= 1<<2,		// Special server sayto, we don't want to see it as the sender.
} sayflags_t;

// some functions
void HU_AddChatText(const char *text, boolean playsound);

// set true when key is pressed while chat is open
extern boolean chat_keydown;

// set true when entering a chat message
extern boolean chat_on;

// set true when push-to-talk is held
extern boolean g_voicepushtotalk_on;

// keystrokes in the console or chat window
extern boolean hu_keystrokes;

extern patch_t *pinggfx[5];
extern patch_t *framecounter;
extern patch_t *frameslash;

// set true whenever the tab rankings are being shown for any reason
extern boolean hu_showscores;

// init heads up data at game startup.
void HU_Init(void);

void HU_LoadGraphics(void);

// Load a HUDGFX patch or NULL/missingpat (dependent on required boolean).
patch_t *HU_UpdateOrBlankPatch(patch_t **user, boolean required, const char *format, ...);
//#define HU_CachePatch(...) HU_UpdateOrBlankPatch(NULL, false, __VA_ARGS__) -- not sure how to default the missingpat here plus not currently used
#define HU_UpdatePatch(user, ...) HU_UpdateOrBlankPatch(user, true, __VA_ARGS__)

// reset heads up when consoleplayer respawns.
void HU_Start(void);

boolean HU_Responder(event_t *ev);
void HU_Ticker(void);
void HU_Drawer(void);
void HU_DrawSongCredits(void);
void HU_TickSongCredits(void);
char HU_dequeueChatChar(void);
void HU_Erase(void);
void HU_clearChatChars(void);
void HU_drawPing(fixed_t x, fixed_t y, UINT32 ping, UINT32 lag, UINT32 packetloss, INT32 flags, SINT8 toside); // Lat': Ping drawer for scoreboard.
void HU_drawMiniPing(INT32 x, INT32 y, UINT32 ping, UINT32 lag, INT32 flags);

// CECHO interface.
void HU_ClearCEcho(void);
void HU_SetCEchoDuration(INT32 seconds);
void HU_SetCEchoFlags(INT32 flags);
void HU_DoCEcho(const char *msg);

// Titlecard CECHO shite
void HU_DoTitlecardCEcho(player_t *player, const char *msg, boolean interrupt);
void HU_DoTitlecardCEchoForDuration(player_t *player, const char *msg, boolean interrupt, tic_t duration);
void HU_ClearTitlecardCEcho(void);

void DoSayCommand(char *message, SINT8 target, UINT8 flags, UINT8 source);

// Demo playback info
extern UINT32 hu_demotime;
extern UINT32 hu_demolap;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
