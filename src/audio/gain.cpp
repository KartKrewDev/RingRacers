// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "gain.hpp"

#include <algorithm>

using std::size_t;

using srb2::audio::Filter;
using srb2::audio::Gain;
using srb2::audio::Sample;

constexpr const float kGainInterpolationAlpha = 0.8f;

template <size_t C>
size_t Gain<C>::filter(tcb::span<Sample<C>> input_buffer, tcb::span<Sample<C>> buffer)
{
	size_t written = std::min(buffer.size(), input_buffer.size());
	for (size_t i = 0; i < written; i++)
	{
		buffer[i] = input_buffer[i];
		buffer[i] *= gain_;
		gain_ += (new_gain_ - gain_) * kGainInterpolationAlpha;
	}

	return written;
}

template <size_t C>
void Gain<C>::gain(float new_gain)
{
	new_gain_ = std::max(new_gain, 0.f);
}

template <size_t C>
Gain<C>::~Gain() = default;

template class srb2::audio::Gain<1>;
template class srb2::audio::Gain<2>;
