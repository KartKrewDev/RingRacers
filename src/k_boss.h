// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2022 by Viv "toaster" Grannell
// Copyright (C) 2018-2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_boss.h
/// \brief Boss battle game logic

#ifndef __K_BOSS__
#define __K_BOSS__

#include "doomdef.h"
#include "doomstat.h"

typedef enum
{
	SPOT_NONE = 0,
	SPOT_WEAK,
	SPOT_BUMP,
} spottype_t;

#define NUMWEAKSPOTS 8
#define WEAKSPOTANIMTIME (3*TICRATE)

typedef struct weakspot_t
{
	mobj_t *spot;
	spottype_t type;
	tic_t time;
	UINT16 color;
	boolean minimap;
} weakspot_t;

#define BOSSHEALTHBARLEN 110

extern struct bossinfo
{
	boolean boss;						///< If true, then we are fighting a boss
	UINT8 healthbar;					///< Actual health bar fill amount
	UINT8 visualbar;					///< Tracks above, but with delay
	fixed_t visualdiv;					///< How far apart health bar divisions should appear
	tic_t visualbarimpact;				///< Bar jitter (on damage)
	UINT8 healthbarpinch;				///< If actual health is lower than this, make it orange
	UINT8 barlen;						///< The length of the bar (only reduced when a boss is deceased)
	char *enemyname;					///< The name next to the bar
	weakspot_t weakspots[NUMWEAKSPOTS]; ///< Array of weak spots (for minimap/object tracking)
	boolean encore;						///< Copy of encore, just to make sure you can't cheat it with cvars
	spottype_t doweakspotsound;			///< If nonzero, at least one weakspot was declared this tic
	tic_t titleshow;					///< Show this many letters on the titlecard
	sfxenum_t titlesound;				///< Sound to play when title typing
	char *subtitle;						///< The subtitle under the titlecard
} bossinfo;

void K_BossInfoTicker(void);
void K_InitBossHealthBar(const char *enemyname, const char *subtitle, sfxenum_t titlesound, fixed_t pinchmagnitude, UINT8 divisions);
void K_UpdateBossHealthBar(fixed_t magnitude, tic_t jitterlen);
void K_DeclareWeakspot(mobj_t *spot, spottype_t spottype, UINT16 color, boolean minimap);

#endif
