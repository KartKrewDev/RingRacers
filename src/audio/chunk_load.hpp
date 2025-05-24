// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_CHUNK_LOAD_HPP__
#define __SRB2_AUDIO_CHUNK_LOAD_HPP__

#include <cstddef>
#include <optional>

#include <tcb/span.hpp>

#include "sound_chunk.hpp"

namespace srb2::audio
{

/// @brief Try to load a chunk from the given byte span.
std::optional<SoundChunk> try_load_chunk(tcb::span<std::byte> data);

} // namespace srb2::audio

#endif // __SRB2_AUDIO_CHUNK_LOAD_HPP__
