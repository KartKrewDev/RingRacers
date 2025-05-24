// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_OPTIONS_HPP__
#define __SRB2_MEDIA_OPTIONS_HPP__

#include <map>
#include <optional>
#include <string_view>

#include "../core/hash_map.hpp"
#include "../core/string.h"
#include "../core/vector.hpp"
#include "../command.h"

namespace srb2::media
{

class Options
{
public:
	using map_t = srb2::HashMap<srb2::String, consvar_t>;

	template <typename T>
	struct Range
	{
		std::optional<T> min, max;
	};

	// Registers all options as cvars.
	static void register_all();

	Options(const char* prefix, map_t map);

	template <typename T>
	T get(const char* option) const;

	template <typename T>
	static consvar_t values(const char* default_value, const Range<T> range, std::map<std::string_view, T> list = {});

private:
	static srb2::Vector<consvar_t*> cvars_;

	const char* prefix_;
	map_t map_;

	const consvar_t& cvar(const char* option) const;
};

// clang-format off
extern template consvar_t Options::values(const char* default_value, const Range<int> range, std::map<std::string_view, int> list);
extern template consvar_t Options::values(const char* default_value, const Range<float> range, std::map<std::string_view, float> list);
// clang-format on

}; // namespace srb2::media

#endif // __SRB2_MEDIA_OPTIONS_HPP__
