// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_WEBM_VP8_HPP__
#define __SRB2_MEDIA_WEBM_VP8_HPP__

#include "vp8.hpp"
#include "webm_encoder.hpp"

namespace srb2::media
{

class WebmVP8Encoder : public WebmEncoder<mkvmuxer::VideoTrack>, public VP8Encoder
{
public:
	WebmVP8Encoder(WebmContainer& container, webm::track trackid, VideoEncoder::Config cfg) :
		WebmEncoder(container, trackid), VP8Encoder(cfg)
	{
	}

	virtual BitRate estimated_bit_rate() const override final
	{
		auto _ = container_.queue_guard();

		const int frames = frame_count().frames;

		if (frames <= 0)
		{
			return {};
		}

		return {(size() * 8) / frames, std::chrono::duration<float>(1.f / frame_rate())};
	}
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_WEBM_VP8_HPP__
