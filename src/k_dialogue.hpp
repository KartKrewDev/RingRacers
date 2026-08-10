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

#ifndef K_DIALOGUE_HPP
#define K_DIALOGUE_HPP

#include <string_view>

#include "core/hash_map.hpp"
#include "core/string.h"
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
	void SetSpeaker(srb2::String skinName, int portraitID);
	void SetSpeaker(srb2::String name, patch_t *patch, uint8_t *colormap, sfxenum_t voice);

	void NewText(std::string_view newText);

	bool Active(void);
	bool TextDone(void);
	bool Dismissable(void);
	void SetDismissable(bool value);

	void Tick(void);
	void Draw(void);

	int32_t SlideAmount(fixed_t multiplier);
	int32_t FadeAmount(void);

	void Dismiss(void);
	void Unset(void);

	uint32_t GetNewEra(void);
	bool EraIsValid(int32_t comparison);

	class Typewriter
	{
	public:
		static constexpr fixed_t kTextSpeedDefault = FRACUNIT;
		static constexpr fixed_t kTextPunctPause = (FRACUNIT * TICRATE * 2) / 5;

		srb2::String text;
		srb2::String textDest;

		fixed_t textTimer;
		fixed_t textSpeed;
		bool textDone;
		int textLines;

		sfxenum_t voiceSfx;
		bool syllable;

		void NewText(const srb2::String& newText);
		void ClearText(void);

		void WriteText(void);
		void CompleteText(void);
	};

private:
	Typewriter typewriter;

	int32_t current_era;

	patch_t *bgPatch;
	patch_t *confirmPatch;

	srb2::String speaker;
	patch_t *portrait;
	uint8_t *portraitColormap;

	srb2::HashMap<std::string_view, patch_t*> patchCache;

	bool active;
	fixed_t slide;
	int32_t fade;

	bool dismissable;
	bool freeze;

	void Init(void);
	//void Unset(void);

	bool Pressed(void);
	bool Held(void);

};

}; // namespace srb2

extern srb2::Dialogue g_dialogue;

#endif //K_DIALOGUE_HPP
