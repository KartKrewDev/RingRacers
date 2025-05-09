// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef sanitize_h
#define sanitize_h

#include "doomtype.h"

#ifdef __cplusplus
#include <string_view>

#include "core/string.h"

namespace srb2::sanitize
{

enum class SanitizeMode
{
	kPrintable,
	kKeepColors,
};

enum class ParseMode
{
	kConsume,
	kPreserve,
};

// sanitizes string of all 0x80 codes
srb2::String sanitize(std::string_view in, SanitizeMode mode);

// sanitizes string of all 0x80 codes then parses caret codes
srb2::String parse_carets(std::string_view in, ParseMode mode);

}; // namespace srb2

extern "C" {
#endif

void D_SanitizeKeepColors(char *out, const char *in, size_t out_size); // SanitizeMode::kKeepColors
void D_ParseCarets(char *out, const char *in, size_t out_size); // ParseMode::kConsume

// returns string width in pixels
INT32 M_DrawCaretString(INT32 x, INT32 y, const char *string, boolean preserve);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // sanitize_h
