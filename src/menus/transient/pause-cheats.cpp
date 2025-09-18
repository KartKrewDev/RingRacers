// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
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
#include "../../k_hud.h"
#include "../../k_menu.h"
#include "../../screen.h"

extern "C" void COM_Lua_f(void);

using srb2::Draw;

namespace
{

enum Mode
{
	kCvars,
	kCommands,
	kNumModes
};

const char* mode_strings[kNumModes] = {
	"Options",
	"Commands",
};

std::vector<menuitem_t> g_menu;
std::vector<INT32> g_menu_offsets;
int g_menu_cursors[kNumModes];

int menu_mode()
{
	return PAUSE_CheatsDef.extra1;
}

void menu_mode(int mode)
{
	g_menu_cursors[menu_mode()] = itemOn;

	PAUSE_CheatsDef.extra1 = mode;

	itemOn = g_menu_cursors[menu_mode()];
}

void list_cvars()
{
	for (consvar_t* var = consvar_vars; var; var = var->next)
	{
		if (!(var->flags & (CV_CHEAT | CV_ADDEDBYLUA)))
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

		g_menu.push_back(menuitem_t {status, var->name, var->description, nullptr, srb2::itemaction(var), 0, height});
	}
}

void list_commands()
{
	static const auto call = [](INT32 idx) { COM_ImmedExecute(currentMenu->menuitems[idx].text); };

	for (xcommand_t* cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!(cmd->debug || cmd->function == COM_Lua_f))
		{
			continue;
		}

		g_menu.push_back(menuitem_t {IT_STRING | IT_CALL, cmd->name, nullptr, nullptr, srb2::itemaction(call), 0, 8});
	}
}

void group_menu()
{
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

	switch (menu_mode())
	{
	case kCvars:
		list_cvars();
		break;

	case kCommands:
		list_commands();
		break;
	}

	std::sort(g_menu.begin(), g_menu.end(),
		[](menuitem_t& a, menuitem_t& b) { return std::strcmp(a.text, b.text) < 0; });

	group_menu();

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

boolean menu_input(INT32)
{
	// C button: cycle through modes
	if (M_MenuButtonPressed(0, MBT_Y))
	{
		menu_mode((menu_mode() + 1) % kNumModes);

		// reload menu
		menu_open();
	}

	return false;
}

void draw_menu()
{
	constexpr int kMargin = 4;

	Draw draw = Draw(0, 0).align(Draw::Align::kCenter);

	draw.patch("MENUHINT");

	const menuitem_t& item = currentMenu->menuitems[itemOn];

	if (item.tooltip)
	{
		draw.xy(BASEVIDWIDTH/2, 12).font(Draw::Font::kThin).text(item.tooltip);
	}
	draw = draw.y(27 + kMargin);

	draw.x(BASEVIDWIDTH/2).font(Draw::Font::kGamemode).text(mode_strings[menu_mode()]);
	K_DrawGameControl(draw.x() + 8, draw.y()-6, 0, M_MenuButtonHeld(0, MBT_Y) ? "<y_pressed> Switch Page" : "<y> Switch Page", 0, MENU_FONT, 0);
	// K_drawButton((draw.x() + 8) * FRACUNIT, (draw.y() + 8) * FRACUNIT, 0, kp_button_y[0], M_MenuButtonHeld(0, MBT_Y));
	draw = draw.y(32 + kMargin);

	currentMenu->y = std::min(static_cast<INT32>(draw.y()), (BASEVIDHEIGHT/2) - g_menu_offsets[itemOn]);

	V_SetClipRect(0, draw.y() * FRACUNIT, BASEVIDWIDTH * FRACUNIT, (BASEVIDHEIGHT - draw.y() - kMargin) * FRACUNIT, 0);
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
	kCvars, 0,
	MBF_SOUNDLESS,
	nullptr,
	0, 0,
	draw_menu,
	nullptr,
	nullptr,
	menu_open,
	menu_close,
	menu_input,
};
