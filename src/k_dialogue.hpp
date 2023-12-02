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

	void NewText(std::string newText);

	bool Active(void);
	bool TextDone(void);
	bool Dismissable(void);
	void SetDismissable(bool value);

	void Tick(void);
	void Draw(void);

	INT32 SlideAmount(fixed_t multiplier);

	void Dismiss(void);
	void Unset(void);

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

		sfxenum_t voiceSfx;
		bool syllable;

		void NewText(std::string newText);
		void ClearText(void);

		void WriteText(void);
		void CompleteText(void);
	};

private:
	Typewriter typewriter;

	patch_t *bgPatch;
	patch_t *confirmPatch;

	std::string speaker;
	patch_t *portrait;
	UINT8 *portraitColormap;

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
