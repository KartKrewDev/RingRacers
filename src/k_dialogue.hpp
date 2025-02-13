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
/// \file  k_dialogue.hpp
/// \brief Basic text prompts

#ifndef __K_DIALOGUE_HPP__
#define __K_DIALOGUE_HPP__

#include <string>
#include <string_view>
#include <unordered_map>

#include "doomdef.h"
#include "doomtype.h"
#include "typedef.h"
#include "sounds.h"
#include "v_video.h"

namespace srb2
{

class Dialogue
{
public:
	static constexpr fixed_t kSlideSpeed = FRACUNIT / (TICRATE / 5);

	void SetSpeaker(void);
	void SetSpeaker(std::string skinName, int portraitID);
	void SetSpeaker(std::string name, patch_t *patch, UINT8 *colormap, sfxenum_t voice);

	void NewText(std::string_view newText);

	bool Active(void);
	bool TextDone(void);
	bool Dismissable(void);
	void SetDismissable(bool value);

	void Tick(void);
	void Draw(void);

	INT32 SlideAmount(fixed_t multiplier);

	void Dismiss(void);
	void Unset(void);

	UINT32 GetNewEra(void);
	bool EraIsValid(INT32 comparison);

	class Typewriter
	{
	public:
		static constexpr fixed_t kTextSpeedDefault = FRACUNIT;
		static constexpr fixed_t kTextPunctPause = (FRACUNIT * TICRATE * 2) / 5;

		std::string text;
		std::string textDest;

		fixed_t textTimer;
		fixed_t textSpeed;
		bool textDone;
		int textLines;

		sfxenum_t voiceSfx;
		bool syllable;

		void NewText(std::string newText);
		void ClearText(void);

		void WriteText(void);
		void CompleteText(void);
	};

private:
	Typewriter typewriter;

	INT32 current_era;

	patch_t *bgPatch;
	patch_t *confirmPatch;

	std::string speaker;
	patch_t *portrait;
	UINT8 *portraitColormap;

	std::unordered_map<std::string_view, patch_t*> patchCache;

	bool active;
	fixed_t slide;

	bool dismissable;
	bool freeze;

	void Init(void);
	//void Unset(void);

	bool Pressed(void);
	bool Held(void);

};

}; // namespace srb2

extern srb2::Dialogue g_dialogue;

#endif //__K_DIALOGUE_HPP__
