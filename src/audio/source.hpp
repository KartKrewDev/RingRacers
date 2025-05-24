// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_SOURCE_HPP__
#define __SRB2_AUDIO_SOURCE_HPP__

#include <tcb/span.hpp>

#include "sample.hpp"

namespace srb2::audio
{

template <size_t C>
class Source
{
public:
	virtual std::size_t generate(tcb::span<Sample<C>> buffer) = 0;

	virtual ~Source() = default;
};

// This audio DSP is Stereo, FP32 system-endian, 44100 Hz internally.
// Conversions to other formats should be handled elsewhere.

constexpr const std::size_t kSampleRate = 44100;

} // namespace srb2::audio

#endif // __SRB2_AUDIO_SOURCE_HPP__
