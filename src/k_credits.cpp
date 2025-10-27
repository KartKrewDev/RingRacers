// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_credits.cpp
/// \brief Credits sequence

#include "k_credits.h"

#include <algorithm>
#include <fmt/format.h>

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_time.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_joy.h"
#include "i_threads.h"
#include "dehacked.h"
#include "g_input.h"
#include "console.h"
#include "m_random.h"
#include "m_misc.h" // moviemode functionality
#include "y_inter.h"
#include "m_cond.h"
#include "p_local.h"
#include "p_setup.h"
#include "st_stuff.h" // hud hiding
#include "fastcmp.h"
#include "r_fps.h"

#include "lua_hud.h"
#include "lua_hook.h"

// SRB2Kart
#include "k_menu.h"
#include "k_grandprix.h"
#include "music.h"
#include "r_main.h"
#include "m_easing.h"

using srb2::JsonArray;
using srb2::JsonObject;
using srb2::JsonValue;

enum credits_slide_types_e
{
	CRED_TYPE_SCROLL,
	CRED_TYPE_SLIDE,
	CRED_TYPE_TITLEDROP,
	CRED_TYPE_TYLER52,
	CRED_TYPE_KARTKREW,
	CRED_TYPE_SONGS,
	CRED_TYPE__MAX
};

struct credits_slide_s
{
	credits_slide_types_e type;
	srb2::String label;
	srb2::Vector<srb2::String> strings;
	size_t strings_height;
	boolean play_demo_afterwards;
	int fade_out_music;
};

static srb2::Vector<struct credits_slide_s> g_credits_slides;

struct credits_star_s
{
	fixed_t x, y;
	fixed_t vel_x, vel_y;
	INT32 frame;
};

static struct credits_s
{
	size_t current_slide;
	srb2::Vector<UINT16> demo_maps;
	boolean skip;

	srb2::Vector<struct credits_star_s> stars;

	fixed_t transition;
	fixed_t transition_prev;
	boolean transition_reverse;

	tic_t animation_timer;

	srb2::Vector<srb2::Vector<srb2::String>> split_slide_strings;
	size_t split_slide_id;
	tic_t split_slide_delay;

	UINT64 scroll_timer;
	UINT64 scroll_timer_prev;

	int tyler_fade;

	tic_t finish_counter;
	tic_t input_delay;

	tic_t demo_exit;

	boolean havent_ticked;
} g_credits;

constexpr const fixed_t kScrollFactor = FRACUNIT * 7 / 8;
constexpr const int kSkipSpeed = 8;
constexpr const int kScrollSkipSpeed = 4;

static bool g_deferred_continue_credits = false;

