// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

// TODO: remove this file once hwr2 twodee is finished

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "../cxxutil.hpp"
#include "avrecorder_impl.hpp"

using namespace srb2::media;

using Impl = AVRecorder::Impl;

VideoFrame::instance_t Impl::convert_staging_video_frame(const StagingVideoFrame& staging)
{
	VideoFrame::instance_t frame = video_encoder_->new_frame(staging.width, staging.height, staging.pts);

	SRB2_ASSERT(frame != nullptr);

	const VideoFrame::Buffer& buffer = frame->rgba_buffer();

	const uint8_t* s = staging.screen.data();
	uint8_t* p = buffer.plane.data();

	// Convert from RGB8 to RGBA8
	for (int y = 0; y < frame->height(); ++y)
	{
		for (int x = 0; x < frame->width(); ++x)
		{
			p[x * 4] = s[x * 3];
			p[x * 4 + 1] = s[x * 3 + 1];
			p[x * 4 + 2] = s[x * 3 + 2];
			p[x * 4 + 3] = 255;
		}

		s += staging.width * 3;
		p += buffer.row_stride;
	}

	return frame;
}

AVRecorder::StagingVideoFrame::instance_t AVRecorder::new_staging_video_frame(uint32_t width, uint32_t height)
{
	std::optional<int> pts = impl_->advance_video_pts();

	if (!pts)
	{
		return nullptr;
	}

	return std::make_unique<StagingVideoFrame>(width, height, *pts);
}

void AVRecorder::push_staging_video_frame(StagingVideoFrame::instance_t frame)
{
	auto _ = impl_->queue_guard();

	impl_->video_queue_.vec_.emplace_back(std::move(frame));
	impl_->wake_up_worker();
}
