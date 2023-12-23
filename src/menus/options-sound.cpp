/// \file  menus/options-sound.c
/// \brief Sound Options

#include <array>
#include <cstdlib>

#include "../v_draw.hpp"

#include "../console.h"
#include "../k_menu.h"
#include "../s_sound.h"	// sounds consvars
#include "../g_game.h" // cv_chatnotifications

using srb2::Draw;

namespace
{

int flip_delay = 0;

struct Slider
{
	enum Id
	{
		kMusicVolume,
		kSfxVolume,
		kNumSliders
	};

	Slider(bool(*toggle)(bool), consvar_t& volume) : toggle_(toggle), volume_(volume) {}

	bool(*toggle_)(bool);
	consvar_t& volume_;

	int shake_ = 0;

	void draw(int x, int y, bool selected)
	{
		constexpr int kWidth = 111;

		Draw h(320 - x - kWidth, y);

		if (selected)
		{
			int ofs = skullAnimCounter / 5;
			Draw arrows = h.font(Draw::Font::kConsole).align(Draw::Align::kLeft).flags(highlightflags);

			arrows.x(-10 - ofs).text("\x1C");
			arrows.x(kWidth + 2 + ofs).text("\x1D");

			h.xy(kWidth + 9, -3).small_button(Draw::Button::z, false);
		}

		h = h.y(1);
		h.size(kWidth, 7).fill(31);

		Draw s = h.xy(1, 2).size(10, 4);
		int color = toggle_(false) ? aquamap[0] : 15;

		int n = volume_.value / 10;
		for (int i = 0; i < n; ++i)
		{
			s.fill(color);
			s = s.x(11);
		}

		s.width(volume_.value % 10).fill(color);

		n = std::atoi(volume_.defaultvalue);
		h.x(1 + shake_ + n + (n / 10)).size(1, 7).fill(35);

		if (!toggle_(false))
		{
			h
				.x(kWidth / 2)
				.font(Draw::Font::kConsole)
				.align(Draw::Align::kCenter)
				.flags(V_40TRANS)
				.text("S I L E N T");
		}
	}

	void input(INT32 c)
	{
		M_ChangeCvarDirect(c, &volume_);

		shake_ = !shake_;
		flip_delay = 2;
	}
};

std::array<Slider, Slider::kNumSliders> sliders{{
	{
		[](bool toggle) -> bool
		{
			if (toggle)
			{
				CV_AddValue(&cv_gamedigimusic, 1);
			}

			return !S_MusicDisabled();
		},
		cv_digmusicvolume,
	},
	{
		[](bool toggle) -> bool
		{
			if (toggle)
			{
				CV_AddValue(&cv_gamesounds, 1);
			}

			return !S_SoundDisabled();
		},
		cv_soundvolume,
	},
}};

void slider_routine(INT32 c)
{
	sliders.at(currentMenu->menuitems[itemOn].mvar2).input(c);
}

void draw_routine()
{
	int x = currentMenu->x - (menutransition.tics * 48);
	int y = currentMenu->y;

	M_DrawGenericOptions();

	for (int i = 0;  i < currentMenu->numitems; ++i)
	{
		const menuitem_t& it = currentMenu->menuitems[i];

		if ((it.status & IT_TYPE) == IT_ARROWS)
		{
			sliders.at(it.mvar2).draw(x, y, i == itemOn);
		}

		y += 8;
	}
}

void tick_routine(void)
{
	M_OptionsTick();

	if (flip_delay && !--flip_delay)
	{
		for (Slider& slider : sliders)
		{
			slider.shake_ = 0;
		}
	}
}

boolean input_routine(INT32)
{
	UINT8 pid = 0; // todo: Add ability for any splitscreen player to bring up the menu.

	const menuitem_t& it = currentMenu->menuitems[itemOn];

	if (M_MenuButtonPressed(pid, MBT_Z) && (it.status & IT_TYPE) == IT_ARROWS)
	{
		sliders.at(it.mvar2).toggle_(true);
		return true;
	}

	return false;
}

}; // namespace

menuitem_t OPTIONS_Sound[] =
{


	{IT_STRING | IT_ARROWS | IT_CV_SLIDER, "SFX Volume", "Adjust the volume of sound effects.",
		NULL, {.routine = slider_routine}, 0, Slider::kSfxVolume},

	{IT_STRING | IT_ARROWS | IT_CV_SLIDER, "Music Volume", "Adjust the volume of music playback.",
		NULL, {.routine = slider_routine}, 0, Slider::kMusicVolume},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Reverse L/R Channels", "Reverse left & right channels for Stereo playback.",
		NULL, {.cvar = &stereoreverse}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Notifications", "Set when to play notification sounds when chat messages are received.",
		NULL, {.cvar = &cv_chatnotifications}, 0, 0},

	{IT_STRING | IT_CVAR, "Character Voices", "Set how often to play character voices in game.",
		NULL, {.cvar = &cv_kartvoices}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Play Music While Unfocused", "Keeps playing music even if the game is not the active window.",
		NULL, {.cvar = &cv_playmusicifunfocused}, 0, 0},

	{IT_STRING | IT_CVAR, "Play SFX While Unfocused", "Keeps playing sound effects even if the game is not the active window.",
		NULL, {.cvar = &cv_playsoundifunfocused}, 0, 0},

	// @TODO: Sound test (there's currently no space on this menu, might be better to throw it in extras?)
};

menu_t OPTIONS_SoundDef = {
	sizeof (OPTIONS_Sound) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Sound,
	48, 80,
	SKINCOLOR_THUNDER, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	draw_routine,
	M_DrawOptionsCogs,
	tick_routine,
	NULL,
	NULL,
	input_routine,
};
