// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-sound.c
/// \brief Sound Options

#include <array>
#include <cstdlib>

#include "../v_draw.hpp"

#include "../doomstat.h"
#include "../console.h"
#include "../k_menu.h"
#include "../m_cond.h"
#include "../m_easing.h"
#include "../s_sound.h"	// sounds consvars
#include "../g_game.h" // cv_chatnotifications

extern "C" consvar_t cv_mastervolume, cv_continuousmusic;

using srb2::Draw;

namespace
{

int flip_delay = 0;

struct Slider
{
	enum Id
	{
		kMasterVolume,
		kMusicVolume,
		kSfxVolume,
		kVoiceVolume,
		kNumSliders
	};

	Slider(bool(*toggle)(bool), consvar_t& volume) : toggle_(toggle), volume_(volume) {}

	bool(*toggle_)(bool);
	consvar_t& volume_;

	int shake_ = 0;

	void draw(int x, int y, bool shaded, bool selected)
	{
		constexpr int kWidth = 111;

		Draw h(320 - x - kWidth, y);

		if (selected)
		{
			int ofs = skullAnimCounter / 5;
			Draw arrows = h.font(Draw::Font::kMenu).align(Draw::Align::kLeft).flags(highlightflags);

			arrows.x(-10 - ofs).text("\x1C");
			arrows.x(kWidth + 2 + ofs).text("\x1D");

			if (&volume_ != &cv_voicevolume)
			{
				Draw::TextElement tx = Draw::TextElement().parse("<z_animated>");
				h.xy(kWidth + 9, -2).text(tx.string());
			}
		}

		h = h.y(1);
		h.size(kWidth, 7).fill(31);

		Draw s = shaded ? h.xy(1, 5).size(10, 1) : h.xy(1, 2).size(10, 4);
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

		if (!toggle_(false) && &volume_ != &cv_voicevolume)
		{
			h
				.x(kWidth / 2)
				.font(Draw::Font::kMenu)
				.align(Draw::Align::kCenter)
				.flags(V_40TRANS)
				.text("S I L E N T");
		}
		else if (!toggle_(false))
		{
			h
				.x(kWidth / 2)
				.y(-1)
				.font(Draw::Font::kThin)
				.align(Draw::Align::kCenter)
				.flags(V_20TRANS)
				.text("DEAFENED (Voice Options)");
		}
	}

