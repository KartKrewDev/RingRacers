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

struct specialStage specialStage;

/*--------------------------------------------------
	void K_ResetSpecialStage(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetSpecialStage(void)
{
	memset(&specialStage, 0, sizeof(struct specialStage));
}

/*--------------------------------------------------
	void K_InitSpecialStage(void)

		See header file for description.
--------------------------------------------------*/
void K_InitSpecialStage(void)
{
	INT32 i;

	specialStage.beamDist = UINT32_MAX; // TODO: make proper value
	P_SetTarget(&specialStage.ufo, Obj_CreateSpecialUFO());

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];
		if (player->spectator == true)
		{
			continue;
		}

		if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			continue;
		}

		// Rolling start? lol
		P_InstaThrust(player->mo, player->mo->angle, K_GetKartSpeed(player, false, false));
	}
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

	if (specialStage.beamDist <= moveDist)
	{
		specialStage.beamDist = 0;

		// TODO: Fail Special Stage
	}
	else
	{
		specialStage.beamDist -= moveDist;
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

		if (player->distancetofinish > specialStage.beamDist)
		{
			P_DoTimeOver(player);
		}
	}
}

/*--------------------------------------------------
	void K_TickSpecialStage(void)

		See header file for description.
--------------------------------------------------*/
void K_TickSpecialStage(void)
{
	if (specialStage.active == false)
	{
		return;
	}

	K_MoveExitBeam();
}