void F_LoadCreditsDefinitions(void)
{
	// Load credits definitions from bios.pk3
	if (g_credits_slides.empty() == false)
	{
		// TODO: Allow custom credits definition files.
		// Either append the new data to the start or the end. Not sure which would be better.
		return;
	}

	lumpnum_t credits_lump_id = W_GetNumForLongName("credits_def");
	size_t credits_lump_len = W_LumpLength(credits_lump_id);
	const char *credits_lump = static_cast<const char *>( W_CacheLumpNum(credits_lump_id, PU_CACHE) );

	srb2::String json_string { credits_lump, credits_lump_len };
	JsonValue credits_parsed = JsonValue::from_json_string(json_string);
	if (credits_parsed.is_array() == false)
	{
		I_Error("credits_def parse error: Not a JSON array");
		return;
	}
	JsonArray credits_array = credits_parsed.as_array();

	if (credits_array.size() == 0)
	{
		return;
	}

	try
	{
		for (JsonValue& slide_obj : credits_array)
		{
			struct credits_slide_s slide;

			srb2::String type_str = slide_obj.value("type", srb2::String("scroll"));

			if (type_str == "scroll")
			{
				slide.type = CRED_TYPE_SCROLL;
			}
			else if (type_str == "slide")
			{
				slide.type = CRED_TYPE_SLIDE;
			}
			else if (type_str == "titledrop")
			{
				slide.type = CRED_TYPE_TITLEDROP;
			}
			else if (type_str == "tyler52")
			{
				slide.type = CRED_TYPE_TYLER52;
			}
			else if (type_str == "kartkrew")
			{
				slide.type = CRED_TYPE_KARTKREW;
			}
			else if (type_str == "songs")
			{
#if 0
				slide.type = CRED_TYPE_SONGS;
#else
				// TODO
				continue;
#endif
			}
			else
			{
				throw std::runtime_error("unexpected type name '" + type_str + "'");
			}

			slide.label = slide_obj.value("label", srb2::String(""));

			slide.strings_height = 0;

			if (slide_obj.contains("strings"))
			{
				JsonValue strings_value = slide_obj.at("strings");
				if (strings_value.is_array() == true)
				{
					JsonArray& strings_array = strings_value.as_array();
					for (size_t i = 0; i < strings_array.size(); i++)
					{
						slide.strings.push_back( strings_array.at(i).get<srb2::String>() );

						if (slide.type == CRED_TYPE_SCROLL)
						{
							if (slide.strings[i].empty())
							{
								slide.strings_height += 40;
							}
							else
							{
								if (slide.strings[i].at(0) == '*')
								{
									slide.strings_height += 30;
								}
								else
								{
									slide.strings_height += 12;
								}
							}
						}
						else if (slide.type == CRED_TYPE_SLIDE)
						{
							slide.strings_height += 30;
						}
					}
				}
			}

			slide.play_demo_afterwards = slide_obj.value("demo", false);
			slide.fade_out_music = slide_obj.value("fade_out_music", 0);

			g_credits_slides.push_back( slide );
		}
	}
	catch (const std::exception& ex)
	{
		I_Error("credits_def parse error: %s", ex.what());
	}
}

void F_CreditsReset(void)
{
	g_credits.stars.clear();
	g_credits.split_slide_strings.clear();
	g_credits.split_slide_id = 0;
	g_credits.split_slide_delay = 0;

	g_credits.transition = g_credits.transition_prev = 0;
	g_credits.transition_reverse = false;

	g_credits.scroll_timer = g_credits.scroll_timer_prev = 0;
	g_credits.animation_timer = 0;
	g_credits.tyler_fade = 0;

	g_credits.finish_counter = 0;
	g_credits.demo_exit = 0;

	g_credits.havent_ticked = true; // fucking stupid bullshit
}

static void F_InitCreditsSlide(void)
{
	struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];

	if (slide->type == CRED_TYPE_SLIDE)
	{
		// How many can be shown on one screen
		constexpr const size_t kMaxSlideStrings = 4;
		const size_t num_strings = slide->strings.size();

		if (num_strings <= kMaxSlideStrings)
		{
			// Our job is easy. Simply copy what is already there.
			g_credits.split_slide_strings.push_back( slide->strings );
		}
		else
		{
			// Try to divide it up relatively evenly into multiple sub-slides.
			const size_t num_sub_screens = (num_strings - 1) / kMaxSlideStrings + 1;
			size_t max_strings_per_screen = (num_strings - 1) / num_sub_screens + 1;

			size_t str_id = 0;
			srb2::Vector<srb2::String> screen_strings;

			if (max_strings_per_screen == kMaxSlideStrings
				&& num_strings % kMaxSlideStrings == 1)
			{
				// 13 strings is unlucky and creates a page
				// that only has one name on the end.
				// This fixes it.
				screen_strings.emplace_back( slide->strings[str_id] );
				str_id++;
				max_strings_per_screen--;
			}

			while (str_id < num_strings)
			{
				for (size_t i = 0; i < max_strings_per_screen; i++)
				{
					screen_strings.emplace_back( slide->strings[str_id] );
					str_id++;

					if (str_id >= num_strings)
					{
						break;
					}
				}

				g_credits.split_slide_strings.push_back( screen_strings );
				screen_strings.clear();
			}
		}
	}
