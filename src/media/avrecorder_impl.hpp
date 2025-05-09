// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_AVRECORDER_IMPL_HPP__
#define __SRB2_MEDIA_AVRECORDER_IMPL_HPP__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "../core/string.h"
#include "../i_time.h"
#include "avrecorder.hpp"
#include "container.hpp"

namespace srb2::media
{

class AVRecorder::Impl
{
public:
	template <typename T>
	class Queue
	{
	public:
		// The use of typename = void is a GCC bug.
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
		// Explicit specialization inside of a class still
		// does not work as of 12.2.1.

		template <typename, typename = void>
		struct Traits
		{
		};

		template <typename _>
		struct Traits<AudioEncoder, _>
		{
			using frame_type = float;
		};

		template <typename _>
		struct Traits<VideoEncoder, _>
		{
			using frame_type = StagingVideoFrame::instance_t;
		};

		srb2::Vector<typename Traits<T>::frame_type> vec_;

		// This number only decrements once a frame has
		// actually been written to container.
		std::size_t queued_frames_ = 0;

		Queue(const std::unique_ptr<T>& encoder, Impl& impl) : encoder_(encoder.get()), impl_(&impl) {}

		// This method handles validation of the queue,
		// finishing the queue and advancing PTS. Returns true
		// if PTS was advanced.
		bool advance(int pts, int duration);

		// True if no more data may be queued.
		bool finished() const { return finished_; }

		// Presentation Time Stamp; one frame for video
		// encoders, one sample for audio encoders.
		int pts() const { return pts_; }

	private:
		using time_unit_t = std::chrono::duration<float>;

		T* const encoder_;
		Impl* const impl_;

		bool finished_ = (encoder_ == nullptr);
		int pts_ = -1; // valid pts starts at 0

		// Actual duration of PTS unit.
		time_unit_t time_scale() const;
	};

	const std::optional<std::size_t> max_size_;
	std::optional<std::chrono::duration<float>> max_duration_;

	// max_duration_ may be readjusted in case a queue
	// finishes early for any reason. max_duration_config_ is
	// the original, unmodified value.
	const decltype(max_duration_) max_duration_config_ = max_duration_;

	std::unique_ptr<MediaContainer> container_;
	std::unique_ptr<AudioEncoder> audio_encoder_;
	std::unique_ptr<VideoEncoder> video_encoder_;

	Queue<AudioEncoder> audio_queue_ {audio_encoder_, *this};
	Queue<VideoEncoder> video_queue_ {video_encoder_, *this};

	// This class becomes invalid if:
	//
	// 1) an exception occurred
	// 2) the object has begun destructing
	std::atomic<bool> valid_ = true;

	// Average number of frames actually encoded per second.
	std::atomic<float> video_frame_rate_avg_ = 0.f;

	Impl(Config config);
	~Impl();

	// Returns valid PTS if enough time has passed.
	std::optional<int> advance_video_pts();

	// Use before accessing audio_queue_ or video_queue_.
	auto queue_guard() { return std::lock_guard(queue_mutex_); }

	// Use to notify worker thread if queues were modified.
	void wake_up_worker() { queue_cond_.notify_one(); }

private:
	enum class QueueState
	{
		kEmpty,	   // all queues are empty
		kFlushed,  // a queue was flushed but more data may be waiting
		kFinished, // all queues are finished -- no more data may be queued
	};

	const tic_t epoch_;

	VideoEncoder::FrameCount video_frame_count_reference_ = {};

	std::thread thread_;
	mutable std::recursive_mutex queue_mutex_; // guards audio and video queues
	std::condition_variable_any queue_cond_;

	std::unique_ptr<AudioEncoder> make_audio_encoder(const Config cfg) const;
	std::unique_ptr<VideoEncoder> make_video_encoder(const Config cfg) const;

	QueueState encode_queues();
	void update_video_frame_rate_avg();

	void worker();

	void container_dtor_handler(const MediaContainer& container) const;

	VideoFrame::instance_t convert_staging_video_frame(const StagingVideoFrame& indexed);
};

template <>
inline AVRecorder::Impl::Queue<AudioEncoder>::time_unit_t AVRecorder::Impl::Queue<AudioEncoder>::time_scale() const
{
	return time_unit_t(1.f / encoder_->sample_rate());
}

template <>
inline AVRecorder::Impl::Queue<VideoEncoder>::time_unit_t AVRecorder::Impl::Queue<VideoEncoder>::time_scale() const
{
	return time_unit_t(1.f / encoder_->frame_rate());
}

extern template class AVRecorder::Impl::Queue<AudioEncoder>;
extern template class AVRecorder::Impl::Queue<VideoEncoder>;

}; // namespace srb2::media

#endif // __SRB2_MEDIA_AVRECORDER_IMPL_HPP__
