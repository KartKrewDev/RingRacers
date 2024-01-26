#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "core/static_vec.hpp"
#include "v_draw.hpp"

#include "g_game.h"
#include "k_battle.h"
#include "k_hud.h"
#include "k_kart.h"
#include "k_objects.h"
#include "k_specialstage.h"
#include "m_fixed.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_draw.h"
#include "r_fps.h"
#include "r_main.h"
#include "st_stuff.h"
#include "v_video.h"

#ifdef WIN32
#undef near
#undef far
#endif

using namespace srb2;

extern "C" consvar_t cv_debughudtracker, cv_battleufotest;

namespace
{

enum class Visibility
{
	kVisible,
	kTransparent,
	kFlicker,
};

struct TargetTracking
{
	static constexpr int kMaxLayers = 2;

	struct Animation
	{
		int frames;
		int tics_per_frame;
		StaticVec<patch_t**, kMaxLayers> layers;
		int32_t video_flags = 0;
	};

	struct Graphics
	{
		struct SplitscreenPair
		{
			Animation p1;
			std::optional<Animation> p4;
		};

		SplitscreenPair near;
		std::optional<SplitscreenPair> far;
	};

	mobj_t* mobj;
	trackingResult_t result;
	fixed_t camDist;
	bool foreground;
	playertagtype_t nametag;

	skincolornum_t color() const
	{
		switch (mobj->type)
		{
		case MT_OVERTIME_CENTER:
			return SKINCOLOR_BLUE;

		case MT_MONITOR:
		case MT_EMERALD:
			return static_cast<skincolornum_t>(mobj->color);

		case MT_PLAYER:
			return player_emeralds_color();

		case MT_SUPER_FLICKY:
			return Obj_SuperFlickyOwner(mobj) ? static_cast<skincolornum_t>(Obj_SuperFlickyOwner(mobj)->color) : SKINCOLOR_NONE;

		default:
			return SKINCOLOR_NONE;
		}
	}

	Animation animation() const
	{
		const fixed_t farDistance = 1280 * mapobjectscale;
		bool useNear = (camDist < farDistance);

		Graphics gfx = graphics();
		Graphics::SplitscreenPair& pair = useNear || !gfx.far ? gfx.near : *gfx.far;
		Animation& anim = r_splitscreen <= 1 || !pair.p4 ? pair.p1 : *pair.p4;

		return anim;
	}

	bool uses_off_screen_arrow() const
	{
		switch (mobj->type)
		{
		case MT_SPRAYCAN:
			return false;

		default:
			return true;
		}
	}

	StaticVec<uint32_t, 7> player_emeralds_vec() const
	{
		StaticVec<uint32_t, 7> emeralds;

		const player_t* player = mobj->player;

		if (player == nullptr)
		{
			return emeralds;
		}

		for (int i = 0; i < 7; ++i)
		{
			const uint32_t emeraldFlag = (1U << i);

			if (player->emeralds & emeraldFlag)
			{
				emeralds.push_back(emeraldFlag);
			}
		}

		return emeralds;
	}

	skincolornum_t player_emeralds_color() const
	{
		const StaticVec emeralds = player_emeralds_vec();

		if (emeralds.empty())
		{
			return SKINCOLOR_NONE;
		}

		constexpr tic_t kPeriod = TICRATE / 2;
		const int idx = (leveltime / kPeriod) % emeralds.size();

		return static_cast<skincolornum_t>(K_GetChaosEmeraldColor(emeralds[idx]));
	}

	const uint8_t* colormap() const
	{
		const skincolornum_t clr = color();

		if (clr != SKINCOLOR_NONE)
		{
			return R_GetTranslationColormap(TC_RAINBOW, clr, GTC_CACHE);
		}

		return nullptr;
	}

