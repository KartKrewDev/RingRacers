// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/pause-cheats.c
/// \brief Cheats directory, for developers

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

#include "../../v_draw.hpp"
#include "../../v_video.h"

#include "../../command.h"
#include "../../doomtype.h"
#include "../../k_menu.h"
#include "../../screen.h"

using srb2::Draw;

namespace
{

std::vector<menuitem_t> g_menu;
std::vector<INT32> g_menu_offsets;

void sort_menu()
{
	std::sort(g_menu.begin(), g_menu.end(),
		[](menuitem_t& a, menuitem_t& b) { return std::strcmp(a.text, b.text) < 0; });

	int old_key = '\0';

	// Can't use range-for because iterators are invalidated
	// by std::vector::insert.
	for (std::size_t i = 0; i < g_menu.size(); ++i)
	{
		int new_key = g_menu[i].text[0];

		if (new_key == old_key)
		{
			// Group cvars starting with the same letter
			// together.
			continue;
		}

		old_key = new_key;

		if (i == 0)
		{
			continue;
		}

		constexpr int spacer = 8;

		g_menu.insert(
			g_menu.begin() + i,
			menuitem_t {IT_SPACE | IT_DYLITLSPACE, nullptr, nullptr, nullptr, {}, spacer, spacer}
		);

		i++; // skip the inserted element
	}
}

void menu_open()
{
	g_menu = {};
	g_menu_offsets = {};

	for (consvar_t* var = consvar_vars; var; var = var->next)
	{
		if (!(var->flags & CV_CHEAT))
		{
			continue;
		}

		UINT16 status = IT_STRING | IT_CVAR;
		INT32 height = 8;

		if (!var->PossibleValue && !(var->flags & CV_FLOAT))
		{
			status |= IT_CV_STRING;
			height += 16;
		}

		g_menu.push_back(menuitem_t {status, var->name, nullptr, nullptr, {.cvar = var}, 0, height});
	}

	sort_menu();

	INT32 y = 0;

	for (menuitem_t& item : g_menu)
	{
		g_menu_offsets.push_back(y);
		y += item.mvar2;
	}

	PAUSE_CheatsDef.menuitems = g_menu.data();
	PAUSE_CheatsDef.numitems = g_menu.size();
}

boolean menu_close()
{
	PAUSE_CheatsDef.menuitems = nullptr;
	PAUSE_CheatsDef.numitems = 0;

	g_menu = {};
	g_menu_offsets = {};

	return true;
}

void draw_menu()
{
	auto tooltip = Draw(0, 0);

	tooltip.patch("MENUHINT");

	const menuitem_t& item = currentMenu->menuitems[itemOn];

	if (const consvar_t* cvar = item.itemaction.cvar; cvar && cvar->description)
	{
		tooltip.xy(BASEVIDWIDTH/2, 12).font(Draw::Font::kThin).align(Draw::Align::kCenter).text(cvar->description);
	}

	constexpr int kTooltipHeight = 27;
	constexpr int kPad = 4;
	int y = tooltip.y() + kTooltipHeight + kPad;

	currentMenu->y = std::min(y, (BASEVIDHEIGHT/2) - g_menu_offsets[itemOn]);

	V_SetClipRect(0, y * FRACUNIT, BASEVIDWIDTH * FRACUNIT, (BASEVIDHEIGHT - y - kPad) * FRACUNIT, 0);
	M_DrawGenericMenu();
	V_ClearClipRect();
}

}; // namespace

menu_t PAUSE_CheatsDef = {
	0,
	&PAUSE_MainDef,
	0,
	nullptr,
	48, 0,
	0, 0,
	MBF_SOUNDLESS,
	nullptr,
	0, 0,
	draw_menu,
	nullptr,
	menu_open,
	menu_close,
	nullptr,
};
