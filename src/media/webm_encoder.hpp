// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_WEBM_ENCODER_HPP__
#define __SRB2_MEDIA_WEBM_ENCODER_HPP__

#include <mkvmuxer/mkvmuxer.h>

#include "encoder.hpp"
#include "webm_container.hpp"

namespace srb2::media
{

template <typename T = mkvmuxer::Track>
class WebmEncoder : virtual public MediaEncoder
{
public:
	WebmEncoder(WebmContainer& container, webm::track trackid) : container_(container), trackid_(trackid)
	{
		container_.init_queue(trackid_);
	}

protected:
	WebmContainer& container_;
	webm::track trackid_;

	std::size_t size() const { return container_.track_size(trackid_); }
	time_unit_t duration() const { return container_.track_duration(trackid_); }

	static T* get_track(const WebmContainer& container, webm::track trackid) { return container.get_track<T>(trackid); }

	T* track() const { return get_track(container_, trackid_); }

	virtual void write_frame(frame_buffer_t p, time_unit_t ts, bool is_key_frame) override final
	{
		const auto ts_nano = std::chrono::duration_cast<webm::duration>(ts);

		container_.queue_frame(p, trackid_, ts_nano.count(), is_key_frame);
	}
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_WEBM_ENCODER_HPP__
