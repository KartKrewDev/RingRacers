// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "screen_capture.hpp"

#include <tcb/span.hpp>

#include "../m_misc.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

ScreenshotPass::ScreenshotPass() = default;
ScreenshotPass::~ScreenshotPass() = default;

void ScreenshotPass::capture(Rhi& rhi)
{
	bool doing_screenshot = takescreenshot || moviemode != MM_OFF || g_takemapthumbnail != TMT_NO;

	if (!doing_screenshot)
	{
		return;
	}

	pixel_data_.clear();
	packed_data_.clear();
	// Pixel data must be in pack alignment (4) so a stride of non-multiple 4 must align to 4
	uint32_t stride = width_ * 3;
	uint32_t read_stride = ((stride + (kPixelRowPackAlignment - 1)) & ~(kPixelRowPackAlignment - 1));
	pixel_data_.resize(read_stride * height_); // 3 bytes per pixel for RGB8
	packed_data_.resize(stride * height_);

	tcb::span<std::byte> data_bytes = tcb::as_writable_bytes(tcb::span(pixel_data_));
	rhi.read_pixels({0, 0, width_, height_}, PixelFormat::kRGB8, data_bytes);

	for (uint32_t row = 0; row < height_; row++)
	{
		// Read the aligned data into unaligned linear memory, flipping the rows in the process.
		uint32_t pixel_data_row = (height_ - row) - 1;
		uint8_t* pixel_data_row_ptr = &pixel_data_[pixel_data_row * read_stride];
		std::move(pixel_data_row_ptr, pixel_data_row_ptr + stride, &packed_data_[row * stride]);
	}

	if (g_takemapthumbnail != TMT_NO)
	{
		M_SaveMapThumbnail(width_, height_, tcb::as_bytes(tcb::span(packed_data_)));
	}

	if (takescreenshot)
	{
		M_DoScreenShot(width_, height_, tcb::as_bytes(tcb::span(packed_data_)));
	}

	if (moviemode != MM_OFF)
	{
		M_SaveFrame(width_, height_, tcb::as_bytes(tcb::span(packed_data_)));
	}
}
