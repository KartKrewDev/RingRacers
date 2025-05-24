// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_WAV_PLAYER_HPP__
#define __SRB2_AUDIO_WAV_PLAYER_HPP__

#include <cstddef>

#include <tcb/span.hpp>

#include "source.hpp"
#include "wav.hpp"

namespace srb2::audio
{

class WavPlayer final : public Source<1>
{
	Wav wav_;
	std::size_t position_;
	bool looping_;

public:
	WavPlayer();
	WavPlayer(const WavPlayer& rhs);
	WavPlayer(WavPlayer&& rhs) noexcept;

	WavPlayer& operator=(const WavPlayer& rhs);
	WavPlayer& operator=(WavPlayer&& rhs) noexcept;

	WavPlayer(Wav&& wav) noexcept;

	virtual std::size_t generate(tcb::span<Sample<1>> buffer) override;

	bool looping() const { return looping_; }
	void looping(bool looping) { looping_ = looping; }

	std::size_t sample_rate() const { return wav_.sample_rate(); }
	float duration_seconds() const { return wav_.length() / static_cast<float>(wav_.sample_rate()); }
	void seek(float seconds) { position_ = seconds * wav_.sample_rate(); }
};

} // namespace srb2::audio

#endif // __SRB2_AUDIO_WAV_PLAYER_HPP__
