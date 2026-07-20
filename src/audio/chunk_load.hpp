// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_AUDIO_CHUNK_LOAD_HPP
#define SRB2_AUDIO_CHUNK_LOAD_HPP

#include <cstddef>
#include <optional>
#include <span>

#include "sound_chunk.hpp"

namespace srb2::audio
{

/// @brief Try to load a chunk from the given byte span.
std::optional<SoundChunk> try_load_chunk(std::span<std::byte> data);

} // namespace srb2::audio

#endif // SRB2_AUDIO_CHUNK_LOAD_HPP

