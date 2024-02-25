// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <string>

#include <fmt/format.h>

#include "../math/vec.hpp"

#include "../g_input.h"
#include "../g_game.h"
#include "../i_joy.h"
#include "../k_hud.h"
#include "../v_draw.hpp"

using srb2::Draw;
using srb2::math::Vec2;

namespace
{

const char* dpad_suffix(const Vec2<float>& v)
{
	if (v.y > 0)
	{
		if (v.x < 0)
			return "UL";
		else if (v.x > 0)
			return "UR";
		else
			return "U";
	}
	else if (v.y < 0)
	{
		if (v.x < 0)
			return "DL";
		else if (v.x > 0)
			return "DR";
		else
			return "D";
	}
	else
	{
		if (v.x < 0)
			return "L";
		else if (v.x > 0)
			return "R";
		else
			return "N";
	}
}

}; // namespace

void K_DrawInputDisplay(INT32 x, INT32 y, UINT8 pid)
void K_DrawInputDisplay(INT32 x, INT32 y, INT32 flags, char mode, UINT8 pid, boolean local, boolean transparent)
{
	const menucmd_t& cmd = menucmd[pid];
	const std::string prefix = "PR_";
	auto gfx = [&](auto format, auto&&... args) { return prefix + fmt::format(format, args...); };
	auto but = [&](char key, INT32 gc) { return gfx(G_PlayerInputAnalog(pid, gc, 0) ? "BT{}B" : "BT{}", key); };

	Draw box(x, y);

	box.patch(gfx("CONT"));

	Vec2<float> dpad = {
		(G_PlayerInputAnalog(pid, gc_right, 0) - G_PlayerInputAnalog(pid, gc_left, 0)) / (float)JOYAXISRANGE,
		(G_PlayerInputAnalog(pid, gc_up, 0) - G_PlayerInputAnalog(pid, gc_down, 0)) / (float)JOYAXISRANGE,
	};

	box.patch(gfx("PAD{}", dpad_suffix(dpad)));
	box.patch(but('A', gc_a));
	box.patch(but('B', gc_b));
	box.patch(but('C', gc_c));
	box.patch(but('X', gc_x));
	box.patch(but('Y', gc_y));
	box.patch(but('Z', gc_z));
	box.patch(but('L', gc_l));
	box.patch(but('R', gc_r));
	box.patch(but('S', gc_start));
}
