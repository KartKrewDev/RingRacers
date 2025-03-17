// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <string_view>

#include "r_debug.hpp"
#include "v_draw.hpp"

#include "core/hash_map.hpp"
#include "core/hash_set.hpp"
#include "doomdef.h"
#include "doomtype.h"
#include "r_textures.h"
#include "screen.h"

using srb2::Draw;

namespace
{

srb2::HashSet<INT32> frame_list;

}; // namespace

namespace srb2::r_debug
{

void add_texture_to_frame_list(INT32 texnum)
{
	if (cht_debug & DBG_RENDER)
	{
		frame_list.insert(texnum);
	}
}

void clear_frame_list()
{
	frame_list.clear();
}

void draw_frame_list()
{
	if (!(cht_debug & DBG_RENDER))
	{
		return;
	}

	Draw line = Draw(4, BASEVIDHEIGHT - 20)
		.font(Draw::Font::kConsole)
		.scale(0.5)
		.flags(V_ORANGEMAP | V_SNAPTOLEFT | V_SNAPTOBOTTOM);

	line.y(-4 * static_cast<int>(frame_list.size() - 1)).size(32, 4 * frame_list.size()).fill(31);

	for (INT32 texnum : frame_list)
	{
		line.text("{}", std::string_view {textures[texnum]->name, 8});
		line = line.y(-4);
	}
}

}; // namespace srb2::r_debug
