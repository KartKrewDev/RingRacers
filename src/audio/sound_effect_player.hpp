// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_SOUND_EFFECT_PLAYER_HPP__
#define __SRB2_AUDIO_SOUND_EFFECT_PLAYER_HPP__

#include <cstddef>

#include <tcb/span.hpp>

#include "sound_chunk.hpp"
#include "source.hpp"

namespace srb2::audio
{

class SoundEffectPlayer final : public Source<2>
{
public:
	virtual std::size_t generate(tcb::span<Sample<2>> buffer) override final;

	virtual ~SoundEffectPlayer() final;

	void start(const SoundChunk* chunk, float volume, float sep);
	void update(float volume, float sep);
	void reset();
	bool finished() const;

	bool is_playing_chunk(const SoundChunk* chunk) const;

private:
	float volume_;
	float sep_;

	std::size_t position_;

	const SoundChunk* chunk_;
};

} // namespace srb2::audio

#endif // __SRB2_AUDIO_SOUND_EFFECT_PLAYER_HPP__
