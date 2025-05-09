// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_AUDIO_ENCODER_HPP__
#define __SRB2_MEDIA_AUDIO_ENCODER_HPP__

#include <tcb/span.hpp>

#include "encoder.hpp"

namespace srb2::media
{

class AudioEncoder : virtual public MediaEncoder
{
public:
	using sample_buffer_t = tcb::span<const float>;

	struct Config
	{
		int channels;
		int sample_rate;
	};

	virtual void encode(sample_buffer_t samples) = 0;

	virtual int channels() const = 0;
	virtual int sample_rate() const = 0;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_AUDIO_ENCODER_HPP__
