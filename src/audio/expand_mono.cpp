// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "expand_mono.hpp"

#include <algorithm>

using std::size_t;

using namespace srb2::audio;

ExpandMono::~ExpandMono() = default;

size_t ExpandMono::filter(tcb::span<Sample<1>> input_buffer, tcb::span<Sample<2>> buffer)
{
	for (size_t i = 0; i < std::min(input_buffer.size(), buffer.size()); i++)
	{
		buffer[i].amplitudes[0] = input_buffer[i].amplitudes[0];
		buffer[i].amplitudes[1] = input_buffer[i].amplitudes[0];
	}
	return std::min(input_buffer.size(), buffer.size());
}
