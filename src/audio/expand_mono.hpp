// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_EXPAND_MONO_HPP__
#define __SRB2_AUDIO_EXPAND_MONO_HPP__

#include <tcb/span.hpp>

#include "filter.hpp"

namespace srb2::audio {

class ExpandMono : public Filter<1, 2> {
public:
	virtual ~ExpandMono();
	virtual std::size_t filter(tcb::span<Sample<1>> input_buffer, tcb::span<Sample<2>> buffer) override final;
};

} // namespace srb2::audio

#endif // __SRB2_AUDIO_EXPAND_MONO_HPP__
