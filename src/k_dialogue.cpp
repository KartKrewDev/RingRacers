// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
// Copyright (C) 2020 by Sonic Team Junior
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_dialogue.cpp
/// \brief Basic text prompts

#include "k_dialogue.hpp"
#include "k_dialogue.h"

#include <algorithm>
#include <string_view>

#include "core/string.h"
#include "info.h"
#include "sounds.h"
#include "g_game.h"
#include "v_video.h"
#include "r_draw.h"
#include "m_easing.h"
#include "r_skins.h"
#include "s_sound.h"
#include "z_zone.h"
#include "k_hud.h"
#include "p_tick.h" // P_LevelIsFrozen

#include "v_draw.hpp"

#include "acs/interface.h"

using srb2::Dialogue;

// Dialogue::Typewriter

void Dialogue::Typewriter::ClearText(void)
{
	text.clear();
	textDest.clear();
}

void Dialogue::Typewriter::NewText(const srb2::String& newText)
{
	text.clear();

	textDest = newText;
	std::reverse(textDest.begin(), textDest.end());

	textTimer = kTextPunctPause;
	textSpeed = kTextSpeedDefault;
	textDone = false;
	textLines = 1;

	syllable = true;
}

void Dialogue::Typewriter::WriteText(void)
{
	if (textDone)
		return;

	bool voicePlayed = false;
	bool empty = textDest.empty();

	textTimer -= textSpeed;

	while (textTimer <= 0 && !empty)
	{
		char c = textDest.back(), nextc = '\n';
		text.push_back(c);

		textDest.pop_back();
		empty = textDest.empty();

		if (c & 0x80)
		{
			// Color code support
			continue;
		}

		if (c == '\n')
			textLines++;

		if (!empty)
			nextc = textDest.back();

		if (voicePlayed == false
			&& std::isprint(c)
			&& c != ' ')
		{
			if (syllable)
			{
				S_StopSoundByNum(voiceSfx);
				S_StartSound(nullptr, voiceSfx);
			}

			syllable = !syllable;
			voicePlayed = true;
		}

		if (c == '-' && empty)
		{
			textTimer += textSpeed;
		}
		else if (c != '+' && c != '"' // tutorial hack
			&& std::ispunct(c)
			&& std::isspace(nextc))
		{
			// slow down for punctuation
			textTimer += kTextPunctPause;
		}
		else
		{
			textTimer += FRACUNIT;
		}
	}

	textDone = (textTimer <= 0 && empty);
}

void Dialogue::Typewriter::CompleteText(void)
{
	while (!textDest.empty())
	{
		char c = textDest.back();

		if (c == '\n')
			textLines++;

		text.push_back( c );
		textDest.pop_back();
	}

	textTimer = 0;
	textDone = true;
}

// Dialogue

void Dialogue::Init(void)
{
	if (!active)
	{
		auto cache = [](const char* name)
		{
			return std::pair {std::string_view {name}, srb2::Draw::cache_patch(name)};
		};
		patchCache = {
			cache("TUTDIAGA"),
			cache("TUTDIAGB"),
			cache("TUTDIAGC"),
			cache("TUTDIAGD"),
			cache("TUTDIAGF"),
			cache("TUTDIAGE"),
			cache("TUTDIAG2"),
		};
	}

	active = true;
}

void Dialogue::SetSpeaker(void)
{
	// Unset speaker
	speaker.clear();

	portrait = nullptr;
	portraitColormap = nullptr;

	typewriter.voiceSfx = sfx_ktalk;
}

void Dialogue::SetSpeaker(srb2::String skinName, int portraitID)
{
	Init();

	// Set speaker based on a skin
	int skinID = -1;
	if (!skinName.empty())
	{
		skinID = R_SkinAvailable(skinName.c_str());
	}

	if (skinID >= 0 && skinID < numskins)
	{
		const skin_t *skin = skins[skinID];
		const spritedef_t *sprdef = &skin->sprites[SPR2_TALK];

		if (sprdef->numframes > 0)
		{
			portraitID %= sprdef->numframes;

			const spriteframe_t *sprframe = &sprdef->spriteframes[portraitID];

			portrait = static_cast<patch_t *>( W_CachePatchNum(sprframe->lumppat[0], PU_CACHE) );
			portraitColormap = R_GetTranslationColormap(skinID, static_cast<skincolornum_t>(skin->prefcolor), GTC_CACHE);
		}
		else
		{
			portrait = nullptr;
			portraitColormap = nullptr;
		}

		speaker = skin->realname;

		typewriter.voiceSfx = skin->soundsid[ S_sfx[sfx_ktalk].skinsound ];
	}
	else
	{
		SetSpeaker();
	}
}

