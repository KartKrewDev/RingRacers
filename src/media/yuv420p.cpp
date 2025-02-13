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
#include <cstdint>
#include <memory>

#include <libyuv/convert.h>
#include <libyuv/scale_argb.h>
#include <tcb/span.hpp>

#include "../cxxutil.hpp"
#include "yuv420p.hpp"

using namespace srb2::media;

YUV420pFrame::YUV420pFrame(int pts, Buffer y, Buffer u, Buffer v, const BufferRGBA& rgba)
	: VideoFrame(pts)
	, y_(y)
	, u_(u)
	, v_(v)
	, rgba_(&rgba)
{

}

YUV420pFrame::~YUV420pFrame() = default;

bool YUV420pFrame::BufferRGBA::resize(int width, int height)
{
	if (width == width_ && height == height_)
	{
		return false;
	}

	width_ = width;
	height_ = height;

	row_stride = width * 4;

	const std::size_t new_size = row_stride * height;

	// Overallocate since the vector's alignment can't be
	// easily controlled. This is not a significant waste.
	vec_.resize(new_size + (kAlignment - 1));

	void* p = vec_.data();
	std::size_t n = vec_.size();

	p = std::align(kAlignment, 1, p, n);
	SRB2_ASSERT(p != nullptr);

	plane = tcb::span<uint8_t>(reinterpret_cast<uint8_t*>(p), new_size);

	return true;
}

void YUV420pFrame::BufferRGBA::erase()
{
	std::fill(vec_.begin(), vec_.end(), 0);
}

void YUV420pFrame::BufferRGBA::release()
{
	if (!vec_.empty())
	{
		*this = {};
	}
}

const VideoFrame::Buffer& YUV420pFrame::rgba_buffer() const
{
	return *rgba_;
}

void YUV420pFrame::convert() const
{
	// ABGR = RGBA in memory
	libyuv::ABGRToI420(
		rgba_->plane.data(),
		rgba_->row_stride,
		y_.plane.data(),
		y_.row_stride,
		u_.plane.data(),
		u_.row_stride,
		v_.plane.data(),
		v_.row_stride,
		width(),
		height()
	);
}

void YUV420pFrame::scale(const BufferRGBA& scaled_rgba)
{
	int vw = scaled_rgba.width();
	int vh = scaled_rgba.height();

	uint8_t* p = scaled_rgba.plane.data();

	const float ru = width() / static_cast<float>(height());
	const float rs = vw / static_cast<float>(vh);

	// Maintain aspect ratio of unscaled. Fit inside scaled
	// aspect by centering image.

	if (rs > ru) // scaled is wider
	{
		vw = vh * ru;
		p += (scaled_rgba.width() - vw) / 2 * 4;
	}
	else
	{
		vh = vw / ru;
		p += (scaled_rgba.height() - vh) / 2 * scaled_rgba.row_stride;
	}

	// Curiously, this function doesn't care about channel order.
	libyuv::ARGBScale(
		rgba_->plane.data(),
		rgba_->row_stride,
		width(),
		height(),
		p,
		scaled_rgba.row_stride,
		vw,
		vh,
		libyuv::FilterMode::kFilterNone
	);

	rgba_ = &scaled_rgba;
}
