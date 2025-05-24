// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_debug_parser.cpp
/// \brief Helper functions for the debugrender_highlight command

#include "r_debug_detail.hpp"

#include "doomdef.h"
#include "r_main.h"

using namespace srb2::r_debug;
using namespace srb2::r_debug::detail;

char* detail::skip_alnum(char* p, int mode)
{
	while (*p != '\0' && !isalnum(*p) == !mode)
	{
		p++;
	}

	return p;
}

char* detail::parse_highlight_arg(char* p)
{
	INT32 k;
	const HighlightDesc* key;

	const auto old = static_cast<debugrender_highlight_t>(debugrender_highlight);

	char* t;
	int c;

	p = skip_alnum(p, 0); // skip "whitespace"

	if (*p == '\0')
	{
		return NULL;
	}

	t = skip_alnum(p, 1); // find end of word

	c = *t;	   // save to restore afterward
	*t = '\0'; // isolate word string

	for (k = 0; (key = &kHighlightOptions[k]), k < NUM_SW_HI; ++k)
	{
		// allow an approximate match
		if (!strncasecmp(p, key->label, (t - p)))
		{
			debugrender_highlight |= (1 << k);
			// keep going to match multiple with same
			// prefix
		}
	}

	if (debugrender_highlight == old)
	{
		// no change? Foolish user
		CONS_Alert(CONS_WARNING, "\"%s\" makes no sense\n", p);
	}

	*t = c; // restore

	return t; // skip to end of word
}

void detail::highlight_help(bool only_on)
{
	int32_t k;
	const HighlightDesc* key;

	for (k = 0; (key = &kHighlightOptions[k]), k < NUM_SW_HI; ++k)
	{
		const bool on = (debugrender_highlight & (1 << k)) != 0;

		if (!only_on || on)
		{
			CONS_Printf("%s\x80 \x87%s\x80 - %s\n", on ? "\x83 ON" : "\x85OFF", key->label, key->description);
		}
	}

	if (!only_on)
	{
		CONS_Printf("\nYou can change the highlights by using a command like:\n\n"
					"\x87    debugrender_highlight planes sprites\n"
					"\x87    debugrender_highlight none\n");
	}
}
