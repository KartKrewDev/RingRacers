// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_WEBM_VORBIS_HPP__
#define __SRB2_MEDIA_WEBM_VORBIS_HPP__

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#include "../cxxutil.hpp"
#include "vorbis.hpp"
#include "webm_encoder.hpp"

namespace srb2::media
{

class WebmVorbisEncoder : public WebmEncoder<mkvmuxer::AudioTrack>, public VorbisEncoder
{
public:
	WebmVorbisEncoder(WebmContainer& container, webm::track trackid, AudioEncoder::Config cfg) :
		WebmEncoder(container, trackid), VorbisEncoder(cfg)
	{
		// write Vorbis extra data

		const auto p = make_vorbis_private_data();

		if (!track()->SetCodecPrivate(reinterpret_cast<const uint8_t*>(p.data()), p.size()))
		{
			throw std::runtime_error(fmt::format("mkvmuxer::AudioTrack::SetCodecPrivate, size={}", p.size()));
		}
	}

	virtual BitRate estimated_bit_rate() const override final
	{
		auto _ = container_.queue_guard();

		const std::chrono::duration<float> t = duration();

		if (t <= t.zero())
		{
			return {};
		}

		using namespace std::chrono_literals;
		return {static_cast<std::size_t>((size() * 8) / t.count()), 1s};
	}

private:
	std::vector<std::byte> make_vorbis_private_data();
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_WEBM_VORBIS_HPP__
