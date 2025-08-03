// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_tally.h
/// \brief End of level tally screen animations

#ifndef __K_TALLY_H__
#define __K_TALLY_H__

#include "typedef.h"
#include "doomtype.h"
#include "doomdef.h"
#include "sounds.h"

#define TALLY_WINDOW_SIZE (2)

#define MUSIC_COUNTDOWN_MAX (K_TallyDelay() + (modeattacking ? 5*TICRATE : 8*TICRATE))

typedef enum
{
	TALLY_STAT_NA,
	TALLY_STAT_TIME,
	TALLY_STAT_TOTALRINGS,
} tally_stat_e;

typedef enum
{
	TALLY_BONUS_NA,
	TALLY_BONUS_RING,
	TALLY_BONUS_EXP,
	TALLY_BONUS_PRISON,
	TALLY_BONUS_SCORE,
	TALLY_BONUS_POWERSTONES,
} tally_bonus_e;

typedef enum
{
	TALLY_ST_IGNORE,

	TALLY_ST_GOTTHRU_SLIDEIN,
	TALLY_ST_GOTTHRU_SLIDEUP,
	TALLY_ST_BOXES_SLIDEIN,
	TALLY_ST_TEXT_APPEAR,
	TALLY_ST_TEXT_PAUSE,
	TALLY_ST_GRADE_APPEAR,
	TALLY_ST_GRADE_VOICE,
	TALLY_ST_DONE,

	TALLY_ST_GAMEOVER_SLIDEIN,
	TALLY_ST_GAMEOVER_LIVES,
	TALLY_ST_GAMEOVER_DONE,
} tally_state_e;

#define TALLY_DIRECTOR_TIME (4 * TICRATE)

struct level_tally_t
{
	boolean active;
	player_t *owner;

	UINT16 gt;
	boolean gotThru;
	char header[64];
	boolean showRoundNum;
	sfxenum_t gradeVoice;

	// Stats
	INT32 time;
	UINT16 ringPool;
	UINT8 livesAdded;
	tally_stat_e stats[TALLY_WINDOW_SIZE];

	// Possible grade metrics
	UINT8 position, numPlayers;
	UINT8 rings;
	UINT16 exp, totalExp;
	UINT16 prisons, totalPrisons;
	INT32 points, pointLimit;
	UINT8 powerStones;
	tally_bonus_e bonuses[TALLY_WINDOW_SIZE];
	INT32 rank; // FIXME: should be gp_rank_e, weird circular dependency happened

	// Animations
	tally_state_e state;
	INT32 hudSlide;
	INT32 delay;
	INT32 transition, transitionTime;
	UINT8 lines, lineCount;
	INT32 displayStat[TALLY_WINDOW_SIZE];
	INT32 displayBonus[TALLY_WINDOW_SIZE];
	UINT8 tickSound;
	UINT8 xtraBlink;
	boolean showGrade;
	boolean done;
	boolean releasedFastForward;
	INT32 directorWait;

#ifdef __cplusplus
	boolean UseBonuses(void);
	void DetermineBonuses(void);
	void DetermineStatistics(void);
	INT32 CalculateGrade(void);
	void Init(player_t *player);
	void NewLine(void);
	boolean IncrementLine(void);
	void Tick(void);
	void Draw(void);
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

void K_InitPlayerTally(player_t *player);
void K_TickPlayerTally(player_t *player);
void K_DrawPlayerTally(void);
boolean K_PlayerTallyActive(player_t *player);
tic_t K_TallyDelay(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __K_TALLY_H__
