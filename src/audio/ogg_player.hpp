// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_OGG_SOURCE_HPP__
#define __SRB2_AUDIO_OGG_SOURCE_HPP__

#include <cstddef>
#include <optional>

#include <stb_vorbis.h>
#include <tcb/span.hpp>

#include "ogg.hpp"
#include "source.hpp"

namespace srb2::audio
{

template <size_t C>
class OggPlayer final : public Source<C>
{
	bool playing_;
	bool looping_;
	std::optional<std::size_t> loop_point_;
	Ogg ogg_;

public:
	OggPlayer(Ogg&& ogg) noexcept;

	OggPlayer(const OggPlayer&) = delete;
	OggPlayer(OggPlayer&& rhs) noexcept;

	OggPlayer& operator=(const OggPlayer&) = delete;
	OggPlayer& operator=(OggPlayer&& rhs) noexcept;

	virtual std::size_t generate(tcb::span<Sample<C>> buffer) override final;

	bool looping() const { return looping_; }

	void looping(bool looping) { looping_ = looping; }

	bool playing() const { return playing_; }
	void playing(bool playing) { playing_ = playing; }
	void seek(float position_seconds);
	void loop_point_seconds(float loop_point);

	void reset();
	std::size_t sample_rate() const;

	float duration_seconds() const;
	std::optional<float> loop_point_seconds() const;
	float position_seconds() const;

	~OggPlayer();
};

extern template class OggPlayer<1>;
extern template class OggPlayer<2>;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_OGG_SOURCE_HPP__