#if 0
	else if (slide->type == CRED_TYPE_SONGS)
	{
		// Auto fill out with music credits
		slide->strings.clear();

		slide->strings.push_back("*MUSIC");
		slide->strings_height += 30;

		// I do not even remotely understand the sequence code even a little bit
		// so SOMEONE ELSE can do it! I don't care!
		// ---
		// "I know how it work's" - toast 110124
		musicdef_t *def = soundtest.sequence.next;
		while (def)
		{
			if (def->title
#if 0 // Let's not make the credits variable-length
			&& !S_SoundTestDefLocked(def)
#endif
			)
			{
				slide->strings.push_back("#" + srb2::String(def->title));
				slide->strings_height += 12;

				if (def->author)
				{
					slide->strings.push_back("by " + srb2::String(def->author));
					slide->strings_height += 12;
				}

				if (def->source)
				{
					slide->strings.push_back("from " + srb2::String(def->source));
					slide->strings_height += 12;
				}

				if (def->composers)
				{
					slide->strings.push_back("originally by " + srb2::String(def->composers));
					slide->strings_height += 12;
				}

				slide->strings.push_back(" ");
				slide->strings_height += 12;
			}

			def = def->sequence.next;
		}
	}
#endif

	// Clear the console hud just to avoid anything getting in the way.
	CON_ClearHUD();
}

static void F_NewCreditsMusic(const char *trackname, bool looping)
{
	Music_Remap("credits", trackname);
	Music_Loop("credits", looping);
	Music_Play("credits");
}

void F_ConsiderCreditsMusicUpdate(void)
{
	if (!Music_CanLoop("credits") && I_GetSongLength() == I_GetSongPosition())
	{
		F_NewCreditsMusic("_title", true);
	}
}

void F_StartCredits(void)
{
	// Prepare evaluation screen at the end
	F_InitGameEvaluation();

	G_SetGamestate(GS_CREDITS);

	// Just in case they're open ... somehow
	M_ClearMenus(true);

	if (g_credits_cutscene)
	{
		F_StartCustomCutscene(g_credits_cutscene - 1, false, false);
		return;
	}

	gameaction = ga_nothing;
	paused = false;

	CON_ToggleOff();
	Music_StopAll();
	S_StopSounds();

	Music_Play("credits_silence"); // mask any music changes from demos
	F_NewCreditsMusic("_creds", false);

	F_CreditsReset();

	g_credits.demo_maps.clear();
	g_credits.current_slide = 0;

	g_credits.input_delay = TICRATE;
	g_credits.skip = false;

	F_InitCreditsSlide();
}

static void F_CreditsNextSlide(void)
{
	F_CreditsReset();

	g_credits.current_slide++;
	if (g_credits.current_slide >= g_credits_slides.size())
	{
		// You watched all the credits? What a trooper!
		gamedata->everfinishedcredits = true;

		if (M_UpdateUnlockablesAndExtraEmblems(true, true))
		{
			G_SaveGameData();
		}

		F_StartGameEvaluation();
		return;
	}

	F_InitCreditsSlide();
}

void F_DeferContinueCredits(void)
{
	g_deferred_continue_credits = true;
	demo.attract = DEMO_ATTRACT_OFF;
}

boolean F_IsDeferredContinueCredits(void)
{
	return g_deferred_continue_credits;
}

void F_ContinueCredits(void)
{
	g_deferred_continue_credits = false;
	G_SetGamestate(GS_CREDITS);
	F_CreditsReset();
	demo.attract = DEMO_ATTRACT_OFF;

	// Do not wipe back to credits, since F_CreditsDemoExitFade exists.
	wipegamestate = GS_LEVEL;

	// Returning from playing a demo.
	// Go to the next slide.
	F_CreditsNextSlide();
}

