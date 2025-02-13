// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <functional>

#include <fmt/format.h>

#include "../core/static_vec.hpp"

#include "../doomstat.h"
#include "../g_game.h"
#include "../k_hud.h"
#include "../k_powerup.h"
#include "../p_local.h"
#include "../r_fps.h"
#include "../v_draw.hpp"

using srb2::Draw;

namespace
{

struct Icon
{
	kartitems_t powerup;
	tic_t time;

	Icon() {}

	explicit Icon(int k) :
		powerup(static_cast<kartitems_t>(k)),
		time(K_PowerUpRemaining(stplyr, powerup))
	{
	}

	int letter() const { return 'A' + (powerup - FIRSTPOWERUP); }

	bool operator <(const Icon& b) const { return time < b.time; }
	bool operator >(const Icon& b) const { return time > b.time; }
};

srb2::StaticVec<Icon, NUMPOWERUPS> get_powerup_list(bool ascending)
{
	srb2::StaticVec<Icon, NUMPOWERUPS> v;

	for (int k = FIRSTPOWERUP; k < ENDOFPOWERUPS; ++k)
	{
		Icon ico(k);

		if (ico.time)
		{
			v.push_back(ico);
		}
	}

	if (ascending)
	{
		std::stable_sort(v.begin(), v.end(), std::less<Icon>());
	}
	else
	{
		std::stable_sort(v.begin(), v.end(), std::greater<Icon>());
	}

	return v;
}

}; // namespace

void K_drawKartPowerUps(void)
{
	struct Offsets
	{
		Draw row;
		const char* sprite;
		int spr_x;
		int spr_y;
		int shift_x;
		int dir;
	};

	auto make_offsets = []() -> Offsets
	{
		auto make_drawer = [](int x, int y, Draw::Font font) -> Draw
		{
			return Draw(x, y).font(font).align(Draw::Align::kRight).flags(V_SLIDEIN);
		};

		const int viewnum = R_GetViewNumber();

		// 1/2P
		switch (r_splitscreen)
		{
		case 0:
			return { make_drawer(307, 58, Draw::Font::kZVote), "PWRU", -17, 7, -35, -1 };

		case 1:
			return { make_drawer(318, viewnum == 0 ? 58 : 147, Draw::Font::kPing), "PWRS", -9, 6, -19, -1 };
		}

		// 3/4P
		int x = 21;
		int y = 47;

		int dir = 1;

		switch (viewnum)
		{
		case 1:
		case 3:
			x = 318;
			dir = -1;
		}

		switch (viewnum)
		{
		case 2:
		case 3:
			y += 100;
		}

		return { make_drawer(x, y, Draw::Font::kPing), "PWRS", -9, 5, 19 * dir, dir };
	};

	Offsets i = make_offsets();

	srb2::StaticVec<Icon, NUMPOWERUPS> powerup_list = get_powerup_list(i.dir == -1);
	for (const Icon& ico : powerup_list)
	{
		i.row.xy(i.spr_x, i.spr_y)
			.colormap(static_cast<skincolornum_t>(stplyr->skincolor))
			.patch(fmt::format("{0}{1:c}L{1:c}R", i.sprite, ico.letter()).c_str());
		i.row.text("{}", (ico.time + (TICRATE / 2)) / TICRATE);
		i.row = i.row.x(i.shift_x);
	}
}
