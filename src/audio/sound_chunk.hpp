// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_AUDIO_SOUND_CHUNK_HPP
#define SRB2_AUDIO_SOUND_CHUNK_HPP

#include <vector>

#include "sample.hpp"

namespace srb2::audio
{

struct SoundChunk
{
	std::vector<Sample<1>> samples;
};

} // namespace srb2::audio

#endif // SRB2_AUDIO_SOUND_CHUNK_HPP
