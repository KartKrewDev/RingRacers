// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <initializer_list>
#include <optional>
#include <utility>

#include <fmt/format.h>

#include "../v_draw.hpp"

#include "../command.h"
#include "../d_clisrv.h"
#include "../d_player.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../g_game.h"
#include "../g_party.h"
#include "../i_time.h"
#include "../k_director.h"
#include "../k_hud.h"
#include "../p_local.h"
#include "../r_fps.h"

extern "C" consvar_t cv_maxplayers;

using srb2::Draw;

namespace
{

struct List
{
	struct Field
	{
		Field(const char* label, Draw::Button button, std::optional<bool> pressed = {}) :
			label_(Draw::TextElement(label).font(Draw::Font::kThin)),
			button_(button),
			pressed_(pressed)
		{
		}

		int width() const { return label_.width() + kButtonWidth + kButtonMargin + kFieldSpacing; }

		void draw(const Draw& row, bool left) const
		{
			Draw col = row.x(left ? kButtonWidth + kButtonMargin : kFieldSpacing);

			col.text(label_);
			col = col.x(left ? -(kButtonWidth + kButtonMargin) : width() - (kButtonWidth + kFieldSpacing));

			//if (r_splitscreen)
			{
				auto small_button_offset = [&]
				{
					switch (button_)
					{
					case Draw::Button::l:
					case Draw::Button::r:
						return -4;

					default:
						return -2;
					}
				};

				col.y(small_button_offset()).small_button(button_, pressed_);
			}
#if 0
			else
			{
				col.y(-4).button(button_, pressed_);
			}
#endif
		}

	private:
		static constexpr int kButtonWidth = 14;
		static constexpr int kButtonMargin = 2;
		static constexpr int kFieldSpacing = 8;

		Draw::TextElement label_;
		Draw::Button button_;
		std::optional<bool> pressed_;
	};

	List(int x, int y) : row_(split_draw(x, y, left_)) {}

	void insert(std::initializer_list<Field> fields)
	{
		auto total_width = [&fields]
		{
			int width = 0;

			for (const Field& field : fields)
			{
				width += field.width();
			}

			return width;
		};

		Draw col = left_ ? row_ : row_.x(-total_width());

		for (const Field& field : fields)
		{
			field.draw(col, left_);
			col = col.x(field.width());
		}

		//row_ = row_.y(r_splitscreen ? -13 : -17);
		row_ = row_.y(-13);
	}

private:
	bool left_ = r_splitscreen > 1 && R_GetViewNumber() & 1;
	Draw row_;

	static Draw split_draw(int x, int y, bool left)
	{
		return Draw(
			left ? x : (BASEVIDWIDTH / (r_splitscreen > 1 ? 2 : 1)) - x,
			(BASEVIDHEIGHT / (r_splitscreen ? 2 : 1)) - y
		)
			.align(Draw::Align::kLeft)
			.flags(
				V_SNAPTOBOTTOM |
				(left ? V_SNAPTOLEFT : V_SNAPTORIGHT) |
				(r_splitscreen > 1 ? V_HUDTRANS : V_SLIDEIN) |
				V_SPLITSCREEN
			);
	}
};

}; // namespace

void K_drawSpectatorHUD(boolean director)
{
	const UINT8 viewnum = R_GetViewNumber();

	UINT8 numingame = 0;

	for (UINT8 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			numingame++;
		}
	}

	player_t* player = [viewnum]() -> player_t*
	{
		if (viewnum >= G_PartySize(consoleplayer))
		{
			return nullptr;
		}

		UINT8 p = G_PartyMember(consoleplayer, viewnum);

		if (!playeringame[p] || players[p].spectator == false)
		{
			return nullptr;
		}

		return &players[p];
	}();

	List list = [director]
	{
		switch (r_splitscreen)
		{
		case 0:
			return List(director ? 20 : 20, 34);

		case 1:
			return List(40, 30);

		default:
			return List(10, 24);
		}
	}();

	if (player)
	{
		std::string label = [player]
		{
			if (player->flashing)
			{
				return ". . .";
			}
			else if (player->pflags & PF_WANTSTOJOIN)
			{
				return "Cancel Join";
			}
			else
			{
				return "Join";
			}
		}();

		if (cv_maxplayers.value)
		{
			label += fmt::format(" [{}/{}]", numingame, cv_maxplayers.value);
		}

		list.insert({{label.c_str(), Draw::Button::l}});
	}

	if (director || camera[viewnum].freecam)
	{
		// Not locked into freecam -- can toggle it.
		if (director)
		{
			list.insert({{"Freecam", Draw::Button::c}});
		}
		else
		{
			bool press = D_LocalTiccmd(viewnum)->buttons & BT_RESPAWN;
			const char* label = (press && I_GetTime() % 16 < 8) ? "> <" : ">< ";

			list.insert({{label, Draw::Button::y, press}, {"Exit", Draw::Button::c}});
		}
	}

	if (director)
	{
		if (numingame > 1)
		{
			list.insert({{"+", Draw::Button::a}, {"-", Draw::Button::x}});
		}

		if (player)
		{
			list.insert({{K_DirectorIsEnabled(viewnum) ? "\x82" "Director" : "Director", Draw::Button::r}});
		}
	}
	else
	{
		auto bt = D_LocalTiccmd(viewnum)->buttons;

		list.insert({{"", Draw::Button::r, bt & BT_DRIFT}, {"Pivot", Draw::Button::b, bt & BT_LOOKBACK}});
		list.insert({{"+", Draw::Button::a, bt & BT_ACCELERATE}, {"-", Draw::Button::x, bt & BT_BRAKE}});
	}
}