	void input(INT32 c)
	{
		M_ChangeCvarDirect(c, &volume_);

		if (!toggle_(false))
		{
			toggle_(true);
		}

		shake_ = !shake_;
		flip_delay = 2;
	}
};

std::array<Slider, Slider::kNumSliders> sliders{{
	{
		[](bool toggle) -> bool
		{
			bool n = !S_MusicDisabled() || !S_SoundDisabled();

			if (toggle)
			{
				n = !n;
				CV_SetValue(&cv_gamedigimusic, n);
				CV_SetValue(&cv_gamesounds, n);
				CV_SetValue(&cv_voice_selfdeafen, n);
			}

			return n;
		},
		cv_mastervolume,
	},
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
	{
		[](bool toggle) -> bool
		{
			if (toggle)
			{
				CV_AddValue(&cv_voice_selfdeafen, 0);
			}

			return !S_VoiceDisabled();
		},
		cv_voicevolume,
	},
}};

void slider_routine(INT32 c)
{
	sliders.at(currentMenu->menuitems[itemOn].mvar2).input(c);
}

void restartaudio_routine(INT32)
{
	COM_ImmedExecute("restartaudio");
}

void draw_routine()
{
	int x = currentMenu->x - M_EaseWithTransition(Easing_Linear, 5 * 48);
	int y = currentMenu->y;

	M_DrawGenericOptions();

	for (int i = 0;  i < currentMenu->numitems; ++i)
	{
		const menuitem_t& it = currentMenu->menuitems[i];

		if ((it.status & IT_TYPE) == IT_ARROWS)
		{
			sliders.at(it.mvar2).draw(
				x,
				y,
				false,
				i == itemOn
			);
		}

		y += 9;
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

void init_routine(void)
{
	OPTIONS_Sound[sopt_followhorns].status = IT_SECRET;
	OPTIONS_Sound[sopt_attackmusic].status = IT_SECRET;

	// We show a kindness. If you have online available,
	// even if you haven't found a single Follower, you
	// should be able to turn off other people's horns.
	bool allow = M_SecretUnlocked(SECRET_ONLINE, true);
	if (!allow)
	{
		UINT16 j;
		for (j = 0; j < numfollowers; j++)
		{
			if (!K_FollowerUsable(j))
				continue;

			allow = true;
			break;
		}
	}

	if (allow)
		OPTIONS_Sound[sopt_followhorns].status = IT_STRING | IT_CVAR;

	OPTIONS_Sound[sopt_followhorns].tooltip = cv_tastelesstaunts.value
		? "Press B to announce that you are pressing B."
		: "Followers taunt your opponents when looking back at them.";

	if (M_SecretUnlocked(SECRET_TIMEATTACK, true) ||
		M_SecretUnlocked(SECRET_PRISONBREAK, true) ||
		M_SecretUnlocked(SECRET_SPECIALATTACK, true))
	{
		OPTIONS_Sound[sopt_attackmusic].status = IT_STRING | IT_CVAR;
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

	{IT_STRING | IT_ARROWS | IT_CV_SLIDER, "Volume", "Loudness of all game audio.",
		NULL, srb2::itemaction(slider_routine), 0, Slider::kMasterVolume},

	{IT_STRING | IT_ARROWS | IT_CV_SLIDER, "SFX Volume", "Loudness of sound effects.",
		NULL, srb2::itemaction(slider_routine), 0, Slider::kSfxVolume},

	{IT_STRING | IT_ARROWS | IT_CV_SLIDER, "Music Volume", "Loudness of music.",
		NULL, srb2::itemaction(slider_routine), 0, Slider::kMusicVolume},

	{IT_STRING | IT_ARROWS | IT_CV_SLIDER, "Voice Volume", "Loudness of voice chat.",
		NULL, srb2::itemaction(slider_routine), 0, Slider::kVoiceVolume},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Preferences...",  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Notifications", "Play a sound effect when chat messages appear.",
		NULL, srb2::itemaction(&cv_chatnotifications), 0, 0},

	{IT_STRING | IT_CVAR, "Character Voices", "Characters speak when interacting on the course.",
		NULL, srb2::itemaction(&cv_kartvoices), 0, 0},

	{IT_STRING | IT_CVAR, "Follower Horns", NULL, // set in init_routine
		NULL, srb2::itemaction(&cv_karthorns), 0, 0},

	{IT_STRING | IT_CVAR, "Continuous Attack Music", "Keep music playing seamlessly when retrying in Attack modes.",
		NULL, srb2::itemaction(&cv_continuousmusic), 0, 0},

	{IT_STRING | IT_CVAR, "Streamer-Safe Music", "Only play music safe for video platforms.",
		NULL, srb2::itemaction(&cv_streamersafemusic), 0, 0},

	{IT_SPACE | IT_DYBIGSPACE, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Advanced...",  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Hear Tabbed-out", "Keep playing game audio when the window is out of focus (FOCUS LOST).",
		NULL, srb2::itemaction(&cv_bgaudio), 0, 0},

	{IT_STRING | IT_CVAR, "Mixing Buffer Size", "Audio buffer size. Higher is faster but more delay.",
		NULL, srb2::itemaction(&cv_soundmixingbuffersize), 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "\x85" "Restart Audio", "Reboot the game's audio system.",
		NULL, srb2::itemaction(restartaudio_routine), 0, 0},
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
	init_routine,
	NULL,
	input_routine,
};
