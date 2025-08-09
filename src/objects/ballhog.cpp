// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ballhog.cpp
/// \brief Ballhog item code.

#include <algorithm>

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
#include "../k_collide.h"

namespace
{

struct hog_angles
{
	fixed_t x, y;
	hog_angles(fixed_t nx, fixed_t ny) : x(nx), y(ny) {}
};

std::vector<struct hog_angles> g_hogangles;

};

static void CalculateHogAngles(UINT8 n)
{
	const UINT8 total_hogs = n;

	// This algorithm should probably be replaced to
	// maximize more space covered, but the desired effect
	// is achieved for 1 to 5, which is the vast majority of uses.
	g_hogangles.clear();

	if (total_hogs == 0)
	{
		return;
	}

	if (total_hogs == 1 || total_hogs > 4)
	{
		// Add a point to the exact middle.
		g_hogangles.emplace_back(0, 0);
		n--;
	}

	if (total_hogs > 1)
	{
		const fixed_t base_radius = mobjinfo[MT_BALLHOG].radius * 12;
		fixed_t radius = base_radius;
		UINT8 max_points = 6;
		angle_t circle_offset = 0;

		if (total_hogs < 5)
		{
			// Reduce size to get more space covered.
			radius /= 2;
			max_points = 4;
		}

		while (n > 0)
		{
			const UINT8 add_points = std::min<UINT8>(n, max_points);

			angle_t angle = circle_offset;
			const angle_t angle_offset = ANGLE_MAX / add_points;

			for (UINT8 c = 0; c < add_points; c++)
			{
				g_hogangles.emplace_back(
					FixedMul(FINECOSINE( angle >> ANGLETOFINESHIFT ), radius),
					FixedMul(  FINESINE( angle >> ANGLETOFINESHIFT ), radius)
				);
				angle += angle_offset;
				n--;
			}

			radius += base_radius;
			circle_offset += ANGLE_MAX / max_points;
			max_points += (max_points / 2);
		}
	}
}

UINT8 K_HogChargeToHogCount(INT32 charge, UINT8 cap)
{
	return std::clamp<UINT8>((charge / BALLHOGINCREMENT), 0, cap);
}

static boolean HogReticuleEmulate(mobj_t *mobj)
{
	fixed_t x, y, z;

	//I_Assert(mobj != NULL);
	//I_Assert(P_MobjWasRemoved(mobj) == false);

	x = mobj->x, y = mobj->y, z = mobj->z;

	//if (mobj->momx || mobj->momy)
	{
		if (P_XYMovement(mobj) == false)
		{
			return true;
		}

		if (P_MobjWasRemoved(mobj) == true)
		{
			return true;
		}
	}

	//if (mobj->momz)
	{
		if (P_ZMovement(mobj) == false)
		{
			return true; // mobj was removed
		}

		if (P_MobjWasRemoved(mobj) == true)
		{
			return true;
		}
	}

	if (P_MobjWasRemoved(mobj) == true)
	{
		return true;
	}

	if (x == mobj->x && y == mobj->y && z == mobj->z)
	{
		return true;
	}

	return false;
}

static void HogReticuleTest(player_t *player, vector3_t *ret, fixed_t final_scale)
{
	// Emulate the movement of a tossed ballhog
	// until it hits something.

	fixed_t dir = FRACUNIT;
	if (player->throwdir == 1)
	{
		dir = 2*FRACUNIT;
	}
	else if (player->throwdir == -1)
	{
		dir = FRACUNIT/2;
	}

	// Use pre-determined speed for tossing
	fixed_t proj_speed = FixedMul(82 * FRACUNIT, K_GetKartGameSpeedScalar(gamespeed));

	// Scale to map scale
	// Intentionally NOT player scale, that doesn't work.
	proj_speed = FixedMul(proj_speed, mapobjectscale);

	// Shoot forward
	//P_MoveOrigin(mo, player->mo->x, player->mo->y, player->mo->z + (player->mo->height / 2));
	// FINE! YOU WIN! I'll make an object every time I need to test this...
	mobj_t *mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height / 2), MT_BALLHOG_RETICULE_TEST);
	mo->fuse = 2; // if something goes wrong, destroy it

	mo->angle = player->mo->angle;

	// These are really weird so let's make it a very specific case to make SURE it works...
	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		mo->z -= player->mo->height;
		mo->eflags |= MFE_VERTICALFLIP;
		mo->flags2 |= (player->mo->flags2 & MF2_OBJECTFLIP);
	}

	P_SetTarget(&mo->target, player->mo);
	mo->threshold = 10;

	mo->extravalue2 = dir;

	fixed_t proj_speed_z = ((20 * FRACUNIT) + (10 * dir)) + (FixedDiv(player->mo->momz, mapobjectscale) * P_MobjFlip(player->mo)); // Also intentionally not player scale
	P_SetObjectMomZ(mo, proj_speed_z, false);

	angle_t fa = (player->mo->angle >> ANGLETOFINESHIFT);
	mo->momx = player->mo->momx + FixedMul(FINECOSINE(fa), FixedMul(proj_speed, dir));
	mo->momy = player->mo->momy + FixedMul(  FINESINE(fa), FixedMul(proj_speed, dir));

	if (mo->eflags & MFE_UNDERWATER)
	{
		mo->momz = (117 * mo->momz) / 200;
	}

	P_SetScale(mo, final_scale);
	mo->destscale = final_scale;

	// Contra spread shot scale up
	mo->destscale = mo->destscale << 1;
	mo->scalespeed = abs(mo->destscale - mo->scale) / (2*TICRATE);

	ret->x = mo->x;
	ret->y = mo->y;
	ret->z = mo->z;

	constexpr INT32 max_iteration = 256;
	for (INT32 i = 0; i < max_iteration; i++)
	{
#if 0
		mobj_t *test = P_SpawnMobj(mo->x, mo->y, mo->z, MT_THOK);
		test->tics = 2;
#endif

		if (HogReticuleEmulate(mo) == true)
		{
			break;
		}

		ret->x = mo->x;
		ret->y = mo->y;
		ret->z = mo->z;
	}

	P_RemoveMobj(mo);
}