static UINT16 F_PickRandomCreditsDemoMap(void)
{
	srb2::Vector<UINT16> allowedMaps;

	for (INT32 i = 0; i < basenummapheaders; i++) // Only take from the base game.
	{
		if (mapheaderinfo[i] == NULL || mapheaderinfo[i]->lumpnum == LUMPERROR)
		{
			// Doesn't exist?
			continue;
		}

		if ((mapheaderinfo[i]->typeoflevel & TOL_RACE) == 0)
		{
			// We want Race gametype demos, since they will be
			// the most suited to the "camera left behind" effect.
			continue;
		}

		if (mapheaderinfo[i]->ghostCount == 0)
		{
			// It doesn't have any demos...
			continue;
		}

		if ((mapheaderinfo[i]->menuflags & LF2_HIDEINMENU) == LF2_HIDEINMENU)
		{
			// Secret map.
			continue;
		}

		if (M_MapLocked(i + 1) == true)
		{
			// We haven't earned this one.
			continue;
		}

		if (std::find(g_credits.demo_maps.begin(), g_credits.demo_maps.end(), i) != g_credits.demo_maps.end())
		{
			// Already was added.
			continue;
		}

		// Got past the gauntlet, so we can allow this one.
		allowedMaps.push_back(i);
	}

	if (allowedMaps.size() > 0)
	{
		return allowedMaps[ M_RandomKey(allowedMaps.size()) ];
	}

	return UINT16_MAX;
}

static boolean F_CreditsPlayDemo(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];
	staffbrief_t *brief;

	if (slide->play_demo_afterwards == false)
	{
		return false;
	}

	if (g_credits.skip == true)
	{
		return false;
	}

	UINT16 map_id = F_PickRandomCreditsDemoMap();
	if (map_id == UINT16_MAX)
	{
		return false;
	}
	g_credits.demo_maps.push_back(map_id);

	UINT8 ghost_id = M_RandomKey( mapheaderinfo[map_id]->ghostCount );
	brief = mapheaderinfo[map_id]->ghostBrief[ghost_id];

	demo.attract = DEMO_ATTRACT_CREDITS;
	demo.ignorefiles = true;
	demo.loadfiles = false;

	G_DoPlayDemoEx("", (brief->wad << 16) | brief->lump);

	g_fast_forward = 30 * TICRATE;
	// Slow computers, don't wait all day
	g_fast_forward_clock_stop = I_GetTime() + 2 * TICRATE;
	g_credits.demo_exit = 0;
	return true;
}

constexpr const unsigned int kDemoExitTicCount = 10 * TICRATE;

void F_TickCreditsDemoExit(void)
{
	g_credits.demo_exit++;

	if (!menuactive && M_MenuConfirmPressed(0))
	{
		g_credits.demo_exit = std::max<tic_t>(g_credits.demo_exit, kDemoExitTicCount - 64);
	}

	if (INT32 val = F_CreditsDemoExitFade(); val >= 0)
	{
		// Fade down sounds with screen fade
		I_SetSfxVolume(cv_soundvolume.value * (31 - val) / 31);
	}

	if (g_credits.demo_exit > kDemoExitTicCount)
	{
		G_CheckDemoStatus();
	}
}

INT32 F_CreditsDemoExitFade(void)
{
	return std::clamp<INT32>(
		31 - ((kDemoExitTicCount - static_cast<INT32>(g_credits.demo_exit)) / 2),
		-1, 31
	);
}

static void F_CreditsSlideFinish(void)
{
	if (F_CreditsPlayDemo() == true)
	{
		return;
	}

	if (g_credits.skip == true)
	{
		// FIXME: use shorter wipe, instead of no wipe
		wipegamestate = GS_CREDITS;
	}
	else
	{
		wipegamestate = GS_NULL;
	}

	F_CreditsNextSlide();
}

