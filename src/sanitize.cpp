// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <string_view>

#include "doomtype.h"
#include "sanitize.h"

using namespace srb2::sanitize;

namespace
{

bool print_filter(char c)
{
	return !std::isprint(c);
}

bool color_filter(char c)
{
	return print_filter(c) && (c & 0xF0) != 0x80; // color codes
}

template <typename F>
std::string& filter_out(std::string& out, const std::string_view& range, F filter)
{
	std::remove_copy_if(
		range.begin(),
		range.end(),
		std::back_inserter(out),
		filter
	);
	return out;
};

}; // namespace

namespace srb2::sanitize
{

std::string sanitize(std::string_view in, SanitizeMode mode)
{
	std::string out;
	return filter_out(out, in, [mode]
		{
			switch (mode)
			{
			default:
			case SanitizeMode::kPrintable:
				return print_filter;
			case SanitizeMode::kKeepColors:
				return color_filter;
			}
		}());
}

}; // namespace srb2

void D_SanitizeKeepColors(char *out, const char *in, size_t out_size)
{
	strlcpy(out, sanitize(in, SanitizeMode::kKeepColors).c_str(), out_size);
}
