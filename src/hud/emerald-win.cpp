// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>

#include "../v_draw.hpp"

#include "../doomdef.h"
#include "../i_time.h"
#include "../k_battle.h"
#include "../k_hud.h"
#include "../m_easing.h"
#include "../m_fixed.h"
#include "../p_tick.h"
#include "../screen.h"

using srb2::Draw;

namespace
{

fixed_t interval(tic_t t, tic_t s, tic_t d)
{
	return (std::min(std::max(t, s) - s, d) * FRACUNIT) / d;
}

}; // namespace

void K_drawEmeraldWin(boolean overlay)
{
	if (leveltime < g_emeraldWin)
	{
		return;
	}

	constexpr float kScale = 0.5;
	constexpr int kWidth = (69 + 4) * 2 * kScale;
	constexpr int kMid = (72 * kScale) / 2;
	constexpr int kYOffset = (68 * kScale) - kMid;

	constexpr int kTop = 86;
	constexpr int kBot = 129;

	constexpr tic_t kDelay = 24;
	constexpr tic_t kSlide = 12;

	constexpr tic_t kFlashStart = (6 * kDelay) + kSlide;
	constexpr tic_t kFlash = 10;

	INT32 flags = 0;

	tic_t t = leveltime - g_emeraldWin;

	if (overlay)
	{
		if (t >= kFlashStart && t - kFlashStart <= kFlash)
		{
			flags = V_ADD | (Easing_InOutSine(interval(t, kFlashStart, kFlash), 0, 9) << V_ALPHASHIFT);
		}
		else
		{
			return;
		}
	}
	else
	{
		flags = (I_GetTime() & 1) ? V_ADD : 0;
	}

	patch_t* emer = Draw::cache_patch("EMRCA0");
	Draw row = Draw(BASEVIDWIDTH / 2, kYOffset).scale(kScale).flags(flags);
	//Draw(0, row.y()).size(BASEVIDWIDTH, 1).fill(35);

	Draw top = row.y(kTop);
	Draw bot = row.xy(-kWidth / 2, kBot);

	auto put = [&](Draw& row, int offscreen, int x, int n)
	{
		row
			.xy(x * kWidth, Easing_OutSine(interval(t, kDelay * n, kSlide), offscreen, 0))
			.colormap(static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + n))
			.patch(emer);
	};

	put(top, -kTop - kMid, -1, 3);
	put(top, -kTop - kMid, 0, 0);
	put(top, -kTop - kMid, 1, 4);

	put(bot, 200 - kBot + kMid, -1, 5);
	put(bot, 200 - kBot + kMid, 0, 1);
	put(bot, 200 - kBot + kMid, 1, 2);
	put(bot, 200 - kBot + kMid, 2, 6);
}
