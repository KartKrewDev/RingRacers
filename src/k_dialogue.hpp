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
/// \file  k_dialogue.hpp
/// \brief Basic text prompts

#ifndef __K_DIALOGUE_HPP__
#define __K_DIALOGUE_HPP__

#include <string>

#include "doomdef.h"
#include "doomtype.h"
#include "typedef.h"
#include "v_video.h"

namespace srb2
{

class Dialogue
{
private:
	patch_t *bgPatch;
	patch_t *confirmPatch;

	bool active;

	std::string speaker;
	patch_t *portrait;
	UINT8 *portraitColormap;

	std::string text;
	std::string textDest;

	fixed_t textTimer;
	fixed_t textSpeed;
	bool textDone;

	bool freeze;
	std::string script;

	void UpdatePatches(void);

	void WriteText(void);
	void CompleteText(void);

public:
	static constexpr fixed_t kTextSpeedDefault = FRACUNIT;
	static constexpr fixed_t kTextPunctPause = FRACUNIT * TICRATE * 3 / 5;

	void SetSpeaker(void);
	void SetSpeaker(std::string skinName, int portraitID);
	void SetSpeaker(std::string customName, std::string customPatch, UINT8 *customColormap);

	void NewText(std::string newText);

	bool Active(void);
	bool TextDone(void);

	void Tick(void);
	void Draw(void);

	void Dismiss(void);
};

}; // namespace srb2

extern srb2::Dialogue g_dialogue;

#endif //__K_DIALOGUE_HPP__