void Dialogue::SetSpeaker(srb2::String name, patch_t *patch, UINT8 *colormap, sfxenum_t voice)
{
	Init();

	// Set custom speaker
	speaker = name;

	if (speaker.empty())
	{
		portrait = nullptr;
		portraitColormap = nullptr;
		typewriter.voiceSfx = sfx_ktalk;
		return;
	}

	portrait = patch;
	portraitColormap = colormap;

	typewriter.voiceSfx = voice;
}

void Dialogue::NewText(std::string_view rawText)
{
	Init();

	char* newText = V_ScaledWordWrap(
		275 << FRACBITS,
		FRACUNIT, FRACUNIT, FRACUNIT,
		0, HU_FONT,
		srb2::Draw::TextElement().parse(rawText).string().c_str() // parse special characters
	);

	typewriter.NewText(newText);

	Z_Free(newText);
}

bool Dialogue::Active(void)
{
	return active;
}

bool Dialogue::TextDone(void)
{
	return typewriter.textDone;
}

bool Dialogue::Dismissable(void)
{
	return dismissable;
}

void Dialogue::SetDismissable(bool value)
{
	dismissable = value;
}

bool Dialogue::Held(void)
{
	return ((players[serverplayer].cmd.buttons & BT_VOTE) == BT_VOTE);
}

bool Dialogue::Pressed(void)
{
	return (
		((players[serverplayer].cmd.buttons & BT_VOTE) == BT_VOTE) &&
		((players[serverplayer].oldcmd.buttons & BT_VOTE) == 0)
	);
}

void Dialogue::Tick(void)
{
	if (Active())
	{
		if (slide < FRACUNIT)
		{
			slide += kSlideSpeed;
		}

		if (P_LevelIsFrozen() || (gametyperules & GTR_BOSS))
		{
			if (fade > 0)
			{
				fade--;
			}
		}
		else if (fade < 5)
		{
			fade++;
		}
	}
	else
	{
		if (fade > 0)
		{
			fade--;
		}

		if (slide > 0)
		{
			slide -= kSlideSpeed;

			if (slide <= 0)
			{
				Unset();
			}
		}
	}

	slide = std::clamp<size_t>(slide, 0, FRACUNIT);

	if (slide != FRACUNIT)
	{
		return;
	}

	typewriter.WriteText();

	if (Dismissable() == true)
	{
		if (Pressed() == true)
		{
			if (TextDone())
			{
				Dismiss();
			}
			else
			{
				typewriter.CompleteText();
			}
		}
	}
}

INT32 Dialogue::SlideAmount(fixed_t multiplier)
{
	if (slide == 0)
		return 0;
	if (slide == FRACUNIT)
		return multiplier;
	return Easing_OutCubic(slide, 0, multiplier);
}

INT32 Dialogue::FadeAmount(void)
{
	return fade;
}

