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

#ifndef Y_INTER_H
#define Y_INTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	dboolean rankingsmode; // rankings mode
	dboolean gotthrough; // show "got through"
	dboolean showrank; // show rank-restricted queue entry at the end, if it exists
	dboolean encore; // encore mode
	dboolean isduel; // duel mode
	uint8_t winningteam; // teamplay
	dboolean showroundnum; // round number

	char headerstring[64]; // holds levelnames up to 64 characters

	uint8_t numplayers; // Number of players being displayed
	uint8_t halfway; // Position at which column switches

	int8_t num[MAXPLAYERS]; // Player #
	uint8_t pos[MAXPLAYERS]; // player positions. used for ties

	uint32_t val[MAXPLAYERS]; // Gametype-specific value
	char strval[MAXPLAYERS][MAXPLAYERNAME+1];

	int16_t increase[MAXPLAYERS]; // how much did the score increase by?
	uint8_t jitter[MAXPLAYERS]; // wiggle

	int8_t grade[MAXPLAYERS]; // grade, if not a bot

	uint8_t mainplayer; // Most successful local player
	int32_t linemeter; // For GP only
} y_data_t;

void Y_DrawIntermissionHeader(int32_t x, int32_t y, dboolean gotthrough, const char *headerstring, dboolean showroundnum, dboolean small);
void Y_IntermissionDrawer(void);
void Y_Ticker(void);

// Specific sub-drawers
void Y_PlayerStandingsDrawer(y_data_t *standings, int32_t xoffset);
void Y_RoundQueueDrawer(y_data_t *standings, int32_t offset, dboolean doanimations, dboolean widescreen, dboolean adminmode);
void Y_DrawIntermissionButton(int32_t startslide, int32_t through, dboolean widescreen);
void Y_DrawRankMode(int32_t x, int32_t y, dboolean center);

void Y_StartIntermission(void);
void Y_MidIntermission(void);
void Y_EndIntermission(void);

void Y_PlayIntermissionMusic(void);

dboolean Y_IntermissionPlayerLock(void);

typedef enum
{
	int_none,
	int_time,				// Always time
	int_score,				// Always score
	int_scoreortimeattack,	// Score unless 1P
} intertype_t;

extern intertype_t intertype;

dboolean Y_ShouldDoIntermission(void);
intertype_t Y_GetIntermissionType(void);
void Y_DetermineIntermissionType(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // Y_INTER_H
