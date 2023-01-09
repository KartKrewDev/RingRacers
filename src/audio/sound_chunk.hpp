// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_SOUND_CHUNK_HPP__
#define __SRB2_AUDIO_SOUND_CHUNK_HPP__

#include <vector>

#include "source.hpp"

namespace srb2::audio
{

struct SoundChunk
{
	std::vector<Sample<1>> samples;
};

} // namespace srb2::audio

#endif // __SRB2_AUDIO_SOUND_CHUNK_HPP__