	bool is_player_nametag_on_screen() const
	{
		const player_t* player = mobj->player;

		if (nametag == PLAYERTAG_NONE)
		{
			return false;
		}

		if (player->spectator)
		{
			// Not in-game
			return false;
		}

		if (mobj->renderflags & K_GetPlayerDontDrawFlag(stplyr))
		{
			// Invisible on this screen
			return false;
		}

		if (camDist > 8192*mapobjectscale)
		{
			// Too far away
			return false;
		}

		if (!P_CheckSight(stplyr->mo, const_cast<mobj_t*>(mobj)))
		{
			// Can't see
			return false;
		}

		return true;
	}

private:
	Graphics graphics() const
	{
		using layers = decltype(Animation::layers);
		switch (mobj->type)
		{
		case MT_SUPER_FLICKY:
			return {
				{ // Near
					{4, 2, {kp_superflickytarget[0]}}, // 1P
					{{4, 2, {kp_superflickytarget[1]}}}, // 4P
				},
			};

		case MT_SPRAYCAN:
			return {
				{ // Near
					{6, 2, {kp_spraycantarget_near[0]}, V_ADD}, // 1P
					{{6, 2, {kp_spraycantarget_near[1]}, V_ADD}}, // 4P
				},
				{{ // Far
					{6, 2, {kp_spraycantarget_far[0]}, V_ADD}, // 1P
					{{6, 2, {kp_spraycantarget_far[1]}, V_ADD}}, // 4P
				}},
			};

		case MT_BATTLEUFO_SPAWNER:
			return {};

		default:
			return {
				{ // Near
					{8, 2, {kp_capsuletarget_near[0]}}, // 1P
					{{8, 2, {kp_capsuletarget_near[1]}}}, // 4P
				},
				{{ // Far
					{2, 3, foreground ?
						layers {kp_capsuletarget_far[0], kp_capsuletarget_far_text} :
						layers {kp_capsuletarget_far[0]}}, // 1P
					{{2, 3, {kp_capsuletarget_far[1]}}}, // 4P
				}},
			};
		}
	}
};

bool is_player_tracking_target(player_t *player = stplyr)
{
	if ((gametyperules & (GTR_BUMPERS|GTR_CLOSERPLAYERS)) != (GTR_BUMPERS|GTR_CLOSERPLAYERS))
	{
		return false;
	}

	if (K_Cooperative())
	{
		return false;
	}

	if (player == nullptr)
	{
		return false;
	}

	if (player->spectator)
	{
		return false;
	}

	if (inDuel)
	{
		// Always draw targets in 1v1 but don't draw player's
		// own target on their own viewport.
		return player != stplyr;
	}

	// Except for DUEL mode, Overtime hides all TARGETs except
	// the kiosk.
	if (battleovertime.enabled)
	{
		return false;
	}

	if (player->emeralds != 0 && K_IsPlayerWanted(stplyr))
	{
		// The player who is about to win because of emeralds
		// gets a TARGET on them
		if (K_NumEmeralds(player) == 6) // 6 out of 7
		{
			return true;
		}

		// WANTED player sees TARGETs on players holding
		// emeralds
		if (K_IsPlayerWanted(stplyr))
		{
			return true;
		}
	}

	return K_IsPlayerWanted(player);
}

bool is_object_tracking_target(const mobj_t* mobj)
{
	switch (mobj->type)
	{
	case MT_BATTLECAPSULE:
	case MT_CDUFO:
		return battleprisons;

	case MT_PLAYER:
		return is_player_tracking_target(mobj->player);

	case MT_OVERTIME_CENTER:
		return inDuel == false && battleovertime.enabled;

	case MT_EMERALD:
		return Obj_EmeraldCanHUDTrack(mobj) &&
			((specialstageinfo.valid && specialstageinfo.ufo) || is_player_tracking_target());

	case MT_MONITOR:
		return is_player_tracking_target() && Obj_MonitorGetEmerald(mobj) != 0;

	case MT_SUPER_FLICKY:
		return Obj_IsSuperFlickyWhippable(mobj, stplyr->mo);

	case MT_SPRAYCAN:
		return !(mobj->renderflags & (RF_TRANSMASK | RF_DONTDRAW)) && // the spraycan wasn't collected yet
			P_CheckSight(stplyr->mo, const_cast<mobj_t*>(mobj));

	case MT_BATTLEUFO_SPAWNER:
		return cv_battleufotest.value;

	default:
		return false;
	}
}

Visibility is_object_visible(const mobj_t* mobj)
{
	switch (mobj->type)
	{
	case MT_SPRAYCAN:
	case MT_SUPER_FLICKY:
		// Always flickers.
		return Visibility::kFlicker;

	default:
		// Transparent when not visible.
		return P_CheckSight(stplyr->mo, const_cast<mobj_t*>(mobj)) ? Visibility::kVisible : Visibility::kTransparent;
	}
}

void K_DrawTargetTracking(const TargetTracking& target)
{
	if (target.nametag != PLAYERTAG_NONE)
	{
		K_DrawPlayerTag(target.result.x, target.result.y, target.mobj->player, target.nametag);
		return;
	}

	Visibility visibility = is_object_visible(target.mobj);

	if (visibility == Visibility::kFlicker && (leveltime & 1))
	{
		return;
	}

	const uint8_t* colormap = target.colormap();

	const trackingResult_t& result = target.result;
	int32_t timer = 0;

	if (result.onScreen == false)
	{
		// Off-screen, draw alongside the borders of the screen.
		// Probably the most complicated thing.

		if (target.uses_off_screen_arrow() == false)
		{
			return;
		}

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
		arrowDir.y = P_MobjFlip(target.mobj) * FRACUNIT;

		// Simply pointing towards the result doesn't work, so inaccurate hack...
		borderDir.x = FixedMul(
			FixedMul(
				FINESINE((result.angle >> ANGLETOFINESHIFT) & FINEMASK),
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

		if (target.mobj->type == MT_BATTLECAPSULE
			|| target.mobj->type == MT_CDUFO)
		{
			targetPatch = kp_capsuletarget_icon[timer & 1];
		}

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

		if (targetPatch)
		{
			targetPos.x = arrowPos.x - (arrowDir.x * 12);
			targetPos.y = arrowPos.y - (arrowDir.y * 12);

			targetPos.x -= (targetPatch->width << FRACBITS) >> 1;
			targetPos.y -= (targetPatch->height << FRACBITS) >> 1;
		}

		arrowPos.x -= (arrowPatch->width << FRACBITS) >> 1;
		arrowPos.y -= (arrowPatch->height << FRACBITS) >> 1;

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

		if (targetPatch)
		{
			V_DrawFixedPatch(targetPos.x, targetPos.y, FRACUNIT, V_SPLITSCREEN, targetPatch, colormap);
		}

		V_DrawFixedPatch(arrowPos.x, arrowPos.y, FRACUNIT, V_SPLITSCREEN | arrowFlags, arrowPatch, colormap);
	}
	else
	{
		// Draw simple overlay.
		vector2_t targetPos = {result.x, result.y};
		INT32 trans = [&]
		{
			switch (visibility)
			{
			case Visibility::kTransparent:
				return V_30TRANS;

			default:
				return target.foreground ? 0 : V_80TRANS;
			}
		}();

		TargetTracking::Animation anim = target.animation();

		for (patch_t** array : anim.layers)
		{
			patch_t* patch = array[(leveltime / anim.tics_per_frame) % anim.frames];

			V_DrawFixedPatch(
				targetPos.x - ((patch->width << FRACBITS) >> 1),
				targetPos.y - ((patch->height << FRACBITS) >> 1),
				FRACUNIT,
				V_SPLITSCREEN | anim.video_flags | trans,
				patch,
				colormap
			);
		};

		if (target.mobj->type == MT_BATTLEUFO_SPAWNER) // debug
		{
			using srb2::Draw;
			Draw(FixedToFloat(result.x), FixedToFloat(result.y))
				.flags(V_SPLITSCREEN)
				.font(Draw::Font::kThin)
				.align(Draw::Align::kCenter)
				.text("BUFO ID: {}", Obj_BattleUFOSpawnerID(target.mobj));
		}
	}
}

void K_CullTargetList(std::vector<TargetTracking>& targetList)
{
	constexpr int kBlockSize = 20;
	constexpr int kXBlocks = BASEVIDWIDTH / kBlockSize;
	constexpr int kYBlocks = BASEVIDHEIGHT / kBlockSize;
	bool map[kXBlocks][kYBlocks] = {};

	constexpr fixed_t kTrackerRadius = 30*FRACUNIT/2; // just an approximation of common HUD tracker

	int debugColorCycle = 0;

	std::for_each(
		targetList.rbegin(),
		targetList.rend(),
		[&](TargetTracking& tr)
		{
			if (tr.result.onScreen == false)
			{
				return;
			}

			if (tr.nametag != PLAYERTAG_NONE)
			{
				return;
			}

			fixed_t x1 = std::max(((tr.result.x - kTrackerRadius) / kBlockSize) / FRACUNIT, 0);
			fixed_t x2 = std::min(((tr.result.x + kTrackerRadius) / kBlockSize) / FRACUNIT, kXBlocks - 1);
			fixed_t y1 = std::max(((tr.result.y - kTrackerRadius) / kBlockSize) / FRACUNIT, 0);
			fixed_t y2 = std::min(((tr.result.y + kTrackerRadius) / kBlockSize) / FRACUNIT, kYBlocks - 1);

			bool allMine = true;

			for (fixed_t x = x1; x <= x2; ++x)
			{
				for (fixed_t y = y1; y <= y2; ++y)
				{
					if (map[x][y])
					{
						allMine = false;
					}
					else
					{
						map[x][y] = true;

						if (cv_debughudtracker.value)
						{
							V_DrawFill(
								x * kBlockSize,
								y * kBlockSize,
								kBlockSize,
								kBlockSize,
								(39 + debugColorCycle) | V_SPLITSCREEN
							);
						}
					}
				}
			}

			if (allMine)
			{
				// This tracker claims every square
				tr.foreground = true;
			}

			if (++debugColorCycle > 8)
			{
				debugColorCycle = 0;
			}
		}
	);
}

}; // namespace

void K_drawTargetHUD(const vector3_t* origin, player_t* player)
{
	std::vector<TargetTracking> targetList;

	mobj_t* mobj = nullptr;
	mobj_t* next = nullptr;

	for (mobj = trackercap; mobj; mobj = next)
	{
		next = mobj->itnext;

		if (mobj->health <= 0)
		{
			continue;
		}

		bool tracking = is_object_tracking_target(mobj);
		playertagtype_t nametag = mobj->player ? K_WhichPlayerTag(mobj->player) : PLAYERTAG_NONE;

		if (tracking == false && nametag == PLAYERTAG_NONE)
		{
			continue;
		}

		vector3_t pos = {
			R_InterpolateFixed(mobj->old_x, mobj->x) + mobj->sprxoff,
			R_InterpolateFixed(mobj->old_y, mobj->y) + mobj->spryoff,
			R_InterpolateFixed(mobj->old_z, mobj->z) + mobj->sprzoff + (mobj->height >> 1),
		};

		TargetTracking tr;

		tr.mobj = mobj;
		tr.camDist = R_PointToDist2(origin->x, origin->y, pos.x, pos.y);
		tr.foreground = false;
		tr.nametag = PLAYERTAG_NONE;

		if (tracking)
		{
			K_ObjectTracking(&tr.result, &pos, false);
			targetList.push_back(tr);
		}

		if (!mobj->player)
		{
			continue;
		}

		tr.nametag = nametag;

		if (tr.is_player_nametag_on_screen())
		{
			fixed_t headOffset = 36*mobj->scale;
			if (stplyr->mo->eflags & MFE_VERTICALFLIP)
			{
				pos.z -= headOffset;
			}
			else
			{
				pos.z += headOffset;
			}
			K_ObjectTracking(&tr.result, &pos, false);

			if (tr.result.onScreen == true)
			{
				targetList.push_back(tr);
			}
		}
	}

	// Sort by distance from camera. Further trackers get
	// drawn first so nearer ones draw over them.
	std::sort(targetList.begin(), targetList.end(), [](const auto& a, const auto& b) { return a.camDist > b.camDist; });

	K_CullTargetList(targetList);

	std::for_each(targetList.cbegin(), targetList.cend(), K_DrawTargetTracking);
}
