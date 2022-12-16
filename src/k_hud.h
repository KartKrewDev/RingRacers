// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hud.h
/// \brief HUD drawing functions exclusive to Kart

#include "doomtype.h"
#include "doomstat.h"
#include "hu_stuff.h"

#ifndef __K_HUD__
#define __K_HUD__

#define RINGANIM_NUMFRAMES 10
#define RINGANIM_DELAYMAX 5

#define POS_DELAY_TIME 10

struct trackingResult_t
{
	fixed_t x, y;
	fixed_t scale;
	boolean onScreen;
};

void K_ObjectTracking(trackingResult_t *result, vector3_t *point, boolean reverse);

const char *K_GetItemPatch(UINT8 item, boolean tiny);
void K_LoadKartHUDGraphics(void);
void K_drawKartHUD(void);
void K_drawKartFreePlay(void);
void K_drawKartTimestamp(tic_t drawtime, INT32 TX, INT32 TY, INT16 emblemmap, UINT8 mode);
void K_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer, INT32 hilicol);

#endif