static boolean F_TickCreditsScroll(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];
	UINT32 scroll_max = FixedDiv(slide->strings_height + BASEVIDHEIGHT, kScrollFactor);

	if (g_credits.skip == true)
	{
		g_credits.scroll_timer += kScrollSkipSpeed;
	}
	else
	{
		g_credits.scroll_timer++;
	}

	if (g_credits.scroll_timer > scroll_max)
	{
		g_credits.scroll_timer = scroll_max;
	}

	return (g_credits.scroll_timer >= scroll_max - (2 * TICRATE));
}

static void F_CreditsStarParticle(fixed_t x, fixed_t y)
{
	struct credits_star_s star;

	star.x = x;
	star.y = y;
	star.frame = 0;

	star.vel_x = M_RandomRange(-2, 2) * FRACUNIT / 4;

	g_credits.stars.push_back(star);
}

static boolean F_TickCreditsSlide(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];

	if (g_credits.split_slide_delay > 0)
	{
		g_credits.split_slide_delay--;
		return false;
	}

	if (g_credits.transition < FRACUNIT)
	{
		g_credits.transition = std::min<INT32>(g_credits.transition + (FRACUNIT / TICRATE), FRACUNIT);

		if (g_credits.split_slide_id < g_credits.split_slide_strings.size())
		{
			constexpr const fixed_t label_space = 30 * FRACUNIT;
			const fixed_t strings_height = g_credits.split_slide_strings[ g_credits.split_slide_id ].size() * 30 * FRACUNIT;

			fixed_t y = 0;
			if (slide->label.empty() == false)
			{
				y += label_space;
			}
			y += ((BASEVIDHEIGHT * FRACUNIT) - y) / 2;
			y -= strings_height / 2;

			UINT8 side = 0;
			UINT8 star_index = 0;

			for (auto& str : g_credits.split_slide_strings[ g_credits.split_slide_id ])
			{
				const fixed_t str_width = V_StringScaledWidth(
					FRACUNIT, FRACUNIT, FRACUNIT,
					0, LSLOW_FONT,
					str.c_str()
				);

				fixed_t x = 32 * FRACUNIT;
				fixed_t slide_out = -BASEVIDWIDTH * FRACUNIT;

				if (side == 1)
				{
					x = (BASEVIDWIDTH * FRACUNIT) - x - str_width;
					slide_out = -slide_out;
				}
				else
				{
					x += str_width;
				}

				fixed_t ease = 0;
				if (g_credits.transition_reverse)
				{
					ease = Easing_InOutSine(g_credits.transition, 0, -slide_out);
				}
				else
				{
					ease = Easing_InOutSine(g_credits.transition, slide_out, 0);
				}

				if ((g_credits.animation_timer + star_index) % 2 == 0)
				{
					F_CreditsStarParticle(
						x + ease,
						y + (16 * FRACUNIT)
					);
				}

				y += 30 * FRACUNIT;
				side = (side + 1) & 1;
				star_index++;
			}
		}

		return false;
	}

	if (g_credits.split_slide_id < g_credits.split_slide_strings.size() - 1)
	{
		if (g_credits.transition_reverse)
		{
			g_credits.split_slide_id++;
		}
		else
		{
			g_credits.split_slide_delay = 2*TICRATE;
		}

		g_credits.transition = g_credits.transition_prev = 0;
		g_credits.transition_reverse = !g_credits.transition_reverse;
		return false;
	}

	return true;
}

static boolean F_TickCreditsTyler52(void)
{
	if (g_credits.animation_timer > TICRATE && g_credits.animation_timer < (2*TICRATE) - 17)
	{
		g_credits.tyler_fade++;
	}
	else
	{
		g_credits.tyler_fade--;
	}

	g_credits.tyler_fade = std::clamp(g_credits.tyler_fade, 0, 8);
	return true;
}

static void F_TickCreditsStars(void)
{
	for (auto& star : g_credits.stars)
	{
		star.vel_y += FRACUNIT / 4;

		if (g_credits.animation_timer % 2 == 0)
		{
			star.frame++;
		}
	}

	g_credits.stars.erase(
		std::remove_if(
			g_credits.stars.begin(),
			g_credits.stars.end(),
			[](struct credits_star_s const &star) { return star.frame > 11; }
		),
		g_credits.stars.end()
	);
}

