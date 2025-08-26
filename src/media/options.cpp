// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <fmt/format.h>

#include "../cxxutil.hpp"
#include "../m_fixed.h"
#include "options.hpp"

using namespace srb2::media;

Options::Options(const char* prefix, map_t map) : prefix_(prefix), map_(map)
{
	for (auto& [suffix, cvar] : map_)
	{
		cvar.name = strdup(fmt::format("{}_{}", prefix_, suffix).c_str());
		cvars_.emplace_back(&cvar);
	}
}

const consvar_t& Options::cvar(const char* option) const
{
	const consvar_t& cvar = map_.at(option);

	SRB2_ASSERT(cvar.string != nullptr);

	return cvar;
}

template <>
int Options::get<int>(const char* option) const
{
	return cvar(option).value;
}

template <>
float Options::get<float>(const char* option) const
{
	return FixedToFloat(cvar(option).value);
}

template <typename T>
consvar_t Options::values(const char* default_value, const Range<T> range, std::map<std::string_view, T> list)
{
	const std::size_t min_max_size = (range.min || range.max) ? 2 : 0;
	auto* arr = new CV_PossibleValue_t[list.size() + min_max_size + 1];

	auto cast = [](T n)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			return FloatToFixed(n);
		}
		else
		{
			return n;
		}
	};

	if (min_max_size)
	{
		// Order is very important, MIN then MAX.
		arr[0] = {range.min ? cast(*range.min) : INT32_MIN, "MIN"};
		arr[1] = {range.max ? cast(*range.max) : INT32_MAX, "MAX"};
	}

	{
		std::size_t i = min_max_size;

		for (const auto& [k, v] : list)
		{
			arr[i].value = cast(v);
			arr[i].strvalue = k.data();

			i++;
		}

		arr[i].value = 0;
		arr[i].strvalue = nullptr;
	}

	int32_t flags = CV_SAVE;

	if constexpr (std::is_floating_point_v<T>)
	{
		flags |= CV_FLOAT;
	}

	return CVAR_INIT(nullptr, default_value, flags, arr, nullptr);
}

void Options::register_all()
{
	for (auto cvar : cvars_)
	{
		CV_RegisterVar(cvar);
	}

	cvars_ = {};
}

// clang-format off
template consvar_t Options::values(const char* default_value, const Range<int> range, std::map<std::string_view, int> list);
template consvar_t Options::values(const char* default_value, const Range<float> range, std::map<std::string_view, float> list);
// clang-format on
