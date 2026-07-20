// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_MEDIA_WEBM_HPP
#define SRB2_MEDIA_WEBM_HPP

#include <chrono>
#include <cstdint>
#include <ratio>

namespace srb2::media::webm
{

using track = uint64_t;
using timestamp = uint64_t;
using duration = std::chrono::duration<timestamp, std::nano>;

}; // namespace srb2::media::webm

#endif // SRB2_MEDIA_WEBM_HPP
