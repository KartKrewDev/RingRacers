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

#ifndef K_TALLY_H
#define K_TALLY_H

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
	dboolean active;
	player_t *owner;

	uint16_t gt;
	dboolean gotThru;
	char header[64];
	dboolean showRoundNum;
	sfxenum_t gradeVoice;

	// Stats
	int32_t time;
	uint16_t ringPool;
	uint8_t livesAdded;
	tally_stat_e stats[TALLY_WINDOW_SIZE];

	// Possible grade metrics
	uint8_t position, numPlayers;
	uint8_t rings;
	uint16_t exp, totalExp;
	uint16_t prisons, totalPrisons;
	int32_t points, pointLimit;
	uint8_t powerStones;
	tally_bonus_e bonuses[TALLY_WINDOW_SIZE];
	int32_t rank; // FIXME: should be gp_rank_e, weird circular dependency happened

	// Animations
	tally_state_e state;
	int32_t hudSlide;
	int32_t delay;
	int32_t transition, transitionTime;
	uint8_t lines, lineCount;
	int32_t displayStat[TALLY_WINDOW_SIZE];
	int32_t displayBonus[TALLY_WINDOW_SIZE];
	uint8_t tickSound;
	uint8_t xtraBlink;
	dboolean showGrade;
	dboolean done;
	dboolean releasedFastForward;
	int32_t directorWait;

#ifdef __cplusplus
	dboolean UseBonuses(void);
	void DetermineBonuses(void);
	void DetermineStatistics(void);
	int32_t CalculateGrade(void);
	void Init(player_t *player);
	void NewLine(void);
	dboolean IncrementLine(void);
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
dboolean K_PlayerTallyActive(player_t *player);
tic_t K_TallyDelay(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // K_TALLY_H
