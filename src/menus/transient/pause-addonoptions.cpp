// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by AJ "Tyron" Martinez
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/pause-addonoptions.c
/// \brief Addon options

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

#include "../../z_zone.h"

extern "C" {
#include "../../lua_script.h"
#include "../../lua_libs.h"
#include "../../lua_hook.h"
};

extern "C" void COM_Lua_f(void);

using srb2::Draw;

namespace
{

enum Mode
{
	kUser,
	kAdmin,
	kNumModes
};

const char* mode_strings[kNumModes] = {
	"User Settings",
	"Host Settings",
};

std::vector<menuitem_t> g_menu;
std::vector<INT32> g_menu_offsets;
int g_menu_cursors[kNumModes];

int menu_mode()
{
	return PAUSE_AddonOptionsDef.extra1;
}

boolean admin_mode()
{
	return !!(menu_mode() == kAdmin);
}

void menu_mode(int mode)
{
	g_menu_cursors[menu_mode()] = itemOn;

	PAUSE_AddonOptionsDef.extra1 = mode;

	itemOn = g_menu_cursors[menu_mode()];
}

void list_cvars()
{
	for (consvar_t* var = consvar_vars; var; var = var->next)
	{
		if (!(var->flags & CV_ADDEDBYLUA))
			continue;

		if (var->flags & CV_NOSHOWHELP)
			continue;

		if (!admin_mode() != !(var->flags & CV_NETVAR)) // LOL.
			continue;

		UINT16 status = IT_STRING | IT_CVAR;
		INT32 height = 8;

		if (!var->PossibleValue && !(var->flags & CV_FLOAT))
		{
			status |= IT_CV_STRING;
			height += 16;
		}

		g_menu.push_back(menuitem_t{ status, var->name, var->description, nullptr, srb2::itemaction(var), 0, height });
	}
}

void list_commands()
{
	static const auto call = [](INT32 idx) { COM_ImmedExecute(currentMenu->menuitems[idx].text); };
	UINT16 flags;

	for (xcommand_t* cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!(cmd->function == COM_Lua_f))
			continue;

		// Ha Ha What The Fuck
		// Taken from COM_Lua_f with only a vague idea of what I am doing. Sorry!
		char* buf;

		I_Assert(gL != NULL);

		lua_settop(gL, 0); // Just in case...
		lua_pushcfunction(gL, LUA_GetErrorMessage);

		lua_getfield(gL, LUA_REGISTRYINDEX, "COM_Command"); // push COM_Command
		I_Assert(lua_istable(gL, -1));

		// use buf temporarily -- must use lowercased string
		buf = Z_StrDup(cmd->name);
		strlwr(buf);
		lua_getfield(gL, -1, buf); // push command info table
		I_Assert(lua_istable(gL, -1));
		lua_remove(gL, -2); // pop COM_Command
		Z_Free(buf);

		lua_rawgeti(gL, -1, 2); // push flags from command info table
		if (lua_isboolean(gL, -1))
			flags = (lua_toboolean(gL, -1) ? COM_ADMIN : 0);
		else
			flags = (UINT16)lua_tointeger(gL, -1);
		lua_pop(gL, 1); // pop flags

		lua_pop(gL, 1); // pop command info table

		if (!admin_mode() != !(flags & COM_ADMIN))
			continue;

		if (flags & COM_NOSHOWHELP)
			continue;

		g_menu.push_back(menuitem_t{ IT_STRING | IT_CALL, cmd->name, "No information available for commands. Press to execute.", nullptr, srb2::itemaction(call), 0, 8 });
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

	list_cvars();
	list_commands();

	std::sort(g_menu.begin(), g_menu.end(),
		[](menuitem_t& a, menuitem_t& b) { return std::strcmp(a.text, b.text) < 0; });

	if (g_menu.size() == 0)
	{
		g_menu.insert(
			g_menu.begin(),
			menuitem_t {IT_DISABLED, "No addon options!", nullptr, nullptr, {}, 0, 0}
		);
	}

	group_menu();

	INT32 y = 0;

	for (menuitem_t& item : g_menu)
	{
		g_menu_offsets.push_back(y);
		y += item.mvar2;
	}

	PAUSE_AddonOptionsDef.menuitems = g_menu.data();
	PAUSE_AddonOptionsDef.numitems = g_menu.size();
}

boolean menu_close()
{
	PAUSE_AddonOptionsDef.menuitems = nullptr;
	PAUSE_AddonOptionsDef.numitems = 0;

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

		if (!server && !IsPlayerAdmin(consoleplayer))
			menu_mode(kUser);

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
	if (server || IsPlayerAdmin(consoleplayer))
		K_DrawGameControl(draw.x() + 8, draw.y()-6, 0, M_MenuButtonHeld(0, MBT_Y) ? "<y_pressed> Switch Page" : "<y> Switch Page", 0, MENU_FONT, 0);
	// K_drawButton((draw.x() + 8) * FRACUNIT, (draw.y() + 8) * FRACUNIT, 0, kp_button_y[0], M_MenuButtonHeld(0, MBT_Y));
	draw = draw.y(32 + kMargin);

	currentMenu->y = std::min(static_cast<INT32>(draw.y()), (BASEVIDHEIGHT/2) - g_menu_offsets[itemOn]);

	V_SetClipRect(0, draw.y() * FRACUNIT, BASEVIDWIDTH * FRACUNIT, (BASEVIDHEIGHT - draw.y() - kMargin) * FRACUNIT, 0);
	M_DrawGenericMenu();
	V_ClearClipRect();
}

}; // namespace

menu_t PAUSE_AddonOptionsDef = {
	0,
	&PAUSE_MainDef,
	0,
	nullptr,
	48, 0,
	kUser, 0,
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
