#include <algorithm>
#include <cstddef>

#include "k_hud.h"
#include "m_fixed.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_fps.h"
#include "r_main.h"
#include "v_video.h"

namespace
{

struct TargetTracking
{
	mobj_t* mobj;
	vector3_t point;
	fixed_t camDist;
};

void K_DrawTargetTracking(TargetTracking* target)
{
	trackingResult_t result = {};
	int32_t timer = 0;

	K_ObjectTracking(&result, &target->point, false);

	if (result.onScreen == false)
	{
		// Off-screen, draw alongside the borders of the screen.
		// Probably the most complicated thing.

		int32_t scrVal = 240;
		vector2_t screenSize = {};

		int32_t borderSize = 7;
		vector2_t borderWin = {};
		vector2_t borderDir = {};
		fixed_t borderLen = FRACUNIT;

		vector2_t arrowDir = {};

		vector2_t arrowPos = {};
		patch_t* arrowPatch = nullptr;
		int32_t arrowFlags = 0;

		vector2_t targetPos = {};
		patch_t* targetPatch = nullptr;

		timer = (leveltime / 3);

		screenSize.x = vid.width / vid.dupx;
		screenSize.y = vid.height / vid.dupy;

		if (r_splitscreen >= 2)
		{
			// Half-wide screens
			screenSize.x >>= 1;
			borderSize >>= 1;
		}

		if (r_splitscreen >= 1)
		{
			// Half-tall screens
			screenSize.y >>= 1;
		}

		scrVal = std::max(screenSize.x, screenSize.y) - 80;

		borderWin.x = screenSize.x - borderSize;
		borderWin.y = screenSize.y - borderSize;

		arrowDir.x = 0;
		arrowDir.y = P_MobjFlip(target->mobj) * FRACUNIT;

		// Simply pointing towards the result doesn't work, so inaccurate hack...
		borderDir.x = FixedMul(
			FixedMul(
				FINESINE((-result.angle >> ANGLETOFINESHIFT) & FINEMASK),
				FINECOSINE((-result.pitch >> ANGLETOFINESHIFT) & FINEMASK)
			),
			result.fov
		);

		borderDir.y = FixedMul(FINESINE((-result.pitch >> ANGLETOFINESHIFT) & FINEMASK), result.fov);

		borderLen = R_PointToDist2(0, 0, borderDir.x, borderDir.y);

		if (borderLen > 0)
		{
			borderDir.x = FixedDiv(borderDir.x, borderLen);
			borderDir.y = FixedDiv(borderDir.y, borderLen);
		}
		else
		{
			// Eh just put it at the bottom.
			borderDir.x = 0;
			borderDir.y = FRACUNIT;
		}

		targetPatch = kp_capsuletarget_icon[timer & 1];

		if (abs(borderDir.x) > abs(borderDir.y))
		{
			// Horizontal arrow
			arrowPatch = kp_capsuletarget_arrow[1][timer & 1];
			arrowDir.y = 0;

			if (borderDir.x < 0)
			{
				// LEFT
				arrowDir.x = -FRACUNIT;
			}
			else
			{
				// RIGHT
				arrowDir.x = FRACUNIT;
			}
		}
		else
		{
			// Vertical arrow
			arrowPatch = kp_capsuletarget_arrow[0][timer & 1];
			arrowDir.x = 0;

			if (borderDir.y < 0)
			{
				// UP
				arrowDir.y = -FRACUNIT;
			}
			else
			{
				// DOWN
				arrowDir.y = FRACUNIT;
			}
		}

		arrowPos.x = (screenSize.x >> 1) + FixedMul(scrVal, borderDir.x);
		arrowPos.y = (screenSize.y >> 1) + FixedMul(scrVal, borderDir.y);

		arrowPos.x = std::clamp(arrowPos.x, borderSize, borderWin.x) * FRACUNIT;
		arrowPos.y = std::clamp(arrowPos.y, borderSize, borderWin.y) * FRACUNIT;

		targetPos.x = arrowPos.x - (arrowDir.x * 12);
		targetPos.y = arrowPos.y - (arrowDir.y * 12);

		arrowPos.x -= (arrowPatch->width << FRACBITS) >> 1;
		arrowPos.y -= (arrowPatch->height << FRACBITS) >> 1;

		targetPos.x -= (targetPatch->width << FRACBITS) >> 1;
		targetPos.y -= (targetPatch->height << FRACBITS) >> 1;

		if (arrowDir.x < 0)
		{
			arrowPos.x += arrowPatch->width << FRACBITS;
			arrowFlags |= V_FLIP;
		}

		if (arrowDir.y < 0)
		{
			arrowPos.y += arrowPatch->height << FRACBITS;
			arrowFlags |= V_VFLIP;
		}

		V_DrawFixedPatch(targetPos.x, targetPos.y, FRACUNIT, V_SPLITSCREEN, targetPatch, nullptr);

		V_DrawFixedPatch(arrowPos.x, arrowPos.y, FRACUNIT, V_SPLITSCREEN | arrowFlags, arrowPatch, nullptr);
	}
	else
	{
		// Draw simple overlay.
		const fixed_t farDistance = 1280 * mapobjectscale;
		bool useNear = (target->camDist < farDistance);

		patch_t* targetPatch = nullptr;
		vector2_t targetPos = {};

		bool visible = P_CheckSight(stplyr->mo, target->mobj);

		if (visible == false && (leveltime & 1))
		{
			// Flicker when not visible.
			return;
		}

		targetPos.x = result.x;
		targetPos.y = result.y;

		if (useNear == true)
		{
			timer = (leveltime / 2);
			targetPatch = kp_capsuletarget_near[timer % 8];
		}
		else
		{
			timer = (leveltime / 3);
			targetPatch = kp_capsuletarget_far[timer & 1];
		}

		targetPos.x -= (targetPatch->width << FRACBITS) >> 1;
		targetPos.y -= (targetPatch->height << FRACBITS) >> 1;

		V_DrawFixedPatch(targetPos.x, targetPos.y, FRACUNIT, V_SPLITSCREEN, targetPatch, nullptr);
	}
}

}; // namespace

