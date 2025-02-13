// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  y_inter.h
/// \brief Tally screens, or "Intermissions" as they were formally called in Doom

#ifndef __Y_INTER_H__
#define __Y_INTER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	boolean rankingsmode; // rankings mode
	boolean gotthrough; // show "got through"
	boolean showrank; // show rank-restricted queue entry at the end, if it exists
	boolean encore; // encore mode
	boolean isduel; // duel mode
	boolean showroundnum; // round number

	char headerstring[64]; // holds levelnames up to 64 characters

	UINT8 numplayers; // Number of players being displayed

	SINT8 num[MAXPLAYERS]; // Player #
	UINT8 pos[MAXPLAYERS]; // player positions. used for ties

	UINT8 character[MAXPLAYERS]; // Character #
	UINT16 color[MAXPLAYERS]; // Color #

	UINT32 val[MAXPLAYERS]; // Gametype-specific value
	char strval[MAXPLAYERS][MAXPLAYERNAME+1];

	INT16 increase[MAXPLAYERS]; // how much did the score increase by?
	UINT8 jitter[MAXPLAYERS]; // wiggle

	SINT8 grade[MAXPLAYERS]; // grade, if not a bot

	UINT8 mainplayer; // Most successful local player
	INT32 linemeter; // For GP only
} y_data_t;

void Y_DrawIntermissionHeader(INT32 x, INT32 y, boolean gotthrough, const char *headerstring, boolean showroundnum, boolean small);
void Y_IntermissionDrawer(void);
void Y_Ticker(void);

// Specific sub-drawers
void Y_PlayerStandingsDrawer(y_data_t *standings, INT32 xoffset);
void Y_RoundQueueDrawer(y_data_t *standings, INT32 offset, boolean doanimations, boolean widescreen);
void Y_DrawIntermissionButton(INT32 startslide, INT32 through, boolean widescreen);

void Y_StartIntermission(void);
void Y_EndIntermission(void);

boolean Y_ShouldDoIntermission(void);
void Y_DetermineIntermissionType(void);

void Y_PlayIntermissionMusic(void);

typedef enum
{
	int_none,
	int_time,				// Always time
	int_score,				// Always score
	int_scoreortimeattack,	// Score unless 1P
} intertype_t;

extern intertype_t intertype;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __Y_INTER_H__
