// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_boss.h
/// \brief Boss battle game logic

#ifndef K_BOSS_H
#define K_BOSS_H

#include "doomdef.h"
#include "doomstat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	SPOT_NONE = 0,
	SPOT_WEAK,
	SPOT_BUMP,
} spottype_t;

#define NUMWEAKSPOTS 8
#define WEAKSPOTANIMTIME (3*TICRATE)

struct weakspot_t
{
	mobj_t *spot;
	spottype_t type;
	tic_t time;
	uint16_t color;
	dboolean minimap;
};

#define BOSSHEALTHBARLEN 110

extern struct bossinfo
{
	dboolean valid;						///< If true, then data in this struct is valid

	uint8_t healthbar;					///< Actual health bar fill amount
	uint8_t visualbar;					///< Tracks above, but with delay
	fixed_t visualdiv;					///< How far apart health bar divisions should appear
	tic_t visualbarimpact;				///< Bar jitter (on damage)
	uint8_t healthbarpinch;				///< If actual health is lower than this, make it orange
	uint8_t barlen;						///< The length of the bar (only reduced when a boss is deceased)
	char *enemyname;					///< The name next to the bar
	weakspot_t weakspots[NUMWEAKSPOTS]; ///< Array of weak spots (for minimap/object tracking)
	dboolean coolintro;					///< Determines whether the map start(s/ed) with a boss-specific intro.
	spottype_t doweakspotsound;			///< If nonzero, at least one weakspot was declared this tic
	tic_t titleshow;					///< Show this many letters on the titlecard
	sfxenum_t titlesound;				///< Sound to play when title typing
	char *subtitle;						///< The subtitle under the titlecard
} bossinfo;

/*--------------------------------------------------
	void K_ResetBossInfo(void);

		Resets boss information to a clean slate.
--------------------------------------------------*/

void K_ResetBossInfo(void);

/*--------------------------------------------------
	void K_ResetBossInfo(void);

		Updates boss information and timers for this level tic.
--------------------------------------------------*/

void K_BossInfoTicker(void);

/*--------------------------------------------------
	void K_InitBossHealthBar(const char *enemyname, const char *subtitle, sfxenum_t titlesound, fixed_t pinchmagnitude, uint8_t divisions);

		Initialises boss information for opponent spawn, including filling the health bar.

	Input Arguments:-
		enemyname - Zone memory string for HUD/titlecard name.
		subtitle - Zone memory string for titlecard subtitle.
		titlesound - Sound effect enum for titlecard typewriting.
		pinchmagnitude - 0-FRACUNIT range for healthbar to display pinch status at.
		divisions - # of segments on healthbar.
--------------------------------------------------*/

void K_InitBossHealthBar(const char *enemyname, const char *subtitle, sfxenum_t titlesound, fixed_t pinchmagnitude, uint8_t divisions);

/*--------------------------------------------------
	void K_UpdateBossHealthBar(fixed_t magnitude, tic_t jitterlen);

		Updates boss healthbar to a new magnitude.

	Input Arguments:-
		magnitude - 0-FRACUNIT range for healthbar to update to.
		jitterlen - Duration healthbar should vibrate for.
--------------------------------------------------*/

void K_UpdateBossHealthBar(fixed_t magnitude, tic_t jitterlen);

/*--------------------------------------------------
	void K_DeclareWeakspot(mobj_t *spot, spottype_t spottype, uint16_t color, dboolean minimap);

		Updates the list of Weakspots for the HUD/minimap object tracking.

	Input Arguments:-
		spot - mobj_t reference.
		spottype - Type of spot.
		color - Color of associated UI elements.
		minimap - If true, appear on minimap.
--------------------------------------------------*/

void K_DeclareWeakspot(mobj_t *spot, spottype_t spottype, uint16_t color, dboolean minimap);

/*--------------------------------------------------
	dboolean K_CheckBossIntro(void);

		Checks whether the Versus-specific intro is playing for this map start.

	Return:-
		true if cool intro in action,
		otherwise false.
--------------------------------------------------*/

dboolean K_CheckBossIntro(void);

// Arena objects

dboolean VS_ArenaCenterInit(mobj_t *mobj, mapthing_t *mthing);
mobj_t *VS_GetArena(int32_t bossindex);
fixed_t *VS_PredictAroundArena(mobj_t *arena, mobj_t *movingobject, fixed_t magnitude, angle_t mompoint, fixed_t radiussubtract, dboolean forcegoaround, fixed_t radiusdeltafactor);
fixed_t *VS_RandomPointOnArena(mobj_t *arena, fixed_t radiussubtract);

// Blend Eye

void VS_BlendEye_Init(mobj_t *mobj);
void VS_BlendEye_Thinker(mobj_t *mobj);
dboolean VS_BlendEye_Touched(mobj_t *special, mobj_t *toucher);
void VS_BlendEye_Damage(mobj_t *mobj, mobj_t *inflictor, mobj_t *source, int32_t damage);
void VS_BlendEye_Death(mobj_t *mobj);

dboolean VS_BlendEye_Eye_Thinker(mobj_t *mobj);
void VS_BlendEye_Glass_Death(mobj_t *mobj);
void VS_BlendEye_Eggbeater_Touched(mobj_t *t1, mobj_t *t2);
void VS_BlendEye_Generator_DeadThinker(mobj_t *mobj);

dboolean VS_PuyoTouched(mobj_t *special, mobj_t *toucher);
void VS_PuyoThinker(mobj_t *mobj);
void VS_PuyoDeath(mobj_t *mobj);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
