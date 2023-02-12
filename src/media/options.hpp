// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_OPTIONS_HPP__
#define __SRB2_MEDIA_OPTIONS_HPP__

#include <map>
#include <unordered_map>

#include "../command.h"

namespace srb2::media
{

class Options
{
public:
	using map_t = std::unordered_map<const char*, consvar_t>;

	Options(const char* prefix, map_t map);

	template <typename T>
	T get(const char* option) const;

	template <typename T>
	static consvar_t range(const char* default_value, T min, T max);

	template <typename T>
	static consvar_t range_min(const char* default_value, T min);

	template <typename T>
	static consvar_t value_map(const char* default_value, std::map<const char*, T> values);

private:
	const char* prefix_;
	map_t map_;

	const consvar_t& cvar(const char* option) const;
};

void register_options();

}; // namespace srb2::media

#endif // __SRB2_MEDIA_OPTIONS_HPP__