void K_UpdateBallhogReticules(player_t *player, UINT8 num_hogs, boolean on_release)
{
	if (player == nullptr)
	{
		return;
	}

	if (player->mo == nullptr || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	const UINT8 start_hogs = num_hogs;
#if 1
	if (start_hogs == 0)
	{
		return;
	}
#endif

	CalculateHogAngles(num_hogs);

	// Calculate center positon
	fixed_t final_scale = K_ItemScaleFromConst(K_GetItemScaleConst(player->mo->scale));

	vector3_t center = {0, 0, 0};
	HogReticuleTest(player, &center, final_scale); // Originally this was called for everything, but it's more optimized to only run 1 prediction.

	constexpr tic_t kBallhogReticuleTime = (TICRATE * 3 / 4);

	// Update existing reticules.
	mobj_t *reticule = player->ballhogreticule;
	while (reticule != nullptr && P_MobjWasRemoved(reticule) == false)
	{
		mobj_t *next = reticule->hnext;
		boolean removed = false;

		if (num_hogs > 0)
		{
			const UINT8 old_hogs = reticule->extravalue1;

			UINT8 angle_index = num_hogs - 1;
			fixed_t x_offset = FixedMul(g_hogangles[angle_index].x, final_scale);
			fixed_t y_offset = FixedMul(g_hogangles[angle_index].y, final_scale);

			if (on_release == true)
			{
				reticule->extravalue1 = start_hogs;
				P_MoveOrigin(
					reticule,
					center.x + x_offset,
					center.y + y_offset,
					center.z
				);
			}
			else
			{
				if (start_hogs != old_hogs)
				{
					// Reset to the middle
					P_SetOrigin(reticule, center.x, center.y, center.z);
					reticule->extravalue1 = start_hogs;
				}

				// Move to new position
				P_MoveOrigin(
					reticule,
					reticule->x + (((center.x + x_offset) - reticule->x) / (BALLHOGINCREMENT / 2)),
					reticule->y + (((center.y + y_offset) - reticule->y) / (BALLHOGINCREMENT / 2)),
					reticule->z + ((center.z - reticule->z) / (BALLHOGINCREMENT / 2))
				);
			}

			reticule->tics = kBallhogReticuleTime;
			reticule->color = player->skincolor;

			reticule->destscale = final_scale * 2;
			P_SetScale(reticule, reticule->destscale);

			num_hogs--;
		}
#if 0
		else
		{
			// Too many reticules exist, so remove the remainder.
			P_RemoveMobj(reticule);
			removed = true;
		}
#endif

		if (next == nullptr || P_MobjWasRemoved(next) == true)
		{
			break;
		}

		if (removed == true)
		{
			P_SetTarget(&next->hprev, nullptr);
		}
		reticule = next;
	}

	// Not enough reticules exist, so make new ones.
	while (num_hogs > 0)
	{
		mobj_t *new_reticule = P_SpawnMobjFromMobj((reticule != nullptr) ? reticule : player->mo, 0, 0, 0, MT_BALLHOG_RETICULE);
		if (new_reticule == nullptr)
		{
			break;
		}

		if (reticule != nullptr)
		{
			P_SetTarget(&reticule->hnext, new_reticule);
			P_SetTarget(&new_reticule->hprev, reticule);
		}
		else
		{
			P_SetTarget(&player->ballhogreticule, new_reticule);
			P_SetTarget(&new_reticule->hprev, player->mo);
		}

		new_reticule->extravalue1 = start_hogs;

		new_reticule->tics = kBallhogReticuleTime;
		new_reticule->color = player->skincolor;

		new_reticule->destscale = final_scale * 2;
		P_SetScale(new_reticule, new_reticule->destscale);

		if (on_release == true)
		{
			UINT8 angle_index = num_hogs - 1;
			fixed_t x_offset = FixedMul(g_hogangles[angle_index].x, final_scale);
			fixed_t y_offset = FixedMul(g_hogangles[angle_index].y, final_scale);

			P_SetOrigin(new_reticule, center.x + x_offset, center.y + y_offset, center.z);
		}
		else
		{
			P_SetOrigin(new_reticule, center.x, center.y, center.z);
		}

		reticule = new_reticule;
		num_hogs--;
	}
}

void K_DoBallhogAttack(player_t *player, UINT8 num_hogs)
{
	// Update reticules instantly, then untie them to us.
	K_UpdateBallhogReticules(player, num_hogs, true);
	P_SetTarget(&player->ballhogreticule, nullptr);

	CalculateHogAngles(num_hogs);

	while (num_hogs > 0)
	{
		UINT8 angle_index = num_hogs - 1;

		fixed_t x_offset = g_hogangles[angle_index].x;
		fixed_t y_offset = g_hogangles[angle_index].y;

		K_ThrowKartItemEx(
			player,
			false, MT_BALLHOG, 1, 2,
			0,
			x_offset, y_offset
		);

		num_hogs--;
	}
}
