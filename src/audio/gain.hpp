// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_GAIN_HPP__
#define __SRB2_AUDIO_GAIN_HPP__

#include <span>

#include "filter.hpp"

namespace srb2::audio
{

template <size_t C>
class Gain : public Filter<C, C>
{
public:
	virtual std::size_t filter(std::span<Sample<C>> input_buffer, std::span<Sample<C>> buffer) override final;
	void gain(float new_gain);

	virtual ~Gain();

private:
	float new_gain_ {1.f};
	float gain_ {1.f};
};
} // namespace srb2::audio

#endif // __SRB2_AUDIO_GAIN_HPP__
