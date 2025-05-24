// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_debug_detail.cpp
/// \brief Software renderer debugging, internal header

#ifndef __R_DEBUG_DETAIL__
#define __R_DEBUG_DETAIL__

#include "r_main.h"

namespace srb2::r_debug::detail
{

struct HighlightDesc
{
	uint8_t color;
	const char* label;
	const char* description;
};

constexpr HighlightDesc kHighlightOptions[NUM_SW_HI] = {
	{0x96, "planes", "Sector floor/ceiling"},
	{0x49, "fofplanes", "FOF top/bottom"},
	{0xB6, "fofsides", "FOF sides"},
	{0x7A, "midtextures", "Two-sided midtexture"},
	{0xC9, "walls", "Sector upper/lower texture, one-sided midtexture"},
	{0x23, "sprites", "Sprites"},
	{0x0F, "sky", "Sky texture"}};

char* skip_alnum(char* p, int mode);
char* parse_highlight_arg(char* p);
void highlight_help(bool only_on);

}; // srb2::r_debug::detail

#endif // __R_DEBUG_DETAIL__
