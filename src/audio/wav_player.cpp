// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "wav_player.hpp"

using namespace srb2;

using srb2::audio::WavPlayer;

WavPlayer::WavPlayer() : WavPlayer(audio::Wav {})
{
}

WavPlayer::WavPlayer(const WavPlayer& rhs) = default;

WavPlayer::WavPlayer(WavPlayer&& rhs) noexcept = default;

WavPlayer& WavPlayer::operator=(const WavPlayer& rhs) = default;

WavPlayer& WavPlayer::operator=(WavPlayer&& rhs) noexcept = default;

WavPlayer::WavPlayer(audio::Wav&& wav) noexcept : wav_(std::forward<Wav>(wav)), position_(0), looping_(false)
{
}

std::size_t WavPlayer::generate(tcb::span<audio::Sample<1>> buffer)
{
	std::size_t samples_read = 0;
	while (samples_read < buffer.size())
	{
		const std::size_t read_this_time = wav_.get_samples(position_, buffer.subspan(samples_read));
		position_ += read_this_time;
		samples_read += read_this_time;

		if (position_ > wav_.length() && looping_)
		{
			position_ = 0;
		}
		if (read_this_time == 0 && !looping_)
		{
			break;
		}
	}
	return samples_read;
}