static void F_HandleCreditsTick(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];

	g_credits.animation_timer++;
	F_TickCreditsStars();

	boolean finalize_slide = true;
	switch (slide->type)
	{
		case CRED_TYPE_SCROLL:
		{
			finalize_slide = F_TickCreditsScroll();
			break;
		}
		case CRED_TYPE_SLIDE:
		{
			finalize_slide = F_TickCreditsSlide();
			break;
		}
		case CRED_TYPE_TYLER52:
		{
			finalize_slide = F_TickCreditsTyler52();
			break;
		}
		case CRED_TYPE_SONGS:
		{
			// TODO
			break;
		}
		default:
		{
			break;
		}
	}

	if (g_credits.finish_counter > 0)
	{
		g_credits.finish_counter--;

		if (g_credits.finish_counter == 0)
		{
			F_CreditsSlideFinish();
		}

		return;
	}
	else if (finalize_slide)
	{
		if (slide->fade_out_music)
		{
			I_FadeSong(0, slide->fade_out_music, nullptr);
		}

		if (g_credits.current_slide >= g_credits_slides.size() - 1)
		{
			g_credits.finish_counter = 5 * TICRATE;
		}
		else
		{
			g_credits.finish_counter = 2 * TICRATE;
		}
	}
}

void F_CreditTicker(void)
{
	g_credits.havent_ticked = false;

	F_ConsiderCreditsMusicUpdate();

	g_credits.transition_prev = g_credits.transition;
	g_credits.scroll_timer_prev = g_credits.scroll_timer;

	if (g_credits.input_delay > 0)
	{
		g_credits.input_delay--;
	}
	else
	{
		g_credits.skip = (!menuactive && M_MenuConfirmHeld(0));
	}

	if (g_credits.current_slide >= g_credits_slides.size() - 1)
	{
		// Don't skip the last slide
		g_credits.skip = false;
	}

	if (g_credits.skip == true)
	{
		for (size_t i = 0; i < kSkipSpeed; i++)
		{
			F_HandleCreditsTick();
		}
	}
	else
	{
		F_HandleCreditsTick();
	}
}

static void F_DrawCreditsScroll(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];

	fixed_t y = (BASEVIDHEIGHT * FRACUNIT);

	y -= Easing_Linear(
		rendertimefrac,
		g_credits.scroll_timer_prev * kScrollFactor,
		g_credits.scroll_timer * kScrollFactor
	);

	for (auto& str : slide->strings)
	{
		if (str.empty())
		{
			y += 40 * FRACUNIT;
		}
		else
		{
			srb2::String new_str = str;

			if (new_str.at(0) == '*')
			{
				if (y > -20 * FRACUNIT)
				{
					new_str.erase(0, 1);

					const fixed_t str_width = V_StringScaledWidth(
						FRACUNIT, FRACUNIT, FRACUNIT,
						0, LSLOW_FONT,
						new_str.c_str()
					);

					V_DrawStringScaled(
						((BASEVIDWIDTH * FRACUNIT) - str_width) / 2, y,
						FRACUNIT, FRACUNIT, FRACUNIT,
						0, nullptr, LSLOW_FONT,
						new_str.c_str()
					);
				}

				y += 30 * FRACUNIT;
			}
			else
			{
				if (y > -10 * FRACUNIT)
				{
					if (new_str.at(0) == '#')
					{
						new_str.erase(0, 1);

						const fixed_t str_width = V_StringScaledWidth(
							FRACUNIT, FRACUNIT, FRACUNIT,
							0, MENU_FONT,
							new_str.c_str()
						);

						V_DrawStringScaled(
							((BASEVIDWIDTH * FRACUNIT) - str_width) / 2, y,
							FRACUNIT, FRACUNIT, FRACUNIT,
							V_YELLOWMAP, nullptr, MENU_FONT,
							new_str.c_str()
						);
					}
					else
					{
						V_DrawStringScaled(
							32 * FRACUNIT, y,
							FRACUNIT, FRACUNIT, FRACUNIT,
							0, nullptr, MENU_FONT,
							new_str.c_str()
						);
					}
				}

				y += 12 * FRACUNIT;
			}
		}

		if (((y / FRACUNIT) * vid.dupy) > vid.height)
		{
			break;
		}
	}

	if (slide->label.empty() == false)
	{
		const fixed_t label_width = V_StringScaledWidth(
			FRACUNIT, FRACUNIT, FRACUNIT,
			0, LSHI_FONT,
			slide->label.c_str()
		);
		V_DrawStringScaled(
			((BASEVIDWIDTH * FRACUNIT) - label_width) / 2, 15 * FRACUNIT,
			FRACUNIT, FRACUNIT, FRACUNIT,
			0, nullptr, LSHI_FONT,
			slide->label.c_str()
		);
	}
}