void Dialogue::Draw(void)
{
	if (slide == 0)
	{
		return;
	}

	const UINT8 bgcol = 235, speakerhilicol = 240;

	const fixed_t height = 78 * FRACUNIT;

	INT32 speakernameedge = -6;

	srb2::Draw drawer =
		srb2::Draw(
			BASEVIDWIDTH, BASEVIDHEIGHT - FixedToFloat(SlideAmount(height) - height)
		).flags(V_SNAPTOBOTTOM);

	// TODO -- hack, change when dialogue is made per-player/netsynced
	UINT32 speakerbgflags = (players[consoleplayer].nocontrol == 0 && P_LevelIsFrozen() == false)
		? V_30TRANS
		: 0;

	drawer
		.flags(speakerbgflags|V_VFLIP|V_FLIP)
		.patch(patchCache["TUTDIAGA"]);

	drawer
		.flags(V_VFLIP|V_FLIP)
		.patch(patchCache["TUTDIAGB"]);

	if (portrait != nullptr)
	{
		drawer
			.flags(V_VFLIP|V_FLIP)
			.patch(patchCache["TUTDIAGC"]);

		drawer
			.xy(-10-32, -41-32)
			.colormap(portraitColormap)
			.patch(portrait);

		speakernameedge -= 39; // -45
	}

	const char *speakername = speaker.c_str();

	const INT32 arrowstep = 8; // width of TUTDIAGD

	if (speakername && speaker[0])
	{
		INT32 speakernamewidth = V_MenuStringWidth(speakername, 0);
		INT32 existingborder = (portrait == nullptr ? -4 : 3);

		INT32 speakernamewidthoffset = (speakernamewidth + (arrowstep - existingborder) - 2) % arrowstep;
		if (speakernamewidthoffset)
		{
			speakernamewidthoffset = (arrowstep - speakernamewidthoffset);
			speakernamewidth += speakernamewidthoffset;
		}

		if (portrait == nullptr)
		{
			speakernameedge -= 3;
			speakernamewidth += 3;
			existingborder += 2;
			drawer
				.xy(speakernameedge, -36)
				.width(2)
				.height(3+11)
				.fill(bgcol);
		}

		if (speakernamewidth > existingborder)
		{
			drawer
				.x(speakernameedge - speakernamewidth)
				.width(speakernamewidth - existingborder)
				.y(-36-3)
				.height(3)
				.fill(bgcol);

			drawer
				.x(speakernameedge - speakernamewidth)
				.width(speakernamewidth - existingborder)
				.y(-38-11)
				.height(11)
				.fill(speakerhilicol);
		}

		speakernameedge -= speakernamewidth;

		drawer
			.xy(speakernamewidthoffset + speakernameedge, -39-9)
			.font(srb2::Draw::Font::kMenu)
			.text(speakername);

		speakernameedge -= 5;

		drawer
			.xy(speakernameedge, -36)
			.flags(V_VFLIP|V_FLIP)
			.patch(patchCache["TUTDIAGD"]);

		drawer
			.xy(speakernameedge, -36-3-11)
			.width(5)
			.height(3+11)
			.fill(bgcol);

		drawer
			.xy(speakernameedge + 5, -36)
			.flags(V_VFLIP|V_FLIP)
			.patch(patchCache["TUTDIAGF"]);
	}

	while (speakernameedge > -142) // the left-most edge
	{
		speakernameedge -= arrowstep;

		drawer
			.xy(speakernameedge, -36)
			.flags(V_VFLIP|V_FLIP)
			.patch(patchCache["TUTDIAGD"]);
	}

	drawer
		.xy(speakernameedge - arrowstep, -36)
		.flags(V_VFLIP|V_FLIP)
		.patch(patchCache["TUTDIAGE"]);

	srb2::String intertext = "<large>";

	drawer
		.xy(10 - BASEVIDWIDTH, -3-32)
		.font(srb2::Draw::Font::kConsole)
		.text( typewriter.text.c_str() );

	if (TextDone())
	{
		drawer
			.xy(-18 - 3, -7-5)
			.patch(patchCache["TUTDIAG2"]);

		if (Held())
			intertext += "<z_pressed>";
		else
			intertext += "<z_animated>";

		drawer.xy(-18, -7-8 - 14).align(Draw::Align::kCenter).font(Draw::Font::kMenu).text(srb2::Draw::TextElement().parse(intertext).string());
	}
}

void Dialogue::Dismiss(void)
{
	active = false;
	typewriter.ClearText();
}

UINT32 Dialogue::GetNewEra(void)
{
	return (++current_era);
}

bool Dialogue::EraIsValid(INT32 comparison)
{
	return (current_era == comparison);
}

void Dialogue::Unset(void)
{
	Dismiss();
	SetSpeaker();
	slide = 0;
	fade = 0;
	current_era = 0;
}

/*
	Ideally, the Dialogue class would be on player_t instead of in global space
	for full multiplayer compatibility, but right now it's only being used for
	the tutorial, and I don't feel like writing network code. If you feel like
	doing that, then you can remove g_dialogue entirely.
*/

Dialogue g_dialogue;

void K_UnsetDialogue(void)
{
	g_dialogue.Unset();
}

void K_DrawDialogue(void)
{
	g_dialogue.Draw();
}

void K_TickDialogue(void)
{
	g_dialogue.Tick();
}

INT32 K_GetDialogueSlide(fixed_t multiplier)
{
	return g_dialogue.SlideAmount(multiplier);
}

INT32 K_GetDialogueFade(void)
{
	return g_dialogue.FadeAmount();
}
