// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_MEDIA_ENCODER_HPP
#define SRB2_MEDIA_ENCODER_HPP

#include <chrono>
#include <cstddef>
#include <span>

namespace srb2::media
{

class MediaEncoder
{
public:
	using time_unit_t = std::chrono::duration<float>;

	struct BitRate
	{
		std::size_t bits; // 8 bits = 1 byte :)
		time_unit_t period;
	};

	virtual ~MediaEncoder() = default;

	// Should be called finally but it's optional.
	virtual void flush() = 0;

	virtual const char* name() const = 0;

	// Returns an average bit rate over a constant period of
	// time, assuming no frames drops.
	virtual BitRate estimated_bit_rate() const = 0;

protected:
	using frame_buffer_t = std::span<const std::byte>;

	virtual void write_frame(frame_buffer_t frame, time_unit_t timestamp, bool is_key_frame) = 0;
};

}; // namespace srb2::media

#endif // SRB2_MEDIA_ENCODER_HPP

