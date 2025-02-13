// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  duel-bomb.c
/// \brief Duel mode bombs.

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_respawn.h"
#include "../k_collide.h"

#define bomb_dir(o) ((o)->movedir)

static fixed_t GetBombSpeed(mobj_t *bomb)
{
	return FixedMul(bomb->info->speed, bomb->scale);
}

static void UpdateBombMovement(mobj_t *bomb)
{
	const fixed_t spd = GetBombSpeed(bomb);
	bomb->momx = FixedMul(spd, FINECOSINE(bomb_dir(bomb) >> ANGLETOFINESHIFT));
	bomb->momy = FixedMul(spd,   FINESINE(bomb_dir(bomb) >> ANGLETOFINESHIFT));
}

void Obj_DuelBombThink(mobj_t *bomb)
{
	boolean grounded = P_IsObjectOnGround(bomb);

	if (grounded == true)
	{
		UpdateBombMovement(bomb);
	}
}

void Obj_DuelBombReverse(mobj_t *bomb)
{
	bomb_dir(bomb) += ANGLE_180;
	UpdateBombMovement(bomb);
}

void Obj_DuelBombTouch(mobj_t *bomb, mobj_t *toucher)
{
	player_t *player = toucher->player;
	mobj_t *boom = NULL;

	if (bomb->health <= 0 || toucher->health <= 0)
	{
		return;
	}

	if (player->flashing > 0 || player->hyudorotimer > 0 || P_PlayerInPain(player))
	{
		// No interaction
		return;
	}

	// Create explosion
	boom = P_SpawnMobjFromMobj(bomb, 0, 0, 0, MT_BOOMEXPLODE);
	boom->momz = 5 * boom->scale;
	boom->color = SKINCOLOR_KETCHUP;
	S_StartSound(boom, bomb->info->attacksound);

	if (player->invincibilitytimer > 0
		|| K_IsBigger(toucher, bomb) == true
		|| player->flamedash > 0)
	{
		// Kill without damaging.
		P_KillMobj(bomb, toucher, toucher, DMG_NORMAL);
		return;
	}

	P_DamageMobj(toucher, bomb, bomb, 1, DMG_TUMBLE);
	P_KillMobj(bomb, toucher, toucher, DMG_NORMAL);
}

void Obj_DuelBombInit(mobj_t *bomb)
{
	bomb_dir(bomb) = bomb->angle + ANGLE_90;
	UpdateBombMovement(bomb);
}
