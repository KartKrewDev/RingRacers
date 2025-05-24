// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <chrono>
#include <cstddef>
#include <mutex>
#include <utility>

#include "avrecorder_impl.hpp"

using namespace srb2::media;

using Impl = AVRecorder::Impl;

template <typename T>
bool Impl::Queue<T>::advance(int new_pts, int duration)
{
	if (!impl_->valid_ || finished())
	{
		return false;
	}

	new_pts += duration;

	// PTS must only advance.
	if (new_pts <= pts())
	{
		return false;
	}

	auto finish = [this]
	{
		finished_ = true;

		const auto t = impl_->container_->duration();

		// Tracks are ultimately cut to the shortest among
		// them, therefore it would be pointless for another
		// queue to continue beyond this point.
		//
		// This is relevant if finishing due to size
		// constraint; in that case, another queue might be
		// far behind this one in terms of size and would
		// continue in vain.

		if (!impl_->max_duration_ || t < impl_->max_duration_)
		{
			impl_->max_duration_ = t;
		}

		impl_->wake_up_worker();
	};

	if (impl_->max_duration_)
	{
		const int final_pts = *impl_->max_duration_ / time_scale();

		if (new_pts > final_pts)
		{
			return finish(), false;
		}
	}

	if (impl_->max_size_)
	{
		constexpr float kError = 0.99f; // 1% muxing overhead

		const MediaEncoder::BitRate est = encoder_->estimated_bit_rate();

		const float br = est.bits / 8.f;

		// count size of already queued frames too
		const float t = ((duration + queued_frames_) * time_scale()) / est.period;

		if ((impl_->container_->size() + (t * br)) > (*impl_->max_size_ * kError))
		{
			return finish(), false;
		}
	}

	pts_ = new_pts;
	queued_frames_ += duration;

	return true;
}

Impl::QueueState Impl::encode_queues()
{
	bool remain = false;
	bool flushed = false;

	auto check = [&, this](auto& q, auto encode)
	{
		std::unique_lock lock(queue_mutex_);

		if (!q.finished())
		{
			remain = true;
		}

		if (!q.vec_.empty())
		{
			const std::size_t n = q.queued_frames_;

			auto copy = std::move(q.vec_);

			lock.unlock();
			encode(std::move(copy));
			lock.lock();

			q.queued_frames_ -= n;

			flushed = true;
		}
	};

	auto encode_audio = [this](auto copy) { audio_encoder_->encode(copy); };
	auto encode_video = [this](auto copy)
	{
		for (auto& p : copy)
		{
			auto frame = convert_staging_video_frame(*p);

			video_encoder_->encode(std::move(frame));
		}

		update_video_frame_rate_avg();
	};

	check(audio_queue_, encode_audio);
	check(video_queue_, encode_video);

	if (flushed)
	{
		return QueueState::kFlushed;
	}
	else if (remain)
	{
		return QueueState::kEmpty;
	}
	else
	{
		return QueueState::kFinished;
	}
}

void Impl::update_video_frame_rate_avg()
{
	constexpr auto period = std::chrono::duration<float>(1.f);

	auto& ref = video_frame_count_reference_;
	const auto count = video_encoder_->frame_count();
	const auto t = (count.duration - ref.duration);

	if (t >= period)
	{
		video_frame_rate_avg_ = (count.frames - ref.frames) * (period / t);
		ref = count;
	}
}

template class Impl::Queue<AudioEncoder>;
template class Impl::Queue<VideoEncoder>;