void K_drawTargetHUD(const vector3_t* origin, player_t* player)
{
	constexpr std::size_t kMaxTargetHUD = 32;

	std::size_t i, j;

	TargetTracking targetList[kMaxTargetHUD];
	std::size_t targetListLen = 0;

	mobj_t* mobj = nullptr;
	mobj_t* next = nullptr;

	for (mobj = trackercap; mobj; mobj = next)
	{
		TargetTracking* target = nullptr;

		next = mobj->itnext;

		if (mobj->health <= 0)
		{
			continue;
		}

		if (mobj->type != MT_BATTLECAPSULE)
		{
			continue;
		}

		target = &targetList[targetListLen];

		target->mobj = mobj;
		target->point.x = R_InterpolateFixed(mobj->old_x, mobj->x);
		target->point.y = R_InterpolateFixed(mobj->old_y, mobj->y);
		target->point.z = R_InterpolateFixed(mobj->old_z, mobj->z);
		target->point.z += (mobj->height >> 1);
		target->camDist = R_PointToDist2(origin->x, origin->y, target->point.x, target->point.y);

		targetListLen++;

		if (targetListLen >= kMaxTargetHUD)
		{
			break;
		}
	}

	if (targetListLen > 0)
	{
		// Sort by distance from camera.
		if (targetListLen > 1)
		{
			for (i = 0; i < targetListLen - 1; i++)
			{
				std::size_t swap = i;

				for (j = i + 1; j < targetListLen; j++)
				{
					TargetTracking* cj = &targetList[j];
					TargetTracking* cSwap = &targetList[swap];

					if (cj->camDist > cSwap->camDist)
					{
						swap = j;
					}
				}

				if (swap != i)
				{
					TargetTracking temp = targetList[swap];
					targetList[swap] = targetList[i];
					targetList[i] = temp;
				}
			}
		}

		for (i = 0; i < targetListLen; i++)
		{
			K_DrawTargetTracking(&targetList[i]);
		}
	}
}
