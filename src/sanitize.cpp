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
#include <cctype>
#include <iterator>
#include <string_view>

#include "core/string.h"
#include "doomtype.h"
#include "sanitize.h"
#include "v_draw.hpp"

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
srb2::String& filter_out(srb2::String& out, const std::string_view& range, F filter)
{
	std::remove_copy_if(
		range.begin(),
		range.end(),
		std::back_inserter(out),
		filter
	);
	return out;
};

int hexconv(int c)
{
	if (std::isdigit(c))
		return c - '0';

	c = std::toupper(c);
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');

	return -1;
}

}; // namespace

namespace srb2::sanitize
{

srb2::String sanitize(std::string_view in, SanitizeMode mode)
{
	srb2::String out;
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

srb2::String parse_carets(std::string_view in, ParseMode mode)
{
	srb2::String out;

	using std::size_t;
	for (;;)
	{
		size_t p = in.find('^');

		// copy chars up until the caret
		// but filter out codes outside of the ASCII range
		filter_out(out, in.substr(0, p), print_filter);

		if (p == in.npos)
		{
			break; // end of input
		}

		in.remove_prefix(p);

		// need two characters for caret code
		// convert to color byte
		if (int c; in.length() > 1 && (c = hexconv(in[1])) != -1)
		{
			out.push_back(0x80 | c);
		}

		if (mode != ParseMode::kConsume)
		{
			// preserve caret code in output
			filter_out(out, in.substr(0, 2), print_filter);
		}

		if (in.length() < 2)
		{
			break;
		}

		in.remove_prefix(2);
	}

	return out;
}

}; // namespace srb2

void D_SanitizeKeepColors(char *out, const char *in, size_t out_size)
{
	strlcpy(out, sanitize(in, SanitizeMode::kKeepColors).c_str(), out_size);
}

void D_ParseCarets(char *out, const char *in, size_t out_size)
{
	strlcpy(out, parse_carets(in, ParseMode::kConsume).c_str(), out_size);
}

INT32 M_DrawCaretString(INT32 x, INT32 y, const char *string, boolean preserve)
{
	using srb2::Draw;
	Draw::TextElement text(parse_carets(string, preserve ? ParseMode::kPreserve : ParseMode::kConsume));
	text.font(Draw::Font::kConsole);
	Draw(x, y).text(text);
	return text.width();
}
