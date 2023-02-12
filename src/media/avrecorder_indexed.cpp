// RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
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

VideoFrame::instance_t Impl::convert_indexed_video_frame(const IndexedVideoFrame& indexed)
{
	VideoFrame::instance_t frame = video_encoder_->new_frame(indexed.width, indexed.height, indexed.pts);

	SRB2_ASSERT(frame != nullptr);

	const VideoFrame::Buffer& buffer = frame->rgba_buffer();

	const uint8_t* s = indexed.screen.data();
	uint8_t* p = buffer.plane.data();

	for (int y = 0; y < frame->height(); ++y)
	{
		for (int x = 0; x < frame->width(); ++x)
		{
			const RGBA_t& c = indexed.palette[s[x]];

			reinterpret_cast<uint32_t*>(p)[x] = c.rgba;
		}

		s += indexed.width;
		p += buffer.row_stride;
	}

	return frame;
}

AVRecorder::IndexedVideoFrame::instance_t AVRecorder::new_indexed_video_frame(uint32_t width, uint32_t height)
{
	std::optional<int> pts = impl_->advance_video_pts();

	if (!pts)
	{
		return nullptr;
	}

	return std::make_unique<IndexedVideoFrame>(width, height, *pts);
}

void AVRecorder::push_indexed_video_frame(IndexedVideoFrame::instance_t frame)
{
	auto _ = impl_->queue_guard();

	impl_->video_queue_.vec_.emplace_back(std::move(frame));
	impl_->wake_up_worker();
}
