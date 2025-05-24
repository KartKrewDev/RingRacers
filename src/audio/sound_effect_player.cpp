// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "sound_effect_player.hpp"

#include <cmath>

using std::size_t;

using srb2::audio::Sample;
using srb2::audio::SoundEffectPlayer;

size_t SoundEffectPlayer::generate(tcb::span<Sample<2>> buffer)
{
	if (!chunk_)
		return 0;
	if (position_ >= chunk_->samples.size())
	{
		return 0;
	}

	size_t written = 0;
	for (; position_ < chunk_->samples.size() && written < buffer.size(); position_++)
	{
		float mono_sample = chunk_->samples[position_].amplitudes[0];

		float sep_pan = ((sep_ + 1.f) / 2.f) * (3.14159 / 2.f);

		float left_scale = std::cos(sep_pan);
		float right_scale = std::sin(sep_pan);
		buffer[written] = {mono_sample * volume_ * left_scale, mono_sample * volume_ * right_scale};
		written += 1;
	}
	return written;
}

void SoundEffectPlayer::start(const SoundChunk* chunk, float volume, float sep)
{
	this->update(volume, sep);
	position_ = 0;
	chunk_ = chunk;
}

void SoundEffectPlayer::update(float volume, float sep)
{
	volume_ = volume;
	sep_ = sep;
}

void SoundEffectPlayer::reset()
{
	position_ = 0;
	chunk_ = nullptr;
}

bool SoundEffectPlayer::finished() const
{
	if (!chunk_)
		return true;
	if (position_ >= chunk_->samples.size())
		return true;
	return false;
}

bool SoundEffectPlayer::is_playing_chunk(const SoundChunk* chunk) const
{
	return chunk_ == chunk;
}

SoundEffectPlayer::~SoundEffectPlayer() = default;