static void F_DrawCreditsSlide(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];
	constexpr const fixed_t label_space = 30 * FRACUNIT;

	const fixed_t transition = Easing_Linear(rendertimefrac, g_credits.transition_prev, g_credits.transition);

	const fixed_t label_width = V_StringScaledWidth(
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, LSHI_FONT,
		slide->label.c_str()
	);
	V_DrawStringScaled(
		((BASEVIDWIDTH * FRACUNIT) - label_width) / 2, label_space / 2,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, nullptr, LSHI_FONT,
		slide->label.c_str()
	);

	if (g_credits.split_slide_id >= g_credits.split_slide_strings.size())
	{
		return;
	}

	const srb2::Vector<srb2::String> *slide_strings = &g_credits.split_slide_strings[ g_credits.split_slide_id ];
	const fixed_t strings_height = slide_strings->size() * 30 * FRACUNIT;

	fixed_t y = 0;
	if (slide->label.empty() == false)
	{
		y += label_space;
	}
	y += ((BASEVIDHEIGHT * FRACUNIT) - y) / 2;
	y -= strings_height / 2;

	UINT8 side = 0;

	for (auto& str : g_credits.split_slide_strings[ g_credits.split_slide_id ])
	{
		const fixed_t str_width = V_StringScaledWidth(
			FRACUNIT, FRACUNIT, FRACUNIT,
			0, LSLOW_FONT,
			str.c_str()
		);

		fixed_t x = 32 * FRACUNIT;
		fixed_t slide_out = -BASEVIDWIDTH * FRACUNIT;

		if (side == 1)
		{
			x = (BASEVIDWIDTH * FRACUNIT) - x - str_width;
			slide_out = -slide_out;
		}

		fixed_t ease = 0;
		if (g_credits.transition_reverse)
		{
			ease = Easing_InOutSine(transition, 0, -slide_out);
		}
		else
		{
			ease = Easing_InOutSine(transition, slide_out, 0);
		}

		V_DrawStringScaled(
			x + ease, y,
			FRACUNIT, FRACUNIT, FRACUNIT,
			0, nullptr, LSLOW_FONT,
			str.c_str()
		);

		y += 30 * FRACUNIT;
		side = (side + 1) & 1;
	}
}

static void F_DrawCreditsTitleDrop(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];

	V_DrawFixedPatch(
		4 * FRACUNIT, -BASEVIDHEIGHT * (FRACUNIT / 2),
		FRACUNIT, 0,
		static_cast<patch_t *>(W_CachePatchName(
			(M_UseAlternateTitleScreen() ? "KTSJUMPR1" : "KTSBUMPR1"),
			PU_CACHE
		)),
		nullptr
	);

	const fixed_t label_width = V_StringScaledWidth(
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, LSHI_FONT,
		slide->label.c_str()
	);
	V_DrawStringScaled(
		((BASEVIDWIDTH * FRACUNIT) - label_width) / 2, 120 * FRACUNIT,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, nullptr, LSHI_FONT,
		slide->label.c_str()
	);
}

