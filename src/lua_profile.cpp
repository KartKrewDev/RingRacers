// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

#include <fmt/format.h>

extern "C" {
#include "blua/lua.h"
};

#include "core/hash_map.hpp"
#include "core/string.h"
#include "core/vector.hpp"
#include "v_draw.hpp"

#include "command.h"
#include "doomtype.h"
#include "i_system.h"
#include "lua_profile.h"
#include "m_perfstats.h"

extern "C" consvar_t cv_lua_profile;

namespace
{

precise_t g_time_reference;
int g_tics_counted;

double g_running_tic_time;
double g_avg_tic_time;

bool g_invalid;

}; // namespace

struct lua_timer_t
{
	struct Stat
	{
		double calls = 0;
		double time = 0.0;
	};

	Stat running, avg;
};

namespace
{

srb2::HashMap<srb2::String, lua_timer_t> g_tic_timers;

}; // namespace

lua_timer_t* LUA_BeginFunctionTimer(lua_State* L, int fn_idx, const char* name)
{
	lua_Debug ar;

	lua_pushvalue(L, fn_idx);
	lua_getinfo(L, ">S", &ar);

	auto label = [&]
	{
		using sv = std::string_view;
		sv view{ar.source};

		switch (view.front())
		{
		case '@': // file
			if (std::size_t p = view.rfind('/'); p != sv::npos) // lump or base
			{
				return view.substr(p + 1);
			}
			break;

		case '=': // ??
			view.remove_prefix(1);
			break;
		}

		return view;
	};

	auto [it, ins] = g_tic_timers.try_emplace(fmt::format("{}:{} ({})", label(), ar.linedefined, name));
	auto& [key, timer] = *it;

	g_time_reference = I_GetPreciseTime();

	return &timer;
}

void LUA_EndFunctionTimer(lua_timer_t* timer)
{
	precise_t t = I_GetPreciseTime() - g_time_reference;
	double precision = I_GetPrecisePrecision();

	timer->running.time += t / precision;
	timer->running.calls += 1.0;
}

void LUA_ResetTicTimers(void)
{
	if (cv_lua_profile.value <= 0)
	{
		return;
	}

	if (g_tics_counted < cv_lua_profile.value)
	{
		g_tics_counted++;

		double precision = I_GetPrecisePrecision();
		g_running_tic_time += ps_prevtictime / precision;
	}
	else
	{
		double counted = g_tics_counted;

		for (auto& [key, timer] : g_tic_timers)
		{
			timer.avg = {timer.running.calls / counted, timer.running.time / counted};
			timer.running = {};
		}

		g_avg_tic_time = g_running_tic_time / counted;
		g_running_tic_time = 0.0;

		g_tics_counted = 1;

		g_invalid = false;
	}
}

void LUA_RenderTimers(void)
{
	using srb2::Draw;

	constexpr int kRowHeight = 4;

	Draw row = Draw(0, kRowHeight).font(Draw::Font::kConsole).align(Draw::Align::kLeft).scale(0.5).flags(V_MONOSPACE);
	row.y(-kRowHeight).text("-- AVERAGES PER TIC (over {} tics) --", cv_lua_profile.value);

	if (g_invalid)
	{
		row.flags(V_GRAYMAP).text("  <Data pending>");
		return;
	}

	srb2::Vector<decltype(g_tic_timers)::Iter> view;
	view.reserve(g_tic_timers.size());

	auto color_flag = [](double t)
	{
		if (t < 10.0)
		{
			return V_GRAYMAP;
		}
		else if (t >= 100.0)
		{
			return V_YELLOWMAP;
		}
		else
		{
			return 0;
		}
	};

	{
		double cum = 0.0;

		for (auto it = g_tic_timers.begin(); it != g_tic_timers.end(); ++it)
		{
			auto& [key, timer] = *it;

			cum += timer.avg.time;

			if (timer.avg.time > 0.0)
			{
				view.push_back(it);
			}
		}

		Draw tally = row.flags(color_flag(cum * 1'000'000.0));

		tally.text("{:8.2f} us - TOTAL", cum * 1'000'000.0);
		tally.y(kRowHeight).text("{:8.2f} ms", cum * 1000.0);
		tally.y(kRowHeight * 2).text(
			"{:8.2f}% overhead ({:.2f} / {:.2f}) <-- not counting rendering time",
			(cum / g_avg_tic_time) * 100.0,
			cum * TICRATE,
			g_avg_tic_time * TICRATE
		);

		row = row.y(kRowHeight * 4);
	}

	std::sort(
		view.rbegin(),
		view.rend(),
		[](auto a, auto b)
		{
			auto& [k1, t1] = *a;
			auto& [k2, t2] = *b;
			return t1.avg.time < t2.avg.time;
		}
	);

	for (auto it : view)
	{
		auto& [key, timer] = *it;

		double t = timer.avg.time * 1'000'000.0;

		row.flags(color_flag(t)).text(
			"{:>8.2f} us {:>8.2f} calls - {}",
			t,
			timer.avg.calls,
			key
		);

		row = row.y(kRowHeight);
	}
}

extern "C" void lua_profile_OnChange(void)
{
	g_invalid = true;

	if (cv_lua_profile.value == 0)
	{
		g_tic_timers = {};
	}
}
