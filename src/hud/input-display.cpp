// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <string>

#include <fmt/format.h>

#include "../math/vec.hpp"

#include "../g_input.h"
#include "../g_game.h"
#include "../i_joy.h"
#include "../k_hud.h"
#include "../k_kart.h"
#include "../m_easing.h"
#include "../p_tick.h"
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

void K_DrawInputDisplay(float x, float y, INT32 flags, char mode, UINT8 pid, boolean local, boolean transparent)
{
	auto fade_in = []
	{
		constexpr tic_t kStart = TICRATE;
		constexpr tic_t kDuration = TICRATE/2;
		fixed_t f = std::min(std::max(leveltime, kStart) - kStart, kDuration) * FRACUNIT / kDuration;
		return Easing_Linear(f, 0, 9);
	};
	auto alpha_to_flag = [](int alpha) { return (9 - alpha) << V_ALPHASHIFT; };

	// No assigned device? Well, you got to this screen somehow.
	// Use menu behavior to guess from connected devices.
	boolean guessinput = (G_GetDeviceForPlayer(pid) == -1);

	// PLEASE DO NOT DANNY FUCKING PHANTOM ON THE PROFILE CONTROLS MENU
	int alpha = (mode == '_') ? 9 : fade_in();

	if (alpha == 0)
		return;

	const ticcmd_t& cmd = players[displayplayers[pid]].cmd;
	const boolean analog = (mode == '4' || mode == '5') ? players[displayplayers[pid]].analoginput : false;
	const std::string prefix = fmt::format("PR{}", mode);
	auto gfx = [&](auto format, auto&&... args) { return prefix + fmt::format(format, args...); };
	auto but = [&](char key, INT32 gc, UINT32 bt)
	{
		bool press = local ? G_PlayerInputAnalog(pid, gc, guessinput) : ((cmd.buttons & bt) == bt);
		return gfx(press ? "BT{}B" : "BT{}", key);
	};

	Draw box = Draw(x, y).flags(flags);

	box.flags(alpha_to_flag(alpha / (transparent ? 2 : 1))).patch(gfx("CONT"));
	box = box.flags(alpha_to_flag(alpha));

	Vec2<float> dpad = local ?
		Vec2<float> {
			(G_PlayerInputAnalog(pid, gc_right, guessinput) - G_PlayerInputAnalog(pid, gc_left, guessinput)) / (float)JOYAXISRANGE,
			(G_PlayerInputAnalog(pid, gc_up, guessinput) - G_PlayerInputAnalog(pid, gc_down, guessinput)) / (float)JOYAXISRANGE,
		} :
		Vec2<float> {
			-cmd.turning * (encoremode ? -1 : 1) / (float)KART_FULLTURN,
			cmd.throwdir / (float)KART_FULLTURN,
		};

	box.patch(gfx("PAD{}", analog ? "N" : dpad_suffix(dpad)));
	box.patch(but('A', gc_a, BT_ACCELERATE));
	box.patch(but('B', gc_b, BT_LOOKBACK));
	box.patch(but('C', gc_c, BT_SPINDASHMASK));
	box.patch(but('X', gc_x, BT_BRAKE));
	box.patch(but('Y', gc_y, BT_RESPAWN));
	box.patch(but('Z', gc_z, BT_VOTE));
	box.patch(but('L', gc_l, BT_ATTACK));
	box.patch(but('R', gc_r, BT_DRIFT));
	box.patch(but('S', gc_start, 0xFFFFFFFF));

	if (mode == '4' || mode == '5') // Saturn 3D
	{
		float dist = (mode == '4') ? 3.f : 2.f;

		if (!analog)
			dist = 0;

		box.patch(gfx("JOY1"));
		box.xy(dpad.x * dist, -dpad.y * dist).patch(gfx("JOY2"));
	}
}