static void F_DrawCreditsTyler52(void)
{
	patch_t *tyler_patch = static_cast<patch_t *>(W_CachePatchName("TYLER52", PU_CACHE));
	V_DrawStretchyFixedPatch(
		0, 0,
		(BASEVIDWIDTH * FRACUNIT) / tyler_patch->width,
		(BASEVIDHEIGHT * FRACUNIT) / tyler_patch->height,
		V_90TRANS,
		tyler_patch,
		nullptr
	);

	if (g_credits.tyler_fade < 8)
	{
		V_DrawFadeScreen(0xFF00, 31 - (g_credits.tyler_fade * 4));
	}

	srb2::String memory_str = "In memory of";
	const fixed_t memory_width = V_StringScaledWidth(
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, LSLOW_FONT,
		memory_str.c_str()
	);
	V_DrawStringScaled(
		((BASEVIDWIDTH * FRACUNIT) - memory_width) / 2, 60 * FRACUNIT,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, nullptr, LSLOW_FONT,
		memory_str.c_str()
	);

	srb2::String tyler_str = "Tyler52";
	const fixed_t tyler_width = V_StringScaledWidth(
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, LSHI_FONT,
		tyler_str.c_str()
	);
	V_DrawStringScaled(
		((BASEVIDWIDTH * FRACUNIT) - tyler_width) / 2, 110 * FRACUNIT,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, nullptr, LSHI_FONT,
		tyler_str.c_str()
	);
}

static void F_DrawCreditsKartKrew(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];

	const fixed_t label_width = V_StringScaledWidth(
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, LSLOW_FONT,
		slide->label.c_str()
	);
	V_DrawStringScaled(
		((BASEVIDWIDTH * FRACUNIT) - label_width) / 2, 40 * FRACUNIT,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, nullptr, LSLOW_FONT,
		slide->label.c_str()
	);

	V_DrawFixedPatch(
		111 * FRACUNIT, 70 * FRACUNIT,
		FRACUNIT / 2, 0,
		static_cast<patch_t *>(W_CachePatchName(
			"KKLOGO_C",
			PU_CACHE
		)),
		nullptr
	);

	V_DrawFixedPatch(
		111 * FRACUNIT, 70 * FRACUNIT,
		FRACUNIT / 2, 0,
		static_cast<patch_t *>(W_CachePatchName(
			"KKTEXT_C",
			PU_CACHE
		)),
		nullptr
	);
}

void F_CreditDrawer(void)
{
	const struct credits_slide_s *slide = &g_credits_slides[ g_credits.current_slide ];
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	switch (slide->type)
	{
		case CRED_TYPE_SCROLL:
		{
			F_DrawCreditsScroll();
			break;
		}
		case CRED_TYPE_SLIDE:
		{
			F_DrawCreditsSlide();
			break;
		}
		case CRED_TYPE_TITLEDROP:
		{
			F_DrawCreditsTitleDrop();
			break;
		}
		case CRED_TYPE_TYLER52:
		{
			F_DrawCreditsTyler52();
			break;
		}
		case CRED_TYPE_KARTKREW:
		{
			F_DrawCreditsKartKrew();
			break;
		}
		case CRED_TYPE_SONGS:
		{
			// TODO
			break;
		}
		default:
		{
			break;
		}
	}

	UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_JAWZ, GTC_CACHE);
	for (auto& star : g_credits.stars)
	{
		V_DrawFixedPatch(
			star.x, star.y,
			FRACUNIT / 2, V_ADD,
			static_cast<patch_t *>(W_CachePatchName(
				va("KINB%c0", 'A' + star.frame),
				PU_CACHE
			)),
			colormap
		);

		star.x += FixedMul(star.vel_x, renderdeltatics);
		star.y += FixedMul(star.vel_y, renderdeltatics);
	}
}
