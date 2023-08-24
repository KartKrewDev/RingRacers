// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sonic Team Junior
// Copyright (C) by Kart Krew
// Copyright (C) by Sally "TehRealSalt" Cochenour
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_dialogue.cpp
/// \brief Basic text prompts

#include "k_dialogue.hpp"
#include "k_dialogue.h"

#include <string>
#include <algorithm>

#include "info.h"
#include "g_game.h"
#include "v_video.h"
#include "r_draw.h"
#include "r_skins.h"
#include "z_zone.h"

#include "acs/interface.h"

using srb2::Dialogue;

void Dialogue::SetSpeaker(void)
{
	// Unset speaker
	speaker.clear();
	portrait = nullptr;
	portraitColormap = nullptr;
}

void Dialogue::SetSpeaker(std::string skinName, int portraitID)
{
	// Set speaker based on a skin
	int skinID = -1;

	if (!skinName.empty())
	{
		skinID = R_SkinAvailable(skinName.c_str());
	}

	if (skinID >= 0 && skinID < numskins)
	{
		const skin_t *skin = &skins[skinID];
		const spritedef_t *sprdef = &skin->sprites[SPR2_TALK];

		if (sprdef->numframes > 0)
		{
			portraitID %= sprdef->numframes;
		}

		const spriteframe_t *sprframe = &sprdef->spriteframes[portraitID];

		speaker = skin->realname;
		portrait = static_cast<patch_t *>( W_CachePatchNum(sprframe->lumppat[0], PU_CACHE) );
		portraitColormap = R_GetTranslationColormap(skinID, static_cast<skincolornum_t>(skin->prefcolor), GTC_CACHE);
	}
	else
	{
		SetSpeaker();
	}
}

void Dialogue::SetSpeaker(std::string customName, std::string customPatch, UINT8 *customColormap)
{
	// Set custom speaker
	speaker = customName;

	if (speaker.empty())
	{
		portrait = nullptr;
		portraitColormap = nullptr;
		return;
	}

	portrait = static_cast<patch_t *>( W_CachePatchName(customPatch.c_str(), PU_CACHE) );
	portraitColormap = customColormap;
}

void Dialogue::NewText(std::string newText)
{
	text.clear();

	textDest = newText;
	std::reverse(textDest.begin(), textDest.end());

	textTimer = kTextPunctPause;
	textSpeed = kTextSpeedDefault;
	textDone = false;
}

bool Dialogue::Active(void)
{
	return (!speaker.empty());
}

bool Dialogue::TextDone(void)
{
	return textDone;
}

void Dialogue::WriteText(void)
{
	textTimer -= textSpeed;

	while (textTimer <= 0 && !textDest.empty())
	{
		char c = textDest.back();
		text.push_back(c);

		if (c == '.' || c == '!' || c == '?')
		{
			// slow down for punctuation
			textTimer += kTextPunctPause;
		}
		else
		{
			textTimer += FRACUNIT;
		}

		textDest.pop_back();
	}

	textDone = (textTimer <= 0 && textDest.empty());
}

void Dialogue::CompleteText(void)
{
	while (!textDest.empty())
	{
		text.push_back( textDest.back() );
		textDest.pop_back();
	}

	textTimer = 0;
	textDone = true;
}

void Dialogue::Tick(void)
{
	if (Active() == false)
	{
		return;
	}

	WriteText();

	bool pressed = (
		((players[serverplayer].cmd.buttons & BT_VOTE) == BT_VOTE) &&
		((players[serverplayer].oldcmd.buttons & BT_VOTE) == 0)
	);

	if (pressed == true)
	{
		if (textDone)
		{
			SetSpeaker();
		}
		else
		{
			CompleteText();
		}
	}
}

void Dialogue::UpdatePatches(void)
{
	if (bgPatch == nullptr)
	{
		bgPatch = static_cast<patch_t *>(W_CachePatchName("TUTDIAG1", PU_HUDGFX) );
	}

	if (confirmPatch == nullptr)
	{
		confirmPatch = static_cast<patch_t *>(W_CachePatchName("TUTDIAG2", PU_HUDGFX) );
	}
}

void Dialogue::Draw(void)
{
	if (Active() == false)
	{
		return;
	}

	UpdatePatches();

	V_DrawFixedPatch(
		0, 0,
		FRACUNIT,
		V_SNAPTOTOP,
		bgPatch,
		nullptr
	);

	if (portrait != nullptr)
	{
		V_DrawFixedPatch(
			10 * FRACUNIT, 41 * FRACUNIT,
			FRACUNIT,
			V_SNAPTOTOP,
			portrait,
			portraitColormap
		);
	}

	V_DrawString(
		45, 39,
		V_SNAPTOTOP,
		speaker.c_str()
	);

	V_DrawString(
		10, 3,
		V_SNAPTOTOP,
		text.c_str()
	);

	if (textDone)
	{
		V_DrawFixedPatch(
			304 * FRACUNIT, 7 * FRACUNIT,
			FRACUNIT,
			V_SNAPTOTOP,
			confirmPatch,
			nullptr
		);
	}
}

void Dialogue::Dismiss(void)
{
	if (Active() == false)
	{
		return;
	}

	SetSpeaker();

	text.clear();
	textDest.clear();

	if (G_GamestateUsesLevel() == true && !script.empty())
	{
		ACS_Execute(script.c_str(), nullptr, 0, nullptr);
	}

	script.clear();
}

/*
	Ideally, the Dialogue class would be on player_t instead of in global space
	for full multiplayer compatibility, but right now it's only being used for
	the tutorial, and I don't feel like writing network code. If you feel like
	doing that, then you can remove g_dialogue entirely.
*/

Dialogue g_dialogue;

void K_DismissDialogue(void)
{
	g_dialogue.Dismiss();
}

void K_DrawDialogue(void)
{
	g_dialogue.Draw();
}

void K_TickDialogue(void)
{
	g_dialogue.Tick();
}
