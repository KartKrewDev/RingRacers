// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <vector>

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

static consvar_t range_cvar(const char* default_value, int32_t min, int32_t max, int32_t flags = 0)
{
	return CVAR_INIT(
		nullptr,
		default_value,
		CV_SAVE | flags,
		new CV_PossibleValue_t[] {{min, "MIN"}, {max, "MAX"}, {}},
		nullptr
	);
}

template <>
consvar_t Options::range<float>(const char* default_value, float min, float max)
{
	return range_cvar(default_value, FloatToFixed(min), FloatToFixed(max), CV_FLOAT);
}

template <>
consvar_t Options::range_min<float>(const char* default_value, float min)
{
	return range_cvar(default_value, FloatToFixed(min), INT32_MAX);
}

template <>
consvar_t Options::range<int>(const char* default_value, int min, int max)
{
	return range_cvar(default_value, min, max);
}

template <>
consvar_t Options::range_min<int>(const char* default_value, int min)
{
	return range_cvar(default_value, min, INT32_MAX);
}

template <>
consvar_t Options::value_map<int>(const char* default_value, std::map<const char*, int> values)
{
	auto* arr = new CV_PossibleValue_t[values.size() + 1];

	std::size_t i = 0;

	for (const auto& [k, v] : values)
	{
		arr[i].value = v;
		arr[i].strvalue = k;

		i++;
	}

	arr[i].value = 0;
	arr[i].strvalue = nullptr;

	return CVAR_INIT(nullptr, default_value, CV_SAVE, arr, nullptr);
}

void Options::register_all()
{
	for (auto cvar : cvars_)
	{
		CV_RegisterVar(cvar);
	}

	cvars_ = {};
}
