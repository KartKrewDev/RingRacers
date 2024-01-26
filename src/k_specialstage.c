// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_specialstage.c
/// \brief Special Stage game logic

#include "k_specialstage.h"
#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"
#include "k_kart.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "z_zone.h"
#include "k_waypoint.h"
#include "k_objects.h"
#include "music.h"

struct specialstageinfo specialstageinfo;

/*--------------------------------------------------
	void K_ResetSpecialStage(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetSpecialStage(void)
{
	memset(&specialstageinfo, 0, sizeof(struct specialstageinfo));
	specialstageinfo.beamDist = UINT32_MAX;
}

/*--------------------------------------------------
	void K_InitSpecialStage(void)

		See header file for description.
--------------------------------------------------*/
void K_InitSpecialStage(void)
{
	if ((gametyperules & (GTR_CATCHER|GTR_CIRCUIT)) != (GTR_CATCHER|GTR_CIRCUIT))
	{
		return;
	}

	specialstageinfo.valid = true;

	specialstageinfo.beamDist = UINT32_MAX; // TODO: make proper value
	P_SetTarget(&specialstageinfo.ufo, Obj_CreateSpecialUFO());
}

/*--------------------------------------------------
	static void K_MoveExitBeam(void)

		Updates the exit beam.
--------------------------------------------------*/
static void K_MoveExitBeam(void)
{
	UINT32 moveDist = 0;
	INT32 i;

	if (leveltime <= 2)
	{
		return;
	}

	moveDist = (8 * mapobjectscale) / FRACUNIT;

	if (specialstageinfo.beamDist <= moveDist)
	{
		specialstageinfo.beamDist = 0;

		// TODO: Fail Special Stage
	}
	else
	{
		specialstageinfo.beamDist -= moveDist;
	}

	// Find players who are now outside of the level.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];

		if (player->spectator == true
			|| player->exiting > 0
			|| (player->pflags & PF_NOCONTEST))
		{
			continue;
		}

		if (player->distancetofinish > specialstageinfo.beamDist)
		{
			P_DoPlayerExit(player, PF_NOCONTEST);
		}
	}
}

/*--------------------------------------------------
	void K_TickSpecialStage(void)

		See header file for description.
--------------------------------------------------*/
void K_TickSpecialStage(void)
{
	if (specialstageinfo.valid == false)
	{
		return;
	}

	if (P_MobjWasRemoved(specialstageinfo.ufo))
	{
		P_SetTarget(&specialstageinfo.ufo, NULL);
	}

	K_MoveExitBeam();
}

/*--------------------------------------------------
	mobj_t *K_GetPossibleSpecialTarget(void)

		See header file for description.
--------------------------------------------------*/
mobj_t *K_GetPossibleSpecialTarget(void)
{
	if (specialstageinfo.valid == false)
		return NULL;

	if (specialstageinfo.ufo == NULL
	|| P_MobjWasRemoved(specialstageinfo.ufo))
		return NULL;

	if (specialstageinfo.ufo->health <= 1) //UFOEmeraldChase(specialstageinfo.ufo)
		return NULL;

	return specialstageinfo.ufo;
}

/*--------------------------------------------------
	void K_FadeOutSpecialMusic(UINT32 distance)

		See header file for description.
--------------------------------------------------*/
void K_FadeOutSpecialMusic(UINT32 distance)
{
	if (specialstageinfo.valid == false)
	{
		return;
	}

	const UINT32 threshold = FixedMul(16000, mapobjectscale);

	Music_LevelVolume(min(distance, threshold) * 100 / threshold);
}
