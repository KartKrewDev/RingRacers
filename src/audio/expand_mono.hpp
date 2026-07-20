// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_AUDIO_EXPAND_MONO_HPP
#define SRB2_AUDIO_EXPAND_MONO_HPP

#include <span>

#include "filter.hpp"

namespace srb2::audio
{

class ExpandMono : public Filter<1, 2>
{
public:
	virtual ~ExpandMono();
	virtual std::size_t filter(std::span<Sample<1>> input_buffer, std::span<Sample<2>> buffer) override final;
};

} // namespace srb2::audio

#endif // SRB2_AUDIO_EXPAND_MONO_HPP
