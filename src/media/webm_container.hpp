// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_WEBM_CONTAINER_HPP__
#define __SRB2_MEDIA_WEBM_CONTAINER_HPP__

#include <cstddef>
#include <mutex>
#include <vector>

#include <mkvmuxer/mkvmuxer.h>

#include "../core/hash_map.hpp"
#include "container.hpp"
#include "webm.hpp"
#include "webm_writer.hpp"

namespace srb2::media
{

class WebmContainer : virtual public MediaContainer
{
public:
	WebmContainer(Config cfg);
	~WebmContainer();

	virtual std::unique_ptr<AudioEncoder> make_audio_encoder(AudioEncoder::Config config) override final;
	virtual std::unique_ptr<VideoEncoder> make_video_encoder(VideoEncoder::Config config) override final;

	virtual const char* name() const override final { return "WebM"; }
	virtual const char* file_name() const override final { return writer_.name(); }

	virtual time_unit_t duration() const override final;
	virtual std::size_t size() const override final;

	std::size_t track_size(webm::track trackid) const;
	time_unit_t track_duration(webm::track trackid) const;

	template <typename T = mkvmuxer::Track>
	T* get_track(webm::track trackid) const
	{
		return reinterpret_cast<T*>(segment_.GetTrackByNumber(trackid));
	}

	void init_queue(webm::track trackid) { queue_.try_emplace(trackid); }

	// init_queue MUST be called before using this function.
	void queue_frame(
		tcb::span<const std::byte> buffer,
		webm::track trackid,
		webm::timestamp timestamp,
		bool is_key_frame
	);

	auto queue_guard() const { return std::lock_guard(queue_mutex_); }

private:
	struct FrameQueue
	{
		struct Frame
		{
			std::vector<std::byte> buffer;
			webm::timestamp timestamp;
			bool is_key_frame;

			Frame(tcb::span<const std::byte> buffer_, webm::timestamp timestamp_, bool is_key_frame_) :
				buffer(buffer_.begin(), buffer_.end()), timestamp(timestamp_), is_key_frame(is_key_frame_)
			{
			}
		};

		std::vector<Frame> frames;
		std::size_t data_size = 0;

		webm::timestamp flushed_timestamp = 0;
		webm::timestamp queued_timestamp = 0;
	};

	mkvmuxer::Segment segment_;
	WebmWriter writer_;

	mutable std::recursive_mutex queue_mutex_;

	srb2::HashMap<webm::track, FrameQueue> queue_;

	webm::timestamp latest_timestamp_ = 0;
	std::size_t queue_size_ = 0;

	bool finalized_ = false;
	const dtor_cb_t dtor_cb_;

	void write_frame(
		tcb::span<const std::byte> buffer,
		webm::track trackid,
		webm::timestamp timestamp,
		bool is_key_frame
	);

	// Returns the largest timestamp that can be written.
	webm::timestamp flush_queue();
	webm::timestamp flush_single_queue(webm::track trackid, webm::timestamp flushed_timestamp);
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_WEBM_CONTAINER_HPP__
