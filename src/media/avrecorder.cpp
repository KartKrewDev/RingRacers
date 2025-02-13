// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <exception>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "../cxxutil.hpp"
#include "../i_time.h"
#include "../m_fixed.h"
#include "avrecorder_impl.hpp"
#include "webm_container.hpp"

using namespace srb2::media;

using Impl = AVRecorder::Impl;

namespace
{

constexpr auto kBufferMethod = VideoFrame::BufferMethod::kEncoderAllocatedRGBA8888;

}; // namespace

Impl::Impl(Config cfg) :
	max_size_(cfg.max_size),
	max_duration_(cfg.max_duration),

	container_(std::make_unique<WebmContainer>(MediaContainer::Config {
		cfg.file_name,
		[this](const MediaContainer& container) { container_dtor_handler(container); },
	})),

	audio_encoder_(make_audio_encoder(cfg)),
	video_encoder_(make_video_encoder(cfg)),

	epoch_(I_GetTime()),

	thread_([this] { worker(); })
{
}

std::unique_ptr<AudioEncoder> Impl::make_audio_encoder(const Config cfg) const
{
	if (!cfg.audio)
	{
		return nullptr;
	}

	const Config::Audio& a = *cfg.audio;

	return container_->make_audio_encoder({2, a.sample_rate});
}

std::unique_ptr<VideoEncoder> Impl::make_video_encoder(const Config cfg) const
{
	if (!cfg.video)
	{
		return nullptr;
	}

	const Config::Video& v = *cfg.video;

	return container_->make_video_encoder({v.width, v.height, v.frame_rate, kBufferMethod});
}

Impl::~Impl()
{
	valid_ = false;
	wake_up_worker();
	thread_.join();

	try
	{
		// Finally flush encoders, unless queues were finished
		// already due to time or size constraints.

		if (!audio_queue_.finished())
		{
			audio_encoder_->flush();
		}

		if (!video_queue_.finished())
		{
			video_encoder_->flush();
		}
	}
	catch (const std::exception& ex)
	{
		CONS_Alert(CONS_ERROR, "AVRecorder::Impl::~Impl: %s\n", ex.what());
		return;
	}
}

std::optional<int> Impl::advance_video_pts()
{
	auto _ = queue_guard();

	// Don't let this queue grow out of hand. It's normal
	// for encoding time to vary by a small margin and
	// spend longer than one frame rate on a single
	// frame. It should normalize though.

	if (video_queue_.vec_.size() >= 3)
	{
		return {};
	}

	SRB2_ASSERT(video_encoder_ != nullptr);

	const float tic_pts = video_encoder_->frame_rate() / static_cast<float>(TICRATE);
	const int pts = ((I_GetTime() - epoch_) + FixedToFloat(g_time.timefrac)) * tic_pts;

	if (!video_queue_.advance(pts, 1))
	{
		return {};
	}

	return pts;
}

void Impl::worker()
{
	for (;;)
	{
		QueueState qs;

		try
		{
			while ((qs = encode_queues()) == QueueState::kFlushed)
				;
		}
		catch (const std::exception& ex)
		{
			CONS_Alert(CONS_ERROR, "AVRecorder::Impl::worker: %s\n", ex.what());
			break;
		}

		if (qs != QueueState::kFinished && valid_)
		{
			std::unique_lock lock(queue_mutex_);

			queue_cond_.wait(lock);
		}
		else
		{
			break;
		}
	}

	// Breaking out of the loop ensures invalidation!
	valid_ = false;
}

const char* AVRecorder::file_extension()
{
	return "webm";
}

AVRecorder::AVRecorder(const Config config) : impl_(std::make_unique<Impl>(config))
{
}

AVRecorder::~AVRecorder()
{
	// impl_ is destroyed in a background thread so it doesn't
	// block the thread AVRecorder was destroyed in.
	//
	// TODO: Save into a thread pool instead of detaching so
	//       the thread could be joined at program exit and
	//       not possibly terminate before fully destroyed?

	std::thread([_ = std::move(impl_)] {}).detach();
}

const char* AVRecorder::format_name() const
{
	return impl_->container_->name();
}

void AVRecorder::push_audio_samples(audio_buffer_t buffer)
{
	const auto _ = impl_->queue_guard();

	auto& q = impl_->audio_queue_;

	if (!q.advance(q.pts(), buffer.size()))
	{
		return;
	}

	using T = const float;
	tcb::span<T> p(reinterpret_cast<T*>(buffer.data()), buffer.size() * 2); // 2 channels

	std::copy(p.begin(), p.end(), std::back_inserter(q.vec_));

	impl_->wake_up_worker();
}

bool AVRecorder::invalid() const
{
	return !impl_->valid_;
}
